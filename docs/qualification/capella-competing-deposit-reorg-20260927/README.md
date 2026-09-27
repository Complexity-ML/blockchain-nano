# Réorganisation du reactor avec dépôts concurrents

## Scénario intégré

Trois propriétaires Blockchain/RocksDB indépendants partent du même ancrage.
Deux producteurs isolés exécutent les dépôts A/B et B/A, puis produisent leurs
blocs signés jusqu'au slot 49. Le troisième reactor admet les deux branches
par son chemin normal `block`, sans snapshot de branche injecté ni mutation
directe de son registre. Les dépôts passent par le contrat EVM, l'historique
natif, le vote de dépôt et les opérations obligatoires du bloc.

Les deux nouveaux validateurs portent le registre à 66 entrées. La clé A est
résolue à l'index 64 sur AB et 65 sur BA. Des votes signés par des membres de
comité distincts sélectionnent successivement AB puis BA dans l'observateur.
Après chaque sélection, le test vérifie les index, l'historique de dépôt et
la racine de l'état EVM, puis rouvre le propriétaire et son reactor à partir
de l'engagement conservé. Les deux restaurations retrouvent le bon registre.

Après passage de A à l'index 65, une demande d'attestation avec son ancien
index 64 lève `StaleCapellaDuty` avant le signataire ; le checkpoint et le
journal ne changent pas. La demande utilisant 65 est reconnue, mais ne signe
pas puisque le validateur n'est pas encore actif. Zéro appel au signataire
pour ces deux contrôles.

## Résultat et échecs conservés

`consensus_capella_competing_deposit_reorg` passe en **22,09 s** (22,10 s CTest).
Le run précédent sans contrôle final de signature passait en 21,97 s.

Le premier build du nouveau test échouait sur une conversion uint64_t vers
GMP ambiguë ; conversion décimale explicite corrigée. Le premier run échouait
au-delà de 64 blocs : le test réutilisait le budget du banc de crash, insuffisant
pour ses deux branches. Le budget de ce seul scénario vaut maintenant
`2 * (period + period/2 + 1)`, soit 98 blocs pour le preset minimal. L'observateur
en conserve 97 à la fin. Les budgets octets et toutes les règles de consensus
restent inchangés. Logs initiaux, rejeux, JUnit et empreintes finales conservés ;
les espaces finaux des logs ont été normalisés.

## Limites

Les producteurs adversariaux utilisent volontairement des journaux séparés
pour signer les branches concurrentes avec les clés de fixture. Aucune clé
réelle, aucun fonds et aucun pair public. Les appels sont orchestrés dans un
processus avec trois bases, sans HTTP/P2P/WAN ni injection de threads concurrents.
Deux branches sont sélectionnées ; cela ne prétend pas prouver deux bascules
supplémentaires par rapport à la tête initialement choisie par le fork choice.

La nouvelle clé reste inactive. La finalité après reorg, l'activation puis ses
devoirs avec le nouvel index, les partitions distribuées et les crashes pendant
la bascule restent à qualifier. Cette preuve complète les contrôles primitifs
AB/BA, mais ne clôture ni EIP-6110, ni le cycle validateur, ni la production.
Aucun code de production ni déploiement VPS modifié par ce commit.
