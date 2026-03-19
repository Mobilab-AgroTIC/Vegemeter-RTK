<h1>Prendre en main le Végémèter-RTK</h1>
Le Végémètre RTK est un capteur piéton qui permet de réaliser des mesures spectrales géoréférencées au champs. Le capteur réalise 10 mesures par seconde et enregistre simultanemenent les données GNSS de position, altitude, précision ainsi que les 18 bandes spectrales (visible et proche infrarouge) produite par le capteur AS7265x.
Les données sont envoyées en temps réel sur la console mobilab où elles peuvent être consultées et télécharger.

Le Végémètre-RTK a donc besoin d'un accès à internet par wifi pour fonctionner.

Le Végémètre-RTK calcule en permanence sa position avec une précision centimétrique grâce au réseau Centipède-RTK.

Le Végémètre-RTK mesure l'intensité lumineuse refleté par votre surface pour chaque bande spectrale. Cette intensité varie fortement avec la variation naturelle de l'ensoleillement. Cela ne posera pas de problème si vous souhaitez calculer des indices de végétations normalisés comme le NDVI. Cependant si vous souhaitez traiter les longueurs d'ondes de manière indépendantes vous devrez normaliser la valeur mesurée sur votre zone d'observation par une valeur mesurée sur une surface de référence (une moquette grise). Le Végémètre-RTK ne fait pas ce calcul, il se fait post-acquisition.

<h2>1. Partage de connexion</h2>
Le Végémètre se connecte par défaut à un accès wifi. Avec votre smartphone, générez un partage de connexion avec ces informations :

- nom : __bbsocoul__
- mot de passe : __bbsocoul__

*Si vous ne pouvez pas générer de partage de connexion, il est possible de connecter le Végémètre à autre Wifi mais pour cela il faut modifier le code embarqué dessus en suivant le tutoriel d'assemblage (lien à venir).*

<h2>2. Démarrer le végémètre</h2>
Brancher l'alimentation USB de la batterie externe située à l'extérieur du boitier. Ca y est le Végémètre RTK démarre !

<h2>3. Bouton de labellisation</h2>

Un bouton sur le côté du boitier permet de labelliser les données (pour ensuite pouvoir les trier).
- **Position 0 : "Garbage"**. Les données ne sont pas importantes (position à utiliser en bout de parcelle)
- **Position 1 : "Measurement"**. Les données sont importantes et correspondent à ce que vous mesurez (la végétation)
- **Position 2 : "Calibration"**. Les données sont importantes et correspondent à une mesure sur une surface de référence

Basculez le bouton dans la **position 0 "Garbage"** pour le démarrage

<img 
  src="https://github.com/user-attachments/assets/34d80394-0675-4680-b171-cd83fbb5d11b"
  alt="vegemetreRTK_EnvoieDonnees"
  style="width: 50%; height: auto;"
/>

<h2>4. Envoi et stockage des données</h2>

Le stockage des données se fait en ligne sur la console Mobilab. Les données sont donc envoyées en temps réel et cela peut rapidement collecter beaucoup de données inutiles. Pour cela nous vous recommandons d'utiliser le bouton **"envoi de données"** pour activer ou désactiver l'envoi (et donc le stockage) des données.

Basculez-le en mode **"actif"** :

<img 
  src="https://github.com/user-attachments/assets/cc6d0651-09b1-4fa9-9b33-86390c07a374"
  alt="vegemetreRTK_EnvoieDonnees"
  style="width: 50%; height: auto;"
/>

