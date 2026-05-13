#include "driver_f9p.h"
#include "common/config.h"
#include "SparkFun_u-blox_GNSS_Arduino_Library.h"
#include "common/messages.h"
#include "common/logger.h"
#include "NTRIPClient.h"

//static HardwareSerial* f9pSerial = nullptr;
static int pinPower = -1;

SFE_UBLOX_GNSS myGNSS;

// Dernière mesure GNSS
gnss_snapshot_t gnss_msg;
extern QueueHandle_t QueueGNSS;
extern NTRIPClient ntripclient;
static HardwareSerial* f9p_serial = nullptr;


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
    xQueueSend(QueueGNSS, &gnss_msg, 0); // Le 0 indique un délai d'attente nul
    LOG_INFO("[F9P] Data successfully sent to QueueGNSS : timestamp : %lu", gnss_msg.ts_ms);
    LOG_INFO("[F9P] NAV-PVT → QueueGNSS ts=%lu fix=%u carrSoln=%u",
             gnss_msg.ts_ms, gnss_msg.fixtype, gnss_msg.carrsoln);

    // 2. Génération GGA à fréquence réduite
    static uint8_t gga_counter = 0;
    if (++gga_counter < GNSS_RATE_GGA) return;
    gga_counter = 0;

    char gga[130];
    if (generate_gga_from_pvt(ubxData, gga, sizeof(gga))) {
        ntripclient.setLastGGA(String(gga));
        LOG_INFO("[F9P] GGA → NTRIP : %s", gga);
    } else {
        LOG_WARN("[F9P] GGA non générée : pas de fix valide (fixType=%u)", ubxData->fixType);
    }
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
        pinMode(pinPower, OUTPUT);
        digitalWrite(pinPower, HIGH);
        delay(1500);
    }
    else {
        LOG_WARN("[F9P] Warning: no power pin defined");
    }

    // UART
    f9p_serial=&serial;
    serial.begin(baudrate, SERIAL_8N1, rxPin, txPin);
    delay(1000);

    if (!serial) {
        LOG_ERROR("[F9P] Error: cannot start serial");
        return false;
    }
    else {
        LOG_INFO("[F9P] Serial started");
    }

    if(myGNSS.begin(serial)){
        LOG_INFO("[F9P] GNSS detected");
    } else {
        LOG_ERROR("[F9P] Error: GNSS not detected");
        return false;
    }
    
    myGNSS.setUART1Output(COM_TYPE_UBX); // Active UBX + NMEA
    myGNSS.setNavigationFrequency(GNSS_NAV_RATE); // Set the navigation rate
    myGNSS.configureMessage(UBX_CLASS_RXM, UBX_RXM_RTCM, COM_PORT_UART1, 1);
    myGNSS.setAutoPVTcallbackPtr(&ubx_callback);
    //myGNSS.setAutoRELPOSNEDcallback(&rxm_rtcm_callback);
    myGNSS.saveConfiguration();
    delay(100);

    LOG_INFO("[F9P] GNSS init OK");
    return true;
}

