# Capella : réutilisation du transport RPC privé — 27 septembre 2026

**Deux exécutions distribuées réussies après correction**, dont une confirmation sur le binaire reconstruit sans la sonde temporaire des votes. Deux nœuds locaux réels avec libp2p, chacun possédant 32 clés distinctes ; même checkpoint finalisé d’époque 2 observé trois fois dans le délai initial de 210 secondes. Aucun seuil ni règle de consensus modifié.

## Correction

`network/core.hpp` conservait une connexion TCP neuve par opération RPC. Le client réutilise désormais les connexions HTTP/1.1 terminées, avec au plus quatre connexions inactives conservées par défaut (paramètre du client, pas plafond de validateurs). Les appels concurrents ne partagent pas une connexion occupée. Une connexion inactive depuis dix secondes n’est pas réutilisée, le serveur ayant un délai de quinze secondes.

Les erreurs de transport ne provoquent **aucun rejeu automatique** : une requête peut avoir été exécutée avant la perte de sa réponse. Une fermeture explicite du serveur entraîne une nouvelle connexion pour le prochain appel. Les règles de session, les reçus d’admission et les contrôles de consensus restent en vigueur.

## Preuves et limites du diagnostic

Avant correction, le run instrumentant les erreurs RPC échoue à atteindre l’époque 2 (époque 1 commune seulement), avec 568 erreurs internes à l’ouverture des catalogues. Les précédents contre-exemples restent conservés dans les dossiers voisins. Le message générique ne permet pas, seul, de prouver l’exception interne exacte ou un épuisement de ports. Les mesures globales du Mac atteignaient 13 796 sockets TIME_WAIT avant correction et 356 lors d’une mesure après correction ; elles sont indicatives et ne sont pas attribuées exclusivement à Nano.

Après correction, aucune tentative d’ouverture de catalogue n’échoue dans les deux runs conservés. Des tentatives de livraison échouent encore ponctuellement puis sont reprises ; elles ne sont pas masquées par les compteurs de succès. La première exécution avec trace des votes ne présente plus d’admission distante retardée au-delà du slot d’émission dans les paires de traces rapprochées. Cette observation ne démontre pas une borne générale de latence.

La confirmation propre finalise sur les deux nœuds la racine `0x0606b174e635d18261022c678392e96c1a8321968f28ee9a00f828317cd21ad9` à l’époque 2. Les empreintes des binaires et sources d’exécution ont été capturées avant cette confirmation et revérifiées après. Les runs diagnostiques préliminaires n’ont pas de capture indépendante des empreintes de leurs binaires : la confirmation propre est la preuve de référence.

## Validation

- 17/17 tests réseau passent après modification du transport partagé.
- Six scénarios HTTP réels : 256 appels sur une seule connexion ; contrôle négatif sans cache (256 connexions) ; fermeture explicite ; réponse perdue ; erreur RPC sur connexion réutilisable ; fermeture silencieuse entre deux appels. Aucun rejeu automatique dans les cas ambigus. Le sixième scénario a été ajouté et passé séparément après le run des 17 tests.
- Deux runs du même test distribué passent, avec deux racines différentes entre les runs car les genesis sont distincts, mais accord entre nœuds dans chaque run.
- Le rattrapage réel d’un pair tardif passe aussi : même identité P2P, état neuf, accusé volontairement perdu, puis égalité de tête d’exécution et de racine d’état (`recovery.log`). Ce scénario court ne qualifie pas lui-même la finalité distribuée.

Le patch sans contexte (`git apply --unidiff-zero`) et les journaux sont conservés. `manifest.json` distingue le hash du fichier de test avant confirmation et celui de la revue finale après ajout du scénario de fermeture silencieuse ; les sources et binaires d’exécution n’ont pas changé entre les deux.

## Travail restant

Cette correction lève le contre-exemple local observé dans ces deux exécutions ; elle ne clôture pas la production. Restent notamment les partitions avec validateurs actifs, la convergence après reconnexion, les redémarrages prolongés, le WAN, la charge et les grands inventaires. Une ouverture de catalogue dont l’accusé est perdu conserve un résultat inconnu et peut dépendre du TTL : sa récupération reste une limite à traiter, indépendamment de la réduction des connexions.
