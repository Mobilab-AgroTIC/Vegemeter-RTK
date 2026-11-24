#pragma once

// Configuration du boîtier.
// config.h n'est PAS versionné (.gitignore) : il contient les identifiants WiFi / MQTT.
// Modèle versionné : config.exemple.h -> le copier en config.h puis renseigner les valeurs.
// Tout nouveau paramètre ajouté ici doit aussi être ajouté dans config.exemple.h.

// Config General
#define BBSOCOUL_ID "V2.2_B10"
#define BTN_SEND_DATA_PIN 34
#define BTN_LABEL_1_PIN 32
#define BTN_LABEL_2_PIN 33
#define LOG_LEVEL 2 // 0=NONE (STATUS seul) 1=ERROR 2=WARNING 3=INFO 4=DEBUG (voir common/logger.h)
#define STATUS_PERIOD_MS 5000   // période du bloc STATUS (toujours affiché, même en LOG_LEVEL 0)
#define LOG_THROTTLE_MS 10000   // délai minimal entre deux rappels d'une même erreur répétitive

// Config GNSS
#define GNSS_RX_PIN 27
#define GNSS_TX_PIN 26
#define GNSS_POWER_PIN 25
#define GNSS_POWER_ON_DELAY_MS 1000 // délai avant d'alimenter le F9P : laisse le PMU AXP2101 démarrer sur batterie sans l'appel de courant du F9P
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


// Config Spectro (I2C — V3 exploratoire)
// Adresse I2C fixe (0x49) : un capteur par bus. Bus 0 (Wire) = capteur réfléchi.
// Les cavaliers du Triad doivent être en position I2C (défaut SparkFun).
#define SPECTRO_SDA_PIN 13
#define SPECTRO_SCL_PIN 15
#define SPECTRO_I2C_FREQ 400000   // 100000 ou 400000 Hz (max capteur : 400 kHz)
// Gain : 0 = 1x, 1 = 3.7x, 2 = 16x, 3 = 64x
// ⚠️ À aligner sur le gain utilisé en UART (ATGAIN) pour garder des mesures comparables.
#define SPECTRO_GAIN 1
// Temps d'intégration (1-255, × ~2.8 ms).
// 20 = valeur par défaut du Triad (datasheet AS7265x, Figure 59) -> ~56 ms,
// soit ~112 ms par mesure complète (mode 6 canaux = 2 banques).
#define SPECTRO_INTEGRATION_TIME_LOOP 20

// Second spectro : rayonnement incident, sur le bus 1 (Wire1).
// Même gain et même temps d'intégration que le capteur réfléchi.
// ⚠️ Broches à vérifier selon le câblage (21/22 = I2C par défaut de l'ESP32,
// partagées avec le PMU AXP2101 de la T-PCIE à l'adresse 0x34 : pas de conflit avec 0x49).
#define SPECTRO_INCIDENT_ENABLE 1        // 1 = capteur incident présent, 0 = ignoré
#define SPECTRO_INCIDENT_SDA_PIN 21
#define SPECTRO_INCIDENT_SCL_PIN 22
#define SPECTRO_INCIDENT_PERIOD_MS HEARTBEAT_PERIOD_MS // une mesure incidente publiée par période

// Ancienne liaison UART : utilisée uniquement par tests_hw/spectro_test.cpp
#define SPECTRO_RX_PIN 13
#define SPECTRO_TX_PIN 15
#define SPECTRO_BAUDRATE 115200
#define SPECTRO_UART_PORT 2


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
// Connexion "zombie" : client vu connecté mais plus aucun publish accepté (tampon TCP plein,
// le broker n'acquitte plus rien). Au-delà de ce délai, la connexion est fermée puis rouverte.
#define MQTT_STALL_TIMEOUT_MS 20000

// Config Heartbeat : envoi MQTT périodique même si les capteurs ne renvoient rien
// (permet de tester la chaîne de transmission WiFi -> MQTT -> broker)
#define HEARTBEAT_ENABLE 1                // 1 = actif, 0 = désactivé
#define HEARTBEAT_PERIOD_MS 2000          // envoi d'un heartbeat si aucune fusion depuis X ms
#define HEARTBEAT_BYPASS_SEND_BUTTON 1    // 1 = les heartbeats sont envoyés même si le bouton d'envoi est inactif


// Taille des queues
#define QUEUE_GNSS_LEN 10
#define QUEUE_SPECTRO_LEN 10
#define QUEUE_SPECTRO_INCIDENT_LEN 5
#define QUEUE_PUBLISH_LEN 500
//#define QUEUE_GGA_LEN 5