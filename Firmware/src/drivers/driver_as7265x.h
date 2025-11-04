#include "Arduino.h"
#include "common/messages.h"

bool driver_as7265x_init(HardwareSerial& serial, int txPin, int rxPin, uint32_t baudrate);

bool driver_as7265x_get_data();