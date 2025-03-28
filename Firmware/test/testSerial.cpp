#include "SparkFun_AS7265X.h" //Click here to get the library: http://librarymanager/All#SparkFun_AS7265X
#include <Wire.h>
#include <WiFi.h>
#include "NTRIPClient.h"
#include <HardwareSerial.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> 

void setup() {

  Serial.begin(115200);

}

void loop() {
  Serial.println("Hello world!");
  delay(100);
}
