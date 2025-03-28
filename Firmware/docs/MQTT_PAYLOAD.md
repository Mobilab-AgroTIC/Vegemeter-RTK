# Format de la trame MQTT par version

Toutes les versions publient sur le topic **`esp32/update`** une trame **JSON**.
Ce document décrit, pour chaque version du firmware, les champs envoyés, leur
type, leur unité, et comment le serveur peut reconnaître la version d'une trame.

| Version | Bibliothèque MQTT | QoS | Structure | `lat` / `lon` | Altitude |
|---------|-------------------|-----|-----------|---------------|----------|
| v2.0.0  | PubSubClient      | 0   | plate, clés d'une lettre | degrés décimaux (`y` / `x`) | m (`z`) |
| v2.1.0  | AsyncMqttClient   | 1   | imbriquée (`gnss`, `spectro`) | entier ×1e-7 deg, **écrit en flottant** | mm, flottant |
| v2.2.0  | AsyncMqttClient   | 1   | identique à v2.1.0 | identique à v2.1.0 | identique |
| v2.2.1  | AsyncMqttClient   | 1   | identique à v2.1.0 | entier ×1e-7 deg, **écrit en entier** | mm, entier |
| v2.2.2  | AsyncMqttClient   | 1   | v2.2.1 + `type`, `gnss_valid`, `spectro_valid`, `diag` | identique à v2.2.1 | identique |

## Reconnaître la version d'une trame côté serveur

Il n'existe pas (encore) de champ « version du firmware » dans la trame.
Le `sensor_id` contient bien une chaîne de type `V2.x_By`, mais elle est
saisie à la main à chaque flash et **n'est pas fiable** (la v2.2.0 envoie par
exemple `"V2.1_B1"`).

Arbre de décision recommandé :

```
la clé "i" existe ?                       -> v2.0.0
sinon la clé "type" existe ?              -> v2.2.2 (ou ultérieure)
sinon                                     -> v2.1.0 / v2.2.0 / v2.2.1
```

v2.1.0, v2.2.0 et v2.2.1 ont **exactement les mêmes clés**. Seule la façon
d'écrire `gnss.lat`, `gnss.lon` et `gnss.hMSL` change (voir plus bas). Une fois
le JSON décodé, les valeurs sont les mêmes : un serveur qui convertit
`lat * 1e-7` traite les trois versions de la même façon.

> Recommandation : ajouter dans une prochaine version un champ explicite
> (par ex. `"fw": "2.2.3"`) et séparer l'identifiant matériel du boîtier
> (`sensor_id`) de la version du firmware.

## Points communs à toutes les versions

- **Bandes spectrales** (capteur AS7265x, 18 canaux) : clés `R S T U V W G H I J K L A B C D E F`.
  Correspondance longueurs d'onde (datasheet AS7265x / librairie SparkFun) :

  | Puce | Clés → longueur d'onde (nm) |
  |------|-----------------------------|
  | AS72653 (UV)  | A 410, B 435, C 460, D 485, E 510, F 535 |
  | AS72652 (VIS) | G 560, H 585, I 645, J 705, K 900, L 940 |
  | AS72651 (NIR) | R 610, S 680, T 730, U 760, V 810, W 860 |

- **Label** (boutons physiques) : `0` = aucun, `1` = bouton label 1 (GPIO 32), `2` = bouton label 2 (GPIO 33).
- **Bouton d'envoi** (GPIO 34) : si inactif, la trame n'est pas publiée. Le
  compteur de messages continue malgré tout à s'incrémenter : **un trou dans
  `message_id` n'indique pas une perte réseau**.
- Les compteurs (`m`, `message_id`) et horodatages internes (`ts`, `ts_ms`,
  `uptime_ms`) **repartent de 0 à chaque redémarrage** du boîtier.
- `fixtype` / `f` : type de fix u-blox (`0` pas de fix, `2` 2D, `3` 3D, `4` GNSS+DR…).
- `carrsoln` / `k` : solution RTK u-blox (`0` aucune, `1` RTK flottant, `2` RTK fixe).

---

