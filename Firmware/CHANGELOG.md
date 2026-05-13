# Changelog

Le détail de la trame MQTT de chaque version est dans
[docs/MQTT_PAYLOAD.md](docs/MQTT_PAYLOAD.md).

Légende : 🔴 = change la trame MQTT (adaptation serveur nécessaire) ·
🟠 = même trame, comportement d'envoi différent.

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
