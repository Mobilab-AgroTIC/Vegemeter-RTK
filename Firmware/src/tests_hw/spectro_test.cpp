// ============================================================================
// Test matériel autonome du spectromètre AS7265x (SparkFun Triad) en UART
// ============================================================================
//
// Ce programme remplace le firmware BBSOCOUL : il ne lance ni FreeRTOS tasks,
// ni WiFi/MQTT, ni GNSS. Il parle directement au Triad pour vérifier :
//   1. que le capteur répond (AT), sinon il scanne les baudrates courants ;
//   2. ses informations (versions, température, réglages) ;
//   3. l'acquisition en continu (ATCDATA) avec le temps de réponse.
//
// Compiler / téléverser / ouvrir le moniteur :
//   pio run -e spectro_test -t upload -t monitor
//
// Commandes dans le moniteur série (taper puis Entrée) :
//   p       pause / reprise des mesures automatiques
//   i       réafficher les informations du capteur
//   h       aide
//   AT...   toute commande commençant par AT est envoyée telle quelle au
//           capteur et sa réponse est affichée (ex : ATGAIN=2, ATTEMP)
//
// Les broches et le baudrate viennent de src/common/config.h.
// ============================================================================

#include <Arduino.h>
#include "common/config.h"

#define TEST_PERIOD_MS      1000   // intervalle entre deux mesures automatiques
#define AT_TIMEOUT_MS       1000   // timeout d'une commande AT

static HardwareSerial SpectroSerial(SPECTRO_UART_PORT);

static bool     paused       = false;
static uint32_t ok_count     = 0;
static uint32_t fail_count   = 0;
static uint32_t last_measure = 0;
static String   console_line;

// Canaux renvoyés par ATCDATA, dans l'ordre de la trame
static const char  CHANNEL_NAMES[18] = {'R','S','T','U','V','W','G','H','I','J','K','L','A','B','C','D','E','F'};
// Longueur d'onde (nm) de chaque canal, même ordre (datasheet AS7265x)
static const int   CHANNEL_NM[18]    = {610,680,730,760,810,860,560,585,645,705,900,940,410,435,460,485,510,535};

// ---------------------------------------------------------------------------
// Envoie une commande AT, attend "OK" ou "ERROR". Retourne la réponse (trim).
// ---------------------------------------------------------------------------
static bool at_command(const char* cmd, String& resp, uint32_t timeout_ms = AT_TIMEOUT_MS, uint32_t* elapsed_ms = nullptr) {
    while (SpectroSerial.available()) SpectroSerial.read(); // purge
    SpectroSerial.print(cmd);
    SpectroSerial.print("\r\n");

    resp = "";
    uint32_t start = millis();
    while (millis() - start < timeout_ms) {
        while (SpectroSerial.available()) resp += (char)SpectroSerial.read();
        if (resp.indexOf("OK") >= 0 || resp.indexOf("ERROR") >= 0) break;
        delay(1);
    }
    if (elapsed_ms) *elapsed_ms = millis() - start;
    resp.trim();
    return resp.indexOf("OK") >= 0 && resp.indexOf("ERROR") < 0;
}

// Affiche "cmd -> réponse" (pour les commandes d'information)
static void print_query(const char* label, const char* cmd) {
    String resp;
    bool ok = at_command(cmd, resp);
    resp.replace("OK", "");
    resp.trim();
    Serial.printf("  %-28s %-10s : %s\n", label, cmd, ok ? resp.c_str() : "(pas de réponse / ERROR)");
}

static void print_help() {
    Serial.println();
    Serial.println("Commandes : p = pause/reprise | i = infos capteur | h = aide | AT... = commande brute");
    Serial.println();
}

static void print_info() {
    Serial.println();
    Serial.println("---- Informations capteur ----");
    print_query("Version logicielle",           "ATVERSW");
    print_query("Version matérielle",           "ATVERHW");
    print_query("Températures (°C, 3 puces)",   "ATTEMP");
    print_query("Temps d'intégration (×2.8ms)", "ATINTTIME");
    print_query("Gain (0=1x 1=3.7x 2=16x 3=64x)", "ATGAIN");
    print_query("Mode de mesure (défaut 2)",    "ATTCSMD");
    Serial.println();
}

// ---------------------------------------------------------------------------
// Aucune réponse : on essaie les baudrates courants pour orienter le diagnostic
// ---------------------------------------------------------------------------
static void baudrate_scan() {
    const uint32_t bauds[] = {9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600};
    Serial.println("Recherche du capteur sur d'autres baudrates...");
    for (uint32_t b : bauds) {
        SpectroSerial.updateBaudRate(b);
        delay(50);
        String resp;
        bool ok = at_command("AT", resp, 300);
        Serial.printf("  %7lu bauds : %s\n", (unsigned long)b,
                      ok ? "OK  <-- le capteur répond ici !" : (resp.length() ? "octets reçus mais illisibles" : "rien"));
        if (ok) {
            Serial.printf("\n=> Le capteur est configuré à %lu bauds au lieu de %d.\n", (unsigned long)b, SPECTRO_BAUDRATE);
            Serial.println("   Corriger SPECTRO_BAUDRATE dans config.h, ou remettre le capteur à 115200 (ATBAUD).");
            return; // on reste sur le baudrate qui fonctionne pour la suite du test
        }
    }
    SpectroSerial.updateBaudRate(SPECTRO_BAUDRATE);
    Serial.println();
    Serial.println("=> Aucune réponse quel que soit le baudrate. A vérifier :");
    Serial.println("   - alimentation du Triad (3.3V) et masse commune avec l'ESP32");
    Serial.printf ("   - câblage : TX du Triad -> GPIO%d (RX ESP32), RX du Triad -> GPIO%d (TX ESP32)\n", SPECTRO_RX_PIN, SPECTRO_TX_PIN);
    Serial.println("   - cavaliers du Triad en mode UART (et non I2C)");
    Serial.printf ("   - test de boucle : capteur débranché, relier GPIO%d et GPIO%d : la commande doit revenir en écho\n",
                   SPECTRO_RX_PIN, SPECTRO_TX_PIN);
    Serial.println("Le test continue : les mesures seront retentées en boucle.");
}

