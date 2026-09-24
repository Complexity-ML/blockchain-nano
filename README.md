# Nano — blockchain-nano

Nano est une blockchain C++20 avec EVM native, stockage RocksDB et réseau
cpp-libp2p. Le cœur et les tests d’intégration s’exécutent sans interpréteur
Python. Le prototype historique ne constitue plus une cible de compatibilité.
La préparation à un réseau public reste en cours ; voir `docs/PRODUCTION-GOAL.md`.
Les genesis livrés activent les signatures de producteur
(voir `docs/GENESIS-VERSIONS.md`).

## Compiler et tester

Dépendances : compilateurs C et C++20, GNU Make ou CMake 3.20+, GMP, OpenSSL et
RocksDB (version locale validée : 11.8.1) et Boost.Beast (Boost 1.92.0). JSON nlohmann 3.12.0 est fourni
avec sa licence dans `third_party/json.hpp`.

```sh
brew install cmake gmp openssl rocksdb boost icu4c
make -j4 all test
# Ou avec CMake :
cmake -S . -B build-cmake -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build build-cmake -j4
ctest --test-dir build-cmake --output-on-failure
```

Sur Linux, adapter le préfixe Make : `make PREFIX=/usr test`.
Le chemin CMake de ce Mac est configuré dans `.vscode/settings.json`.

## Modules disponibles

Cryptographie, RLP, trie Merkle Patricia, signatures et keystores, transactions
Legacy/EIP-2930/EIP-1559, état des comptes et contrats, blocs, reçus et bloom,
EVM native avec appels et créations, précompilés 1 à 9, consensus PoW/PoS
avec registre persistant, mempool, exécution et rejeu atomique des blocs.

`include/nano/opcodes.hpp` expose **NanoOpCodes** : 169 valeurs distinctes,
plus les alias SHA3 et DIFFICULTY, provenant des sources evm.codes / EthereumJS
10.1.0. Les instructions EOF/proposées du catalogue ne sont pas toutes exécutables.
Les sources et licences sont dans `third_party/opcodes/`.

## Stockage RocksDB

`ChainStore` utilise exclusivement RocksDB. Le chemin fourni désigne un dossier.
Chaque ajout regroupe bloc, état, transactions, reçus, index et tête de chaîne
dans un `WriteBatch`, avec WAL et synchronisation disque (`sync=true`).
Les racines et métadonnées sont vérifiées avant écriture ; les doublons et
les blocs qui ne prolongent pas la chaîne sont rejetés. La réouverture restaure
la tête, les états historiques, les transactions et les reçus.

L'état est actuellement conservé sous forme de snapshot JSON complet par bloc ;
le stockage incrémental des comptes reste à développer. Les anciens fichiers
SQLite ne sont pas convertis automatiquement. RocksDB verrouille son dossier
pour empêcher deux processus de l'ouvrir simultanément.

## Travail restant

La finalité BFT et sa machine de tours restent à intégrer au réseau. La conformité
à un fork EVM précis, les essais prolongés sous charge et partitions, la revue
indépendante et le bridge d’entrée/sortie restent ouverts. Les primitives et
scénarios déjà vérifiés sont décrits dans `docs/FINALITY.md` et
`docs/NETWORK-ROBUSTNESS.md`. L’EVM est native, sans evmone.

## Chaîne locale et CLI

`build/nano` permet d'initialiser et de rouvrir une chaîne RocksDB depuis un
genesis JSON, de miner une chaîne PoW, de produire un bloc PoS avec `validate` et d'importer
des blocs par rejeu.

```sh
./build/nano init genesis/testnet.json /tmp/nano-testnet
./build/nano status genesis/testnet.json /tmp/nano-testnet
./build/nano --help
```

Les genesis mainnet/testnet fournis sont `pos`. En l'absence de validateurs
inscrits, le producteur bootstrap est celui du Python. `validate` contrôle la
sélection du producteur ; `import` vérifie aussi les engagements d'exécution.
Le registre PoS (mises, verrous, compteurs) est enregistré avec chaque bloc et
restauré au démarrage. Les inscriptions, ajouts et retraits passent par des
transactions signées vers le contrat NanoStaking. Comme le format Python, le bloc
PoS ne contient pas de signature authentifiant son producteur.
Les oncles PoW sont validés lors du rejeu.
La sélection des transactions respecte les nonces même si les frais de la
transaction suivante sont supérieurs. Le pool est actuellement en mémoire.

Validation : 382 contrôles du cœur et 33 contrôles d'intégration de chaîne,
notamment rejeu entre deux bases, rejet d'un bloc falsifié sans modification de
l'état, reprise après fermeture et refus d'une identité de réseau incompatible.

