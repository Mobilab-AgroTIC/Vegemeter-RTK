#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ArduinoJson.h"
#include "AsyncMqttClient.h"
#include "driver_mqtt_ntrip.h"
#include "WiFi.h"
#include "common/config.h"
#include "common/messages.h"
#include "NTRIPClient.h"
#include "drivers/driver_button.h"

extern AsyncMqttClient mqttClient;
extern NTRIPClient ntripclient;

// --- Callback : WiFi connecté + IP obtenue ---
void onWifiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("[WIFI] Got IP, trying MQTT connect...");


  // Lancer MQTT (si pas déjà connecté)
  if (!mqttClient.connected()) {
    mqttClient.connect();
  }
  
  // Lancer NTRIP
  setup_ntrip(NTRIP_HOST,NTRIP_PORT,NTRIP_MNTPNT,NTRIP_USER,NTRIP_PASSWORD);
  
}

// --- Callback : WiFi perdu ---
void onWifiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("[WIFI] Lost connection, reconnecting WiFi...");
  WiFi.disconnect();
  WiFi.reconnect();
}

// --- Initialisation WiFi ---
void setup_wifi(const char* ssid, const char* password) {
  Serial.printf("[WIFI] start connect to %s\n", ssid);

  WiFi.mode(WIFI_STA);
  WiFi.onEvent(onWifiGotIP, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(onWifiDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  WiFi.begin(ssid, password);

  Serial.println("[WIFI] WiFi.begin() called (async). Waiting for events...");
}


void setup_ntrip(char* host,int port,char* mntpnt,char* user,char* psw){
    Serial.println("Requesting MountPoint's Raw data");
    if(!ntripclient.reqRaw(host,port,mntpnt,user,psw)){
        Serial.println("failed to connect to ntrip");
        //delay(15000);
        //ESP.restart();
    }
    Serial.println("Requesting MountPoint is OK");
}

// --- Callback : connecté au broker ---
void onMqttConnect(bool sessionPresent) {
  Serial.printf("[MQTT] Connected. Session present: %s\n", sessionPresent ? "true" : "false");
}

// --- Callback : déconnecté du broker ---
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.printf("[MQTT] Disconnected (reason %d)\n", (int)reason);
  if (WiFi.isConnected()) {
    Serial.println("[MQTT] WiFi OK, scheduling reconnect...");
    // On ne reconnecte pas immédiatement pour éviter une boucle
    vTaskDelay(pdMS_TO_TICKS(2000));
    mqttClient.connect();
  } else {
    Serial.println("[MQTT] Waiting for WiFi before reconnecting...");
  }
}

// --- Initialisation MQTT ---
void setup_mqtt(const char* mqtt_server, const char* mqtt_user, const char* mqtt_password, uint16_t mqtt_port) {
  Serial.println("[MQTT] Setup...");
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCredentials(mqtt_user, mqtt_password);
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  // ⚠️ Pas de mqttClient.connect() ici !
  // Il sera appelé quand WiFi obtiendra une IP (dans util.cpp)
}

// Déclaration de la fonction qui sera appelée dans le loop ou par une autre tâche
void prepare_and_send_mqtt_message(const fused_message_t &msg) {
    // Le buffer JSON doit être suffisamment grand pour tout le contenu.
    // 768 octets semblent adéquats, mais vérifiez si nécessaire.
    StaticJsonDocument<768> doc;
    JsonObject root = doc.to<JsonObject>();

    // Identité / meta
    root["sensor_id"] = BBSOCOUL_ID;
    root["ts"] = msg.ts_ms;
    root["delta_gnss_spectro_ms"] = msg.delta_time_gnss_spectro_ms;
    root["message_id"] = msg.message_id;

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
    size_t measured_size = measureJson(doc);
    // Utiliser un buffer dynamique ou un array temporaire pour la sérialisation
    // Créer un buffer sur le tas (heap) pour éviter un débordement de pile (stack overflow)
    char* payload = (char*)malloc(measured_size + 1);
    if (!payload) {
        Serial.println("[MQTT] Allocation failed for payload.");
        return;
    }
    size_t len = serializeJson(doc, payload, measured_size + 1);

    // Publier
    bool state = state_button_send_data(); // Assurez-vous que cette fonction est disponible
    
    if (state) {
        if (mqttClient.connected()) {
            // Utilisation de QoS 1 (At least once) par défaut pour un envoi fiable,
            // ou changez à QoS 0 si la performance est critique et la perte acceptable.
            // Le "true" à la fin signifie que le message n'est PAS retenu (non-retained).
            uint16_t packetId = mqttClient.publish(MQTT_OUTPUT, 1, false, payload, len); 
            
            if (packetId > 0) {
                Serial.printf("[MQTT] Published fused msg (%d bytes), packet ID: %u\n", len, packetId);
            } else {
                Serial.println("[MQTT] Publish failed (packetId=0)");
            }
        } else {
            Serial.println("[MQTT] Client not connected, message dropped.");
            // Logique de reconnexion/mise en file d'attente à implémenter si nécessaire
        }
    } else {
        Serial.println("[MQTT] Sending Deactivated by button state.");
    }
    
    free(payload); // Libérer la mémoire allouée
}