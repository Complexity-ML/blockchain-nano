# Feuille de route EIP — état réel de Nano

### Partition Docker Capella qualifiée — 27 septembre 2026

Deux daemons actifs, 32 validateurs chacun : après 96 secondes de coupure réseau demandées, finalité commune d’époque 7 retrouvée en 142,223 secondes. Le défaut de démarrage avec superviseur Linux PID 1 est corrigé. [Preuves, échecs initiaux et limites](qualification/capella-docker-partition-20260927/README.md). Une seule campagne locale ; WAN, partitions asymétriques, adversaires, charge et production complète restent ouverts.

Qualification complémentaire : [panne P2P avec validateurs Capella actifs](qualification/capella-transport-outage-20260927/README.md). Après 96 secondes de suspension des seuls daemons, deux branches distinctes convergent vers une finalité commune d’époque 4. Test local passé ; la partition de paquets avec daemons actifs est qualifiée séparément ci-dessus. La production complète reste ouverte.

### Correction du transport privé — 27 septembre 2026

Le client RPC réutilise désormais ses connexions HTTP sans rejouer les requêtes ambiguës. **Deux runs locaux avec 32 validateurs par nœud atteignent une finalité commune d’époque 2**, dont une confirmation sur binaire propre. 17 tests réseau passent, six scénarios HTTP sont qualifiés et la reprise avec accusé perdu passe. [Preuves et limites](qualification/capella-core-keepalive-20260927/README.md). Les contre-exemples ci-dessous sont historiques ; les partitions asymétriques, le WAN et la qualification de production restent ouverts.

Diagnostic antérieur à la correction du transport, conservé comme contre-exemple : [retards d’admission des votes Capella](qualification/capella-vote-propagation-20260927/README.md). Le même test distribué échoue encore ; des votes distants sont admis jusqu’à 12 slots après leur émission observée. Les préparations différées tracées réussissent. La cause du retard reste à isoler ; aucune clôture de la finalité ni de la production.

### Contre-exemple historique avant correction : finalité Capella répartie — 27 septembre 2026

