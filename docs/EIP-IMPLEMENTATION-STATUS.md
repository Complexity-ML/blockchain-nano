# Feuille de route EIP — état réel de Nano

Dernière revue partielle : 26 septembre 2026 (intégration Bellatrix et reprise).
Source de vérité : code actuel, tests et rapports. Les anciennes notes chronologiques ne remplacent pas cette matrice.

- **Implémenté** : comportement présent dans le périmètre indiqué ; ne signifie
  pas que toute la blockchain est prête pour la production.
- **Partiel** : une couche ou le runner fonctionne, mais activation/intégration
  réseau ou consensus manque encore.
- **Non commencé** : pas d’implémentation spécifique intégrée identifiée dans
  cette revue. Une ligne de documentation ou une bibliothèque ne compte pas.
- **Terminé pour production** : réservé à une preuve de bout en bout sur le
  profil de lancement ; aucune simple liste EIP n’accorde ce statut.

Les campagnes officielles enregistrées couvrent 44 244 cas d’état et 52 870
cas de blocs dans leur périmètre sélectionné. Ces totaux ne constituent pas
une preuve individuelle pour chaque ligne. La traçabilité précise par EIP
reste à compléter. Le genesis réseau autorise actuellement Paris/Shanghai.

| EIP | État actuel | Périmètre réel | Code / preuves à consulter | Reste à faire / critère de clôture | Priorité |
| --- | --- | --- | --- | --- | --- |
| [3076](https://eips.ethereum.org/EIPS/eip-3076) | Partiel | Journal v5 et devoirs locaux Bellatrix à plusieurs clés ; redémarrage ordonné et reprise SIGKILL à témoins qualifiés sur 64 clés synthétiques | src/consensus_beacon_signer.cpp ; src/consensus_bellatrix_validator.cpp ; tests/test_bellatrix_transition.cpp ; docs/BEACON-SIGNING-JOURNAL.md | Gestion indépendante des témoins, migration des journaux existants, pannes de stockage et charge ; ne vaut pas qualification de tout le cycle validateur | Validateurs |
| [2](https://eips.ethereum.org/EIPS/eip-2) | Implémenté | Transactions et signatures | src/transaction.cpp ; tests/fixtures/ethereum-official.json | Conserver les régressions du profil activé et relier les cas exacts à chaque EIP | Socle |
| [155](https://eips.ethereum.org/EIPS/eip-155) | Implémenté | Transactions et signatures | src/transaction.cpp ; tests/fixtures/ethereum-official.json | Conserver les régressions du profil activé et relier les cas exacts à chaque EIP | Socle |
| [2718](https://eips.ethereum.org/EIPS/eip-2718) | Implémenté | Transactions et signatures | src/transaction.cpp ; tests/fixtures/ethereum-official.json | Conserver les régressions du profil activé et relier les cas exacts à chaque EIP | Socle |
| [2930](https://eips.ethereum.org/EIPS/eip-2930) | Implémenté | Transactions et signatures | src/transaction.cpp ; tests/fixtures/ethereum-official.json | Conserver les régressions du profil activé et relier les cas exacts à chaque EIP | Socle |
| [1559](https://eips.ethereum.org/EIPS/eip-1559) | Implémenté | Base fee et exécution | src/block.cpp ; src/engine.cpp ; src/evm.cpp | Recommandation RPC du pourboire encore fixe ; distinguer cet écart de la formule consensus | Socle |
| [3198](https://eips.ethereum.org/EIPS/eip-3198) | Implémenté | Base fee et exécution | src/block.cpp ; src/engine.cpp ; src/evm.cpp | Recommandation RPC du pourboire encore fixe ; distinguer cet écart de la formule consensus | Socle |
| [140](https://eips.ethereum.org/EIPS/eip-140) | Implémenté | Moteur EVM et règles d’état | src/evm.cpp ; src/engine.cpp ; tests/fixtures/ethereum-official.json | Maintenir la qualification et détailler les preuves par EIP ; pas une validation du réseau complet | Socle |
| [150](https://eips.ethereum.org/EIPS/eip-150) | Implémenté | Moteur EVM et règles d’état | src/evm.cpp ; src/engine.cpp ; tests/fixtures/ethereum-official.json | Maintenir la qualification et détailler les preuves par EIP ; pas une validation du réseau complet | Socle |
| [161](https://eips.ethereum.org/EIPS/eip-161) | Implémenté | Moteur EVM et règles d’état | src/evm.cpp ; src/engine.cpp ; tests/fixtures/ethereum-official.json | Maintenir la qualification et détailler les preuves par EIP ; pas une validation du réseau complet | Socle |
| [170](https://eips.ethereum.org/EIPS/eip-170) | Implémenté | Moteur EVM et règles d’état | src/evm.cpp ; src/engine.cpp ; tests/fixtures/ethereum-official.json | Maintenir la qualification et détailler les preuves par EIP ; pas une validation du réseau complet | Socle |
| [211](https://eips.ethereum.org/EIPS/eip-211) | Implémenté | Moteur EVM et règles d’état | src/evm.cpp ; src/engine.cpp ; tests/fixtures/ethereum-official.json | Maintenir la qualification et détailler les preuves par EIP ; pas une validation du réseau complet | Socle |
| [214](https://eips.ethereum.org/EIPS/eip-214) | Implémenté | Moteur EVM et règles d’état | src/evm.cpp ; src/engine.cpp ; tests/fixtures/ethereum-official.json | Maintenir la qualification et détailler les preuves par EIP ; pas une validation du réseau complet | Socle |
| [1014](https://eips.ethereum.org/EIPS/eip-1014) | Implémenté | Moteur EVM et règles d’état | src/evm.cpp ; src/engine.cpp ; tests/fixtures/ethereum-official.json | Maintenir la qualification et détailler les preuves par EIP ; pas une validation du réseau complet | Socle |
| [1344](https://eips.ethereum.org/EIPS/eip-1344) | Implémenté | Moteur EVM et règles d’état | src/evm.cpp ; src/engine.cpp ; tests/fixtures/ethereum-official.json | Maintenir la qualification et détailler les preuves par EIP ; pas une validation du réseau complet | Socle |
| [2200](https://eips.ethereum.org/EIPS/eip-2200) | Implémenté | Moteur EVM et règles d’état | src/evm.cpp ; src/engine.cpp ; tests/fixtures/ethereum-official.json | Maintenir la qualification et détailler les preuves par EIP ; pas une validation du réseau complet | Socle |
| [2929](https://eips.ethereum.org/EIPS/eip-2929) | Implémenté | Moteur EVM et règles d’état | src/evm.cpp ; src/engine.cpp ; tests/fixtures/ethereum-official.json | Maintenir la qualification et détailler les preuves par EIP ; pas une validation du réseau complet | Socle |
| [3529](https://eips.ethereum.org/EIPS/eip-3529) | Implémenté | Moteur EVM et règles d’état | src/evm.cpp ; src/engine.cpp ; tests/fixtures/ethereum-official.json | Maintenir la qualification et détailler les preuves par EIP ; pas une validation du réseau complet | Socle |
| [3855](https://eips.ethereum.org/EIPS/eip-3855) | Implémenté | Profil Shanghai sélectionnable au genesis | src/chain.cpp ; src/evm.cpp ; src/engine.cpp | Maintenir les tests d’activation et de limites | Socle |
| [3860](https://eips.ethereum.org/EIPS/eip-3860) | Implémenté | Profil Shanghai sélectionnable au genesis | src/chain.cpp ; src/evm.cpp ; src/engine.cpp | Maintenir les tests d’activation et de limites | Socle |
| [1153](https://eips.ethereum.org/EIPS/eip-1153) | Partiel pour la production | Moteur Cancun présent ; pas de genesis réseau Cancun | src/evm.cpp ; src/chain.cpp ; docs/EVM-CONFORMANCE.md | Activation réseau cohérente du fork, admission, régressions et reprise | Avant activation Cancun |
| [5656](https://eips.ethereum.org/EIPS/eip-5656) | Partiel pour la production | Moteur Cancun présent ; pas de genesis réseau Cancun | src/evm.cpp ; src/chain.cpp ; docs/EVM-CONFORMANCE.md | Activation réseau cohérente du fork, admission, régressions et reprise | Avant activation Cancun |
| [6780](https://eips.ethereum.org/EIPS/eip-6780) | Partiel pour la production | Moteur Cancun présent ; pas de genesis réseau Cancun | src/evm.cpp ; src/chain.cpp ; docs/EVM-CONFORMANCE.md | Activation réseau cohérente du fork, admission, régressions et reprise | Avant activation Cancun |
| [7702](https://eips.ethereum.org/EIPS/eip-7702) | Partiel pour la production | Autorisations et exécution Prague ; non activé sur réseau | src/authorization.cpp ; src/transaction.cpp ; src/engine.cpp | Mempool/P2P, admission type 4 et activation réseau Prague | Avant activation Prague |
| [4844](https://eips.ethereum.org/EIPS/eip-4844) | Partiel | Blobs/KZG dans le moteur et les runners | src/kzg.cpp ; src/engine.cpp ; tests/test_blobs.cpp | Admission réseau type 3, sidecars, disponibilité, rétention et récupération | Avant service blobs |
| [4895](https://eips.ethereum.org/EIPS/eip-4895) | Partiel | Traitement des retraits dans exécution | src/engine.cpp ; tests/test_withdrawals.cpp | Origine et autorisation des retraits par le nouveau consensus Nano | Consensus cible |
| [4788](https://eips.ethereum.org/EIPS/eip-4788) | Partiel | Appel système côté exécution | src/engine.cpp ; tests/test_beacon.cpp | Provisionnement versionné du contrat (code, nonce, adresse) au genesis si fork actif dès le départ, sinon à la transition ; fourniture authentifiée des racines par le nouveau consensus | Consensus cible |
| [6110](https://eips.ethereum.org/EIPS/eip-6110) | Partiel | Requêtes côté exécution ; cycle consensus non terminé | src/requests.cpp ; src/engine.cpp ; tests/test_requests.cpp | Registre BLS, dépôts, sorties, consolidations et application des requêtes dans le consensus | Consensus cible |
| [7685](https://eips.ethereum.org/EIPS/eip-7685) | Partiel | Requêtes côté exécution ; cycle consensus non terminé | src/requests.cpp ; src/engine.cpp ; tests/test_requests.cpp | Qualifier encodage, ordre par type, omission des requêtes vides et requests_hash ; rejeter les engagements altérés ; appliquer dépôts/sorties/consolidations au consensus et vérifier reprise/rejeu | Consensus cible |
| [7002](https://eips.ethereum.org/EIPS/eip-7002) | Partiel | Requêtes côté exécution ; cycle consensus non terminé | src/requests.cpp ; src/engine.cpp ; tests/test_requests.cpp | Registre BLS, dépôts, sorties, consolidations et application des requêtes dans le consensus | Consensus cible |
| [7251](https://eips.ethereum.org/EIPS/eip-7251) | Partiel | Requêtes côté exécution ; cycle consensus non terminé | src/requests.cpp ; src/engine.cpp ; tests/test_requests.cpp | Registre BLS, dépôts, sorties, consolidations et application des requêtes dans le consensus | Consensus cible |
| [7549](https://eips.ethereum.org/EIPS/eip-7549) | Non commencé côté consensus | Pas de traitement spécifique intégré identifié | Consensus actuel : src/consensus_round.cpp ; src/bft_reactor.cpp | Implémenter sur le socle epochs/attestations/domaines ; vecteurs officiels et tests réseau | Consensus cible |
| [7044](https://eips.ethereum.org/EIPS/eip-7044) | Partiel | Domaine Capella durable qualifié dans le traitement des sorties Deneb ; non activé réseau | include/nano/consensus_exit.hpp ; src/consensus_slashing.cpp ; tests/test_deneb_exit_official.cpp ; denebExitQualification | Raccorder aux blocs et au gossip ; activer un profil réseau cohérent et vérifier sa transition | Consensus cible |
| [7045](https://eips.ethereum.org/EIPS/eip-7045) | Non commencé côté consensus | Pas de traitement spécifique intégré identifié | Consensus actuel : src/consensus_round.cpp ; src/bft_reactor.cpp | Implémenter sur le socle epochs/attestations/domaines ; vecteurs officiels et tests réseau | Consensus cible |
| [7917](https://eips.ethereum.org/EIPS/eip-7917) | Non commencé côté consensus | Pas de traitement spécifique intégré identifié | Consensus actuel : src/consensus_round.cpp ; src/bft_reactor.cpp | Implémenter sur le socle epochs/attestations/domaines ; vecteurs officiels et tests réseau | Consensus cible |
| [8045](https://eips.ethereum.org/EIPS/eip-8045) | Non commencé | Propositions demandées ; aucune implémentation spécifique identifiée | docs/VALIDATOR-EIP-ROADMAP.md | Épingler version et dépendances, implémenter et tester dans un profil expérimental distinct | Extension demandée |
| [8068](https://eips.ethereum.org/EIPS/eip-8068) | Non commencé | Propositions demandées ; aucune implémentation spécifique identifiée | docs/VALIDATOR-EIP-ROADMAP.md | Épingler version et dépendances, implémenter et tester dans un profil expérimental distinct | Extension demandée |
| [8363](https://eips.ethereum.org/EIPS/eip-8363) | Non commencé | Propositions demandées ; aucune implémentation spécifique identifiée | docs/VALIDATOR-EIP-ROADMAP.md | Épingler version et dépendances, implémenter et tester dans un profil expérimental distinct | Extension demandée |
| [7805](https://eips.ethereum.org/EIPS/eip-7805) | Non commencé | Propositions demandées ; aucune implémentation spécifique identifiée | docs/VALIDATOR-EIP-ROADMAP.md | Épingler version et dépendances, implémenter et tester dans un profil expérimental distinct | Extension demandée |
| [7732](https://eips.ethereum.org/EIPS/eip-7732) | Non commencé | Propositions demandées ; aucune implémentation spécifique identifiée | docs/VALIDATOR-EIP-ROADMAP.md | Épingler version et dépendances, implémenter et tester dans un profil expérimental distinct | Extension demandée |
| [8141](https://eips.ethereum.org/EIPS/eip-8141) | Non commencé | Propositions demandées ; aucune implémentation spécifique identifiée | docs/VALIDATOR-EIP-ROADMAP.md | Épingler version et dépendances, implémenter et tester dans un profil expérimental distinct | Extension demandée |
| [7594](https://eips.ethereum.org/EIPS/eip-7594) | Non commencé | KZG existant ne constitue pas PeerDAS | src/kzg.cpp ; network/main.cpp | Custody, colonnes, preuves de cellules, échantillonnage et disponibilité réseau | Avant service PeerDAS |
| [7516](https://eips.ethereum.org/EIPS/eip-7516) | Partiel pour la production | BLOBBASEFEE implémenté dans le moteur ; lit la base fee des blobs du bloc | src/evm.cpp ; tests/test_blobs.cpp | Qualifier opcode 0x4a, coût 2 gas, valeur issue du bloc et rejet avant Cancun ; admission, RPC et rejeu multi-nœuds après activation | Avant activation Cancun |
| [2537](https://eips.ethereum.org/EIPS/eip-2537) | Partiel pour la production | Sept précompiles BLS12-381 implémentés | src/bls.cpp ; src/precompiles.cpp ; tests/test_bls.cpp | Qualifier les sept adresses, encodages, contrôles de sous-groupes applicables et gas/erreurs ; activation Prague et rejeu réseau ; distinct du BLS de consensus | Avant activation Prague |
| [2935](https://eips.ethereum.org/EIPS/eip-2935) | Partiel pour la production | Appel système d’historique implémenté dans le moteur | src/engine.cpp ; tests/test_requests.cpp | Provisionner code/nonce/adresse au genesis ou à la transition ; tester appel système avant transactions, fenêtre 8191, limites, reprise et rejeu réseau ; BLOCKHASH inchangé | Avant activation Prague |
| [7623](https://eips.ethereum.org/EIPS/eip-7623) | Partiel pour la production | Plancher de gas calldata dans le moteur | src/engine.cpp ; docs/EVM-CONFORMANCE.md | Activation Prague et admission/estimation cohérentes du plancher | Avant activation Prague |
| [7691](https://eips.ethereum.org/EIPS/eip-7691) | Partiel | Paramètres blobs Prague dans le moteur | src/engine.cpp | Intégration exécution/consensus et disponibilité réseau | Avant blobs Prague |
| [7840](https://eips.ethereum.org/EIPS/eip-7840) | À implémenter / qualifier | Configuration du calendrier des blobs non établie dans le genesis réseau | src/chain.cpp ; src/engine.cpp | Configuration versionnée et validation du calendrier ; statut Informational distinct du consensus | Configuration forks |
| [7642](https://eips.ethereum.org/EIPS/eip-7642) | Applicabilité à documenter | Nano utilise cpp-libp2p, pas le protocole Ethereum eth/69 | network/main.cpp | Nécessaire seulement si compatibilité filaire eth/69 revendiquée ; ne pas confondre avec EVM | Périmètre réseau |

## Hors numéros d’EIP : fondations nécessaires

| Composant | État | Reste à faire |
| --- | --- | --- |
| Admission au-delà de 1 000 | Implémentée et testée localement à 1 001 par transactions signées, RPC et réouverture | Lecture du staking et transport encore bornés ; ne pas promettre capacité réseau illimitée |
| Votes individuels à grande échelle | 100 000 et 1 million validés localement, certificats complets | Intégrer comités/agrégats et qualifier le réseau ; pas assimiler à autant de nœuds |
| Permutation des comités Ethereum | Primitives qualifiées localement jusqu’à 1 million ; affectations de slots intégrées aux devoirs Bellatrix et sélection groupée testée sur 64 clés | Qualifier le registre ouvert, les opérations et le réseau à grande échelle ; aucune équivalence avec 1 million de nœuds |
| BLS de consensus / agrégation | Partiel : signatures BLS des devoirs et blocs Bellatrix intégrées ; primitives et scénarios à clés synthétiques qualifiés | Cycle du registre/PoP, agrégats de réseau et changements de validateurs à qualifier de bout en bout |
| Domaines et racines SSZ de signature | Intégrés aux signatures Bellatrix locales avec identité/registre authentifiés ; tests de refus et de reprise existants | Qualifier les transitions de forks et l’activation publique ; les fixtures ne prouvent pas ces opérations réseau |
| Casper FFG / LMD-GHOST | Partiel : transitions et choix de branche Bellatrix implémentés ; finalité locale et scénario distribué ciblé enregistrés | Reprise après partition de 96 s réussie sur deux nœuds / un hôte ; partitions prolongées, autres répartitions du stake et exploitation distribuée à qualifier |
| Registre beacon / RANDAO / slashing | Partiel : opérations et snapshots locaux ; RANDAO intégré aux propositions Bellatrix, journal anti-slashing par clé | Cycle ouvert entrées/sorties/sanctions, disponibilité des opérations et reprise après panne à terminer et qualifier |
| ERC-20/721/1155 | Contrats et scénarios ciblés existants | Étendre les scénarios ; pas de revendication « tout OpenZeppelin testé » |
| ERC-1271/165/2612 et EIP-712 | Audit de couverture nécessaire | Répertorier contrats et cas exacts ; éviter d’assimiler compilation et comportement |

## Mise à jour obligatoire après chaque travail

Pour changer un état : indiquer le changement, le profil de fork, les tests
positifs/négatifs, le rapport de campagne et les limites restantes. Une EIP
présente dans un runner reste partielle tant que le réseau requis ne l’admet
pas. Les dépendances des forks complets doivent être conservées ; cette matrice
est une feuille de route priorisée, pas une sélection arbitraire de règles.

Sources et priorités : [validateurs](VALIDATOR-EIP-ROADMAP.md) et
[EVM/production](EVM-PRODUCTION-EIP-ROADMAP.md). Les propositions Draft/Review
restent expérimentales. Les paramètres économiques Nano doivent être documentés
séparément des valeurs Ethereum. Les gates de durabilité, réseau, exploitation
et sécurité restent applicables en plus de cette feuille de route.

Preuves locales : `ethereumCommitteePrimitiveQualification` et
`validatorOpenAdmissionQualification` dans `tests/fixtures/ethereum-official.json`,
avec vérificateurs et contrôles négatifs. Elles ne valident pas le nouveau
consensus complet ni un réseau d’un million de nœuds.

## Clarifications RPC et sérialisation — revue du 24 septembre 2026

- `maxFeePerGas` et `maxPriorityFeePerGas` sont déjà traités dans les
  simulations (`src/rpc.cpp`, `tests/test_rpc.cpp`). Le pourboire recommandé
  fixe est un écart distinct, pas une absence de ces champs.
- `eth_createAccessList` : aucune implémentation identifiée ; à ajouter et
  comparer à un client de référence, notamment pour les accès froids et les
  listes fournies en entrée.
- Les transactions de types 3 et 4 gardent leurs enveloppes typées **RLP**
  selon EIP-4844/EIP-7702. **SSZ** concerne les objets de consensus et les
  sidecars correspondants : ne pas convertir leurs transactions en SSZ.
- EIP-2935 ne remplace pas la sémantique de l’opcode BLOCKHASH.
- EIP-7685 figurait déjà comme partielle ; pas de nouvelle ligne en doublon.
- Le profil complet Prague se contrôle aussi contre la meta-EIP 7600 ; la
  liste de quelques briques n’est pas une preuve de conformité complète.

Preuve `consensusBlsVerifyQualification` : primitive individuelle BLS, distincte
des précompiles EIP-2537. Aucun certificat agrégé ni poids de vote autorisé
par ce seul résultat.

Les critères de clôture ci-dessus sont des travaux à vérifier, pas des tests
déclarés réussis par cette revue documentaire. Références normatives :
[EIP-7516](https://eips.ethereum.org/EIPS/eip-7516),
[EIP-2537](https://eips.ethereum.org/EIPS/eip-2537),
[EIP-2935](https://eips.ethereum.org/EIPS/eip-2935),
[EIP-7685](https://eips.ethereum.org/EIPS/eip-7685) et
[profil Pectra](https://eips.ethereum.org/EIPS/eip-7600).

Preuve `consensusSigningQualification` : racines SSZ et construction des domaines,
avec corpus officiel épinglé et contrôle négatif du vérificateur. Cette primitive
ne constitue pas encore une protection des votes du réseau par ces domaines.

Campagne `hundredThousandVotesQualification` : 100 000 votes individuels,
certificat complet, doublons et quatre entrées invalides ; 148,42 s au total,
332 742 656 octets de RSS maximal et 55 800 000 octets de messages cumulés.
Mesure locale unique, sans promesse de débit ni qualification multi-nœuds.

Campagne `millionVotesQualification` : 1 000 000 de votes individuels, certificat
complet, doublons, équivoque et changement de registre vérifiés. Total :
1 482,79 s ; RSS maximal : 3 170 107 392 octets ; messages : 558 000 000 octets.
Réception avec contrôle du quorum : 267,78 s ; doublons : 264,30 s.
Ces coûts ne conviennent pas à une participation individuelle de tout ce
registre à chaque bloc rapide : comités et agrégats restent à intégrer.
Aucun million de nœuds ni million d’inscriptions au staking n’a été testé.

Preuve `consensusBlsAggregateQualification` : FastAggregateVerify, 12 vecteurs
officiels et 33 mutations rejetées ; exige des preuves de possession vérifiées
par le registre. Ne valide pas les poids, les quorums ou les signataires autorisés.

Campagne `realNetworkScaleQualification` : 4 puis 16 vrais nœuds locaux
(cœur C++, RocksDB et relais libp2p distincts), 4 blocs certifiés et 1 transaction,
avec un signataire de fixture ; accord exact des hash, racines et reçus.
Le scénario séparé de 4 validateurs BFT actifs Shanghai passe également.
10 000 nœuds non exécutés : ce Mac dispose de 24 Gio et limite globale de
6 000 processus, contre au moins 20 000 nécessaires avec cœur et relais.
L’utilisateur a retenu une petite échelle réelle sur ce Mac ; aucune preuve
de réseau 10 000 nœuds, multi-hôte ou d’endurance n’en découle.

Preuve `consensusDepositQualification` : racines SSZ DepositMessage (5 vecteurs
Ethereum), 3 signatures synthétiques et 24 mutations rejetées. La signature
prouve la possession de la clé ; elle ne prouve ni le financement ni l’ordre
d’un dépôt. Admission au registre et cycle EIP-6110 restent partiels.

Preuve `stakingQueryRulesQualification` : budgets des lectures de staking
configurables dans le genesis signé, engagés dans son hash ; réouverture
avec règles différentes refusée. Trois suites passent, puis un cas renforcé
vérifie l’épuisement du budget global sans publication partielle du registre.
Les valeurs historiques sont conservées si la configuration est absente.
Ce changement ne supprime pas les coûts de lecture du registre entier ni
les copies d’état par appel : pagination/incréments restent ouverts.

Preuve `stakingPaginationQualification` : interface paginée du contrat et
synchronisation intégrée au moteur sous `stakingQueryRules.version=2`.
Équivalence exacte avec l’interface historique pour 5 tailles de page ;
6 pages invalides/doublons et 4 configurations invalides rejetées.
1 001 inscriptions signées, certificats BFT via RPC et réouverture auditée
passent sur Paris et Shanghai. Anciennes interfaces conservées par défaut.
La pagination est implémentée ; le parcours incrémental du registre, les
copies d’état et la qualification de grands réseaux restent ouverts.

Qualification locale complémentaire (24 septembre 2026) : les appels EVM
statiques lisent désormais l'état sans le copier ni créer de snapshots de comptes.
`readonlyEvmQualification` conserve 18 artefacts vérifiés pour 9 tests Release,
dont 20 cas statiques, annulation, rollback du parent et régressions staking/RPC.
Cela réduit le coût des getters paginés mais ne clôture ni EIP-214 sur le corpus
officiel complet après modification, ni le passage réseau à 100 000 validateurs.

Rafraîchissement EVM : `readonlyEvmCorpusQualification` vérifie 5 387/5 387 cas
sur les 317 fichiers de fixtures d'état actuellement versionnés, dont 505 rejets.
Identités et empreintes contrôlées ; zéro mismatch/unsupported/error. Ce périmètre
ne remplace pas les corpus historiques plus larges ni les scénarios de blocs et
réseau. Aucun changement de statut de production n'en découle.

Consensus : `ConsensusCommittee` relie désormais PoP, appartenance unique,
agrégats BLS et poids sans débordement. Trois vérifications positives et vingt
rejets adversariaux sont archivés dans `committeeParticipationQualification`.
L'état autorisant ces membres/poids/domaines, le traitement des époques et
l'intégration réseau restent requis ; aucun statut d'EIP complet n'est ajouté.

Attestations : la racine SSZ typée (slot, index, bloc, checkpoints source/cible)
est vérifiée contre cinq vecteurs officiels et reliée au test d'agrégat du comité.
`consensusAttestationQualification` conserve les preuves ; fenêtre d'inclusion,
règles de fork et finalité restent ouvertes, notamment pour 7045/7549.

Slashing : le prédicat de double vote / vote englobant d'attestations est testé
sur 24 cas locaux (`attestationConflictQualification`). Il ne sanctionne pas
encore les validateurs : validation complète des preuves et mutation persistante
du registre restent ouvertes.

Slashing authentifié : `verify_attester_slashing` vérifie maintenant les deux
signatures et l'intersection des signataires avec cohérence des clés historiques
(`authenticatedSlashingQualification` : 3 preuves positives, 10 rejets).
L'application des pénalités et son intégration persistante restent ouvertes.


Transition de sanctions : `slashingTransitionQualification` archive 26 scénarios
locaux de comptabilité et d'éligibilité Phase0, plus les régressions des preuves
BLS et comités. Les sorties différées, récompenses et rejets atomiques sont testés
en mémoire. Ce résultat ne clôture aucune EIP : persistance par bloc, cycle
d'époques et raccordement réseau restent ouverts. Voir la feuille de route
validateurs pour le profil exact et les limites.


Reprise du registre : `slashingRecoveryQualification` ajoute un snapshot local
strict et une réouverture RocksDB après sortie abrupte du processus écrivain.
11 contrôles et 19 rejets de snapshots passent ; le rejeu ne double pas la
sanction. Cette qualification du stockage n'active pas le registre dans le BFT
et ne prouve pas l'engagement du consensus dans les blocs ni la reprise après
coupure électrique. Les EIP gardent leur statut actuel.


Slashing par époque : `slashingEpochQualification` ajoute pénalité corrélée,
rotation du bucket et contrôle de séquence, avec 11 contrôles et 8 rejets.
Les quatre suites passent. Le format snapshot v2 inclut les paramètres ajoutés
et rejette v1. Le cycle complet des époques et le consensus réseau restent
ouverts ; aucun statut d'EIP n'est promu sur cette sous-transition.


Preuve officielle de slashing : `officialSlashingQualification` compare sans
écart 320 soldes (90 modifiés) dans les cinq fixtures Phase0 minimal du composant
`process_slashings`, épinglées à une révision Ethereum. Le calcul pur est testé
sur leurs états isolés ; la transition complète conserve ses contrôles de
cohérence. Cinq suites passent. Ce périmètre ne clôture ni le cycle complet des
époques ni une EIP de consensus ; l'intégration réseau reste ouverte.


RANDAO : `randaoQualification` vérifie les révélations BLS liées à l'époque,
au fork et au genesis, les rejets de slots rejoués, la rotation et les graines
retardées. 13 contrôles et 19 rejets locaux passent avec les régressions BLS/SSZ.
L'éligibilité du proposant, la persistance et le raccordement au consensus actif
restent requis ; aucune activation réseau n'est revendiquée.


RANDAO officiel : `officialRandaoQualification` valide 3 révélations BLS et
192 mixes à partir de trois scénarios de blocs Phase0 minimal épinglés, avec
15 rejets locaux. Les indices des proposants viennent des fixtures : sélection,
validation complète des blocs et intégration réseau ne sont pas qualifiées.
Trois suites passent et les contrôles négatifs sont archivés.


Proposants : `proposerQualification` recalcule les 3 indices officiels du corpus
RANDAO et teste 98 comparaisons pondérées, 10 rejets et l'annulation. Les accès
scalaires au shuffle concordent avec 240 permutations officielles. Les mesures
100 000/1 000 000 concernent des registres locaux à poids maximaux égaux, sans
réseau. Profil Phase0 seulement ; Electra/EIP-8045 et activation restent ouverts.


Jonction proposant/RANDAO : `selectedRandaoQualification` relie la sélection,
l'éligibilité et la clé du registre à la révélation BLS. Trois contrôles positifs,
12 rejets locaux et 6 rejets ajoutés aux 3 projections officielles passent dans
quatre suites. Le registre authentifié, le bloc complet, le commit durable et
le raccordement au réacteur restent ouverts ; aucun statut d'EIP n'est promu.

Signature des blocs : `beaconHeaderQualification` vérifie cinq racines SSZ
BeaconBlockHeader et trois signatures de blocs officiels, avec 25 mutations de
racines et 33 rejets de signatures/contextes. Cinq suites passent. La racine du
corps est fournie par la fixture officielle, sans calcul complet du corps ni
transition d'état ; intégration au réacteur et commit atomique restent ouverts.
Les statuts réseau/EIP ne changent pas.

Corps des blocs : `beaconBodyQualification` calcule le corps SSZ Phase0 complet
sur cinq fixtures officielles couvrant toutes ses opérations. Six suites
passent, ainsi qu'une campagne ciblée ASan/UBSan. Les trois signatures de blocs
utilisent maintenant ce corps recalculé, levant la dépendance à une racine de
fixture de la campagne précédente. La validité des opérations, la transition
complète, les schémas des forks suivants et l'intégration réseau restent ouverts.
Aucun statut d'EIP ni de production n'est promu par ce seul calcul SSZ.

Authentification commune : `blockAuthenticationQualification` relie corps SSZ,
signature, parent/slot attendus, proposant et RANDAO dans une préparation sans
mutation canonique. Trois blocs officiels, 54 rejets, 404 troncatures, 21
annulations, quatre contrôles locaux et sept erreurs de configuration sont
qualifiés dans six suites. Le candidat n'est pas un post-état validé ; opérations,
transition complète, commit durable et activation restent ouverts.

Sanction des proposants : `proposerSlashingQualification` couvre les quinze
scénarios officiels Phase0 (3 succès, 12 rejets, dont un registre amont incohérent).
La transition économique partagée avec les attestations reproduit les registres
et 192 soldes attendus. Rejeu, snapshot et débordement sont contrôlés ; sept
suites passent. Le traitement dans les blocs et l'activation réseau restent
ouverts ; ce profil Phase0 ne promeut aucun EIP Electra ni statut production.

Traitement des sanctions dans le bloc : `blockOperationsQualification` relie
l'authentification signée au RANDAO et aux proposer slashings ordonnés. Trois
blocs officiels reproduisent les projections attendues, deux blocs authentifiés
à preuves répétées sont rejetés sans mutation. Six suites passent, avec échec
tardif et annulation. Les autres opérations restent explicitement non traitées ;
état complet, commit et activation réseau ne sont pas qualifiés.

Cycle de sortie : `voluntaryExitQualification` couvre les dix cas officiels
Phase0, avec 320 enregistrements conformes, quatre rejeux et des rejets de
signature/domaine/débordement sans mutation. La file de sortie est mutualisée
avec les sanctions ; sept suites passent. Le raccordement aux blocs/réseau,
les paiements et les règles modernes EIP-7044/7002 restent ouverts.

EIP-7044 : `denebExitQualification` qualifie le domaine Capella figé et les
transitions de sortie sur seize cas officiels Deneb, avec 448 enregistrements
conformes et deux différences explicites par rapport à Phase0. Six suites
passent. Statut désormais partiel : bloc/gossip, activation et profil réseau
complet restent requis ; aucune activation Deneb n'est annoncée.

Admission des validateurs : `depositTransitionQualification` valide les dix-sept
scénarios officiels de dépôts Phase0, avec preuve Merkle ordonnée, PoP des nouvelles
clés, recharges et 966 enregistrements conformes. Les credentials/éligibilité
sont conservés dans le registre v3 ; le snapshot de dépôt conserve root/count/cursor.
Onze suites passent. La racine financée doit encore être fournie par le consensus,
l'activation et le traitement complet des blocs restent ouverts. Ce travail ne
vaut pas implémentation d'EIP-6110 ni ouverture effective du registre réseau.

Intégration du registre dans les blocs : `registryBlockOperationsQualification`
compose sanctions de proposants, dépôts et sorties Phase0 sur état privé. Huit
suites passent ; six scénarios officiels sont exécutés (trois valides/trois invalides),
deux séquences traversant une époque restent différées et non comptées comme succès.
Attestations non prises en charge signalées distinctement des blocs invalides.
Ce progrès ne clôt ni EIP-6110 ni EIP-7044 réseau : Eth1, époques, état complet et
commit/réacteur restent à intégrer. Les statuts EIP de la matrice restent inchangés.

Cycle de vie Phase0 : `registryEpochQualification` ajoute la programmation des
activations après finalité et des éjections, avec file partagée et churn paramétré.
Les quinze cas officiels de registry_updates sont couverts (14 conformes, 1 rejet
attendu), ainsi que deux registres synthétiques locaux de 100 000/1 000 000 entrées.
Neuf suites passent. Cette sous-transition ne clôt pas le traitement d'époque,
la finalité ou les EIP de staking modernes ; aucun statut réseau n'est promu.

Finalité de composant : `phase0FfgQualification` calcule justification/finalisation
Phase0 depuis comités, bitlists et soldes de l'état authentifié. Dix scénarios
officiels et sept suites conformes ; seuil exact de deux tiers testé au-delà de
uint64, doublons/sanctionnés/cibles incorrectes contrôlés. L'admission des
attestations, l'ordre d'époque complet, le choix de branche et le commit/réseau
restent ouverts ; aucun EIP réseau ni objectif de production n'est déclaré clos.

Admission Phase0 : `attestationInclusionQualification` ajoute la validation des
attestations (comité, fenêtre, source, BLS/fork/genesis), les métadonnées d'inclusion
et leur composition atomique dans les blocs signés avant dépôts/sorties. Les 41
cas officiels donnent 17 états conformes/24 rejets attendus ; sept suites passent.
Deux séquences officielles multi-époques supplémentaires sont différées. Les
sanctions d'attestateurs, opérations d'époque complètes et commit/réacteur réseau
restent ouverts ; EIP-7045/7549 et forks modernes ne sont pas promus par ce travail.

Sanctions d'attestateurs : `attesterSlashingQualification` authentifie les preuves
indexées et les intègre avant attestations/dépôts/sorties. Trente cas officiels
isolés, quatre blocs officiels et onze suites sont qualifiés ; un état synthétique
à chronologie incohérente concorde sur le composant mais reste rejeté par le chemin
contrôlé des blocs. Les cinq types d'opérations Phase0 sont composables ; Eth1,
époques, state_root, commit et réseau restent ouverts. Aucun EIP moderne n'est clos.

Soldes effectifs du socle Phase0 : `effectiveBalanceQualification` qualifie le
composant d'hystérésis sur l'unique scénario officiel (64 entrées, 8 changements)
et les frontières/annulations locales. Détail dans `VALIDATOR-EIP-ROADMAP.md`.
Le traitement complet des époques et son intégration réseau restent ouverts ;
cette preuve ne clôt ni EIP-7251 ni la production.

Fin d'époque du socle Phase0 : `epochMaintenanceQualification` couvre les resets
Eth1/sanctions/RANDAO, les HistoricalBatch SSZ et la rotation des attestations,
avec six scénarios officiels. Voir `VALIDATOR-EIP-ROADMAP.md` pour les frontières
et critères restants. Ces composants ne clôturent pas le traitement complet des
époques, EIP-6110 ou l'intégration réseau.

Récompenses Phase0 : `rewardsQualification` qualifie les 19 scénarios officiels
sur le calcul isolé après FFG (1 216 entrées comparées), y compris inactivité,
doublons et genesis. Les contrôles d'annulation et de débordement sont détaillés
dans `VALIDATOR-EIP-ROADMAP.md`. Ce socle ne clôt pas les récompenses modernes,
EIP-8068/8363 ou l'intégration du consensus au réseau.

Époque Phase0 composée : `orderedEpochQualification` trace l'ordre des dix étapes,
la cohérence des paramètres et le marqueur de retraitement. Six projections de
fixtures de slots concordent ; un scénario d'historiques reste différé. Cette
preuve ne couvre ni `process_slot`, ni la racine BeaconState, ni la persistance
ou le consensus réseau. Voir `VALIDATOR-EIP-ROADMAP.md` avant toute conclusion de
conformité complète ou d'activation d'un EIP.

Racine SSZ Phase0 : `beaconStateRootQualification` couvre les 123 vecteurs
BeaconState officiels du preset minimal et les 21 champs. Le hash complet
n'est pas encore raccordé au traitement des slots ou à la validation des blocs ;
les invariants d'admission du consensus restent distincts du schéma SSZ. Aucun
EIP/fork moderne ni état réseau n'est déclaré complet par cette preuve.

Slots Phase0 : `fullSlotsQualification` compare désormais les états SSZ complets
sur les sept scénarios officiels (100 slots), incluant `historical_accumulator`.
La nouvelle preuve dépasse les projections de `orderedEpochQualification` sans
les réécrire. Les blocs signés complets, la persistance/reprise et le consensus
réseau restent ouverts ; aucun fork post-Phase0 n'est activé par cette étape.

Blocs signés Phase0 : `fullBlocksQualification` couvre les 45 scénarios officiels
`sanity/blocks` minimal (29 états complets conformes, 16 rejets), en composant
slots/époques, votes Eth1, opérations et vérification du state_root. Les anciennes
séquences différées sont couvertes ici sans réécriture des campagnes historiques.
Le stockage durable, l'admission des états, le fork choice et l'intégration au
réseau restent ouverts. Il ne s'agit pas d'une activation des forks modernes.

Persistance Phase0 : `slotSnapshotQualification` trace une enveloppe SSZ/profil/
marqueur canonique et une reprise après `_exit` post-commit RocksDB. La poursuite
de la séquence reproduit l'état officiel, avec 25 enveloppes invalides rejetées.
Cette preuve locale ne clôt pas les fautes pendant écriture, la liaison EVM /
consensus, les snapshots réseau ou le commit certifié du nouveau consensus.

Fautes de persistance : `snapshotSyncQualification` couvre une erreur fsync et
trois dispositions de suffixe WAL (conservé/perdu/tronqué), avec reprise cohérente
SSZ/marqueur et continuation officielle. Portée locale macOS, stockage d'exécution
synthétique de support ; pas de preuve de coupure physique, de Linux ou de liaison
EVM/Beacon active. Les critères réseau/durabilité restants demeurent ouverts.

Choix de branche Phase0 : calcul de tête sur snapshot authentifié ajouté,
avec tests locaux de pondération/bonus/viabilité et cent arbres générés. Voir
`docs/VALIDATOR-EIP-ROADMAP.md`, section du 25 septembre sur le choix de tête.
Les handlers d'événements, leur authentification, la persistance du Store et
l'intégration réseau demeurent ouverts ; aucun EIP/fork réseau n'est activé
par cette primitive.

Admission fork choice : contrôle temporel/cible, comité dérivé, BLS et derniers
messages implémentés sur contexte cible authentifié fourni par l'appelant.
Tests locaux : 11 rejets, 70 annulations et effet sur la tête ; aucune activation
réseau ni qualification du Store complet. La dérivation SSZ du contexte, les
autres événements et la persistance restent à réaliser.

Équivoques du fork choice : double authentification BLS et intersection des
signataires implémentées ; les tests locaux vérifient l'exclusion du poids et
le changement de tête. La persistance de cet ensemble et le traitement réseau
restent ouverts. Aucune pénalité financière hors transition de bloc n'est ajoutée.

Contextes du fork choice : extraction depuis le post-state SSZ lié à l'en-tête
authentifié et avancement des slots sautés ajoutés. Qualification locale sur
29 séquences de blocs officielles (24 contextes avancés), neuf rejets et sept
annulations. Ce n'est ni une admission d'ancre non fiable ni une activation réseau.

Horloge du fork choice : expiration du bonus et promotion des checkpoints/du
registre ajoutées, avec mille comparaisons locales de rattrapage. Source d'horloge,
événements blocs, calcul des checkpoints non réalisés et persistance restent à
raccorder ; aucune activation réseau supplémentaire.

Store de fork choice en mémoire : admission de blocs reliée à la transition SSZ,
au bonus et aux checkpoints FFG. Sous-ensemble genesis : 15 séquences officielles
valides et 12 invalides, 16 blocs acceptés ; 18 fixtures intermédiaires explicitement
exclues de ce test de Store. Replay officiel fork_choice, durabilité, import
opérationnel d'ancre et réseau encore ouverts.

Adaptateurs Store : votes gossip, preuves et horloge utilisent désormais les
contextes SSZ des blocs conservés. Cinq suites passent. Écart de qualification
confirmé : les fixtures fork_choice du commit épinglé commencent avec Altair ;
leur replay fidèle nécessite les transitions/structures Altair, encore absentes
de ce pipeline Phase0. Ne pas assimiler les tests locaux au corpus officiel.

Prérequis Altair : schéma BeaconState à 24 champs et hash_tree_root implémentés,
123 vecteurs officiels minimaux concordants. Le support reste structurel :
transitions Altair et replay fork_choice non implémentés dans ce pipeline.
Aucun fork réseau activé par cet ajout.

Altair : opération isolée de mise à jour des scores d'inactivité implémentée,
21 post-states officiels concordants. Les fixtures synthétiques ne qualifient
pas l'admission d'état ; composition d'epoch et exécution réseau encore ouvertes.

Altair : récompenses/pénalités isolées ajoutées, 15 post-states officiels
concordants et saturation par catégorie vérifiée. Ne clôt ni l'époque complète,
ni l'admission des états, ni la qualification réseau.

Altair : justification/finalisation isolée ajoutée, dix post-states officiels
concordants et seuils pondérés testés. Les indicateurs de participation restent
soumis à authentification en amont ; ce résultat ne prouve pas la finalité réseau.

Altair : sélection/rotation des sync committees ajoutée, cinq post-states officiels
concordants. Répétitions de membres conservées ; budget local d'échantillonnage
explicite. Le traitement SyncAggregate des blocs et la composition d'epoch restent
ouverts, sans activation réseau.

Altair : rotation de participation isolée implémentée, douze post-states officiels
concordants. Opération non idempotente : application unique et persistance doivent
être assurées par la composition d'epoch, encore ouverte.

Altair : époque complète ordonnée et process_slots ajoutés, sept séquences
minimales officielles (100 slots) concordantes. Marqueur anti-rejeu et échec tardif
sans publication partielle vérifiés. Transition de blocs Altair, replay fork_choice,
durabilité et réseau encore ouverts.

Altair : opération SyncAggregate implémentée et qualifiée sur les 24 cas officiels
minimal (16 états exacts, huit signatures rejetées), avec tests de corruption,
autre genesis, débordement et annulation. Cette étape complète la vérification
isolée des sync aggregates, pas la transition complète des blocs ni le consensus
réseau. L'admission doit authentifier le comité, sa clé agrégée et les PoPs.
Voir la campagne `altair-sync-aggregate-20260925` et la feuille de route validateurs.

Altair : attestations et récompenses de proposant implémentées, 41 opérations
officielles minimal qualifiées (17 états exacts, 24 rejets). Les répétitions ne
paient pas deux fois les mêmes indicateurs de participation. Contrôles BLS,
fenêtre d'inclusion et comités sont dérivés de l'état admis. La transition complète
des blocs reste ouverte ; aucune activation réseau. Campagne
`altair-attestation-20260925`, détails dans la feuille de route validateurs.

Altair : dépôts hérités de Phase0 adaptés à l'état Altair, avec initialisation
à zéro des nouvelles listes de participation/inactivité. Les 19 opérations
minimal officielles passent (17 états exacts, deux rejets), ainsi que rejeu,
preuves modifiées, débordements et annulations. Cela ne constitue pas EIP-6110
ni l'authentification réseau de la source des dépôts. Campagne
`altair-deposit-20260925`, détails dans la feuille de route validateurs.

Altair : sanctions de proposants et d'attestateurs adaptées à l'état SSZ complet,
avec paramètres économiques Altair. Sur 45 fixtures officielles : 11 transitions
strictes exactes, 33 rejets et un calcul synthétique exact dont l'état incohérent
reste rejeté par l'admission stricte. Rejeu, mutations, débordements et annulations
sont testés. Aucun passage du consensus actif à Altair ; composition complète
et fork_choice restent ouverts. Campagne `altair-slashing-20260925`.

Altair : transition complète des blocs signés composée avec hash du corps SSZ,
slots/époques, signatures/RANDAO, votes Eth1 et toutes les opérations jusqu'au
SyncAggregate et à la racine finale. Les 52 séquences officielles minimal passent
(37 post-states exacts, 15 rejets ; 105 blocs acceptés avec racine vérifiée).
Cette composition ne vaut ni fork_choice Altair, ni admission/migration de fork,
ni persistance/raccordement au réseau actif. Campagne
`altair-full-blocks-20260925`, détails dans la feuille de route validateurs.

Fork choice Altair : Store événementiel en mémoire ajouté, dix scénarios officiels
get_head qualifiés (109 blocs, 104 ticks, 19 votes, une preuve séparée, 1081 champs
vérifiés). L'écart initial venait des attestations embarquées omises par le replay ;
la réception les traite après validation du bloc. Les attentes restent inchangées,
et l'oracle Ethereum officiel confirme zéro écart. Les profils de proposer boost
restent explicites. Autres groupes fork_choice, persistance et réseau encore
ouverts ; campagne `altair-fork-head-fixed-20260925` et feuille de route validateurs.

Fork choice Altair : extension aux 23 scénarios officiels on_block et huit reorg,
sans changement des attentes ni du moteur. Avec get_head, les trois suites passent
(41 scénarios, 10040 champs ; quatre blocs invalides attendus rejetés).
Campagne `altair-fork-block-reorg-20260925`. Le Store reste en mémoire ;
withholding/ex_ante/get_proposer_head, persistance/reprise et raccordement EVM/P2P
restent ouverts. Aucun EIP supplémentaire n'est déclaré intégralement activé.

Fork choice Altair : les cinq scénarios minimal withholding/ex_ante passent
(77 événements blocs, 870 champs), avec confirmation indépendante par l'oracle
Ethereum officiel, BLS actif, zéro écart. Campagne
`altair-fork-withholding-ex-ante-20260925`. Les deux scénarios ex_ante mainnet
ne sont pas couverts ; get_proposer_head, stockage durable et intégration réseau
restent ouverts. Aucun changement du profil réseau actif ou du statut des EIP.

Altair : optimisation optionnelle get_proposer_head implémentée avec règles
explicites et poids authentifiés. Deux scénarios officiels (312 champs) confirmés
par l'oracle Ethereum ; dix gardes locales. Les six groupes fork_choice minimal
passent ensemble (48 scénarios, 11222 champs). Campagne
`altair-fork-proposer-20260925`. Cela ne clôt pas les tests mainnet, l'admission
de snapshots, la persistance/reprise ni le raccordement EVM/P2P ; aucun nouveau
profil de consensus n'est activé.

Altair : enveloppe locale versionnée de sauvegarde d'état SSZ et marqueur d'époque,
liée au profil complet et à une racine authentifiée extérieure. Reprise après
_exit suivant un commit RocksDB, continuation identique au post-state officiel,
12 contrôles et 105 rejets de snapshots ; régressions Phase0/blocs Altair vertes.
Campagne `altair-snapshot-recovery-20260925`. Ne sauvegarde pas encore le Store
fork_choice complet ; coupure électrique, liaison EVM/consensus et réseau restent
non qualifiés. Aucun statut d'EIP ou profil réseau activé n'est promu.

Altair : sauvegarde canonique du Store complet et reconstruction du registre
justifié, avec digest externe et budgets explicites. Les 18 scénarios get_head/reorg
passent après 35 commits RocksDB, arrêts _exit et réouvertures ; tête, poids et
continuation conservés. Un écart de registre lors d'un checkpoint avançant d'époque
sans changer de racine a été corrigé, échec conservé. Huit suites vertes ; campagne
`altair-store-snapshot-20260925`. Interruptions pendant commit, anti-rollback,
ancrage réseau, élagage et intégration EVM/P2P restent ouverts. Pas d'activation.

Persistance du Store : commit ChainStore dédié, séquence + digest, remplacement
atomique synchronisé et rejet des écrivains périmés. Erreur d'écriture : handle
bloqué jusqu'à réouverture. Tests de concurrence, six SIGKILL, restauration d'une
ancienne base détectée avec engagement extérieur, EIO de synchronisation macOS
avec suffixe WAL conservé/perdu et poursuite après réouverture. Les 35 reprises
Altair utilisent désormais cette API sans blocs d'exécution artificiels.
Campagne `fork-checkpoint-commit-20260925` ; intégration au réacteur/EVM, Linux,
ancrage opérationnel et perte électrique restent non qualifiés.

Altair : coordinateur événementiel durable ajouté. Tick/bloc/vote/slashing sont
préparés puis committés avant publication ; conflit/erreur disque bloque l'instance,
annulation/refus laisse l'état inchangé. Get_head/reorg : 787 commits (18 init),
17 reprises, 3721 champs officiels conformes ; gardes d'annulation/publication et
fsync EIO macOS vérifiées. Campagne `altair-durable-reactor-20260925`.
Snapshots complets par événement : débit non qualifié. Aucun raccordement P2P,
activation réseau ou lien atomique EVM n'est déduit de cette intégration locale.

Altair : décodeurs SSZ bornés de votes et preuves raccordés aux entrées du
coordinateur durable. 71 encodages officiels pré-Electra, 768 entrées mal formées
ou hors limites rejetées ; cinq suites vertes, reprises événementielles conservées.
Campagne `consensus-message-ssz-20260925`. La validation de signature/consensus
reste distincte du codec. Transport P2P, cadrage/décompression/quotas et schéma
Electra EIP-7549 restent à intégrer ; aucune activation réseau.

Transport Altair : adaptateur d'enveloppe Nano JSON/hex préparé, identité/profil
et tailles vérifiés, clés dupliquées rejetées, budget partagé InboundBudget et
framing JsonFrame réutilisés. Test local : deux blocs officiels committés avant
acceptation, 14 rejets de messages, 24 refus par quota ; trois suites vertes.
Campagne `altair-wire-admission-20260925`. Pas encore de flux libp2p réel ni de
handler enregistré ; négociation/runtime/files et essais distribués restent ouverts.

Altair : adaptateur asynchrone de streams ajouté (lecture fragmentée bornée,
PeerId du transport, réservation maintenue jusqu'à fin réelle du worker, annulation
et réponse unique). Dix cas contrôlés ; neuf suites réseau et deux régressions
libp2p/BFT locales vertes. Campagne `altair-async-stream-20260925`.
Ces connexions réelles concernent le transport existant : Altair attend encore
handler/négociation, runtime et file worker bornée. Aucun nouveau profil actif.

Complément transport Altair (25 septembre 2026) : file worker séquentielle réelle
avec budgets cumulés de messages/payloads, annulation coopérative et drainage.
Dix suites réseau passent ; douze cas de stream incluent le vrai worker derrière
un stream contrôlé. La file n'est pas encore instanciée dans le daemon ni reliée
au coordinateur durable. Cela ne change aucun statut d'activation EIP/consensus.
Voir `docs/VALIDATOR-EIP-ROADMAP.md`, section « File worker Altair bornée ».

Complément service Altair (25 septembre 2026) : la file worker est raccordée au
coordinateur durable pour blocs/votes/slashings, avec annulation aux checkpoints
et décision après commit. Les 18 scénarios officiels minimal head/reorg passent
par ce chemin (3 721 checks, 17 reprises). Ce service cœur n'est pas encore exposé
au daemon séparé ; aucun réseau Altair ni nouveau profil EIP n'est activé.
Détails et limites dans « Service worker–consensus durable » de la feuille
validateurs ; preuves `build-cmake/qualification/altair-durable-worker-20260925/`.

Complément IPC Altair (25 septembre 2026) : entrée `nano_altairReceive` désactivée
par défaut, authentifiée par le jeton réseau et limitée à une écoute loopback
lorsqu'un service est injecté. Le trajet HTTP–worker–RocksDB est testé sur deux
blocs officiels ; RPC/HTTP existants passent. L'instanciation dans le runtime,
l'appel du daemon et la récupération des acquittements perdus restent ouverts.
Aucune activation EIP/consensus ; détails dans la section « Entrée RPC privée
Altair » de `docs/VALIDATOR-EIP-ROADMAP.md`.

Complément configuration Altair (25 septembre 2026) : le cœur peut recharger
explicitement le profil complet déjà sérialisé dans les snapshots (91 quantités,
sans défaut implicite), avec contrôle de taille/digest avant parsing. Deux blocs
officiels et la reprise des snapshots passent, avec 502 profils invalides refusés.
La composition au démarrage avec une ancre authentifiée reste ouverte ; ce
chargeur n'active aucun fork ni EIP. Voir « Chargement explicite du profil Altair »
dans `docs/VALIDATOR-EIP-ROADMAP.md`.

Complément bootstrap Altair (25 septembre 2026) : import local borné des fichiers
état/bloc avec engagements externes de racines et domaine de fork. Deux ancres
officielles (dont une au slot 17), onze refus et quatre annulations passent avant
publication durable explicite. Ce chargeur ne fournit pas la politique de confiance,
de fraîcheur ou de finalité ; son intégration à `run_node` reste ouverte. Aucun
fork activé. Détails dans « Import local d'une ancre vérifiée » de la feuille
validateurs.

Complément runtime Altair (25 septembre 2026) : `nano run` peut maintenant
composer explicitement profil/ancre/Store/service RPC, avec initialisation et
reprise strictement séparées. Le démarrage géré est testé avec les vrais processus
Nano et nano-p2p, mais le protocole Altair n'est pas encore appelé par le daemon,
son horloge n'est pas pilotée et ses décisions ne sont pas liées à l'EVM. Aucun
statut EIP de production n'est clos par cette intégration. Voir
`docs/ALTAIR-RUNTIME.md` et la section « Composition dans nano run » de la feuille
validateurs.

Complément horloge Altair (25 septembre 2026) : runtime à heure locale configurable,
ticks périodiques et avant traitement, déduplication des instants, blocage sur
recul/saut excessif. Statut courant accessible par RPC privé authentifié. Cinq
suites passent, dont huit gardes temporelles déterministes. Les écritures restent
des snapshots complets ; ni la tenue en charge ni la synchronisation d'horloges
multi-hôtes ne sont qualifiées. Aucun protocole P2P Altair ni liaison EVM activés.
Voir `docs/ALTAIR-RUNTIME.md` et la feuille validateurs.

Complément IPC Altair (25 septembre 2026) : tickets bornés par session, retry sans
redispatch, consultation/annulation/retrait authentifiés et protection contre un
Submit retardé après annulation. La perte d'une réponse HTTP après admission est
testée avec un seul commit du bloc officiel. Les tickets ne persistent pas après
redémarrage ; leur rapprochement avec l'état durable et leur usage par le daemon
restent ouverts. Aucun changement d'activation EIP/réseau. Voir la section tickets
de `docs/ALTAIR-RUNTIME.md`.

Complément client IPC Altair (25 septembre 2026) : dispatcher asynchrone borné
Submit/Job/Cancel/Forget, avec récupération de réponses perdues et conservation
de l'admission quand le résultat est incertain. Onze suites réseau et deux tests
HTTP d'intégration passent. `Core::call` est exercé avec un commit unique des blocs
officiels ; l'instanciation dans le daemon et la négociation P2P restent ouvertes.
Aucun fork ni statut production activé ; détails dans `docs/ALTAIR-RUNTIME.md`.

Complément identité Altair (25 septembre 2026) : le statut RPC privé expose une
empreinte du profil complet, du choix de branche, de l'ancre et de la chaîne,
ainsi que les limites wire. Treize scénarios d'identité et quatre suites ciblées
passent ; la négociation par le daemon reste à intégrer. Aucun état EIP ni fork
activé ne change. Voir `docs/ALTAIR-RUNTIME.md` et la feuille validateurs.

Complément reprise IPC Altair (25 septembre 2026) : rotation atomique des sessions
à tickets pour refuser les anciennes admissions retardées, sans effacer les
workers encore actifs ni réécrire les décisions commitées. Cinq suites ciblées
passent, dont 64 courses admission/rotation. L'orchestration du daemon reste à
raccorder ; aucun statut EIP, fork ou production n'est promu par ce test.
Voir `docs/ALTAIR-RUNTIME.md` pour le périmètre et les limites de récupération.

Complément intégration daemon Altair (25 septembre 2026) : récupération IPC
réellement appelée par `nano-p2p` avant construction de son client à tickets,
avec contrôle du profil, rotation confirmée et attente d'un inventaire vide.
Douze suites réseau et cinq suites cœur/intégration passent, dont seize scénarios
de récupération contrôlée et le démarrage/reprise des processus réels. Le handler
Altair P2P, la propagation et la liaison atomique EVM restent ouverts. Aucun fork
ni statut EIP de conformité n'est promu. Voir `docs/ALTAIR-RUNTIME.md`.

Complément admission réseau Altair (25 septembre 2026) : handler libp2p optionnel
avec handshake de profil complet, passage par les tickets et conservation de la
capacité jusqu'au résultat durable. Treize suites réseau et six suites cœur passent ;
le trajet réel reçoit deux blocs officiels et le payload maximal de 8 Mio. L'IPC
compact corrige le dépassement dû à l'hex sans réduire le profil ni élargir HTTP.
L'horloge de qualification est injectée ; ni propagation automatique, ni finalité
distribuée, ni liaison EVM ne sont closes. Aucun statut EIP/fork de production
n'est promu. Voir `docs/ALTAIR-RUNTIME.md` et la feuille validateurs.

Complément persistance Altair (25 septembre 2026) : blocs signés conservés dans
le même checkpoint que l'état, lecture RPC privée paginée, vérification des octets
et signatures à la reprise. Huit suites passent, incluant parcours officiels
head/reorg, corruption d'archives, erreur WAL injectée, annulation avant commit et
lecture des anciens checkpoints sans invention de signatures. Aucune conformité
EIP/fork n'est promue : diffusion automatique, pruning, stockage incrémental et
liaison consensus–EVM restent ouverts. Voir `docs/ALTAIR-RUNTIME.md`.


### Propagation Altair automatique qualifiée localement — 25 septembre 2026

Le relais de blocs utilise l'archive signée durable et des curseurs par pair,
avancés uniquement après acquittement positif. Catalogue stable sur tick/ajout,
réinitialisation lors d'un changement de session ou d'époque du catalogue.
La campagne `build-cmake/qualification/altair-block-propagation-20260925/`
passe 14 tests réseau et 3 tests cœur, dont deux vrais daemons/deux cœurs HTTP,
perte d'acquittement et redémarrage du daemon source avec deux blocs officiels.
Voir `docs/ALTAIR-RUNTIME.md` pour le périmètre exact et les limites.
Aucun statut EIP ou profil réseau n'est promu : finalité distribuée, tâches et
signatures de validateurs, gossip des votes, réconciliation après rejet permanent
et liaison atomique EVM/consensus restent à terminer.


### Rattrapage après panne du cœur Altair — 25 septembre 2026

`consensus_altair_restart_libp2p` passe avec un vrai processus cœur récepteur,
deux daemons libp2p, deux `SIGKILL` suivis de réouvertures de RocksDB et un
émetteur laissé actif. Le récepteur retrouve son engagement séquence/digest,
change de session de gossip, récupère le bloc officiel manqué et conserve les
SSZ ainsi que la tête attendue après la seconde réouverture. Preuve locale :
`build-cmake/qualification/altair-core-crash-catchup-20260925/verified.log`.
Le test utilise une horloge de fixture et un engagement externe connu ; il
n'interrompt pas un commit en cours et ne simule pas de coupure électrique.
Aucun EIP, profil réseau ou statut de production n'est promu par cet essai.


### Attestations signées durables — 25 septembre 2026

Le réacteur archive les SSZ canoniques des attestations qui modifient les
derniers votes, atomiquement avec l'état de consensus (`nano-altair-durable-v3`).
Les doublons n'amplifient pas l'archive ; la fenêtre courant/précédent borne sa
rétention, avec limites de ressources explicites et restauration revalidant
les signatures. Les formats v1/v2 restent acceptés sans backfill implicite.
Qualification : `build-cmake/qualification/altair-signed-votes-20260925/` ; voir
`docs/ALTAIR-RUNTIME.md` pour les tests et limites. Le raccordement de ces votes
à la diffusion P2P reste ouvert, ainsi que les devoirs/signatures des validateurs,
la finalité distribuée et la liaison atomique EVM/consensus. Aucun statut EIP
ni profil réseau n'est promu.


### Bridge : compromission de quorum (scénario Ronin)

La clôture de production inclut désormais explicitement la matrice
`docs/BRIDGE-SECURITY-REQUIREMENTS.md` : quorum et clés administratives compromis,
révocation, preuve de finalité, rejouabilité et réconciliation des réserves.
Tous ces essais de bridge restent non commencés. Le consensus et les EIP ne
constituent pas une preuve de sécurité des retraits inter-chaînes.


### Diffusion P2P des attestations qualifiée localement — 25 septembre 2026

L'archive signée des votes est exposée par deux lectures privées authentifiées
et diffusée avec des curseurs distincts de ceux des blocs. Alternance par pair,
ACK obligatoire, reprise de catalogue après expiration et contrôle de profil.
La campagne `build-cmake/qualification/altair-vote-propagation-20260925/` passe
14 tests réseau et 3 tests P2P. Le scénario officiel à branche courte plus lourde
vérifie qu'une attestation reçue via deux vrais daemons change la tête distante ;
la perte d'ACK et l'expiration sont également vérifiées.
Pas de génération autonome de votes, de finalité multi-epoch distribuée ni de
preuve de débit à grande échelle. Les tâches/signatures des validateurs,
slashings P2P, réconciliation et liaison atomique EVM restent ouverts. Aucun
profil réseau ni statut EIP n'est promu par ces tests.


### Protection des signatures Beacon — 25 septembre 2026

`BeaconSigningJournal` réserve durablement avant le callback de signature BLS.
Propositions monotones, sources non décroissantes et cibles croissantes protègent
contre les doubles signatures/votes qui s'entourent, avec répétition exacte du
dernier message autorisée. Base dédiée par clé, engagement externe à la reprise,
refus d'une instance périmée et vérification de la signature retournée.
La campagne `build-cmake/qualification/beacon-signing-journal-20260925/` passe le
journal (18 refus avant signature, concurrence, faute de synchronisation et crash)
et les deux suites cryptographiques officielles. Aucun devoir de validateur
n'est activé. L'import/export EIP-3076 est ajouté à la matrice comme non commencé.


### EIP-3076 : export et arrêt durable (25 septembre 2026)

`retire_and_export_interchange()` produit le format v5 après synchronisation du
retrait permanent du journal source. La campagne
`build-cmake/qualification/beacon-signing-export-20260925/journal-tests.log`
valide les réservations exportées, l'idempotence, les refus après réouverture
et l'échec de synchronisation injecté. Le journal actif v1 reste lisible.
L'import/fusion et les vecteurs officiels inter-clients restent à qualifier.
L'arrêt de tous les utilisateurs externes de la clé est une précondition ; aucune
protection contre une clé volée ni activation réseau n'est revendiquée.


### EIP-3076 : import et corpus inter-clients (25 septembre 2026)

La campagne `build-cmake/qualification/beacon-signing-import-20260925/final-tests.log`
passe les deux suites locales. L'import par clé fusionne les maxima sans les
réduire, conserve les racines inconnues, refuse les documents invalides sans
mutation partielle et ne réactive pas une base retirée. Les snapshots v1/v2 sont
conservés ; v3 représente les protections sans racine connue.

Le corpus épinglé `bb7900688eecd287d8723154faac6addb5e63336` contient 38 cas :
65 imports par clé, 146 tentatives de réservation, 211 rechargements. Quatre cas
avec données invalides sont rejetés à l'import selon l'option upstream ; deux
répétitions exactes sont acceptées au-delà de la stratégie minimale stricte, sans
mutation. Le runner contrôle les racines officielles via le vrai journal, sans
signer. Voir `docs/BEACON-SIGNING-JOURNAL.md` pour l'oracle et la reproduction.
Le chemin du corpus externe est requis explicitement ; son absence n'est pas
une qualification. Le parcours multi-clés et les tâches de validateur restent
ouverts, tout comme les critères globaux de production.

Le verrou de signature est désormais partagé par les journaux associés au même
`ChainStore`, y compris pendant les callbacks. Le test natif bloque un backend,
demande un export depuis un second objet et vérifie l'attente puis les refus des
deux objets. Cela ferme la course lecture/signature entre instances locales ;
les copies indépendantes de clé/base restent hors de cette protection.


### EIP-3076 : parcours CLI hors ligne (25 septembre 2026)

`nano beacon-journal init/status/import/export` exige une identité explicite et,
pour toute base existante, un engagement externe exact. Les résultats sont écrits
dans un dossier neuf hors de la base, avec permissions privées et synchronisation.
L'import/export ne charge aucune clé privée et n'active aucune tâche de validateur.

La campagne `build-cmake/qualification/beacon-journal-cli-20260925/tests.log`
couvre le parcours avec le vrai binaire, la migration entre deux bases, les refus
d'écrasement, de base manquante/verrouillée, d'identité/engagement incorrects et
les protections après retrait/import. Les deux pannes d'artefact injectées sur
macOS vérifient l'échec explicite sans reprise de signature. Le journal et le
corpus officiel font partie des régressions. Voir `docs/BEACON-JOURNAL-CLI.md`.

La base et les reçus externes ne sont pas une transaction atomique commune.
Une panne entre leur écriture peut demander une récupération explicite ; aucun
contournement automatique du contrôle d'engagement n'est ajouté. La coordination
multi-clés, cette récupération et les tâches réelles restent ouvertes.


### Tâches locales d'attestation Altair (25 septembre 2026)

`prepare_altair_attestation_duty` dérive le vote depuis le choix de branche,
l'état avancé, le registre et le RANDAO authentifiés. Le coordinateur durable
relie cette préparation au journal anti-slashing avec identité complète,
engagement courant et horloge locale contrôlés. Il n'accepte aucun champ de vote
fourni par le demandeur et n'active pas de boucle de validation dans `nano run`.

La campagne `build-cmake/qualification/altair-attester-duty-20260925/tests.log`
passe quatre suites ciblées : publication du coordinateur, journal, CLI et
nouvelle tâche. Le scénario composé sur ancre officielle signe une attestation
BLS ensuite admise et archivée par le chemin normal ; il vérifie 14 refus avant
fournisseur, deux variantes d'inéligibilité, une expiration pendant signature
et le refus d'un coordinateur périmé. La proposition de blocs, les autres tâches,
la planification/diffusion, la gestion des clés et l'exécution atomique restent
ouvertes. Périmètre et sources : `docs/ALTAIR-ATTESTER-DUTIES.md`.

La préparation sur un bloc déjà au créneau cible a été corrigée après un test
positif supplémentaire (`current-slot-failure.log` puis `duty-final.log`). Le
refus d'une seconde tête est maintenant vérifié comme refus anti-slashing exact.
`p2p-regression.log` confirme aussi la propagation existante de votes officiels
entre deux démons réels ; cette preuve ne vaut pas planification/diffusion
automatique de la nouvelle tâche locale.


### 2026-09-25 — borne temporelle de sortie des attestations locales

Le contrôle de tâche Altair refuse maintenant un recul d'horloge entre lectures
pendant la même tentative, y compris dans le même slot, et revérifie délai et
annulation après la vérification BLS. Les échecs après réservation ne rendent
pas la signature et ne suppriment pas la protection anti-slashing. Quatre suites
passent dans `build-cmake/qualification/altair-duty-time-20260925/tests.log`, avec
reproduction avant correction conservée dans `before-fix.log` et contrôles de
refus conflictuel après réouverture. Voir [le périmètre des tâches](ALTAIR-ATTESTER-DUTIES.md).
Ce jalon ne clôt aucune EIP ni l'activation réseau : horloge opérationnelle,
planification, propositions, diffusion et binding EVM restent ouverts.

### 2026-09-25 — construction locale des blocs Altair

`prepare_altair_unsigned_block` partage les contrôles de transition avec le
chemin reçu et calcule la racine d'état avant signature. L'admission reçue conserve
ses vérifications BLS et de racine, sans option de contournement publique.
Le corpus officiel fournit 105 reconstructions exactes, 12 refus de candidats
invalides et trois enveloppes incorrectes dont seul le corps est reconstructible.
[Construction et limites](ALTAIR-BLOCK-CONSTRUCTION.md) ; preuves dans
`build-cmake/qualification/altair-unsigned-block-20260925/final-tests.log`.
Ce jalon prépare les tâches de proposition ; il ne clôt pas le consensus autonome,
le binding EVM, la finalité distribuée ou une EIP de fork ultérieur.


### 2026-09-25 — signature des propositions liée au Store

Le coordinateur durable construit maintenant une proposition sur la tête choisie,
contrôle créneau, clé/domaine et journal, puis réserve avant signature BLS. Le
résultat reste soumis à l'admission normale. Qualification composée : bloc
identique au vecteur officiel, admission par un second coordinateur, archive et
restauration, onze refus avant fournisseur et expiration finale conservant la
réservation. Voir [construction et signature](ALTAIR-BLOCK-CONSTRUCTION.md) et
`build-cmake/qualification/altair-proposer-signing-20260925/final-tests.log`.
RANDAO fourni par le corps de test, opérations en attente, automatisation,
diffusion persistante et binding EVM restent ouverts ; aucune activation réseau
ni clôture d'EIP supplémentaire n'est revendiquée.


### 2026-09-25 — RANDAO local du proposant

`sign_randao` dérive le proposant depuis le parent authentifié avancé au slot,
lie clé/époque/fork/genesis et signe uniquement le domaine RANDAO. Le journal
sérialise avec son retrait et vérifie son checkpoint sans modifier les réservations
anti-slashing de blocs/votes. La révélation officielle est reproduite puis utilisée
dans le bloc signé/admis. Sept refus avant fournisseur, signature invalide,
expiration et passage synthétique à l'époque 1 sont couverts par
`build-cmake/qualification/altair-local-randao-20260925/final-tests.log`.
Voir [construction et RANDAO](ALTAIR-BLOCK-CONSTRUCTION.md). Restent la sélection
d'opérations, la planification, la diffusion persistante et la liaison EVM ; ce
jalon n'active aucun réseau et ne clôt pas le consensus de production.

### 2026-09-25 — finalité obtenue avec des blocs et votes générés localement

La qualification `tests/test_altair_local_finality.cpp` enchaîne sélection du
proposant, RANDAO local, construction/signature de blocs, tâches d'attestation,
admission et transitions de finalité. Trois scénarios de 33 blocs / 64 clés de
fixture exigent : finalité avec participation complète, absence de finalité sans
votes ou avec 21 clés votantes. Les bases du coordinateur et des 64 journaux sont
fermées/rouvertes au milieu de chaque scénario. Preuves :
`build-cmake/qualification/altair-local-finality-20260925/final-tests.log`.
[Portée et limites](ALTAIR-LOCAL-FINALITY.md) : test local piloté, sans SyncCommittee
participant, sans réseau distribué ni binding EVM. Le critère de finalité/reprise
réseau du goal reste ouvert ; cette campagne qualifie l'assemblage local.

### 2026-09-25 — rattrapage P2P d'une chaîne produite localement

`consensus_altair_generated_chain_libp2p` connecte deux vrais démons libp2p aux
coordinateurs source/récepteur et exige la réception de 33 blocs générés par les
64 clés de fixture. Vérifications : octets archivés, état SSZ de tête, justification,
checkpoint finalisé et restauration de celui-ci depuis le disque destinataire.
Preuve : `build-cmake/qualification/altair-generated-p2p-20260925/final-tests.log`.
Voir [périmètre réseau](ALTAIR-LOCAL-FINALITY.md). Ce test qualifie un rattrapage
réseau à horloge pilotée ; la finalisation collaborative, les partitions, la
planification autonome et le binding EVM restent ouverts.

### 2026-09-25 — rattrapage après perte d'ACK et panne du démon P2P

Le scénario `consensus_altair_generated_chain_libp2p_restart` exige la perte d'un
ACK observée, un récepteur tué par SIGKILL après persistance d'un préfixe et une
reconnexion des deux démons avec leurs identités conservées. Le rejeu doit préserver
ce préfixe et rejoindre exactement les 33 blocs et la finalité de la source.
Preuve : `build-cmake/qualification/altair-generated-p2p-restart-20260925/tests.log` ;
[périmètre précis](ALTAIR-LOCAL-FINALITY.md). Le cœur reste vivant pendant la panne
P2P. Les partitions de consensus, la redécouverte automatique, le crash du cœur
et la liaison à l'exécution ne sont pas clos par ce test.

### 2026-09-25 — refus d'une réécriture finalisée malgré des signatures valides

`consensus_altair_adversarial_finality` construit deux transitions complètes signées
en contournant volontairement le journal local des clés de fixture. La branche
qui évite le checkpoint finalisé est acceptée par un Store témoin au genesis,
mais refusée par le Store finalisé, avant et après fermeture/réouverture de sa base.
Un bloc concurrent descendant du checkpoint finalisé reste admissible. Preuve :
`build-cmake/qualification/altair-adversarial-finality-20260925/tests.log` ;
[périmètre et limites](ALTAIR-LOCAL-FINALITY.md). Les checkpoints hostiles au bootstrap,
les attaques de quorum, les partitions et les sanctions économiques restent hors
de cette qualification locale.


### 2026-09-25 — preuve de double proposition incluse et sanction persistante

Le scénario adversarial inclut maintenant les deux headers concurrents dans un
bloc produit/signé/admis normalement. Il compare toutes les balances avec une
transition témoin sans preuve, vérifie pénalité/récompense configurées, sortie,
échéance de retrait et vecteur de slashings, puis rouvre la base. Signature de
preuve altérée, doublon et rejeu ultérieur sont refusés avant signature sans
mutation ; un bloc normal est ensuite admis avec état attendu et finalité intacte.
Preuve : `build-cmake/qualification/altair-generated-slashing-20260925/final-tests.log` ;
[limites](ALTAIR-LOCAL-FINALITY.md). Sanctions corrélées, retrait final, preuves
acheminées automatiquement et profil Electra restent hors de ce scénario Altair.

### 2026-09-25 — socle Bellatrix pour le payload d'exécution

Le conteneur `ExecutionPayload` dispose d'un codec SSZ canonique et d'une racine
SSZ qualifiés sur huit vecteurs officiels épinglés : octets/racines exacts,
offsets et budgets contrôlés, uint256 conservé et annulation. Sources, limites et
critères suivants : [payload Bellatrix](BELLATRIX-EXECUTION-PAYLOAD.md). Preuves dans
`build-cmake/qualification/bellatrix-execution-payload-20260925/`.
État : composant de format implémenté, intégration consensus–EVM non commencée
pour ce schéma. Aucune validation d'exécution, commit atomique, activation réseau
Bellatrix ou conformité de fork complète n'est revendiquée.

### 2026-09-25 — candidat Bellatrix vérifié par l'EVM Paris

L'adaptateur `prepare_bellatrix_execution` lie le payload au contexte fourni
(parent, numéro, timestamp, RANDAO), reconstruit son hash RLP et rejoue les
transactions signées sur une copie de l'état parent authentifié. Les engagements
d'exécution faux sont refusés, y compris avec un hash annoncé cohérent. Les
annulations EVM sont propagées sans publication d'état partiel. Quatre
transactions synthétiques (types 0/1/2, stockage, REVERT), 16 payloads invalides,
41 points d'annulation et les huit vecteurs SSZ officiels passent les deux tests
ciblés. [Portée et preuves](BELLATRIX-EXECUTION-PAYLOAD.md).

État : intégration consensus–EVM commencée au niveau du candidat isolé.
L'appel depuis une transition Beacon Bellatrix authentifiée, la persistance
atomique des deux états et leur reprise restent à implémenter/qualifier.
Aucune activation réseau, clôture de fork ou aptitude production n'en découle.

### 2026-09-25 — liaison du payload à l'état Beacon Bellatrix

Les schémas SSZ du header d'exécution, de l'état Beacon (25 champs) et du corps
(10 champs) sont qualifiés par 24 vecteurs officiels épinglés. L'opération
`prepare_bellatrix_payload_state` authentifie la racine de départ, lie le parent
d'exécution au header mémorisé et dérive timestamp/RANDAO de l'état, puis prépare
les deux résultats sans mutation des parents. Les erreurs de disponibilité ou
de format de l'historique `BLOCKHASH` sont fatales, même si leurs effets EVM
seraient compatibles avec les engagements annoncés. Sept tests ciblés passent,
dont les régressions de schémas Phase0/Altair et des blocs Altair complets.
[Preuves, sources et limites](BELLATRIX-EXECUTION-PAYLOAD.md).

État : opération isolée implémentée/testée, transition signée Bellatrix complète
et durabilité conjointe toujours non terminées. Les essais de liaison utilisent
un état synthétique de format ; ils ne ferment aucun EIP ni le profil réseau.

### 2026-09-25 — slots/epochs Bellatrix et racines historiques

`prepare_bellatrix_slots` et `prepare_bellatrix_epoch` avancent désormais un état
déjà Bellatrix avec budgets locaux et marqueur de traitement. Les racines de
slots utilisent les 25 champs, en-tête d'exécution inclus ; les opérations
d'epoch communes sont réutilisées avec paramètres Bellatrix sans substituer une
racine Altair. Sept cas officiels, 100 slots et les sept comparaisons de parcours
groupé/fractionné produisent les post-états exacts. Des essais supplémentaires
couvrent annulation, double epoch, budgets et conservation du header lors d'un
rollover historique. [Sources, preuves et limites](BELLATRIX-SLOTS.md).

État : avancement slots/epochs implémenté et qualifié sur ce corpus ; traitement
signé du bloc Bellatrix, admission dans le choix de branche et persistance
Beacon–EVM conjointe toujours à terminer. Aucune aptitude production déduite.

### 2026-09-25 — bloc Bellatrix signé avec transactions EVM

`prepare_bellatrix_state_transition` raccorde maintenant slots/epochs, signature
BLS et proposant, exécution EVM avant la mise à jour du mix RANDAO, opérations de
bloc et racine finale. Les opérations communes réutilisent le parcours Altair
avec une projection privée réversible ; les engagements restent Bellatrix.
La construction locale est distincte de l'admission et ne signe pas de bloc.

Une chaîne synthétique de neuf blocs signés parmi 64 clés de fixture traverse
une epoch, exécute neuf transactions et inclut une double proposition sanctionnée
avec comparaison de toutes les balances. Sept blocs invalides, sept points
d'annulation et trois budgets sont couverts. Les six suites ciblées passent.
[Sources, preuves et limites](BELLATRIX-TRANSITION.md).

État : transition post-Merge complète implémentée en mémoire, qualification
encore partielle et non différentielle pour Bellatrix. Choix de branche,
signature locale durable et persistance/reprise Beacon–EVM ne sont pas encore
raccordés à ce parcours. Aucun EIP ou profil réseau n'est clôturé par ce test.

### 2026-09-25 — Store Bellatrix lié aux branches EVM

Le parcours signé Bellatrix alimente désormais un Store privé qui conserve
ensemble les états Beacon/EVM et les reçus par branche. La qualification locale
couvre dix blocs conservés, deux bascules de tête pilotées par six votes BLS,
les lectures du hash parent et le refus des liaisons d'exécution incohérentes.
Voir [BELLATRIX-FORK-STORE.md](BELLATRIX-FORK-STORE.md) pour la portée et les
limites : pas de commit atomique RocksDB, de reprise ni d'activation réseau
Bellatrix à ce stade. Cette étape ne clôture aucune qualification de production.

### 2026-09-25 — Primitive de commit atomique consensus/exécution

`commit_execution_checkpoint` regroupe snapshot, branche EVM, états, reçus et
index dans un batch RocksDB synchronisé, avec CAS du jeton et de la tête. Il
supporte les réorganisations, y compris vers une branche plus courte. Tests
locaux de concurrence, SIGKILL, refus de candidats incohérents et erreur de sync
avec conservation/perte du suffixe WAL : voir
[EXECUTION-CHECKPOINT.md](EXECUTION-CHECKPOINT.md). La sérialisation authentifiée
et la reprise du Store Bellatrix ainsi que son reactor réseau restent ouvertes ;
la primitive opaque ne clôture pas à elle seule la persistance du consensus.

### 2026-09-25 — Snapshot et reprise des branches Bellatrix/EVM

Le Store Bellatrix dispose d'une enveloppe canonique liée au profil attendu et
à un digest indépendant. La qualification compose cette sauvegarde avec le commit
atomique RocksDB : trois réouvertures, dont deux changements de branche, retrouvent
la même tête Beacon/EVM, le même état et les mêmes reçus. Six corruptions/écarts
sont refusés. Voir [BELLATRIX-STORE-RECOVERY.md](BELLATRIX-STORE-RECOVERY.md).
Ce sont des checkpoints locaux authentifiés, pas une synchronisation depuis un
pair. Le coordinateur durable Bellatrix, ses archives signées et le réseau restent
à raccorder ; la production et la finalité distribuée ne sont pas déclarées closes.

### 2026-09-25 — Coordinateur durable Bellatrix

Les événements blocs/votes/horloge/slashing sont raccordés au checkpoint atomique
via `BellatrixDurableReactor`. Qualification locale : équivalence au Store pur,
deux bascules de tête, redémarrage avec neuf blocs signés et six votes archivés,
annulations avant écriture, auteur obsolète et erreur de sync sur macOS.
Voir [BELLATRIX-DURABLE-REACTOR.md](BELLATRIX-DURABLE-REACTOR.md). La signature locale,
le transport Bellatrix et la qualification distribuée restent ouverts. Aucun
fork réseau n'est activé et la production n'est pas déclarée qualifiée.

### 2026-09-25 — Signature locale Bellatrix liée au Store

Le coordinateur dérive les duties, construit/exécute les propositions et utilise
le journal anti-slashing durable avant signature. Qualification locale : huit
propositions/reveals, une attestation et cinq refus, dont une double proposition
après réouverture du journal et une expiration pendant la signature. Voir
[BELLATRIX-LOCAL-SIGNING.md](BELLATRIX-LOCAL-SIGNING.md). La boucle autonome et le
transport réseau Bellatrix restent à raccorder ; pas d'activation de production.

### 2026-09-25 — Lot réseau Bellatrix en cours, non qualifié

Le code relie maintenant `run_node`, le runtime Bellatrix, les méthodes privées
`nano_bellatrix*` et le daemon libp2p sous `/nano/bellatrix/1.0.0`. Les blocs/votes
archivés passent par l'inventaire, l'export borné et les tickets d'admission durable.
Chaque enveloppe porte l'identité complète du profil ; les enveloppes Altair à
cinq champs ne sont pas acceptées en Bellatrix. Les imports/synchronisations de
blocs legacy sont refusés dans ce mode ; le gossip des transactions est conservé.

Le mode genesis `beacon` exige Paris. Le runtime partage la base canonique et
exige soit un snapshot initial local authentifié, soit un jeton exact de reprise,
sans repli automatique. Le RPC ne démarre qu'après vérification du checkpoint.
L'horloge locale invalide le coordinateur en cas de discontinuité ou erreur locale.
Tête, état EVM, mempool et événements sont publiés après le commit atomique ; la
préparation de la mempool conserve les métadonnées déjà authentifiées et traite
les transactions communes aux branches avant réadmission des orphelines.

Compilation réussie de `nano`, `nano-p2p`, `nano-beacon-execution-gate-tests`,
`nano-txpool-canonical-tests`, `nano-bellatrix-wire-identity-tests` et
`nano-bellatrix-transition-tests`. Journaux locaux :
`build-cmake/qualification/bellatrix-network-integration-20260925/`.
La première compilation a révélé des comparaisons string/JSON corrigées ; un
appel a ensuite demandé une nouvelle cible avant régénération de CMake, puis la
régénération et la compilation de cette cible ont réussi.

La campagne regroupée a exécuté les nouveaux scénarios. Les nouveaux fichiers
couvrent les contrôles d'accès avant vérification du checkpoint, l'isolation des
mutations legacy, les enveloppes de réseau/IPC et la mempool lors des réorganisations.
Le scénario local de boucle validateur passe après les corrections décrites
ci-dessous. La finalité distribuée et la reprise réseau restent à terminer. La compilation
ne clôture aucun EIP ni critère de production. Aucun push ou déploiement de ce lot.

La boucle validateur locale est maintenant écrite et raccordée à la configuration
optionnelle `bellatrix.validator`. Elle charge les clés BLS depuis les variables
d'environnement nommées (exclues de l'environnement du daemon P2P), vérifie leur
liaison au registre authentifié et utilise un journal distinct par clé. Elle
construit le payload depuis la mempool et l'état de la branche, réserve de la
place pour les attestations, signe via le journal puis repasse par l'admission
normale. Les réservations de signature déjà durables sont respectées après reprise.
Le chemin Beacon de préparation EVM n'ajoute plus la récompense legacy de minage.

Nouveau scénario écrit : `consensus_bellatrix_validator_loop` (64 clés locales,
horloge synthétique, transaction de la mempool, finalité puis réouverture de la
base). **Il passe en 80,26 s et ne représente pas un réseau distribué.**
Résultat : 31 propositions, 200 attestations admises, époque 2 finalisée et
époque 3 justifiée, transaction exécutée, état/racine/finalité retrouvés après
réouverture. Les scénarios de mempool, de contrôle de démarrage, d’identité des
enveloppes et de transitions signées passent également dans leur périmètre.
Le constructeur utilise pour l'instant une participation sync vide et les votes
archivés ; il refuse une proposition nécessitant des dépôts obligatoires qu'aucun
fournisseur local ne lui a donnés. La collecte des opérations, l'agrégation et la
qualification de performance restent ouvertes. La validation complète de chaque
vote candidat est une base de correction, pas une preuve de débit en production.

La première exécution de la boucle a révélé deux écarts corrigés : abstention
lorsqu'un checkpoint d'époque précède l'ancre disponible, puis conservation
durable des votes du slot courant sans effet prématuré sur le choix de branche.
Le tick du slot suivant applique ces votes ; la reprise vérifie aussi les votes
en attente. Le scénario a été prolongé à quatre époques pour respecter le délai
FFG de démarrage, sans réduire les critères : époque finalisée >= 2 et justifiée >= 3.

Le nouveau scénario `consensus_bellatrix_validator_network` utilise deux daemons
libp2p et deux cœurs HTTP/base séparés dans un seul processus de test. Sa première
exécution a synchronisé les blocs et l'état mais a échoué sur la participation :
les retours de messages identiques modifiaient inutilement le jeton durable et
perturbaient les tâches locales. Les doublons exacts vérifient désormais le
checkpoint existant sans recommit. Le scénario réseau passe après cette correction en 112,33 s : même état EVM,
reçu de transaction et époque finalisée 2 sur le nœud observateur. Ce scénario concentre les clés
sur un seul hôte validateur, avec un second nœud observateur ; il ne qualifie pas
la répartition du stake entre machines ni les partitions réseau.

Le contrôle de concurrence a été adapté pour modifier effectivement l'horloge :
un appel identique n'écrit plus de checkpoint. L'ancien scénario supposait à
tort que deux opérations sans changement de contenu créaient un auteur obsolète.
Les contrôles conservent les refus d'auteur obsolète, les annulations avant commit
et l'erreur de synchronisation disque ; la dernière exécution passe en 4,07 s.
Le journal `cas-final.log` conserve ce résultat.

Le scénario ajouté `consensus_bellatrix_runtime_restart` passe en **8,98 s**.
Il appelle le véritable chargeur `start_bellatrix_runtime` avec profil/snapshot
sur disque, 64 clés BLS synthétiques chargées par variables d’environnement et
64 journaux séparés. Il produit un bloc, arrête les services, retient explicitement
les engagements du coordinateur et des journaux, rouvre la base puis produit le
bloc suivant. Un nouvel appel au même slot ne répète pas les signatures réservées.
Il refuse un digest de profil altéré, une capacité de transport insuffisante,
deux sources de confiance simultanées, une clé incompatible avec son index et
un engagement de reprise obsolète. Une régression de l’horloge ferme l’accès à
l’exécution. `BellatrixValidatorLoop::restart_commitments()` permet la capture
explicite des engagements des journaux ; ce n’est pas une sauvegarde externe
automatique ni une garantie contre le rollback de ces engagements.
Preuve locale : `runtime-restart.log` dans le répertoire de qualification du lot.
Ce scénario reste dans un processus avec horloge synthétique : le lancement par
CLI, les cœurs indépendants, le stake réparti et les partitions restent à qualifier.

Les trois contrôles de non-régression du transport partagé passent en 3,06 s :
`consensus_altair_wire_admission`, `consensus_altair_core_client` et
`consensus_altair_libp2p`. Ils ont été reconstruits après les changements aux
schémas d’enveloppes, au dispatch IPC et à la compatibilité des pairs. Le dernier
utilise un vrai daemon et neuf identités de pairs distinctes ; ce résultat couvre
l’admission réseau, pas la finalité distribuée. Journal :
`shared-transport-regression.log`. Les scénarios réseau Bellatrix déjà passés
n’ont pas été rejoués pour cette seule extension du test de configuration.

### 2026-09-25 — Lancement CLI Bellatrix et reprise des processus

`consensus_bellatrix_managed_runtime` lance deux vrais binaires `nano run`,
chacun avec son daemon libp2p, sa base et son serveur HTTP. Le Genesis Beacon
est chargé par le parseur réel ; le snapshot de consensus synthétique correspond
à ce Genesis. Les 64 clés de test sont chargées via l’environnement du seul
nœud validateur. L’autre cœur est observateur. L’horloge est celle du système.
Le scénario attend la synchronisation au bloc 3, arrête le validateur,
reprend avec les engagements capturés à l’arrêt et vérifie la réception du bloc
suivant sur l’observateur resté actif. La première exécution passe en 18,21 s
(blocs 3 puis 4). Le test du chargeur/reprise passe aussi en 6,46 s après
raccordement au même arrêt explicite.

L’arrêt expose `bellatrix-runtime-stopped` avec le jeton du coordinateur et ceux
des journaux des clés. Le chemin `run_node` termine le daemon et les workers HTTP
avant la capture ; `stop_bellatrix_runtime` termine les devoirs locaux, l’horloge
et les workers consensus avant de lire ces engagements. Aucun secret n’est
inclus. Ces métadonnées doivent être retenues séparément et reportées dans la
configuration de reprise ; leur émission dans un journal ne constitue pas une
sauvegarde externe automatique. Un arrêt brutal et la perte du dernier engagement
ne sont pas résolus par une confiance automatique dans le disque.

Périmètre prouvé : processus indépendants, démarrage CLI, production signée,
synchronisation P2P et reprise propre d’un validateur. Restent ouverts : stake
réparti entre signataires indépendants, finalité distribuée, partition/rattrapage,
arrêt brutal et procédures de conservation durable des engagements. Aucun de
ces résultats ne clôture le lancement public ni les autres EIP de la matrice.

Après correction de l’ordre de terminaison HTTP, le scénario concerné est rejoué
et passe en **17,97 s**, avec synchronisation aux blocs 3 puis 4 après reprise.
Journal complet : `managed-drain-qualification.log`. Cette réexécution couvre
la modification de l’arrêt ; les suites EVM et de finalité locale inchangées
n’ont pas été relancées. Aucun push ni déploiement de ce lot à ce stade.

### 2026-09-25 — Qualification des signataires répartis

Nouveau scénario `consensus_bellatrix_distributed_finality` : deux cœurs `nano run`
et deux daemons libp2p sur un même Mac, chacun avec 32 des 64 clés synthétiques.
Les environnements enfants ne reçoivent que leurs clés respectives. Le scénario
redémarre un des validateurs avec ses engagements exacts, puis attend quatre
époques avec horloge réelle. Le critère de passage exige le même checkpoint
finalisé (racine et époque), d’époque >= 2, sur les deux nœuds ; une tête commune
ou un bloc produit ne suffit pas. Les engagements d’arrêt exposent maintenant
également le checkpoint finalisé et l’époque justifiée pour cette vérification.

Le budget propre à ce test est explicite : 128 blocs, 2 048 votes archivés,
64 Mio d’états cumulés, enveloppe de 128 Mio. Ce budget ne modifie pas les règles
du registre ni les paramètres d’un réseau déployé. Il ne corrige pas l’absence
d’élagage des états/blocs du Store, qui reste un écart de fonctionnement durable.
Ce scénario ne couvre ni plusieurs machines physiques ni les partitions.

**Résultat : passage en 183,45 s.** Les deux nœuds finalisent l’époque 2 avec la
même racine `0xc90b8d700df145836b8a3d43af2c85fd536c4a9271a548da0f1a89509c07799f`
et justifient l’époque 3. Le scénario atteint le slot 32 (hauteur d’exécution 31,
un slot sans bloc) après redémarrage d’un validateur à la hauteur 3. Cette preuve
couvre une finalité commune avec signataires répartis entre deux processus,
chacun détenant la moitié des clés, sur **une seule machine physique**.
Journal complet : `distributed-qualification.log` dans le répertoire du lot.
Les partitions, la panne brutale, le rattrapage prolongé, l’élagage et la mise
à l’échelle ne sont pas clos par ce résultat. Le RPC refuse encore explicitement
les tags `safe`/`finalized` ; leur raccordement au checkpoint Beacon reste ouvert.
Aucun push, changement du VPS ou lancement public n’est effectué ici.

### 2026-09-25 — Raccordement RPC du checkpoint finalisé Beacon

Le tag `finalized` est raccordé au bloc d’exécution du checkpoint finalisé
Beacon. Il est publié sous le verrou de chaîne après le même commit atomique
que la tête/l’état, vérifié contre l’ancêtre canonique ou le suffixe validé,
et ne peut reculer ou changer de hash à hauteur égale. Après réouverture,
le coordinateur revalide le snapshot et le bloc canonique avant de rendre
ce tag disponible. Le tag est refusé sans consensus Beacon authentifié ;
il ne constitue jamais un alias de `latest`.

Référence : [paramètre de bloc JSON-RPC Ethereum](https://ethereum.org/en/developers/docs/apis/json-rpc/#the-block-parameter).
Méthodes raccordées par le résolveur commun : lecture de blocs/transactions,
solde, nonce, code, stockage, historique des frais et simulations. `safe` reste
refusé, et le parseur spécifique des filtres/logs ne prend pas encore en charge
`finalized` : aucune conformité RPC complète n’est déclarée ici.
Le scénario de finalité locale comporte désormais une transaction supplémentaire
après le checkpoint : le stockage finalisé doit rester à 2 tandis que la tête
courante vaut 3. Les réponses sont comparées à l’état d’exécution associé au
checkpoint du Store, puis revérifiées après réouverture.

**Qualification passée : 3/3 en 90,11 s.** Contrôle d’accès : 0,56 s ;
configuration/reprise : 9,11 s ; boucle avec finalité et lectures RPC : 80,44 s.
Le scénario compare huit familles de réponses au checkpoint puis après
réouverture, vérifie le stockage finalisé à 2 contre 3 à la tête et l’absence
de mutation canonique par `eth_call`. L’époque 2 reste finalisée, l’époque 3
justifiée. Preuve : `finalized-rpc-qualification.log`. Les scénarios P2P longs,
non modifiés par ce raccordement, n’ont pas été relancés. Les binaires de test
ont été liés au code modifié ; aucun binaire n’a été déployé sur le VPS.

### 2026-09-25 — Coupure du transport entre deux moitiés du stake

Scénario ajouté : `consensus_bellatrix_partition_recovery`. Il démarre les deux
cœurs et daemons réels, répartit les 64 clés en deux ensembles de 32, effectue
une reprise propre puis attend quatre époques. Après établissement d’une finalité
commune, il suspend uniquement les deux daemons enfants par SIGSTOP pendant
deux époques (96 s avec ce preset). Les cœurs/signataires restent actifs.
Il exige que les deux branches avancent et divergent, sans changement du bloc
finalisé. Après SIGCONT, il exige un nouveau bloc finalisé commun plus récent
et la convergence des têtes. Les daemons sont repris même si une assertion échoue.
La coupure est une suspension du composant transport, pas une simulation de
latence/perte sélective de paquets ni de plusieurs machines physiques.

La durée totale est bornée par CTest à 720 s. Aucun trafic de flood ni fonds
réels. `partition-run-inputs.json` conserve les empreintes des binaires et des
sources modifiées utilisées, avec le commit de base. Le résultat doit être lu
avec le journal complet, sans assimiler le démarrage du scénario à son succès.

**Première exécution : échec en 519,26 s.** La finalité reste inchangée pendant
les 96 s d’isolement et les têtes reconvergent ensuite, mais aucun checkpoint
plus récent n’est finalisé dans les cinq époques de récupération. Le journal
`partition-qualification.log` et `partition-run-inputs.json` conservent cet échec.
Il ne faut pas le résumer comme une réussite de la reprise après partition.

L’inspection du relais révèle un blocage de parcours sur un vote explicitement
refusé et une cadence d’un seul élément par seconde, alternant blocs/votes.
Correction en cours de qualification : les votes refusés sont différés jusqu’à
la fin d’un parcours, sans bloquer les suivants ni être considérés comme livrés.
Un ACK absent/ambigu ne fait toujours pas avancer le curseur ; l’ordre des
blocs reste inchangé. Le relais utilise un timer indépendant, 100 ms par défaut,
réglable par `NANO_CONSENSUS_RELAY_INTERVAL_MS` (25..60000 ms), avec toujours une
seule tentative en vol et les mêmes budgets d’admission entrants. Cette cadence
est une politique locale, pas une règle de stake. Le nouveau scénario de transport
`consensus_relay_rejected_vote_progress` vérifie les refus, la reprise différée,
les réponses perdues et la conservation de l’ordre des blocs ; il passe.
Le lien causal avec l’échec de finalité doit encore être confirmé par le scénario
complet, dont les critères restent inchangés.

**Seconde exécution : non qualifiée.** Le contrôle Altair/libp2p passe en 2,30 s,
mais le scénario de partition échoue en 228,84 s sur
`isolated half-stake advanced finality`, avant la récupération réseau. Le journal
complet `partition-recheck.log` contient les traces de relais ;
`partition-recheck-inputs.json` identifie les sources et binaires de ce passage.
Le test local de progression du relais passe en 0,33 s. Cela ne suffit pas à
valider la correction de la récupération après partition.

La trace du second échec ne donne pas les hauteurs et racines des deux réponses
ayant déclenché cette assertion. Il faut capturer les checkpoints au changement
et leur lien au préfixe signé avant coupure pour distinguer une finalisation
retardée utilisant des votes antérieurs d’une finalisation illégitime pendant
l’isolement. Aucune violation du quorum n’est affirmée sur la seule assertion,
et aucune réussite de reprise n’est revendiquée. Les critères de sécurité ne
sont pas retirés. Aucun push ou déploiement de ce correctif non qualifié.

### Diagnostic FFG retardé — 25 septembre 2026

Le test `ethereum_official_altair_ffg` couvre désormais trois assertions supplémentaires : un quorum de l’époque précédente peut finaliser un checkpoint plus ancien sans votes de l’époque courante ; la racine finalisée reste celle du checkpoint ; sans ce quorum précédent, cette finalisation ne se produit pas. Les dix fixtures officielles et ces nouveaux cas passent (0,31 s). Source : [consensus-specs v1.5.0, justification et finalisation](https://github.com/ethereum/consensus-specs/blob/v1.5.0/specs/phase0/beacon-chain.md#justification-and-finalization).

Ce contre-exemple précise la portée d’une assertion de partition ; il ne démontre pas la cause du précédent échec réseau et ne clôt pas la reprise après partition. Le scénario réseau diagnostique conserve son assertion stricte pendant son exécution. Preuves et empreintes : `build-cmake/qualification/bellatrix-network-integration-20260925/{nano-delayed-ffg-test.log,delayed-ffg-inputs.json}`.

### Partition diagnostique : reprise non acquise — 25 septembre 2026

`consensus_bellatrix_partition_recovery` échoue en **518,79 s** après deux époques de coupure puis cinq époques de reconnexion. Pendant la coupure, les branches progressent sans changer la finalité ; le scénario ne rencontre donc pas cette fois l’assertion ambiguë de finalisation retardée. La finalité RPC reste au bloc `0x1`. À l’arrêt ordonné, les deux nœuds exposent la même finalité Beacon d’époque `0x0`, une justification d’époque `0x2`, `clockHealthy=true` et `validator.healthy=true`. Les compteurs locaux sont respectivement 39/35 propositions et 165/171 attestations admises (le compteur source repart après son redémarrage initial).

Cela écarte un arrêt déclaré de l’ordonnanceur ou de l’horloge dans cette exécution. Cela ne prouve ni la livraison utile de chaque vote, ni son inclusion dans la branche canonique : la prochaine investigation porte sur ces deux étapes et la participation par époque. La correction du relais ne clôt donc pas la reprise. Aucun assouplissement d’assertion, push ou déploiement n’a été réalisé. Preuves : `build-cmake/qualification/bellatrix-network-integration-20260925/partition-diagnostic.log` et `partition-diagnostic-inputs.json`.

### Conservation des preuves de partition — 25 septembre 2026

Le scénario sait désormais conserver ses bases **après arrêt des deux processus**, uniquement sur échec et avec `NANO_TEST_PARTITION_EVIDENCE` explicite ; il refuse d’écraser un dossier précédent. Le nouvel outil natif `nano-bellatrix-checkpoint-export` lit une copie de base et vérifie le checkpoint contre le jeton enregistré dans la sortie d’arrêt indépendante avant export. Il ne lance ni réseau ni signataire. Compilation réussie ; validation de l’export sur la prochaine base conservée encore en attente.

Une exécution diagnostique est en cours avec cette conservation, sans modification supplémentaire des règles de consensus ni de l’assertion de partition. Ses entrées sont empreintées dans `build-cmake/qualification/bellatrix-network-integration-20260925/partition-retained-inputs.json`. Aucun verdict nouveau de reprise n’est acquis à ce stade.

### Participation après partition et coût des clés sans devoir — 25 septembre 2026

L’exécution avec conservation échoue en **521,87 s**. Les bases arrêtées sont conservées (42 Mio). L’export natif vérifie avec succès les deux checkpoints contre les engagements d’arrêt ; les rapports dérivés des SSZ indiquent que le maximum de participation cible précédente dans les états retenus n’atteint que 20,3 %, 20,3 %, 9,4 % et 12,5 % aux époques 7 à 10, sur les deux nœuds. Ces maxima sur les branches retenues ne sont pas des métriques de réseau distribué ni une validation complète du parseur diagnostique.

L’inspection du chemin local montre une relecture du checkpoint complet (4 690 018 octets en fin de ce scénario) pour chaque clé interrogée par `sign_randao` et `sign_attestation`, y compris celles sans devoir. La vérification disque est déplacée après la sélection du devoir et reste obligatoire **avant toute signature éligible**. Cela élimine des relectures inutiles ; son effet sur la reprise de finalité reste à mesurer. Une qualification groupée de la chaîne signée et de la boucle de validation est lancée après cette modification.

Preuves : `partition-retained-db/`, `nano-{source,observer}-checkpoint.json`, `nano-{source,observer}-participation.txt` et le script d’analyse conservé, sous `build-cmake/qualification/bellatrix-network-integration-20260925/`.

### Sélection groupée des devoirs locaux — 25 septembre 2026

La seule suppression des lectures disque pour les clés sans devoir préserve les deux tests ciblés (84,17 s cumulées), mais ne réduit pas sensiblement la durée de la boucle locale (80,41 s contre 80,44 s précédemment). Elle ne peut donc pas être présentée comme la cause résolue du blocage.

Le lot ajoute une sélection groupée du proposant et du comité d’attestateurs à partir de l’état authentifié. L’ordonnanceur évite d’interroger les fonctions de signature pour toutes les clés sans devoir ; les fonctions de signature revalident encore identité, slot, engagement disque et journal avant utilisation du signataire. La préparation du comité partage désormais les traversées d’état et le calcul des affectations. Les nouveaux contrôles comparent le proposant à la sélection de référence, vérifient les huit affectations du profil minimal, leur bit unique, l’exclusion d’un équivoque et le refus d’un slot périmé.

Compilation réussie ; qualification groupée runtime/redémarrage, boucle locale et chaîne signée en cours. Ni la reprise réseau ni la capacité à 100 000 validateurs ne sont démontrées par cette modification. Entrées : `batch-duties-inputs.json` dans le dossier de qualification Bellatrix.

La sélection groupée passe ses trois tests : redémarrage 8,66 s, boucle locale 75,01 s et chaîne signée 3,72 s. Ce gain reste modeste et ne prouve pas encore la reprise réseau.

### Attestations identiques déjà incluses — 25 septembre 2026

Les blocs conservés montrent des inclusions répétées des mêmes attestations. Le constructeur local filtre maintenant les empreintes des attestations identiques déjà incluses dans les **ancêtres récents du parent choisi**, sans consulter les branches concurrentes pour décider une exclusion. Les messages restent archivés pour le relais. Les nouveaux votes gardent la validation complète habituelle. La boucle locale vérifie désormais l’absence de doublons identiques dans sa chaîne et la conservation d’au moins 128 attestations utiles ; finalité, transactions, réouverture et garde-fous de signature restent dans la qualification groupée lancée. Résultat encore en attente.

La qualification groupée après déduplication passe **3/3** : runtime/redémarrage 8,69 s, boucle locale **28,04 s**, chaîne signée 3,75 s (40,49 s cumulées). La boucle locale était à 80,41 s avant sélection groupée/déduplication ; ces mesures sur le même Mac indiquent une réduction d’environ 65 % de sa durée, sans constituer un benchmark réseau. Le nouveau contrôle d’unicité et de conservation des votes utiles passe, ainsi que la finalité et la réouverture.

La partition est relancée avec ce lot modifié et conservation des bases en cas d’échec (`partition-optimized-db`, manifeste `partition-optimized-inputs.json`). Sa réussite n’est pas acquise au lancement.

### Reprise : arrêt des propositions et erreurs masquées — 26 septembre 2026

La partition après optimisation échoue en **518,11 s**. Les deux ordonnanceurs restent déclarés sains (203 et 195 attestations admises), finalité d’époque 0 et justification d’époque 2. Les bases concordent : horloge au slot 88, dernier bloc au slot 79. Les gains locaux n’ont donc pas résolu la reprise réseau.

L’inspecteur hors ligne reconstruit l’état depuis l’engagement d’arrêt vérifié. Au slot 88 il trouve le proposant 36, huit attestateurs et un corps de bloc valide de 3 395 octets. Le diagnostic de corps reconnaît uniquement les clés publiques des fixtures synthétiques connues, prépare leur révélation RANDAO et ne publie ni bloc ni transaction. Il opère sur la copie arrêtée, sans horloge active ni réseau. Cela écarte une impossibilité intrinsèque de construire le prochain bloc depuis cet état ; cela ne reproduit pas les courses concurrentes.

Le traitement d’erreur local avait un défaut : n’importe quelle `invalid_argument` pouvait être ignorée si le jeton avait changé entre-temps. La reprise est maintenant limitée à une exception typée `StaleBellatrixDuty`, émise explicitement lors d’un conflit de jeton. Les autres erreurs arrêtent l’ordonnanceur et restent visibles dans `failure` ; `staleDutyRetries` compte les conflits réellement classifiés. La lecture initiale du réacteur est elle aussi couverte par ce traitement. Nouveau contrôle de classification et qualification groupée en cours ; aucun nouveau résultat réseau revendiqué.

La qualification du traitement typé passe **3/3** : runtime/redémarrage 8,82 s, boucle locale 27,78 s, chaîne signée 3,77 s (40,37 s cumulées). La boucle locale termine sans erreur ni reprise de conflit dans ce scénario synthétique. Les conflits réels du réseau et la cause de l’arrêt des propositions restent à reproduire ; aucune nouvelle partition n’est lancée à ce stade. Les bases conservées servent au prochain diagnostic de reprise. Preuves : `nano-duty-race-qualification.log` et `duty-race-inputs.json`.

### Reproduction courte et proposition cohérente — 26 septembre 2026

L’inspection des journaux à partir de leurs engagements d’arrêt ne montre aucune proposition réservée au-delà du slot 79. Sur une **copie distincte**, l’outil natif reconnaissant les clés de fixture synthétiques prépare, signe puis admet le bloc du slot 88 : tête 79 → 88, sans réseau ni fonds réels (`nano-fixture-replay.json`).

La reprise de 24 secondes avec horloge et ordonnanceur actifs, sans P2P, reproduit ensuite le blocage : zéro proposition, neuf attestations, sept reprises pour conflit de version, tête toujours 79 (`nano-fixture-resume.json`). Les bases historiques sont conservées ; chaque essai utilise une autre copie.

Le chemin local de proposition regroupe maintenant RANDAO, construction et signature sous un même verrou du réacteur. Les wrappers unitaires conservent leur verrouillage ; les helpers privés sont utilisés uniquement sous ce verrou. L’horloge ne peut plus changer le jeton entre ces étapes. Délais réels, identité, journaux anti-slashing, validation complète et admission normale après signature restent appliqués. La reprise courte modifiée admet une proposition et treize attestations, avec trois conflits de version, tête 79 → 91 et ordonnanceur/horloge sains (`nano-fixture-atomic.json`). Ces deux observations bornées ne sont pas un benchmark ni une preuve de finalité réseau.

Qualification groupée chaîne signée, boucle locale, redémarrage et scénario à deux processus gérés lancée ; résultat en attente. Entrées empreintées : `atomic-proposal-inputs.json`.

La qualification de la proposition cohérente passe **4/4** en 49,19 s : processus réseau gérés/redémarrage, runtime local, boucle à 64 clés et chaîne signée. La boucle locale prend 26,78 s ; les scénarios conservent les contrôles de finalité, signature, annulation et réouverture dans leurs périmètres respectifs. La matrice principale a été actualisée pour distinguer intégration Bellatrix existante et qualification réseau encore partielle.

Le scénario complet de partition est lancé avec ce lot (`partition-atomic-inputs.json`, conservation prévue dans `partition-atomic-db`). Aucun verdict de reprise ni statut production n’est acquis au lancement.

### Départ à finalité 2, puis conflits locaux persistants — 26 septembre 2026

La partition avec proposition cohérente échoue en **517,25 s**. Avant isolement, la finalité atteint cette fois le bloc 16 / époque 2 ; elle reste inchangée pendant la coupure et la reprise. À l’arrêt : mêmes 77 blocs retenus, dernier slot 83, 218/217 attestations admises, aucune erreur déclarée, mais **206/203 reprises pour conflit de version**. Le résultat ne clôt pas la finalité après reconnexion.

Les opérations locales de devoirs dérivent désormais leurs données depuis l’état courant sous le verrou qui protège l’opération. Les appels locaux ne transportent plus un jeton capturé entre deux prises de verrou, périmé par les publications d’horloge. Les API à engagement explicite continuent à refuser les anciens jetons ; les vérifications disque, registre, clé, slot, signature et journal sont conservées. Les sélecteurs de tâches fournissent seulement des indications : les méthodes de signature redérivent et vérifient les données sous verrou.

Sur une nouvelle copie de la même base de référence, la reprise locale de 24 secondes admet **2 propositions et 14 attestations, zéro conflit de version**, tête 79 → 91, horloge et ordonnanceur sains (`nano-fixture-current.json`). Cette observation n’est pas un résultat réseau. Contrôles supplémentaires : équivalence du comité local au comité du checkpoint, refus d’un slot futur et impossibilité pour une clé non affectée d’atteindre le signataire. La qualification groupée correspondante est en cours. Le README décrit désormais ce profil Bellatrix et ses limites.

La qualification des devoirs dérivés sous verrou passe **4/4** en 49,34 s : chaîne signée 3,48 s, boucle locale 26,88 s, runtime/redémarrage et processus réseau gérés 18,97 s cumulées. Les nouveaux contrôles de clé non affectée et de slot futur passent. Le scénario complet de partition est lancé sur ce lot (`partition-current-inputs.json`, conservation prévue `partition-current-db`) ; son résultat reste en attente.

### Reprise réseau après correction des devoirs locaux — 26 septembre 2026

Le scénario `consensus_bellatrix_partition_recovery` passe en **420,77 s** sur les binaires du manifeste `partition-current-inputs.json`, dont les neuf empreintes ont été revérifiées après le test. Preuve conservée : `build-cmake/qualification/bellatrix-network-integration-20260925/nano-partition-current.log`. Aucun nouveau lancement n’a été nécessaire pour recueillir ce résultat.

Périmètre effectivement exercé : deux processus cœur et deux démons P2P, 64 clés synthétiques réparties entre deux validateurs, horloge réelle, un seul Mac. Le scénario comprend un redémarrage ordonné avec engagements externes, quatre époques initiales, 96 secondes de suspension des communications avec branches concurrentes actives, puis reconnexion. La finalité initiale (bloc 16 / époque 2) reste inchangée pendant la coupure. Après reconnexion, les deux nœuds retrouvent une tête commune et une finalité strictement plus récente ; à l’arrêt, même racine finalisée `0x9732ff022994f1c76ab927942248bc9fa6699db30a0dfbef5dc3baef08c531c5`, époque 7, justification 8. Les assertions de sûreté et de reprise n’ont pas été affaiblies.

Les échecs précédents restent conservés ci-dessus. Cette réussite clôt ce scénario borné de reprise ; elle ne clôt ni le registre ouvert, ni la reprise après crash sans engagement d’arrêt, ni les partitions prolongées, ni la qualification sur plusieurs hôtes. Le journal de réussite n’exporte pas les compteurs finaux de l’ordonnanceur : aucune affirmation de zéro conflit de version réseau n’en est déduite.

Preuve synthétique versionnée (événements observés et empreintes, sans clés privées) : [`qualification/bellatrix-partition-20260926.json`](qualification/bellatrix-partition-20260926.json). Le journal brut reste dans le dossier local de qualification indiqué ci-dessus.

### Vérification groupée des composants partagés — 26 septembre 2026

La revue du lot examine aussi le rattachement du runtime au nœud, le filtrage des secrets transmis au démon, l’arrêt avant capture des engagements, les tags RPC de finalité, la publication du mempool et le relais partagé Altair/Bellatrix. La vérification groupée de fin de lot passe **10/10 en 5,10 s** : historique/capacité/publication canonique du mempool, identité Bellatrix, verrou d’exécution, progression après vote refusé, reçus IPC, client cœur, transport libp2p Altair et journal anti-slashing. Ce contrôle cible les composants partagés modifiés ; les scénarios locaux et la partition réussis ne sont pas relancés.

Preuve versionnée : [`qualification/bellatrix-shared-20260926.json`](qualification/bellatrix-shared-20260926.json), empreintes des exécutables et du journal. Journal brut local : `build-cmake/qualification/bellatrix-network-integration-20260925/nano-bellatrix-lot-shared.log`. Ces résultats ne clôturent pas les critères de production encore ouverts.

### Réservations Beacon avec témoin durable — 26 septembre 2026

Une option C++ de `BeaconSigningJournal` écrit un témoin indépendant avant le principal. À la reprise, elle termine seulement la réservation exacte autorisée par le témoin ; elle refuse une base trop ancienne ou une identité différente. La politique de signature et le mode historique à engagement exact sont conservés. Le témoin doit être protégé de la restauration conjointe avec le principal ; ce n’est pas une détection de rollback de sa propre racine de confiance.

Qualification **2/2 en 2,30 s** : journal existant modifié et nouveau scénario `consensus_beacon_witness_recovery`. Nouveau périmètre : `SIGKILL` après réservation avant signature, réouverture sans reçu d’arrêt, restauration du seul principal à son prédécesseur puis achèvement exact, refus de rollback plus ancien, instance périmée, témoin absent et mauvaise identité, erreurs WAL injectées sur le témoin puis sur le principal empêchant toute signature. La perte de la dernière écriture principale est modélisée par une copie arrêtée ; ce n’est pas un essai de coupure électrique physique.

Preuve versionnée : [`qualification/beacon-witness-20260926.json`](qualification/beacon-witness-20260926.json). Journaux locaux : `build-cmake/qualification/beacon-witness-20260926/`. Raccordement au runtime, migration des journaux existants et reprise du checkpoint consensus/exécution restent ouverts ; aucun redémarrage complet après crash ni statut production n’est revendiqué.

### Raccordement des témoins au runtime Bellatrix — 26 septembre 2026

Le runtime charge désormais un témoin optionnel pour chaque clé et un témoin distinct pour le checkpoint couplé consensus/exécution. La publication du coordinateur synchronise l’intention complète avant le commit atomique principal. Au redémarrage, le snapshot est vérifié (profil, état, archives signées, digest) et seule l’écriture de son prédécesseur exact peut être achevée. Une version plus ancienne est refusée avant exposition de l’exécution. Le mode antérieur à engagement explicite demeure disponible.

Le nouveau scénario `consensus_bellatrix_witness_runtime` passe en **12,28 s** : runtime à 64 clés, reprise sans injecter les engagements d’arrêt, conservation tête/état, absence de signature répétée, reprise des propositions, récupération d’une publication du coordinateur depuis son prédécesseur et refus d’un rollback plus ancien avec exécution fermée. Le scénario existant `consensus_bellatrix_runtime_restart` passe en **6,16 s**. Un premier échec provenait du partage accidentel du témoin entre le cas de configuration invalide et le cas valide ; les dossiers ont été isolés, sans affaiblir le refus d’écrasement. Les deux journaux sont conservés.

Preuve : [`qualification/bellatrix-witness-runtime-20260926.json`](qualification/bellatrix-witness-runtime-20260926.json). Il reste à qualifier un arrêt brutal du processus réseau complet, la perte d’écriture lors d’une publication changeant effectivement la tête EVM et le coût du double stockage sous charge. La récupération actuelle ne protège pas la restauration conjointe des témoins et des bases principales.

### Crash du processus réseau et perte de publication EVM — 26 septembre 2026

Le nouveau scénario `consensus_bellatrix_crash_runtime` passe en **19,14 s** : deux vrais cœurs `nano run`, deux démons libp2p, 64 clés sur le validateur source, un observateur, un seul hôte. Après synchronisation au bloc 3, `SIGKILL` tue le cœur source sans événement `bellatrix-runtime-stopped`. Le redémarrage utilise uniquement les témoins configurés, sans injecter de reçus d’arrêt ; le bloc 3 reste canonique et le bloc 4 atteint l’observateur. Ce scénario ne comporte pas d’assertion de finalité distribuée (un seul nœud détient les clés).

Le scénario natif de récupération est complété par la restauration du principal à une base contenant seulement le genesis, en conservant le témoin de sa première publication réelle. Il reconstruit le bloc et l’état EVM attendus, puis vérifie encore la récupération du prédécesseur d’une publication d’horloge et le refus d’un rollback plus ancien. Cette version passe en **12,20 s** ; les anciennes preuves sont conservées.

Preuves versionnées : [`qualification/bellatrix-crash-20260926.json`](qualification/bellatrix-crash-20260926.json) et [`qualification/bellatrix-execution-recovery-20260926.json`](qualification/bellatrix-execution-recovery-20260926.json). Journaux bruts dans `build-cmake/qualification/beacon-witness-20260926/`. Les limites restantes sont l’indépendance opérationnelle des témoins, leur propre rollback, la migration des journaux existants, les pannes physiques et le coût des snapshots doublés sous charge. Aucun déploiement VPS ni fonds réel.

### Preuve de double vote jusqu’à la sanction locale — 26 septembre 2026

Le réacteur conserve désormais les preuves BLS d’attester slashing admises, dans un format durable v4. Les formats v1/v2/v3 restent lisibles et leur encodage reste inchangé en l’absence de preuves. La restauration revérifie SSZ, empreintes et signatures. La conservation utilise le budget explicite `equivocations` et le budget d’enveloppe ; les doublons exacts n’avancent pas le checkpoint.

Le constructeur local réserve de la capacité pour ces opérations et les insère avant les attestations, avec validation complète de chaque candidat. Il ignore les preuves dont tous les signataires communs sont déjà sanctionnés dans l’état parent ; les preuves restent conservées pour les branches concurrentes. Les transitions économiques existantes appliquent pénalité et sortie. Référence : [traitement officiel des attester slashings](https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#attester-slashings), avec les paramètres Altair/Bellatrix du profil actif.

Le nouveau scénario `consensus_bellatrix_local_slashing` passe en **6,62 s** : preuve altérée refusée sans publication, preuve réelle conservée, doublon sans nouvelle version, redémarrage puis inclusion par le proposant configuré, indicateur `slashed`, sortie programmée et solde débité. Il utilise une signature malveillante synthétique produite hors du journal, pour vérifier le traitement d’un validateur qui contourne volontairement sa protection locale.

L’encodeur ajouté reproduit exactement **30 preuves officielles SSZ** ; la suite étendue passe en **0,30 s**, avec refus des bornes invalides et propagation de l’annulation. Preuve : [`qualification/bellatrix-local-slashing-20260926.json`](qualification/bellatrix-local-slashing-20260926.json). Reste à raccorder l’inventaire/relais autonome de ces preuves, leur élagage après finalité, les sorties volontaires et les dépôts. Ce résultat local ne clôt pas le registre ouvert ni le cycle réseau des validateurs.

### Inventaire et relais des preuves Bellatrix — 26 septembre 2026

Les méthodes privées `nano_bellatrixSlashingInventory` et `nano_bellatrixSlashing` exportent les preuves archivées sous l’identité complète du réseau. Elles exigent le jeton du superviseur et passent dans la file RPC bornée. Le démon Bellatrix alterne désormais blocs, attestations et preuves avec un seul relais en vol. Le mode Altair conserve son alternance précédente. Un refus explicite de preuve permet de visiter les preuves suivantes puis de réessayer ; une réponse absente n’avance jamais le curseur. Un changement de session réinitialise aussi le curseur des preuves.

Qualification des nouveaux contrôles **2/2 en 5,85 s** : inventaire sans autorisation refusé, export SSZ identique, conservation/reprise/inclusion locale ; relais indépendant du transport avec refus, preuve suivante et réponse perdue. `nano-p2p` compile. Preuve : [`qualification/bellatrix-slashing-relay-20260926.json`](qualification/bellatrix-slashing-relay-20260926.json). La propagation effective d’une preuve entre pairs réels et la régression de débit liée à l’alternance sur trois types ne sont pas encore qualifiées ; aucune réussite de réseau distribuée n’est déduite de cette compilation.

### 2026-09-26 — Preuve de slashing relayée entre deux démons

`consensus_bellatrix_slashing_network` passe en **5,99 s**. Deux cœurs dans le processus de test et deux vrais démons `nano-p2p`, sur un Mac et avec horloge synthétique : le destinataire reçoit et archive les octets signés avant tout bloc contenant la preuve, exclut le signataire du choix de branche, puis accepte le bloc qui applique le slashing. Le validateur synthétique 35 passe de 32 000 000 000 à 31 000 000 000 unités de solde consensus, avec sortie à l'epoch 5 ; les états EVM concordent.

Preuve et empreintes : `docs/qualification/bellatrix-slashing-network-20260926.json`. Ce scénario clôt uniquement le raccordement effectif du relais de preuves. La tenue sous charge et la reprise après partition avec l'ordonnancement désormais partagé entre blocs, votes et preuves restent à qualifier ; l'archive de preuves ne dispose pas encore d'élagage après finalité. Aucune conclusion de préparation générale à la production.

### 2026-09-26 — Saturation par preuves redondantes

La revue du lot slashing a identifié qu'un signataire pouvait fabriquer plusieurs paires de votes contradictoires pour le même fautif et occuper l'archive. L'admission conserve désormais une nouvelle preuve seulement si l'intersection de ses signataires couvre au moins un fautif non couvert par les preuves archivées. Les exclusions de choix de branche ne remplacent pas ce contrôle : elles peuvent exister sans preuve en attente pour le constructeur de blocs.

Deux nouveaux cas de `consensus_bellatrix_local_slashing` passent (**5,53 s**, avec le scénario de sanction/reprise existant) : autre preuve signée du même fautif et paire signée sans fautif commun. Ni archive ni checkpoint ne croissent dans ces cas. Rapport : `docs/qualification/bellatrix-slashing-dedup-20260926.json`. Le coût de vérification BLS reste présent ; ce résultat ne qualifie pas une résistance générale au spam. L'élagage des preuves après sanction finalisée reste ouvert.

### 2026-09-26 — Reprise avec relais blocs/votes/preuves

Le scénario `consensus_bellatrix_partition_recovery` passe en **517,21 s** avec l'ordonnancement du relais modifié : deux cœurs indépendants, deux démons P2P, 64 clés synthétiques réparties entre deux validateurs sur un Mac. Coupure de 96 s, branches concurrentes, finalité inchangée pendant la coupure, puis tête commune et finalité commune progressant à l'epoch 8 (justification 9). Rapport : `docs/qualification/bellatrix-slashing-partition-20260926.json`.

Cette exécution a commencé avant le correctif de déduplication des preuves : ses empreintes d'entrée sont conservées séparément ; le correctif a ses nouveaux cas ciblés propres. Les 517,21 s dépassent le passage historique de 420,77 s, sans mesure permettant d'attribuer l'écart au seul relais. Ce résultat démontre la reprise bornée avec le nouvel ordonnancement, pas la tenue sous charge ni une disponibilité distribuée en production.

### 2026-09-26 — Élagage des preuves après sanction finalisée

Le réacteur conserve les preuves tant qu'au moins un fautif commun n'est pas sanctionné dans l'état finalisé authentifié. À l'avancement de la finalité, il retire atomiquement les preuves devenues inutiles ; les rejeux n'en recréent pas. La récupération restaure son snapshot exact sans y appliquer un nouvel élagage.

Nouveau scénario `consensus_bellatrix_slashing_pruning` : **40,72 s**, deux cœurs dans un processus, deux démons P2P, horloge synthétique. Il vérifie la conservation avant finalité, l'élagage après, le rejeu sans écriture, une nouvelle preuve reçue après disparition de l'ancien curseur de catalogue et la réouverture du stockage. Rapport : `docs/qualification/bellatrix-slashing-pruning-20260926.json`. Cela ferme le manque d'élagage des preuves signalé dans les notes précédentes ; l'historique complet des blocs/états et la charge prolongée restent hors de ce résultat.