<h2>4. Consulter les données</h2>
Allez sur la [Console Mobilab](https://console-mobilab.ddns.net/)  (consultable sur PC ou smartphone)
Si vous faites parti du projet de recherche BBSoCouL _(qui a initié le développement du Végémètre-RTK)_ vous devriez avoir un compte pour visualiser les données. Vos identifiants sont :

login : _nom.prenom_

mot de passe : _nom.prenom_

<img 
  src="https://github.com/user-attachments/assets/5df43f8e-425c-4c56-874c-d5de54fd01dd"
  alt="vegemetreRTK_EnvoieDonnees"
  style="width: 40%; height: auto;"
/>

*Si vous n'avez pas de compte, contactez le Mobilab par mail [mobilab@agrotic.org](mailto:mobilab@agrotic.org)*

Vous arrivez par défaut sur le dashboard **Operationnel_bbsocoul_V2.1_XX** (XX correspondant au numero de votre Végéètre-RTK) qui permet de consulter les données "opérationnelles" du Végémètre RTK.

<h3>4.1 Informations générales</h2>
Une première fenêtre "Informations" vous donne des informations sur la hauteur d'acquisition et la vitesse d'avancement à respecter en fonction de ce que vous souhaitez mesurer. Plus vous êtes "lent" plus vous aurez une forte résolution spatiale. Plus vous êtes "bas" plus vous échantillonnez une surface restreinte.

<img 
  src="https://github.com/user-attachments/assets/5e314a51-7f88-4a4d-8247-ad63f660df10"  alt="vegemetreRTK_EnvoieDonnees"
  style="width: 50%; height: auto;"
/>

<h3>4.2 Qualité du GNSS</h2>

- La fenêtre **"Fixtype"** vous informe du niveau de précision du positionnement GNSS. Vous pouvez être en mode sans correction RTK ("no RTK"), en "RTK float" (solution RTK en cours de convergence, précision intermédiaire), ou en **"RTK fix"** (solution RTK stable avec une précision centimétrique).
- La fenêtre **"Précision (mm)"** vous informe de la précision théorique calculée par le capteur en mm. Si vous convergez vers des **valeurs inférieur à 30 (donc 3cm) c'est bon !** En fonction de vos besoins vous pouvez aussi décider de réaliser une acquisition avec moins de précision. Le Végémètre-RTK peut prendre plusieurs minutes pour avoir une précision centimétrique.

<img 
  src="https://github.com/user-attachments/assets/3cb4d3c8-e8b2-401e-abd7-a3e00f2c26d4"
  style="width: 25%; height: auto;"
/>

Votre Végémètre-RTK communique bien ses données et la précision est bonne. 

<h3>4.3 Qualité des données spectrales</h2>
Les deux fenêtre suivantes servent à s'assurer des conditions d'acquisition spectrales :

- La fenêtre **"Button state"** affiche en temps réel l'état de votre bouton de labellisation (rafraichissement toutes les 5 secondes). *Avant de faire une acquisition on vous recommande de tester les 3 positions et s'assurer que cela se met bien à jour dans l'interface de visualisation*
- La fenêtre **"NDVI"** affiche le NDVI. On peut ainsi rapidement vérifier que le capteurs spectrale ne s'est pas débranché ou endommagé. Les valeurs de NDVI doivent être entre 0 et 1 au dessus de la végétation. *Vous pouvez testez rapidement que le NDVI est plus élevé sur des zones denses en végétation (entre 0,6 et 0,8) et plus faible sur sol nu.*

<img 
 src="https://github.com/user-attachments/assets/d08c6bd5-c575-4cd3-bda6-514d506713ba"
  style="width: 25%; height: auto;"
/>

<h3>4.4 En cas de problème</h2>

*Si vous rencontrez les problèmes suivants :*

- _L'interface affiche  "No data" dans chaque fenêtre. Cela signifie que le végémètre n'envoie pas ses données. Voici les choses à vérifier_
     - _Vérifier que le bouton **"envoi des données"** est "actif"_
     - _Vérifier votre **partage de connexion** (son nom et mot de passe doivent être "bbosocoul"). Sur votre smartphone vérifiez le nombre d'appareils connectés. Eteignez et redémarrez le Végémètre-RTK (en debranchant le cable USB) pour regarder si le nombre d'appareils connectés change._
     - _Vérifier que vous avez une connexion internet avec votre smartphone : **données mobiles activées**, mode avion désactivé, votre smartphone capte la 3G ou 4G. Certains wifi peuvent bloquer le fonctionnement du Végémètre-RTK désactivez le wifi sur votre smartphone pour que toute la connexion passe par les données mobiles._
     - _Si toutes ces étapes n'ont pas fonctionné, contactez-nous_
- _La précision ne devien pas centimétrique :_
     - _Eteignez et redémarrez le Végémètre-RTK, placez le dans un endroit extérieur dégagé (pas de bâtiments ou d'arbres à proximité (~10 mètres)), attendez jusqu'à 5 minutes_
     - _Vérifier sur la [carte Centipède](https://docs.centipede.fr/) que vous avez des bases à moins de 50km de votre localisation_
     - _Contactez-nous si le problème persiste_
- _Pour tout autre problème, nous contacter_

<h2>5. Routine d'acquisition </h2>

Voici ce que nous recommandons pour réaliser une acquisition au champ. Votre Végémètre-RTK est éteint et vous suivez les étapes suivantes

- Mettez le bouton labellisation en **"Position 0 : Garbage"**. Le **bouton envoi de données "actif"**
- Activez votre partage de connexion sur votre smartphone
- Branchez le cordon USB
- Connectez vous à la console Mobilab avec votre smartphone
- Assurez vous de la bonne réception des données. Attendez le **Fixtype "Fix RTK".**
- Déterminez la hauteur d'acquisition et votre vitesse d'avancement en fonction de ce que vous souhaitez mesurer
- Placez vous à l'aplomb de la zone à mesurer et basculer le **bouton de labellisation en Position 1 : Measurement**
- Marchez dans votre zone à mesurer.
- Dès que vous souhaitez vous arrêter ou que vous sortez de la parcelle. basculez le **bouton de labellisation en Position 0 : Garbage**.
- **Passez sur une surface de référence** toutes les 30 minutes et au moins une fois dans l'acquisitio. Pour cela :
     - Finissez votre acquisition puis basculez le **bouton de labellisation en Position 0 : Garbage**.
     - Placez vous à l'aplomb de votre surface de référence. *Attention à adapter la hauteur d'acquisition pour que le diamètre de votre emprise au sol soit plus petit que votre surface de référence.*
     - Basculez le **bouton de labellisation en Position 2 : Calibration**.
     - Attendre au moins 10 secondes
     - Basculez le **bouton de labellisation en Position 0 : Garbage**.
     - Vous pouvez reprendre vos acquisition
- Si vous faites une pause prolongée ou un changement de parcelle, désactivez l'envoi des données

<h2>6. Télécharger les données </h2>
Les données sont sauvegardé dans la Console Mobilab qui n'est pas une solution de stockage pérenne. En cas de surcharge de l'espace disque sur le serveur on peut décider d'archiver les données. Pour cela, après chaque journée d'acquisition, téléchargez les données acquises.

Le téléchargement peut se faire depuis un PC ou smartpone mais ça sera beaucoup plus confortable depuis un PC.

- Cliquez sur le dossier de votre Végémètre-RTK

<img 
 src="https://github.com/user-attachments/assets/9943e40c-cd5c-4389-868a-8199b9c46ef6"
  style="width: 50%; height: auto;"
/>

- Cliquez sur le dashboard "Export_data_V2.1_XX"

<img 
  src="https://github.com/user-attachments/assets/ab8476f4-d650-4406-865c-d766ed628f4d"
  style="width: 50%; height: auto;"
/>

- Sélectionnez la date d'acquisition que vous souhaitez exporter. Si vous double-cliquez sur un jour cela sélectionne la journée entière.

<img 
  src="https://github.com/user-attachments/assets/69b478aa-8980-45f2-8583-6b0c478cdf88"
  style="width: 50%; height: auto;"
/>

- Dans la fenêtre "Export_data" allez dans le menu et cliquez sur **"Inspecter les données"**

<img 
  src="https://github.com/user-attachments/assets/bea67ee1-caa5-4f6e-b076-581e98735fc2"
  style="width: 50%; height: auto;"
/>

- Dans "afficher le dataframe", sélectionnez **"série jointe par une chronologie"**. Puis décochez la case **"Données formatées"**. Enfin cliquez sur **"Téléchargez en format csv"**

<img 
  src="https://github.com/user-attachments/assets/632439e5-d539-456d-ad63-11bfebcd239e"
  style="width: 50%; height: auto;"
/>



<h2>Produire une carte</h2>
A partir du .csv vous pouvez produire des cartes, pour l'instant nous vous recommandons de le faire sur QGIS. Si vous ne maitrisez pas le logiciel, envoyez-nous vos données pour qu'on puisse vous renvoyer la carte.

Voici quelques informations pratiques pour faire votre carte sur QGIS :
- Ouvrez le .csv avec un editeur de texte type bloc-note ou Notepad++
- Faites un Ctrl+H pour rechercher et remplacer :
     - Les **bbsocoul.** par rien (QGIS ne comprendra pas les entête de colonnes sinon)
     - Les **"** par rien (QGIS ne comprendra pas les entête de colonnes sinon)
- Importez le .csv dans QGIS avec le code EPSG:4171

<h2>Specification techniques</h2>

|paramètre|valeur|
|---------|------|
|Fréquence d'acquisition| 10 mesures par seconde|
|Longueurs d'ondes acquise|410, 435, 460, 485, 510, 535, 560, 585, 610, 645, 680, 705, 730, 760, 810, 860, 900, 940|
|Ouverture du cône d'acquisition|40°|
|Serveur NTRIP pour correction RTK|crtk.net|
|Point de montage Centipède-RTK|NEAR|
