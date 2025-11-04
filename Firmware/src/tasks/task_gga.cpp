#include "Arduino.h"
#include "drivers/NTRIPClient.h"
#include "common/config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern NTRIPClient ntripclient;
extern HardwareSerial SerialF9P;

void task_gga(void *parameter) {
  Serial.println("[TASK] Démarrage de la tâche GGA...");

  for (;;) {
    String lastGGA = ntripclient.getLastGGA();
    Serial.print("[GGA] last GGA :");
    Serial.println(lastGGA);
    if (!lastGGA.isEmpty()) {
      Serial.println("[NTRIP] Envoi de la trame GGA au caster...");
      ntripclient.sendGGA(lastGGA.c_str(), NTRIP_HOST, NTRIP_PORT, NTRIP_USER, NTRIP_PASSWORD, NTRIP_MNTPNT);
      Serial.println("[NTRIP] Trame GGA envoyée : " + lastGGA);
    } else {
      Serial.println("[NTRIP] Aucune trame GGA disponible.");
    }

    vTaskDelay(pdMS_TO_TICKS(NTRIP_DELAY_SEND_GGA));  // par ex. toutes les 10 s
  }
}


void startTaskGGA(int priority, int core) {
    xTaskCreatePinnedToCore(task_gga, "GGA", 4096, NULL, priority, NULL,core); 
}