## Serveur JSON-RPC HTTP

```sh
./build/nano serve genesis/testnet.json /tmp/nano-testnet 8545
```

Écoute locale par défaut sur `127.0.0.1` ; le dernier argument permet de préciser
une autre adresse. Arrêt par SIGINT/SIGTERM. Le serveur Boost.Beast limite le
corps des requêtes à 16 Mio, les en-têtes à 16 Kio et les connexions actives à 128.
Les lectures HTTP ont un délai de 15 secondes. CORS et OPTIONS sont pris en charge.

Méthodes raccordées : identité web3/net/eth, état du nœud local, blocs et
transactions par hash/numéro/index, reçus, balances, code, stockage, nonces,
`eth_sendRawTransaction`, `eth_call`, `eth_estimateGas`, `txpool_content`,
`txpool_status`, `nno_getValidators`. Les appels simulent sur une copie de l'état ;
l'estimation cherche une limite de gas exécutable. Les lectures historiques
utilisent les snapshots RocksDB. Le nonce pending compte les nonces contigus.

`eth_call` et `eth_estimateGas` acceptent le tag explicite `pending` : ils
simulent un bloc candidat et les transactions admissibles de la mempool sur
une copie de l'état. Le tag `latest` et le paramètre omis décrivent le bloc
courant. Les autres lectures de blocs/état `pending`, ainsi que les tags
`safe`/`finalized`, restent indisponibles et renvoient une erreur explicite.

L'estimation renvoie une limite vérifiée par simulation. Si le gas mesuré ne
suffit pas aux appels imbriqués, la recherche peut s'arrêter lorsque son
intervalle représente au plus 1,5 % de la limite proposée : elle ne promet
pas le minimum exact. Le plafond du demandeur et son budget restent respectés.
Un changement d'état avant inclusion peut modifier le résultat. Aucune clé
n'est chargée par défaut :
`eth_accounts` expose les comptes explicitement chargés ; il est vide par défaut.
`nano serve` démarre uniquement le RPC ; `nano run` raccorde aussi les pairs.
Les 61 noms de routes Python sont présents ; leur audit comportemental reste partiel.

Les simulations HTTP et WebSocket partagent deux workers dédiés et une capacité de
huit simulations par nœud, en cours ou réservées. Chaque appel d'un lot compte,
notifications comprises ; un lot doit réserver toute sa capacité avant de
commencer. Le surplus reçoit HTTP 503 ou une erreur JSON-RPC `-32005` sur
WebSocket, sans exécution partielle du lot refusé à l'admission. Une tâche qui
attend plus de quatre secondes avant de démarrer expire : HTTP 503 ou
JSON-RPC `-32005` avec `RPC simulation queue wait exceeded`. Le worker vérifie
aussi cette échéance avant toute exécution. La réservation reste comptée jusqu'au
retrait effectif de la tâche expirée. Sur WebSocket, les éléments non exécutés
restants du lot sont abandonnés ; les éléments déjà terminés ne sont pas annulés.
Les notifications ne produisent pas de réponse JSON-RPC. Le calcul utilise une copie
de l'état, du bloc et de ses hash d'ancêtres et libère le verrou RPC pendant
l'exécution EVM. Les abonnements WebSocket restent traités dans la boucle réseau.
La capture de l’état, des transactions et des ancêtres du candidat `pending`
reste synchronisée ; sa sélection et son exécution utilisent ensuite une copie
autonome hors des verrous RPC et chaîne pour les appels directs à `eth_call`
et `eth_estimateGas`. Les estimations internes du portefeuille et des commandes
staking libèrent aussi le verrou RPC pendant le calcul. Les commandes de signature,
d'envoi et de staking passent par la même file bornée, même si le gas est fourni.
Après une estimation, le compte doit toujours être déverrouillé ; le nonce implicite
est choisi sous verrou juste avant la signature et l'admission. Cette capacité ne
constitue pas un budget cumulé de gas ni un délai maximal d'exécution. Les
simulations disposent d'un délai coopératif de cinq secondes dès la préparation
de l'état, partagé par la construction du candidat `pending`, ses transactions
et tous les essais d'estimation. Son expiration renvoie `-32002` et restaure les
écritures de la simulation. L'attente en file et l'acquisition initiale du verrou
RPC restent hors de ce délai. Une copie d'état, un accès disque ou une routine
native déjà lancée ne sont contrôlés qu'au prochain point de contrôle ; le délai
n'est donc pas une borne stricte. La production et la validation de consensus
n'activent pas cette limite locale. L'arrêt du serveur annule également les simulations coopératives et empêche le démarrage des éléments encore en attente. Il ne préempte pas une routine native déjà en cours. Une déconnexion
WebSocket annule aussi le travail associé et libère sa réservation. La lecture
WebSocket reste active pendant les simulations : les réponses à des requêtes
distinctes peuvent arriver dans un ordre différent, avec leurs identifiants
JSON-RPC respectifs. Une erreur de socket HTTP explicite annule également le calcul surveillé. Une
simple fin du canal d'envoi reste compatible avec l'attente de la réponse.
La surveillance ne consomme pas les données d'une requête suivante ; si celles-ci
sont déjà disponibles, une rupture ultérieure peut attendre la prochaine
lecture ou écriture pour être détectée.

