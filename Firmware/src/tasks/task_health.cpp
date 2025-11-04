#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "Arduino.h"

extern QueueHandle_t QueueSpectro;
extern QueueHandle_t QueuePublish;
extern QueueHandle_t QueueGNSS;

void task_health(void *pvParameters) {
    while (1) {
        // Queues
        Serial.printf("[Health] GNSS Queue: %u / %u free\n",
                      uxQueueMessagesWaiting(QueueGNSS),
                      uxQueueSpacesAvailable(QueueGNSS));

        Serial.printf("[Health] Spectro Queue: %u / %u free\n",
                      uxQueueMessagesWaiting(QueueSpectro),
                      uxQueueSpacesAvailable(QueueSpectro));

        Serial.printf("[Health] Publish Queue: %u / %u free\n",
                      uxQueueMessagesWaiting(QueuePublish),
                      uxQueueSpacesAvailable(QueuePublish));

        // Mémoire
        Serial.printf("[Health] Free heap: %u bytes\n", xPortGetFreeHeapSize());

        vTaskDelay(pdMS_TO_TICKS(5000)); // toutes les 5 sec
    }
}
void startTaskHealth(){
    xTaskCreatePinnedToCore(task_health,"Health",4096,NULL,1,NULL,tskNO_AFFINITY);
}
