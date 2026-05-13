#include "task_ntrip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "common/logger.h"
#include "drivers/NTRIPClient.h"
extern NTRIPClient ntripclient;
extern HardwareSerial SerialF9P;


void taskNTRIP(void *pvParameters){
    uint8_t buffer[256];
    uint32_t lastGGATime = 0;
    bool connected = false;
    for (;;) {
        // --- Connexion / reconnexion ---
        if (!ntripclient.connected()) {
            connected = false;
            ntripclient.setTimeout(3000); // ← timeout borné !
            int port = NTRIP_PORT;
            if (ntripclient.reqRaw((char*)NTRIP_HOST, port,
                                   (char*)NTRIP_MNTPNT,
                                   (char*)NTRIP_USER,
                                   (char*)NTRIP_PASSWORD)) {
                connected = true;
                LOG_INFO("[NTRIP] Connecté au caster");
            } else {
                LOG_WARN("[NTRIP] Connexion échouée, retry dans 5s");
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }
        }

        // --- Lecture RTCM → F9P ---
        int len = ntripclient.available();
        if (len > 0) {
            // On limite la lecture à la taille de notre buffer
            int toRead = (len > sizeof(buffer)) ? sizeof(buffer) : len;
            
            // Lecture par bloc (si ton driver le permet, sinon boucle rapide)
            for (int i = 0; i < toRead; i++) {
                buffer[i] = ntripclient.read();
            }
            
            // Écriture par bloc vers le F9P (beaucoup plus efficace)
            SerialF9P.write(buffer, toRead);
            LOG_INFO("[NTRIP] Buffer available: %d bytes\n", len);
        }

        // --- Envoi GGA toutes les NTRIP_DELAY_SEND_GGA ms ---
        if (millis() - lastGGATime > NTRIP_DELAY_SEND_GGA) {
            String gga = ntripclient.getLastGGA();
            //String gga = "$GPGGA,123519.00,4336.5050,N,00352.7480,E,1,08,0.9,12.5,M,47.0,M,,*5C";//trame GGA random pour test
            LOG_INFO("[NTRIP] Last GGA: %s", gga.c_str());
            if (!gga.isEmpty() && ntripclient.connected()) {
                ntripclient.print(gga);
                ntripclient.print("\r\n");
                LOG_INFO("[NTRIP] GGA envoyé");
            }
            lastGGATime = millis();
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

}

void startTaskNTRIP(int priority, int core) {
    xTaskCreatePinnedToCore(taskNTRIP, "NTRIP", 4096, NULL, priority, NULL,core); 
}