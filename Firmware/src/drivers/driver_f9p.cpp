#include "driver_f9p.h"
#include "common/config.h"
#include "SparkFun_u-blox_GNSS_Arduino_Library.h"
#include "common/messages.h"
#include "common/logger.h"
#include "NTRIPClient.h"

// Valeur par défaut si le config.h local ne la définit pas encore
#ifndef GNSS_POWER_ON_DELAY_MS
#define GNSS_POWER_ON_DELAY_MS 1000
#endif

//static HardwareSerial* f9pSerial = nullptr;
static int pinPower = -1;

SFE_UBLOX_GNSS myGNSS;

// Dernière mesure GNSS
gnss_snapshot_t gnss_msg;
extern QueueHandle_t QueueGNSS;
extern NTRIPClient ntripclient;
static HardwareSerial* f9p_serial = nullptr;
static bool f9p_init_ok = false;
static volatile uint32_t pvt_count = 0; // nombre de NAV-PVT reçus depuis le démarrage


// ============================================================
// Utilitaire : calcul checksum NMEA (XOR entre $ et *)
// ============================================================
static uint8_t nmea_checksum(const char* sentence) {
    uint8_t cs = 0;
    for (const char* p = sentence + 1; *p && *p != '*'; p++) {
        cs ^= (uint8_t)*p;
    }
    return cs;
}

// ============================================================
// Génération d'une trame GGA depuis un paquet NAV-PVT
// Retourne true si la trame a pu être générée (fix valide)
// ============================================================
static bool generate_gga_from_pvt(UBX_NAV_PVT_data_t *ubxData, char* gga, size_t gga_size) {

    // Qualité fix au sens NMEA GGA
    uint8_t quality = 0;
    if      (ubxData->fixType >= 3 && ubxData->flags.bits.carrSoln == 2) quality = 4; // RTK fixe
    else if (ubxData->fixType >= 3 && ubxData->flags.bits.carrSoln == 1) quality = 5; // RTK flottant
    else if (ubxData->fixType >= 3)                                       quality = 1; // 3D GPS
    else if (ubxData->fixType == 2)                                       quality = 1; // 2D GPS

    if (quality == 0) return false;

    // Lat/lon UBX (1e-7 deg) → format NMEA DDMM.MMMMM
    double lat_deg = ubxData->lat * 1e-7;
    double lon_deg = ubxData->lon * 1e-7;
    int    lat_d   = (int)fabs(lat_deg);
    double lat_m   = (fabs(lat_deg) - lat_d) * 60.0;
    int    lon_d   = (int)fabs(lon_deg);
    double lon_m   = (fabs(lon_deg) - lon_d) * 60.0;
    char   lat_h   = (lat_deg >= 0.0) ? 'N' : 'S';
    char   lon_h   = (lon_deg >= 0.0) ? 'E' : 'W';

    double alt_m   = ubxData->hMSL * 1e-3;
    float  hdop    = fminf(ubxData->hAcc * 1e-3f, 99.9f);
    double utc_sec = ubxData->sec + (ubxData->iTOW % 1000) * 1e-3;

    char body[120];
    snprintf(body, sizeof(body),
        "$GPGGA,%02u%02u%06.3f,"
        "%02d%08.5f,%c,"
        "%03d%08.5f,%c,"
        "%u,%02u,%.1f,"
        "%.3f,M,%.3f,M,,",
        ubxData->hour, ubxData->min, utc_sec,
        lat_d, lat_m, lat_h,
        lon_d, lon_m, lon_h,
        quality, ubxData->numSV, hdop,
        alt_m, 0.0
    );

    snprintf(gga, gga_size, "%s*%02X", body, nmea_checksum(body));
    return true;
}

