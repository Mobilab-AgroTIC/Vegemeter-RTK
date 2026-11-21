#include "task_ntrip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "common/logger.h"
#include "drivers/NTRIPClient.h"
#include "WiFi.h"
extern NTRIPClient ntripclient;
extern HardwareSerial SerialF9P;

// État lu par la tâche Health (bloc STATUS)
static volatile bool     ntrip_connected  = false;
static volatile uint32_t rtcm_bytes_total = 0;
static volatile uint32_t last_rtcm_ms     = 0;

bool ntrip_is_connected()          { return ntrip_connected; }
uint32_t ntrip_get_rtcm_bytes()    { return rtcm_bytes_total; }
uint32_t ntrip_get_last_rtcm_ms()  { return last_rtcm_ms; }

void taskNTRIP(void *pvParameters){
    uint8_t buffer[256];
    uint32_t lastGGATime = 0;
    for (;;) {
        // --- Connexion / reconnexion ---
        if (!ntripclient.connected()) {
            if (ntrip_connected) {
                LOG_WARN("[NTRIP] Connexion au caster perdue, reconnexion...");
                ntrip_connected = false;
            }
            if (!WiFi.isConnected()) {
                // Inutile de tenter sans WiFi (le WiFi signale déjà son propre problème)
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }
            ntripclient.setTimeout(3000); // ← timeout borné !
            int port = NTRIP_PORT;
            if (ntripclient.reqRaw((char*)NTRIP_HOST, port,
                                   (char*)NTRIP_MNTPNT,
                                   (char*)NTRIP_USER,
                                   (char*)NTRIP_PASSWORD)) {
                ntrip_connected = true;
                LOG_INFO("[NTRIP] Connecté au caster %s:%d (mountpoint %s)", NTRIP_HOST, NTRIP_PORT, NTRIP_MNTPNT);
            } else {
                LOG_THROTTLED(LOG_WARN, LOG_THROTTLE_MS,
                              "[NTRIP] Connexion au caster %s:%d échouée, nouvel essai toutes les 5 s",
                              NTRIP_HOST, NTRIP_PORT);
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }
        }

        // --- Lecture RTCM → F9P ---
        int len = ntripclient.available();
        if (len > 0) {
            // On limite la lecture à la taille de notre buffer
            int toRead = (len > (int)sizeof(buffer)) ? (int)sizeof(buffer) : len;

            // Lecture par bloc (si ton driver le permet, sinon boucle rapide)
            for (int i = 0; i < toRead; i++) {
                buffer[i] = ntripclient.read();
            }

            // Écriture par bloc vers le F9P (beaucoup plus efficace)
            SerialF9P.write(buffer, toRead);
            rtcm_bytes_total += toRead;
            last_rtcm_ms = millis();
            LOG_DEBUG("[NTRIP] RTCM -> F9P : %d octets (%d disponibles)", toRead, len);
        }

        // --- Envoi GGA toutes les NTRIP_DELAY_SEND_GGA ms ---
        if (millis() - lastGGATime > NTRIP_DELAY_SEND_GGA) {
            String gga = ntripclient.getLastGGA();
            //String gga = "$GPGGA,123519.00,4336.5050,N,00352.7480,E,1,08,0.9,12.5,M,47.0,M,,*5C";//trame GGA random pour test
            if (!gga.isEmpty() && ntripclient.connected()) {
                ntripclient.print(gga);
                ntripclient.print("\r\n");
                LOG_DEBUG("[NTRIP] GGA envoyée au caster : %s", gga.c_str());
            } else {
                LOG_DEBUG("[NTRIP] Pas de GGA disponible à envoyer");
            }
            lastGGATime = millis();
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

}

void startTaskNTRIP(int priority, int core) {
    xTaskCreatePinnedToCore(taskNTRIP, "NTRIP", 4096, NULL, priority, NULL,core);
}
