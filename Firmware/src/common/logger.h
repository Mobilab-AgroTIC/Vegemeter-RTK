#include "config.h"

// Gestion des messages d'erreur

// 0=ONLY_HEALTH 1=ERROR 2=WARNING 3=INFO 4=DEBUG
// USAGE : LOG_INFO("My text %d", val);
#define LOG_HEALTH(fmt, ...)   do { if (LOG_LEVEL >= 0) Serial.printf("[H] " fmt "\n", ##__VA_ARGS__); } while(0)
#define LOG_ERROR(fmt, ...)   do { if (LOG_LEVEL >= 1) Serial.printf("[E] " fmt "\n", ##__VA_ARGS__); } while(0)
#define LOG_WARN(fmt, ...)    do { if (LOG_LEVEL >= 2) Serial.printf("[W] " fmt "\n", ##__VA_ARGS__); } while(0)
#define LOG_INFO(fmt, ...)    do { if (LOG_LEVEL >= 3) Serial.printf("[I] " fmt "\n", ##__VA_ARGS__); } while(0)
#define LOG_DEBUG(fmt, ...)   do { if (LOG_LEVEL >= 4) Serial.printf("[D] " fmt "\n", ##__VA_ARGS__); } while(0)