# État actuel du portage Nano

Le portage intégral n'est pas terminé. Ce document distingue les fonctionnalités
natives vérifiées des écarts avec le Python. `MIGRATION-HISTORY.md` conserve le
journal des étapes ; ses anciennes listes de tâches ne sont plus l'état courant.
`COMPLETION-AUDIT.md` relie les exigences aux preuves et aux travaux non clos.

## Référence et décisions

Source : `/Users/boris/Dev/blockchain-python`, commit
`77f2664390db135171608e7bbb613f4ea6468312`. Le dépôt original n'est pas modifié.
`source-inventory.json` inventorie les fichiers Python/JSON/protobuf/stubs de
référence, y compris nanolib. Cette bibliothèque est remplacée par cpp-libp2p,
selon la demande utilisateur : chaque module interne de nanolib n'a donc pas
vocation à être retraduit.

Le produit s'appelle Nano. Son EVM est native, sans evmone. `NanoOpCodes` déclare
169 valeurs distinctes et deux alias (SHA3/DIFFICULTY), d'après evm.codes et
EthereumJS ; déclarer les instructions proposées/EOF ne les active pas.
Les mentions NanoPy dans les fixtures, genesis et identifiants réseau désignent
la référence et restent nécessaires à leur traçabilité ou au protocole.

La cible utilisateur est désormais le niveau production pour un réseau public,
avec correction des défauts de la référence. Les critères de clôture figurent
dans `PRODUCTION-GOAL.md` ; le choix de règles corrigées est acté.

## Fonctionnalités et preuves

| Domaine | Implémentation et preuves actuelles |
|---|---|
| Comptes et transactions | GMP, Keccak, RLP strict, trie MPT, signatures secp256k1, transactions 0/1/2 et reçus ; fixtures Python dans `tests/fixtures/python-core.json` |
| Portefeuilles | Keystore V3, BIP39/32/44, EIP712, CLI et comptes RPC ; fixtures eth-account, tests core et wallet_cli |
| EVM | Instructions exécutées par le Python, appels imbriqués, CREATE/CREATE2, rollback et précompilés 1–9 ; tests core |
| Comparaisons EVM | 451 exécutions Python capturées : 400 arithmétiques, 24 mémoire/stockage/logs/erreurs, 15 appels imbriqués et 12 contextes de bloc ; provenance et divergence RETURN dans `tests/fixtures/python-evm.md` |
| Comparaisons précompilés | 111 appels Python capturés pour les adresses 1–7, dont 107 comparaisons exactes résultat/gaz et 4 rejets natifs de courbes invalides ; `tests/fixtures/python-precompiles.md` |
| PoW | Preuve Nano à 64 tours Keccak, difficulté, production et rejeu ; fixtures Python et tests chain/uncles |
| PoS | Sélection MT19937 comparée au Python, stake/verrouillage/récompenses/slashing dans le module ; registre de contrat synchronisé et persisté, validation automatique et distante testées |
| Mempool | Doublons, remplacement, limites, ordre des nonces, budget gas, soumission et inclusion ; tests core/chain/rpc |
| Stockage | RocksDB exclusivement, WriteBatch synchronisées, états et index historiques, réouverture et restauration de développement ; tests core/chain/migration |
| RPC | HTTP et WebSocket, filtres/logs, historique, frais, comptes, contrôle de validation et découverte ; tests rpc/http/events/validator |
| Transport | cpp-libp2p 0.1.37, Noise, multiplexage, identité persistante, bootstrap IP/DNS4/DNS6/DNSADDR, synchronisation et gossip ; tests bootstrap/network |
| Interopérabilité transport | Module Python chargé avec nanolib et extension de statut genesis dans la fixture ; échanges dans les deux sens et non-admission du statut historique ; moteurs C++ des deux côtés, test python_interop |
| Exécution | `nano run` supervise et arrête son enfant P2P ; installation CMake déplacée, lancement via PATH/lien symbolique ; tests supervisor/installed_node |
| Migration | `nano migrate` rejoue les historiques ChainDB JSON compatibles dans un nouveau RocksDB ; vérifie et publie sans écrasement ; tests migration/migration_cli |