## v2.0.0

Trame plate, clés d'une lettre. Envoyée à chaque solution GNSS (NAV-PVT) si le
bouton d'envoi est actif.

```json
{
  "i": "bbsocoul_1",
  "m": 1532,
  "y": 43.6170123,
  "x": 3.8712345,
  "z": 52.345,
  "h": 14,
  "f": 3,
  "k": 2,
  "t": 31,
  "d": "2025-03-28 09:41:27.300",
  "s": 1,
  "n": 0,
  "R": 1234.56, "S": 987.12, "T": 456.78, "U": 321.09, "V": 210.98, "W": 198.76,
  "G": 876.54, "H": 765.43, "I": 654.32, "J": 543.21, "K": 432.1,  "L": 321.0,
  "A": 210.12, "B": 309.87, "C": 408.76, "D": 507.65, "E": 606.54, "F": 705.43
}
```

| Clé | Type | Unité | Description |
|-----|------|-------|-------------|
| `i` | string | | identifiant du boîtier |
| `m` | int | | compteur de messages (incrémenté même si non envoyé) |
| `y` | float | degrés | latitude |
| `x` | float | degrés | longitude |
| `z` | float | m | altitude au-dessus du niveau moyen de la mer |
| `h` | int | mm | précision horizontale estimée (hAcc) |
| `f` | int | | type de fix |
| `k` | int | | solution RTK (carrSoln) |
| `t` | int | ms | durée entre la réception GNSS et la lecture spectro |
| `d` | string | UTC | date GNSS `AAAA-MM-JJ hh:mm:ss.mmm` |
| `s` | int | | connexion NTRIP (non mis à jour dans le code : toujours `0`) |
| `n` | int | | label (0, 1, 2) |
| `R`…`F` | float | comptes | 18 bandes spectrales |

⚠️ Si le capteur spectral renvoie une erreur, les 18 bandes contiennent des
valeurs non initialisées (aléatoires).

## v2.1.0

Nouvelle trame imbriquée, **incompatible** avec v2.0.0.

```json
{
  "sensor_id": "V2.1_B7",
  "ts": 812345,
  "delta_gnss_spectro_ms": 12,
  "message_id": 4521,
  "gnss": {
    "ts_ms": 812320,
    "lat": 4.36170123e8,
    "lon": 3.8712345e7,
    "hMSL": 52345,
    "hdop": 0,
    "hAcc": 14,
    "fixtype": 3,
    "carrsoln": 2,
    "fix": 0,
    "gSpeed": 1250,
    "gnss_date": { "year": 2025, "month": 11, "day": 4, "hour": 9, "min": 41, "sec": 27, "ms": 300 }
  },
  "spectro": {
    "ts_ms": 812332,
    "label": 0,
    "R": 1234.56, "S": 987.12, "T": 456.78, "U": 321.09, "V": 210.98, "W": 198.76,
    "G": 876.54, "H": 765.43, "I": 654.32, "J": 543.21, "K": 432.1,  "L": 321.0,
    "A": 210.12, "B": 309.87, "C": 408.76, "D": 507.65, "E": 606.54, "F": 705.43
  }
}
```

| Clé | Type | Unité | Description |
|-----|------|-------|-------------|
| `sensor_id` | string | | identifiant (`BBSOCOUL_ID` dans `config.h`) |
| `ts` | int | ms depuis le démarrage | instant de la fusion |
| `delta_gnss_spectro_ms` | int (≥ 0) | ms | `spectro.ts_ms - gnss.ts_ms` |
| `message_id` | int | | compteur de messages fusionnés |
| `gnss.ts_ms` | int | ms depuis le démarrage | réception de la solution GNSS |
| `gnss.lat` | nombre | 1e-7 degré | latitude ×10⁷ (diviser par 10 000 000) |
| `gnss.lon` | nombre | 1e-7 degré | longitude ×10⁷ |
| `gnss.hMSL` | nombre | mm | altitude au-dessus du niveau moyen de la mer |
| `gnss.hdop` | float | | **jamais renseigné : toujours 0** |
| `gnss.hAcc` | int | mm | précision horizontale estimée |
| `gnss.fixtype` | int | | type de fix |
| `gnss.carrsoln` | int | | solution RTK |
| `gnss.fix` | int | | **jamais renseigné : toujours 0** (utiliser `fixtype`) |
| `gnss.gSpeed` | int | mm/s | vitesse sol |
| `gnss.gnss_date.*` | int | UTC | date/heure GNSS, `ms` = millisecondes |
| `spectro.ts_ms` | int | ms depuis le démarrage | lecture du capteur spectral |
| `spectro.label` | int | | label (0, 1, 2) |
| `spectro.R`…`F` | float | comptes | 18 bandes spectrales |

