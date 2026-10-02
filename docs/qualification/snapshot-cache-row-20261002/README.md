# Comparaison du cache JSON sans copie — candidat non promu

Révision 2db1840c86d66129c55570e4abc60ff2335f6a3b, parent bad3e0c0, branche `qualification/checkpoint-serialization-20261002`. La ligne JSON entière, la justification et le statut d’ancrage sont comparés à l’entrée privée déjà validée ; la copie de la grande ligne dans un tuple de validation n’est créée que sur cache miss. Aucune comparaison réduite au root ou aux métadonnées. Validation d’exécution/checkpoints et formats conservés ; aucun header/layout modifié.

## AMD64 terminal

Trois cas exacts, deux passent, un échoue, aucun skip. Chaîne signée avec les contrôles cold/warm du cache, mutations de l’état/justification/gas/SSZ, changement de règles et annulation : passe 8,493 s. Nouveau scénario des retransmissions et stale-writer : passe 0,816 s. Partition : timeout 240,047 s, dernière observation slot 96, finalité commune 10, quatre avancements consécutifs ; assertions finales non atteintes. Aucun gain de slot terminal observé : le délai reste ouvert.

Sources/programmes/bibliothèques identiques avant/après ; post-manifeste 0, CTest/container 8. Build existant après fin de la campagne précédente, recompilation du fichier modifié et relink nano/test avec même source et headers. Pas de nouvelle image. Rapports et manifeste des raw reports dans `terminal-amd64/`. Ces trois cas ne constituent pas une régression globale.

## ARM64 en cours

Source propre figée dans `/tmp/nano-snapshot-cache-arm-source-20261002`, même révision. Image ARM64 existante et volume `nano-linux-sync-build` réutilisés ; nouveau répertoire de compilation cohérente `/build/snapshot-cache-row-2db1840c-arm64-20261002`, cœur et cibles nano/Bellatrix/Capella/stockage/témoin entièrement compilés après le changement de layout hérité. Treize cas pertinents, délais CMake d’origine ; résultat non acquis. Container `nano-snapshot-cache-row-arm64-test` réellement actif, rapports `/tmp/nano-snapshot-cache-arm-reports-20261002`. P2P existant séparément identifié, pas de nouvelle qualification globale réseau.

Prochaine analyse AMD64 : échantillonnage CPU du processus isolé pour identifier le coût dominant restant. `perf` disponible ; compteur matériel cycles non supporté sur ce VPS, task-clock disponible. Ne pas déduire un gain des seules modifications de code. Toutes les exigences de production et anciens échecs restent ouverts ; aucun runtime promu sur main.

## ARM64 terminal et correction du montage de fixtures

13 cas, 11 réussites, 2 échecs, aucun skip, entrées inchangées et post-manifeste 0. Partition passe 70,708 s dans son délai original 240 s ; les assertions finales sont atteintes. Les deux échecs `consensus_beacon_witness_recovery` / `consensus_bellatrix_witness_runtime` sont `filesystem error: cannot copy: Permission denied`, avant les opérations de rollback concernées. Ne pas les présenter comme deux succès initiaux.

Diagnostic minimal de `std::filesystem::copy` : même programme, mêmes modes/UID affichés, copie refusée sur bind mount macOS et réussie sur volume Linux. Premier essai de compilation de ce probe arrêté par TMPDIR sur root read-only ; sorties 127 conservées, puis compilation réussie avec TMPDIR dans le volume. Probe et résultats dans `copy-mount-probe/`.

Rejeu de ces deux cas seulement, fixtures temporaires sur le volume Linux existant. Aucun changement de runtime, binaire, capacités ou timeout. 2/2 passent (0,140 s / 1,709 s environ), source manifeste identique à la campagne ARM et mêmes hashes de tous les programmes communs et bibliothèques. Contrôles raw exacts conservés dans `witness-volume-retry-arm64/`. Cette qualification ne prouve pas une panne physique du disque ni tous les critères WAN/production. Le timeout AMD64 reste ouvert.
