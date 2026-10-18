# Proposition de lot : présélection RPC sans décodage de Block

Revue de sources publiques uniquement, base vérifiée `801d793f0f05c3b46f97d34d86973f1487e36561`. Aucun programme du projet, test, import de module, build, Docker, SSH, SDK, autorité privée ou modification du dépôt n'a été exécuté. Cette proposition n'est ni une implémentation ni une qualification.

## Écart causal

- `src/rpc.cpp:624–643` crée le budget partagé, mais la ligne638 sélectionne encore un `Block` avant de rendre le worker et de relâcher le mutex RPC (`:895–896`). La seule vérification suivante est après cette sélection (`:639`).
- `src/storage.cpp:156–161,508–510` copie le JSON complet avec `Get(string*)`, le parse, puis `Block::from_json` sous le mutex du stockage. `src/block.cpp:12–16` reconstruit toutes les transactions et appelle `sender()` pour chacune. Le coût ne se limite donc pas à une copie du header.
- `Rpc::block`, `src/rpc.cpp:275–281`, appelle aussi `Blockchain::tip()` pour `latest`. `src/chain.cpp:173` copie `tip_` complet. `tip_header()` (`:174`) ne ferme pas seul le cas Beacon : `src/chain_beacon.cpp:4–9` appelle `store_.latest()` pour vérifier la tête durable. `finalized_block()` (`:13–16`) retourne également une copie complète.
- Les seuls index actuels sont `block/<height>`, `state/<height>` et `hash/<hash>` : deux publications (`src/storage.cpp:496,656`), deux suppressions de suffixe (`:458,629`). Aucun header indépendant n'existe. Un wrapper `Header::from_json(read(block/...))` laisserait le parsing/allocation du corps ; un SAX filtrant doit encore parcourir un corps legacy.
- `certificate_impl`, `src/storage.cpp:431–440`, décode lui aussi le Block et vérifie les signatures. Ne pas l'introduire dans le chemin RPC rapide ni remplacer globalement les garanties de `latest()`/`certificate_at()` par un simple index dérivé.

## Arbitrage recommandé

Ajouter un index dérivé `execution-header/v1/<height>` contenant exactement `Header::to_json()` (hash inclus). Le publier et l'effacer dans les **mêmes WriteBatch synchrones** que les quatre chemins block/hash ci-dessus. Aucun scan au démarrage, nouveau schema global, backfill dans un RPC ou migration silencieuse. Genesis est publié par `commit_impl`, donc reçoit aussi l'index ; les anciennes bases restent lisibles par le fallback décrit ci-dessous. Pruning ne supprime que les états et doit conserver cet index comme les blocs/receipts.

API indicative dans `ChainStore` :

```cpp
std::optional<Header> header_by_number(
    const Int&, size_t maximum_serialized_bytes,
    const std::function<void()>& checkpoint = {}) const;
std::optional<Header> header_by_hash(
    const Bytes&, size_t maximum_serialized_bytes,
    const std::function<void()>& checkpoint = {}) const;
```

Le cap est fourni par la politique RPC existante `maximum_snapshot_bytes` ; aucun membre ajouté à `RpcStateReadLimits`/`RpcSimulationLimits`, aucun changement d'ABI de layout nécessaire. L'API valide les tailles et bornes d'entrée avant accès, vérifie le poison d'écriture incertaine via le chemin habituel et capture index canonique + valeur `PinnableSlice` sous le mutex de stockage. **Copie bornée, callbacks, parsing, hash et projection se font après déverrouillage.** Aucun callback arbitraire sous mutex : les tests l'utilisent pour une vraie réorganisation.

Pour un index présent : nombre exact, champ hash obligatoire, recomputation via `Header::from_json`, hash demandé et reverse index `hash/<computed-hash>` doivent concorder. Une valeur présente malformée, excessive ou contradictoire est une corruption : **pas de fallback**. Ne pas écrire un header « réparé » depuis le chemin de lecture. L'index dérivé ne remplace pas l'authentification des blocs, la disponibilité du corps ni celle d'un checkpoint consensus.

Pour un index absent seulement : pinner le `block/<height>` legacy en cohérence avec le reverse index, puis libérer le mutex. Vérifier le cap avant de matérialiser une copie. Projeter les seuls champs Header sans DOM du bloc, transaction, `sender()`, uncle ou withdrawal-body. Le parseur/input doit vérifier le budget aussi **à l'intérieur des longues chaînes** (lecture par quanta bornés), pas seulement à la fin d'un événement JSON ; filtrer des événements seul ne borne pas un token calldata géant. Il faut consommer/valider le JSON complet et refuser un encodage corrompu. Le fallback reste linéaire et peut utiliser des buffers internes bornés : **ne pas le présenter comme zéro lecture/parsing du corps ou O(header)**. `Get`/cache/I/O interne RocksDB demeure non préemptible, comme pour `serialized_state_at:512–526` ; ce lot vise le CPU applicatif et les verrous, pas une borne physique de latence disque.

## Raccordement RPC et Beacon

`prepare_state_read` ne doit plus obtenir de Block sous le mutex RPC. Il valide/capture méthode, selector, clés et deadline, puis rend une closure. La sélection header historique/hash, le fallback éventuel et `serialized_state_at` s'effectuent dans cette closure après déverrouillage ; tous partagent le même `check`. Le nonce `pending` garde sa sémantique existante et ne fabrique pas de root pending.

