#include "driver_as7265x.h"
#include "common/config.h"
#include "common/messages.h"
#include "common/logger.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "HardwareSerial.h"
#include "Arduino.h"
#include "drivers/driver_button.h"

// Global variables, assuming they are defined in the main project file
extern QueueHandle_t QueueSpectro;
extern spectro_sample_t spectro_msg;

static HardwareSerial* as7265_serial = nullptr;

// ------------------------------------------------------------
// Temporisations dérivées du temps d'intégration
// (datasheet AS7265x v1-04, Figure 59 :
//   ATINTTIME : 1-255, durée = valeur × ~2.8 ms, défaut 20
//   ATTCSMD   : défaut 2 = bank0 + bank1, soit 2 périodes d'intégration)
// ------------------------------------------------------------
static_assert(SPECTRO_INTEGRATION_TIME_LOOP >= 1 && SPECTRO_INTEGRATION_TIME_LOOP <= 255,
              "SPECTRO_INTEGRATION_TIME_LOOP doit être compris entre 1 et 255 (ATINTTIME)");
static constexpr int SPECTRO_INTEGRATION_TIME_MS = (SPECTRO_INTEGRATION_TIME_LOOP * 28 + 9) / 10;
static constexpr int SPECTRO_MEASUREMENT_MS      = 2 * SPECTRO_INTEGRATION_TIME_MS;
// Timeout de réponse à ATCDATA : une mesure complète + marge, au moins 500 ms
static constexpr uint32_t SPECTRO_RESPONSE_TIMEOUT_MS =
    (SPECTRO_MEASUREMENT_MS + 200 > 500) ? (SPECTRO_MEASUREMENT_MS + 200) : 500;
// Timeout des commandes de contrôle (AT, ATINTTIME)
static constexpr uint32_t SPECTRO_AT_TIMEOUT_MS = 500;

// État du capteur (lu par la tâche Health pour le bloc STATUS)
static as7265x_status_t status = {};

// ============================================================
// Suivi des échecs : un seul ERROR à l'apparition du problème,
// puis un rappel limité en fréquence, puis un INFO au retour.
// ============================================================
static void report_failure(const char* reason) {
    status.consecutive_failures++;
    if (status.consecutive_failures == 1) {
        LOG_ERROR("[AS7265X] %s", reason);
    } else {
        LOG_THROTTLED(LOG_WARN, LOG_THROTTLE_MS,
                      "[AS7265X] Toujours en échec : %s (%lu échecs consécutifs)",
                      reason, (unsigned long)status.consecutive_failures);
    }
}

static void report_success() {
    if (status.consecutive_failures > 0) {
        LOG_INFO("[AS7265X] Capteur de nouveau opérationnel après %lu échecs",
                 (unsigned long)status.consecutive_failures);
    }
    status.consecutive_failures = 0;
    status.success_count++;
    status.last_success_ms = millis();
}

as7265x_status_t driver_as7265x_get_status() {
    return status;
}

// ============================================================
// Commandes AT
// ============================================================

// Vide le buffer de réception : une réponse arrivée en retard (après un
// timeout) ne doit pas être lue comme la réponse à la commande suivante.
static void flush_rx() {
    size_t n = 0;
    while (as7265_serial->available()) {
        as7265_serial->read();
        n++;
    }
    if (n > 0) {
        LOG_DEBUG("[AS7265X] %u octets obsolètes purgés du buffer RX", (unsigned)n);
    }
}

