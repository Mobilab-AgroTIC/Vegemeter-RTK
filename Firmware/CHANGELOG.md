# Changelog

Le détail de la trame MQTT de chaque version est dans
[docs/MQTT_PAYLOAD.md](docs/MQTT_PAYLOAD.md).

Légende : 🔴 = change la trame MQTT (adaptation serveur nécessaire) ·
🟠 = même trame, comportement d'envoi différent.

## [2.2.2] – à venir

> Branche `V2.2.2_send_empty_data`, en cours de test.

### Trame MQTT
- 🔴 Nouveaux champs à la racine : `type` (`"fused"` / `"heartbeat"`),
  `gnss_valid`, `spectro_valid`, et bloc `diag` (`uptime_ms`, `rssi`, `free_heap`).
- 🔴 Nouveau message **heartbeat** : si aucune fusion GNSS + spectro depuis 2 s,
  une trame est envoyée avec les données disponibles ; les blocs absents sont
  remplis de zéros (`*_valid = false`). Envoyée même si le bouton d'envoi est inactif.
- Les blocs `gnss` et `spectro` sont inchangés par rapport à 2.2.1.

### Autres changements
- Temps d'intégration du capteur spectral configurable (`SPECTRO_INTEGRATION_TIME_LOOP`)
  et envoyé au démarrage (`ATINTTIME`), avec vérification de la réponse `AT`.
- Une trame spectrale incomplète est rejetée au lieu de produire des valeurs partielles.
- Les trames trop grandes sont abandonnées au lieu d'être réessayées indéfiniment.
- Journalisation revue : bloc `STATUS` périodique, messages en français,
  limitation des erreurs répétitives (`LOG_THROTTLE_MS`).
- `platformio.ini` nettoyé (environnements inutilisés supprimés) ; programme de
  test matériel du spectro (`src/tests_hw/spectro_test.cpp`, env `spectro_test`).
- `src/common/config.h` n'est plus versionné (il contient les identifiants WiFi / MQTT) : copier `src/common/config.exemple.h` en `config.h` puis le compléter.

## [2.2.1] – 2026-05-13

### Trame MQTT
- 🔴 `gnss.lat`, `gnss.lon` et `gnss.hMSL` passent de `double` à `int32_t` :
  ils sont écrits en **entiers** (`436170123`) et non plus en notation
  scientifique (`4.36170123e8`). Mêmes clés, mêmes unités (1e-7 degré, mm).
  Corrige les latitudes aberrantes (validé sur un tour de stade).

### Autres changements
- `sensor_id` : `V2.2_B8`.

## [2.2.0] – 2026-05-13

### Trame MQTT
- Mêmes clés et mêmes unités que 2.1.0.
- 🟠 `delta_gnss_spectro_ms` devient signé (peut être négatif).
- 🟠 Fusion revue : association de la position GNSS et de la mesure spectrale
  les plus proches dans le temps ; une mesure spectrale n'est plus réutilisée
  dans plusieurs trames.
- 🟠 En cas de coupure du broker, les trames sont conservées (file de 500
  messages) et renvoyées à la reconnexion : arrivées en retard et en rafale
  possibles, doublons possibles (QoS 1).
- 🟠 `sensor_id` du firmware publié : `V2.1_B1` (ne reflète pas la version).

### Autres changements
- Système de journalisation centralisé avec niveaux (`LOG_LEVEL`).
- Trame GGA envoyée au caster NTRIP générée directement depuis NAV-PVT ;
  la tâche GGA est supprimée et intégrée à la tâche NTRIP.
- Reconnexion automatique au caster NTRIP, lectures non bloquantes
  (corrections des blocages de tâches).
- MQTT : keep-alive 15 s, buffer de sérialisation statique.
- Répartition des tâches FreeRTOS entre les deux cœurs revue.

## [2.1.0] – 2025-11-04

### Trame MQTT
- 🔴 **Nouveau format, incompatible avec 2.0.0** : trame imbriquée
  `{ sensor_id, ts, delta_gnss_spectro_ms, message_id, gnss{…}, spectro{…} }`.
- 🔴 Position transmise en unités u-blox brutes : `lat`/`lon` en 1e-7 degré,
  `hMSL` en mm (au lieu de degrés et mètres).
- 🔴 Nouveaux champs : `gSpeed` (mm/s), date GNSS détaillée (`gnss_date`),
  horodatages internes GNSS et spectro.
- Champs `hdop` et `fix` présents mais jamais renseignés (toujours 0).
- 🟠 Publication en QoS 1 (AsyncMqttClient) au lieu de QoS 0.

### Autres changements
- Réécriture complète sur FreeRTOS : tâches GNSS, spectro, fusion, MQTT, NTRIP,
  GGA et santé, communiquant par files.
- Configuration centralisée dans `src/common/config.h`.
- Liaison F9P à 460 800 bauds ; trame GGA reconstruite depuis UBX (GNGGA) pour le caster.
- WiFi et MQTT asynchrones avec reconnexion sur événement.

## [2.0.0] – 2025-03-28

Première version publiée.

### Trame MQTT
- Trame JSON plate à clés d'une lettre (`i, m, y, x, z, h, f, k, t, d, s, n`
  + 18 bandes spectrales), topic `esp32/update`, QoS 0.

### Fonctionnalités
- ESP32 + récepteur GNSS u-blox F9P avec corrections RTK NTRIP (réseau Centipede,
  point de montage `NEAR` : base la plus proche).
- Spectromètre AS7265x (18 bandes) lu en UART par commandes AT.
- Boutons : envoi des données, label 1, label 2.
- Suppression de la lecture série brute et de la mesure de température.
