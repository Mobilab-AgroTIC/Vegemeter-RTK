#include "driver_f9p.h"
#include "common/config.h"
#include "SparkFun_u-blox_GNSS_Arduino_Library.h"
#include "common/messages.h"
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
    Serial.println("[F9P] Data successfully sent to QueueGNSS : timestamp : ");
}



// gga_callback ne fonctionne pas ou du moins, elle n'est jamais appelée
void gga_callback(NMEA_GGA_data_t *nmeaData) {
    // Convertir les octets NMEA en String
    String ggaMessage = "";
    for (int i = 0; i < nmeaData->length; i++) {
        ggaMessage += (char)nmeaData->nmea[i];
    }

    // Sauvegarde dans ton client NTRIP
    ntripclient.setLastGGA(ggaMessage);
    Serial.printf("[F9P] GGA received: %s\n", ggaMessage.c_str());
}
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
        Serial.println("[F9P] Warning: no power pin defined");
    }

    // UART
    f9p_serial=&serial;
    serial.begin(baudrate, SERIAL_8N1, rxPin, txPin);
    delay(1000);

    if (!serial) {
        Serial.println("[F9P] Error: cannot start serial");
        return false;
    }
    else {
        Serial.println("[F9P] Serial started");
    }

    if(myGNSS.begin(serial)){
        Serial.println("[F9P] GNSS detected");
    } else {
        Serial.println("[F9P] Error: GNSS not detected");
        return false;
    }
    
    //myGNSS.setUART1Output(COM_TYPE_UBX); //Set the UART port to output UBX only
    myGNSS.setUART1Output(COM_TYPE_UBX | COM_TYPE_NMEA); // Active UBX + NMEA
    myGNSS.disableNMEAMessage(UBX_NMEA_GLL, COM_PORT_UART1); // Several of these are on by default on ublox board so let's disable them
    myGNSS.disableNMEAMessage(UBX_NMEA_GSA, COM_PORT_UART1);
    myGNSS.disableNMEAMessage(UBX_NMEA_GSV, COM_PORT_UART1);
    myGNSS.disableNMEAMessage(UBX_NMEA_RMC, COM_PORT_UART1);
    myGNSS.disableNMEAMessage(UBX_NMEA_VTG, COM_PORT_UART1);
    myGNSS.enableNMEAMessage(UBX_NMEA_GGA, COM_PORT_UART1, GNSS_RATE_GGA);
    myGNSS.setNavigationFrequency(GNSS_NAV_RATE); // Set the navigation rate
    //myGNSS.setAutoPVT(true); // A voir si on le laisse
    myGNSS.setAutoPVTcallbackPtr(&ubx_callback);
    myGNSS.setNMEAGNGGAcallbackPtr(&gga_callback); //Ne fonctionne pas pour l'instant donc production d'un GGA factice à partir de UBX dans generate GGA
    myGNSS.saveConfiguration();
    delay(100);

    Serial.println("[F9P] GNSS init OK");
    return true;
}

// ============================================================
// Envoi d’une trame RTCM
// ============================================================
void driver_f9p_sendRTCM(const uint8_t* data, size_t length) {
    if (f9p_serial) {
        f9p_serial->write(data, length);
        f9p_serial->flush(); // attendre la fin
        Serial.printf("[F9P] RTCM sent (%d bytes)\n", length);
    }
    else {
        Serial.println("[F9P] Error sending RTCM : serial not started");
    }
}