// ---------------------------------------------------------------------------
// Une mesure ATCDATA, affichée par longueur d'onde croissante
// ---------------------------------------------------------------------------
static void measure() {
    String resp;
    uint32_t elapsed = 0;
    // ATCDATA peut prendre jusqu'à une mesure complète (2 périodes d'intégration)
    uint32_t timeout = 2 * SPECTRO_INTEGRATION_TIME_LOOP * 3 + 500;
    bool ok = at_command("ATCDATA", resp, timeout, &elapsed);

    if (!ok) {
        fail_count++;
        Serial.printf("[%7.1f s] ECHEC ATCDATA (%lu ms) : %s | ok=%lu echecs=%lu\n",
                      millis() / 1000.0, (unsigned long)elapsed,
                      resp.length() ? resp.c_str() : "aucune réponse",
                      (unsigned long)ok_count, (unsigned long)fail_count);
        return;
    }

    resp.replace("OK", "");
    resp.replace(" ", "");
    float values[18];
    int n = 0;
    char* tok = strtok((char*)resp.c_str(), ",");
    while (tok && n < 18) {
        values[n++] = atof(tok);
        tok = strtok(NULL, ",");
    }
    if (n < 18) {
        fail_count++;
        Serial.printf("[%7.1f s] Trame incomplète : %d/18 valeurs | ok=%lu echecs=%lu\n",
                      millis() / 1000.0, n, (unsigned long)ok_count, (unsigned long)fail_count);
        return;
    }
    ok_count++;

    // Tri par longueur d'onde pour un affichage lisible
    int order[18];
    for (int i = 0; i < 18; i++) order[i] = i;
    for (int i = 0; i < 17; i++)
        for (int j = i + 1; j < 18; j++)
            if (CHANNEL_NM[order[j]] < CHANNEL_NM[order[i]]) { int t = order[i]; order[i] = order[j]; order[j] = t; }

    Serial.printf("[%7.1f s] Mesure OK en %lu ms | ok=%lu echecs=%lu\n",
                  millis() / 1000.0, (unsigned long)elapsed, (unsigned long)ok_count, (unsigned long)fail_count);
    for (int row = 0; row < 3; row++) {
        Serial.print("   ");
        for (int col = 0; col < 6; col++) {
            int k = order[row * 6 + col];
            Serial.printf("%c %3dnm:%9.2f  ", CHANNEL_NAMES[k], CHANNEL_NM[k], values[k]);
        }
        Serial.println();
    }
}

// ---------------------------------------------------------------------------
// Lecture des commandes tapées dans le moniteur série
// ---------------------------------------------------------------------------
static void handle_console() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c != '\r' && c != '\n') {
            console_line += c;
            continue;
        }
        console_line.trim();
        if (console_line.length() == 0) continue;

        if (console_line.equalsIgnoreCase("p")) {
            paused = !paused;
            Serial.println(paused ? ">> Mesures en pause" : ">> Mesures reprises");
        } else if (console_line.equalsIgnoreCase("i")) {
            print_info();
        } else if (console_line.equalsIgnoreCase("h")) {
            print_help();
        } else if (console_line.startsWith("AT") || console_line.startsWith("at")) {
            console_line.toUpperCase();
            String resp;
            uint32_t elapsed = 0;
            at_command(console_line.c_str(), resp, 2000, &elapsed);
            Serial.printf(">> %s -> %s (%lu ms)\n", console_line.c_str(),
                          resp.length() ? resp.c_str() : "(aucune réponse)", (unsigned long)elapsed);
        } else {
            Serial.printf(">> Commande inconnue : %s\n", console_line.c_str());
            print_help();
        }
        console_line = "";
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println();
    Serial.println("==================================================");
    Serial.println(" TEST SPECTRO AS7265x (Triad) - UART");
    Serial.printf (" UART%d | RX=GPIO%d TX=GPIO%d | %d bauds\n",
                   SPECTRO_UART_PORT, SPECTRO_RX_PIN, SPECTRO_TX_PIN, SPECTRO_BAUDRATE);
    Serial.println("==================================================");

    SpectroSerial.begin(SPECTRO_BAUDRATE, SERIAL_8N1, SPECTRO_RX_PIN, SPECTRO_TX_PIN);
    delay(1000); // laisse le temps au Triad de démarrer

    // 1. Communication
    String resp;
    bool alive = false;
    for (int attempt = 1; attempt <= 3 && !alive; attempt++) {
        alive = at_command("AT", resp);
        Serial.printf("AT (essai %d/3) : %s\n", attempt, alive ? "OK" : (resp.length() ? resp.c_str() : "aucune réponse"));
    }
    if (!alive) {
        baudrate_scan();
    }

    // 2. Informations et configuration
    print_info();
    char cmd[24];
    snprintf(cmd, sizeof(cmd), "ATINTTIME=%d", SPECTRO_INTEGRATION_TIME_LOOP);
    Serial.printf("Configuration %s : %s\n", cmd, at_command(cmd, resp) ? "OK" : "ECHEC");

    print_help();
}

void loop() {
    handle_console();
    if (!paused && millis() - last_measure >= TEST_PERIOD_MS) {
        last_measure = millis();
        measure();
    }
    delay(5);
}