La suite native RPC comporte 931 contrôles lors de la qualification de
l’estimation des appels imbriqués. Des suites distinctes couvrent HTTP, le cœur
et la chaîne ; les résultats et limites sont conservés dans
[QUALIFICATION-FAILURES.md](docs/QUALIFICATION-FAILURES.md).

## Logs, filtres et WebSocket

Le même port de `nano serve` accepte HTTP et WebSocket (`ws://127.0.0.1:8545/`).
Les requêtes JSON-RPC ordinaires et les lots fonctionnent sur les deux transports.
`eth_subscribe` prend en charge `newHeads`, `newPendingTransactions` et `logs` ;
`eth_unsubscribe` retire uniquement les abonnements de la connexion concernée.

`eth_getLogs`, `eth_newFilter`, `eth_newBlockFilter`,
`eth_newPendingTransactionFilter`, `eth_getFilterChanges`, `eth_getFilterLogs`
et `eth_uninstallFilter` sont raccordés. Les filtres appliquent adresse(s), topics
positionnels et alternatives OR. Les logs incluent les index et les hashes du
bloc et de la transaction. Les blocs sont annoncés après leur commit RocksDB.

Les filtres et abonnements sont volatils. Le journal garde 10000 événements ; un
lecteur trop ancien doit recréer son filtre. Une recherche de logs est limitée à
10000 blocs et 10000 résultats. Limites : 1024 filtres HTTP, 64 abonnements par
WebSocket et 256 messages en attente par connexion. Les réponses WebSocket
sérialisées sont limitées à 8 Mio par connexion et 64 Mio partagés par serveur,
en incluant le message en cours d'écriture. La sérialisation s'arrête si le
budget disponible est dépassé ; la connexion est fermée. Cette fermeture
n'annule pas une transaction déjà soumise. Les tampons réseau du système,
les objets JSON avant sérialisation et la capacité des allocations ne sont pas
comptés dans ces plafonds de charge utile ; ils ne constituent donc pas un
plafond de mémoire totale du processus.
Les événements sont relevés toutes les 100 ms ; un client trop lent est déconnecté. Les réorganisations de
chaîne et les notifications `removed=true` restent à développer.

25 tests supplémentaires vérifient les filtres et les abonnements sur de vraies
connexions WebSocket, notamment les événements pending déjà minés au moment de
la lecture, les logs historiques, le désabonnement et l'isolation des connexions.

## Validateur natif séparé

```sh
# VALIDATOR_KEY contient la clé privée du validateur (jamais affichée).
./build/nano validator examples/validator.json --once
./build/nano validator examples/validator.json
```

Le mode continu vérifie la sélection puis produit à l'intervalle configuré. Le
mode `--once` effectue un cycle et quitte ; un validateur non sélectionné est
signalé comme ignoré. Les champs `nodes`, `block_time`, `rpc_timeout`,
`max_retries` et `validator_key` du JSON Python sont reconnus ; la variable
`VALIDATOR_KEY` sert de repli si la clé n'est pas dans le fichier.

Le client utilise HTTP/HTTPS, vérifie le certificat et le nom d'hôte TLS,
réessaie les erreurs de transport et bascule entre les nœuds. Les erreurs RPC
sont renvoyées directement. Les méthodes `validator_getBlockTemplate`,
`validator_submitBlock`, `validator_isSelected`,
`validator_getPendingTransactions` et `validator_getSyncStatus` sont disponibles.

Le template contient un bloc complet avec ses engagements d'exécution et les
alias `coinbase`/`blockNumber` attendus par le client Python. Sa création ne
modifie pas l'état. La soumission rejoue et vérifie le bloc ; les engagements
falsifiés et parents périmés sont rejetés. Resoumettre le même bloc déjà accepté
retourne son résultat sans nouvelle écriture. Les frais retournés viennent des
reçus effectivement enregistrés. Le statut `ready` décrit la chaîne locale ;
`networkConnected` et les hauteurs connues proviennent du transport en mode
`nano run`. Le mode RPC seul reste sans pairs.

