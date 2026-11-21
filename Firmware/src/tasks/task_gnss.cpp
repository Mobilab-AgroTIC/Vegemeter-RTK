#include "task_gnss.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "common/messages.h"
#include "common/logger.h"
#include "drivers/driver_f9p.h"
#include "common/config.h"

static void task_gnss(void *pvParameters) {
    (void) pvParameters;
    LOG_DEBUG("[GNSS] Tâche démarrée");
    
    while (1) {

        // Appel périodique à la librairie GNSS SparkFun (callbacks internes)
        myGNSS.checkUblox();
        myGNSS.checkCallbacks();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void startTaskGNSS(int priority, int core) {
    xTaskCreatePinnedToCore(task_gnss, "GNSS", 8196, NULL, priority, NULL,core); 
}
