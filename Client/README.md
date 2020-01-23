Client
=====

* `cameraAPI`: API pour la communication avec la caméra

* `main.c`: Link entre IHM et cameraAPI
	* Port par défaut 32424 (comunication caméra)
	* Port Broadcast IP: 5678 (30 premières secondes côté server)
	* Si Adrresse IP en input alors pas besoin de détecter, sinon nécessaire
	
* TODO:
	
	* Voir la fonction `cameraAPI_getIP()` fonctionne mais gestion de Thread avec GTK doit être un g_pthread (à faire). 
	* Désactiver les boutons tant que pas de caméra détectée
	* Image par défaut qui explique les manip de l'IHM
	* Boutons pour bouger image