Pour latest/finalized, fournir dans `Blockchain` un contexte **header seulement** (tip et éventuel finalized) sous le mutex chaîne, sans appeler `tip()`/`finalized_block()` ni la vérification globale qui charge un Block. Le contrôle dédié aux lectures Beacon doit conserver : ready/finalized disponibles, poison, appartenance canonique, tête durable liée à la tête cachée. Vérifier la tête via le nouveau chemin borné, hors mutex RPC. Si la tête a changé entre capture et contrôle durable, revalider/reprendre la capture sous le même budget plutôt que signaler faussement une corruption ; ne jamais appeler un callback utilisateur sous le mutex chaîne. Toute boucle de retry doit avoir une terminaison bornée et un refus d'activité concurrente distinct d'une corruption.

Conserver **inchangées** les API globales `require_execution_ready_locked`, `latest`, `certificate_impl` et leurs validations de corps/certificat. Le chemin dédié ne doit ni ignorer des métadonnées certifiées contradictoires (`certified_tip_impl:418–429`), ni appeler l'ancien décodeur coûteux pour les vérifier. Pour Beacon couplé, une présence de certification legacy incompatible doit rester un refus fermé ; ne pas prétendre valider un certificat en vérifiant uniquement son hash. Cette frontière doit faire l'objet d'une revue de code explicite avant exécution.

La jointure finale reste `serialized_state_at(header.number, header.hash(), cap, check)` : reorg avant pin ⇒ refus canonique ; reorg/pruning après pin ⇒ état capturé inchangé, jamais l'état d'une branche de remplacement. Conserver les codes Unknown block objet/legacy, requireCanonical, état pruné et timeout. Les dépassements de cap sont des refus de ressource `-32005`, les encodages persistés invalides des erreurs internes, sans résultat de repli.

## Lot et tests pertinents

Six fichiers de production ciblés : `include/nano/storage.hpp`, `src/storage.cpp`, `include/nano/chain.hpp`, `src/chain.cpp`, `src/chain_beacon.cpp`, `src/rpc.cpp`. Trois TU de régression ciblés : `tests/test_rpc_state_read_cost.cpp`, `tests/test_rpc_state_block_id.cpp`, `tests/test_rpc_get_proof.cpp`. `CMakeLists.txt:162–175` contient déjà les trois cibles et CLI : étendre le cas existant `rpc_state_read_cost --state-read-cost-self-test` plutôt qu'ajouter une cible/sélection CI équivalente. Aucun opcode, hash de bloc, règle consensus ou fork activé ne change.

1. **Oracle causal indexé** : stockage possédé, fermer son unique poignée, rendre seulement le body JSON invalide hors ligne, conserver header/hash/state ; rouvrir `ChainStore` seul, header indexé exact réussi. Le lecteur Block ancien échoue. Ne pas rouvrir `Blockchain` en prétendant que son audit de corps ignore désormais la corruption. Vérifier aussi index présent invalide ⇒ refus sans fallback.
2. **RPC gros corps, petit état** : bloc réel avec calldata/corps supérieur au cap opérateur, header et état inférieurs. Les cinq lecteurs (quatre scalaires + proof), latest/historique/hash retournent leurs valeurs exactes par index, sans coût dépendant des transactions. Ne pas employer seulement un seuil wall-clock comme oracle anti-copie. Le nouveau code ne doit contenir aucun appel Block/from_json/sender dans cette présélection.
3. **Legacy borné** : retirer seulement l'index hors ligne ; résultat header identique sur ancien JSON valide ; cap dépassé avant copie/parse ; annulation injectée au milieu d'une longue chaîne et timeout partagé avant la lecture d'état ; malformed JSON/number/hash/index ⇒ refus sans mutation ni réparation.
4. **Verrous et conservation** : pause à un checkpoint de présélection legacy hors verrous, prouver que cheap RPC et publication réelle avancent ; libération RAII des barrières même en échec, join avant destruction des objets. Canonical state/pool/events et contenu du disque inchangés pour un refus.
5. **Atomicité index/reorg/pruning/reopen** : nouveaux commits ordinaires et couplés, suppression de tous les headers abandonnés, remplacement de hauteur avec hash distinct, limites de retention, reopen et readonly. Aucun ancien hash ne doit sélectionner l'état de sa sœur ; index neuf et legacy donnent le même Header.
6. **Beacon** : latest/finalized body-free mais ready=false, tête durable/cache divergente, poison et certification contradictoire restent refusés ; commit concurrent cohérent n'est pas étiqueté corruption. Pas de nouvelle finalité/duty/campagne.

Les tests actuels de coût (`test_rpc_state_read_cost.cpp:15–24`) n'ont que le genesis (zéro transaction) et bloquent au poll64 après présélection. Les tests reorg/retention existants (`test_rpc_state_block_id.cpp:91–106,128`) fixent polls3/6/7 : l'ajout de checkpoints déplacera possiblement ces phases. Conserver les oracles de phase et erreurs exactes ; adapter les points de déclenchement selon la source revue, ne pas accepter les deux résultats ni simplement élargir les délais. Le garde/profil EVM/C6 précédemment qualifié reste historique ; une nouvelle cohorte Core ABI/source doit qualifier ce changement, sans réutiliser aveuglément un Core retenu ancien.
