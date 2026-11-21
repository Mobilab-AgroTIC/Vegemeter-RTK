#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ArduinoJson.h"
#include "AsyncMqttClient.h"
#include "driver_mqtt.h"
#include "WiFi.h"
#include "common/config.h"
#include "common/messages.h"
#include "common/logger.h"
#include "common/logger.h"
#include "NTRIPClient.h"
#include "drivers/driver_button.h"

extern AsyncMqttClient mqttClient;
extern NTRIPClient ntripclient;

// États mémorisés pour ne logger que les transitions (et pas chaque tentative)
static bool wifi_up = false;
static bool mqtt_up = false;

// --- Callback : WiFi connecté + IP obtenue ---
void onWifiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  wifi_up = true;
  LOG_INFO("[WIFI] Connecté à \"%s\" : IP %s, signal %d dBm",
           WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());

  // Lancer MQTT (si pas déjà connecté)
  if (!mqttClient.connected()) {
    LOG_DEBUG("[MQTT] Connexion au broker %s:%d...", MQTT_BROKER, MQTT_PORT);
    mqttClient.connect();
  }
}

// --- Callback : WiFi perdu ---
void onWifiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  uint8_t reason = info.wifi_sta_disconnected.reason;
  if (wifi_up) {
    LOG_WARN("[WIFI] Connexion perdue (raison %u), reconnexion...", reason);
  } else {
    // Réseau absent ou mauvais mot de passe : cet événement revient à chaque tentative
    LOG_THROTTLED(LOG_WARN, LOG_THROTTLE_MS,
                  "[WIFI] Connexion à \"%s\" impossible (raison %u), nouvelle tentative...",
                  WIFI_SSID, reason);
  }
  wifi_up = false;
  WiFi.disconnect();
  WiFi.reconnect();
}

