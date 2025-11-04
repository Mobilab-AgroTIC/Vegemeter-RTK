#include "Arduino.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "WiFi.h"
#include "AsyncMqttClient.h"

#include "common/config.h"
#include "common/messages.h"

#include "tasks/task_gnss.h"
#include "tasks/task_spectro.h"
#include "tasks/task_fusion.h"
#include "tasks/task_ntrip.h"
#include "tasks/task_mqtt.h"
#include "tasks/task_health.h"
#include "tasks/task_gga.h"
#include "drivers/driver_f9p.h"
#include "drivers/driver_as7265x.h"
#include "drivers/driver_mqtt_ntrip.h"
#include "drivers/driver_button.h"

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
    Serial.println("Starting BBSOCOUL...");

    pinMode(BTN_LABEL_1_PIN,INPUT_PULLUP);
    pinMode(BTN_LABEL_2_PIN,INPUT_PULLUP);
    pinMode(BTN_SEND_DATA_PIN,INPUT_PULLUP);
    setup_mqtt(MQTT_BROKER,MQTT_USER, MQTT_PASSWORD,MQTT_PORT);
    setup_wifi(WIFI_SSID, WIFI_PASSWORD);
    
    
    //setup_ntrip(NTRIP_HOST,NTRIP_PORT,NTRIP_MNTPNT,NTRIP_USER,NTRIP_PASSWORD);


    driver_f9p_init(SerialF9P, GNSS_TX_PIN, GNSS_RX_PIN, GNSS_POWER_PIN, GNSS_BAUDRATE);
    driver_as7265x_init(SerialAS7265,SPECTRO_TX_PIN,SPECTRO_RX_PIN,SPECTRO_BAUDRATE);
    
    
    // Création des queues
    QueueSpectro = xQueueCreate(QUEUE_SPECTRO_LEN, sizeof(spectro_sample_t));
    QueueGNSS = xQueueCreate(QUEUE_GNSS_LEN, sizeof(gnss_snapshot_t));
    QueuePublish = xQueueCreate(QUEUE_PUBLISH_LEN, sizeof(fused_message_t));

    
    // Création des tâches
    startTaskGNSS(5,1);
    startTaskNTRIP(4,1);
    startTaskGGA(1,1);
    startTaskSpectro(4,0);
    startTaskFusion(4,0);
    startTaskMQTT(2,0);
    startTaskHealth();

}

void loop() {
    // Nothing here, everything is handled in tasks
    // Just to satisfy Arduino framework
}

