# Recompilation indépendante des programmes de livraison — en attente

Candidate figée : `efbeb9bca3d42e41edb9cb81afba98c0b953a90f`.
Ce contrôleur attend la réussite terminale de la restauration, elle-même bloquée
sur les deux campagnes complètes Docker. Il ne constitue pas une réussite de
reproductibilité ni une qualification de production.

Il réutilise les images par digest et les volumes actuels, puis compile seulement
`nano` et `nano-p2p` dans un répertoire neuf par architecture. Il refuse un
conteneur déjà actif, une révision sale, un manifeste de sources divergent ou
une empreinte de programme original modifiée. Les sources suivies et les deux
programmes originaux sont relus avant et après la compilation ; les nouveaux
SHA-256 doivent égaler ceux des programmes qualifiés. Une divergence est un
échec conservé, jamais un changement du résultat attendu.

Les logs de compilation sont écrits pendant l'exécution, y compris en cas
d'échec ou délai dépassé. Un timeout arrête uniquement le conteneur de
recompilation nommé par ce contrôleur. Aucune reconstruction d'image, aucune
suppression de volume, aucune nouvelle suite CTest.

Le premier contrôleur a été arrêté pendant son attente, avant toute compilation,
pour corriger cette conservation des erreurs et ajouter le contrôle des fichiers
réels. Le remplacement est `/tmp/nano-reproducible-delivery-control-v2-20261004`.
Les contrôles ciblés démontrent : relecture de 16 494 sources conforme au
manifeste, refus d'un exit 7 avec log, conservation du log après timeout.
Ils ne démontrent pas encore la reproductibilité des programmes.

Limite : dépendances de l'image et cache épinglés déjà présents ; ce n'est pas
une reconstruction hermétique de toutes les dépendances depuis leurs sources,
ni une comparaison ARM64/AMD64 entre elles.
