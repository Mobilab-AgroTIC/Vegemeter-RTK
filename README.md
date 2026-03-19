# Végémèter-RTK
Le Végémètre RTK est un capteur piéton qui permet de réaliser des mesures spectrales géoréférencées au champs. Le capteur réalise 10 mesures par seconde et enregistre simultanemenent les données GNSS de position, altitude, précision ainsi que les 18 bandes spectrales (visible et proche infrarouge) produite par le capteur AS7265x.
Les données sont envoyées en temps réel sur la console mobilab où elles peuvent être consultées et télécharger.

Le Végémètre-RTK a donc besoin d'un accès à internet par wifi pour fonctionner.

Le Végémètre-RTK calcule en permanence sa position avec une précision centimétrique grâce au réseau Centipède-RTK.

# Générer un accès Wifi
Le Végémètre se connecte par défaut à un accès wifi. Avec votre smartphone, générez générez un partage de connexion avec ces informations :
- nom : __bbsocoul__
- mot de passe : __bbsocoul__

*Si vous ne pouvez pas générer de partage de connexion, il est possible de connecter le Végémètre à autre Wifi mais pour cela il faut modifier le code embarqué dessus en suivant le tutoriel d'assemblage.*

# Démarrer votre végémètre
Brancher l'alimentation USB de la batterie externe située à l'extérieur du boitier. Ca y est le Végémètre RTK démarre (vous devriez voir des led s'allumer sur le capteur spectral)

# Activez l'envoi des données
Le stockage des données se fait en ligne sur la console Mobilab. Les données sont donc envoyées en temps réel et cela peut rapidement collecter beaucoup de données inutiles. Pour cela nous vous recommandons d'utiliser le bouton **"envoi de données"** pour activer ou désactiver l'envoi (et donc le stockage) des données.

Basculez-le en mode **"actif"** :

<img 
  src="https://github.com/user-attachments/assets/cc6d0651-09b1-4fa9-9b33-86390c07a374"
  alt="vegemetreRTK_EnvoieDonnees"
  style="width: 50%; height: auto;"
/>


# Connectez-vous pour consulter les données
Allez sur la [Console Mobilab](https://console-mobilab.ddns.net/) (consultable sur PC ou smartphone)
Si vous faites parti du projet de recherche BBSoCouL qui a initié le développement du Végémètre-RTK vous devriez avoir un compte pour visualiser les données. Vos identifiants sont :

login : nom.prenom

mot de passe : nom.prenom

<img 
  src="https://github.com/user-attachments/assets/5df43f8e-425c-4c56-874c-d5de54fd01dd"
  alt="vegemetreRTK_EnvoieDonnees"
  style="width: 25%; height: auto;"
/>

*Si vous n'avez pas de compte, contactez le Mobilab par mail [mobilab@agrotic.org](mailto:mobilab@agrotic.org)*

# Assurez-vous de la qualité des données collectées
Vous arrivez par défaut sur le dashboard _Operationnel_bbsocoul_V2.1_XX_ (XX correspondant au numero de votre Végéètre-RTK) qui permet de consulter les données "opérationnelles" du Végémètre RTK.

Une première fenêtre "Informations" vous donne des informations sur la hauteur d'acquisition et la vitesse d'avancement à respecter en fonction de ce que vous souhaitez mesurer. Plus vous êtes "lent" plus vous aurez une forte résolution spatiale. Plus vous êtes "bas" plus vous échantillonnez une surface restreinte.

<img 
  src="https://github.com/user-attachments/assets/5e314a51-7f88-4a4d-8247-ad63f660df10"  alt="vegemetreRTK_EnvoieDonnees"
  style="width: 50%; height: auto;"
/>


La fenêtre "Fixtype" vous informe du niveau de précision du positionnement GNSS. Vous pouvez être en mode sans correction RTK ("no RTK"), en "RTK float" (solution RTK en cours de convergence, précision intermédiaire), ou en "RTK fix" (solution RTK stable avec une précision centimétrique).

La fenêtre "Précision (mm)" vous informe de la précision théorique calculée par le capteur en mm. Si vous convergez vers des valeurs inférieur à 30 (donc 3cm) c'est bon ! En fonction de vos besoins vous pouvez aussi décider de réaliser une acquisition avec moins de précision.

<img 
  src="https://github.com/user-attachments/assets/3cb4d3c8-e8b2-401e-abd7-a3e00f2c26d4"
  style="width: 25%; height: auto;"
/>

Votre Végémètre-RTK communique bien ses données et la précision est bonne. 

Si cela ne fonctionne pas :

- L'interface affiche  "No data" dans chaque fenêtre. Cela signifie que le végémètre n'envoie pas ses données. Voici les choses à vérifier
     - Vérifier l'état du bouton **"envoi des données"**. Il doit êtr actif
     - Vérifier votre partage de connexion (son nom et mot de passe doivent être _"bbosocoul"_). Normalement sur votre smartphone vous pouvez voir le nombre d'appareils connectés. Eteignez et redémarrez le Végémètre-RTK (en debranchant le cable USB) pour regarder si le nombre d'appareils connectés change.
     - Vérifier que vous avez une connexion internet avec votre smartphone : données mobiles activées, mode avion désactivé, votre smartphone capte la 3G ou 4G. Certains wifi peuvent bloquer le fonctionnement du Végémètre-RTK désactivez le wifi sur votre smartphone pour que toute la connexion passe par les données mobiles
     - Si toutes ces étapes n'ont pas permis d'obtenir des données. Contactez-nous
- Vous n'arrivez pas à obtenir une précision centimétrique :
     - Eteignez et redémarrez le Végémètre-RTK, placez le dans un endroit extérieur dégagé (pas de bâtiments ou d'arbres à 1proximité (~10 mètres)), attendez jusqu'à 5 minutes
     - Vérifier sur la [carte Centipède](https://docs.centipede.fr/) que vous avez des bases à moins de 50km de votre localisation
     - Contactez-nous si le problème persiste

# Collectez vos données et utilisez le bouton de labellisation

# Téléchargez les données

# Produire une carte
Actuellement vous pourrez produire une carte facilement pour des indices de végétation simple


# Specification techniques

|paramètre|valeur|
|---------|------|
|fréquence d'acquisition| 9 à 10 par seconde|
|longueurs d'ondes acquise|410, 435, 460, 485, 510, 535, 560, 585, 610, 645, 680, 705, 730, 760, 810, 860, 900, 940|
|Ouverture du cône d'acquisition|40°|
|Point de montage centipède|NEAR|
