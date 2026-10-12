# Audit de l’activation EVM/RPC — 12 octobre 2026

Source examinée : `main` propre, `0842078e253e42424ef554d4c7662cd09d8305ea`. Les dossiers `src`, `include`, `third_party`, `network` ne diffèrent pas du cœur `85eb492f641452c4746ee60f027eacefa775b0b0` (diff Git vide). Revue statique de fichiers publics uniquement ; aucune suite relancée, aucun nœud démarré, aucune clé ou configuration privée consultée. Les chemins et lignes ci-dessous sont relatifs à `/Users/boris/Dev/blockchain-nano`.

## Ce qui est activable et configuré

| Couche | Constat vérifié dans le code/fichier | Référence |
| --- | --- | --- |
| Genesis réseau explicite | Seuls `Paris` et `Shanghai` sont acceptés par `evmFork` ; Cancun/Prague/Osaka sont refusés. | `src/chain.cpp:11` |
| Runtime Bellatrix | Exige `consensus=beacon` et le profil `Paris`. | `src/consensus_bellatrix_runtime.cpp:35` |
| Runtime Capella | Exige `consensus=beacon` et le profil `Shanghai`, puis un profil consensus authentifié par son SHA-256 et son chain ID. Le constructeur de propositions utilise effectivement Shanghai. | `src/consensus_capella_runtime.cpp:54`, `:55`, `:57` ; `src/consensus_capella_local_body.cpp:212` |
| Fichiers génériques mainnet/testnet | `genesis/mainnet.json` et `genesis/testnet.json` déclarent le PoS historique avec bootstrap, mais **aucun `evmFork`**. Par lecture du code, ils sélectionnent donc la valeur par défaut `NanoExperimental`, pas Capella/Shanghai. Leur nom ne prouve pas qu’ils sont le manifeste de lancement actuel. | `genesis/mainnet.json:2`, `genesis/testnet.json:2` ; `include/nano/chain.hpp:12` ; `src/chain.cpp:9` |
| Compose générique | Monte `genesis/testnet.json` et `deploy/docker/node.json`. Ce dernier ne configure aucun runtime Bellatrix/Capella ; ce Compose ne reproduit donc pas la campagne privée Capella. | `compose.yaml:23`, `:24`, `:27` ; `deploy/docker/node.json:1` |
| Fixture publique Capella | La fixture WAN épinglée de septembre contient explicitement `consensus=beacon`, `evmFork=Shanghai`. C’est une fixture historique, pas une observation du runtime du 12 octobre. | `docs/qualification/capella-wan-pinned-20260929/assets/genesis.json:1` |

L’état réellement utilisé par une future reprise doit rester lié aux fichiers genesis/profil engagés par ses capsules et ses journaux. Cet audit ne lit pas les fichiers privés de cette reprise et ne certifie aucun nœud vivant. Il faut un manifeste de lancement explicite ; substituer les fichiers génériques ci-dessus changerait le domaine réseau et le consensus.

## Implémentation distincte de l’activation

| Brique | Implémentation observée | Écart qui reste ouvert |
| --- | --- | --- |
| Cancun : 1153/5656/6780/7516/4844 | TLOAD/TSTORE/MCOPY, SELFDESTRUCT post-Cancun, BLOBHASH/BLOBBASEFEE, tarification/admission moteur des blobs et KZG sont présents. Les opcodes sont refusés avant Cancun. | Aucun genesis réseau Cancun ; admission de transactions type 3 refusée ; sidecars, disponibilité, rétention et récupération réseau ne sont pas qualifiés. |
| Prague : 7702/2537 | Autorisations/délégation du type 4 et sept précompiles BLS12-381 présents dans le moteur. Les adresses 11–17 ne sont reconnues par l’EVM qu’à partir de Prague. | Aucun genesis Prague ; mempool et simulations RPC refusent type 4. Le BLS de consensus déjà utilisé par Nano ne constitue pas l’activation EIP-2537. |
| Contrats systèmes : 4788/2935/7002/7251 | Appels systèmes à leurs adresses présentes dans le moteur, sous `EthereumValidated` et les révisions Cancun/Prague appropriées. | Aucun provisionnement code/nonce de ces comptes dans les deux genesis génériques ; activation cohérente, données consensus authentifiées et cycle des requêtes restent requis. Capella/Shanghai n’exécute pas ces appels modernes. |
| Osaka | Profil enum, contrôles et précompile P-256 présents dans le moteur de qualification. | Aucune configuration réseau Osaka acceptée ; aucun statut réseau ou production déduit. |

