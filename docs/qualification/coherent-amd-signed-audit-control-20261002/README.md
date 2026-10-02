# Audit signé AMD64 préparé après la campagne cohérente

Le contrôleur attend les codes terminaux de la campagne complète et du supplément réseau. Il vérifie que les propriétaires en attente sont réellement présents via PID et commande dans `/proc`, sans redémarrage automatique. Une preuve absente après arrêt du propriétaire ou un code non nul refuse le lancement.

Puis `run.py` réexécute la revue terminale des 427 cas, les 5 387 variantes EEST, les 21 cas réseau et les SHA actuels des exécutables. Il refuse ensuite toute variation des sources, du binaire Nano ou de la fixture RocksDB signée (99 blocs / 97 transactions). Les quatre requêtes existantes sont inchangées : checkpoint finalisé accepté, tête non finalisée/époque future/racine malformée rejetées avec le motif attendu. Chaque cas conserve sa limite de 180 s.

Aucune compilation. Image AMD64 et volume de build existants, sources/fixture/volume en lecture seule, utilisateur ubuntu, réseau désactivé, 1 CPU/2 GiB, capacités retirées. Les rapports et états sont en dehors des checkouts Git. Ces quatre cas sont un rejeu de qualification du nouveau binaire, sans prétendre ajouter de nouveaux scénarios historiques. Ils ne prouvent pas la reprise des duties ni la finalité WAN. Tous les critères de production restent ouverts tant que les résultats ne sont pas acquis et examinés.

Les scripts et requêtes sont transmis au VPS par Git. Le lanceur devra préfixer PATH avec `/home/ubuntu/nano-coherent-network-build-control-20261002` pour réutiliser son wrapper Docker sudo. Aucun test signé n’a été exécuté lors de la préparation.
