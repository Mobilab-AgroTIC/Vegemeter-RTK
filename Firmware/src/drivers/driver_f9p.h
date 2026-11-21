#ifndef DRIVER_F9P_H
#define DRIVER_F9P_H

#include "common/messages.h"
#include <Arduino.h>
#include "SparkFun_u-blox_GNSS_Arduino_Library.h"



extern SFE_UBLOX_GNSS myGNSS;


// Initialisation du F9P
bool driver_f9p_init(HardwareSerial& serial, int txPin, int rxPin, int powerPin, uint32_t baudrate);

// État du driver (pour le bloc STATUS)
bool driver_f9p_is_initialized();
uint32_t driver_f9p_get_pvt_count();
const char* f9p_fix_type_name(uint8_t fixType);
const char* f9p_carr_soln_name(uint8_t carrSoln);

// Envoyer une trame RTCM au F9P
void driver_f9p_sendRTCM(const uint8_t* data, size_t length);


#endif // DRIVER_F9P_H