Remarques :
- `lat`, `lon` et `hMSL` sont des entiers u-blox stockés en `double` : ArduinoJson
  les écrit en **notation scientifique** dès qu'ils dépassent 10⁷
  (`4.36170123e8`). La valeur décodée est correcte, mais un parseur qui attend
  un entier peut échouer.
- La fusion attend une nouvelle position GNSS mais réutilise le dernier
  échantillon spectral : **une même mesure spectrale peut apparaître dans
  plusieurs trames consécutives**.

## v2.2.0

**Mêmes clés, même ordre, mêmes unités que v2.1.0.** Changements de comportement :

- `delta_gnss_spectro_ms` devient **signé** (peut être négatif) : la fusion
  associe désormais la position GNSS et la mesure spectrale les plus proches
  dans le temps, dans un sens ou dans l'autre.
- Chaque mesure spectrale n'est plus utilisée qu'une seule fois.
- Si le broker est injoignable, les trames sont mises en file (jusqu'à 500) et
  renvoyées à la reconnexion : **des trames peuvent arriver en rafale et en
  retard**. Avec le QoS 1, un doublon reste possible : dédupliquer sur
  (`sensor_id`, `message_id`, `ts`).
- `sensor_id` du firmware publié : `"V2.1_B1"` (et non `V2.2`).

## v2.2.1

**Mêmes clés que v2.1.0 / v2.2.0.** Seul changement : `gnss.lat`, `gnss.lon` et
`gnss.hMSL` sont désormais des **entiers** dans le JSON :

```json
"lat": 436170123,
"lon": 38712345,
"hMSL": 52345,
```

Les unités ne changent pas (1e-7 degré et mm).

## v2.2.2 (en cours de test)

Ajout de champs à la racine et d'un type de message `heartbeat`. Les blocs
`gnss` et `spectro` sont inchangés par rapport à v2.2.1.

```json
{
  "sensor_id": "V2.2_B10",
  "type": "fused",
  "ts": 812345,
  "delta_gnss_spectro_ms": -8,
  "message_id": 4521,
  "gnss_valid": true,
  "spectro_valid": true,
  "diag": { "uptime_ms": 812350, "rssi": -67, "free_heap": 182344 },
  "gnss": { "...": "identique à v2.2.1" },
  "spectro": { "...": "identique à v2.2.1" }
}
```

| Clé ajoutée | Type | Unité | Description |
|-------------|------|-------|-------------|
| `type` | string | | `"fused"` (GNSS + spectro) ou `"heartbeat"` |
| `gnss_valid` | bool | | le bloc `gnss` contient une vraie mesure |
| `spectro_valid` | bool | | le bloc `spectro` contient une vraie mesure |
| `diag.uptime_ms` | int | ms | temps depuis le démarrage, au moment de l'envoi |
| `diag.rssi` | int | dBm | force du signal WiFi |
| `diag.free_heap` | int | octets | mémoire libre |

**Heartbeat** : si aucune fusion GNSS + spectro n'a eu lieu depuis 2 s, le
boîtier envoie un `heartbeat` contenant ce qui est disponible. Un bloc dont le
flag `*_valid` est `false` est **rempli de zéros** (`lat: 0`, `lon: 0`,
date `0`…) : le serveur doit filtrer ces trames et ne pas les traiter comme des
mesures. Les heartbeats sont envoyés **même si le bouton d'envoi est inactif**.
