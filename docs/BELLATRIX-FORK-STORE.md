# Choix de branche Bellatrix et état EVM associé

`BellatrixForkStore` conserve, pour chaque racine Beacon, le bloc signé validé
sous forme d'en-tête, son état Beacon, son bloc d'exécution, son état EVM et ses
reçus. Les enveloppes signées originales ne sont pas archivées par cette API.
La réception sélectionne les deux parents dans le même enregistrement et
applique la transition Bellatrix complète avant de produire un candidat privé.

Les votes BLS, le tick, le proposer boost, les checkpoints réalisés/non réalisés
et les preuves d'attester slashing réutilisent les primitives de choix de branche
qualifiées pour Altair. Les contextes de comité et de proposant sont dérivés
des états Bellatrix authentifiés, avec leurs transitions de slots/epochs.
`bellatrix_fork_head` retourne un enregistrement Beacon/EVM lié et vérifié ;
il ne publie pas une tête canonique dans RocksDB.

L'initialisation exige une ancre de confiance externe : état et corps Beacon,
bloc et état EVM, reçus et jusqu'à 256 en-têtes d'exécution antérieurs. Elle
vérifie les racines, le payload, les reçus et la chaîne de hashes des ancêtres.
Ce contrôle de cohérence ne remplace pas l'authentification de l'ancre.
`BLOCKHASH` suit les parents de la branche exécutée, puis les ancêtres de
l'ancre ; une histoire manquante interrompt la préparation.

## Qualification locale

Le test `consensus_bellatrix_signed_execution_chain` utilise 64 clés de fixture
et neuf blocs synthétiques signés. Il initialise le Store au premier bloc,
reçoit les suivants, exécute une lecture `BLOCKHASH` et un incrément de stockage
par transaction. Un second bloc au slot 9 contient une autre transaction :
le Store conserve dix blocs et deux états EVM distincts.

Six votes individuels BLS, issus des comités du slot, font passer la tête vers
la branche concurrente puis revenir sur la branche initiale. À chaque étape,
la racine EVM retournée est comparée à celle de l'exécution attendue. Le test
couvre aussi les blocs futurs, parents inconnus, signatures invalides, doublons,
ancres incomplètes, états EVM incohérents et trois interruptions de préparation
sans mutation du Store d'entrée. Les sept refus et sept interruptions de la
transition signée précédente restent exercés.

Preuves locales : `build-cmake/qualification/bellatrix-fork-store-20260925/`.
Le premier essai supposait six membres dans un comité minimal ; le scénario
final répartit correctement ses votes entre les comités réels du slot.

## Limites

Ce Store est un candidat complet copié en mémoire, sans politique de pruning,
archivage des enveloppes ni qualification de capacité. Ce n'est pas encore le
reactor réseau Bellatrix. Le scénario n'établit pas la finalité sur plusieurs
epochs, ne lance pas 64 nœuds et ne compare pas ses post-états à un autre client.
La lecture `BLOCKHASH` est vérifiée sur la chaîne construite ; des descendants
des deux branches divergentes restent à ajouter à cette qualification.

Le raccordement suivant est une publication durable indivisible du choix de
branche, de l'exécution canonique et des reçus, avec reprise après interruption,
puis le raccordement réseau et les scénarios de partitions/pannes. Aucun fork
réseau, déploiement VPS ou fonds réel n'est activé par ce changement.