## Audit RPC

`rpc-port-map.json` relève les 61 routes enregistrées dans le serveur HTTP Python,
avec hash du fichier source, fonctions et emplacements natifs. Les 61 noms ont une
route native ; cette présence ne prouve pas la parité de tous leurs paramètres,
erreurs et effets. Les abonnements WebSocket sont traités séparément.

Les 33 réponses de lecture de `tests/fixtures/python-rpc.json` viennent du
nœud et du serveur RPC Python originaux, sans persistance ni serveur ouvert.
La suite RPC compare les réponses JSON complètes (comptes, pool vide, recherches
absentes, identifiants et Keccak), avec des contrôles complémentaires des erreurs et des lots.
La provenance et les limites sont dans `tests/fixtures/python-rpc.md`.

Décision utilisateur : les mutations locales sont remplacées par des transactions
signées vers `contracts/NanoStaking.sol`. `nno_registerValidator`, `nno_addStake`
et `nno_withdrawStake` signent avec un compte local explicitement déverrouillé
et renvoient un hash de transaction. Les commandes CLI `stake-register`,
`stake-add`, `stake-withdraw` signent côté client et utilisent
`eth_sendRawTransaction`, sans portefeuille sur le serveur public.
L'état change à l'inclusion uniquement ; ce résultat remplace les booléens ou
montants immédiats du Python. Les suites staking et staking_cli vérifient le
cycle complet, les refus, le rejeu et la persistance. Les 454 contrôles natifs
couvrent aussi l’annulation d’un remboursement refusé et une tentative de
retrait réentrant depuis un destinataire contractuel. Voir `contracts/README.md`.
`nno_getStake`, `nno_getValidators`, `nno_startValidating` et
`nno_stopValidating` sont intégrées et testées.

L’audit des enveloppes, notifications et effets séquentiels des lots est
détaillé dans `RPC-BEHAVIOR.md`, avec les différences explicites du Python.

## Correspondance des commandes CLI

Le fichier source `nanopy/cli/main.py` expose dix commandes. Les syntaxes
changent pour utiliser les genesis, configurations JSON et keystores natifs :

| Commande source | Commande Nano | Vérification |
|---|---|---|
| node | run / serve | supervisor, http, installed_node |
| accounts --count | wallet create-many | wallet_cli : quantité, déchiffrement, permissions, refus d'écrasement |
| wallet --words | wallet hd-create / hd-derive | wallet_cli et fixtures HD |
| balance | rpc URL balance | rpc_cli |
| send | rpc URL send | rpc_cli : montants exacts, inclusion, reçu |
| info | rpc URL info | rpc_cli |
| nodes | rpc URL nodes | rpc_cli |
| network | rpc URL network | rpc_cli |
| mine --rpc | rpc URL mine | rpc_cli : bloc produit, refus hors dev_mode |
| deploy | rpc URL deploy | rpc_cli : déploiement, reçu et appel du contrat |

La génération de comptes conserve les clés complètes dans des keystores V3
chiffrés, au lieu d'imprimer un préfixe de clé comme le Python. Elle exige un
nouveau dossier et un mot de passe, avec 10 comptes par défaut et une limite de
100 par invocation. Les lanceurs séparés node/validator ont leurs propres
configurations ; ce tableau porte sur le groupe de commandes `nanopy/cli`.

### Client validateur séparé

`nano validator config.json [--once]` reprend la sélection, la demande de modèle,
la soumission et la bascule entre URL du client Python. Les tentatives réseau
utilisent le délai configuré et des reprises progressives ; une erreur RPC ne
provoque pas une bascule. `failover_timeout` est déclaré mais inutilisé dans le
Python original : aucun délai supplémentaire n'est inventé pour ce champ.

