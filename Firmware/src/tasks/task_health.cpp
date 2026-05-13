#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "Arduino.h"
#include "common/logger.h"
#include "common/messages.h"


extern QueueHandle_t QueueSpectro;
extern QueueHandle_t QueuePublish;
extern QueueHandle_t QueueGNSS;
extern gnss_snapshot_t gnss_msg;
extern spectro_sample_t spectro_msg;

void task_health(void *pvParameters) {
    while (1) {
        // Queues
        LOG_HEALTH("[Health] GNSS Queue: %u / %u free", uxQueueMessagesWaiting(QueueGNSS), uxQueueSpacesAvailable(QueueGNSS));

        LOG_HEALTH("[Health] Spectro Queue: %u / %u free", uxQueueMessagesWaiting(QueueSpectro), uxQueueSpacesAvailable(QueueSpectro));

        LOG_HEALTH("[Health] Publish Queue: %u / %u free", uxQueueMessagesWaiting(QueuePublish), uxQueueSpacesAvailable(QueuePublish));

        // Mémoire
        LOG_HEALTH("[Health] Free heap: %u bytes", xPortGetFreeHeapSize());

        // GNSS data
        LOG_HEALTH("[Health] GNSS hAcc: %u mm, Sats: %u, Fix: %u, CarrSoln: %u", 
                   gnss_msg.hAcc, gnss_msg.num_sats, gnss_msg.fixtype, gnss_msg.carrsoln);

        // Sepctro data        // On pourrait ajouter des infos sur le dernier échantillon spectro reçu, par exemple la
        LOG_HEALTH("[Health] Spectro last sample: ts=%lu ms, label=%u, R=%.2f, S=%.2f, T=%.2f", 
                   spectro_msg.ts_ms, spectro_msg.label, spectro_msg.R, spectro_msg.S, spectro_msg.T);



        vTaskDelay(pdMS_TO_TICKS(5000)); // toutes les 5 sec
    }
}
void startTaskHealth(){
    xTaskCreatePinnedToCore(task_health,"Health",4096,NULL,1,NULL,tskNO_AFFINITY);
}
