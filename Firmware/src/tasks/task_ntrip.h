#pragma once
#include <stdint.h>

void startTaskNTRIP(int priority, int core);

void taskNTRIP(void *pvParameters);

// État (bloc STATUS)
bool ntrip_is_connected();
uint32_t ntrip_get_rtcm_bytes();    // octets RTCM transmis au F9P depuis le démarrage
uint32_t ntrip_get_last_rtcm_ms();  // millis() de la dernière réception RTCM (0 = jamais)
