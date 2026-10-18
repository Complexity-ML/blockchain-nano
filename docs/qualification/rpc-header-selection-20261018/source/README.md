# RPC header selection : proposition de sources, non qualifiée

Base exacte : `801d793f0f05c3b46f97d34d86973f1487e36561`. Ce lot contient dix sources : six fichiers de production, trois TU existants étendus et une seule définition CMake `NANO_TEST_SOURCE` pour le fixture du test de coût. Les sources et oracles ont reçu une revue interne ; le manifeste reste **source-only**, pas une qualification ni une autorisation opératoire générale. Aucun compilateur, test, programme du projet, import de module projet, Docker, SSH, provider SDK ou contrôle privé n'a été consommé par les auteurs. Aucun fichier R n'a été écrit. Aucune réussite, ABI qualifiée ou readiness production n'est revendiquée.

## Correction de la cause

Avant ce lot, les lecteurs d'état appelaient `block_by_number/hash`, `tip()` ou `finalized_block()` sous le verrou RPC avant de rendre le worker. Le stockage lisait et parsait le JSON complet sous son mutex ; `Block::from_json` reconstruisait les transactions et leurs émetteurs. `tip_header()` seul ne fermait pas le cas Beacon, car sa vérification de disponibilité appelait encore `store.latest()`.

La préparation RPC ne garde désormais que les paramètres validés, le selector et la deadline. La closure après `Rpc::one` unlock sélectionne le contexte Header, la donnée historique, puis l'état pinné et son trie. Le nonce pending conserve sa voie mempool précédente. Le budget `now/timeout`, l'annulation et `maximum_snapshot_bytes` existants sont partagés ; aucun champ/layout de `RpcStateReadLimits` ou `RpcSimulationLimits` n'est ajouté.

## Stockage et compatibilité

`execution-header/v1/` + `hex(bytes(height,8))` stocke exactement `Header::to_json()`, hash inclus. Les deux Put et deux Delete sont ajoutés aux **mêmes batches synchrones** que block/state/hash : commit ordinaire, checkpoint couplé, rewind et suppression de suffixe. Leur coût augmente les budgets physiques du WriteBatch ; il ne faut pas reprendre une ancienne qualification de tailles physiques en la prétendant inchangée. Le pruning states-only conserve les headers.

Les nouvelles API publiques sont `header_by_number(number, cap, check)`, `header_by_hash(hash, cap, check)` et `uncertified_tip_header(cap, check)`. Elles prennent un snapshot RocksDB sous mutex après vérification du poison. Toutes les lectures Get, les callbacks, la projection, le hash et le reverse index se font **hors mutex du stockage**, sur le même snapshot. La lease RAII est libérée à chaque retour/refus ; la poignée DB doit survivre à l'appel, comme pour les autres méthodes synchrones. Le poison est revérifié avant retour.

Une valeur d'index présente doit avoir un header valide, son hash déclaré exact, la hauteur exacte et son reverse index canonique cohérent. Inconnues ou duplications de champs d'index, hash absent, type invalide, encodage invalide ou divergence sont refusés. Un index présent invalide n'autorise jamais de fallback, réparation ou écriture.

**Index absent seulement** : lire un PinnableSlice legacy du bloc sur le snapshot, vérifier le cap configuré avant parsing, puis projeter les seuls champs Header par SAX. Aucune transaction ni son émetteur, withdrawals-body ou uncle n'est reconstruite. Le JSON entier reste validé syntaxiquement. Un itérateur vérifie la deadline tous les 4 096 octets, y compris au milieu d'une longue chaîne ; la profondeur est bornée à16. Les champs Header matérialisés sont limités à `min(cap,64KiB)` pour empêcher une conversion GMP géante sur disque corrompu. Aucun scan startup, migration CLI, backfill ou nouvelle version de schema global.

La compatibilité legacy reste **linéaire dans le corps** : le lexer peut allouer un token borné par le cap, et RocksDB conserve ses buffers/cache/I/O non préemptibles. Ce chemin n'est ni O(header), ni zéro parsing du corps, ni une borne de latence disque. Le chemin indexé ne consulte pas le body. L'index est dérivé ; il ne certifie ni disponibilité/intégrité du body ni authentification d'un checkpoint externe. Les audits globaux de corps/certificats ne sont pas remplacés. Les 64KiB bornent les valeurs scalaires matérialisées avant conversion, pas les overheads d'allocateur/JSON.

## Readiness Beacon et courses

`Blockchain::StateReadContext` copie seulement Header tip + optional Header finalized sous le mutex chaîne. Une chaîne Beacon doit déjà être ready et avoir son finalized authentifié par le coordinator. Son contrôle durable dédié lit le tip header sur snapshot et refuse tout marker ou record de certification legacy incompatible, sans appeler `certificate_impl` ou interpréter une empreinte comme un certificat. Les méthodes globales `require_execution_ready_locked`, `latest`, `certificate_impl`, `certificate_at`, `tip`, `tip_header` et `finalized_block` conservent leurs implémentations et garanties.

