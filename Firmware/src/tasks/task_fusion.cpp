#include "task_fusion.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "common/messages.h"
#include "common/config.h"
#include "Arduino.h"
#include "drivers/driver_button.h"

extern QueueHandle_t QueueGNSS;
extern QueueHandle_t QueueSpectro;
extern QueueHandle_t QueuePublish;

extern spectro_sample_t spectro_msg;
extern gnss_snapshot_t gnss_msg;

void task_fusion(void *pvParameters) {
    //gnss_snapshot_t gnss_msg;
    //spectro_sample_t spectro_msg;

    // Buffers internes
    gnss_snapshot_t last_gnss;
    spectro_sample_t last_spectro;
    bool gnss_ready = false;
    bool spectro_ready = false;
    uint32_t messageid = 0;

    for (;;) {
        // Lire un message GNSS ou Spectro (selon ce qui arrive)
        if (xQueueReceive(QueueGNSS, &gnss_msg, 10 / portTICK_PERIOD_MS) == pdPASS) {
            last_gnss = gnss_msg;
            gnss_ready = true;
        }

        if (xQueueReceive(QueueSpectro, &spectro_msg, 0) == pdPASS) {
            last_spectro = spectro_msg;
            spectro_ready = true;
        }

        // Fusion seulement si on a les deux
        if (gnss_ready && spectro_ready) {
            // Vérifier si le spectro est postérieur au GNSS
            if (last_spectro.ts_ms >= last_gnss.ts_ms) {
                fused_message_t fused;
                
                //fused.ts_ms_gnss=last_gnss.ts_ms;
                //fused.ts_ms_spectro=last_gnss.ts_ms;
                fused.ts_ms = millis(); // timestamp global = spectro
                fused.gnss = last_gnss;
                fused.spectro = last_spectro;

                fused.delta_time_gnss_spectro_ms = 
                    (uint16_t)(last_spectro.ts_ms - last_gnss.ts_ms);

                fused.sensor_id = BBSOCOUL_ID;
                fused.message_id = messageid;
                messageid += 1; 

                // Envoi dans QueuePublish
                if (xQueueSend(QueuePublish, &fused, 0) != pdPASS) {
                    Serial.println("[Fusion] Warning: QueuePublish pleine !");
                } else {
                    Serial.printf("[Fusion] Message fusionné GNSS+Spectro, Δ=%u ms\n",
                                  fused.delta_time_gnss_spectro_ms);
                }

                // On attend un nouveau GNSS pour la prochaine fusion
                gnss_ready = false;
            }
        }
    vTaskDelay(10);
    }
}



void startTaskFusion(int priority, int core) {
    xTaskCreatePinnedToCore(task_fusion, "FUSION", 8192, NULL, priority, NULL,core); 
}