22 contrôles supplémentaires couvrent client, bascule, délai, production via
HTTP, comptabilité, resoumission et reprise. L'inscription optionnelle au staking est disponible avec `register: true`
(voir `contracts/README.md`). Les statistiques du client restent en mémoire.

## Réseau cpp-libp2p

Le transport s'exécute dans `nano-p2p`, un processus C++ séparé. La version
épinglée de cpp-libp2p utilise BoringSSL et Boost 1.87 ; le cœur utilise OpenSSL
et Boost 1.92. Cette séparation évite de mélanger leurs symboles et ABI.
Le processus réseau communique avec le RPC du cœur sur la boucle locale.

```sh
./scripts/build-libp2p.sh
./scripts/build-network.sh
# Terminal 1 : cœur de chaîne
./build/nano serve genesis/testnet.json /tmp/nano-chain 8545
# Terminal 2 : transport (ajouter les multiaddresses des pairs en arguments)
./build-network/nano-p2p 8545 /tmp/nano-peer.key /ip4/127.0.0.1/tcp/30303
```

`CMAKE_BIN` sélectionne CMake pour `build-network.sh`, `NANO_DEPS_DIR` permet de
réutiliser un cache cpp-libp2p existant. L'adresse complète `/p2p/...` est affichée
au démarrage. La clé Ed25519 P2P est générée aléatoirement et enregistrée avec
permissions 0600 ; la clé de portefeuille n'est pas utilisée par le transport.

TCP + Noise + multiplexage cpp-libp2p raccordent les protocoles historiques
`/nanopy/handshake/1.0.0`, `blocksync`, `blockannounce`, `chainsync` et `txgossip`.
Les IDs historiques sont conservés pour le protocole ; le logiciel s'appelle
Nano. Les messages JSON peuvent être fragmentés entre plusieurs lectures.
Le lecteur repère progressivement la fin de l’objet puis analyse le JSON une
seule fois. Il limite chaque message à 16 Mio et 64 niveaux d’imbrication ;
lecture et écriture expirent après 10 secondes. Les messages sortants dépassant
16 Mio sont refusés. Le test `json_frame` du build réseau couvre fragments,
échappements, fins de message, tailles et profondeurs limites. Le daemon admet au plus 4 flux entrants simultanés par pair et 16 au total,
jusqu’à la fin de leurs traitements asynchrones. Un budget par pair permet une
rafale de 16 requêtes et se recharge à 8/s ; le budget global permet 128 requêtes
et se recharge à 64/s. Au plus 256 identités sont suivies ; les entrées inactives
depuis 60 secondes peuvent être évincées lorsque la table est pleine. Les
requêtes excédentaires sont refusées sans file d’attente. Ces limites portent
sur les flux applicatifs entrants après identification libp2p, pas sur les
handshakes cryptographiques, les connexions TCP ou les octets par seconde.
Les flux sortants et la résistance à des identités Sybil restent à qualifier.
Le handshake exige un chain ID entier positif et un hash genesis de 32 octets,
identiques à ceux du cœur local. Un pair omettant le genesis est refusé, même
s’il partage le chain ID ; les anciens pairs qui ne fournissent pas ce champ
ne sont donc plus admis.
La synchronisation demande au plus 50 blocs par lot et le cœur rejoue chaque
bloc reçu. Les transactions signées et nouveaux blocs sont diffusés aux pairs.

Tests d’intégration C++ (CMake configuré avec `NANO_NETWORK_EXECUTABLE`) :

```sh
ctest --test-dir build-cmake --output-on-failure -R '^(network|signed_network)$'
```

Ce test utilise deux nœuds locaux, synchronise des blocs historiques, propage
une transaction puis un bloc, compare état/reçus et vérifie l'identité après
redémarrage. Il vérifie aussi le refus d'un chain ID ou genesis incompatible.

Restent notamment la découverte automatique des pairs et la gestion des
branches/réorganisations. Le réseau utilise actuellement les pairs fournis
explicitement.

## Démarrage coordonné du nœud

```sh
./build/nano run genesis/testnet.json /tmp/nano-chain examples/node.json
```

Cette commande démarre RPC et P2P, surveille le processus `nano-p2p` et l'arrête
avec le cœur. Une sortie inattendue du transport arrête le nœud avec un code
d'erreur. Aucun redémarrage automatique n'est encore effectué.

