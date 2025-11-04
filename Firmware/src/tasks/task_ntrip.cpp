#include "task_ntrip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "drivers/driver_mqtt_ntrip.h"

extern NTRIPClient ntripclient;
extern HardwareSerial SerialF9P;

void task_ntrip(void *pvParameters){
    while (1) {
        while (ntripclient.available()) {
            char ch = ntripclient.read();
            SerialF9P.write(ch); // envoi direct vers F9P
        }
        vTaskDelay(pdMS_TO_TICKS(100));  // intervalle de gestion léger
    }
}

void startTaskNTRIP(int priority, int core) {
    xTaskCreatePinnedToCore(task_ntrip, "NTRIP", 4096, NULL, priority, NULL,core); 
}