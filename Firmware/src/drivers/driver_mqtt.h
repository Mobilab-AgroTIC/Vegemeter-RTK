#pragma once
#include "common/messages.h"

// Résultat d'une tentative de publication
typedef enum {
    MQTT_SEND_OK,       // publié sur le broker
    MQTT_SEND_SKIPPED,  // volontairement non envoyé (bouton d'envoi inactif)
    MQTT_SEND_RETRY,    // échec temporaire (broker non connecté...) : à réessayer
    MQTT_SEND_DROPPED   // échec définitif (JSON trop grand) : abandonné
} mqtt_send_result_t;

void setup_mqtt(const char* mqtt_server, const char* mqtt_user, const char* mqtt_password, uint16_t mqtt_port);
mqtt_send_result_t prepare_and_send_mqtt_message(const fused_message_t &msg);
void setup_wifi(const char* ssid, const char* password);
