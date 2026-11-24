#include "task_spectro.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "common/messages.h"
#include "common/config.h"
#include "common/logger.h"
#include "drivers/driver_as7265x.h"
#include "drivers/driver_button.h"

extern QueueHandle_t QueueSpectro;
extern QueueHandle_t QueueSpectroIncident;

// Dernier échantillon valide de chaque capteur (lu par la tâche Health)
spectro_sample_t spectro_msg;
spectro_sample_t spectro_incident_msg;

// Mesure, étiquette et envoi dans la queue de fusion
static void acquire(spectro_source_t id, spectro_sample_t& last, QueueHandle_t queue,
                    const char* queue_name) {
    spectro_sample_t sample;
    if (!driver_as7265x_measure(id, sample)) return; // erreur déjà loggée par le driver

    sample.label = get_button_label();
    last = sample;

    if (xQueueSend(queue, &sample, 0) == pdTRUE) {
        LOG_DEBUG("[SPECTRO] Échantillon %s -> %s (ts=%lu ms, A=%.2f)",
                  id == SPECTRO_INCIDENT ? "incident" : "réfléchi", queue_name,
                  (unsigned long)sample.ts_ms, sample.A);
    } else {
        LOG_THROTTLED(LOG_WARN, LOG_THROTTLE_MS, "[SPECTRO] %s pleine : échantillon perdu", queue_name);
    }
}

// Capteur réfléchi : acquisition en continu
void task_spectro(void *pvParameters) {
    while (1) {
        acquire(SPECTRO_REFLECTED, spectro_msg, QueueSpectro, "QueueSpectro");
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Capteur incident : une mesure toutes les SPECTRO_INCIDENT_PERIOD_MS.
// Bus I2C distinct du capteur réfléchi : les deux tâches ne se bloquent pas.
void task_spectro_incident(void *pvParameters) {
    TickType_t last_wake = xTaskGetTickCount();
    while (1) {
        acquire(SPECTRO_INCIDENT, spectro_incident_msg, QueueSpectroIncident, "QueueSpectroIncident");
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SPECTRO_INCIDENT_PERIOD_MS));
    }
}

void startTaskSpectro(int priority, int core) {
    xTaskCreatePinnedToCore(task_spectro, "SPECTRO", 4096, NULL, priority, NULL, core);
}

void startTaskSpectroIncident(int priority, int core) {
    xTaskCreatePinnedToCore(task_spectro_incident, "SPECTRO_INC", 4096, NULL, priority, NULL, core);
}
