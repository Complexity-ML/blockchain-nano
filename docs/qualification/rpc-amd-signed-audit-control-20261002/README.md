# Audit signé AMD64 du nouveau binaire RPC préparé

Contrôleur terminal uniquement, sans queue : exige les statuts zéro du contrôleur complet, des 427 cas et du supplément réseau 21 (y compris sa revue indépendante), avant création de l’état du contrôleur. Réexécute ensuite la revue complète, l’audit exact EEST, réseau21 et empreintes actuelles des programmes dans le volume conservé. Source propre d159, image fd9 et volume `nano-clean-rpc-build-20261002` ; sous-dossier `rpc-full-d1597d9a-amd64`.

Les quatre demandes signées existantes et la référence de fixture restent inchangées : positif 99 blocs/97 transactions avec engagements externes ; tête non finalisée, epoch futur et racine malformée refusés. Délais 180 secondes inchangés. Aucun build : utilisateur ubuntu, réseau désactivé, 1 CPU/2 Go, capacités retirées, sources/fixture/programmes montés en lecture seule. Les 21 journaux ne font pas partie de cette copie de chaîne ; elle ne remplace pas la sauvegarde complète pour la reprise de duties.

Code syntaxiquement vérifié, jamais lancé lors de cette préparation. Après acquisition des gates seulement, préfixer PATH par `/home/ubuntu/nano-rpc-full-amd64-control-20261002/bin` pour que la revue utilise le wrapper Docker sudo existant, puis exécuter `python3 -B run.py`. Les sorties doivent être neuves ; aucune relance automatique après un échec. `signed-review.py` est une revue indépendante des sorties, pas un lanceur.

Cette préparation ne vaut pas résultat AMD64, reprise des signataires, finalité WAN ou qualification production. Tous les autres gates et les échecs historiques restent conservés.

Inventaire VPS actuel vérifié en lecture seule : neuf fichiers de la copie de chaîne identiques à la référence figée (`36439318…`). Fixture présente hors Docker après reset, aucun audit lancé. Elle ne contient pas les journaux complets des signataires.
