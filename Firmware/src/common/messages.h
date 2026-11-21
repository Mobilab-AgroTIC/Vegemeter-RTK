#pragma once
#include <stdint.h>
#include <stdbool.h>

// Structures de données échangées entre tâches

// GNSS snapshot
typedef struct {
    uint64_t ts_ms;
    int32_t lat, lon, hMSL;
    uint8_t num_sats;
    uint8_t fix_type;
    uint8_t carsoln;
    float hdop;
    uint32_t hAcc;
    uint8_t fixtype;
    uint8_t carrsoln;
    uint32_t gSpeed;
    uint16_t year;
    uint8_t month, day, hour, min, sec;
    uint32_t ms;
    // uint32_t seq;
} gnss_snapshot_t;

// Spectro sample
typedef struct {
    uint64_t ts_ms;
    uint8_t label; // 0: none, 1: sample, 2: calibration
    float R;
    float S;
    float T;
    float U;
    float V;
    float W;
    float G;
    float H;
    float I;
    float J;
    float K;
    float L;
    float A;
    float B;
    float C;
    float D;
    float E;
    float F;
} spectro_sample_t;

// Type de message publié
typedef enum : uint8_t {
    MSG_TYPE_FUSED     = 0, // fusion GNSS + spectro complète
    MSG_TYPE_HEARTBEAT = 1  // envoi périodique, données capteurs éventuellement absentes
} msg_type_t;

// Fused message
// ⚠️ Cette structure est copiée octet par octet dans une queue FreeRTOS :
// elle ne doit contenir que des types "plain old data" (pas de std::string).
typedef struct {
    char sensor_id[16];
    msg_type_t msg_type;
    bool gnss_valid;     // true si le bloc gnss contient une donnée reçue
    bool spectro_valid;  // true si le bloc spectro contient une donnée reçue
    uint64_t ts_ms;
    gnss_snapshot_t gnss;
    spectro_sample_t spectro;
    int16_t delta_time_gnss_spectro_ms;
    uint32_t message_id;

} fused_message_t;