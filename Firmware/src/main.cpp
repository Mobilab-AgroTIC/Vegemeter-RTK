#include "Arduino.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "WiFi.h"
#include "AsyncMqttClient.h"

#include "common/config.h"
#include "common/messages.h"
#include "common/logger.h"


#include "tasks/task_gnss.h"
#include "tasks/task_spectro.h"
#include "tasks/task_fusion.h"
#include "tasks/task_ntrip.h"
#include "tasks/task_mqtt.h"
#include "tasks/task_health.h"

#include "drivers/driver_f9p.h"
#include "drivers/driver_as7265x.h"
#include "drivers/driver_mqtt.h"
#include "drivers/driver_button.h"
#include "drivers/NTRIPClient.h"

// Déclaration des queues globales
QueueHandle_t QueueSpectro;
QueueHandle_t QueuePublish;
QueueHandle_t QueueGNSS;

HardwareSerial SerialF9P(GNSS_UART_PORT);
HardwareSerial SerialAS7265(SPECTRO_UART_PORT);

NTRIPClient ntripclient;
AsyncMqttClient mqttClient;

void setup() {
    // Init hardware
    Serial.begin(115200);
    Serial.println();
    LOG_HEALTH("==================================================");
    LOG_HEALTH("BBSOCOUL %s | niveau de log : %s (%d)", BBSOCOUL_ID, log_level_name(LOG_LEVEL), LOG_LEVEL);
    LOG_HEALTH("WiFi \"%s\" | MQTT %s:%d topic %s", WIFI_SSID, MQTT_BROKER, MQTT_PORT, MQTT_OUTPUT);
    LOG_HEALTH("==================================================");

    pinMode(BTN_LABEL_1_PIN,INPUT_PULLUP);
    pinMode(BTN_LABEL_2_PIN,INPUT_PULLUP);
    pinMode(BTN_SEND_DATA_PIN,INPUT_PULLUP);
    setup_mqtt(MQTT_BROKER,MQTT_USER, MQTT_PASSWORD,MQTT_PORT);
    setup_wifi(WIFI_SSID, WIFI_PASSWORD);
    

    bool gnss_ok    = driver_f9p_init(SerialF9P, GNSS_TX_PIN, GNSS_RX_PIN, GNSS_POWER_PIN, GNSS_BAUDRATE);
    bool spectro_ok = driver_as7265x_init(SerialAS7265,SPECTRO_TX_PIN,SPECTRO_RX_PIN,SPECTRO_BAUDRATE);
    // Résumé toujours affiché (même en LOG_LEVEL 0)
    LOG_HEALTH("Init capteurs : GNSS F9P %s | Spectro AS7265X %s",
               gnss_ok ? "OK" : "ECHEC", spectro_ok ? "OK" : "ECHEC");

    
    // Création des queues
    QueueSpectro = xQueueCreate(QUEUE_SPECTRO_LEN, sizeof(spectro_sample_t));
    QueueGNSS = xQueueCreate(QUEUE_GNSS_LEN, sizeof(gnss_snapshot_t));
    QueuePublish = xQueueCreate(QUEUE_PUBLISH_LEN, sizeof(fused_message_t));

    
    // Lancement des tâches
    startTaskGNSS(5, 1);      // Core 1 — seul, aucune compétition
    startTaskNTRIP(3, 0);     // Core 0 
    startTaskSpectro(5, 0);   // Core 0 — acquisition spectro
    startTaskFusion(4, 0);    // Core 0 — fusion
    startTaskMQTT(2, 0);      // Core 0 — AsyncTCP est ici, pas de cross-core
    startTaskHealth();
}

void loop() {
    // Nothing here, everything is handled in tasks
    // Just to satisfy Arduino framework
}