La configuration accepte `rpc_host`, `rpc_port`, `p2p_host`, `p2p_port` et
`bootnodes` (multiaddresses complètes). `listen_addr`, `p2p_identity` et
`p2p_binary` remplacent les valeurs déduites si nécessaire. Les ports zéro sont
acceptés pour les tests. L'interface RPC doit inclure la boucle IPv4 locale pour
que le processus réseau puisse joindre le cœur.

Le transport transmet un rapport authentifié par un jeton éphémère fourni dans
son environnement. Les rapports expirent après cinq secondes sans actualisation.
Le superviseur retire de l’environnement du processus P2P `VALIDATOR_KEY`,
`NANO_PRIVATE_KEY`, `NANO_MNEMONIC`, `NANO_HD_PASSPHRASE`,
`NANO_WALLET_PASSWORD` et les variables de mot de passe déclarées dans `wallets`.
Le jeton réseau hérité est remplacé par un nouveau jeton propre au lancement.
Ce filtrage concerne ces variables connues ; le processus enfant conserve les
autres variables et les droits système de l’utilisateur qui le lance.
`net_listening`, `net_peerCount`, `eth_syncing`, `validator_getSyncStatus`,
`nno_getNodes` et `nno_getNetworkInfo` reflètent alors l'état observé. Le validateur
attend si un pair annonce une chaîne plus haute que la chaîne locale.

Le test `tests/test_supervisor.cpp` lance deux nœuds complets, vérifie leurs pairs
et leur synchronisation, l'arrêt coordonné, le retrait d'un pair et la reprise
des données après une panne provoquée du processus P2P.

`eth_feeHistory` renvoie les frais et taux de gaz des blocs RocksDB, ainsi que
les percentiles de pourboires pondérés par le gaz consommé. Maximum : 1024 blocs
et 100 percentiles. Les transactions legacy et EIP-1559 sont prises en compte.

## Portefeuilles locaux

`nano wallet create wallet.json` crée une clé aléatoire et un fichier Web3 V3
chiffré, avec permissions 0600. Définir `NANO_WALLET_PASSWORD` dans
l'environnement. `nano wallet import wallet.json` importe `NANO_PRIVATE_KEY`
vers un nouveau fichier chiffré. Un fichier existant n'est jamais écrasé.

`nano wallet address wallet.json` affiche l'adresse et la clé publique.
`nano wallet sign-message wallet.json 0x68656c6c6f` signe des octets en EIP-191.
`nano wallet sign-transaction wallet.json transaction.json` produit la transaction
signée (`raw`, `hash`) sans l'envoyer. Les clés privées ne sont pas affichées.
Les portefeuilles HD utilisent BIP-39/32/44 et ICU pour la normalisation Unicode.
`nano wallet sign-typed-data wallet.json typed-data.json` signe un document
EIP-712 (`types`, `primaryType`, `domain`, `message`) et renvoie hash et signature.

`nano wallet hd-create phrase.json [12|15|18|21|24]` crée une phrase anglaise
et la sauvegarde dans un fichier privé (0600), sans l'afficher dans le terminal.
Ce fichier contient la phrase **en clair** et doit être conservé comme un secret.
`nano wallet hd-derive wallet.json [index]` dérive un compte Ethereum et le
chiffre en Web3 V3. Définir `NANO_MNEMONIC`, `NANO_WALLET_PASSWORD` et,
si utilisée, la passphrase `NANO_HD_PASSPHRASE` (également lors de la création).
Le chemin de dérivation est `m/44'/60'/0'/0/index`.

## Comptes disponibles via RPC local

Ajouter une liste `wallets` à la configuration du nœud :

```json
{"wallets":[{"path":"/chemin/wallet.json","password_env":"NANO_ACCOUNT_PASSWORD","name":"principal"}]}
```

Le mot de passe est lu dans la variable indiquée au démarrage. `nano run` utilise
sa configuration habituelle ; `nano serve genesis.json db 8545 127.0.0.1 accounts.json`
accepte un fichier de comptes en dernier argument.
Les RPC `eth_accounts`, `eth_sign`, `personal_sign`, `eth_signTypedData_v4`,
`eth_signTransaction` et `eth_sendTransaction` utilisent ces comptes.
La signature de transaction seule ne l'envoie pas. L'envoi remplit les champs
absents (nonce pending, chainId, frais et estimation du gaz).

Les comptes restent déverrouillés pendant la vie du processus. Cette fonction
exige une écoute locale ; les requêtes HTTP/WebSocket comportant un en-tête
Origin sont refusées dans ce mode. Les applications de navigateur peuvent
utiliser leur propre portefeuille sur un nœud sans comptes chargés.

