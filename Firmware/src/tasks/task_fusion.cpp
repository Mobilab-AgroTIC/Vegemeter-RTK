#include "task_fusion.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "common/messages.h"
#include "common/config.h"
#include "common/logger.h"
#include "common/logger.h"
#include "Arduino.h"
#include "drivers/driver_button.h"

extern QueueHandle_t QueueGNSS;
extern QueueHandle_t QueueSpectro;
extern QueueHandle_t QueuePublish;

extern spectro_sample_t spectro_msg;
extern gnss_snapshot_t gnss_msg;


void task_fusion(void *pvParameters) {
    // Suggestion de fonction par Claude en utilisant des while pour vider les queues et ainsi garder uniquement le message le plus récent de chaque capteur, puis faire une fusion basée sur les timestamps pour trouver les messages les plus proches temporellement. Cela permet d'avoir une fusion plus précise même si les messages n'arrivent pas exactement en même temps.

    gnss_snapshot_t    last_gnss;
    spectro_sample_t   last_spectro;
    bool gnss_ready    = false;
    bool spectro_ready = false;
    uint32_t messageid = 0;

    for (;;) {

        // --- Vidage des queues : on garde uniquement le message le plus récent ---
        gnss_snapshot_t tmp_gnss;
        while (xQueueReceive(QueueGNSS, &tmp_gnss, pdMS_TO_TICKS(10)) == pdPASS) {
            last_gnss  = tmp_gnss;
            gnss_ready = true;
        }

        spectro_sample_t tmp_spectro;
        while (xQueueReceive(QueueSpectro, &tmp_spectro, 0) == pdPASS) {
            last_spectro  = tmp_spectro;
            spectro_ready = true;
        }

        if (!gnss_ready || !spectro_ready) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        // --- Affinage : cherche le spectro le plus proche du GNSS ---
        spectro_sample_t candidate_spectro;
        while (xQueuePeek(QueueSpectro, &candidate_spectro, 0) == pdPASS) {
            int32_t delta_current   = abs((int32_t)(last_spectro.ts_ms    - last_gnss.ts_ms));
            int32_t delta_candidate = abs((int32_t)(candidate_spectro.ts_ms - last_gnss.ts_ms));
            if (delta_candidate < delta_current) {
                xQueueReceive(QueueSpectro, &last_spectro, 0);
            } else {
                break;
            }
        }

        // --- Affinage : cherche le GNSS le plus proche du spectro ---
        gnss_snapshot_t candidate_gnss;
        while (xQueuePeek(QueueGNSS, &candidate_gnss, 0) == pdPASS) {
            int32_t delta_current   = abs((int32_t)(last_spectro.ts_ms - last_gnss.ts_ms));
            int32_t delta_candidate = abs((int32_t)(last_spectro.ts_ms - candidate_gnss.ts_ms));
            if (delta_candidate < delta_current) {
                xQueueReceive(QueueGNSS, &last_gnss, 0);
            } else {
                break;
            }
        }

        // --- Fusion ---
        fused_message_t fused;
        fused.ts_ms                      = millis();
        fused.gnss                       = last_gnss;
        fused.spectro                    = last_spectro;
        fused.delta_time_gnss_spectro_ms = (int16_t)(last_spectro.ts_ms - last_gnss.ts_ms);
        fused.sensor_id                  = BBSOCOUL_ID;
        fused.message_id                 = messageid++;

        if (xQueueSend(QueuePublish, &fused, 0) != pdPASS) {
            LOG_WARN("[Fusion] QueuePublish pleine !");
        } else {
            LOG_INFO("[Fusion] Δ=%d ms", fused.delta_time_gnss_spectro_ms);
        }

        gnss_ready    = false;
        spectro_ready = false;

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void task_fusion_old(void *pvParameters) {
    //gnss_snapshot_t gnss_msg;
    //spectro_sample_t spectro_msg;

    // Buffers internes
    gnss_snapshot_t last_gnss;
    spectro_sample_t last_spectro;
    //bool gnss_ready = true; // test pour désactiver l'acquisition GNSS et vérifier si c'est cela qui bloque la fusion
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
            
            fused_message_t fused;
            
            fused.ts_ms = millis(); // timestamp global = spectro
            fused.gnss = last_gnss;
            fused.spectro = last_spectro;

            fused.delta_time_gnss_spectro_ms = 
                (int16_t)(last_spectro.ts_ms - last_gnss.ts_ms);

            fused.sensor_id = BBSOCOUL_ID;
            fused.message_id = messageid;
            messageid += 1; 

                // Envoi dans QueuePublish
                if (xQueueSend(QueuePublish, &fused, 0) != pdPASS) {
                    LOG_WARN("[Fusion] Warning: QueuePublish pleine !");
                } else {
                    LOG_INFO("[Fusion] Message fusionné GNSS+Spectro, Δ=%u ms", fused.delta_time_gnss_spectro_ms);
                }

            // On attend un nouveau GNSS pour la prochaine fusion
            gnss_ready = false; // force la lecture d'un nouveau message GNSS avant de refaire une fusion
            spectro_ready = false; // force la lecture d'un nouveau message spectro avant de refaire une fusion
            
           
        } 
    vTaskDelay(10);
    }
}



void startTaskFusion(int priority, int core) {
    xTaskCreatePinnedToCore(task_fusion, "FUSION", 8192, NULL, priority, NULL,core); 
}
