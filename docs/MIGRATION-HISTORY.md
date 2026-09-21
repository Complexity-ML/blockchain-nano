# Journal historique — états intermédiaires

Ce document archive les étapes passées ; il ne décrit pas l’état actuel.
Consulter MIGRATION.md pour le suivi courant.

# Suivi du portage

## Source et inventaire

Source immuable de référence : `77f2664390db135171608e7bbb613f4ea6468312`.
`source-inventory.json` contient les chemins, tailles en lignes et SHA-256 des
353 fichiers Python/JSON/protobuf/stubs inventoriés (74 936 lignes). Les archives
`dist`, les métadonnées egg-info et les caches Python ne constituent pas du code
à traduire et sont exclus du projet C++.

## Correspondances

| Python | C++ | État |
|---|---|---|
| crypto/keys.py | crypto.cpp, types.cpp | Signatures comparées au Python |
| crypto/wallet.py | wallet.cpp, hdwallet.cpp, typed_data.cpp | Keystore V3, HD BIP39/32/44, EIP712 et CLI portés |
| core/transaction.py | transaction.cpp, receipt.cpp, engine.cpp | Types 0/1/2, signatures, reçus et exécution intégrés |
| core/account.py, core/state.py | state.cpp, rlp.cpp | Comptes, trie, snapshots portés |
| core/block.py | block.cpp, storage.cpp | Blocs et en-têtes d'oncles conservés ; ascendance et inclusion PoW vérifiées |
| genesis/*.json | genesis/*.json | Copie identique |
| vm/*.py | evm.cpp, precompiles.cpp, bn_pairing.cpp | Instructions source et précompilés 1 à 9 ; conformité aux forks à compléter |
| consensus/pow.py | consensus.cpp | Hash, difficulté, minage et validation d'en-têtes |
| consensus/pos.py, engine.py | pos.cpp, engine.cpp, staking.cpp | Sélection, récompenses, registre persistant et lecture du contrat intégrés |
| storage/*.py | storage.cpp, migration.cpp | RocksDB atomique ; import vérifié des historiques JSON compatibles, divergences Python restant à traiter |
| network/txpool.py | txpool.cpp | Porté, intégré et testé |
| network/rpc.py, http_rpc.py | rpc*.cpp, http.cpp, websocket.cpp | RPC courantes et contrôle validateur intégrés ; staking direct restant |
| network/p2p.py, libp2p_network.py | network/*.cpp | cpp-libp2p, bootstrap IP/DNS, sync et gossip testés ; transport Python vérifié, consensus historique divergent |
| nanolib/** | cpp-libp2p v0.1.37 | Remplacement explicite demandé par l'utilisateur |
| node/**, validator/**, cli/** | chain.cpp, node.cpp, client.cpp, cli_client.cpp | Nœud supervisé, client validateur et CLI intégrés ; couverture complète à auditer |

## Tests effectués

`make -j4 test` : 163 vérifications passent sur macOS arm64 avec Apple Clang.
Les fixtures `tests/fixtures/python-core.json` viennent directement des modules
Python source et de leurs dépendances eth-keys, eth-utils, RLP et HexaryTrie.
La suite compare le hash de signature, les octets signés, le hash final et
l'adresse récupérée pour 24 transactions. Elle compare aussi des tries de
0/1/2/16/100 entrées, les allocations genesis, du stockage contractuel et plusieurs
longueurs autour de la taille de bloc Keccak.

## Différences intentionnelles et points à résoudre

- Décodage RLP strict, rejet de signatures high-s et vérification des largeurs.
- Le type 1 avec liste d'accès vide reste de type 1 ; Python le réinterprète
  parfois en type 0 dans `Transaction.__post_init__`.
- Les hashes d'en-têtes sont recalculés : pas de cache susceptible de devenir
  périmé après mutation d'un champ.
- Le code source EVM contient des particularités de gas, des erreurs de rollback
  et des précompilés incomplets. Définir et tester explicitement les écarts
  avant toute prétention de compatibilité consensus/réseau existant.
- Vérifier tous les points publics de `nanolib` ; une simple substitution TCP
  ne serait pas équivalente à libp2p (Noise, mux, discovery, pubsub, QUIC, etc.).

## Suite

1. Porter EVM/précompilés et tester appels imbriqués, gas et rollback.
2. Porter consensus PoS/PoW, pool, exécution des transactions et persistence.
3. Porter RPC HTTP/WebSocket et valider les abonnements.
4. Porter le réseau et tester synchronisation entre deux nœuds locaux.
5. Compléter CLI/validateur, audit de couverture de l'inventaire, build CMake et
   validation de bout en bout avant de déclarer le portage complet.

## Décisions utilisateur et avancement EVM

- Pas d’evmone : moteur EVM C++ propre à NanoPy.
- cpp-libp2p retenu explicitement pour le réseau ; version et commit dans
  `dependencies.lock.json`, compilation via `scripts/build-libp2p.sh`.
- 171 vérifications passent après les premiers tests EVM. Les valeurs de gas
  reproduisent pour le moment la table et les suppléments du Python, qui ne
  correspondent pas tous aux règles Ethereum.
- Appels imbriqués, CREATE/CREATE2 exécutés et précompilés restent à implémenter.
  Un opcode non pris en charge produit une erreur explicite.
- Le script de build cpp-libp2p est ajouté mais la compilation et l’intégration
  du transport ne sont pas encore validées.

## Deuxième étape EVM

190 vérifications passent avec Make. CALL, CALLCODE, DELEGATECALL, STATICCALL,
CREATE et CREATE2 sont implémentés. Les tests vérifient les transferts de valeur,
le rollback du sous-appel et du parent, les logs annulés, la propagation du mode
statique, le stockage du delegate et le déploiement du code retourné par CREATE.

Précompilés implémentés : ECRECOVER, SHA256, RIPEMD160, identité, MODEXP,
BN128 addition et multiplication. Pairing et BLAKE2F restent à porter ; adresse
10 reste sans implémentation comme dans la source Python. Ils échouent
explicitement en attendant leur implémentation.

Les appels corrigent certains comportements erronés de la source : snapshot avant
transfert, propagation du mode statique, retrait des logs en échec, contrôle des
collisions CREATE et coût de dépôt de code. Ces écarts doivent être couverts et
explicités avant de conclure à une compatibilité avec les blocs historiques.
Les coûts de base restent pour le moment ceux de NanoPy, y compris leurs écarts
avec Ethereum. Le moteur n'est donc pas annoncé comme conforme à un hard fork
Ethereum complet.

cpp-libp2p : compilation active via Hunter dans `.deps`, cache local ignoré par
Git. Le handle de compilation courant appartient à cette session de travail ;
consulter le processus et `work/libp2p-build.log` avant toute relance.

## Nom et import du catalogue

Nom utilisateur : Nano. Nom exact de l’enum : `NanoOpCodes`.
169 opcodes distincts et 2 alias importés à partir d’evm.codes
(commit `224365a22b948c0465e44f08e8496cdf41945d48`) et de sa dépendance
EthereumJS EVM 10.1.0. Les instructions EOF/proposées sont déclarées, sans
activation implicite. Table vérifiée intégralement nom/valeur.
Les mentions NanoPy dans les documents historiques, fixtures et données genesis
désignent la source Python et sont conservées pour la traçabilité.

## Étape BLAKE2F et PoW

206 vérifications passent avec Make. BLAKE2F calcule la compression réelle ;
les vecteurs officiels 4 et 5 de https://eips.ethereum.org/EIPS/eip-152 passent,
avec rejet des mauvaises longueurs/drapeaux et dépassement du budget gas.
Cela remplace volontairement le placeholder Python qui renvoie son entrée h.
BN128 pairing (adresse 8) est maintenant porté avec mcl ; voir ci-dessous.

La preuve de travail simplifiée (64 tours Keccak) et l'ajustement de difficulté
sont comparés directement au Python dans `tests/fixtures/python-pow.json`.
Minage à difficulté 1 et validation d'en-tête sont testés. Ce module ne constitue
pas encore le moteur complet de validation/exécution/finalisation de blocs.
Le moteur PoS et sa sélection déterministe compatible Python restent à porter.

La compilation cpp-libp2p continue : Boost terminé, libsecp256k1 en cours au
dernier contrôle. Le dépôt Python original est toujours sans modification Git.

## Proof of Stake et compilation réseau

344 vérifications passent. `pos.cpp` porte l'inscription, l'ajout de stake,
le verrouillage de 100 blocs, le retrait, les récompenses composées, le slashing
et la sélection pondérée. Le générateur MT19937 reproduit le seeding entier,
getrandbits et la rejection sampling de Python ; 128 sélections sont comparées
au Python avec plusieurs montants de stake. Les interactions avec le contrat
staking et la persistance du jeu de validateurs restent à intégrer.

cpp-libp2p : la première compilation a échoué dans fmt (dépendance de qtils),
sur un diagnostic consteval AppleClang. Le patch local
`patches/fmt-appleclang-consteval.patch` désactive ce contrôle à la compilation
sur AppleClang pour cette version de fmt, en conservant le formatage runtime.
Il est appliqué au cache source fmt ; l'automatisation de ce patch dans une
installation propre reste à terminer. Reprise en cours, journal de travail :
`work/libp2p-build-patched.log`. Ne pas confondre cette reprise avec un succès
final du build réseau.

## Pool de transactions

354 vérifications passent. Le pool C++ est protégé par mutex et gère doublons,
remplacements à +10 %, limite par compte, capacité, recherche, suppression des
transactions minées, priorité des frais, budget gas et contenu JSON.
La limite de remplacement utilise des entiers exacts (pas de flottant).
L'éviction retire réellement le plus faible prix, corrigeant le heap Python qui
pouvait retirer la première entrée. Les nonces reflètent le maximum en attente,
comme la source ; la validation séquentielle par rapport à l'état doit encore
être ajoutée dans le moteur de blocs.

Hunter réextrait fmt et efface le patch source. Le build réseau a donc été
relancé avec `-DFMT_CONSTEVAL=` dans les flags du script sur macOS ; ce réglage
survit à la réextraction. Journal actuel : `work/libp2p-build-flags.log`.
Le patch source précédent est conservé pour expliquer le diagnostic, il n'est
plus le mécanisme effectif de compilation.

## Reçus et racine de trie

360 vérifications passent. `Receipt` gère les métadonnées JSON-RPC, logs/bloom,
encodage RLP et préfixes de type 1/2. Les valeurs sont comparées à rlp,
eth-bloom et HexaryTrie ; le décodage JSON rejette un bloom corrompu.
L'intégration des reçus au moteur d'exécution des transactions reste à faire.

Le flag fmt au niveau du projet n'était pas transmis par Hunter. La reprise
actuelle utilise `cmake/libp2p-toolchain.cmake`, partagé avec les dépendances.
Journal de cette reprise : `work/libp2p-build-toolchain.log`.

## Exécution de blocs et compilation libp2p

370 vérifications passent. `ExecutionEngine` relie signatures, nonce, solde,
EVM, frais, remboursement, déploiement, reçus, récompenses et racines de blocs.
`execute_block` applique un candidat atomiquement ; `verify_block` rejoue ses
transactions et rejette les engagements d'état/gas/transactions/reçus incorrects.
Validation des en-têtes/PoS, persistance et réception réseau restent à connecter.

Tests : transfert simple à 21000 gas, conservation des balances/frais, hash final
dans les reçus, rejeu déterministe, rejet de racine falsifiée avec rollback,
rollback de bloc après nonce dupliqué, appel revert conservant le nonce et les
frais mais annulant valeur/stockage. Les frais vont au producteur comme en Python.
Le double comptage du gas intrinsèque du moteur Python est corrigé ; les écarts
historiques doivent être explicitement traités avant d'importer une chaîne existante.

cpp-libp2p v0.1.37 : build et installation terminés avec code de sortie 0 via
`cmake/libp2p-toolchain.cmake`. Artefacts dans `.deps/install` du checkout de
travail ; transport pas encore lié au nœud Nano. Le toolchain corrige bien le
problème fmt des compilations imbriquées. Journal : `work/libp2p-build-toolchain.log`.

## Remplacement du stockage par RocksDB

`ChainStore` utilise RocksDB 11.8.1, sans dépendance SQLite. Blocs et snapshots
d'état JSON sont indexés par hauteur ; hashes, transactions et reçus disposent
de leurs propres clés. Un WriteBatch avec WAL synchronisé publie atomiquement
les données et métadonnées de chaîne. Un marqueur de schéma protège l'ouverture
d'une base incompatible. L'API de stockage reste identique.

Tests : réouverture, états historiques, index, reçus, en-têtes des oncles, rejet
des doublons, absence d'écriture partielle après rejet d'un batch préparé et
exclusion d'une seconde ouverture concurrente. Aucune conversion automatique
de fichiers SQLite ni mesure de débit n'est fournie. Le stockage incrémental
et le branchement au futur nœud restent à faire.

## Cycle de chaîne et première CLI native

`Genesis`, `Blockchain` et `nano` raccordent configuration genesis, état,
exécution, mempool, consensus PoW et RocksDB. L'identité persistée associe ID de
chaîne, hash genesis et mode de consensus. Les écritures réussissent avant la
publication du nouvel état en mémoire. La CLI propose init/status/mine/import.
Les 14 tests de chaîne vérifient également l'ordre des nonces, le rejeu par un
second gestionnaire de chaîne et la restauration de l'état canonique.

Limites explicites : pas encore de serveur réseau/RPC, d'import PoS, de registre
persistant des validateurs, de réorganisation de chaîne ou de validation complète
des oncles. La CLI est un outil local ; elle n'est pas encore le nœud complet.

## Raccordement PoS et snapshots des validateurs

Production `validate` et import PoS sont raccordés à la chaîne. La sélection
pondérée conserve l'algorithme Python ; le bootstrap utilise l'adresse source.
Les snapshots incluent mises, activation, inscription, verrou, dernier bloc et
compteur ; ils sont écrits dans le même batch RocksDB que le bloc et l'état.
Les doublons de validateurs et snapshots invalides sont rejetés à la lecture.

Correction de l'incohérence Python : `validate_and_create_block` ajoutait une
seconde récompense liquide après finalisation de la racine. Le port C++ conserve
la récompense liquide du moteur, avant finalisation, et applique la récompense
composée au registre des validateurs. Cette correction est un écart économique
avec le producteur Python historique ; importer une chaîne existante requiert
un traitement de migration explicite, pas une prétention de compatibilité exacte.

Tests : producteur inattendu, seal invalide, racine falsifiée, registre absent
après import rejeté, égalité entre producteur et réplique, reprise bootstrap et
reprise d'un registre non vide avec ses récompenses et compteurs.
Le format de bloc source n'authentifie pas le producteur par signature. Le
contrat de staking est désormais raccordé (voir ci-dessous) ; l'inscription directe via le nœud reste à porter.

## JSON-RPC et HTTP natifs

Ajout de `Rpc` et du serveur HTTP asynchrone Boost.Beast, exposé par `nano serve`.
Le JSON-RPC 2.0 prend en charge lots, notifications et codes d'erreur standard.
Les appels EVM sont isolés, avec état et contexte du bloc demandé. L'estimation
utilise une recherche de gas exécutable au lieu de la constante de secours du
Python. Les reçus RPC complètent les métadonnées des logs et le prix effectif.

Tests : 22 contrôles RPC (historique, simulation, estimation, envoi signé,
transactions pending puis minées, index, erreurs) et 6 contrôles HTTP réels.
Le serveur reste une interface locale sans production automatique ni P2P.
Les méthodes non portées renvoient Method not found, sans réponses factices.

## Filtres et WebSocket

Ajout d'un journal d'événements borné dans `Blockchain`, partagé par les filtres
RPC et abonnements WebSocket. Les nouvelles têtes ne sont publiées qu'après
persistance. Les événements pending restent lisibles après minage tant que le
journal les conserve. `eth_getLogs` et les filtres interrogent les reçus réels ;
ils remplacent les réponses vides simplifiées du Python.

Boost.Beast prend en charge l'upgrade sur le port HTTP existant. Les sessions
sérialisent leurs écritures, bornent leurs files et libèrent leurs abonnements à
la fermeture. Les abonnements sont propres à chaque connexion ; les lots imbriqués
sont rejetés. 25 contrôles supplémentaires portent sur les événements et le
transport WebSocket. Réorganisations et notifications de logs supprimés restent
non implémentées, comme la gestion de branches dans le stockage actuel.

## Méthodes et client du validateur

Le pipeline de préparation est commun au producteur local et aux templates RPC.
La création d'un template simule sur une copie ; la soumission vérifie le bloc
complet et ses alias, plutôt que d'ignorer les engagements fournis comme le
serveur Python. Un doublon accepté est idempotent pour permettre la reprise
après perte d'une réponse réseau. Les erreurs ne renvoient pas de traceback.

Le client C++ HTTP/HTTPS porte les réessais et la bascule, les contrôles de
sélection et le cycle template/soumission. La commande `nano validator` accepte
un fichier de configuration et `--once`, avec clé fournie via `VALIDATOR_KEY`
ou configuration. 22 tests supplémentaires utilisent des connexions locales,
un endpoint indisponible et un endpoint muet ; ils vérifient aussi restauration
du registre et de la chaîne après production distante.

Les statistiques client persistantes, inscription automatique, endpoints réseau
et synchronisation entre pairs restent incomplets. Le format PoS est toujours
celui sans signature d'en-tête décrit plus haut ; dériver l'adresse depuis la clé
dans le client n'ajoute pas à lui seul une preuve cryptographique au bloc.

## Transport cpp-libp2p fonctionnel

Le nouveau sous-projet `network/` construit `nano-p2p` contre la version
cpp-libp2p déjà épinglée. Il utilise TCP/Noise et une identité Ed25519 persistée.
Son processus séparé empêche les collisions OpenSSL/BoringSSL et Boost.
Le cœur expose des lectures de statut, lots de blocs et événements ; les imports
passent par le vérificateur existant, jamais par un chemin sans validation.

La vérification locale couvre synchronisation initiale, gossip de transaction,
annonce d'un nouveau bloc, égalité hash/racine/reçu et conservation du peer ID.
Les IDs de protocole Python sont conservés. Le gossip TX est désormais raccordé,
alors que le module Python examiné déclarait cet ID sans installer son handler.
Il utilise un objet JSON {raw: transaction_signee_hex}.

Interopérabilité Python réelle, découverte et gestion de forks restent à vérifier/porter. La vérification
locale à deux processus réseau ne vaut pas validation sur réseau public.

## Supervision et télémétrie P2P

`nano run` lance le cœur et `nano-p2p` avec `posix_spawn`, supervise le processus
enfant et garantit son arrêt/récupération à la sortie. Si le transport termine
inopinément, le cœur sort en erreur. La clé de liaison éphémère est transmise par
environnement ; les rapports réseau sans ce jeton sont refusés.

La télémétrie alimente les RPC de pairs, synchronisation et disponibilité du
validateur. Elle expire après cinq secondes. Les bootstrap multiaddresses sont
les valeurs effectivement configurées, sans liste de serveurs publics inventée.
Quatre tests RPC supplémentaires vérifient autorisation et état de synchronisation.
Le test de supervision couvre deux nœuds, leur arrêt et la panne d'un enfant,
avec relecture de la chaîne conservée sur disque.

## Historique des frais RPC

`eth_feeHistory` lit les blocs et reçus persistés. Les frais de base, taux de gaz
et pourboires effectifs remplacent les constantes renvoyées par Python.
Les percentiles sont pondérés par le gaz consommé, pour transactions legacy et
EIP-1559 ; un bloc vide renvoie des récompenses nulles. Le dernier frais de base
est calculé avec la même règle que la production du prochain bloc.
Les requêtes sont limitées à 1024 blocs et 100 percentiles ordonnés entre 0 et 100.
Les tags pending, safe et finalized restent indisponibles comme pour les autres
lectures de blocs. Les champs de blobs ne sont pas exposés : Nano ne traite pas
encore les transactions de type 3.
Référence : https://ethereum.github.io/execution-apis/api/methods/eth_feeHistory/

## Résolution des ancêtres dans la VM

`BLOCKHASH` consulte désormais les en-têtes RocksDB via le contexte d'exécution,
pour la production, le rejeu de blocs et les simulations RPC. Le contexte est
transmis aux appels imbriqués. Les blocs courant, futurs et plus anciens que
256 blocs renvoient zéro sans lecture disque. La VM isolée sans fournisseur
conserve un résultat nul.

Cela complète une instruction laissée factice dans Python. Les contrats qui
l'utilisent peuvent donc produire un état différent du prototype Python ; ce
changement doit être coordonné pour toute chaîne existante.

## Gestion des portefeuilles

`Wallet` expose création, chiffrement Web3 V3, relecture, signature de messages
et de transactions. `KeyStore` gère noms, adresses, compte par défaut et
suppression. Contrairement au Python source, supprimer un compte supprime tous
ses alias ; une collision de noms est rejetée plutôt que de remplacer un compte.
Le registre reste en mémoire et est raccordé aux comptes RPC configurés.
La CLI locale `nano wallet` permet création/import et signature avec mot de
passe par environnement. HD BIP-39/32/44 est décrit ci-dessous.

## Signature EIP-712

Le portefeuille et la CLI signent désormais les données structurées EIP-712.
L'encodeur traite dépendances triées, structures récursives finies, tableaux
fixes/dynamiques imbriqués, entiers signés/non signés, octets et chaînes UTF-8.
Les tailles, bornes entières, champs manquants/supplémentaires et types inconnus
sont rejetés ; bool ne convertit pas implicitement chaînes ou nombres.
Les limites de profondeur et de volume bornent les documents pathologiques.

Sept vecteurs comparés à eth-account 0.13.7 vérifient digest et signature exacte,
dont l'exemple Mail de l'EIP, limites numériques, tableaux et domaine avec salt.
Les fixtures se régénèrent avec `scripts/generate-typed-fixtures.py` dans un
Python disposant de eth-account ; Nano n'en dépend pas à l'exécution.
Le document doit contenir explicitement EIP712Domain et primaryType.
L'intégration aux comptes RPC est décrite ci-dessous.
Référence : https://eips.ethereum.org/EIPS/eip-712

## Portefeuilles HD

BIP-39 anglais : génération 12/15/18/21/24 mots, décodage et checksum.
BIP-32 : dérivation privée, enfants normaux/durcis et chemins explicites.
BIP-44 Ethereum : comptes `m/44'/60'/0'/0/index`.
ICU normalise les passphrases Unicode en NFKD avant PBKDF2-HMAC-SHA512.
Les 15 vecteurs eth-account couvrent cinq tailles d'entropie, trois passphrases
et quatre chemins par graine ; ils vérifient graines, clés et adresses exactes.
Les tests CLI vérifient création de phrase, restauration et export chiffré.
L'export xpub/xprv n'est pas exposé, comme dans le Wallet Python examiné.
La liste anglaise MIT et sa provenance sont dans third_party/bip39.

## RPC des comptes

Chargement atomique des keyfiles chiffrés à partir de la configuration, liste
eth_accounts, signatures EIP-191 et EIP-712, signature et envoi de transactions.
Les champs absents utilisent la chaîne active et le nonce pending contigu.
Legacy, EIP-2930 et EIP-1559 sont pris en charge ; les types et frais incohérents
sont refusés. Deux transactions locales successives sont testées jusqu'au minage.
Une erreur de rechargement ne remplace pas les comptes déjà chargés.
Le mode comptes exige loopback et refuse Origin avant traitement RPC ou upgrade
WebSocket ; les secrets de portefeuille sont exclus de l'environnement P2P.
Les méthodes personal de création/déverrouillage à distance ne sont pas exposées.

## Commandes clientes

Le groupe natif `nano rpc URL` porte balance, info, nodes, network, send et deploy
de la CLI Python. Les clés proviennent des keyfiles chiffrés locaux et les montants
utilisent des entiers exacts, remplaçant la conversion float du prototype.
Les paramètres source --private-key et --from sont remplacés par le keyfile,
qui fournit l'adresse du signataire. Le nonce pending évite le doublon entre
envois successifs. Déploiement : estimation du gaz, signature, soumission et
attente du reçu avec délai explicite ; la commande receipt permet le suivi.
Un test avec vrai nœud PoS local vérifie deux transferts dont un wei, puis le
déploiement et l'exécution d'un contrat retournant 42. Les comptes du serveur
restent verrouillés : le client signe localement.

## Registre PoS issu du contrat de staking

`config.stakingContract` dans le genesis active la lecture du contrat par la VM
native : getValidators (b7ab4db5), getStake (7a766460), isValidator (facd743b),
avec les sélecteurs utilisés par le Python source. Les appels sont statiques,
avec budget de gaz et limites de tableaux ; les réponses ABI invalides sont
rejetées. La configuration fait partie de l'identité du dossier RocksDB.

Le registre initial vient de l'état genesis. Après chaque bloc accepté, les
stakes, statuts actifs et suppressions sont relus dans l'état exécuté et le
registre est enregistré atomiquement avec le bloc. La sélection du producteur
utilise le registre du parent. Un contrat déployé désactive le bootstrap même
si aucun validateur actif n'est retourné.

Écart explicite avec le Python : sa synchronisation ajoutait uniquement les
nouveaux validateurs. Nano reflète aussi les changements et retraits. Avec un
contrat configuré, celui-ci est la source des montants stakés ; les récompenses
liquides du moteur ne sont pas ajoutées artificiellement à ces montants. Les
compteurs de blocs validés sont conservés. Le mode sans contrat conserve sa
logique de registre existante.

Les tests utilisent un petit contrat de fixture, pas un contrat de staking de
production. Ils couvrent un changement par transaction, le rejeu indépendant,
la réouverture et le rejet d'une configuration modifiée. Le port des méthodes
RPC d'inscription/ajout/retrait direct reste à réaliser, ainsi que la validation
sur un contrat de staking réel fourni par le projet.

## Précompilation BN128 pairing

L'adresse 8 utilise mcl (commit cbb18eb08b86129cf936a6436b5e6c68a2ce8ddf),
compilé statiquement avec la courbe BN_SNARK1. Le moteur natif continue de gérer
la VM ; mcl fournit uniquement les opérations de pairing. Les coordonnées sont
contrôlées avant conversion : pas de réduction implicite modulo le corps.
Les points G1/G2 sont validés, et G2 doit appartenir au sous-groupe attendu,
y compris lorsqu'un autre point de la paire est à l'infini.

La vérification utilise les 14 vecteurs go-ethereum, avec sorties exactes,
frontières de gaz et cas invalides. Coût : 45000 + 34000 par paire. La source mcl,
sa licence et le commit sont inclus dans third_party/mcl. Les fixtures et leurs
licences sont séparées du logiciel livré. La précompilation 10 reste absente,
comme dans le registre Python source.

## Snapshots RPC de développement

`evm_snapshot` capture hauteur et hash canonical, `evm_revert` restaure cette
identité après vérification. Une WriteBatch supprime les blocs, états, reçus,
transactions, registres et index de hash postérieurs, puis restaure les métadonnées.
Le cœur publie son état restauré après réussite du commit. La mempool est vidée,
les filtres invalidés et les flux WebSocket reçoivent une erreur de réinitialisation.

La fonction est désactivée par défaut, exige dev_mode et une écoute locale,
et ne peut pas être activée avec le P2P supervisé. Contrairement au snapshot
Python en mémoire, les index persistés ne conservent pas la branche abandonnée.
Les tests couvrent restauration PoW/PoS, resoumission, branche alternative et
réouverture. Les identifiants de snapshots ne survivent pas au processus.

## Horloge de développement et production manuelle

`evm_increaseTime` et `evm_setNextBlockTimestamp` sont raccordés à `evm_mine`.
La production manuelle réutilise le moteur PoW ou le validateur PoS sélectionné,
puis consomme le timestamp programmé seulement après réussite du bloc.
Les timestamps non croissants et les débordements uint64 sont refusés.
Les snapshots capturent le décalage et l'échéance programmée.

Le Python expose evm_mine mais le refuse comme obsolète ; Nano le rend fonctionnel
uniquement dans le mode développement local explicite, sans P2P. L'horloge
simulée est réservée à cette commande, pas aux validateurs réseau.
Les tests couvrent le rejeu du même hash après restauration, le retour à
l'horloge initiale, et une production/restauration PoS via HTTP.

### Consultation du stake

La RPC `nno_getStake` utilise le registre PoS restauré ou le getter du contrat
configuré. L'appel statique est limité à 5 millions de gas et valide un résultat
ABI de 32 octets. Contrairement au Python qui masquait les erreurs du contrat
par zéro, Nano renvoie `-32000` si ce getter échoue ou renvoie une valeur mal
formée. Un contrat non déployé conserve le résultat zéro du Python.


### Démarrage et arrêt de la validation locale

`nno_startValidating` et `nno_stopValidating` pilotent une tâche périodique du
serveur HTTP, sans thread de validation séparé. Les appels sont idempotents,
mais refusent une coinbase sans portefeuille chargé ; le démarrage exige PoS
et un validateur actif. Contrairement au Python qui renvoyait vrai même quand
le démarrage était impossible, Nano renvoie alors une erreur RPC explicite.
Le test HTTP couvre l'attente de synchronisation, la production avec inclusion
d'une transaction, l'arrêt effectif et la persistance après réouverture.


### Import ChainDB et écarts historiques vérifiés

Le stockage `nanopy/storage` est en JSON. L'ancienne mention d'une migration
SQLite pour ces modules était incorrecte : les adaptateurs SQLite source sont
ceux du stockage de pairs dans `nanolib`, remplacé par cpp-libp2p.

`nano migrate` importe le format ChainDB dans un nouveau dossier RocksDB par
rejeu complet. Il résout les transactions référencées par hash, vérifie leurs
hashes et expéditeurs, les en-têtes, engagements d'exécution et la difficulté
cumulée. Les blocs et états ne deviennent visibles au chemin final qu'après
fermeture/réouverture et publication sans écrasement. Les sources sont relues
pour détecter leur modification durant le travail ; le nœud source doit rester
arrêté. Les caches d'état hors bloc ne sont pas une preuve canonique et ne sont
pas copiés. Les chaînes à registre PoS modifié hors bloc restent incompatibles.

Le générateur `scripts/generate-chaindb-fixture.py` capture aussi le Python
original : le genesis construit puis alloué conserve un hash périmé ; le moteur
PoW garde la base fee du parent et utilise keccak des octets vides pour le hash
des oncles vides. Les tests CLI vérifient leur rejet explicite. La fixture de
genesis importable reconstruit explicitement son en-tête pour rafraîchir ce
cache (voir sa provenance), sans modifier le dépôt Python. Ces observations
empêchent de déclarer la compatibilité historique complète.


### Bootstrap DNS

`network/bootstrap.hpp` porte la découverte via bootstrap DNSADDR du
`nanolib.discovery.bootstrap` source en utilisant le résolveur de cpp-libp2p.
Les tests isolent les réponses TXT pour vérifier récursion, cycles, limites,
erreurs et identités ; le test à deux nœuds utilise réellement un nom DNS4
local pour établir la connexion puis synchroniser et propager les données.
Le nœud Python n'active pas mDNS/UPnP dans son appel à `new_host`; ces options
facultatives de nanolib ne sont pas activées par ce portage. Une découverte
DHT ou mDNS supplémentaire n'est pas présentée comme implémentée.

### Contrôle des oncles lors du rejeu

La validation des oncles PoW est intégrée avant l'exécution des récompenses.
Elle contrôle la profondeur 1–6, le maximum de deux en-têtes, l'ascendance,
l'exclusion des ancêtres canoniques et des inclusions précédentes, ainsi que
l'en-tête PoW Nano et la base fee. La recherche couvre les sept derniers
ancêtres persistés. Les tests couvrent récompenses, rejeu sur un autre nœud,
réouverture, double inclusion et absence de mutation après rejet.

Le Python calculait les récompenses sans ces contrôles complets. Cette
restriction est donc un écart explicite pour les historiques Python contenant
des oncles invalides. Les contraintes d'ascendance et de double inclusion
suivent les règles décrites dans `VerifyUncles` de go-ethereum :
https://github.com/ethereum/go-ethereum/blob/master/consensus/ethash/consensus.go
Le hash PoW et les règles d'en-têtes demeurent ceux de Nano, pas Ethash.

### Interopérabilité du transport original

Le test `python_interop` charge le module `nanopy/network/libp2p_network.py`
et `nanolib` du dépôt de référence sans modifier leurs fichiers.
`tests/python_transport_peer.py` fournit un adaptateur de nœud vers un moteur
C++ via RPC et une extension JSON de test qui ajoute `genesis_hash` aux statuts.
Cette extension est nécessaire depuis que Nano exige l’identité du genesis.
Un scénario sans extension observe une tentative de handshake dans chaque sens,
puis vérifie pendant trois secondes l’absence d’admission et contrôle que la
chaîne native reste inchangée. Les contrôles unitaires `peer_status` vérifient
séparément le rejet déterministe des champs manquants ou incohérents. Dans les deux sens de connexion, les tests vérifient
handshake, synchronisation historique, transaction incluse, reçus, annonce C++
vers Python et récupération C++ d'un bloc servi par Python. Les blocs à comparer
proviennent donc des règles C++ ; l'incompatibilité des historiques source
signalée précédemment n'est pas résolue par ce test.

L'environnement de référence validé utilise Python 3.13 et les dépendances
figées dans `tests/interop-requirements.txt`. Python 3.14 a échoué lors de la
construction de coincurve 21.0.0 ; aucune modification du dépôt original ni
installation de nanolib dans le runtime C++ n'a été nécessaire. Le gossip de
transactions du réseau Python est explicitement non implémenté dans la source.


### Installation et chemin CMake

CMake installe désormais le moteur, le transport précompilé configuré, les
genesis et la documentation. Le nœud localise son propre exécutable avec
l'API macOS ou `/proc/self/exe` sous Linux avant de chercher `nano-p2p` à côté.
La validation réelle sur macOS couvre un préfixe déplacé, un chemin avec
espaces, l'invocation depuis un autre répertoire via PATH ou un lien
symbolique, la réponse RPC et l'arrêt sans processus P2P restant.

CMake 4.4.3 est installé par Homebrew sur cette machine. Le réglage VS Code
pointe vers `/opt/homebrew/bin/cmake`, au lieu du venv temporaire. Les scripts
de dépendances acceptent `CMAKE_BIN` et `NANO_DEPS_DIR`. Les bibliothèques
système du moteur restent requises à l'exécution.