Les statistiques de session sont portées dans `stats` (montants exacts en wei)
et testées sur un bloc réel. `VALIDATOR_KEY`, `VALIDATOR_NODES` et
`VALIDATOR_BLOCK_TIME` permettent une configuration par environnement ; le JSON
explicite prime pour les nœuds et l'intervalle. L'inscription optionnelle du client (`register: true`) exige les pins `chain_id`
et `genesis_hash`, vérifiés avant chaque requête et chaque basculement RPC
(voir `BLOCK-SIGNATURES.md`). Elle signe localement et
attend le reçu avant la validation ; un stake existant évite un nouveau dépôt.
La suite staking_cli couvre aussi le délai expiré, sans production de bloc.

### Mineur PoW de bibliothèque

`Miner` porte le mineur en arrière-plan de `nanopy/consensus/pow.py` : callbacks
pour obtenir et soumettre un bloc, recherche limitée à 30 secondes par modèle,
arrêt, redémarrage et mesure du débit. Une absence de modèle attend une seconde,
avec interruption possible. Les exceptions des callbacks arrêtent le worker et
restent consultables par `error()`. Les callbacks doivent terminer : `stop()`
attend leur retour, sans pouvoir interrompre un code utilisateur bloqué.
L'objet est contrôlé par un seul thread propriétaire et possède son moteur PoW.
Ce composant n'active pas automatiquement le minage dans le serveur RPC.
La suite miner vérifie une preuve valide, le double démarrage, l'attente sans
modèle, l'annulation d'une recherche difficile, l'erreur et le redémarrage.

### Registre de portefeuilles

`KeyStore` conserve l'ordre d'ajout des comptes, comme le dictionnaire Python.
Un ajout répété ne déplace pas un compte ; un retrait suivi d'un nouvel ajout le
place à la fin. Retirer le compte par défaut sélectionne le premier restant.
`create_wallet(name)` crée et inscrit un portefeuille. Les tests core couvrent
cet ordre, les alias et l'absence de mutation si un nom est déjà utilisé.
Le nettoyage de tous les alias au retrait et le refus des collisions de noms
sont conservés : le Python peut laisser un alias utilisable après retrait.

## Écarts intentionnels et limites

- Les signatures et RLP sont contrôlés strictement. Un type 1 à liste d'accès
  vide reste de type 1 ; le Python peut le réinterpréter en type 0.
- Les hashes d'en-têtes sont recalculés. Le genesis Python peut conserver un
  hash périmé après allocation. Son moteur conserve aussi la base fee du parent
  et un hash d'oncles vides différent. Les fixtures ChainDB originales montrent
  ces incompatibilités ; leur import est refusé, sans réécriture silencieuse.
- Les récompenses liquides ne sont pas doublées comme dans le chemin du nœud
  Python. Les oncles PoW sont validés (ascendance, âge, preuve, inclusion unique).
- CALL/CREATE respectent les snapshots avant transfert, le mode statique et le
  rollback des logs. RETURN refuse une expansion mémoire dépassant le gaz ; le
  Python peut annoncer un succès consommant plus de gaz que fourni.
- BLAKE2F calcule la compression réelle au lieu du placeholder Python. BN128
  pairing utilise mcl avec validation des points et du sous-groupe. L'adresse
  précompilée 10 n'est pas implémentée, comme dans la source.
- BLOCKHASH consulte les 256 ancêtres ; le Python source renvoie un placeholder.
  La VM conserve certaines conventions de gaz de la source et limite la mémoire
  à 1 Mio. Elle n'est pas déclarée conforme à un hard fork Ethereum complet.
- Le contrat de staking configuré est la source des montants et statuts ; Nano
  reflète aussi les changements/retraits. Le contrat NanoStaking fourni est
  compilé avec solc 0.8.28 et testé dans le moteur natif. Il reste non audité.
  Les blocs PoS ne comportent toujours pas de signature de leur producteur ;
  signer les transactions de staking ne corrige pas cette limite.
