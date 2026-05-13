#pragma once

// Config General
#define BBSOCOUL_ID "V2.2_B8"
#define BTN_SEND_DATA_PIN 34
#define BTN_LABEL_1_PIN 32
#define BTN_LABEL_2_PIN 33
#define LOG_LEVEL 2 // 0=NONE 1=ERROR 2=WARNING 3=INFO 4=DEBUG

// Config GNSS
#define GNSS_RX_PIN 27
#define GNSS_TX_PIN 26
#define GNSS_POWER_PIN 25
#define GNSS_BAUDRATE 460800
#define GNSS_UART_PORT 1
#define GNSS_RATE_GGA 50 // Envoi d'une trame GGA toutes les 10 mesures (1 par seconde si sample rate 10Hz)
#define GNSS_NAV_RATE 10
// Config NTRIP
#define NTRIP_HOST "crtk.net"
#define NTRIP_PORT 2101
#define NTRIP_USER "centipede"
#define NTRIP_PASSWORD "centipede"
#define NTRIP_MNTPNT "NEAR"
#define NTRIP_DELAY_SEND_GGA 5000 // ms


// Config Spectro

#define SPECTRO_RX_PIN 13
#define SPECTRO_TX_PIN 15
#define SPECTRO_BAUDRATE 115200
#define SPECTRO_UART_PORT 2
#define SPECTRO_INTEGRATION_TIME_LOOP 20 //~2.8ms per loop


// Config MQTT 
#define WIFI_SSID "VOTRE_SSID"
#define WIFI_PASSWORD "VOTRE_MOT_DE_PASSE_WIFI"
#define MQTT_BROKER "broker.example.org"
#define MQTT_PORT 1883
#define MQTT_USER "VOTRE_UTILISATEUR_MQTT"
#define MQTT_PASSWORD "VOTRE_MOT_DE_PASSE_MQTT"
#define MQTT_INPUT "esp32/input"
#define MQTT_OUTPUT "esp32/update"
#define MQTT_BUFFERSIZE 500


// Taille des queues
#define QUEUE_GNSS_LEN 10
#define QUEUE_SPECTRO_LEN 10
#define QUEUE_PUBLISH_LEN 500
//#define QUEUE_GGA_LEN 5