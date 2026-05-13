#include <AsyncMqttClient.h>
#include "common/messages.h"
#include "common/config.h"
#include "common/logger.h"
#include "drivers/driver_mqtt.h"


extern QueueHandle_t QueuePublish;
extern AsyncMqttClient mqttClient;  // déjà configuré dans setup_wifi()
//extern const char* mqtt_output;
//extern const char* matUuid;

void task_mqtt(void *pvParameters) {
    fused_message_t msg;
    static uint32_t publish_count = 0;
    
    for (;;) {
        // Attendre un message à publier (bloquant jusqu'à réception)
        if (xQueueReceive(QueuePublish, &msg, portMAX_DELAY) == pdPASS) {
            if (prepare_and_send_mqtt_message(msg)) {
                publish_count++;
                // Log tous les 50 messages
                if (publish_count % 50 == 0) {
                    LOG_HEALTH("[MQTT] Published: %u messages\n", publish_count);
                }
            } else {
                // Réinsérer le message en tête de la queue pour une nouvelle tentative
                if (xQueueSendToFront(QueuePublish, &msg, pdMS_TO_TICKS(100)) != pdPASS) {
                    LOG_WARN("[MQTT] Failed to requeue message %u\n", msg.message_id);
                } else {
                    LOG_WARN("[MQTT] Message %u requeued for retry\n", msg.message_id);
                }
                // Attendre avant la prochaine tentative pour éviter de boucler trop vite
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
    }
    // L'AsyncMqttClient n'a pas besoin de .loop(), donc on n'en met pas.
    // On pourrait ajouter un petit vTaskDelay(1) pour garantir la bonne préemption,
    // mais portMAX_DELAY suffit si la seule activité est la réception de queue.
}

void startTaskMQTT(int priority, int core) {
    xTaskCreatePinnedToCore(task_mqtt, "MQTT", 8192, NULL, priority, NULL,core); 
}