Références : `src/evm.cpp:59`, `:131`, `:132`, `:158`, `:159`, `:160`, `:236` ; `src/engine.cpp:34`, `:49`, `:66`, `:129`, `:130`, `:133`, `:138`, `:141` ; `include/nano/evm.hpp:10`, `:12` ; `src/precompiles.cpp:20` ; `src/consensus_bls.cpp:2`. L’admission réseau refuse uniformément `type > 2` : `src/chain.cpp:161`. La matrice conserve ses statuts partiels : `docs/EIP-IMPLEMENTATION-STATUS.md:1074` à `:1102`.

## RPC : acquis présents et lacunes réelles

`eth_call`/`eth_estimateGas` traitent les frais dynamiques : champs nuls normalisés, incompatibilité gasPrice/frais dynamiques refusée, priority fee bornée par max fee, budget de fonds appliqué à l’estimation, contexte pending capturé quand demandé. Le moteur de simulation reçoit le fork du genesis. Ce n’est donc pas un oubli général de `maxFeePerGas`/`maxPriorityFeePerGas` (`src/rpc.cpp:58`, `:467`, `:468`, `:469`, `:480`, `:491`, `:499`).

Le pourboire recommandé est calculé sur l’historique par une politique locale configurable ; l’ancienne recommandation constante de 1 gwei n’est pas la logique actuelle (`src/rpc_fees.cpp:6`, `:29`, `:37`, `:49` ; `src/rpc.cpp:200` ; `docs/RPC-FEE-POLICY.md:3`).

`eth_createAccessList` n’a aucune branche de dispatch dans `src/rpc.cpp` ; il aboutit au refus « Method not found » (`:461`). Les simulations refusent type 3/4 (`:73`, `:467`). Le tag `safe` et les lectures de blocs `pending` restent indisponibles dans le résolveur de blocs (`:110`), distinct des simulations pending déjà implémentées. Ces constats statiques ne constituent pas une nouvelle comparaison RPC avec Anvil/Geth.

## Limites de qualification et action suivante

Les matrices définissent explicitement implémenté/partiel/non commencé et séparent moteur, admission et activation (`docs/EIP-IMPLEMENTATION-STATUS.md:1038`, `:1044`, `:1050` ; `docs/EVM-PRODUCTION-EIP-ROADMAP.md:1076`). Les résultats historiques 44 244 états/52 870 cas de blocs sont une sélection épinglée, pas une certification de fork/réseau (`docs/EVM-CONFORMANCE.md:1107`, `:1111`). Le rafraîchissement plus récent documenté porte sur 5 387 états et 2 412 cas de blocs du corpus versionné ; la portée plus large reste à rafraîchir (`:1148`, `:1155`, `:1159`). Aucun de ces résultats n’a été rejoué ou promu par cette revue.

Priorité de livraison EVM : conserver le profil Capella/Shanghai effectivement engagé, définir le manifeste public de lancement avec les mêmes domaines et vérifier les binaires candidats exacts. Maintenir Cancun/Prague/Osaka et les services blobs dans leurs statuts d’intégration ouverts ; leur présence dans un runner ne permet pas de les annoncer activés. Compléter séparément la matrice RPC et les preuves par EIP. La reprise des signataires, la finalité commune, le cycle validateur, les ressources, le bridge/Base–Nano/Doppler et la revue indépendante demeurent des gates distincts.

`source-manifest.json` contient les empreintes publiques des fichiers examinés, la revision et le résultat du diff. Aucun statut de production ou EIP n’est modifié.
