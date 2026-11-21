#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "Arduino.h"
#include "WiFi.h"
#include "AsyncMqttClient.h"
#include "common/config.h"
#include "common/logger.h"
#include "common/messages.h"
#include "drivers/driver_f9p.h"
#include "drivers/driver_as7265x.h"
#include "tasks/task_mqtt.h"
#include "tasks/task_ntrip.h"
#include "task_health.h"


extern QueueHandle_t QueueSpectro;
extern QueueHandle_t QueuePublish;
extern QueueHandle_t QueueGNSS;
extern gnss_snapshot_t gnss_msg;
extern spectro_sample_t spectro_msg;
extern AsyncMqttClient mqttClient;

// Une mesure est considérée "fraîche" si elle date de moins de :
#define STATUS_STALE_MS 3000

// Écrit "il y a 1.2 s" / "jamais" dans buf
static const char* age_str(char* buf, size_t size, uint32_t ts_ms) {
    if (ts_ms == 0) {
        snprintf(buf, size, "jamais");
    } else {
        uint32_t age = millis() - ts_ms;
        snprintf(buf, size, "il y a %lu.%01lu s", (unsigned long)(age / 1000), (unsigned long)((age % 1000) / 100));
    }
    return buf;
}

// ------------------------------------------------------------
// Bloc STATUS : résumé de l'état du capteur, toujours affiché
// (y compris en LOG_LEVEL 0) toutes les STATUS_PERIOD_MS.
// ------------------------------------------------------------
static void print_status_block() {
    char age[24];
    uint32_t up = millis() / 1000;

    LOG_HEALTH("------ %s | uptime %02lu:%02lu:%02lu ------", BBSOCOUL_ID,
               (unsigned long)(up / 3600), (unsigned long)((up / 60) % 60), (unsigned long)(up % 60));

    // --- GNSS ---
    gnss_snapshot_t g = gnss_msg; // copie locale (mise à jour par la tâche GNSS)
    if (!driver_f9p_is_initialized()) {
        LOG_HEALTH("GNSS    : ERREUR | F9P non détecté à l'initialisation");
    } else if (g.ts_ms == 0 || millis() - (uint32_t)g.ts_ms > STATUS_STALE_MS) {
        LOG_HEALTH("GNSS    : MUET   | aucune position reçue (dernière : %s)",
                   age_str(age, sizeof(age), (uint32_t)g.ts_ms));
    } else if (g.fixtype == 0) {
        LOG_HEALTH("GNSS    : NO FIX | sats=%u | recherche de satellites", g.num_sats);
    } else {
        LOG_HEALTH("GNSS    : OK     | fix=%s RTK=%s sats=%u hAcc=%lu.%03lu m | lat=%.7f lon=%.7f",
                   f9p_fix_type_name(g.fixtype), f9p_carr_soln_name(g.carrsoln), g.num_sats,
                   (unsigned long)(g.hAcc / 1000), (unsigned long)(g.hAcc % 1000),
                   g.lat * 1e-7, g.lon * 1e-7);
    }

    // --- Spectro ---
    as7265x_status_t s = driver_as7265x_get_status();
    spectro_sample_t sp = spectro_msg;
    if (s.success_count > 0 && s.consecutive_failures == 0) {
        LOG_HEALTH("SPECTRO : OK     | %lu mesures | label=%u R=%.2f S=%.2f T=%.2f A=%.2f",
                   (unsigned long)s.success_count, sp.label, sp.R, sp.S, sp.T, sp.A);
    } else {
        LOG_HEALTH("SPECTRO : ERREUR | %s | %lu échecs consécutifs | dernière mesure : %s",
                   s.init_ok ? "ne répond plus" : "pas de réponse depuis le démarrage",
                   (unsigned long)s.consecutive_failures,
                   age_str(age, sizeof(age), s.last_success_ms));
    }

    // --- Réseau ---
    char wifi[40];
    if (WiFi.isConnected()) snprintf(wifi, sizeof(wifi), "OK (%d dBm)", WiFi.RSSI());
    else                    snprintf(wifi, sizeof(wifi), "DECONNECTE");
    char ntrip[48];
    if (ntrip_is_connected()) {
        snprintf(ntrip, sizeof(ntrip), "OK (%lu ko RTCM, %s)",
                 (unsigned long)(ntrip_get_rtcm_bytes() / 1024),
                 age_str(age, sizeof(age), ntrip_get_last_rtcm_ms()));
    } else {
        snprintf(ntrip, sizeof(ntrip), "DECONNECTE");
    }
    LOG_HEALTH("RESEAU  : WiFi %s | MQTT %s | NTRIP %s",
               wifi, mqttClient.connected() ? "OK" : "DECONNECTE", ntrip);

    // --- Envoi MQTT ---
    LOG_HEALTH("ENVOI   : %lu publiés (dont %lu heartbeats) | %lu ignorés (bouton) | %lu perdus | %u en attente",
               (unsigned long)mqtt_get_published_count(), (unsigned long)mqtt_get_heartbeat_count(),
               (unsigned long)mqtt_get_skipped_count(), (unsigned long)mqtt_get_dropped_count(),
               (unsigned)uxQueueMessagesWaiting(QueuePublish));

    // --- Système ---
    LOG_HEALTH("SYSTEME : heap libre %lu o (min %lu o) | queues GNSS %u/%d SPECTRO %u/%d PUBLISH %u/%d",
               (unsigned long)ESP.getFreeHeap(), (unsigned long)ESP.getMinFreeHeap(),
               (unsigned)uxQueueMessagesWaiting(QueueGNSS), QUEUE_GNSS_LEN,
               (unsigned)uxQueueMessagesWaiting(QueueSpectro), QUEUE_SPECTRO_LEN,
               (unsigned)uxQueueMessagesWaiting(QueuePublish), QUEUE_PUBLISH_LEN);
}

void task_health(void *pvParameters) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(STATUS_PERIOD_MS));
        print_status_block();
    }
}

void startTaskHealth(){
    xTaskCreatePinnedToCore(task_health,"Health",4096,NULL,1,NULL,tskNO_AFFINITY);
}
