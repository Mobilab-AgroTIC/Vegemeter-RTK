#pragma once
#include <stdint.h>

void task_mqtt(void *pvParameters);
void startTaskMQTT(int priority, int core);

// Compteurs (bloc STATUS)
uint32_t mqtt_get_published_count(); // messages publiés sur le broker (dont heartbeats)
uint32_t mqtt_get_heartbeat_count(); // heartbeats publiés
uint32_t mqtt_get_skipped_count();   // messages ignorés (bouton d'envoi inactif)
uint32_t mqtt_get_dropped_count();   // messages perdus (queue pleine, JSON trop grand)
