Client
=====

Choix
-----


Architecture de fichier
---

* `CMakeLists.txt`: Fichier de configuration pour la compilation effectuée par `cmake`, voir partie [Compilation](#C).

* `ressources/`: Ressources pour le client

	* `IHM.glade`: IHM sous glade, importée via un builder en C.
	
	* `stryle.css`: Feuille CSS permettant de personaliser l'affichage de l'IHM.

* `src/`: Dossier des codes sources `.c`
	
	* `cameraAPI.c`: Fonctions permettant de communiquer via socket avec le `server`.

	* `main.c`: Programme principal créant l'IHM en la reliant à l'API de la caméra que nous avons développé.

* `include/`: Dossier contenant les headers `.h`

	* `cameraAPI.h`: Hearder associé à `cameraAPI.c`.
	
	* `libCamera.h`: Hearder associé à `libCamera.c`.
	


Dépendances
---

* GTK 3+ et GDK Pixbuffer:

		sudo apt-get install libgtk-3-dev

* CMake:
	
		sudo apt-get install cmake

<a name=C>Compilation</a>
---

Pour la compilation du client nous utilisons `cmake`. Il faut se placer dans le dossier git clone `rpi_camera_ip` pui effectuer les commandes suivantes:

	$ cd Client/
	$ mkdir build
	$ cd build/
	$ cmake ..
	$ make
	$ cd ../bin/

Le binaire sera alors compilé dans le dossier `bin` sous le nom `client`.
