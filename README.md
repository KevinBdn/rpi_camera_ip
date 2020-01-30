# Projet - PI Camera IP

Voici le répertoire Github du projet de système embarqué.

Groupe
--

* Kévin BEDIN
* Riwan CUINAT
* Caine SILVA

Objectifs
---

L'objectif de ce projet est de mettre en place une caméra IP à l'aide d'une RPI3 ainsi que de développer un client.

La fonctionnalité exigée étant de pouvoir prendre une image depuis la caméra IP à partir d'un Client installable.

Fonctionnalités implémentées
---
Nous avons fait le choix de développer l'ensemble du projet en C, client comme server.

Les fonctionnalités implémentées sont :

* **1.** Prendre une photo et la visualiser
> Les images sont envoyées en format JPEG

* **2.** Connaître l'état de disponibilité de la caméra IP via une LED
> La gestion du GPIO se fait en C au travers du `sysfs`, voir le [Server](Server/README.md).

* **3.** Caméra _plug and play_ : détection automatique de la caméra sur le réseau depuis le client.
> L'adresse IP est récupérée via broadcast de cette dernière par la RPI3.

* **4.** Visualisation du flux vidéo en directe depuis un client.
> Le flux vidéo est un envoi successif d'image JPEG. La latence dépend de la qualité du réseau.

Utilisation
----

Pour l'utilisation nous vous invitons à vous référer aux trois sous parties:

[1 - RPI: Comment installer la caméra IP](RPI/README.md)

[2 - Client: Compilation et utilisation du client](Client/README.md)

[3 - Server: Cross compilation et installation du server](Server/README.md)


