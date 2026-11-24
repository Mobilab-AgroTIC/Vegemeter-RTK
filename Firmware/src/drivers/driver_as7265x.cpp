#include "driver_as7265x.h"
#include "common/config.h"
#include "common/messages.h"
#include "common/logger.h"
#include "freertos/FreeRTOS.h"
#include "Arduino.h"
#include <string.h>

// ============================================================
// Protocole I2C de l'AS7265x (datasheet AS7265x v1-04, §I2C)
//
// Le capteur n'expose que 3 registres physiques. Les registres
// "virtuels" (configuration, données) sont lus/écrits un octet à la
// fois via WRITE_REG / READ_REG, en surveillant STATUS_REG :
//   - TX_VALID = 1 : le capteur n'a pas encore consommé l'octet écrit
//   - RX_VALID = 1 : un octet est disponible dans READ_REG
// ============================================================
static constexpr uint8_t AS7265X_ADDR    = 0x49;   // fixe, non modifiable

static constexpr uint8_t REG_STATUS      = 0x00;
static constexpr uint8_t REG_WRITE       = 0x01;
static constexpr uint8_t REG_READ        = 0x02;
static constexpr uint8_t STATUS_RX_VALID = 0x01;
static constexpr uint8_t STATUS_TX_VALID = 0x02;

// Registres virtuels
static constexpr uint8_t VREG_CONFIG     = 0x04;
static constexpr uint8_t VREG_INT_TIME   = 0x05;
static constexpr uint8_t VREG_CAL_FIRST  = 0x14;   // 6 floats de 4 octets : 0x14, 0x18 ... 0x28
static constexpr uint8_t VREG_DEV_SELECT = 0x4F;

// Bits de VREG_CONFIG
static constexpr uint8_t CFG_GAIN_SHIFT  = 4;      // bits 5:4
static constexpr uint8_t CFG_GAIN_MASK   = 0x30;
static constexpr uint8_t CFG_MODE_SHIFT  = 2;      // bits 3:2 (BANK)
static constexpr uint8_t CFG_MODE_MASK   = 0x0C;
static constexpr uint8_t CFG_DATA_RDY    = 0x02;
static constexpr uint8_t MODE_6CH_ONE_SHOT = 3;

// Bits de VREG_DEV_SELECT : présence des deux esclaves (AS72652 VIS, AS72653 UV)
static constexpr uint8_t DEV_SLAVES_MASK = 0x30;

// Puces du Triad, dans l'ordre de lecture
static constexpr uint8_t DEV_NIR = 0x00;   // AS72651 (maître) : R S T U V W
static constexpr uint8_t DEV_VIS = 0x01;   // AS72652          : G H I J K L
static constexpr uint8_t DEV_UV  = 0x02;   // AS72653          : A B C D E F

// Timeout d'une opération sur registre virtuel (attente TX_VALID / RX_VALID)
static constexpr uint32_t VREG_TIMEOUT_MS = 20;
// Après N échecs consécutifs, on réinitialise le bus I2C (SDA bloquée, etc.)
static constexpr uint32_t BUS_RECOVERY_AFTER_FAILURES = 5;

// ------------------------------------------------------------
// Temporisations dérivées du temps d'intégration
// (datasheet AS7265x : INT_TIME 1-255, durée = valeur × ~2.8 ms ;
//  en mode 6 canaux, une mesure complète = 2 périodes d'intégration)
// ------------------------------------------------------------
static_assert(SPECTRO_INTEGRATION_TIME_LOOP >= 1 && SPECTRO_INTEGRATION_TIME_LOOP <= 255,
              "SPECTRO_INTEGRATION_TIME_LOOP doit être compris entre 1 et 255");
static_assert(SPECTRO_GAIN >= 0 && SPECTRO_GAIN <= 3,
              "SPECTRO_GAIN doit valoir 0 (1x), 1 (3.7x), 2 (16x) ou 3 (64x)");
