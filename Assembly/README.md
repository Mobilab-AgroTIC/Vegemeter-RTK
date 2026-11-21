# Tutoriel d'assemblage du Végémètre-RTK

Ce tutoriel vous permet d'assembler tous les composants pour obtenir un Végémètre-RTK, depuis la commande du matériel jusqu'au premier démarrage.

> 📷 Les emplacements marqués **« Photo à ajouter »** ou **« Capture à ajouter »** attendent une image. Déposez-la dans [media/](media/) avec le nom indiqué, puis remplacez la ligne par la balise `<img>` donnée en commentaire juste en dessous.

## Sommaire

1. [Contenu du dossier](#1-contenu-du-dossier)
2. [Matériel nécessaire](#2-matériel-nécessaire)
3. [Outillage](#3-outillage)
4. [Impression du boîtier](#4-impression-du-boîtier)
5. [Préparation et soudure des composants](#5-préparation-et-soudure-des-composants)
6. [Câblage](#6-câblage)
7. [Mise à jour du firmware de la F9P](#7-mise-à-jour-du-firmware-de-la-f9p)
8. [Téléversement du programme sur la Lilygo](#8-téléversement-du-programme-sur-la-lilygo)
9. [Montage dans le boîtier](#9-montage-dans-le-boîtier)
10. [Premier démarrage et vérifications](#10-premier-démarrage-et-vérifications)

---

## 1. Contenu du dossier

| Dossier / fichier | Contenu |
|---|---|
| [F9P_firmware/u-center_v22.07.exe](F9P_firmware/u-center_v22.07.exe) | Logiciel u-blox u-center **v22.07** (Windows) pour configurer et mettre à jour la F9P |
| [F9P_firmware/UBX_F9_100_HPG132.df73486d99374142f3aabf79b7178f48.bin](F9P_firmware/UBX_F9_100_HPG132.df73486d99374142f3aabf79b7178f48.bin) | Firmware u-blox ZED-F9P **HPG 1.32** |
| [STL/](STL/) | Fichiers `.stl` d'impression 3D du boîtier |
| [media/](media/) | Photos et captures d'écran de ce tutoriel |

## 2. Matériel nécessaire

| # | Composant | Qté | Fournisseur | Remarque |
|---|---|---|---|---|
| 1 | Lilygo T-PCIE v1.2 (AXP2101, ESP32-WROVER 16 Mo) | 1 | [TinyTronics](https://www.tinytronics.nl/nl/development-boards/microcontroller-boards/met-wi-fi/lilygo-t-pcie-v1.2-axp2101-esp32-wrover-16mb) | Carte mère : ESP32 + slot mini PCIe |
| 2 | Récepteur GNSS RTK u-blox ZED-F9P au format mini PCIe (ex-Drotek **DP0602**) | 1 | [Siderion – récepteurs GNSS](https://siderion.io/collections/gnss-receivers) · [ancienne fiche Drotek DP0602](https://store-drotek.com/923-DP0602.html) | ⚠️ La DP0602 est annoncée en fin de vie et n'apparaît pas au catalogue Siderion. Vérifier la disponibilité ou un équivalent mPCIe auprès de Siderion |
| 3 | Antenne GNSS multibande L1/L2 Siderion **SA233** (ex-Drotek DA233) | 1 | [Siderion – SA233](https://siderion.io/products/sa233-multiband-rtk-gnss-antenna) | [Documentation](https://siderion.gitbook.io/siderion-docs/KDjA5svYRpvM3ouQSdFC/antenna/sa233) · [Datasheet](https://siderion-sas.github.io/SiderionDocs/SiderionDocs_SA233.pdf) |
| 4 | Câble d'antenne U.FL → SMA femelle | 1 | [TinyTronics](https://www.tinytronics.nl/en/cables-and-connectors/cables-and-adapters/others/sma-female-to-ufl-female-cable) · [alternative MHZshop](https://www.mhzshop.com/shop/Cables-et-cordons/Cordons-Radio/SMA/Cordon-UFL-vers-SMA-Femelle-chassis-12-cm-1-37mm-Embase-SMA-hexagonale-11mm.html) | Relie la F9P à l'embase SMA du boîtier |
| 5 | Capteur spectral SparkFun Triad AS7265x (SEN-15050) | 1 | [Digikey](https://www.digikey.fr/fr/products/detail/sparkfun-electronics/SEN-15050/9739876) | 18 bandes (410 → 940 nm) |
| 6 | Port USB-C femelle vers DIP (à souder) | 1 | [TinyTronics](https://www.tinytronics.nl/nl/kabels-en-connectoren/connectoren/usb/usb-c-2.0-female-naar-dip-adapter) | Alimentation depuis l'extérieur du boîtier |
| 7 | Batterie Li-Po 3,7 V 2000 mAh | 1 | [TinyTronics](https://www.tinytronics.nl/nl/power/batterijen/li-po/li-po-accu-3.7v-2000mah) | Référence à vérifier |
| 8 | Interrupteur 3 positions (bouton de labellisation) | 1 | *Référence à trouver* | Positions 0 / 1 / 2 |
| 9 | Interrupteur 2 positions (envoi des données) | 1 | *Référence à trouver* | |
| 10 | Broches mâles au pas de 2,54 mm | — | *Référence à trouver* | Non fournies avec le port USB-C |
| 11 | Fils de câblage (type Dupont / silicone) | — | *Référence à trouver* | |
| 12 | Visserie du boîtier | — | *Référence à trouver* | À préciser avec les fichiers STL |

📷 *Photo à ajouter : l'ensemble du matériel étalé avant assemblage — `media/01_materiel.jpg`*
<!-- <img src="media/01_materiel.jpg" alt="Matériel nécessaire" style="width: 60%; height: auto;" /> -->

## 3. Outillage

- Fer à souder, étain
- Ordinateur **Windows** (pour u-center) avec [VS Code](https://code.visualstudio.com/) et l'extension [PlatformIO](https://platformio.org/)
- Câble USB-C de données (pas un câble de charge seule)
- Imprimante 3D (ou service d'impression)
- Tournevis, pince coupante

## 4. Impression du boîtier

Les fichiers d'impression sont dans le dossier [STL/](STL/).

| Fichier | Pièce | Qté | Remarque |
|---|---|---|---|
| *à compléter* | | | |

Paramètres d'impression conseillés : *à compléter (matériau, remplissage, hauteur de couche, supports).*

📷 *Photo à ajouter : les pièces imprimées — `media/02_boitier_imprime.jpg`*
<!-- <img src="media/02_boitier_imprime.jpg" alt="Pièces du boîtier imprimées" style="width: 60%; height: auto;" /> -->

## 5. Préparation et soudure des composants

### 5.1 Lilygo T-PCIE

- Souder les broches sur la carte Lilygo T-PCIE comme sur la photo, de manière à avoir les petites extrémités du côté du slot mPCIe.

📷 *Photo à ajouter : broches soudées sur la Lilygo, côté slot mPCIe — `media/03_lilygo_broches.jpg`*
<!-- <img src="media/03_lilygo_broches.jpg" alt="Broches soudées sur la Lilygo T-PCIE" style="width: 50%; height: auto;" /> -->

### 5.2 Port USB-C

- Souder des broches (non fournies par TinyTronics) sur le module USB-C, du côté du port USB.

📷 *Photo à ajouter : module USB-C avec broches soudées — `media/04_usbc_broches.jpg`*
<!-- <img src="media/04_usbc_broches.jpg" alt="Module USB-C avec broches" style="width: 40%; height: auto;" /> -->

### 5.3 Capteur AS7265x

Le firmware communique avec le capteur spectral **en liaison série (UART)**, et non en I²C. *À compléter : préparation du capteur (cavaliers / pastilles à modifier pour passer en mode UART, broches à souder).*

📷 *Photo à ajouter : capteur AS7265x préparé — `media/05_as7265x.jpg`*
<!-- <img src="media/05_as7265x.jpg" alt="Capteur AS7265x préparé" style="width: 40%; height: auto;" /> -->

## 6. Câblage

### 6.1 Installation de la F9P

- Insérer la carte F9P dans le slot mini PCIe de la Lilygo puis la visser.
- Connecter le câble U.FL → SMA sur le connecteur d'antenne de la F9P.

La liaison série F9P ↔ ESP32 passe par le slot mPCIe : il n'y a rien à câbler.

📷 *Photo à ajouter : F9P insérée dans le slot et câble U.FL branché — `media/06_f9p_slot.jpg`*
<!-- <img src="media/06_f9p_slot.jpg" alt="F9P dans le slot mPCIe" style="width: 50%; height: auto;" /> -->

### 6.2 Brochage

Les broches sont définies dans [Firmware/src/common/config.exemple.h](../Firmware/src/common/config.exemple.h).

| Élément | Broche ESP32 (GPIO) | Remarque |
|---|---|---|
| AS7265x – RX de l'ESP32 (vers TX du capteur) | 13 | UART 115200 bauds |
| AS7265x – TX de l'ESP32 (vers RX du capteur) | 15 | |
| AS7265x – alimentation | 3V3 / GND | |
| Interrupteur « envoi des données » | 34 | Contact vers GND. ⚠️ Le GPIO 34 n'a pas de pull-up interne : vérifier la présence d'une résistance de tirage vers 3V3 |
| Interrupteur de labellisation – position 1 | 32 | Contact vers GND (pull-up interne) |
| Interrupteur de labellisation – position 2 | 33 | Contact vers GND (pull-up interne) |
| F9P – UART (via mPCIe) | 26 (TX) / 27 (RX) | Aucun câblage, 460800 bauds |
| F9P – alimentation (via mPCIe) | 25 | Aucun câblage |
| Port USB-C | *à compléter* | *à compléter* |
| Batterie | Connecteur batterie de la Lilygo | |

Position 0 « Garbage » du bouton de labellisation : ni la broche 32 ni la broche 33 ne sont reliées à GND.

📷 *Schéma ou photo à ajouter : câblage complet — `media/07_cablage.jpg`*
<!-- <img src="media/07_cablage.jpg" alt="Schéma de câblage" style="width: 70%; height: auto;" /> -->

## 7. Mise à jour du firmware de la F9P

La F9P doit être mise à jour avec le firmware HPG 1.32, puis configurée en **460800 bauds** sur l'UART1 : c'est la vitesse attendue par le programme de la Lilygo (`GNSS_BAUDRATE`).

Fichiers nécessaires (dossier [F9P_firmware/](F9P_firmware/)) :
- `u-center_v22.07.exe` : **utiliser cette version de u-center** (v22.07)
- `UBX_F9_100_HPG132.df73486d99374142f3aabf79b7178f48.bin` : le firmware

### 7.1 Connexion à u-center

- Installer u-center v22.07.
- Connecter la F9P au PC. *À compléter : méthode de connexion (port USB de la F9P, ou à travers la Lilygo).*
- Dans u-center, sélectionner le port COM et le débit, puis vérifier que la F9P renvoie des messages.

📷 *Capture à ajouter : u-center connecté à la F9P — `media/08_ucenter_connexion.png`*
<!-- <img src="media/08_ucenter_connexion.png" alt="Connexion u-center" style="width: 70%; height: auto;" /> -->

### 7.2 Flasher le firmware

- Menu **Tools > Firmware Update...**
- Dans **Firmware image**, sélectionner le fichier `.bin`.
- *À compléter : options à cocher.*
- Cliquer sur **GO** (flèche verte en bas à gauche) et attendre la fin de la mise à jour.

📷 *Capture à ajouter : fenêtre Firmware Update — `media/09_ucenter_firmware_update.png`*
<!-- <img src="media/09_ucenter_firmware_update.png" alt="Firmware Update u-center" style="width: 70%; height: auto;" /> -->

### 7.3 Régler le débit à 460800 bauds

- Aller dans **View > Configuration View > PRT (Ports)**, sélectionner **UART1** et régler le baudrate sur **460800**, puis cliquer sur **Send**.

<img width="1487" height="803" alt="Configuration du port UART1 à 460800 bauds" src="https://github.com/user-attachments/assets/9d1c03a0-4b20-409b-908a-bdcbf3a7ac59" />

### 7.4 Sauvegarder la configuration

- Penser à sauvegarder la configuration dans **CFG (Configuration)** > **Save current configuration**, puis **Send**. Sinon le réglage est perdu au prochain redémarrage.

<img width="1486" height="831" alt="Sauvegarde de la configuration dans CFG" src="https://github.com/user-attachments/assets/30a760d7-425a-4e34-887e-57a27e809133" />

## 8. Téléversement du programme sur la Lilygo

Le programme se trouve dans le dossier [Firmware/](../Firmware/).

1. Ouvrir le dossier `Firmware/` dans VS Code avec PlatformIO.
2. Copier `src/common/config.exemple.h` en `src/common/config.h` et renseigner :
   - `BBSOCOUL_ID` : l'identifiant du Végémètre ;
   - `WIFI_SSID` / `WIFI_PASSWORD` : le Wi-Fi (partage de connexion) ;
   - `MQTT_BROKER`, `MQTT_USER`, `MQTT_PASSWORD` : la connexion à la console Mobilab ;
   - les paramètres `NTRIP_*` sont déjà réglés pour Centipède-RTK (`crtk.net`, point de montage `NEAR`) : ne les modifier que pour changer de base.
3. Brancher la Lilygo en USB.
4. Sélectionner l'environnement `LilyGo-T-PCIE` puis cliquer sur **Upload**.
5. Ouvrir le **moniteur série** (115200 bauds) pour vérifier le démarrage.

📷 *Capture à ajouter : téléversement réussi dans PlatformIO — `media/10_platformio_upload.png`*
<!-- <img src="media/10_platformio_upload.png" alt="Téléversement PlatformIO" style="width: 70%; height: auto;" /> -->

📷 *Capture à ajouter : moniteur série au démarrage — `media/11_moniteur_serie.png`*
<!-- <img src="media/11_moniteur_serie.png" alt="Moniteur série" style="width: 70%; height: auto;" /> -->

## 9. Montage dans le boîtier

*À compléter : ordre de montage.*

- Fixer l'embase SMA du câble d'antenne sur le boîtier.
- Installer le capteur AS7265x face à la fenêtre de mesure.
- Installer les interrupteurs de labellisation et d'envoi des données.
- Installer le port USB-C.
- Installer la batterie et la Lilygo, puis fermer le boîtier.
- Visser l'antenne SA233 sur l'embase SMA.

📷 *Photo à ajouter : intérieur du boîtier avec les composants en place — `media/12_montage_interieur.jpg`*
<!-- <img src="media/12_montage_interieur.jpg" alt="Intérieur du boîtier" style="width: 60%; height: auto;" /> -->

📷 *Photo à ajouter : Végémètre-RTK fermé — `media/13_vegemetre_final.jpg`*
<!-- <img src="media/13_vegemetre_final.jpg" alt="Végémètre-RTK assemblé" style="width: 60%; height: auto;" /> -->

## 10. Premier démarrage et vérifications

Suivre le [guide de prise en main](../README.md), puis vérifier sur la console Mobilab :

- [ ] les données arrivent (pas de « No data ») ;
- [ ] le Fixtype passe en **RTK fix** en extérieur dégagé ;
- [ ] le **Button state** suit les 3 positions du bouton de labellisation ;
- [ ] le **NDVI** est cohérent (plus élevé sur la végétation que sur le sol nu) ;
- [ ] l'interrupteur « envoi des données » coupe et rétablit bien l'envoi.