// ============================================================
// Callback NAV-PVT (automatique)
// ============================================================
void ubx_callback(UBX_NAV_PVT_data_t *ubxData) {
    gnss_msg.lat  = ubxData->lat;       // degrés
    gnss_msg.lon = ubxData->lon;       // degrés
    gnss_msg.hMSL  = ubxData->hMSL;   // m
    gnss_msg.fixtype   = ubxData->fixType;
    gnss_msg.carrsoln  = ubxData->flags.bits.carrSoln;
    gnss_msg.num_sats  = ubxData->numSV;
    gnss_msg.hAcc      = ubxData->hAcc;   // mm
    gnss_msg.gSpeed    = ubxData->gSpeed;
    gnss_msg.year      = ubxData->year;
    gnss_msg.month     = ubxData->month;
    gnss_msg.day       = ubxData->day;
    gnss_msg.hour      = ubxData->hour;
    gnss_msg.min       = ubxData->min;
    gnss_msg.sec       = ubxData->sec;
    gnss_msg.ms        = ubxData->iTOW % 1000;
    gnss_msg.ts_ms     = millis();
    pvt_count++;

    if (xQueueSend(QueueGNSS, &gnss_msg, 0) == pdTRUE) {
        LOG_DEBUG("[F9P] NAV-PVT -> QueueGNSS ts=%lu fix=%u carrSoln=%u sats=%u",
                  (unsigned long)gnss_msg.ts_ms, gnss_msg.fixtype, gnss_msg.carrsoln, gnss_msg.num_sats);
    } else {
        LOG_THROTTLED(LOG_WARN, LOG_THROTTLE_MS, "[F9P] QueueGNSS pleine : mesure perdue");
    }

    // 1. Changement de qualité de fix -> INFO (une ligne par changement seulement)
    static int last_fix = -1, last_carr = -1;
    if (gnss_msg.fixtype != last_fix || gnss_msg.carrsoln != last_carr) {
        LOG_INFO("[F9P] Fix : %s / RTK : %s (sats=%u, hAcc=%lu mm)",
                 f9p_fix_type_name(gnss_msg.fixtype), f9p_carr_soln_name(gnss_msg.carrsoln),
                 gnss_msg.num_sats, (unsigned long)gnss_msg.hAcc);
        last_fix  = gnss_msg.fixtype;
        last_carr = gnss_msg.carrsoln;
    }

    // 2. Génération GGA à fréquence réduite
    static uint8_t gga_counter = 0;
    if (++gga_counter < GNSS_RATE_GGA) return;
    gga_counter = 0;

    static bool gga_ok = true; // pour ne prévenir qu'au changement d'état
    char gga[130];
    if (generate_gga_from_pvt(ubxData, gga, sizeof(gga))) {
        ntripclient.setLastGGA(String(gga));
        LOG_DEBUG("[F9P] GGA -> NTRIP : %s", gga);
        if (!gga_ok) LOG_INFO("[F9P] Fix valide : envoi de la position GGA au caster NTRIP");
        gga_ok = true;
    } else {
        if (gga_ok) LOG_WARN("[F9P] Pas de fix GNSS valide (fixType=%u) : GGA non envoyée au caster NTRIP", ubxData->fixType);
        gga_ok = false;
    }
}

const char* f9p_fix_type_name(uint8_t fixType) {
    switch (fixType) {
        case 0:  return "aucun";
        case 1:  return "dead reckoning";
        case 2:  return "2D";
        case 3:  return "3D";
        case 4:  return "GNSS+DR";
        case 5:  return "temps seul";
        default: return "?";
    }
}

const char* f9p_carr_soln_name(uint8_t carrSoln) {
    switch (carrSoln) {
        case 0:  return "aucun";
        case 1:  return "float";
        case 2:  return "fixed";
        default: return "?";
    }
}

bool driver_f9p_is_initialized() {
    return f9p_init_ok;
}

uint32_t driver_f9p_get_pvt_count() {
    return pvt_count;
}

// ============================================================
// Callback RTCM (automatique)
// ============================================================
/* void rxm_rtcm_callback(UBX_NAV_RELPOSNED_data_t msg) {
    // msg->refStationId contient l'ID de la base (0 à 4095)
    LOG_INFO("[F9P] RTCM Received -: %d", msg.refStationId);
        
    // Optionnel : stocker l'ID dans une variable globale ou l'envoyer à votre log système
    //gnss_msg.baseStationId = msg->refStationId; 

} */

// ============================================================
// Initialisation du F9P
// ============================================================
bool driver_f9p_init(HardwareSerial& serial, int txPin, int rxPin, int powerPin, uint32_t baudrate) {
    pinPower = powerPin;

    // Alimentation F9P
    if (pinPower >= 0) {
        // Démarrage différé : sur batterie, l'appel de courant du F9P au boot
        // fait couper l'AXP2101. On laisse le rail se stabiliser avant de l'alimenter.
        pinMode(pinPower, OUTPUT);
        digitalWrite(pinPower, LOW);
        delay(GNSS_POWER_ON_DELAY_MS);
        LOG_DEBUG("[F9P] Mise sous tension après %d ms", GNSS_POWER_ON_DELAY_MS);
        digitalWrite(pinPower, HIGH);
        delay(1500);
    }
    else {
        LOG_WARN("[F9P] Pas de broche d'alimentation définie");
    }

    // UART
    f9p_serial=&serial;
    serial.begin(baudrate, SERIAL_8N1, rxPin, txPin);
    delay(1000);

    if (!serial) {
        LOG_ERROR("[F9P] Impossible de démarrer l'UART");
        return false;
    }
    LOG_DEBUG("[F9P] UART démarré (%lu bauds, RX=%d, TX=%d)", (unsigned long)baudrate, rxPin, txPin);

    if (!myGNSS.begin(serial)) {
        LOG_ERROR("[F9P] Récepteur GNSS non détecté : vérifier alimentation, câblage et baudrate");
        return false;
    }
    LOG_DEBUG("[F9P] Récepteur GNSS détecté");
    
    myGNSS.setUART1Output(COM_TYPE_UBX); // Active UBX + NMEA
    myGNSS.setNavigationFrequency(GNSS_NAV_RATE); // Set the navigation rate
    myGNSS.configureMessage(UBX_CLASS_RXM, UBX_RXM_RTCM, COM_PORT_UART1, 1);
    myGNSS.setAutoPVTcallbackPtr(&ubx_callback);
    //myGNSS.setAutoRELPOSNEDcallback(&rxm_rtcm_callback);
    myGNSS.saveConfiguration();
    delay(100);

    f9p_init_ok = true;
    LOG_INFO("[F9P] Initialisation OK (navigation %d Hz)", GNSS_NAV_RATE);
    return true;
}

