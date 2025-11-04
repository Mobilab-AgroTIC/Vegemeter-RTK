#include "drivers/NTRIPClient.h"
#pragma once



void startTaskNTRIP(int priority, int core);

void taskNTRIP(void *pvParameters);