**ÉCHEC** du test à deux vrais pairs libp2p, chacun avec 32 clés distinctes :
aucun checkpoint finalisé commun d’epoch ≥2 en 210 secondes. Finalité bloquée
à zéro, justification finale 1 contre 2 ; têtes de vote divergentes dès le slot 19.
Les 64 derniers votes observés diffèrent entre nœuds malgré des relais réussis.
[Contre-exemple et traces](qualification/capella-distributed-finality-20260927/README.md).
Cause non établie : priorité au suivi des votes acquittés jusqu’à leur application
différée/choix de branche. Le succès du rattrapage simple ne clôture pas ce blocage.

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
| [4895](https://eips.ethereum.org/EIPS/eip-4895) | Partiel | Retraits EVM/Capella, publication atomique et réseau local qualifiés dans des scénarios bornés ; file durable de changements BLS et sélection locale raccordée au constructeur de propositions | src/engine.cpp ; src/consensus_capella_withdrawals.cpp ; src/consensus_capella_bls_change.cpp ; src/consensus_capella_local_body.cpp ; tests/test_capella_transition.cpp | Reprise Linux/VPS, pruning, réorganisations profondes et charge/WAN des changements BLS ; migration réseau et qualification complète du profil avant activation publique | Consensus cible |
| [4788](https://eips.ethereum.org/EIPS/eip-4788) | Partiel | Appel système côté exécution | src/engine.cpp ; tests/test_beacon.cpp | Provisionnement versionné du contrat (code, nonce, adresse) au genesis si fork actif dès le départ, sinon à la transition ; fourniture authentifiée des racines par le nouveau consensus | Consensus cible |
| [6110](https://eips.ethereum.org/EIPS/eip-6110) | Partiel | Requêtes côté exécution ; cycle consensus non terminé | src/requests.cpp ; src/engine.cpp ; tests/test_requests.cpp | Registre BLS, dépôts, sorties, consolidations et application des requêtes dans le consensus | Consensus cible |
| [7685](https://eips.ethereum.org/EIPS/eip-7685) | Partiel | Requêtes côté exécution ; cycle consensus non terminé | src/requests.cpp ; src/engine.cpp ; tests/test_requests.cpp | Qualifier encodage, ordre par type, omission des requêtes vides et requests_hash ; rejeter les engagements altérés ; appliquer dépôts/sorties/consolidations au consensus et vérifier reprise/rejeu | Consensus cible |
| [7002](https://eips.ethereum.org/EIPS/eip-7002) | Partiel | Requêtes côté exécution ; cycle consensus non terminé | src/requests.cpp ; src/engine.cpp ; tests/test_requests.cpp | Registre BLS, dépôts, sorties, consolidations et application des requêtes dans le consensus | Consensus cible |
| [7251](https://eips.ethereum.org/EIPS/eip-7251) | Partiel | Requêtes côté exécution ; cycle consensus non terminé | src/requests.cpp ; src/engine.cpp ; tests/test_requests.cpp | Registre BLS, dépôts, sorties, consolidations et application des requêtes dans le consensus | Consensus cible |
| [7549](https://eips.ethereum.org/EIPS/eip-7549) | Non commencé côté consensus | Pas de traitement spécifique intégré identifié | Consensus actuel : src/consensus_round.cpp ; src/bft_reactor.cpp | Implémenter sur le socle epochs/attestations/domaines ; vecteurs officiels et tests réseau | Consensus cible |
| [7044](https://eips.ethereum.org/EIPS/eip-7044) | Partiel | Domaine Capella durable qualifié dans le traitement des sorties Deneb ; non activé réseau | include/nano/consensus_exit.hpp ; src/consensus_slashing.cpp ; tests/test_deneb_exit_official.cpp ; denebExitQualification | Raccorder aux blocs et au gossip ; activer un profil réseau cohérent et vérifier sa transition | Consensus cible |
| [7045](https://eips.ethereum.org/EIPS/eip-7045) | Non commencé côté consensus | Pas de traitement spécifique intégré identifié | Consensus actuel : src/consensus_round.cpp ; src/bft_reactor.cpp | Implémenter sur le socle epochs/attestations/domaines ; vecteurs officiels et tests réseau | Consensus cible |
| [7917](https://eips.ethereum.org/EIPS/eip-7917) | Non commencé côté consensus | Pas de traitement spécifique intégré identifié | Consensus actuel : src/consensus_round.cpp ; src/bft_reactor.cpp | Implémenter sur le socle epochs/attestations/domaines ; vecteurs officiels et tests réseau | Consensus cible |
| [8045](https://eips.ethereum.org/EIPS/eip-8045) | Non commencé | Review ; dépend de 7917, filtrage des slashés avant sélection | [Source épinglée et critères](qualification/validator-eip-sources-20260927/README.md) | Prévision stockée complète et stable après slashing ; activation expérimentale explicite | Extension demandée |
| [8068](https://eips.ethereum.org/EIPS/eip-8068) | Non commencé | Draft ; dépend de 7251, hystérésis et seuil temporaire | [Source épinglée et critères](qualification/validator-eip-sources-20260927/README.md) | Migration SSZ, retraits, arrondis et séquences de neutralité qualifiés dans un profil expérimental | Extension demandée |
| [8363](https://eips.ethereum.org/EIPS/eip-8363) | Non commencé | Draft ; destruction de récompenses selon le stake, dépendances fonctionnelles au consensus | [Source épinglée et critères](qualification/validator-eip-sources-20260927/README.md) | Déductions, inactivity leak, transition et comptabilité qualifiées ; paramètres Nano explicites | Extension demandée |
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
| Registre beacon / RANDAO / slashing | Partiel : RANDAO, journal anti-slashing, preuves de sanctions et sorties volontaires signées intégrées aux propositions/archives/P2P Bellatrix ; historiques de dépôts EVM par branche intégrés au Store/checkpoint Bellatrix ; admission et activation avec attestation qualifiées localement sous profil natif explicite ; entrée distante, branches concurrentes et retraits encore ouverts | Cycle ouvert entrées/sorties/sanctions, disponibilité des opérations et reprise après panne à terminer et qualifier |
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

### 2026-09-26 — Sorties volontaires Bellatrix, raccordement local en cours

Admission des sorties signées SSZ de 112 octets, contrôle BLS et ancienneté, déduplication par validateur, archive durable v5 et inclusion par le constructeur local ajoutées. L'admission seule ne modifie pas le registre ; la transition complète du bloc programme la sortie. Le scénario nouveau `consensus_bellatrix_voluntary_exit` passe en **5,50 s** : signature invalide, demande future, ancienneté insuffisante, doublon sans publication et sortie sans slashing. Délai synthétique d'ancienneté : une epoch, explicitement limité au test ; sortie à l'epoch 6, retrait autorisable au plus tôt à 262.

Rapport : `docs/qualification/bellatrix-voluntary-exit-core-20260926.json`. Ce lot reste en cours : transport P2P, reprise des sorties en attente et authentification de leur contexte lors des changements de branche ne sont pas qualifiés. Le versement des retraits n'est pas implémenté par ce parcours Bellatrix ; ne pas assimiler cette étape aux EIP-4895, 7002 ou 7044 activées sur le réseau.

### 2026-09-26 — Sorties volontaires : transport et restauration du contexte

L'archive de sorties conserve désormais la racine du bloc d'admission avec les octets SSZ signés. La restauration vérifie le registre et le domaine de signature à ce bloc authentifié, indépendamment de la tête courante. Inventaire/export RPC authentifiés, transport Bellatrix borné à 112 octets et relais P2P des sorties ajoutés.

`consensus_bellatrix_voluntary_exit_network` passe en **9,54 s** : deux cœurs dans le processus du scénario, deux démons P2P, un hôte, ancienneté synthétique d'une epoch. Réception avant inclusion, calendrier de sortie identique après le bloc, puis réouverture conservant archive, contexte et tête ; doublon sans nouvelle publication. Un premier échec de 28,79 s a révélé une catégorie omise dans le dispatch démon-vers-cœur ; correction et échec sont conservés dans `docs/qualification/bellatrix-voluntary-exit-network-20260926.json`.

Restent avant clôture de ce lot : branches concurrentes, élagage à la finalité, cas négatifs du transport et qualification des composants partagés affectés. Le relais alterne désormais blocs/votes/preuves/sorties ; les preuves de partition précédentes concernent l'ordonnancement antérieur. Pas de versement de retrait ni d'activation d'un fork ultérieur dans ce résultat.

### 2026-09-26 — Sorties volontaires : cycle de demande qualifié et livraison

Le lot couvre admission signée, stockage du contexte d'admission authentifié, transport, inclusion, conservation en cas de réorganisation, puis élagage après programmation finalisée de la sortie. Il ne verse pas de retrait et ne constitue pas le cycle complet des dépôts/activations/retraits du registre ouvert.

Nouveaux résultats :
- `consensus_bellatrix_voluntary_exit_pruning` : **44,13 s**, conservation jusqu'à finalité, élagage, rejeu sans publication, nouvelle sortie relayée après changement de catalogue et réouverture.
- `bellatrix_wire_identity` enrichi : **0,33 s**, sorties de taille exactement 112 octets, rejet des tailles 0/111/113, de l'identité étrangère et du protocole legacy ; enveloppes hex et IPC compact.
- `consensus_bellatrix_voluntary_exit_reorg` : **10,81 s**, deux blocs valides signés par un proposant synthétique équivoque ; après expiration du proposer boost, choix de la branche sans sortie, annulation du calendrier, conservation de la demande et réinclusion ; pair synchronisé et stockage rouvert.
- Régression groupée des composants affectés : **5/5 en 18,68 s** (relais, reçus, client IPC, reprise témoin et slashing local).

Rapports : `docs/qualification/bellatrix-exit-lifecycle-20260926.json`, `bellatrix-exit-reorg-20260926.json` et `bellatrix-exit-shared-20260926.json`. Profils synthétiques sur un seul Mac, ancienneté minimale d'une epoch dans ces scénarios seulement. La réorganisation conserve les mêmes clés de registre : les allocations concurrentes de nouveaux validateurs par dépôts ne sont pas couvertes. La charge et la reprise après partition avec l'ordonnancement à quatre catégories restent à qualifier ; les résultats antérieurs de partition restent ceux de leurs empreintes enregistrées.

### 2026-09-26 — Source des dépôts : replay EVM et identité du contrat

`replay_execution_deposits` rejoue un bloc contre l'état parent vérifié, exige une empreinte de bloc issue d'une autorité indépendante du paquet reçu et une empreinte de code du contrat provenant de la configuration de confiance. Il retourne un état candidat, les reçus et les dépôts ordonnés, sans modifier l'état fourni. Les indices doivent être consécutifs et l'extraction respecte un budget et l'annulation. La fonction utilise actuellement l'adresse du contrat Ethereum ; elle ne doit pas être interprétée comme un client léger authentifiant une autre chaîne.

Nouvelle qualification `consensus_execution_deposit_source` : **35 cas officiels, 680 dépôts, 13 cas sans dépôt, 0,80 s**. Les transactions sont exécutées par l'EVM C++ ; le résultat est comparé au post-état officiel. Pour chaque cas, les vues Solidity `get_deposit_count()` et `get_deposit_root()` concordent avec les données extraites et l'arbre SSZ reconstruit indépendamment ; les montants correspondent à l'augmentation du solde du contrat. Les refus couvrent mauvaises empreintes bloc/code, état parent incohérent, engagement de reçus altéré, indice incorrect, capacité épuisée et annulation. Les cas officiels incluent appels directs/imbriqués, revert, out-of-gas et appels sans dépôt effectif.

Référence : [contrat de dépôt Phase0](https://ethereum.github.io/consensus-specs/specs/phase0/deposit-contract/). Preuve : [`qualification/execution-deposit-source-20260926.json`](qualification/execution-deposit-source-20260926.json). **Statut réseau inchangé : raccordement partiel.** Ce helper n'est pas encore appelé par le runtime Bellatrix. Restent l'historique de dépôts lié aux branches, les preuves Merkle, le vote Eth1 et la consommation par le registre avec PoP, puis activation et restitution. Le profil Prague des fixtures n'active pas Prague dans le réseau Bellatrix ; aucun versement ou dépôt réel n'est effectué.


### 2026-09-26 — Historique de dépôts par branche et preuves vers le registre

`ExecutionDepositHistory` ajoute un arbre immuable de profondeur 32, des preuves Phase0 avec mélange du compteur et une ancre de bloc d'exécution. L'ajout partage les sous-arbres inchangés et copie le chemin de la feuille ; une branche n'avance jamais le compteur d'une autre. Les vues exécutées du contrat (`get_deposit_count`, `get_deposit_root`) doivent correspondre à l'historique avant et après ajout. Les snapshots se reconstruisent seulement contre un état EVM et un bloc authentifiés indépendamment, avec identité de code et capacité explicite ; les données du snapshot ne sont pas leur propre preuve de confiance.

La campagne finale passe **2/2 en 1,13 s**. Nouvelle couverture : toutes les **680 preuves des 35 cas officiels** vérifiées, puis un parcours Paris utilisant de vraies transactions signées et le contrat Solidity : dépôt financé avec PoP BLS vers `process_phase0_deposit`, complément conservant les coordonnées de retrait, nouvelle clé sans PoP ignorée avec consommation d'indice, branches concurrentes et preuves croisées refusées, rejeu sans double crédit, conservation des arbres précédents, bloc sans dépôt et restauration JSON. Modification/réordonnancement des feuilles, ancre étrangère et dépassement de capacité sont refusés.

Preuve : [`qualification/execution-deposit-history-20260926.json`](qualification/execution-deposit-history-20260926.json). Les rapports précédents restent inchangés. **Le raccordement runtime Bellatrix, les votes Eth1, la persistance atomique avec le checkpoint réseau, l'activation et le versement des retraits restent ouverts.** La restauration qualifiée ici est celle du snapshot de l'historique, pas une reprise après crash RocksDB. Ce lot ne clôt pas EIP-6110 réseau et ne mesure pas encore 100 000 ou 1 000 000 validateurs.


### 2026-09-26 — Dépôts raccordés au Store Bellatrix et au checkpoint EVM

Le profil Bellatrix peut désormais inclure l'empreinte du code du contrat de dépôt. Cette extension participe à l'identité du réseau ; le profil et les snapshots historiques restent encodés à l'identique quand elle est absente. Chaque bloc accepté porte son historique immuable de dépôts, construit à partir des reçus déjà vérifiés par la transition complète, sans second replay des transactions. L'ancre exige un historique cohérent avec le contrat (vide par défaut, historique authentifié fourni explicitement pour une ancre plus récente).

L'historique figure dans le snapshot existant du Store, publié par le coordinateur couplé consensus/exécution. La restauration vérifie les données contre l'état EVM et les vues du contrat. **Campagne finale 4/4 en 5,79 s** : nouveau scénario `consensus_bellatrix_deposit_store` (0,80 s), régression de la chaîne signée sans dépôt, sources officielles et historiques/proofs. Le nouveau scénario exécute deux dépôts, accepte une branche concurrente sans le second dépôt, contrôle les compteurs propres aux branches, refuse une altération du snapshot malgré son checksum recalculé et le chargement sous profil historique, puis ferme et rouvre réellement `Blockchain` et le réacteur avec le même checkpoint. Un échec initial de nettoyage provenait de l'ordre de destruction des objets du test ; corrigé, conservé dans les preuves.

Rapport : [`qualification/bellatrix-deposit-store-20260926.json`](qualification/bellatrix-deposit-store-20260926.json). **Restent la sélection/vote Eth1, la fourniture des dépôts obligatoires au proposant local et l'admission/activation automatique du registre.** La réouverture testée est ordonnée : aucune nouvelle preuve de SIGKILL avec dépôts, de réseau multi-hôte ou de charge n'est revendiquée. Les snapshots complets et les historiques répétés dans les enregistrements persistent comme coût de passage à l'échelle. Aucun déploiement ni fonds réel.


### 2026-09-26 — Vote natif et admission automatique par le proposant Bellatrix

L'extension de profil explicite `nano-native-eth1-v1` active la sélection d'une source d'exécution stable pour chaque période de vote : dernier ancêtre précédant le début de période, avec repli sur l'ancre authentifiée. Le nœud refuse tout vote natif ne correspondant pas à l'historique de dépôts d'un ancêtre de son propre parent, avant signature locale et lors de l'admission du bloc. **Cette politique Nano ne reproduit pas la sélection temporelle des blocs Eth1 externes d'Ethereum et ne constitue pas EIP-6110.** Les profils sans dépôts et ceux de suivi seul gardent leurs identités précédentes.

Le constructeur anticipe exactement la majorité stricte du traitement Eth1, réserve le budget des opérations obligatoires et inclut les preuves Merkle ordonnées avant les sorties volontaires. La transition complète conserve le contrôle de possession BLS avant admission d'une nouvelle clé. Aucun crédit de stake n'est effectué par un simple message de pair.

La nouvelle qualification `consensus_bellatrix_native_deposit_admission` passe en **12,42 s** : transaction signée déposée dans la mempool, exécution du contrat, vote puis admission du 65e validateur au slot 12 (période synthétique de huit slots, cinq votes nécessaires), éligibilité à l'activation après traitement d'époque et restauration du registre/curseur après fermeture/réouverture de la base. Racine de dépôt fabriquée refusée avant signature et à réception d'un bloc signé ; preuve obligatoire falsifiée refusée sans appel au signataire ; rejeu du bloc d'admission sans nouvelle publication. Campagne finale **4/4 en 22,04 s**, incluant les propositions historiques, sorties volontaires et stockage des dépôts.

Rapport : [`qualification/bellatrix-native-deposit-20260926.json`](qualification/bellatrix-native-deposit-20260926.json). Restent l'activation effective avec participation de la nouvelle clé, les allocations de registre concurrentes et sorties en attente, le réseau multi-pairs sous ce profil, les pannes et les retraits. Le validateur nouveau n'est pas actif en fin de scénario ; ni EIP-6110 réseau, ni préparation globale à la production ne sont déclarés terminés.


### 2026-09-26 — Activation du validateur déposé et première attestation

`consensus_bellatrix_deposit_activation` prolonge le parcours de dépôt jusqu'à la participation réelle, avec les délais d'activation du profil conservés. Le 65e validateur est éligible après dépôt, reçoit une activation à l'**époque 8**, puis signe une attestation admise et présente dans les messages du choix de branche. Aucun appel au signataire de cette clé n'a lieu avant son activation. La finalité progresse ensuite à l'**époque 7**, après l'entrée en activité. La fermeture et réouverture du nœud conservent le registre, le curseur de dépôt et l'état EVM.

Résultat : **réussi en 139,72 s**, une signature de la nouvelle clé. Les traitements d'activation existants n'ont pas nécessité de modification pour ce scénario. Une observation ponctuelle du processus à 136 s indique 402 080 Kio de RSS et 98,1 % CPU ; ce n'est ni un pic ni un benchmark de capacité. Rapport : [`qualification/bellatrix-deposit-activation-20260926.json`](qualification/bellatrix-deposit-activation-20260926.json).

Limites : un Mac, horloge synthétique, période de vote Eth1 d'une époque, clés du futur validateur préconfigurées directement dans le scheduler. L'entrée via configuration CLI depuis une autre machine et la proposition de bloc par la nouvelle clé restent à qualifier. Les budgets de rétention du test sont augmentés pour ce parcours borné ; cela ne résout pas l'élagage du Store. Restent notamment les allocations concurrentes de nouveaux indices avec sorties en attente, les retraits, le réseau et la charge de production.


### 2026-09-26 — Identité BLS des sorties archivées

Les demandes en attente sont maintenant identifiées par **(indice, clé publique BLS)**. La clé vient du contexte d'admission vérifié ; elle est reconstruite à la restauration, sans accepter un champ de clé supplémentaire depuis un pair ou le snapshot. Déduplication, unicité à la restauration et sélection du proposant utilisent cette identité. Une demande liée à une autre clé est ignorée avant consommation du budget d'essais de proposition. L'élagage conserve les demandes avant finalité ; une sortie correspondante finalisée ou l'attribution finalisée du même indice à une autre clé rend l'ancienne demande obsolète.

La lecture du traitement d'activation confirme que l'éligibilité doit être finalisée avant programmation de l'activation. **Une réorganisation ordinaire changeant la clé d'un validateur déjà actif n'a donc pas été établie comme attaque exploitable.** Le changement explicite l'identité au lieu de dépendre uniquement de cet invariant. Le nouveau contre-exemple utilise deux signatures BLS valides pour des clés différentes au même indice dans des contextes synthétiques ; il ne prétend pas créer deux histoires finalisées admissibles.

Qualification **5/5 en 69,73 s** : nouveau contrôle d'identité, sortie volontaire existante avec cache de clé vérifié, réorganisation/réinclusion, élagage après finalité et parcours de dépôt enrichi d'un refus de sortie signé par une clé non encore activée. Les scénarios réseau existants gardent les mêmes clés entre branches. Rapport : [`qualification/bellatrix-exit-identity-20260926.json`](qualification/bellatrix-exit-identity-20260926.json). Aucun changement du format réseau des sorties, aucun versement de retrait et aucune clôture de préparation générale à la production.


## 2026-09-26 — libsecp256k1 et candidats de retraits Capella

`src/crypto.cpp` délègue désormais dérivation de clé publique, signature RFC6979
low-s et récupération ECDSA à `libsecp256k1` v0.7.1 (source officielle fournie,
SHA-256 épinglé dans `dependencies.lock.json`, licence MIT conservée). Le contexte
est randomisé avant son partage en lecture. Le calcul de signature sur les secrets
avec GMP/OpenSSL est retiré. OpenSSL demeure nécessaire pour TLS, aléa et keystores ;
BLS conserve `blst`. CMake lie la dépendance statiquement ; le Makefile la construit
aussi. Le Dockerfile inclut déjà ces sources via `COPY . .` et sa configuration
CMake ; aucune bibliothèque dynamique supplémentaire n'est nécessaire.

Qualification : 18 vecteurs indépendants ethers 6.16.0, y compris les condensés
supérieurs ou égaux à l'ordre de la courbe ; récupération high-s préservée pour
ECRECOVER, entrées invalides et huit utilisateurs concurrents du contexte.
Les suites core (dont les précompiles et signatures EIP-712), chain, RPC,
authorization, transactions Ethereum et retraits passent : **8/8 en 9,82 s** avec
les deux nouvelles suites. Aucun fork ni statut global de conformité n'est promu.
Docker/VPS non reconstruits ou redéployés sur ce lot. Signature GPG du tag amont
non vérifiée indépendamment ; le hash de l'archive téléchargée est consigné.

Pour EIP-4895/Capella, `consensus_capella_withdrawals` calcule la liste autorisée
(retraits complets/partiels, ordre, adresses, montants, curseurs), débite un candidat
de registre et vérifie le crédit Shanghai dans un candidat EVM. Liste altérée,
double débit, dépassement de curseur, annulation et échec tardif de racine sont
couverts sans mutation des états parents. **Partiel** : la transition SSZ Capella,
l'admission réseau authentifiée, la persistance atomique des curseurs et le passage
du profil Bellatrix/Paris au profil Capella/Shanghai restent à intégrer. Ce module
ne suffit pas à activer les retraits du réseau ; aucun vecteur officiel de consensus
Capella n'est revendiqué dans cette campagne.

Preuves : [rapport](qualification/secp256k1-capella-20260926.json),
`tests/test_secp256k1.cpp`, `tests/fixtures/secp256k1-ethers.json`,
`tests/test_capella_withdrawals.cpp`.
Sources : https://github.com/bitcoin-core/secp256k1/tree/v0.7.1 et
https://ethereum.github.io/consensus-specs/specs/capella/beacon-chain/.


## 2026-09-26 — campagne officielle EEST consume direct

L'outil officiel EEST pilote désormais les runners C++ Nano via un consumer dédié,
sans se présenter comme Geth et sans utiliser un autre moteur. Corpus stable
v5.4.0 épinglé, archive vérifiée contre le digest GitHub publié. Résultats :

| Sélection | Réussis | Échecs / non pris en charge |
| --- | ---: | ---: |
| StateTest Paris | 1 564 | 0 |
| StateTest Shanghai | 1 745 | 0 |
| StateTest Cancun | 16 847 | 0 |
| BlockchainTest Paris | 1 624 | 0 |

Les deux campagnes vont au terme de leur sélection, sans atteindre les seuils
maxfail. Durées pytest : 431,29 s pour les états (deux workers), 171,57 s pour les
blocs (un worker). Quatre contrôles du consumer exigent le rejet d'une racine
fausse, d'un cas absent et d'un fork non implémenté, ainsi que le succès du témoin.
Les verdicts individuels complets sont conservés dans les rapports JSON compressés.

Ce résultat qualifie l'exécution sur ces fixtures, pas l'ensemble du corpus, tous
les forks, toutes les règles d'en-tête ni le consensus réseau. Aucun statut
« terminé pour production » n'en découle. Sur le périmètre observé, aucune
nouvelle divergence EVM n'impose un changement de moteur ; la priorité reste
l'intégration Capella/retraits et du consensus, la rétention du stockage et la
qualification distribuée. Les campagnes des autres profils restent ouvertes.

[Protocole reproductible](EEST.md) ;
[rapport et empreintes](qualification/eest-v5.4.0-20260926.json).


### Limite opérationnelle transverse — allocation RocksDB (2026-09-26)

Le constructeur conserve WAL et `sync=true`, mais désactive la préallocation
`allow_fallocate`. Un comparatif de 66 petites bases reproduit 5,15 Go alloués
avec les défauts contre 2,97 Mo sans préallocation ; les 198 valeurs des trois
variantes survivent à la réouverture. Les tests du journal de signature et des
témoins de récupération passent. Voir
[rapport](qualification/rocksdb-allocation-20260926/README.md).

Cette optimisation n'est ni un pruning de l'état EVM, ni une nouvelle preuve de
conformité EIP. La reprise RPC (~3 s, avec attente volontaire du test) doit rester
distincte de la finalité après crash. Les règles de rétention et l'endurance
restent des critères de clôture séparés.

### Observation Beacon native (2026-09-26, partielle)

Trois lectures HTTP `states/head/{root,fork,finality_checkpoints}` sont raccordées
au runtime Bellatrix avec snapshots validés, file de calcul bornée et accès
loopback. Nouveau test HTTP natif passé, redémarrage runtime passé ; voir
[preuves](qualification/beacon-reads-20260926/README.md) et
[périmètre restant Assertoor](ASSERTOOR-INTEGRATION.md).
Aucune EIP supplémentaire qualifiée par ces lectures ; Assertoor n'a pas encore
été exécuté et les routes genesis/spec/syncing/headers/blocks/events restent ouvertes.

- Extension d'observation du 2026-09-26 : en-têtes Beacon signés par tête/racine/slot
  raccordés aux archives natives. Tests de corruption BLS/SSZ et de cohérence après
  deux changements de tête passés ; [preuves](qualification/beacon-headers-20260926/README.md).
  Ce résultat ne qualifie aucune EIP supplémentaire et ne clôt pas Assertoor.

- Observation Beacon, suite du 2026-09-26 : genesis/version et paramètres issus
  du profil authentifié servis en HTTP natif et testés. La route spec reste
  partielle ; aucun historique Ethereum inventé. Le SDK exact d'Assertoor exige
  NodeSyncing dès son initialisation : télémétrie native encore à raccorder.
  [Preuves et prérequis](qualification/beacon-metadata-20260926/README.md).
  Aucun changement d'état de qualification des EIP ni activation réseau.

- Observation Beacon, suite du 2026-09-26 : NodeSyncing raccordé aux slots annoncés
  par les pairs Bellatrix compatibles via nano-p2p et rapport interne authentifié.
  Tests HTTP (retard, absence, expiration, annonce future) et deux processus avec
  redémarrage passés. [Preuves](qualification/beacon-sync-20260926/README.md).
  Ce statut d'observation n'est ni une preuve de finalité ni une qualification
  Assertoor ; aucune EIP supplémentaire déclarée conforme.

- Observation Beacon, suite du 2026-09-26 : blocs signés Bellatrix exposés en
  JSON/SSZ. SDK exact d'Assertoor exécuté contre Nano ; octets réencodés identiques.
  Huit corps officiels avec opérations non vides passent aussi le round-trip
  externe. [Preuves et limites](qualification/beacon-blocks-20260926/README.md).
  Cela qualifie ces formats, pas une nouvelle EIP ni la suite Assertoor complète.

- 2026-09-26 : événements Beacon `block`/`finalized_checkpoint` publiés après
  commit durable, SSE borné intégré au serveur natif. Cinq tests passent,
  dont réception SDK sur deux processus et validation de 31 événements de
  blocs avec finalité. [Preuves](qualification/beacon-events-20260926/README.md).
  Raccordement Assertoor avancé ; suite complète et qualification production
  toujours ouvertes. Aucun nouveau statut de conformité EIP attribué.

- 2026-09-26 : première exécution réelle d'Assertoor officiel contre deux nœuds
  natifs. Santé EL/CL, progression synchronisée et deux nouveaux corps de blocs
  passent, sans désactivation des contrôles. Cible CTest optionnelle ajoutée.
  [Preuves et limites](qualification/assertoor-native-20260926/README.md).
  Finalité non nulle, partitions et catalogue Assertoor restent à qualifier ;
  aucune EIP ou conformité production supplémentaire déclarée par déduction.

## Historique : échec initial de finalité avec Assertoor — 2026-09-26

La nouvelle cible `consensus_bellatrix_assertoor_finality` répartit les 64 clés
sur deux processus. Santé, synchronisation et propositions passent, mais la
finalité reste à l'époque 0 : timeout après 240 s, campagne 265,84 s. Les reçus
natifs confirment ce résultat ; deux lectures Beacon 503 sont également observées.
Cause non établie. Priorité : contention RPC/consensus et propagation des votes,
avec témoin sans Assertoor ; ne pas assouplir le seuil d'époque finalisée 2.
[Preuves et diagnostic](qualification/assertoor-finality-20260926/README.md).
La finalité sous cette supervision n'est pas qualifiée ; les partitions Assertoor
et la clôture production restent ouvertes.

- 2026-09-26, diagnostic de finalité : le témoin **sans Assertoor** échoue aussi
  à retrouver des têtes communes (253,70 s), malgré un checkpoint finalisé commun
  époque 2 à l'arrêt. Un ordonnancement qui saute les catégories vides corrige
  une limite de capacité du relais, mais ne suffit pas : Assertoor reste en échec
  (269,82 s, finalisée 0). Sorties et slashing passent (10,30/5,33 s).
  [Preuves avant/après](qualification/bellatrix-relay-scheduling-20260926/README.md).
  Résolution toujours ouverte. Conservation automatique des bases et des blocs
  de tête ajoutée pour le prochain diagnostic ; aucun changement de quorum.

- 2026-09-26 — Critère de réussite continue : nouvelle cible
  `consensus_bellatrix_assertoor_sustained`, quatre avancées consécutives de
  finalité commune puis quatre après redémarrage, avec cohérence Beacon/EVM.
  Le diagnostic distingue le défaut JSON/SSZ de la sonde, le retard réel du
  relais et un build sans optimisation. Cache de validation historique ajouté
  sans modification des règles ni des checkpoints ; trois cibles Release
  réussies (9,48 s). Campagne prolongée **réussie en 616,36 s** : quatre
  avancées communes 2→6, puis quatre 7→11 après redémarrage ordonné ; 64
  observations sans écart de tête ou d’époque finalisée. Qualification locale
  bornée en Release, sans conclusion de production ou de tenue indéfinie.
  [Preuves et limites](qualification/assertoor-native-finality-20260926/README.md).

- 2026-09-26 — Qualification de reprise Bellatrix : le nouveau témoin natif
  à livraison directe passe (six avancées 6→12, 122,04 s), mais le scénario
  P2P de partition prolongée reste en échec. Cette différence ne change aucun
  statut EIP en conformité complète ; elle localise la prochaine investigation
  dans l’intégration temps réel et le rattrapage des messages.
  [Résultats et périmètres](qualification/recovery-sustained-20260926/README.md).

### Reprise réseau : état courant après corrections — 2026-09-26

Les échecs ci-dessus restent conservés comme historique. Le relais saute désormais
les votes déjà acquittés avec un budget borné ; le moniteur vérifie la cohérence
des lectures et lie la finalité Beacon réalisée à l’exécution finalisée.

| Scénario | Preuve actuelle | Limite |
| --- | --- | --- |
| Partition P2P de 96 s | Réussite 517,99 s ; reprise 95 s puis trois avancées communes 6→7→8→9 | Deux cœurs, deux P2P, 64 clés synthétiques, un Mac |
| SIGKILL puis redémarrage | Réussite 325,58 s ; trois avancées communes 2→3→4→5 pendant 144,793 s | Arrêt de processus, pas panne électrique |

[Partition](qualification/recovery-sustained-20260926/recovery-bound/README.md) et
[crash](qualification/recovery-sustained-20260926/crash-current/README.md) :
logs, observations et empreintes conservés. Ces résultats ferment ces scénarios
locaux bornés ; aucune EIP supplémentaire ne devient conforme par déduction.
La rétention des états historiques, l’endurance, le multi-hôte et le grand
registre restent ouverts. Le profil réseau reste Bellatrix ; les profils EVM
plus récents du banc ne constituent pas leur activation réseau.

- 2026-09-26 — Primitive d’archive atomique de checkpoint implémentée et testée :
  deux cibles passent en 1,49 s, avec réorganisation, SIGKILL et suffixe WAL
  conservé/perdu. Raccordement des nouveaux blocs signés Bellatrix réalisé :
  neuf archives identiques avant/après réouverture dans le scénario signé.
  **Migration des anciens snapshots, lecture P2P depuis cette archive et
  élagage non réalisés.**
  [Preuves](qualification/checkpoint-archive-20260926/README.md).

## Qualification multi-hôte — 26 septembre 2026

Cœur b7e2748 : Mac natif, Docker ARM64 Mac et Docker AMD64 VPS, deux hôtes
physiques et 64 clés synthétiques 22/21/21. Une fenêtre instrumentée réussit
la finalité commune 2 → 3 → 4 → 5 en 199002 ms. La campagne précédente
échoue au critère de continuité avec 2 → 3 → 4 → 6 ; accord final conservé.
Le retard intermittent d’inclusion reste ouvert. Aucune qualification
d’endurance ou de production : voir [preuves multi-hôte](qualification/multihost-20260926/README.md).

- 2026-09-26, révision 42a3b13 : finalité 2→3→4→5 en 135047 ms,
  mais validateur Docker Mac arrêté par expiration de proposition. Le nœud
  continuait à suivre la chaîne : **qualification globale en échec** malgré
  le code 0 de l'ancien lanceur. Un contrôle des trois statuts validateurs
  rejette maintenant cette campagne et accepte le témoin précédent.
  [Preuves et prochain correctif](qualification/multihost-20260926/with-batch/README.md).

- 2026-09-26 — Dimensionnement de position Uniswap V3 : simulateur entier Python
  comparé au mint des bytecodes officiels sur Nano Paris, deux ordres des actifs.
  Montants, liquidité et tick vérifiés ; aucune qualification du bridge ou de la
  vente publique. [Preuves](qualification/graduation-v3-20260926/README.md).

- 2026-09-26 — Position V3 full range : mint à 100 unités synthétiques puis
  achat/revente du montant reçu sur Nano Paris. Position active conservée,
  soldes et variation de prix vérifiés ; aucun actif réel ni mesure de capacité
  réseau. [Résultats](qualification/graduation-v3-20260926/full-range/README.md).

## Qualification multi-hôte après correction des propositions expirées

Révision b1ccbac : finalité commune 2→3→4→5 en 145763 ms, trois
validateurs sains et actifs (136/133/128 attestations locales admises).
Mac natif + Docker ARM64 + VPS AMD64, deux machines, 64 clés synthétiques.
La campagne précédente avec validateur arrêté reste classée en échec.
Les partitions WAN, l’endurance et le grand registre restent ouverts ; aucun
statut EIP ou production global ne découle de cette fenêtre. [Preuves](qualification/multihost-20260926/with-expiry/README.md).

- Partition WAN du 26 septembre, cœur b1ccbac : **échec de reprise** dans les
  180 s après reconnexion d'une coupure de 96 s. Trois validateurs actifs,
  198/187/185 attestations admises, finalité restée à 2 ; justification 3/3/9
  à l'arrêt. Retard de convergence à diagnostiquer, pas une panne du fournisseur
  de signature observée. [Preuves](qualification/multihost-20260926/wan-partition/README.md).

### Correctifs locaux et nouvelle qualification WAN échouée

- `9e1b904` : expiration normale RANDAO/attestation reprise au slot suivant ;
  engagements durables conservés, erreurs fournisseur fatales. Scénarios locaux
  passés ; [preuves et limites](qualification/multihost-20260926/duty-expiry/README.md).
- `5da8484` : lecture par page de l'inventaire des votes déjà acquittés. Le
  scénario de 70 votes connus puis un vote nouveau passe de 71 à 3 lectures
  du cœur, avec le même budget de parcours et un seul nouvel envoi par tentative.
  [Preuves locales](qualification/multihost-20260926/relay-pages/README.md).

La campagne `5da8484` conserve ces seuils et **échoue** : après la coupure de
96 s, aucune nouvelle finalité commune sous 180 s. Les trois boucles restent
saines ; justification 7/7/7, finalité 2. Le VPS dépasse une fois la durée
du slot (6360 ms). L’asymétrie de visibilité des votes demeure.
[Preuves complètes](qualification/multihost-20260926/wan-pages/README.md).
Ces modifications ne changent pas les règles de fork ni le statut d'une EIP.

### Réception idempotente des votes — qualification locale

Les retransmissions SSZ identiques déjà authentifiées évitent désormais la
revalidation et la sérialisation globale. Signatures altérées, annulation et
réouverture contrôlées ; [preuves locales](qualification/vote-retry-20260927/README.md).
La reprise WAN reste non qualifiée : cette optimisation ne clôt aucun EIP.

La campagne WAN `b2e4ebe` **échoue** également : finalité restée à 2 sous
180 s après reconnexion, trois boucles saines, aucun cycle au-delà du slot.
Au slot 64, visibilité des votes précédents 22/21/64 : le relais sortant et
l’ordre de rattrapage restent à diagnostiquer.
[Preuves](qualification/multihost-20260927/vote-retry/README.md).

### Relais sortant par pair — qualification locale

Une tentative par pair, quatre au total par défaut, configurables via
`NANO_CONSENSUS_RELAY_CONCURRENCY`. Le test de pair lent, saturation et réponse
tardive passe ; le binaire P2P compile. [Preuves](qualification/peer-relay-20260927/README.md).
Effet sur la reprise WAN non encore qualifié ; aucun statut EIP clôturé.

La campagne `c41e7b4` échoue au délai de 180 s, mais une reprise tardive est
observée : finalité 8 commune dans les statuts terminaux et les échantillons
du slot 80 (~184 s après reconnexion). Au slot 72, les trois nœuds connaissent
64 votes précédents. Trois boucles saines. La fenêtre de progression durable
n’a pas été atteinte ; aucun reclassement en réussite.
[Preuves](qualification/multihost-20260927/peer-relay/README.md).

### Reprise WAN suivie de finalité soutenue — périmètre borné

Cœur `c41e7b4`, banc `14f1f15` : coupure 96 s, reprise commune observée
à 186922 ms (**échec du seuil initial 180 s**), puis **8→9→10→11 en 143118 ms**.
Trois boucles saines, checkpoints liés à l’exécution et audit de 207 observations
sans conflit ni régression. Sortie globale 1 conservée. La fenêtre après reprise
est qualifiée dans ce périmètre, pas le délai ni la production globale.
[Preuves](qualification/multihost-20260927/sustained-recovery/README.md).

### Reprise WAN horodatée et correction du parcours d’inventaire

La campagne `80493b7`, banc `398148d`, reproduit le dépassement du délai :
**187542 ms contre 180 s**, puis finalité commune **8→11 en 141066 ms**.
Trois boucles saines ; l’audit de 209 observations ne relève ni conflit par
epoch ni régression. Les traces corrèlent plusieurs votes inclus tard à
17,6–18,2 s entre relais successifs, avec limites d’horloge documentées.
[Campagne et périmètre](qualification/multihost-20260927/timed-recovery/README.md).

La correction `a4bf5e6` conserve une page bornée par pair après accusé et
réduit de 32 à 1 les requêtes d’inventaire pour 32 votes inconnus dans le
nouveau test. Exports et accusés individuels restent obligatoires ; perte
d’accusé, payload indisponible et catalogue renouvelé sont couverts.
L’ancien code échoue au contrôle négatif ; le daemon corrigé compile.
[Preuves locales](qualification/relay-page-cache-20260927/README.md).
Effet WAN encore à mesurer : aucun EIP ni critère de production clôturé.

### Cache des pages : résultat WAN

La campagne `a4bf5e6` échoue encore au seuil de reprise (189200 ms contre
180 s), puis progresse de 8 à 11 en 148066 ms. Au slot 64, seulement 53,1 %
du poids de cible précédente est inclus ; quorum au slot 66. Le gain local
de requêtes ne démontre pas un gain de finalité WAN. Trois boucles saines,
preuves et échec conservés ; aucun statut EIP clôturé.
[Campagne](qualification/multihost-20260927/page-cache-recovery/README.md).

### Possession des votes par le pair émetteur

Après admission durable d’un vote reçu, le relais retient sa possession
par la session émettrice et évite de le lui renvoyer. Rejets, erreurs,
refus de file, autres types et autres sessions sont isolés dans les nouveaux
contrôles. Le relais injecté envoie toujours au pair indépendant.
[Preuves locales](qualification/vote-sender-knowledge-20260927/README.md).
Compilation réussie ; aucun gain WAN ni critère production encore revendiqué.

### Reprise WAN qualifiée avec suppression des renvois

La campagne `c2ec4d7` respecte pour la première fois ce scénario WAN :
reprise commune en **141002 ms** après reconnexion (seuil inchangé 180 s),
puis **7→8→9→10 en 144025 ms**. Trois instances, deux hôtes, 64 validateurs
synthétiques, coupure 96 s ; trois boucles saines, sortie globale 0.
183 observations sans conflit de racine par époque ni régression.
[Preuves](qualification/multihost-20260927/noecho-recovery/README.md).
Les échecs précédents sont conservés. Une campagne ne prouve pas le gain
causal statistique, l'endurance, la charge ou les autres topologies ; aucun
EIP supplémentaire ni la production globale n'est déclaré achevé.

### Capella : changements des clés de retrait qualifiés

`consensus_capella_bls_change` implémente l'opération signée et sa transition
sur état SSZ authentifié : domaine genesis, autorité de la clé de retrait,
mutation atomique des credentials, rejet des doublons et annulation.
14 cas officiels épinglés (7 valides avec état complet identique, 7 invalides),
63 rejets locaux, 7 rejeux et 14 annulations. Contrôle négatif de signature
effectivement détecté ; régression Capella ciblée 3/3.
[Preuves](qualification/capella-bls-change-20260927/README.md).
L'ordre des opérations du bloc, l'activation, le réacteur et le transport
restent à raccorder. EIP-4895 demeure partiel, sans activation réseau.

### Capella : payload SSZ et retraits officiels

Le codec Capella complet est ajouté : offsets canoniques, budgets,
transactions et retraits séparés, racine SSZ du payload et de son header.
30 vecteurs officiels concordent ; 271 rejets locaux, 30 mutations de
retrait et 60 annulations sont vérifiés. Le même décodeur alimente les
54 cas officiels de retraits : 35 états SSZ complets identiques et 19 refus.
[Preuves](qualification/capella-payload-20260927/README.md).
Régression ciblée 5/5. Il reste à composer la transition payload/état EVM,
les slots/epochs, le bloc signé et sa publication durable. Aucun changement
du profil réseau Bellatrix ni clôture EIP-4895.

### Capella : couplage des retraits et de l'exécution Shanghai

Le payload reçu est maintenant relié à un état SSZ authentifié : parent
EVM, timestamp, RANDAO, liste de retraits attendue, débit consensus et
exécution Shanghai sont vérifiés avant retour d'un candidat privé unique.
Le header actualisé conserve les historiques et ajuste leur offset.
[Preuves](qualification/capella-paired-execution-20260927/README.md) :
PUSH0 exécuté, quatre retraits, sept refus dont un paiement excessif
EVM-valide, trois annulations et régression ciblée 6/6.
Le bloc signé complet, slots/epochs Capella, l'activation et le commit
durable restent ouverts ; aucun changement de profil réseau ni clôture EIP.

### Capella : racine du corps de bloc

`capella_beacon_body_root` engage le payload Shanghai et les changements
BLS signés, avec limites et offsets canoniques. 30 racines officielles,
330 refus locaux, 177 mutations et 60 annulations sont qualifiés ; les
cinq suites structurelles ciblées passent.
[Preuves](qualification/capella-body-20260927/README.md).
Il ne s'agit pas encore de la transition signée complète : slots/epochs,
composition, activation et persistance restent requis. Réseau inchangé.

### Capella : slots et époques

Les candidats Capella calculent les racines du bon fork, composent les
opérations d'époque et ajoutent les résumés historiques sans prolonger
l'ancienne liste. Sept scénarios officiels, 100 slots et états SSZ complets
concordants ; rejeu découpé, rollback tardif et annulations qualifiés.
Headers et curseurs non nuls sont conservés. Régression Altair/Bellatrix/
Capella 3/3. [Preuves](qualification/capella-slots-20260927/README.md).
La transition signée complète, l'activation et la publication durable restent
à raccorder. EIP-4895 et le réseau demeurent partiels.

### Capella : composition du bloc signé

La construction et l'admission Capella composent désormais les composants
qualifiés : slots/epochs, signature/proposant, retraits/Shanghai, opérations,
changements BLS, sync aggregate et racine postérieure. Neuf blocs synthétiques
signés franchissent une époque, changent une clé et versent un retrait au
balayage suivant ; 20 refus et 18 annulations conservent les parents.
[Preuves](qualification/capella-signed-transition-20260927/README.md).
Régression des transitions Altair/Bellatrix/Capella 3/3. Ce scénario ne vaut
pas corpus complet Capella. Activation, Store/réacteur, réseau et commit
durable restent ouverts ; aucun EIP ni objectif production n'est clôturé.

### Capella : migration de fork programmée

Migration Bellatrix→Capella authentifiée et calendrier explicite, y compris
frontière sans bloc et entrée au slot exact. Quatorze états officiels SSZ
complets concordent ; dix refus et trois annulations contrôlent les candidats.
[Preuves](qualification/capella-fork-20260927/README.md).
Reste le premier payload Shanghai sur parent Paris, puis la composition
Store/réacteur, l'activation et le commit durable. Le profil réseau ne change
pas ; EIP-4895 et l'objectif production restent partiels.

### Capella : premier payload Shanghai sur parent Paris

Le pont EL autorise maintenant la transition Paris→Shanghai sous contexte
Capella authentifié, tout en rejetant Shanghai→Paris et les profils ultérieurs.
Un état Bellatrix synthétique est migré puis reçoit quatre retraits et une
transaction PUSH0/SSTORE. Refus, annulations et régression 4/4 qualifiés.
[Preuves](qualification/capella-first-payload-20260927/README.md).
Restent le calendrier composé avec l'admission signée, Store/réacteur, commit
durable et qualification réseau. Aucun profil actif ni statut EIP clos ne change.

### Capella : calendrier composé avec les blocs signés

Construction et admission depuis Bellatrix composent désormais migration
programmée et transition Capella complète. Deux nouvelles chaînes de neuf
blocs couvrent le slot exact et trois slots sans bloc à la frontière, avec
parent EL Paris, signatures, retraits et PUSH0/SSTORE. 24 refus et 18 annulations
par nouvelle chaîne ; régression Altair/Bellatrix/Capella/migration 4/4.
[Preuves](qualification/capella-scheduled-block-20260927/README.md).
Restent Store/réacteur, profil durable, commit atomique et qualification réseau.
Les profils actifs et les statuts EIP partiels restent inchangés.

### Shanghai : intégrité et reprise du checkpoint d'exécution

Le commit atomique contrôle maintenant le corps des retraits contre sa racine
et refuse les retraits sans engagement. Qualification Shanghai : réorganisation,
CAS, archives, quatre SIGKILL/réouvertures et faute sync WAL conservée/perdue ;
montants restaurés contrôlés. Régression stockage Paris/Shanghai 4/4.
[Preuves](qualification/shanghai-checkpoint-20260927/README.md).
Le snapshot testé est synthétique : Store Capella, profil durable et réacteur
restent à raccorder. Aucun statut EIP clos ni profil réseau modifié.

### Capella : contextes du choix de branche

Helpers Capella pour registre lié au bloc, checkpoints réalisés/unrealized,
état cible et identité du proposant. Vingt-sept blocs signés contrôlés, six
contextes cibles, refus de headers non liés et de proposants sanctionnés ;
régression ciblée 3/3.
[Preuves](qualification/capella-fork-context-20260927/README.md).
Le Store complet, son snapshot durable et le réacteur réseau restent ouverts.

### Capella : Store de branches en mémoire

Store Capella avec états consensus/EVM couplés, admission signée, horloge et
sélection de tête. Trois chaînes de neuf blocs et trois branches concurrentes
qualifiées, avec refus de signatures/retraits altérés et annulations ; régression
3/3. [Preuves](qualification/capella-store-20260927/README.md).
Qualification spécifique votes/slashing/dépôts, snapshot, conservation des
branches à travers le fork, persistance et réacteur restent ouverts. Le Store
testé démarre sur une ancre Capella authentifiée ; réseau inchangé.

### Capella : snapshot local et restauration

Format local et profil canonique Capella, digest vérifié avant parsing,
registre justifié reconstruit depuis SSZ. Trois chaînes de neuf blocs avec
aller-retour de branches et cache qualifiés ; commit atomique du vrai snapshot
et réouverture RocksDB avec tête/racine EVM concordantes. Régression 3/3.
[Preuves](qualification/capella-snapshot-20260927/README.md).
Restent crash du vrai Store, votes/finalité, branches Bellatrix à travers le
fork et intégration réseau. Checkpoint local authentifié uniquement.

### Capella : crash et reprise du vrai Store

Six SIGKILL/réouvertures sur trois scénarios signés, en alternant les vrais
snapshots d'une chaîne courte et longue avec retrait. Cohérence du token,
du Store restauré, des états EVM et des index canoniques vérifiée ; aucun
commit acquitté perdu dans cette campagne.
[Preuves](qualification/capella-store-crash-20260927/README.md).
Crash de processus uniquement, pas panne électrique. Votes/finalité réseau,
Store vivant traversant le fork et réacteur restent à qualifier/raccorder.

### Capella : votes et réorganisations avec états EVM distincts

Dix-huit votes BLS valides et dix-huit signatures altérées testés, six bascules
de tête/EVM entre deux branches réellement différentes. Trois preuves de
double vote marquent les équivoques ; snapshots avec votes et exclusions
restaurés exactement.
[Preuves](qualification/capella-votes-20260927/README.md).
Qualification d'admission locale et de LMD-GHOST, pas de finalité FFG
multi-époque, de pénalité économique ni de réseau Capella.

### Capella : finalité FFG locale sur plusieurs époques

Deux nouvelles chaînes de 33 blocs avec votes inclus : participation complète,
256 attestations et finalité époque 2 ; contrôle à 50 %, 128 attestations et
aucune progression de justification/finalité. Restauration et crashs conservés ;
régression 3/3. [Preuves](qualification/capella-finality-20260927/README.md).
Preuve locale au Store, pas de réseau multi-hôte. Réacteur, activation du Store
à travers les forks et qualification distribuée restent ouverts.

### Capella : coordinateur de publication durable

Blocs, votes, équivoques et ticks peuvent maintenant publier le Store et le
suffixe EVM dans un commit atomique CAS. Comparaison après événements,
doublons sans réécriture, annulations et redémarrages qualifiés sur cinq
chaînes ; trois suites réussies.
[Preuves](qualification/capella-reactor-20260927/README.md).
Les erreurs disque injectées sont désormais qualifiées localement (voir ci-dessous).
Restent les archives/notifications,
la signature locale, le propriétaire Blockchain et le réseau, ainsi que le
Store vivant traversant les forks. Aucun profil réseau activé.

### Capella : erreur WAL du coordinateur et reprise

Injection EIO pendant la publication d'un bloc signé : cinq accès bloqués après
écriture incertaine. Réouverture avec suffixe conservé ou perdu, snapshots et
index canoniques vérifiés, rejeu identique dans les deux cas. Quatre suites
passent ; le contrôle supplémentaire des index perdus est également vert.
[Preuves et limites](qualification/capella-reactor-sync-20260927/README.md).
Qualification macOS locale seulement ; authentification du token de reprise,
raccordement réseau et migration du Store vivant restent à clôturer.

### Capella : archives atomiques des blocs signés

Le coordinateur conserve désormais le bloc signé exact avec son Store et son
suffixe EVM ; lecture vérifiée SSZ/BLS, doublons sans remplacement. Qualification
locale : 91 archives retrouvées après reprise, branches alternatives conservées,
WAL conservé/perdu cohérent et neuf archives altérées refusées.
[Preuves](qualification/capella-signed-archive-20260927/README.md).
Votes/sorties/preuves de slashing archivés, rétention, notifications, signature
locale, raccordement Blockchain/RPC/P2P et Store inter-forks restent ouverts.
Les checkpoints historiques ne sont pas rétroalimentés en blocs signés.

### Capella : votes gossip durables et application différée

Enveloppe locale versionnée : votes signés et Store engagés atomiquement,
validation SSZ/BLS à la reprise. Les votes du slot courant sont conservés avant
acquittement, appliqués au slot suivant ; doublons sans réécriture et rétention
sur epoch courante/précédente. Dix-huit votes immédiats et douze différés
retrouvés après réouverture ; signatures altérées et entrées dupliquées refusées.
[Preuves](qualification/capella-durable-votes-20260927/README.md).
Restent notamment archives sorties/slashings, file des parents inconnus,
signature locale, notifications, intégration réseau et Store vivant inter-forks.
Aucune activation publique ni conformité réseau complète revendiquée.

### Capella : preuves de double vote durables

Enveloppe locale v2 avec preuves SSZ signées et exclusion fork-choice engagées
atomiquement. Reprise authentifiée, variantes d'un même fautif sans croissance
de l'archive : trois preuves restaurées, dix-huit répétitions sans réécriture,
neuf checkpoints altérés refusés. Quatre suites réussies.
[Preuves](qualification/capella-durable-slashings-20260927/README.md).
Ce n'est pas une sanction financière automatique. Restent inclusion locale dans
les blocs, retrait des preuves consommées finalisées, sorties, signature locale,
notifications, réseau et migration du Store inter-forks.

### Capella : devoirs et signature locale d'attestations

Comités, checkpoint source/cible et domaine dérivés du Store authentifié ;
signature protégée par le journal anti-slashing existant et contrôle d'horloge.
Douze attestations locales comparées, reprise du journal et conflits refusés ;
journaux d'une autre chaîne et résultats expirés rejetés.
[Preuves et correction d'une attente de test](qualification/capella-local-attestation-20260927/README.md).
Restent RANDAO/propositions locales, ordonnanceur, diffusion réseau, exploitation
des journaux et migration inter-forks. Aucune activation réseau Capella.

### Capella : RANDAO et propositions locales protégées

Proposant/domaine dérivés du Store ; corps exécuté avant réservation du header
dans le journal anti-slashing. Quatre suites réussies, 88 révélations RANDAO et
88 blocs signés localement comparés puis reçus sur cinq chaînes. Doubles
propositions après réouverture, corps invalides et signatures expirées refusés.
[Preuves](qualification/capella-local-proposal-20260927/README.md).
Restent assemblage automatique des corps, ordonnanceur, diffusion réseau,
qualification dépôts natifs et première proposition lors de la migration du
Store vivant. Les descendants d'une ancre Capella sont seuls qualifiés ici.

### Capella : assemblage local du corps de bloc

Retraits calculés depuis SSZ authentifié, payload Shanghai exécuté, transactions
sélectionnées par nonce avec budgets explicites ; inclusion bornée des preuves
et votes archivés. Vingt-neuf corps assemblés puis admis sur cinq scénarios,
reprise des cinq blocs supplémentaires et trois fautifs marqués slashed.
Sept suites réussies, dont les contrôles de retraits directement affectés.
[Preuves](qualification/capella-local-body-20260927/README.md).
La finalité produite entièrement par l'assembleur est qualifiée ci-dessous.
Le raccordement mempool propriétaire est qualifié ci-dessous. Restent sorties/changements BLS, contributions sync,
ordonnanceur et réseau.

### Capella : finalité issue de la boucle locale complète

Deux Stores durables indépendants et production entièrement locale : 40 blocs,
272 votes, epoch 3 finalisée. Contrôle avec moitié des identités participantes :
40 blocs, 133 votes, aucune finalité. Stores et journaux réouverts au slot 24,
production poursuivie jusqu'au slot 41 sans divergence.
[Preuves et périmètre](qualification/capella-assembled-finality-20260927/README.md).
Il s'agit de deux coordinateurs dans un processus, sans transport P2P : aucune
qualification distribuée ni activation publique n'est déduite de ces résultats.

### Capella : raccordement au propriétaire d’exécution

Le coordinateur durable publie désormais état, tête, finalité, mempool et événements
via le commit Beacon atomique du propriétaire `Blockchain`. Assemblage depuis
la mempool ; lectures fermées avant attachement et après invalidation/détachement.
Deux propriétaires indépendants : 40 blocs, 272 votes, époque 3 finalisée,
réouverture au slot 24 et contrôles directs des RPC latest/finalized/nonce.
[Preuves et limites](qualification/capella-execution-owner-20260927/README.md).
Le Genesis Shanghai est construit dans le test ; le démarrage public reste Paris.
Restent migration du Store vivant, ordonnanceur/P2P/services Beacon, réorganisations
et pannes d’écriture avec propriétaire, sorties/changements BLS et contributions sync.

### Capella : réorganisations et panne WAL avec propriétaire

Deux nouveaux scénarios qualifient le raccordement `Blockchain` : bascule vers
une branche vide puis retour, réinsertion/retrait de la transaction dans la mempool,
index et reçu RPC cohérents ; erreur fsync injectée, huit accès directs bloqués
et RPC fermé, reprise après conservation ou perte du suffixe WAL puis rejeu.
Quatre suites réussies en 28,78 s, dont deux contrôles existants affectés.
[Preuves](qualification/capella-owner-resilience-20260927/README.md).
Périmètre local synthétique, sans panne électrique physique ni réseau. Le test
fournit le jeton de reprise correspondant à l’issue connue ; l’authentification
indépendante de ce jeton reste requise pour le runtime de production.
Restent migration inter-forks du Store vivant, ordonnanceur/transport Capella,
files de sorties/changements BLS, contributions sync et qualification dépôts natifs.

### Capella : reprise à partir d’un témoin séparé

Le coordinateur peut écrire un journal témoin avant chaque commit principal :
jetons précédent/suivant, snapshot et bloc signé nouvellement archivé. La reprise
accepte exclusivement l’état précédent ou suivant attesté par ce témoin et termine
la publication précédente si nécessaire, archive signée comprise.
Trois cas qualifiés : erreur fsync principale avec suffixe conservé ou perdu,
et erreur fsync du témoin avec intention conservée et principal inchangé.
Témoin vide, signature d’archive corrompue et rollback trop ancien refusés.
Cinq suites réussies en 62,04 s, dont finalité/reprise et réorganisations raccordées.
[Preuves](qualification/capella-witness-recovery-20260927/README.md).
Le témoin reste une source de confiance locale indépendante à provisionner ; deux
répertoires sur le même disque ne protègent pas d’un rollback ou d’une perte commune.
Perte du suffixe du témoin lui-même non qualifiée ici. Raccordement au runtime,
migration du Store vivant, ordonnanceur et P2P Capella restent ouverts.

### Capella : horloge et ordonnanceur des validateurs locaux

Ajout de `CapellaLocalClock` et `CapellaValidatorLoop` : sélection des devoirs depuis
le Store, proposition depuis la mempool, attestation et admission normale des résultats.
Préparation/signature de proposition sérialisées sous le verrou du coordinateur.
Horloge indépendante, regroupement des polls par phase et fermeture de l’exécution
sur discontinuité ; timings d’epoch vérifiés contre le profil de consensus.
Le nouveau scénario produit 40 blocs et 272 votes, finalise l’epoch 3 et reprend
au slot 24 avec témoin et 64 journaux réouverts. Aucun doublon de signature lors
des polls répétés ; annulation et arrêt du worker contrôlés.
[Preuves et limites](qualification/capella-local-scheduler-20260927/README.md).
Il s’agit de l’ordonnanceur local : démarrage CLI/configuration, chargement des clés,
service RPC/Beacon, transport P2P et migration du Store inter-forks restent à raccorder.

### Capella : démarrage local par configuration

`start_capella_runtime` charge profil/snapshot authentifiés, limites, horloge,
clés BLS vérifiées contre le registre, journaux et témoins de reprise ; arrêt propre
avec export des engagements. Seize blocs produits via mempool après chargement de
64 clés synthétiques, avec arrêt/réouverture intermédiaire depuis les témoins du
consensus et des signataires. Quatre configurations invalides refusées.
[Preuves](qualification/capella-configured-runtime-20260927/README.md), nouveau test
réussi en 21,43 s. Aucun changement Bellatrix ni répétition des suites inchangées.
Cette factory C++ ne constitue pas encore le branchement CLI : dispatch CLI,
services RPC/Beacon, transport P2P et migration inter-forks restent ouverts.

### Capella : lectures Beacon authentifiées

Projection des états root/fork/finalité, genesis et paramètres ; en-têtes et blocs
signés JSON/SSZ par tête, finalité, slot ou racine, avec retraits et changements BLS.
Lecture Store/archive sous le même verrou ; blocs non archivés explicitement absents.
Quatorze lots de contrôles sur neuf blocs et six votes/deux réorganisations ;
indicateurs canonical/finalized des branches vérifiés. Handler du runtime vérifié
sur seize blocs/reprise et en-tête finalisé contrôlé après la boucle de quarante blocs.
[Preuves et corrections des essais initiaux](qualification/capella-beacon-reads-20260927/README.md).
Cinq suites distinctes réussies, dont les régressions Beacon Bellatrix.
La lecture Capella est raccordée au runtime C++ ; serveur HTTP/dispatch CLI,
worker d’admission privé, observations de pairs, P2P et migration inter-forks restent ouverts.

### Capella : admission bornée des messages

Service local pour blocs signés, attestations et preuves de slashing, avec identité
liée à la chaîne, au genesis, à l’ancre et au profil complet. File ordonnée bornée
en tâches et octets, annulation avant commit, arrêt avant export des engagements.
Capacités validées avant démarrage des signataires. Dix-sept messages exercés,
deux réorganisations, enveloppes étrangères et saturations rejetées ; runtime
qualifié sur seize blocs avec reprise et cinq configurations invalides refusées.
[Preuves et périmètre](qualification/capella-wire-admission-20260927/README.md).
Deux suites initiales réussies ; seule la suite runtime modifiée a été rejouée.
Il reste à raccorder reçus RPC privés, inventaire/export/synchronisation, serveur
HTTP/CLI et libp2p avec budgets par pair authentifié, puis migration inter-forks.
Ce worker C++ ne constitue pas une qualification réseau ni une activation publique.

### Capella : reçus d’admission privés

Le service expose soumission, consultation, annulation, retrait et rotation de
session sur la même file bornée. Six contrôles ajoutés au test runtime : résultat
commis, répétition identique, conflit de contenu, rejeu retiré, annulation anticipée
et ancienne session. Test réussi en 21,55 s avec seize blocs et reprise.
[Preuves](qualification/capella-admission-receipts-20260927/README.md).
Les reçus sont locaux au processus : perte de session signifie résultat inconnu.
Les points d’entrée C++ restent à exposer par RPC privé authentifié ; inventaire,
export, synchronisation et transport libp2p restent ouverts.

### Capella : export cohérent des archives

Export des blocs signés, votes et preuves de slashing avec identité réseau et
checkpoint lus sous un même verrou ; contrôle facultatif du checkpoint attendu,
limite SSZ et annulation. L’ancre de confiance reste sans signature fabriquée.
Seize messages exportés puis décodés à l’identique après deux réorganisations ;
checkpoint périmé et budget insuffisant refusés. Test enrichi réussi en 5,38 s.
[Preuves](qualification/capella-archive-export-20260927/README.md).
Cette primitive C++ ne clôt pas la synchronisation : pagination d’inventaire,
transport, authentification RPC et budgets réseau restent à raccorder.

### Capella : inventaire paginé lié au checkpoint

Parcours des racines ordonnées avec curseur, nombre d’entrées et taille JSON bornés.
Continuation uniquement avec checkpoint inchangé ; curseurs inconnus refusés.
Seize messages retrouvés sans doublons puis exportés après deux réorganisations,
par pages de deux entrées maximum et 512 octets ; test enrichi réussi en 5,28 s.
[Preuves](qualification/capella-inventory-20260927/README.md).
Les blocs listés sont des candidats connus, dont l’export vérifie la disponibilité
signée. L’ordre par racine n’est pas l’ordre des parents. La pagination sous
activité nécessite encore une stratégie de catalogue stable ; RPC/P2P et
ordonnancement des dépendances restent ouverts avant synchronisation complète.

### Capella : catalogue immuable pendant l’activité

Capture bornée des seules racines avec checkpoint et type de message ; pagination
sans retenir les états EVM ni les payloads. Un catalogue de quatre blocs capturé
au slot 5 reste identique après production supplémentaire et deux réorganisations.
Budget insuffisant et curseur inconnu refusés ; test enrichi réussi en 5,30 s.
[Preuves](qualification/capella-stable-inventory-20260927/README.md).
La pagination locale ne dépend plus des ticks suivants. Restent quotas/expiration
des handles RPC, ordonnancement des parents et transport authentifié. Le catalogue
ne garantit pas la rétention future des payloads après élagage.

### Capella : quotas des sessions d’inventaire

Registre de catalogues avec handles aléatoires, propriétaire, expiration sans
prolongation à la lecture, plafond de handles et budget agrégé des racines.
Lecture/fermeture étrangères refusées ; saturation, libération et expiration
qualifiées dans le test enrichi (5,35 s).
[Preuves](qualification/capella-catalog-sessions-20260927/README.md).
Le futur adaptateur doit fournir le propriétaire authentifié et les budgets
de requête. Raccordement runtime/RPC et synchronisation distribuée restent ouverts.

### Capella : cycle de vie des catalogues dans le runtime

Configuration optionnelle `catalogs` (handles, rootBytes, ttlMs), validée avant
les signataires ; propriétaire partagé avec le coordinateur. L’arrêt invalide
les catalogues et interdit leur réouverture. Runtime : seize blocs avec reprise,
six configurations invalides refusées et trois contrôles de catalogue réussis.
Régression du registre également verte : deux suites en 26,52 s.
[Preuves](qualification/capella-runtime-catalogs-20260927/README.md).
Les services sont raccordés au runtime C++, pas encore aux méthodes HTTP privées
ni au dispatch CLI/P2P. Ces raccordements restent des critères ouverts.

### Capella : méthodes RPC privées raccordées

Injection explicite du runtime dans `Rpc`, exclusive des services Altair/Bellatrix.
Méthodes de statut, reçus, export et catalogues protégées par le jeton privé ;
écoute locale obligatoire via la politique existante du serveur HTTP. Les porteurs
du jeton forment un principal administratif commun, pas des identités P2P distinctes.
Scénario runtime enrichi vert (21,43 s), régression RPC générale verte (6,27 s).
[Preuves et signatures des méthodes](qualification/capella-private-rpc-20260927/README.md).
Qualification limitée au répartiteur `Rpc::handle` : sockets HTTP, dispatch CLI et
synchronisation entre pairs restent à qualifier/raccorder avant activation.

### Capella : qualification HTTP privée

Nouveau scénario `consensus_capella_private_http` : serveur réel sur port loopback
éphémère et client persistant. Écoute publique refusée, mauvais jeton rejeté et
requête d’origine navigateur interdite même avec jeton correct. Soumission/reçu,
export et cycle des catalogues passent par HTTP ; scénario réussi en 17,24 s.
[Preuves](qualification/capella-private-http-20260927/README.md).
Il reste le dispatch CLI, la synchronisation et ses dépendances, puis le transport
libp2p. Ce test local ne prouve pas une qualification distribuée ou sous surcharge.

### Capella : genesis Beacon/Shanghai depuis JSON

Le chargeur accepte maintenant Shanghai explicitement pour Beacon, avec domaine
Capella distinct et engagement de retraits vides. Retraits non vides ou racine
incohérente refusés ; règles Paris inchangées. Les barrières avant vérification du
checkpoint sont testées à l’ouverture et à la réouverture pour les deux profils.
Tests `beacon_execution_gate` et `chain` verts en 4,28 s.
[Preuves](qualification/capella-json-genesis-20260927/README.md).
Ce changement est un prérequis du démarrage CLI : sélection du runtime dans
`run_node`, transport P2P et migration d’une chaîne existante restent ouverts.

### Capella : démarrage du binaire en mode serve

`nano serve` charge le runtime depuis la section `capella` du fichier de service,
exige un jeton privé hexadécimal de 32 octets et l’écoute loopback, puis raccorde
RPC et lectures Beacon. Arrêt HTTP avant export des engagements du runtime.
Nouveau test : deux démarrages réels avec reprise témoin et lectures HTTP, refus
sans jeton et avec écoute publique (1,43 s). Régression CLI existante verte (0,89 s).
[Configuration et preuves](qualification/capella-cli-serve-20260927/README.md).
Ce scénario CLI ne produit pas de blocs et n’utilise pas de clés validateur.
Le mode `run` avec P2P, la synchronisation et la migration inter-forks restent ouverts.

### Capella : rattrapage piloté par inventaire/export

L’export de bloc fournit slot et parent sous le verrou du checkpoint. Nouveau
scénario : second coordinateur et stockage indépendants depuis l’ancre initiale,
inventaire paginé puis seize messages admis normalement. Tête, état EVM et preuves
retenues convergent après les deux réorganisations de la source (test 5,76 s).
[Preuves](qualification/capella-inventory-catchup-20260927/README.md).
Le checkpoint finalisé reste à l’epoch zéro dans ce scénario ; ce n’est pas une
preuve de progression de finalité. La boucle de récupération est un banc de test
local, pas encore le synchroniseur réseau de production.

### Capella : ordonnancement structurel du rattrapage

Primitive C++ bornée : contrôle SSZ, recalcul des racines, refus des doublons et
parents absents, ordre parents avant enfants. Les métadonnées JSON ne déterminent
plus seules l’ordre. Validation BLS/EVM toujours effectuée à l’admission normale.
Quatre contrôles négatifs ajoutés ; rattrapage de seize messages toujours convergent
(test 5,80 s). [Preuves](qualification/capella-sync-order-20260927/README.md).
La recherche réseau des parents manquants et la reprise sur pages élaguées restent
à implémenter ; ce composant n’est pas le synchroniseur réseau complet.

### Capella : récupération bornée des parents

Collecteur itératif depuis les têtes demandées jusqu’aux racines locales connues,
dépendances tirées du SSZ, ancêtres dédupliqués et budgets cumulés. Deux extrémités
de branches suffisent à récupérer neuf blocs en neuf appels, puis l’admission
normale converge avec votes/slashing. Indisponibilité, limite et annulation testées
(5,73 s). [Preuves](qualification/capella-sync-dependencies-20260927/README.md).
Le callback de récupération n’est pas encore un transport libp2p ; sélection des
pairs, délais réseau et reprise après élagage restent à raccorder.

### Capella : récupération de session IPC privée

Le statut publie le profil Capella, les capacités réelles du worker et des messages,
la liaison d’exécution et la session. La machine commune de récupération IPC est
exercée via les RPC Capella : rotation, maintien de highWater=3 et vidage des anciens
jobs. Test runtime passé en 21,57 s ; admission réseau explicitement désactivée.
[Preuves](qualification/capella-ipc-recovery-20260927/README.md).
Le démon libp2p, le relais de catalogues et le démarrage réseau géré restent à
raccorder ; cette preuve locale ne clôture pas la qualification réseau Capella.

### Capella : cycle de vie du nœud géré

`nano run` sélectionne le runtime Capella et ses handlers privés, retire les clés
validateur de l’environnement enfant, arrête les workers et publie le reçu de
reprise. Test du vrai binaire avec enfant de contrôle : trois démarrages, reprise
avec témoins de chaîne/signature, refus de profils concurrents/RPC public et arrêt
sur panne enfant (1,45 s). CLI serve et RPC existantes également vertes.
[Preuves](qualification/capella-managed-node-20260927/README.md).
L’enfant du test n’est pas libp2p : adaptation du démon Capella et relais des
archives/catalogues encore ouverts. Aucun déploiement distribué Capella qualifié.

### Capella : raccordement IPC du véritable démon libp2p

Le démon reconnaît Capella, route les RPC de récupération et conserve l’identité
complète. Les chemins historiques d’import/annonce de blocs sont exclus de ce
profil ; le relais d’archives ancien ne lui est pas appliqué. Deux démarrages réels
`nano` + `nano-p2p` en loopback passent avec reprise persistante et session IPC prête.
[Preuves](qualification/capella-daemon-ipc-20260927/README.md).
Admission Capella toujours désactivée : aucun échange de blocs ni progression de
finalité distribuée prouvés. Relais de catalogues, recherche de parents via pairs
et critères d’activation restent ouverts.

### Capella : relais borné des catalogues

Relais asynchrone de catalogue stable, export unitaire, progression sur accusé
positif, clôture et traitement des réponses tardives. Budgets de pages/entrées et
délai global ; quinze scénarios adverses/succès passent (0,32 s). Intégration via
les vrais RPC et worker : réadmission des archives déjà connues sans changement
du checkpoint, catalogues fermés (21,87 s).
[Preuves](qualification/capella-catalog-relay-20260927/README.md).
L’ordre des racines ne garantit pas l’ordre des parents. Raccordement aux pairs,
collecte des dépendances et reprise après expiration restent requis avant activation.

### Capella : livraison des ancêtres avant les descendants

Préparation bornée depuis l’archive locale jusqu’à l’ancre ou un ancêtre confirmé
par le même pair/session ; livraison séquentielle et cache alimenté uniquement
sur accusé positif. Treize scénarios passent (0,31 s). Le test à deux réacteurs
demande les descendants d’abord : neuf exports/livraisons sans doublons, puis
votes/slashing, avec convergence tête/état/preuves (5,73 s).
[Preuves](qualification/capella-block-delivery-20260927/README.md).
Callbacks encore locaux ; cycle de vie des sessions pairs et raccordement au
démon restent ouverts. Finalité à l’epoch zéro dans cette preuve de rattrapage.

### Capella : rattrapage entre deux vrais pairs libp2p

Le démon relie les catalogues, la livraison des ancêtres et les accusés issus du
worker de consensus. Activation explicite et budgets configurés ; changements de
session/epoch pair invalident les caches et les réponses d’anciennes tentatives.
Deux vrais nœuds en loopback convergent après arrivée tardive, puis après remise
à l’état initial du récepteur conservant son identité P2P et perte volontaire
d’un accusé. Hash de bloc et racine d’état communs sont enregistrés.
[Preuves](qualification/capella-two-peer-network-20260927/README.md).
Validateurs concentrés sur l’émetteur : finalité multi-validateur distribuée,
partitions, WAN, charge durable et longues reprises après élagage restent ouverts.

### 2026-09-27 — Reprise de fermeture des catalogues Capella

Fermeture idempotente authentifiée et une seule reprise sur issue RPC inconnue :
19 scénarios de relais, registre réel avec quotas/expiration, et RPC HTTP local
passent. Aucun changement des règles EIP ou du consensus. L'ouverture dont la
réponse est perdue reste **partielle** : quota conservé jusqu'au TTL ; clôture
requise par identifiant de requête et tests de réordonnancement/annulation.
Pas de qualification production ni de déploiement VPS pour cette tranche.
[Preuves et limites](qualification/capella-catalog-close-20260927/README.md).

### 2026-09-27 — Ouverture de catalogue adressable et annulation préalable

Le démon Capella utilise maintenant un identifiant de requête et une génération
serveur pour libérer une ouverture même sans son accusé. Annulation avant
ouverture, doublons, conflit de contenu, expiration, changement de génération
et redémarrage sont qualifiés localement. Historique des annulations borné par
rotation ; les handles vivants restent consultables et libérables.
20 scénarios de relais, registre réel, RPC et rattrapage à deux processus passent.
L'ancienne ouverture sans identifiant reste disponible et dépend toujours de son
handle/TTL ; le démon ne l'utilise plus. Serveur inaccessible : nettoyage au TTL.
Les effets de rotation sous forte concurrence restent à mesurer. Aucun nouvel
EIP déclaré conforme par cette modification de transport.
[Protocole, preuves et limites](qualification/capella-catalog-acquire-20260927/README.md).

Régression distribuée supplémentaire : deux nœuds locaux × 32 validateurs,
époque 2 finalisée sur une racine commune, trois observations stables.
2 863 relais réussis, 8 échecs fermés ; aucune qualification WAN supplémentaire.

### 2026-09-27 — Concurrence des catalogues : quotas validés, équité ouverte

Nouveau test natif : 32 threads × 32 vagues, budgets séparés de handles/mémoire,
32 ouvertures identiques simultanées et 128 courses ouverture/annulation.
Quotas, absence de résurrection, récupération sans TTL et checkpoint inchangé
validés en 5,92 s. Deux threads sur 32 n'obtiennent aucun accès pendant les vagues
avec capacité 2 : **équité sous contention non démontrée**. Tous récupèrent
ensuite un accès en exécution sérialisée. Ce test du registre ne qualifie pas
l'ordonnancement tournant des pairs du démon, ni un réseau de 32 nœuds.
Aucune nouvelle conformité EIP ni disponibilité production déclarée.
[Mesures, preuve et prochain critère](qualification/capella-catalog-concurrency-20260927/README.md).

### 2026-09-27 — Nettoyage Capella après disparition d'un pair

Défaut de raccordement corrigé : le filtre du démon refusait la nouvelle
libération de catalogue après invalidation du pair. Le filtre partagé autorise
maintenant le nettoyage authentifié, tout en bloquant les nouveaux travaux.
Régression avec filtre/registre/RPC réels : ouverture allouée, pair invalidé,
réponse perdue, libération et quota immédiatement réutilisable. Test HTTP réussi
en 22,78 s ; rattrapage réel à deux processus et reprise après perte d'ACK réussis.
L'équité entre plusieurs pairs concurrents reste à qualifier. Aucun EIP ni
critère de production globale clôturé par cette correction de transport.
[Preuves et limites](qualification/capella-peer-cleanup-20260927/README.md).

### 2026-09-27 — Six nœuds Capella, concurrence et pair non réactif

Qualification du démon réel : un source et cinq followers indépendants via
libp2p. Après un premier relais, le démon d'un follower est suspendu ; pendant
40,10 s, les quatre autres avancent et reçoivent des tours pour les trois
catégories. Tentatives terminées : [5,98,96,96,99], pair suspendu en premier.
Après reprise, les six têtes et racines d'état correspondent. Arrêt propre.
Ce résultat borne un cas d'équité réseau locale ; ni équité générale, ni finalité
avec stake distribué, ni qualification WAN. Le premier essai moins strict est
conservé séparément. Aucun changement du moteur ni nouveau statut EIP.
[Preuves et périmètre](qualification/capella-competing-peers-20260927/README.md).

### 2026-09-27 — Sélection locale des changements BLS de retrait

Nouvelle API du constructeur Capella : candidats signés, budget d'essais,
réservation de place avant transactions, validation complète par ajout. Cas
signature invalide/valide/doublon, limite d'essais, annulation, égalité SSZ avec
construction indépendante, signature et publication testés. Le retrait vers la
nouvelle adresse survient lors d'un balayage ultérieur, jamais dans le bloc du
changement ; deux retraits observés, test final 5,47 s.
**Partiel** : file durable, RPC/gossip et raccordement à `propose_current` restent
à faire ; le scheduler ne fournit pas encore ces opérations. Aucune activation
publique ni clôture complète EIP-4895.
[Code, référence Capella et preuves](qualification/capella-local-bls-changes-20260927/README.md).

### 2026-09-27 — File durable BLS et propositions Capella

La file des changements de credentials BLS est persistée avec le checkpoint,
authentifiée à la reprise, bornée par la configuration et consultée par les
constructeurs ordinaires et `propose_current`. Doublons idempotents, cible
concurrente refusée, admission sans mutation des credentials ; suppression après
finalisation. Qualification : 3 tests de transition/sélection/file passent ;
le nouveau parcours `propose_current` avec deux bases et mempools indépendants
produit 40 blocs et 272 votes, finalise l'époque 3, reprend les deux bases et
64 journaux de signature, verse le retrait et retire la file finalisée (34,56 s).
**Partiel** : entrée RPC/P2P et relais, réorganisations de cette file, pruning de
son contexte d'admission, saturation et crash pendant admission restent à
qualifier. Le test est local, sans transport ni activation publique ; il ne
clôture pas EIP-4895 ou l'objectif production.
[Code et preuves](qualification/capella-queued-bls-changes-20260927/README.md).

### 2026-09-27 — Admission BLS Capella par message et RPC privé

Message `bls_to_execution_change` de 172 octets, activé uniquement sous Capella,
acheminé par la file bornée vers la validation cryptographique et le checkpoint.
Tests : admission native et régression wire (2/2), dispatcher IPC (9 scénarios),
RPC HTTP local réel avec refus du mauvais jeton et admission durable autorisée
(17,17 s). Le daemon compile avec le nouveau type. **Partiel** : export/catalogue
et relais automatique restent à implémenter ; trajet libp2p réel, réorganisations
et pannes de cette file restent à qualifier. Aucun déploiement public.
[Preuves et portée exacte](qualification/capella-bls-admission-20260927/README.md).

### 2026-09-27 — Catalogue BLS et relais libp2p réel

Inventaire trié des changements applicables, catalogue stable et export signé
bornés ; rotation de quatre catégories par pair et contrôle du hash avant relais.
Tests : catalogue natif (7,14 s), 22 scénarios de relais (0,35 s), puis deux vrais
nœuds/daemons libp2p locaux. Une demande déposée uniquement sur le nœud sans
validateur atteint le producteur, est incluse et permet un retrait constaté sur
les deux nœuds. Rattrapage d'un état neuf sous la même identité et récupération
d'une réponse perdue passent aussi. **Partiel** : finalité avec stake distribué,
WAN, réorganisations de la file, saturation, pruning et panne pendant admission
restent hors de cette preuve. Aucun déploiement public.
[Rapport et artefacts](qualification/capella-bls-relay-20260927/README.md).

### 2026-09-27 — Réorganisation et capacité de la file BLS

Nouveau scénario natif (5,90 s) : deux changements de branche par votes signés,
restauration du changement BLS abandonné dans les candidats/catalogue et dans une
proposition valide, deux reconstructions depuis checkpoint, conservation de
l'archive non finalisée. Une file configurée à une entrée refuse une seconde
demande valide sans mutation du checkpoint et conserve l'idempotence du doublon.
Aucun correctif de production nécessaire pour ces cas. **Partiel** : réorganisations
profondes, pruning du contexte, admissions concurrentes et pannes d'écriture
restent à qualifier ; pas de nouvelle preuve réseau ni paiement après réinclusion.
[Scénario, limites et preuves](qualification/capella-bls-reorg-20260927/README.md).

### 2026-09-27 — Erreur de synchronisation pendant admission BLS

Nouveau test macOS (6,21 s), quatre cas : erreur EIO sur WAL principal ou témoin,
avec conservation ou suppression contrôlée du suffixe écrit. Fermeture du réacteur
et des lectures owner/RPC, reprise cohérente, état de consensus inchangé et
resoumission idempotente vérifiés. La demande revient via le témoin lorsqu'il la
conserve ; si le témoin perd sa nouvelle écriture avant publication principale,
la demande doit rester absente jusqu'à resoumission. Aucun correctif du moteur.
**Partiel** : ce n'est pas une coupure physique ni une qualification Linux/VPS ;
pruning, réorganisations profondes et charge restent ouverts.
[Preuves, essai initial et portée](qualification/capella-bls-sync-20260927/README.md).

### 2026-09-27 — Primitive d’élagage Capella et identité réseau stable

Séparation de l’ancre réseau initiale et de l’ancre de stockage, snapshot v2 pour
les états élagués et conservation du format v1 existant. Le candidat pur réduit
41 états de blocs à 18 (1 893 314 → 873 602 octets sérialisés), sans changer la
finalité, les watermarks de votes, l’identité réseau après réouverture ni le
résultat du bloc signé suivant. Test final : 32,88 s ; régression de transition
signée également passée lors du premier essai.
**Partiel** : pas d’élagage automatique en fonctionnement ni de récupération
d’espace RocksDB démontrée. Restent la publication durable, la réconciliation
des archives et opérations en attente, la conservation du contexte des dépôts
natifs, puis les tests prolongés et de panne pendant l’élagage. Aucun EIP clos.
[Code, empreintes et résultats](qualification/capella-pruning-primitive-20260927/README.md).

### 2026-09-27 — Publication durable de l’élagage Capella

API locale `prune_finalized` raccordée au checkpoint du réacteur/owner. Avant
écriture, le candidat doit correspondre exactement à la transformation qualifiée,
puis son enveloppe complète doit passer la validation de reprise (votes, preuves,
changements BLS). Test étendu passé en 34,25 s : annulation sans mutation,
écriture unique, doublon sans écriture, fermeture/réouverture réelle de RocksDB,
votes et identité conservés, même bloc et racine d’état au bloc suivant.
**Partiel** : scheduling automatique, contexte des dépôts natifs, scénarios avec
changements BLS/preuves en attente et pannes pendant l’élagage restent ouverts.
Pas de récupération d’espace disque ni de production démontrée par ce seul test.
[Preuves et portée](qualification/capella-durable-pruning-20260927/README.md).

### 2026-09-27 — Demande BLS en attente lors de l’élagage

Nouveau scénario passé en 34,06 s : le nœud suiveur conserve une demande signée
pendant 40 blocs ; son état d’admission est ensuite supprimé par l’élagage.
Après fermeture/réouverture de RocksDB, les octets signés sont identiques et la
racine d’admission désigne l’état finalisé conservé. Le doublon ne réécrit rien.
Le bloc suivant applique les credentials attendus et donne les mêmes états sur
les stores élagué et non élagué. L’archive non finalisée reste présente.
Aucun correctif du moteur requis. **Partiel** : paiement ultérieur, réorganisation
après élagage, preuves de slashing, dépôts natifs, pannes et déclenchement
automatique restent à qualifier dans ce contexte.
[Scénario et preuves](qualification/capella-pruning-bls-20260927/README.md).

### 2026-09-27 — Préfixes authentifiés d’historique des dépôts

Nouvelle primitive C++ reconstruisant un ancien préfixe de l’arbre des dépôts,
avec partage des sous-arbres et au plus 32 nouveaux nœuds. La racine et le compte
sont vérifiés ; le tuple historique hash d’exécution/compte/racine doit être
préalablement authentifié par l’appelant. Test passé en 0,39 s : quatre préfixes,
six preuves identiques, rejet d’une autre branche et de commitments altérés,
annulation sans mutation. Aucun nouveau profil réseau activé.
**Partiel** : l’élagage natif reste refusé. Il faut conserver les références
d’ancêtres et de périodes, puis raccorder ces préfixes à la sélection des votes
et aux preuves de dépôts obligatoires sans modifier les règles d’admission.
[Code, justification et preuves](qualification/deposit-history-prefix-20260927/README.md).

### 2026-09-27 — Références des dépôts antérieurs à l’ancre Capella

Élagage natif raccordé à la conservation des en-têtes et engagements de dépôts
historiques, sans leurs états EVM/Beacon complets. Lookup des anciens votes,
sélection de période et preuves obligatoires utilisent ces références et les
préfixes authentifiés. Format local v3 ajouté ; v1/v2 conservés.
Tests passés : dix votes de période comparés, preuve obligatoire identique après
deux élagages, rejet d’ancêtres altérés/manquants et d’une branche étrangère ;
régression Capella BLS/élagage également verte (34,02 s).
**Partiel** : le test natif utilise une ascendance Beacon synthétique sur des
blocs d’exécution réellement exécutés. La transition Capella native complète,
la reprise v3, les nouveaux dépôts après élagage et les pannes restent à qualifier.
Les métadonnées d’ancêtres croissent encore avec l’historique ; aucun stockage
indéfiniment borné ni déclenchement automatique n’est démontré.
[Preuves et portée exacte](qualification/capella-native-deposit-pins-20260927/README.md).

### 2026-09-27 — Reprise native Capella du checkpoint v3

Nouveau scénario natif passé en 41,85 s : contrat de dépôt présent au genesis,
40 blocs signés, 272 votes, finalité époque 3, élagage puis vraie réouverture
RocksDB/owner. Le checkpoint passe de 2 528 208 à 1 191 779 octets (41 → 18 états).
Trois snapshots altérés sont refusés malgré recalcul du digest externe.
Un nouveau dépôt signé après élagage produit le même historique et le même état
sur les stores élagué/non élagué, puis survit à une seconde réouverture.
**Partiel** : le registre initial contient 64 validateurs synthétiques ; le test
n’attend pas la consommation du nouveau dépôt par une période de vote complète.
Activation/top-up, pannes pendant élagage, preuves de slashing, scheduling et
croissance prolongée restent ouverts. Aucun espace disque récupéré mesuré.
[Scénario, empreintes et résultat](qualification/capella-native-v3-restart-20260927/README.md).

### 2026-09-27 — Élagage automatique Capella

Le runtime active désormais l’élagage aux ticks ; option locale booléenne
`automaticPruning` (défaut true). Le réacteur attend le contexte d’époque requis
et diffère si un vote signé référence un bloc qui serait supprimé. La publication
utilise la validation complète de reprise et le checkpoint durable existants.
Test natif passé en 67,87 s : 72 blocs, 528 votes, finalité époque 7, six mouvements
d’ancre, maximum 31 états retenus puis 18 finaux ; réouverture des deux owners.
La régression de configuration/runtime passe aussi en 21,75 s.
**Partiel** : ce parcours ne qualifie pas encore le report sur vote de branche
concurrente, les preuves de slashing ou les pannes entre tick et élagage. Les
55 références de dépôts conservées, l’historique EVM et les archives croissent
encore ; aucune borne globale du disque ni qualification WAN/production.
[Implémentation et mesures](qualification/capella-automatic-pruning-20260927/README.md).

### 2026-09-27 — Erreurs de synchronisation pendant l’élagage natif

Nouveau test macOS passé en 67,06 s, quatre cas : erreur EIO du WAL principal
ou témoin, dernière écriture conservée ou supprimée dans une base jetable.
Réacteur/owner/RPC indisponibles après l’erreur ; reprise au checkpoint élagué
si le témoin le conserve, sinon ancien checkpoint jusqu’à la nouvelle tentative.
Votes signés et état EVM préservés, nouvelle tentative idempotente, réouverture
et lecture RPC après reprise réussies. Aucun correctif du moteur nécessaire.
**Partiel** : pas de coupure physique, de qualification Linux/VPS ni d’injection
entre le commit du tick et l’élagage automatique ; archives BLS/slashing en attente
et branches concurrentes restent à qualifier dans ce scénario.
[Quatre cas et preuves](qualification/capella-pruning-sync-20260927/README.md).