## Client RPC en ligne de commande

Le groupe `nano rpc` utilise le client HTTP/HTTPS natif et un endpoint explicite :

```sh
nano rpc http://127.0.0.1:8545 info
nano rpc http://127.0.0.1:8545 nodes
nano rpc http://127.0.0.1:8545 network
nano rpc http://127.0.0.1:8545 balance 0xADRESSE
nano rpc http://127.0.0.1:8545 send wallet.json 0xDESTINATION 1.25
nano rpc http://127.0.0.1:8545 deploy wallet.json contract.hex 120
nano rpc http://127.0.0.1:8545 receipt 0xHASH
```

`send` et `deploy` lisent `NANO_WALLET_PASSWORD` et signent localement ; aucune
clé n'est envoyée au serveur. Les montants sont exprimés en Nano, avec jusqu'à
18 décimales et conversion exacte en wei. Le nonce est lu sur `pending` et le
gaz est estimé avant signature. `send` retourne le hash après acceptation dans
la mempool ; `deploy` affiche d'abord le hash soumis puis attend un reçu réussi
(délai par défaut 120 secondes). Un délai expiré ne retire pas la transaction.
Le fichier de déploiement contient le bytecode de construction hexadécimal.

## Contrat de staking

Pour une chaîne PoS, `config.stakingContract` dans le genesis désigne le contrat
qui expose getValidators, getStake et isValidator avec l'interface NanoPy.
Le registre des validateurs est relu après chaque bloc et persisté avec celui-ci.
Le bootstrap cesse dès que le contrat possède du code ; une liste vide de
validateurs actifs arrête alors la production. Changer cette adresse sur une
base existante est refusé. Le contrat fourni `contracts/NanoStaking.sol` est testé dans le moteur natif,
avec transactions signées, rejeu par un pair et réouverture de RocksDB.
Le déploiement, les commandes et les règles sont détaillés dans
[contracts/README.md](contracts/README.md). Ce contrat n'a pas fait l'objet
d'un audit externe et le réseau n'est pas encore prêt pour la production.

La précompilation BN128 pairing (`0x08`) utilise mcl, fourni dans `third_party/mcl`
et lié statiquement. CMake compile cette dépendance automatiquement. La compilation
avec Make utilise également CMake pour mcl (`make CMAKE=/chemin/vers/cmake`).
Les résultats sont vérifiés avec les 14 vecteurs de pairing de go-ethereum.

Le précompile KZG (`0x0a`) du moteur de qualification Cancun utilise c-kzg-4844
v2.1.8 et blst, épinglés dans `third_party/ckzg/UPSTREAM.json` et liés
statiquement. Les paramètres publics Ethereum sont vérifiés par SHA-256 puis
intégrés au binaire. Les 180 cas officiels KZG et les 10 777 cas du corpus
d'état Cancun épinglé concordent. Cela ne prouve pas une conformité complète :
Cancun reste désactivé côté réseau, et le transport/stockage des blobs n'est
pas encore implémenté.
Le profil de qualification Prague ajoute les sept précompiles BLS12-381
EIP-2537 avec le même blst et les autorisations EIP-7702 de type 4.
Les 13 074 cas d'état Prague épinglés concordent ; Prague est indisponible
comme configuration réseau. Les résultats de blocs et transitions sont détaillés
dans le registre de qualification, avec leurs limites de validation du consensus.
Le profil de qualification Osaka ajoute CLZ, les règles MODEXP, les plafonds
de gas/blobs par transaction et le précompile P-256. Les 13 739 cas d’état
Osaka épinglés concordent ; Osaka reste indisponible comme configuration réseau.
Ce résultat ne qualifie pas PeerDAS ni la disponibilité des blobs.
Le rejeu Release courant des six corpus d'état Berlin, Paris, Shanghai, Cancun,
Prague et Osaka donne 44 244 cas concordants, sans erreur ni exclusion. Les
7 603 fichiers utilisés correspondent à l'archive officielle épinglée. Ce résultat
ne prouve pas la conformité Ethereum complète ni la préparation production.
La comparaison RPC Shanghai garde un désaccord explicite avec Anvil sur
l'admission des créations dont l'initcode dépasse 49 152 octets.
Voir [la qualification EVM](docs/EVM-CONFORMANCE.md),
[le registre des échecs et corrections](docs/QUALIFICATION-FAILURES.md) et
[les dépendances KZG](third_party/ckzg/README.nano.md).

## Snapshots de développement

