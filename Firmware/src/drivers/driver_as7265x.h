#pragma once
#include "Arduino.h"
#include "Wire.h"
#include "common/messages.h"

// ============================================================
// Driver AS7265x (SparkFun Triad) en I2C — V3 exploratoire
//
// L'adresse I2C du capteur est figée (0x49) : un capteur par bus.
// L'ESP32 dispose de deux contrôleurs (Wire, Wire1) :
//   - SPECTRO_REFLECTED : capteur réfléchi (végétation), sur Wire
//   - SPECTRO_INCIDENT  : capteur de rayonnement incident, sur Wire1
// Chaque capteur est piloté par une seule tâche à la fois.
// ============================================================

// État du spectromètre, utilisé pour le bloc STATUS
typedef struct {
    bool     init_ok;               // capteur détecté au moins une fois (ACK I2C + 2 esclaves présents)
    bool     config_ok;             // temps d'intégration et gain appliqués
    uint32_t consecutive_failures;  // échecs consécutifs (0 = capteur opérationnel)
    uint32_t success_count;         // nombre total d'échantillons valides
    uint32_t last_success_ms;       // millis() du dernier échantillon valide (0 = jamais)
    uint32_t last_measure_ms;       // durée de la dernière mesure (déclenchement -> données prêtes)
    uint32_t last_read_ms;          // durée de la lecture I2C des 18 valeurs calibrées
} as7265x_status_t;

// Un capteur = un bus I2C
typedef struct {
    const char*      name;          // préfixe des logs, ex. "AS7265X"
    TwoWire*         wire;
    int              sda;
    int              scl;
    uint32_t         freq;
    const char*      last_error;    // dernière erreur bas niveau (pour les logs)
    as7265x_status_t status;
} as7265x_t;

// Démarre le bus I2C et configure le capteur `id`.
bool driver_as7265x_init(spectro_source_t id, const char* name, TwoWire& wire,
                         int sdaPin, int sclPin, uint32_t freq);

// Mesure complète (déclenchement + attente + lecture des 18 canaux calibrés).
// `out` n'est modifié qu'en cas de succès (ts_ms et source renseignés, label à 0).
bool driver_as7265x_measure(spectro_source_t id, spectro_sample_t& out);

as7265x_status_t driver_as7265x_get_status(spectro_source_t id);
