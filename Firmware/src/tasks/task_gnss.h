#include "SparkFun_u-blox_GNSS_Arduino_Library.h"
#pragma once


extern SFE_UBLOX_GNSS myGNSS;

void startTaskGNSS(int priority, int core);

void taskGNSS(void *pvParameters);