static constexpr int SPECTRO_INTEGRATION_TIME_MS = (SPECTRO_INTEGRATION_TIME_LOOP * 28 + 9) / 10;
static constexpr int SPECTRO_MEASUREMENT_MS      = 2 * SPECTRO_INTEGRATION_TIME_MS;
// Attente de DATA_RDY : une mesure complète + marge, au moins 500 ms
static constexpr uint32_t SPECTRO_RESPONSE_TIMEOUT_MS =
    (SPECTRO_MEASUREMENT_MS + 200 > 500) ? (SPECTRO_MEASUREMENT_MS + 200) : 500;

static const char* gain_name(int g) {
    switch (g) {
        case 0: return "1x";
        case 1: return "3.7x";
        case 2: return "16x";
        default: return "64x";
    }
}

// Un capteur par bus : [SPECTRO_REFLECTED] sur Wire, [SPECTRO_INCIDENT] sur Wire1.
// Chaque instance n'est manipulée que par sa propre tâche d'acquisition.
static as7265x_t sensors[SPECTRO_COUNT] = {};

// ============================================================
// Suivi des échecs : un seul ERROR à l'apparition du problème,
// puis un rappel limité en fréquence, puis un INFO au retour.
// ============================================================
static void report_failure(as7265x_t& d, const char* reason) {
    as7265x_status_t& s = d.status;
    s.consecutive_failures++;
    if (s.consecutive_failures == 1) {
        LOG_ERROR("[%s] %s (%s)", d.name, reason, d.last_error ? d.last_error : "?");
    } else {
        LOG_THROTTLED(LOG_WARN, LOG_THROTTLE_MS,
                      "[%s] Toujours en échec : %s (%s, %lu échecs consécutifs)",
                      d.name, reason, d.last_error ? d.last_error : "?",
                      (unsigned long)s.consecutive_failures);
    }
    // Le capteur a pu redémarrer (perte des réglages) : on reconfigure au prochain tour
    s.config_ok = false;

    if (s.consecutive_failures % BUS_RECOVERY_AFTER_FAILURES == 0) {
        LOG_WARN("[%s] Réinitialisation du bus I2C", d.name);
        d.wire->end();
        d.wire->begin(d.sda, d.scl, d.freq);
    }
}

static void report_success(as7265x_t& d) {
    as7265x_status_t& s = d.status;
    if (s.consecutive_failures > 0) {
        LOG_INFO("[%s] Capteur de nouveau opérationnel après %lu échecs",
                 d.name, (unsigned long)s.consecutive_failures);
    }
    s.consecutive_failures = 0;
    s.success_count++;
    s.last_success_ms = millis();
}

as7265x_status_t driver_as7265x_get_status(spectro_source_t id) {
    return sensors[id].status;
}

// ============================================================
// Accès I2C bas niveau
// ============================================================

static bool phys_write(as7265x_t& d, uint8_t reg, uint8_t value) {
    d.wire->beginTransmission(AS7265X_ADDR);
    d.wire->write(reg);
    d.wire->write(value);
    if (d.wire->endTransmission() != 0) {
        d.last_error = "pas d'ACK en écriture";
        return false;
    }
    return true;
}

static bool phys_read(as7265x_t& d, uint8_t reg, uint8_t& value) {
    d.wire->beginTransmission(AS7265X_ADDR);
    d.wire->write(reg);
    if (d.wire->endTransmission() != 0) {
        d.last_error = "pas d'ACK en lecture";
        return false;
    }
    if (d.wire->requestFrom(AS7265X_ADDR, (uint8_t)1) != 1) {
        d.last_error = "lecture I2C vide";
        return false;
    }
    value = d.wire->read();
    return true;
}

// Attend que (STATUS & mask) soit à l'état voulu.
// Attente active courte (cas normal : quelques centaines de µs), puis on
// rend la main à FreeRTOS si le capteur tarde.
static bool wait_status(as7265x_t& d, uint8_t mask, bool want_set) {
    uint32_t start = millis();
    uint8_t st;
    while (true) {
        if (!phys_read(d, REG_STATUS, st)) return false;
        if (((st & mask) != 0) == want_set) return true;
        uint32_t elapsed = millis() - start;
        if (elapsed > VREG_TIMEOUT_MS) {
            d.last_error = (mask == STATUS_TX_VALID) ? "timeout TX_VALID" : "timeout RX_VALID";
            return false;
        }
        if (elapsed >= 2) vTaskDelay(1);
        else              delayMicroseconds(50);
    }
}

