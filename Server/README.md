Server
=====


#### Architecture de fichier

* `CMakeLists.txt`: Fichier de configuration pour la cross-compilation effectuée par `cmake` depuis docker, voir partie [Cross Compilation](#CC).

* `src/`: Dossier des codes sources `.c`
	
	* `gpioManager.c`: Petite biblitohèque que nous avons développé utilisant `sysfs` pour utiliser un port GPIO en sortie. 

	* `libCamera.c`: Librairie inspirée de `v4l2grab`, n'utilisant que les fonctions nécessaires, permettant de prendre une photo depuis la Pi Camera.

	* `main.c`: Programme principal créant le serveur TCP et gérant l'API de la caméra.

* `include/`: Dossier contenant les headers `.h`

	* `gpioManager.h`: Hearder associé à `gpioManger.c`.
	
	* `libCamera.h`: Hearder associé à `libCamera.c`.
	
* `bin/`: Dossier contenant le résultat de la cross-compilation

	* `server`: Exécutable cross-compilé pour le Raspberry Pi 3B.
	
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


#### <a name="CC">Cross compilation</a>

##### Cloner le répertoire `Github`
	
	$ git clone https://github.com/KevinBdn/rpi_camera_ip.git
	
##### Télécharger l'image docker

Télécharger l'image Docker du système d'exploitation précompilé grâce à Buildroot:

	$ sudo docker pull pblottiere/embsys-rpi3-buildroot-video


##### Lancer le docker

Le chemin du répertoire amenant au dossier cloné devra remplacer le `PATH_TO_REP` ci-dessous.
Lancer le docker en partageant le répertoire _rpi_ip_camera_ précédemment cloné, décompresser le répertoire _buildroot_:
	

	$ sudo docker run -it -v PATH_TO_REP/rpi_camera_ip/:/root/rpi_camera_ip --privileged pblottiere/embsys-rpi3-buildroot-video /bin/bash
		
	docker# cd /root/
	docker# tar zxvf buildroot-precompiled-2017.08.tar.gz


Installer `cmake` dans le docker:
	
	docker# apt-get update
	docker# apt-get install


##### Cross-compiler

Lancer le `cmake` dans le docker:
	
	docker# cd rpi_camera_ip/Server/
	docker# mkdir build
	docker# cd build
	docker# cmake ..
	docker# make

Le fichier exécutable est présent dans le répertoire `rpi_camera_ip/Server/bin` et est nommé `server`. On peut attester du bon fonctionnement de la cross-compilation:
	
	docker# cd ../bin/
	docker# file server
	
	ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV), dynamically linked, interpreter /lib/ld-uClibc.so.0, not stripped
	