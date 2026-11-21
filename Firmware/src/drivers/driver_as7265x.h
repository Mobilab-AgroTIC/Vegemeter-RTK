#include "Arduino.h"
#include "common/messages.h"

// État du spectromètre, utilisé pour le bloc STATUS
typedef struct {
    bool     init_ok;               // réponse "OK" à la commande AT au démarrage
    bool     config_ok;             // ATINTTIME accepté par le capteur
    uint32_t consecutive_failures;  // échecs consécutifs (0 = capteur opérationnel)
    uint32_t success_count;         // nombre total d'échantillons valides
    uint32_t last_success_ms;       // millis() du dernier échantillon valide (0 = jamais)
} as7265x_status_t;

bool driver_as7265x_init(HardwareSerial& serial, int txPin, int rxPin, uint32_t baudrate);

bool driver_as7265x_get_data();

as7265x_status_t driver_as7265x_get_status();