static bool vreg_write(as7265x_t& d, uint8_t vreg, uint8_t value) {
    if (!wait_status(d, STATUS_TX_VALID, false)) return false;
    if (!phys_write(d, REG_WRITE, vreg | 0x80)) return false;   // bit 7 = écriture
    if (!wait_status(d, STATUS_TX_VALID, false)) return false;
    return phys_write(d, REG_WRITE, value);
}

static bool vreg_read(as7265x_t& d, uint8_t vreg, uint8_t& value) {
    // Purge d'un éventuel octet resté dans READ_REG
    uint8_t st;
    if (!phys_read(d, REG_STATUS, st)) return false;
    if (st & STATUS_RX_VALID) {
        uint8_t dummy;
        if (!phys_read(d, REG_READ, dummy)) return false;
    }
    if (!wait_status(d, STATUS_TX_VALID, false)) return false;
    if (!phys_write(d, REG_WRITE, vreg)) return false;
    if (!wait_status(d, STATUS_RX_VALID, true)) return false;
    return phys_read(d, REG_READ, value);
}

// Float IEEE 754 big-endian sur 4 registres virtuels consécutifs
static bool vreg_read_float(as7265x_t& d, uint8_t vreg, float& out) {
    uint32_t raw = 0;
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t b;
        if (!vreg_read(d, vreg + i, b)) return false;
        raw = (raw << 8) | b;
    }
    memcpy(&out, &raw, sizeof(out));
    return true;
}

// ============================================================
// Fonctions capteur
// ============================================================

// ACK à 0x49 + présence des deux esclaves (VIS et UV) derrière le maître NIR
static bool detect(as7265x_t& d) {
    d.wire->beginTransmission(AS7265X_ADDR);
    if (d.wire->endTransmission() != 0) {
        d.last_error = "aucun ACK à l'adresse 0x49";
        return false;
    }
    uint8_t dev;
    if (!vreg_read(d, VREG_DEV_SELECT, dev)) return false;
    if ((dev & DEV_SLAVES_MASK) != DEV_SLAVES_MASK) {
        d.last_error = "esclaves AS72652/AS72653 non détectés";
        return false;
    }
    d.status.init_ok = true;
    return true;
}

// Les réglages ne sont pas conservés à la mise hors tension : on les
// renvoie au démarrage et après chaque échec.
// `verbose` = false quand on retente en boucle (évite de spammer le moniteur).
static bool apply_configuration(as7265x_t& d, bool verbose) {
    d.status.config_ok = false;
    if (!detect(d)) {
        if (verbose) LOG_ERROR("[%s] Capteur non détecté : %s", d.name, d.last_error);
        return false;
    }

    uint8_t cfg;
    bool ok = vreg_write(d, VREG_INT_TIME, SPECTRO_INTEGRATION_TIME_LOOP)
           && vreg_read(d, VREG_CONFIG, cfg);
    if (ok) {
        cfg = (cfg & ~CFG_GAIN_MASK) | (SPECTRO_GAIN << CFG_GAIN_SHIFT);
        ok = vreg_write(d, VREG_CONFIG, cfg);
    }
    if (!ok) {
        if (verbose) LOG_ERROR("[%s] Configuration refusée : %s", d.name, d.last_error);
        return false;
    }

    d.status.config_ok = true;
    if (verbose) {
        LOG_INFO("[%s] Intégration %d cycles (~%d ms), mesure complète ~%d ms, gain %s",
                 d.name, SPECTRO_INTEGRATION_TIME_LOOP, SPECTRO_INTEGRATION_TIME_MS,
                 SPECTRO_MEASUREMENT_MS, gain_name(SPECTRO_GAIN));
    }
    return true;
}

// Déclenche une mesure 6 canaux "one shot" sur les 3 puces.
static bool start_measurement(as7265x_t& d) {
    uint8_t cfg;
    if (!vreg_read(d, VREG_CONFIG, cfg)) return false;
    cfg &= ~CFG_DATA_RDY;
    cfg = (cfg & ~CFG_MODE_MASK) | (MODE_6CH_ONE_SHOT << CFG_MODE_SHIFT);
    return vreg_write(d, VREG_CONFIG, cfg);
}

