# Feuille de route EIP — état réel de Nano

Dernière revue : 24 septembre 2026. Source de vérité : code actuel, tests et
rapports. Les anciennes notes chronologiques ne remplacent pas cette matrice.

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
| [3076](https://eips.ethereum.org/EIPS/eip-3076) | Partiel (API/CLI locale v5) | Journal anti-slashing ; import/fusion, export avec arrêt durable et CLI hors ligne mono-validateur ; corpus inter-clients qualifié dans ce périmètre | src/consensus_beacon_signer.cpp ; docs/BEACON-SIGNING-JOURNAL.md | Coordination multi-clés, tâches réelles et récupération des engagements externes après panne ; aucun statut production | Validateurs |
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
| Permutation des comités Ethereum | Primitive vérifiée : 240 vecteurs officiels, invariants de partition jusqu’à 1 million ; RANDAO et graines testés localement ; non activée | Authentification du proposant/registre, persistance du RANDAO, slots/epochs et activation |
| BLS de consensus / agrégation | Partiel : vérification individuelle (29 cas), agrégée (12 cas) et mutations ; registre/consensus non intégrés | Preuves de possession, domaines, signataires, pondération et doubles comptages |
| Domaines et racines SSZ de signature | Primitive vérifiée : 10 vecteurs officiels ForkData/SigningData et mutations de contexte | Intégrer racine authentifiée du registre genesis, versions par epoch et domaines des votes ; pas encore actif |
| Casper FFG / LMD-GHOST | Non commencé ; BFT natif existant | État consensus, justification/finalité, choix de branche et partitions |
| Registre beacon / RANDAO / slashing | Partiel : sanctions Phase0, snapshots/reprise et comptabilité par époque testés localement ; non activés | Registre authentifié, persistance, epochs/RANDAO, pénalités différées et intégration réseau |
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