// --- Initialisation WiFi ---
void setup_wifi(const char* ssid, const char* password) {
  LOG_INFO("[WIFI] Connexion au réseau \"%s\"...", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.onEvent(onWifiGotIP, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(onWifiDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  WiFi.begin(ssid, password);
  LOG_DEBUG("[WIFI] WiFi.begin() appelé (asynchrone), attente des événements");
}

// --- Callback : connecté au broker ---
void onMqttConnect(bool sessionPresent) {
  mqtt_up = true;
  LOG_INFO("[MQTT] Connecté au broker %s:%d (session présente : %s)",
           MQTT_BROKER, MQTT_PORT, sessionPresent ? "oui" : "non");
}

static const char* mqtt_disconnect_reason_name(AsyncMqttClientDisconnectReason reason) {
  switch (reason) {
    case AsyncMqttClientDisconnectReason::TCP_DISCONNECTED:                   return "connexion TCP fermée / broker injoignable";
    case AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION: return "version de protocole refusée";
    case AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED:           return "identifiant client refusé";
    case AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE:            return "serveur indisponible";
    case AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS:         return "identifiants mal formés";
    case AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED:                return "non autorisé (utilisateur/mot de passe)";
    default:                                                                  return "autre";
  }
}

// --- Callback : déconnecté du broker ---
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  const bool config_error =
      reason == AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED ||
      reason == AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS ||
      reason == AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED ||
      reason == AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION;

  if (config_error) {
    // Problème de configuration : ne se résoudra pas tout seul
    LOG_THROTTLED(LOG_ERROR, LOG_THROTTLE_MS, "[MQTT] Connexion refusée par le broker : %s",
                  mqtt_disconnect_reason_name(reason));
  } else if (mqtt_up) {
    LOG_WARN("[MQTT] Déconnecté du broker : %s", mqtt_disconnect_reason_name(reason));
  } else {
    LOG_THROTTLED(LOG_WARN, LOG_THROTTLE_MS, "[MQTT] Connexion au broker %s:%d impossible : %s",
                  MQTT_BROKER, MQTT_PORT, mqtt_disconnect_reason_name(reason));
  }
  mqtt_up = false;

  if (WiFi.isConnected()) {
    LOG_DEBUG("[MQTT] WiFi OK, nouvelle tentative dans 2 s");
    // On ne reconnecte pas immédiatement pour éviter une boucle
    vTaskDelay(pdMS_TO_TICKS(2000));
    mqttClient.connect();
  } else {
    LOG_DEBUG("[MQTT] Attente du WiFi avant reconnexion");
  }
}

// --- Initialisation MQTT ---
void setup_mqtt(const char* mqtt_server, const char* mqtt_user, const char* mqtt_password, uint16_t mqtt_port) {
  LOG_DEBUG("[MQTT] Configuration du client (broker %s:%u)", mqtt_server, mqtt_port);
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCredentials(mqtt_user, mqtt_password);
  mqttClient.setKeepAlive(15);
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  // ⚠️ Pas de mqttClient.connect() ici !
  // Il sera appelé quand WiFi obtiendra une IP (dans util.cpp)
}

// Déclaration de la fonction qui sera appelée dans le loop ou par une autre tâche
mqtt_send_result_t prepare_and_send_mqtt_message(const fused_message_t &msg) {
    // Le buffer JSON doit être suffisamment grand pour tout le contenu.
    // 768 octets semblent adéquats, mais vérifiez si nécessaire.
    StaticJsonDocument<1024> doc;
    JsonObject root = doc.to<JsonObject>();
    const bool is_heartbeat = (msg.msg_type == MSG_TYPE_HEARTBEAT);

    // Identité / meta
    root["sensor_id"] = BBSOCOUL_ID;
    root["type"] = is_heartbeat ? "heartbeat" : "fused";
    root["ts"] = msg.ts_ms;
    root["delta_gnss_spectro_ms"] = msg.delta_time_gnss_spectro_ms;
    root["message_id"] = msg.message_id;
    root["gnss_valid"] = msg.gnss_valid;
    root["spectro_valid"] = msg.spectro_valid;

    // Diagnostic de la liaison (utile pour tester la transmission)
    JsonObject diag = root.createNestedObject("diag");
    diag["uptime_ms"] = millis();
    diag["rssi"] = WiFi.RSSI();
    diag["free_heap"] = ESP.getFreeHeap();

    // GNSS group
    JsonObject gnss = root.createNestedObject("gnss");
    gnss["ts_ms"] = msg.gnss.ts_ms;
    gnss["lat"] = msg.gnss.lat;
    gnss["lon"] = msg.gnss.lon;
    gnss["hMSL"] = msg.gnss.hMSL;
    gnss["hdop"] = msg.gnss.hdop;
    gnss["hAcc"] = msg.gnss.hAcc;
    gnss["fixtype"] = msg.gnss.fixtype;
    gnss["carrsoln"] = msg.gnss.carrsoln;
    gnss["fix"] = msg.gnss.fix_type;
    gnss["gSpeed"]=msg.gnss.gSpeed;
    JsonObject gnss_date = gnss.createNestedObject ("gnss_date");
    gnss_date["year"] = msg.gnss.year;
    gnss_date["month"] = msg.gnss.month;
    gnss_date["day"] = msg.gnss.day;
    gnss_date["hour"] = msg.gnss.hour;
    gnss_date["min"] = msg.gnss.min;
    gnss_date["sec"] = msg.gnss.sec;
    gnss_date["ms"] = msg.gnss.ms;

    // Spectro group
    JsonObject spectro = root.createNestedObject("spectro");
    spectro["ts_ms"] = msg.spectro.ts_ms;
    spectro["label"] = msg.spectro.label;
    spectro["R"] = msg.spectro.R;
    spectro["S"] = msg.spectro.S;
    spectro["T"] = msg.spectro.T;
    spectro["U"] = msg.spectro.U;
    spectro["V"] = msg.spectro.V;
    spectro["W"] = msg.spectro.W;
    spectro["G"] = msg.spectro.G;
    spectro["H"] = msg.spectro.H;
    spectro["I"] = msg.spectro.I;
    spectro["J"] = msg.spectro.J;
    spectro["K"] = msg.spectro.K;
    spectro["L"] = msg.spectro.L;
    spectro["A"] = msg.spectro.A;
    spectro["B"] = msg.spectro.B;
    spectro["C"] = msg.spectro.C;
    spectro["D"] = msg.spectro.D;
    spectro["E"] = msg.spectro.E;
    spectro["F"] = msg.spectro.F;

    // Sérialiser
    char payload[1024];
    size_t measured_size = measureJson(doc);
    if (doc.overflowed() || measured_size >= sizeof(payload)) {
        LOG_ERROR("[MQTT] JSON trop grand (%u octets), message #%lu abandonné",
                  (unsigned)measured_size, (unsigned long)msg.message_id);
        return MQTT_SEND_DROPPED; // inutile de le réessayer : il échouerait toujours
    }
    size_t len = serializeJson(doc, payload, sizeof(payload));

    // Publier
    bool state = state_button_send_data();
    if (is_heartbeat && HEARTBEAT_BYPASS_SEND_BUTTON) {
        state = true; // les heartbeats servent à tester la liaison : on ignore le bouton
    }
    if (!state) {
        LOG_DEBUG("[MQTT] Message #%lu ignoré (bouton d'envoi inactif)", (unsigned long)msg.message_id);
        return MQTT_SEND_SKIPPED; // voulu : on ne le réessaie pas
    }

    if (!mqttClient.connected()) {
        LOG_DEBUG("[MQTT] Client non connecté, message #%lu à réessayer", (unsigned long)msg.message_id);
        return MQTT_SEND_RETRY;
    }

    // Utilisation de QoS 1 (At least once) par défaut pour un envoi fiable,
    // ou changez à QoS 0 si la performance est critique et la perte acceptable.
    uint16_t packetId = mqttClient.publish(MQTT_OUTPUT, 1, false, payload, len);
    if (packetId > 0) {
        LOG_DEBUG("[MQTT] %s #%lu publié (%u octets, packetId %u)",
                  is_heartbeat ? "Heartbeat" : "Message fusionné",
                  (unsigned long)msg.message_id, (unsigned)len, packetId);
        return MQTT_SEND_OK;
    }

    LOG_THROTTLED(LOG_WARN, LOG_THROTTLE_MS, "[MQTT] Échec de publish (message #%lu), nouvelle tentative",
                  (unsigned long)msg.message_id);
    return MQTT_SEND_RETRY;
}