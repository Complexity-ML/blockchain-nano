# Présélection RPC de header — couverture et clôture, 18 octobre 2026

Lecture publique seule ; aucun programme/test/SDK/Git exécuté, aucun R modifié. R = `/Users/boris/Dev/blockchain-nano`. Ce document fixe des attentes, pas une qualification nouvelle.

## Ce que C6 prouve et ne prouve pas

`R/tests/test_rpc_state_read_cost.cpp:15–24` utilise un genesis de 128 comptes ×16 slots, sans transaction. Quatre annulations au poll64 puis une pause au même point prouvent travail de décodage/MPT annulable et progression de eth_chainId pendant ce décodage détaché ; elles ne visent pas la présélection du bloc. Le State et le pool restent inchangés.

`R/tests/test_rpc_state_block_id.cpp` couvre selectors Paris/Shanghai (59–76), erreurs exactes (78–89), vraies branches A/B distinguables (91–108), pruning réel et réouvertures RO/RW (110–140), HTTP/batch/notifications (143–163). Mais ses blocs ne contiennent qu’une/deux transactions. Les polls3/6/7 représentent respectivement avant capture de l’état / après pin ou copie ; des checkpoints nouveaux peuvent déplacer ces instants sans changer la propriété. Préserver ces propriétés, pas changer une attente pour accepter un résultat candidat.

La lacune est source : `R/src/rpc.cpp:638` appelle block_by_hash ou Rpc::block ; `Rpc::block:275–281` copie notamment chain.tip pour latest. `R/src/storage.cpp:508–509` lit et décode Block complet sous le mutex Store. Le mutex Rpc couvre encore prepare_state_read. Le volume des transactions peut donc dominer avant le travail borné de l’état. Remplacer Block::from_json par un DOM JSON complet puis Header::from_json ne supprime pas ce coût ; latest/finalized ne doivent pas conserver une copie de Block.

C6 conserve O(N), budgets logiques, annulation coopérative et I/O non préemptible : `R/docs/EIP-IMPLEMENTATION-STATUS.md:8325`, `R/docs/PRODUCTION-GOAL.md:5525`. Le vrai main HTTP PoW/Shanghai est qualifié dans son scénario (status:8331), sans authentification des racines par finalité Beacon (8333). Le lot sync local n’élargit pas cette autorité.

## Critères de fermeture de l’API

1. Le chemin de sélection renvoie seulement un Header possédé/pinné, sans copier, décoder ou conserver le body/transactions. Coût explicite borné par la taille du header et les lectures sélectionnées ; aucun DOM complet dissimulé. S’il reste un chemin legacy, son coût et ses refus sont explicites, et un nouveau format est écrit atomiquement sur tous les chemins commit/rewind/checkpoint/reorg concernés. Pas de migration silencieuse lors d’un open RO.
2. Pour hash : recomposition du hash du header et égalité avec la clé demandée ; pour number : hauteur correcte et correspondance avec l’index canonique, sans faire confiance au champ JSON hash. Chaîne/header/state ne se mélangent pas. Les erreurs Unknown block/State unavailable/noncanonical/encoding invalid conservent leur classe et ne retombent jamais sur latest.
3. L’instant de capture reste défini : avant pin de l’état, un reorg peut refuser ; après pin, la réponse peut finir sur l’état/header sélectionnés même après reorg/pruning. requireCanonical=true ne devient pas une promesse de canonicalité au moment de l’envoi HTTP. requireCanonical=false n’invente aucune archive d’état orphelin. `serialized_state_at:512–526` conserve la vérification hash→hauteur et le pin dans une même section sérialisée avec les writers.
4. Le parsing/copie coûteux et les callbacks de travail/cancellation ne bloquent pas les mutex Rpc/Store. Le DB Get et l’attente d’un verrou système ne deviennent pas interruptibles par simple ajout d’un callback. Le progrès concurrent doit être démontré sur la présélection elle-même, distinct du décodage State.
5. Les quatre anciens readers ET eth_getProof utilisent le header sélectionné et la vérification existante StateRoot. Aucun changement de fork, authentification consensus, exposition de clés ou qualification globale n’est déduit.

## Nouveaux tests nécessaires, puis seules régressions touchées

- Bloc valide avec body/transactions volumineux mais état petit : number, hash requireCanonical true/false, earliest, latest et finalized quand réellement disponible. Oracle déterministe de travail/allocation/accès excluant le parcours/copie du body ; un mutant réintroduisant l’ancien chemin Block complet doit échouer. Le seul temps mur ou la seule pause poll64 ne suffit pas.
- Pause/annulation pendant présélection, avant serialized_state_at : réponse RPC légère ET écriture/restauration peuvent avancer ; annulation et deadline donnent le refus attendu sans mutation. Nouveau test de durée bornée, pas flood.
- Ancien stockage sans index éventuel, stockage neuf, réouverture RO/RW ; corruption contrôlée du numéro/hash/header, index manquant/désaccord, trop gros header et limite exacte. Refus fermés, sans fallback sur une autre branche ; vérifier les chemins d’écriture/réorganisation réellement touchés.
- Réutiliser les groupes selectors/errors/reorg/retention des state readers et les preuves EIP-1186 concernées : reorg avant/après capture, suppression par pruning après pin, header/state incohérents. Un proof calculé localement ne devient toujours pas un checkpoint finalisé authentifié.

Les suites antérieures non concernées restent acquises. Publier révision, cible, arguments, oracle, mutant, compteurs/limites et clôtures ; ne pas annoncer tout le RPC, un coût indépendant de RocksDB/I/O, WAN ou production prêt.
