#include "task_fusion.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "common/messages.h"
#include "common/config.h"
#include "common/logger.h"
#include "common/logger.h"
#include "Arduino.h"
#include <string.h>
#include "drivers/driver_button.h"

extern QueueHandle_t QueueGNSS;
extern QueueHandle_t QueueSpectro;
extern QueueHandle_t QueuePublish;

extern spectro_sample_t spectro_msg;
extern gnss_snapshot_t gnss_msg;


// Remplit les champs communs d'un message à publier
static void fill_common_fields(fused_message_t &msg, msg_type_t type, uint32_t &messageid) {
    strncpy(msg.sensor_id, BBSOCOUL_ID, sizeof(msg.sensor_id) - 1);
    msg.sensor_id[sizeof(msg.sensor_id) - 1] = '\0';
    msg.msg_type   = type;
    msg.ts_ms      = millis();
    msg.message_id = messageid++;
}

static bool push_to_publish_queue(const fused_message_t &msg) {
    if (xQueueSend(QueuePublish, &msg, 0) != pdPASS) {
        LOG_THROTTLED(LOG_WARN, LOG_THROTTLE_MS, "[Fusion] QueuePublish pleine : message #%lu perdu",
                      (unsigned long)msg.message_id);
        return false;
    }
    return true;
}

// Log (INFO) uniquement lors du passage fusion <-> heartbeat
static void set_heartbeat_mode(bool &current, bool heartbeat, bool gnss, bool spectro) {
    if (current == heartbeat) return;
    current = heartbeat;
    if (heartbeat) {
        LOG_INFO("[Fusion] Pas de couple GNSS+spectro depuis %d ms : passage en heartbeat (GNSS %s, spectro %s)",
                 HEARTBEAT_PERIOD_MS, gnss ? "OK" : "absent", spectro ? "OK" : "absent");
    } else {
        LOG_INFO("[Fusion] Données GNSS + spectro disponibles : reprise de la fusion");
    }
}

void task_fusion(void *pvParameters) {
    // Suggestion de fonction par Claude en utilisant des while pour vider les queues et ainsi garder uniquement le message le plus récent de chaque capteur, puis faire une fusion basée sur les timestamps pour trouver les messages les plus proches temporellement. Cela permet d'avoir une fusion plus précise même si les messages n'arrivent pas exactement en même temps.

    gnss_snapshot_t    last_gnss    = {};
    spectro_sample_t   last_spectro = {};
    bool gnss_ready    = false;
    bool spectro_ready = false;
    uint32_t messageid = 0;
    uint32_t last_publish_ms = millis(); // dernier message (fusion ou heartbeat) poussé dans QueuePublish
    bool heartbeat_mode = false;

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
#if HEARTBEAT_ENABLE
            // --- Heartbeat : aucune fusion possible depuis HEARTBEAT_PERIOD_MS ---
            // On envoie quand même un message avec les données disponibles
            // (ou des zéros) pour pouvoir tester la transmission MQTT.
            if (millis() - last_publish_ms >= HEARTBEAT_PERIOD_MS) {
                fused_message_t hb = {};
                fill_common_fields(hb, MSG_TYPE_HEARTBEAT, messageid);
                hb.gnss_valid    = gnss_ready;
                hb.spectro_valid = spectro_ready;
                if (gnss_ready)    hb.gnss    = last_gnss;
                if (spectro_ready) hb.spectro = last_spectro;
                hb.delta_time_gnss_spectro_ms = (gnss_ready && spectro_ready)
                    ? (int16_t)(last_spectro.ts_ms - last_gnss.ts_ms) : 0;

                set_heartbeat_mode(heartbeat_mode, true, gnss_ready, spectro_ready);
                if (push_to_publish_queue(hb)) {
                    LOG_DEBUG("[Fusion] Heartbeat #%lu (gnss=%d, spectro=%d)",
                              (unsigned long)hb.message_id, gnss_ready, spectro_ready);
                }
                last_publish_ms = millis();
                gnss_ready    = false;
                spectro_ready = false;
            }
#endif
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
        fused_message_t fused = {};
        fill_common_fields(fused, MSG_TYPE_FUSED, messageid);
        fused.gnss_valid                 = true;
        fused.spectro_valid              = true;
        fused.gnss                       = last_gnss;
        fused.spectro                    = last_spectro;
        fused.delta_time_gnss_spectro_ms = (int16_t)(last_spectro.ts_ms - last_gnss.ts_ms);

        set_heartbeat_mode(heartbeat_mode, false, true, true);
        if (push_to_publish_queue(fused)) {
            LOG_DEBUG("[Fusion] Message #%lu fusionné, Δ GNSS-spectro = %d ms",
                      (unsigned long)fused.message_id, fused.delta_time_gnss_spectro_ms);
        }
        last_publish_ms = millis();

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

            strncpy(fused.sensor_id, BBSOCOUL_ID, sizeof(fused.sensor_id) - 1);
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
