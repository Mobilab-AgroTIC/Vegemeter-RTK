#include "task_spectro.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "common/messages.h"
#include "common/config.h"
#include "drivers/driver_as7265x.h"

spectro_sample_t spectro_msg;

void task_spectro(void *pvParameters){
    while (1) {
        driver_as7265x_get_data();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void startTaskSpectro(int priority, int core) {
    xTaskCreatePinnedToCore(task_spectro, "SPECTRO", 4096, NULL, priority, NULL,core); 
}