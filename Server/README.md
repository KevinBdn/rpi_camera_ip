Server
=====

* `gpioManager`: Petite biblitohèque que nous avons développé utilisant `sysfs` pour utiliser un port GPIO en sortie. 

* `libCamera`: Librairie inspirée de `v4l2`, n'utilisant que les fonctions nécessaires, permettant de prendre une photo depuis la Pi Camera. Les photos sont convertie en jpeg dans un buffer (sans utilisation de fichier), ce qui permet de les envoyer via des sockets.

* `main.c`: Programme principal créant le serveur TCP et gérant l'API de la caméra.

On lance le server avec `sudo`pour la gestion du GPIO.

#### Description

La caméra doit être branché au réseau via son port ethernet, avec une IPV4. 
Le module kernel `bcm2835-v4l2` doit être chargé:

	$ sudo modprobe bcm2835-v4l2

L'IP de la caméra est broadcastée durant les 30 premières secondes en UDP sur le port `5678`. La communication avec le client se fait via le server TCP généré. Une LED branchée sur le GPIO n°18 (numéro de pin 12) indique l'état de la caméra. 

* Server TCP:  Port `32424` par défaut (si pas de port donnée en entré du programme `server`). 

* Indication LED: 

	* S'allume quand le server est bien lancé. 
	* Clignote à 5Hz si problème de connection avec la caméra
	* Clignote à 1Hz quand elle broadcast son IP (30 seconde au démarrage)
	* S'éteint et s'allume quand elle capture une image.