Sur un serveur local sans P2P, activer `{"dev_mode": true}` dans le fichier de
configuration passé à `nano serve genesis.json db 8545 127.0.0.1 dev.json`.
`evm_snapshot` renvoie un identifiant ; `evm_revert [identifiant]` restaure
l'état, les index et le registre PoS, puis invalide ce snapshot et les suivants.
La mempool est vidée et les filtres/abonnements doivent être recréés.
Les identifiants de snapshot vivent en mémoire ; la restauration de la chaîne
est persistée sur disque. Ce mode est refusé par `nano run` et hors loopback.
`evm_increaseTime [secondes]` avance le temps utilisé par `evm_mine`.
`evm_setNextBlockTimestamp [timestamp]` fixe le prochain bloc manuel.
`evm_mine []` produit un bloc local PoW ou PoS et renvoie son hash.
Les snapshots restaurent aussi le décalage temporel et le timestamp programmé.

`nno_getStake(address)` renvoie le stake en quantité hexadécimale. Avec un
contrat de staking configuré, la lecture statique interroge directement
`getStake(address)`, y compris pour une adresse absente des validateurs.
Sans contrat, elle lit le registre PoS durable ; une adresse inconnue vaut zéro.
Un contrat absent vaut zéro ; une réponse mal formée ou un échec d'exécution
produit une erreur RPC, sans modifier l'état.

### Validation automatique du nœud

Avec un genesis PoS, configurez `coinbase` avec l'adresse d'un portefeuille
chargé par `wallets`, puis appelez `nno_startValidating` sans paramètres.
`block_time` règle l'intervalle en secondes (12 par défaut, de 1 à 3600).
Le serveur produit des blocs lorsque cette adresse est sélectionnée et que
la synchronisation est prête. `nno_stopValidating` arrête cette production.
Ces appels nécessitent le portefeuille coinbase déverrouillé et donc un
serveur lié à l'interface locale. Le statut est exposé dans
`nno_getNetworkInfo.node.validating` et `validationError`.

Le démarrage est explicite après chaque lancement ou rechargement des
portefeuilles. Le validateur automatique utilise l'heure réelle ; les réglages
`evm_*` de l’horloge restent réservés à `evm_mine`. Sur les genesis signés,
les producteurs et validateurs doivent fournir une signature de bloc valide.

### Import de l'historique Python

```sh
build/nano migrate genesis.json /chemin/chaindata-python /chemin/nouvelle-base-rocksdb
```

Arrêtez le nœud Python avant l'import. L'importeur C++ lit son format ChainDB
JSON (`meta.json`, `blocks/`, `transactions/`), rejoue chaque bloc et reconstruit
les états historiques, index et reçus dans RocksDB. Le dossier de destination
doit être absent, son parent doit exister, et il doit être extérieur au dossier
source. Les fichiers Python ne sont pas modifiés. La publication est atomique
sur macOS et Linux, après vérification de réouverture et des fichiers source.

Cet import exige un historique compatible avec les règles natives actuelles.
Les hashes périmés du cache Python, les divergences de frais ou de racines et
les blocs incomplets sont refusés avec un diagnostic. Les fichiers de cache
d'état et les mutations locales hors bloc ne sont pas importés ; les reçus
sont recalculés. Il ne s'agit donc pas encore d'une conversion universelle de
toutes les anciennes chaînes. Le stockage principal Python est en JSON ; son
utilisation de SQLite concerne le stockage des pairs de `nanolib`.

### Résolution des bootnodes

Le transport cpp-libp2p accepte les bootnodes IP, `/dns4` et `/dns6`, ainsi que
`/dnsaddr/nom-de-domaine` (enregistrements TXT `_dnsaddr.nom-de-domaine`). Un
suffixe `/p2p/PeerID` fixe l'identité attendue et reste contraignant dans les
réponses DNS imbriquées. Seules les adresses TCP avec une identité sont
retenues. La résolution DNSADDR utilise le résolveur cpp-libp2p/c-ares.

Les résolutions sont rafraîchies toutes les 60 secondes, avec au maximum
quatre niveaux et 64 adresses examinées par bootstrap. Les dernières adresses
résolues sont conservées en cas d'échec DNS. Au plus quatre connexions de
bootstrap sont tentées simultanément et 50 pairs Nano sont admis. Chaque pair
passe toujours le contrôle de chaîne/genesis après la connexion Noise.

### Validation des oncles PoW

