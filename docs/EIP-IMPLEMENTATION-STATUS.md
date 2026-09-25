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
