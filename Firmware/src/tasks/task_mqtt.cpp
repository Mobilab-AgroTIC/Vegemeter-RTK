#include <AsyncMqttClient.h>
#include "common/messages.h"
#include "common/config.h"
#include "common/logger.h"
#include "drivers/driver_mqtt.h"
#include "task_mqtt.h"


extern QueueHandle_t QueuePublish;
extern AsyncMqttClient mqttClient;  // déjà configuré dans setup_wifi()

// Compteurs lus par la tâche Health (bloc STATUS)
static volatile uint32_t published_count = 0;
static volatile uint32_t heartbeat_count = 0;
static volatile uint32_t skipped_count   = 0;
static volatile uint32_t dropped_count   = 0;

uint32_t mqtt_get_published_count() { return published_count; }
uint32_t mqtt_get_heartbeat_count() { return heartbeat_count; }
uint32_t mqtt_get_skipped_count()   { return skipped_count; }
uint32_t mqtt_get_dropped_count()   { return dropped_count; }

void task_mqtt(void *pvParameters) {
    fused_message_t msg;
    bool link_down = false; // pour logger la coupure et le rétablissement une seule fois

    for (;;) {
        // Attendre un message à publier (bloquant jusqu'à réception)
        if (xQueueReceive(QueuePublish, &msg, portMAX_DELAY) != pdPASS) continue;

        switch (prepare_and_send_mqtt_message(msg)) {
            case MQTT_SEND_OK:
                published_count++;
                if (msg.msg_type == MSG_TYPE_HEARTBEAT) heartbeat_count++;
                if (link_down) {
                    LOG_INFO("[MQTT] Publication rétablie (%u messages encore en attente)",
                             (unsigned)uxQueueMessagesWaiting(QueuePublish));
                    link_down = false;
                }
                break;

            case MQTT_SEND_SKIPPED:
                skipped_count++;
                break;

            case MQTT_SEND_DROPPED:
                dropped_count++;
                break;

            case MQTT_SEND_RETRY:
                link_down = true;
                // Réinsérer le message en tête de la queue pour une nouvelle tentative
                if (xQueueSendToFront(QueuePublish, &msg, pdMS_TO_TICKS(100)) != pdPASS) {
                    dropped_count++;
                    LOG_ERROR("[MQTT] QueuePublish pleine : message #%lu perdu", (unsigned long)msg.message_id);
                } else {
                    LOG_THROTTLED(LOG_WARN, LOG_THROTTLE_MS,
                                  "[MQTT] Broker non joignable : %u messages en attente d'envoi",
                                  (unsigned)uxQueueMessagesWaiting(QueuePublish));
                }
                // Attendre avant la prochaine tentative pour éviter de boucler trop vite
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;
        }
    }
}

void startTaskMQTT(int priority, int core) {
    xTaskCreatePinnedToCore(task_mqtt, "MQTT", 8192, NULL, priority, NULL,core);
}