// Envoie une commande AT et lit la réponse jusqu'à "OK" ou "ERROR" (ou timeout).
// Retourne true si la réponse contient "OK" sans "ERROR".
static bool send_at_command(const char* cmd, String& response, uint32_t timeout_ms) {
    flush_rx();
    as7265_serial->println(cmd);

    response = "";
    uint32_t start = millis();
    while (millis() - start < timeout_ms) {
        while (as7265_serial->available()) {
            response += (char)as7265_serial->read();
        }
        if (response.indexOf("OK") >= 0 || response.indexOf("ERROR") >= 0) break;
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    response.trim();
    LOG_DEBUG("[AS7265X] %s -> \"%s\"", cmd, response.c_str());
    return response.indexOf("OK") >= 0 && response.indexOf("ERROR") < 0;
}

// Envoie la configuration au capteur (ATINTTIME).
// Les réglages AT ne sont pas conservés à la mise hors tension : on les
// renvoie à chaque démarrage, et dès que le capteur répond s'il était absent.
// `verbose` = false quand on retente en boucle (évite de spammer le moniteur).
static bool apply_configuration(bool verbose) {
    char cmd[24];
    String resp;

    snprintf(cmd, sizeof(cmd), "ATINTTIME=%d", SPECTRO_INTEGRATION_TIME_LOOP);
    if (!send_at_command(cmd, resp, SPECTRO_AT_TIMEOUT_MS)) {
        if (verbose) {
            LOG_ERROR("[AS7265X] Commande %s refusée ou sans réponse (reçu : \"%s\")", cmd, resp.c_str());
        }
        status.config_ok = false;
        return false;
    }

    status.config_ok = true;
    LOG_INFO("[AS7265X] Temps d'intégration : %d cycles (~%d ms), mesure complète ~%d ms (ATTCSMD=2 : 2 banques)",
             SPECTRO_INTEGRATION_TIME_LOOP, SPECTRO_INTEGRATION_TIME_MS, SPECTRO_MEASUREMENT_MS);
    return true;
}

bool driver_as7265x_init(HardwareSerial& serial, int txPin, int rxPin, uint32_t baudrate) {
    as7265_serial = &serial;
    LOG_INFO("[AS7265X] Initialisation (UART %lu bauds, RX=%d, TX=%d)...",
             (unsigned long)baudrate, rxPin, txPin);
    serial.begin(baudrate, SERIAL_8N1, rxPin, txPin);
    serial.setTimeout(200);

    // Test de communication
    String response;
    if (!send_at_command("AT", response, SPECTRO_AT_TIMEOUT_MS)) {
        status.init_ok = false;
        LOG_ERROR("[AS7265X] Pas de réponse à la commande AT (reçu : \"%s\") : vérifier alimentation et câblage",
                  response.c_str());
        return false;
    }
    status.init_ok = true;
    LOG_INFO("[AS7265X] Capteur détecté (réponse AT OK)");

    // Configuration
    return apply_configuration(true);
}


bool driver_as7265x_get_data() {
    // Capteur absent au démarrage puis apparu (ou configuration refusée) :
    // on (re)tente la configuration avant de lire des données.
    if (!status.config_ok) {
        if (!apply_configuration(false)) {
            report_failure("Pas de réponse du capteur (configuration ATINTTIME impossible)");
            return false;
        }
    }

    LOG_DEBUG("[AS7265X] Envoi commande ATCDATA");
    flush_rx();
    as7265_serial->println("ATCDATA");
    unsigned long startTime = millis();

    // Wait for data with a timeout
    while (!as7265_serial->available()) {
        if (millis() - startTime > SPECTRO_RESPONSE_TIMEOUT_MS) {
            report_failure("Pas de réponse du capteur à ATCDATA (timeout)");
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(5)); // Small delay to avoid busy waiting
    }
    String dataAS = as7265_serial->readStringUntil('\n');

    if (dataAS.length() == 0) {
        report_failure("Réponse vide");
        return false;
    }

    LOG_DEBUG("[AS7265X] Données brutes : %s", dataAS.c_str());

    // Clean up the string and parse the data
    dataAS.trim();
    dataAS.replace("OK", "");
    dataAS.replace(" ", "");

    // Parsing dans une variable locale : une trame incomplète
    // ne laisse pas de valeurs partielles dans spectro_msg.
    spectro_sample_t sample = {};
    float* bands[] = {
        &sample.R, &sample.S, &sample.T, &sample.U, &sample.V, &sample.W,
        &sample.G, &sample.H, &sample.I, &sample.J, &sample.K, &sample.L,
        &sample.A, &sample.B, &sample.C, &sample.D, &sample.E, &sample.F
    };
    const size_t nb_bands = sizeof(bands) / sizeof(bands[0]);

    size_t parsed = 0;
    char* token = strtok((char*)dataAS.c_str(), ",");
    while (token != NULL && parsed < nb_bands) {
        *bands[parsed++] = atof(token);
        token = strtok(NULL, ",");
    }

    if (parsed < nb_bands) {
        LOG_DEBUG("[AS7265X] Trame incomplète : %u/%u bandes", (unsigned)parsed, (unsigned)nb_bands);
        report_failure("Trame incomplète");
        return false;
    }

    sample.ts_ms = millis(); // Timestamp the sample
    sample.label = get_button_label();
    spectro_msg  = sample;
    report_success();

    // Send the parsed data to the queue
    if (xQueueSend(QueueSpectro, &spectro_msg, 0) == pdTRUE) {
        LOG_DEBUG("[AS7265X] Échantillon -> QueueSpectro (ts=%lu ms, A=%.2f)",
                  (unsigned long)spectro_msg.ts_ms, spectro_msg.A);
        return true;
    }

    LOG_THROTTLED(LOG_WARN, LOG_THROTTLE_MS, "[AS7265X] QueueSpectro pleine : échantillon perdu");
    return false;
}