- La réussite du test transport Python ne prouve pas la compatibilité des blocs
  historiques Python : ce test utilise des moteurs C++ des deux côtés. Le gossip
  de transactions du Python est un TODO source, pas un comportement à préserver.
- Les snapshots/horloges RPC sont réservés au mode développement local sans P2P.
  Leur restauration efface aussi les index persistés de la branche abandonnée.
- Les simulations RPC utilisent le prix effectif des transactions de type 2,
  calculé avec la base fee du bloc demandé et plafonné par maxFeePerGas. Les
  tests comparent GASPRICE, l'estimation et une transaction signée exécutée par
  le moteur, y compris les contextes historiques et le prix legacy explicite.
- La simulation de création refuse une adresse contenant du code ou un nonce,
  comme le moteur de transactions. Une adresse préfinancée sans code ni nonce
  reste admissible. Les tests vérifient le dépôt de code, la frontière exacte
  de gaz et l'absence de mutation, y compris après réouverture de RocksDB.
- `eth_coinbase` renvoie l'adresse de récompense configurée du nœud, comme
  dans le Python, et non le producteur du dernier bloc reçu. Le rechargement
  invalide d'une configuration laisse l'adresse précédente intacte.
- CREATE/CREATE2 imputent le dépôt de code au budget du constructeur. Son
  échec (gaz insuffisant ou code trop grand) consomme ce budget, conserve la
  réserve du parent et annule transfert, stockage et logs du constructeur.
  RETURNDATA est vide après cet échec. Huit scénarios vérifient aussi la limite
  où le dépôt tiendrait dans le budget du parent, mais pas celui du constructeur.
  Référence de la réserve : https://eips.ethereum.org/EIPS/eip-150.
- Le stockage garde un snapshot JSON complet par bloc ; l'incrémental serait
  une optimisation, pas une preuve supplémentaire de portage fonctionnel.
  RocksDB verrouille son dossier contre une ouverture concurrente par processus.
  `nano audit` rejoue la chaîne complète et vérifie ses données dérivées ; voir
  `STORAGE-AUDIT.md` pour les limites de cette vérification explicite.
- `nanopy/storage` utilise JSON, pas SQLite. SQLite concernait le stockage de
  pairs nanolib. La commande de migration n'est pas un convertisseur SQLite.
- mDNS/UPnP sont désactivés dans l'appel source à `new_host`. DHT et mDNS ne sont
  pas ajoutés implicitement au périmètre du remplacement cpp-libp2p.

## Compilation et validation

CMake Homebrew : `/opt/homebrew/bin/cmake` (réglage VS Code corrigé).
Les dépendances système GMP, OpenSSL, ICU et RocksDB restent nécessaires ;
l'installation CMake n'est pas une distribution autonome de ces bibliothèques.
Les scripts réseau acceptent `CMAKE_BIN` et `NANO_DEPS_DIR`.

```sh
cmake --build build-cmake -j4
ctest --test-dir build-cmake --output-on-failure
```

La configuration locale comprend 22 suites : core, chain, rpc, http, events,
validator, bootstrap, network, supervisor, wallet_cli, rpc_cli, migration,
migration_cli, uncles, python_interop, installed_node, staking, staking_cli, miner, block_auth, signed_validator_cli et signed_network. Les suites réseau et
interop exigent les exécutables/environnements configurés dans CMake. La suite
core contient 3 271 vérifications après les comparaisons EVM.

## Travail requis avant clôture

1. Authentifier cryptographiquement les blocs PoS avant un réseau public.
2. Compléter l'audit comportemental des API et commandes source, au-delà de la
   présence des routes et des correspondances de modules.
3. Terminer l’audit des différences EVM restantes (création de contrats,
   précompilés et cas limites des appels), en conservant les divergences
   explicites au lieu de revendiquer une parité fausse.
4. Versionner les règles corrigées et définir la transition des données ;
   ne jamais réécrire silencieusement les historiques Python divergents.
5. Effectuer l'audit final de chaque exigence et les essais de bout en bout.
