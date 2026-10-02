# Comparaison du cache JSON sans copie — candidat non promu

Révision 2db1840c86d66129c55570e4abc60ff2335f6a3b, parent bad3e0c0, branche `qualification/checkpoint-serialization-20261002`. La ligne JSON entière, la justification et le statut d’ancrage sont comparés à l’entrée privée déjà validée ; la copie de la grande ligne dans un tuple de validation n’est créée que sur cache miss. Aucune comparaison réduite au root ou aux métadonnées. Validation d’exécution/checkpoints et formats conservés ; aucun header/layout modifié.

## AMD64 terminal

Trois cas exacts, deux passent, un échoue, aucun skip. Chaîne signée avec les contrôles cold/warm du cache, mutations de l’état/justification/gas/SSZ, changement de règles et annulation : passe 8,493 s. Nouveau scénario des retransmissions et stale-writer : passe 0,816 s. Partition : timeout 240,047 s, dernière observation slot 96, finalité commune 10, quatre avancements consécutifs ; assertions finales non atteintes. Aucun gain de slot terminal observé : le délai reste ouvert.

Sources/programmes/bibliothèques identiques avant/après ; post-manifeste 0, CTest/container 8. Build existant après fin de la campagne précédente, recompilation du fichier modifié et relink nano/test avec même source et headers. Pas de nouvelle image. Rapports et manifeste des raw reports dans `terminal-amd64/`. Ces trois cas ne constituent pas une régression globale.

## ARM64 en cours

Source propre figée dans `/tmp/nano-snapshot-cache-arm-source-20261002`, même révision. Image ARM64 existante et volume `nano-linux-sync-build` réutilisés ; nouveau répertoire de compilation cohérente `/build/snapshot-cache-row-2db1840c-arm64-20261002`, cœur et cibles nano/Bellatrix/Capella/stockage/témoin entièrement compilés après le changement de layout hérité. Treize cas pertinents, délais CMake d’origine ; résultat non acquis. Container `nano-snapshot-cache-row-arm64-test` réellement actif, rapports `/tmp/nano-snapshot-cache-arm-reports-20261002`. P2P existant séparément identifié, pas de nouvelle qualification globale réseau.

Prochaine analyse AMD64 : échantillonnage CPU du processus isolé pour identifier le coût dominant restant. `perf` disponible ; compteur matériel cycles non supporté sur ce VPS, task-clock disponible. Ne pas déduire un gain des seules modifications de code. Toutes les exigences de production et anciens échecs restent ouverts ; aucun runtime promu sur main.
