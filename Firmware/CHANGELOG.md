# Changelog

Le détail de la trame MQTT de chaque version est dans
[docs/MQTT_PAYLOAD.md](docs/MQTT_PAYLOAD.md).

Légende : 🔴 = change la trame MQTT (adaptation serveur nécessaire) ·
🟠 = même trame, comportement d'envoi différent.

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
