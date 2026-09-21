# Audit de clôture du portage

Audit du 21 septembre 2026. Le portage intégral reste **non clôturé**. Une suite
verte établit le comportement des scénarios exécutés, pas une identité complète
avec tous les comportements du Python ni une aptitude à la production.

## Exigences explicites

| Exigence | Évidence actuelle | Conclusion |
|---|---|---|
| Projet C++ dans `/Users/boris/Dev/blockchain-nano` | Sources C++20, CMake, Make, exécutables `nano` et `nano-p2p` | Présent et compilé |
| Préserver le Python original | Les SHA256 des 353 fichiers de `source-inventory.json` correspondent au dépôt original ; Git est sans modification | Vérifié sur l'inventaire et Git |
| Nom Nano, énumération `NanoOpCodes`, plus de 150 opcodes | `include/nano/opcodes.hpp`, tests core ; 169 valeurs distinctes et deux alias | Déclarations présentes ; ne pas confondre avec l'exécution des propositions EOF |
| Ne pas utiliser evmone | EVM dans `src/evm.cpp`, précompilés natifs et mcl ; configuration de dépendances | Respecté |
| Remplacer nanolib par cpp-libp2p | Daemon C++ et protocoles dans `network/`, scripts de compilation ; suites network, bootstrap, python_interop, supervisor | Transport intégré ; interop Python avec extension de statut genesis dans la fixture |
| Remplacer SQLite | `src/storage.cpp`, RocksDB, écritures atomiques synchronisées ; tests chain et migration | Stockage actif RocksDB ; aucune conversion SQLite revendiquée |
| Corriger CMake dans VS Code | `.vscode/settings.json` : `/opt/homebrew/bin/cmake`, compilation du dépôt final | Vérifié sur cette machine |
| Staking par transactions signées et contrat | `NanoStaking.sol`, RPC, CLI et client validateur ; staking et staking_cli | Implémenté et testé ; remplace les mutations locales Python |
| Porter intégralement le comportement de la blockchain Python | Comparaisons et correspondances ci-dessous | Non prouvé : divergences historiques et audit comportemental restant |

## Couverture observée par domaine

| Source Python | Implémentation native / preuve | Limite de cette preuve |
|---|---|---|
| core/account, state, block, transaction | core, state, block, transaction, receipt ; fixtures python-core et tests core/chain | Les anciennes identités de blocs divergentes ne sont pas acceptées |
| crypto/keys, wallet | crypto, wallet, hdwallet, typed_data ; fixtures et tests core/wallet_cli | Alias nettoyés au retrait et collisions refusées, contrairement au Python |
| consensus/engine, pow | engine, consensus, chain ; core/chain/uncles/miner | Le mineur de bibliothèque n'active pas un mineur RPC automatique |
| consensus/pos | pos et staking ; fixtures de sélection et suites staking/validator | Les mutations locales RPC sont remplacées ; pas de signature de producteur dans les blocs |
| vm/evm, opcodes, gas_costs | evm et opcodes ; 451 cas Python, tests natifs d'appels/création | Conventions de gaz partielles ; pas de conformité à un hard fork Ethereum complet |
| vm/precompiles | precompiles et bn_pairing ; 111 cas Python, 14 vecteurs pairing et vecteurs BLAKE2F | Courbes invalides rejetées ; placeholder BLAKE2F corrigé ; adresse 10 absente comme dans le registre Python |
| storage/chaindb, statedb | State et ChainStore RocksDB ; état historique, réouverture, migration | Format disque différent ; snapshots complets par bloc ; historiques incompatibles refusés |
| network/http_rpc et rpc | rpc, http, websocket ; rpc/http/events, 33 réponses Python complètes | 61 noms de routes présents ne prouvent pas la parité de chaque paramètre et erreur |
| network/txpool | txpool ; core/chain/rpc | Pool en mémoire, pas de restauration persistante du pool |
| network/libp2p_network | network et superviseur ; network/python_interop/bootstrap/supervisor | Extension genesis dans les statuts Python de test ; moteurs C++ des deux côtés ; aucune preuve de rejeu complet de blocs Python |
| node/main, node | main, node, chain ; HTTP, superviseur, installation déplacée | Dépendances système requises ; pas de déploiement public effectué |
| cli/main | Dix commandes source mappées dans MIGRATION.md ; CLI C++ native | Syntaxes adaptées aux configurations et portefeuilles C++ |
| validator/* | client ; validator et staking_cli | Statistiques de session ; inscription signée optionnelle, pas une mutation instantanée |

## Décisions et travaux qui restent ouverts

1. **Cible production décidée.** L'utilisateur demande de corriger les défauts
   du Python. Les règles corrigées priment désormais sur la parité exacte des
   historiques divergents. Définir les versions de protocole et les modalités
   de transition sans réécriture silencieuse. Voir `PRODUCTION-GOAL.md`.
2. **Audit comportemental des API.** Achever la comparaison des cas de paramètres,
   erreurs et effets au-delà des noms de méthodes. L'inventaire de fichiers et
   la correspondance des routes servent à orienter cet audit, pas à le clore.
3. **Écarts EVM/consensus.** Vérifier et documenter les limites restantes en
   suivant les scénarios de la source. Ne pas effacer une divergence pour
   obtenir artificiellement un compte de tests identique.
4. **Lancement public.** L'authentification cryptographique des producteurs PoS
   et l'audit externe du contrat restent absents. Ce sont des conditions de
   préparation à la production, pas des fonctionnalités déjà présentes dans
   le Python que les tests de portage suffiraient à valider.

## Vérification reproductible

```sh
/opt/homebrew/bin/cmake --build build-cmake -j4
/opt/homebrew/bin/ctest --test-dir build-cmake --output-on-failure
```

Résultat du 21 septembre 2026 : les 19 suites passent en 51,31 secondes.
La configuration finale comporte 19 suites. Les tests réseau et interopérabilité
exigent les chemins de dépendances et environnements déjà configurés dans CMake.
Le cœur comporte 3 271 contrôles ; le staking natif en comporte 454. Consulter
`build-cmake/Testing/Temporary/LastTest.log` pour le dernier résultat local.
