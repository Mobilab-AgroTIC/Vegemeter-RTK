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
Le stockage des données se fait en ligne sur la console Mobilab. Les données sont donc envoyées en temps réel et cela peut rapidement collecter beaucoup de données inutiles. Pour cela nous vous recommandons d'utiliser le bouton *"envoi de données"* pour activer ou désactiver l'envoi (et donc le stockage) des données.

Laissez le en mode **"envoi"**.

# Connectez-vous pour consulter les données
Allez sur la [Console Mobilab](https://console-mobilab.ddns.net/) (consultable sur PC ou smartphone)
Si vous faites parti du projet de recherche BBSoCouL qui a initié le développement du Végémètre-RTK vous devriez avoir un compte pour visualiser les données. Vos identifiants sont :
login : nom.prenom
mot de passe : nom.prenom

*Si vous n'avez pas de compte, contactez le Mobilab par mail [mobilab@agrotic.org](mailto:mobilab@agrotic.org)*

# Assurez-vous de la qualité des données collectées
Vous arrivez par défaut sur le dashboard qui permet de consulter les données "opérationnelles" du Végémètre RTK.

Une première fenêtre "Informations" vous donne des informations sur la hauteur d'acquisition et la vitesse d'avancement à respecter en fonction de ce que vous souhaitez mesurer. Plus vous êtes "lent" plus vous aurez une forte résolution spatiale. Plus vous êtes "bas" plus vous échantillonnez une surface restreinte.

La fenêtre "Fixtype" vous informe du niveau de précision du positionnement GNSS. Vous pouvez être en mode sans correction RTK ("no RTK"), en "RTK float" (solution RTK en cours de convergence, précision intermédiaire), ou en "RTK fix" (solution RTK stable avec une précision centimétrique).

La fenêtre "Précision (mm)" vous informe de la précision théorique calculée par le capteur en mm. Si vous convergez vers des valeurs inférieur à 30 (donc 3cm) c'est bon ! En fonction de vos besoins vous pouvez aussi décider de réaliser une acquisition avec moins de précision.

# Collectez vos données et utilisez le bouton de labellisation

# Téléchargez les données

# Produire une carte
Actuellement vous pourrez produire une carte facilement pour des indices de végétation simple
