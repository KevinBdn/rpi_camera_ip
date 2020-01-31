Server - Cross compilation
=====

Architecture de fichier
---

Voici l'architecture de fichier du côté server. Pour comprendre le fonctionnement du server vous pouvez lire les codes sources des fichiers décrits ci-dessous.

* `CMakeLists.txt`: Fichier de configuration pour la cross-compilation effectuée par `cmake` depuis docker, voir partie [Cross Compilation](#CC).

* `crossCompilation.sh`: Script shell automatisant la cross-compilation au travers d'un docker. Voir partie [Cross Compilation](#CC).

* `src/`: Dossier des codes sources `.c`
	
	* `gpioManager.c`: Petite biblitohèque que nous avons développé utilisant `sysfs` pour utiliser un port GPIO en sortie. 

	* `libCamera.c`: Librairie inspirée de `v4l2grab`, n'utilisant que les fonctions nécessaires, permettant de prendre une photo depuis la Pi Camera.

	* `main.c`: Programme principal créant le serveur TCP et gérant l'API de la caméra.

* `include/`: Dossier contenant les headers `.h`

	* `gpioManager.h`: Hearder associé à `gpioManger.c`.
	
	* `libCamera.h`: Hearder associé à `libCamera.c`.
	
* `bin/`: Dossier contenant le résultat de la cross-compilation

	* `server`: Exécutable cross-compilé pour le Raspberry Pi 3B.


<a name="CC">Cross compilation</a>
---

Vous pouvez compiler le `server` soit manuellement soit automatiquement. Pour la façon automatique vous devez quand même au préalable avoir au moins une fois réalisé les étapes **1.** et **2.** de la méthode manuelle.

### Manuellement

Voici les différentes commandes nécessaires pour cross-compiler le `server`.

**1. Cloner le répertoire `Github`**
	
	$ git clone https://github.com/KevinBdn/rpi_camera_ip.git
	
**2. Télécharger l'image docker**

Télécharger l'image Docker du système d'exploitation précompilé grâce à Buildroot:

	$ sudo docker pull pblottiere/embsys-rpi3-buildroot-video


**3. Lancer le docker**

Le chemin du répertoire amenant au dossier cloné devra remplacer le `$PATH_TO_REP` ci-dessous.
Lancer le docker en partageant le répertoire _rpi_ip_camera_ précédemment cloné, décompresser le répertoire _buildroot_:
	

	$ sudo docker run -it -v $PATH_TO_REP/rpi_camera_ip/:/root/rpi_camera_ip --privileged pblottiere/embsys-rpi3-buildroot-video /bin/bash
		
	docker$ cd /root/
	docker$ tar zxvf buildroot-precompiled-2017.08.tar.gz


Installer `cmake` dans le docker:
	
	docker$ apt-get update
	docker$ apt-get install cmake


**4. Cross-compiler**

Lancer le `cmake` dans le docker:
	
	docker$ cd rpi_camera_ip/Server/
	docker$ mkdir build
	docker$ cd build
	docker$ cmake ..
	docker$ make

Le fichier exécutable est présent dans le répertoire `rpi_camera_ip/Server/bin` et est nommé `server`. On peut attester du bon fonctionnement de la cross-compilation:
	
	docker# cd ../bin/
	docker# file server
	
	ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV), dynamically linked, interpreter /lib/ld-uClibc.so.0, not stripped

### Automatiquement

Cette méthode consiste à utiliser les commandes de la méthode manuelle mais directement depuis un script shell envoyé en entré du docker. Pour ceci veillez à avoir, au préalable, au moins une fois exécuté les étapes **1.** et **2.** de la méthode manuelle.

Placez-vous ensuite dans le répertoire `rpi_camera_ip` cloné puis exécuter la commande ci-dessous:

	$ cd ..
	$ PATH_TO_REP=`pwd`
	$ echo $PATH_TO_REP# doit afficher le chemin d'accès vers le répertoire rpi_camera_ip
	$ sudo docker run -v $PATH_TO_REP/rpi_camera_ip/:/root/rpi_camera_ip --privileged pblottiere/embsys-rpi3-buildroot-video rpi_camera_ip/Server/crossCompilation.sh

