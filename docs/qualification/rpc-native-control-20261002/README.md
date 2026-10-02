# Candidate RPC d159 : binaire natif Mac cohérent

Le test WAN demandé requiert un vrai propriétaire macOS natif ; les images Docker produisent des binaires Linux. Ce contrôleur compile la même candidate propre `d1597d9a` dans un dossier neuf `/tmp/nano-rpc-native-build-20261002`, avec les dépendances cpp-libp2p déjà installées. Aucun build d’image supplémentaire, aucun conteneur ou volume nouveau. Compilation limitée à deux jobs, priorité système réduite au lancement.

Il compile d’abord les helpers réseau puis le cœur et les programmes nécessaires aux 13 cas natifs de la qualification précédente, ainsi que `rpc` qui couvre la nouvelle estimation. Les 14 noms et délais originaux sont explicitement vérifiés. Les 21 cas réseau autonomes utilisent les helpers de ce nouveau sous-dossier ; aucune utilisation de l’ancien `build-network` du dépôt. JUnit et revue existante exigent tous les cas exécutés sans échec/skip.

Les 15 315 sources doivent correspondre à la référence avant compilation, après compilation et après tests. Le contrôleur empreinte les programmes, objets, archives et link.txt locaux, les archives statiques externes effectivement référencées dans ces commandes de liaison et les dépendances dynamiques trouvables sur disque. Ils doivent rester inchangés pendant les tests. Cela n’audite pas intégralement les bibliothèques système dyld et ne prouve pas un build hermétique/reproductible.

Rapports : `/tmp/nano-rpc-native-reports-20261002`. Statut de chaque phase et statut terminal réels conservés. Aucun lancement WAN, aucune réutilisation de fixture restaurée dans ce contrôleur. La régression Docker complète et les autres gates restent requis ; aucun succès présumé.