L'import de blocs PoW contrôle les oncles avant les récompenses : deux au
maximum, profondeur de 1 à 6 blocs, parent dans l'ascendance récente, exclusion
des ancêtres canoniques et des oncles déjà inclus. Chaque en-tête est soumis
aux règles PoW Nano et à la règle de base fee de son parent. Le contrôle des
réutilisations repose sur les blocs RocksDB et survit donc à un redémarrage.
Les blocs PoS continuent de refuser les oncles. La sélection automatique
d'oncles lors du minage n'est pas implémentée.

### Tests natifs

Les tests CLI, réseau, installation, superviseur et reprise SIGKILL sont des
exécutables C++ pilotés par CTest. Le test d’interopérabilité avec le prototype
et les générateurs dépendant de son environnement ont été retirés. Les fixtures
JSON figées restent des données de test ; elles ne lancent pas le prototype.
MCL conserve ses outils amont optionnels, dont des scripts Python ; la compilation
Nano utilise ses sources natives pré-générées, sans invoquer ces scripts.

### Installer les exécutables

Après construction du transport (`scripts/build-libp2p.sh`, puis
`scripts/build-network.sh`), configurez le chemin de `nano-p2p` et installez :

```sh
cmake -S . -B build-cmake -DCMAKE_PREFIX_PATH=/opt/homebrew \
  -DNANO_NETWORK_EXECUTABLE="$PWD/build-network/nano-p2p"
cmake --build build-cmake -j4
cmake --install build-cmake --prefix "$PWD/local"
export PATH="$PWD/local/bin:$PATH"
```

Le préfixe contient `bin/nano`, `bin/nano-p2p`, les genesis dans
`share/nano/genesis` et la documentation dans `share/nano/docs`. Sans
`NANO_NETWORK_EXECUTABLE`, seul le moteur est installé. `nano run` cherche le
transport à côté du vrai exécutable, même lors d'un lancement via `PATH` ou
un lien symbolique ; `p2p_binary` reste disponible pour un chemin explicite.

Le préfixe d'installation peut être déplacé sur la même machine. Les
bibliothèques GMP, OpenSSL, ICU et RocksDB doivent rester installées : ce
préfixe n'est pas une distribution autonome de toutes les dépendances.
Le test `installed_node` vérifie l'installation dans un chemin avec espaces,
son déplacement, les lancements via PATH/lien et l'arrêt du processus P2P.

### Minage de développement par le client RPC

`nano rpc http://127.0.0.1:8545 mine` appelle `evm_mine` et affiche le hash du
bloc produit en JSON. Cette commande correspond au `mine --rpc` du CLI Python.
Elle exige un serveur en `dev_mode`, sans P2P supervisé ; sinon le client échoue
avec l'erreur du serveur. Elle ne requiert pas de portefeuille côté client.

### Génération de plusieurs comptes

`nano wallet create-many nouveau-dossier [nombre]` remplace la commande
`accounts --count` du CLI Python. Le nombre vaut 10 par défaut (1 à 100).
`NANO_WALLET_PASSWORD` fournit le mot de passe de chiffrement des fichiers
keystore V3 `0.json`, `1.json`, etc. La sortie JSON liste les adresses et chemins,
sans clés privées. Le dossier doit être nouveau : mode 0700, fichiers 0600,
aucun écrasement d'un dossier existant.

### Statistiques du validateur

Chaque réponse JSON du client `nano validator` contient `stats` : adresse,
`blocks_produced`, `total_rewards_wei`, `total_gas_fees_wei`, `total_earned_wei`,
`uptime_seconds`, `last_block_time` (secondes Unix ou null), `current_node` et
`running`. Les montants sont des chaînes décimales exactes en wei. Seuls les
blocs soumis avec succès sont comptabilisés. Ces statistiques sont celles de
la session courante, comme dans le validateur Python ; elles repartent de zéro
au redémarrage. Avec `--once`, `running` vaut false dans le résultat final.

Le client validateur lit aussi `VALIDATOR_NODES` (URL séparées par des virgules)
et `VALIDATOR_BLOCK_TIME` (1 à 3600 secondes) lorsque `nodes` ou `block_time`
ne sont pas définis dans son fichier JSON. Les valeurs explicites du fichier
priment. `VALIDATOR_KEY` reste disponible pour fournir la clé hors du fichier.
Les URL vides ou invalides sont refusées avant tout appel réseau.

### Docker

Le [déploiement Docker](docs/DOCKER.md) compile `nano` et `nano-p2p` dans une
image Ubuntu 24.04. Le fichier `compose.yaml` fournit un nœud sans clé de
validation, un volume persistant et un RPC publié sur `127.0.0.1:8545`.
Consulter la documentation pour choisir le genesis et configurer les pairs.