static bool wait_data_ready(as7265x_t& d) {
    uint32_t start = millis();
    while (true) {
        uint8_t cfg;
        if (!vreg_read(d, VREG_CONFIG, cfg)) return false;
        if (cfg & CFG_DATA_RDY) return true;
        if (millis() - start > SPECTRO_RESPONSE_TIMEOUT_MS) {
            d.last_error = "DATA_RDY jamais levé";
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// Lit les 18 valeurs calibrées (float) dans l'ordre de spectro_sample_t
static bool read_calibrated(as7265x_t& d, spectro_sample_t& s) {
    struct { uint8_t dev; float* ch[6]; } chips[] = {
        { DEV_NIR, { &s.R, &s.S, &s.T, &s.U, &s.V, &s.W } },
        { DEV_VIS, { &s.G, &s.H, &s.I, &s.J, &s.K, &s.L } },
        { DEV_UV,  { &s.A, &s.B, &s.C, &s.D, &s.E, &s.F } },
    };
    for (auto& chip : chips) {
        if (!vreg_write(d, VREG_DEV_SELECT, chip.dev)) return false;
        for (uint8_t i = 0; i < 6; i++) {
            if (!vreg_read_float(d, VREG_CAL_FIRST + 4 * i, *chip.ch[i])) return false;
        }
    }
    return true;
}

// ============================================================
// API utilisée par les tâches
// ============================================================

bool driver_as7265x_init(spectro_source_t id, const char* name, TwoWire& wire,
                         int sdaPin, int sclPin, uint32_t freq) {
    as7265x_t& sensor = sensors[id];
    sensor.name = name;
    sensor.wire = &wire;
    sensor.sda  = sdaPin;
    sensor.scl  = sclPin;
    sensor.freq = freq;

    LOG_INFO("[%s] Initialisation (I2C %lu Hz, SDA=%d, SCL=%d)...",
             sensor.name, (unsigned long)freq, sdaPin, sclPin);
    if (!wire.begin(sdaPin, sclPin, freq)) {
        LOG_ERROR("[%s] Impossible de démarrer le bus I2C", sensor.name);
        return false;
    }
    wire.setTimeOut(50);

    if (!apply_configuration(sensor, true)) return false;
    LOG_INFO("[%s] Capteur détecté et configuré", sensor.name);
    return true;
}

bool driver_as7265x_measure(spectro_source_t id, spectro_sample_t& out) {
    as7265x_t& d = sensors[id];
    if (d.wire == nullptr) return false; // driver_as7265x_init() jamais appelé

    // Capteur absent au démarrage puis apparu (ou échec précédent) :
    // on (re)tente la détection et la configuration avant de mesurer.
    if (!d.status.config_ok) {
        if (!apply_configuration(d, false)) {
            report_failure(d, "Capteur non détecté ou configuration impossible");
            return false;
        }
    }

    uint32_t t_start = millis();
    if (!start_measurement(d)) {
        report_failure(d, "Déclenchement de la mesure impossible");
        return false;
    }
    if (!wait_data_ready(d)) {
        report_failure(d, "Pas de données (timeout mesure)");
        return false;
    }
    uint32_t t_ready = millis();

    // Parsing dans une variable locale : une lecture interrompue
    // ne laisse pas de valeurs partielles dans `out`.
    spectro_sample_t sample = {};
    if (!read_calibrated(d, sample)) {
        report_failure(d, "Lecture des valeurs calibrées interrompue");
        return false;
    }
    uint32_t t_end = millis();

    d.status.last_measure_ms = t_ready - t_start;
    d.status.last_read_ms    = t_end - t_ready;
    LOG_DEBUG("[%s] Mesure %lu ms + lecture I2C %lu ms",
              d.name, (unsigned long)d.status.last_measure_ms, (unsigned long)d.status.last_read_ms);

    sample.ts_ms  = t_ready; // horodatage = fin de l'intégration (données prêtes)
    sample.source = id;
    out = sample;
    report_success(d);
    return true;
}
