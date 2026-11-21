#pragma once
#include <Arduino.h>
#include "config.h"

// ============================================================================
// Gestion des logs sur le moniteur série
// ============================================================================
//
// Niveaux (LOG_LEVEL dans config.h) :
//   0 = NONE    : uniquement le bandeau de démarrage et le bloc STATUS périodique
//   1 = ERROR   : + défaillances (capteur absent, perte de données...)
//   2 = WARNING : + situations anormales mais récupérables (déconnexion, pas de fix...)
//   3 = INFO    : + changements d'état (connexion, fix obtenu, capteur qui répond à nouveau...)
//   4 = DEBUG   : + tout le détail, y compris les messages à haute fréquence (10 Hz)
//
// Format d'une ligne :
//   [    12.345] WARN  [F9P] Pas de fix GNSS valide
//    ^ uptime (s)  ^ niveau
//
// Règles d'usage :
//   - un log = une ligne, sans "\n" final (ajouté automatiquement)
//   - ce qui se répète à haute fréquence va en DEBUG
//   - une erreur qui se répète utilise LOG_THROTTLED pour ne pas noyer le moniteur
//   - les uint64_t (ts_ms) doivent être castés : (unsigned long)x avec %lu
//
// USAGE : LOG_INFO("[TAG] My text %d", val);

#define LOG_LEVEL_NONE    0
#define LOG_LEVEL_ERROR   1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_INFO    3
#define LOG_LEVEL_DEBUG   4

#ifndef LOG_THROTTLE_MS
#define LOG_THROTTLE_MS 10000
#endif

// Un seul appel à Serial.printf par ligne : la ligne reste entière même
// si plusieurs tâches FreeRTOS écrivent en même temps.
#define LOG_PRINT_(tag, fmt, ...) do {                                         \
        uint32_t _log_t = millis();                                            \
        Serial.printf("[%6lu.%03lu] " tag " " fmt "\n",                      \
                      (unsigned long)(_log_t / 1000),                          \
                      (unsigned long)(_log_t % 1000), ##__VA_ARGS__);          \
    } while (0)

// Toujours affiché, quel que soit LOG_LEVEL (bandeau, bloc STATUS)
#define LOG_HEALTH(fmt, ...)  LOG_PRINT_("STAT ", fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)   do { if (LOG_LEVEL >= LOG_LEVEL_ERROR)   LOG_PRINT_("ERROR", fmt, ##__VA_ARGS__); } while (0)
#define LOG_WARN(fmt, ...)    do { if (LOG_LEVEL >= LOG_LEVEL_WARNING) LOG_PRINT_("WARN ", fmt, ##__VA_ARGS__); } while (0)
#define LOG_INFO(fmt, ...)    do { if (LOG_LEVEL >= LOG_LEVEL_INFO)    LOG_PRINT_("INFO ", fmt, ##__VA_ARGS__); } while (0)
#define LOG_DEBUG(fmt, ...)   do { if (LOG_LEVEL >= LOG_LEVEL_DEBUG)   LOG_PRINT_("DEBUG", fmt, ##__VA_ARGS__); } while (0)

// Log limité en fréquence : au plus un message toutes les `period_ms`
// pour cet emplacement du code. Le premier appel est toujours affiché.
// Exemple : LOG_THROTTLED(LOG_WARN, 10000, "[MQTT] %u messages en attente", n);
#define LOG_THROTTLED(LOG_MACRO, period_ms, fmt, ...) do {                    \
        static uint32_t _log_last = 0;                                         \
        static bool _log_first = true;                                         \
        uint32_t _log_now = millis();                                          \
        if (_log_first || (_log_now - _log_last) >= (uint32_t)(period_ms)) {   \
            _log_first = false;                                                \
            _log_last = _log_now;                                              \
            LOG_MACRO(fmt, ##__VA_ARGS__);                                     \
        }                                                                      \
    } while (0)

inline const char* log_level_name(int level) {
    switch (level) {
        case LOG_LEVEL_NONE:    return "NONE";
        case LOG_LEVEL_ERROR:   return "ERROR";
        case LOG_LEVEL_WARNING: return "WARNING";
        case LOG_LEVEL_INFO:    return "INFO";
        case LOG_LEVEL_DEBUG:   return "DEBUG";
        default:                return "?";
    }
}
