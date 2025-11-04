#pragma once
#include <stdint.h>
#include <string>

// Structures de données échangées entre tâches

// GNSS snapshot
typedef struct {
    uint64_t ts_ms;
    double lat, lon, hMSL;
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

// Fused message
typedef struct {
    std::string sensor_id;
    uint64_t ts_ms;
    gnss_snapshot_t gnss;
    spectro_sample_t spectro;
    uint16_t delta_time_gnss_spectro_ms;
    uint32_t message_id;

} fused_message_t;