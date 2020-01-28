Client
=====

* `cameraAPI`: API pour la communication avec la caméra

* `main.c`: Link entre IHM et cameraAPI
	* Port par défaut 32424 (comunication caméra)
	* Port Broadcast IP: 5678 (30 premières secondes côté server)
	* Si Adrresse IP en input alors pas besoin de détecter, sinon nécessaire

#### Compilation du client

~~~shell
$ mkdir build
$ cd build
$ cmake ..
$ make
~~~