Chaque tentative capture les deux headers et la génération `event_sequence_`, effectue le contrôle durable hors verrous, puis recapture headers/génération pour distinguer un commit/finality concurrent d'une divergence stable. La génération existante détecte aussi A→B→A ; `restore` ne la réinitialise pas et émet un reset, la publication Beacon dérive sa nouvelle séquence de la précédente avec garde de débordement. Une autre activité peut conservativement provoquer un retry. Au maximum quatre tentatives sous la **même deadline** ; épuisement = `ExecutionCancelled("RPC state-read head changed during capture")`, RPC `-32002`, jamais une fausse annonce de corruption. Une divergence stable donne une erreur interne. Aucun callback n'est appelé sous mutex chaîne/stockage.

Après sélection, `serialized_state_at(number,hash,cap,check)` conserve la jointure existante : reorg avant pin = refus canonique ; reorg/pruning après pin = ancien état capturé exact. Pas de substitution d'un état de sœur. Les tags et erreurs EIP-1898, missing/pruned et pending gardent leur signification.

## Erreurs et oracles attendus

- Cap de lecture dépassé : `std::length_error("Serialized execution header byte limit exceeded")` ; RPC `-32005` avec le même message. Le cap concerne la valeur legacy complète ou la valeur d'index, pas les allocations internes RocksDB.
- Encodage/index/hash/number présent incorrect : `std::runtime_error`, RPC erreur interne `-32603`, sans résultat/fallback.
- Politique impossible, number hors uint64 ou hash non32 : `std::invalid_argument` avant accès, RPC params invalides lorsque pertinent.
- Annulation/timeout dans un long token : `ExecutionCancelled` se propage ; aucune mutation de base, pool, events ou réparation.
- Beacon : ready=false/finalized absent, poison, divergence durable/cache stable et métadonnées certifiées incompatibles restent refusés ; changement concurrent cohérent mène à retry borné, pas corruption.

Les tests ajoutés doivent établir : index indépendant d'un body JSON corrompu sur `ChainStore` seul (sans changer le startup de Blockchain), refus d'index présent invalide sans fallback, legacy valide/cap/cancel/depth, corps volumineux supérieur au cap mais petit header/état en RPC indexé, vraie progression cheap RPC + publication pendant une pause legacy hors verrous, les quatre writers et reorg/pruning/reopen/readonly, et readiness/races Beacon. La seule performance wall-clock n'est pas l'oracle anti-copie.

Les injections de réorganisation des tests existants sont réancrées aux mêmes phases sémantiques après insertion des checkpoints ; erreurs et résultats ne doivent pas être assouplis. Conserver les 24 vecteurs/5 groupes de preuve antérieurs. Les cibles/CLI CMake existent déjà ; ce lot ne doit pas annoncer les 20 CTest quotidiens exécutés ni une CI GitHub verte.

## Oracles prospectifs et limites d'exécution

`prospective-oracles.json` dérive les attentes des sources, avant toute exécution : coût47 checks/23 refus, readers805/179/5 groupes, proof5 groupes/63 refus/24 vecteurs publiés. Les nouveaux contrôles de proof gros corps utilisent `publish=false`. Les trois CLI existantes et leurs timeouts CTest90s sont conservés ; CMake ajoute seulement le chemin de fixture au test de coût, sans nouvelle cible/sélection quotidienne. Root adapte séparément les noms de cas au contrat de son worker (`unit + -self-test`), sans changer arguments ou oracles.

Le fixture Beacon final utilise `durable_owned_fixture.hpp`, deux vrais owners/témoins/réacteurs publics et cinq enfants complets des slots2 à6. Il prépare douze signatures BLS de test, plafonnées à32 ; aucun journal privé, ancien domaine ou duty réel. Il vérifie not-ready, latest/finalized prêts, un retry de publication réelle, quatre changements cohérents aux callbacks2/15/28/41 donnant52 callbacks puis busy, et une contradiction durable/cache volontaire terminale via l'API publique du stockage. Tous les readers sont détruits avant fermeture des owners ; aucun reopen/duty n'est lancé sur la paire volontairement contradictoire.

Un premier draft de test appelait à tort une méthode de publication privée. Ce défaut a été refusé **en revue source avant compilation/exécution/admission** puis remplacé par le vrai réacteur public ; il n'est pas un échec runtime ni un nouveau défaut Core démontré. Le cas réel de fork-choice A→B→A reste ouvert : le correctif de génération existante a été revu dans la source, mais le fixture linéaire ne le qualifie pas. `fullBeaconAbaQualified=false` est imposé par l'oracle. Les bornes depth/scalaires et le poison sont protégés dans la source ; ce lot n'ajoute pas d'injection physique de panne d'I/O incertaine. La finalité indépendante, la finalité commune/WAN, les duties, campagne04 et production restent non qualifiés.

Les quatre CPP et deux headers changent la closure source Core ; Root doit dériver le vrai ensemble de recompilation par la preuve des143 M. L'absence d'ajout de membres data/virtual n'est pas une qualification ABI/binaire. Aucun profil EVM/fork, règle de hash, consensus, signature, duty, réseau, fonds, vente ou campagne04 n'est activé par cette proposition.
