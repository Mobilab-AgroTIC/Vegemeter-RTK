#include <AsyncMqttClient.h>
#include "common/messages.h" // ta struct fused_message_t
#include "common/config.h"
#include "config.h"
#include "drivers/driver_mqtt_ntrip.h"


extern QueueHandle_t QueuePublish;
extern AsyncMqttClient mqttClient;  // déjà configuré dans setup_wifi()
//extern const char* mqtt_output;
//extern const char* matUuid;

void task_mqtt(void *pvParameters) {
    fused_message_t msg;
    
    for (;;) {
        // Attendre un message à publier (bloquant jusqu'à réception)
        if (xQueueReceive(QueuePublish, &msg, portMAX_DELAY) == pdPASS) {
            prepare_and_send_mqtt_message(msg);
        }
    }
    // L'AsyncMqttClient n'a pas besoin de .loop(), donc on n'en met pas.
    // On pourrait ajouter un petit vTaskDelay(1) pour garantir la bonne préemption,
    // mais portMAX_DELAY suffit si la seule activité est la réception de queue.
}

void startTaskMQTT(int priority, int core) {
    xTaskCreatePinnedToCore(task_mqtt, "MQTT", 8192, NULL, priority, NULL,core); 
}