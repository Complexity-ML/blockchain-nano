# EIP et consensus : périmètre demandé pour Nano

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

Liste ajoutée au goal à la demande de l'utilisateur le 24 septembre 2026.
Les statuts Ethereum ci-dessous sont ceux consultés sur eips.ethereum.org à
cette date. Ils ne signifient ni implémentation ni activation dans Nano.
Une proposition non finalisée reste dans le périmètre de travail, avec version
épinglée, tests dédiés et activation distincte du profil stable.

| EIP | Objet | Statut officiel vérifié |
| --- | --- | --- |
| [7251](https://eips.ethereum.org/EIPS/eip-7251) | Solde effectif accru et consolidation des validateurs | Final |
| [7002](https://eips.ethereum.org/EIPS/eip-7002) | Sorties et retraits déclenchés depuis la couche d'exécution | Final |
| [7685](https://eips.ethereum.org/EIPS/eip-7685) | Transport des requêtes exécution vers consensus ; dépendance de 7002/7251 | À suivre dans la spécification épinglée |
| [7917](https://eips.ethereum.org/EIPS/eip-7917) | Prévision déterministe des proposeurs ; dépendance de 8045 | À suivre dans la spécification épinglée |
| [8045](https://eips.ethereum.org/EIPS/eip-8045) | Exclusion des validateurs sanctionnés de la sélection des proposeurs | Review |
| [8068](https://eips.ethereum.org/EIPS/eip-8068) | Neutralité du solde effectif entre capitalisation et prélèvement des gains | Draft |
| [8363](https://eips.ethereum.org/EIPS/eip-8363) | Destruction progressive d'une part des récompenses selon le staking | Draft |
| [7805](https://eips.ethereum.org/EIPS/eip-7805) | FOCIL, listes d'inclusion imposées par le choix de branche | Draft |
| [7594](https://eips.ethereum.org/EIPS/eip-7594) | PeerDAS, disponibilité des blobs par échantillonnage ; dépend de 4844 | Final |
| [7732](https://eips.ethereum.org/EIPS/eip-7732) | ePBS, séparation native proposeur/constructeur | Review |
| [8141](https://eips.ethereum.org/EIPS/eip-8141) | Frame Transaction, évolution du format d'exécution/abstraction de compte | À suivre dans la spécification épinglée |

Le post-quantique est une exigence de recherche et de migration à préciser :
le texte fourni ne donne aucun numéro d'EIP déterminé. Ne pas inventer une
spécification, une date d'activation ou une protection déjà obtenue.

## Prérequis de consensus, au-delà des EIP

Les comités, leur rotation, SSZ, les domaines BLS, RANDAO, le registre,
Casper FFG/LMD-GHOST et le cycle de sanctions/sorties nécessitent une référence
versionnée aux [consensus-specs](https://github.com/ethereum/consensus-specs).
Les EIP seules ne constituent pas une implémentation de consensus.
La transposition des unités ETH en NANO et des paramètres économiques doit
être explicite, déterministe et versionnée, sans conversion implicite.

## Preuves requises avant clôture

Pour chaque élément : dépendances, version amont, différences Nano,
implémentation, vecteurs positifs/négatifs, intégration réseau, activation et
migration documentées. Un test EVM de requête n'établit pas que le registre
consensus applique cette requête. Un statut Final n'établit pas à lui seul la
sécurité du portage. Les versions expérimentales ne doivent pas être présentées
comme un profil Ethereum finalisé ou une production qualifiée.

## Compléments recherchés pour les validateurs

Priorité : compléter le socle de consensus et son cycle de vie avant les
extensions économiques expérimentales. Sources officielles consultées le
24 septembre 2026.

| EIP | Apport et travail à qualifier |
| --- | --- |
| [6110](https://eips.ethereum.org/EIPS/eip-6110) | Dépôts de validateurs fournis par la couche d'exécution ; admission et ordre des requêtes |
| [7549](https://eips.ethereum.org/EIPS/eip-7549) | Agrégation des attestations entre comités ; absence de double comptage et membership |
| [7044](https://eips.ethereum.org/EIPS/eip-7044) | Pérennité des sorties volontaires signées ; domaines de signature et rejeu |
| [7045](https://eips.ethereum.org/EIPS/eip-7045) | Fenêtre d'inclusion des attestations ; slots limites et attestations tardives |
| [4895](https://eips.ethereum.org/EIPS/eip-4895) | Retraits consensus vers exécution ; comptabilité et traitement unique |
| [4788](https://eips.ethereum.org/EIPS/eip-4788) | Racines de blocs beacon accessibles depuis l'EVM ; cohérence EL/CL |

La consolidation 7251 ne réduit pas nécessairement le nombre de machines :
un même opérateur peut déjà héberger plusieurs clés de validation par nœud.
La liste production/exécution est distincte : [EVM-PRODUCTION-EIP-ROADMAP.md](EVM-PRODUCTION-EIP-ROADMAP.md).

## Vérification des participations d'un comité — 24 septembre 2026

`ConsensusCommittee` relie les primitives de dépôt/PoP, domaines de signature
et agrégats BLS. Son constructeur copie un ensemble de membres, rejette indices
et clés dupliqués, poids nuls et preuves de possession invalides. `verify`
exige des indices triés, uniques et présents dans cet ensemble, vérifie la
signature sur la racine avec son domaine fixé puis retourne le poids signé et
le poids total en précision arbitraire. Aucun seuil de finalité n'est déduit.
Référence conceptuelle : [indexed attestations](https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#is_valid_indexed_attestation).

La source d'autorité demeure une précondition : le registre, l'éligibilité,
les poids effectifs, le domaine et le comité du slot doivent provenir de l'état
consensus authentifié. Une preuve de dépôt ne prouve pas les fonds et la racine
d'objet doit engager toutes les données du vote. Cette brique n'est pas encore
appelée par le réacteur BFT et ne remplace pas le cycle d'époques ou la finalité.
Les PoP sont actuellement vérifiées à la construction du snapshot ; l'admission
persistante devra éviter ce travail à chaque comité sans perdre sa garantie.

`committeeParticipationQualification` archive trois vérifications valides,
vingt rejets adversariaux, un total de poids supérieur à uint64 et l'indépendance
du snapshot vis-à-vis des mutations du tableau d'origine. Il s'agit de tests
locaux construits, pas d'une qualification Ethereum complète ou multi-hôte.

## Données d'attestation SSZ — 24 septembre 2026

`ConsensusAttestationData` engage le slot, l'index du comité, la racine du bloc,
les époques et racines des checkpoints source/cible. Sa racine SSZ correspond
aux cinq vecteurs `AttestationData/ssz_random/case_0..4` de la révision
`bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908` de consensus-spec-tests.
Les 35 mutations de champs changent les racines ; le test de comité signe
maintenant cette racine typée et rejette également les sept substitutions.
La nouvelle campagne `consensusAttestationQualification` conserve 27 rejets
BLS/admission en complément des vecteurs SSZ. La campagne précédente à 20
rejets est conservée sans modification.

Le calcul SSZ n'impose pas la fenêtre de slots, la justification/ascendance des
checkpoints ni la règle d'index spécifique au fork (notamment EIP-7549).
Ces contrôles et l'application à la finalité restent à intégrer avec l'état
consensus authentifié ; l'ancien réacteur BFT n'est pas changé par cette brique.

## Contradiction d'attestations — 24 septembre 2026

`is_slashable_attestation_data` reprend le prédicat Ethereum : données distinctes
avec la même époque cible, ou première attestation entourant strictement la
seconde (`source1 < source2` et `target2 < target1`). Le second cas est orienté ;
le traitement des preuves doit respecter cet ordre. Une répétition identique
ne constitue pas un double vote. 24 cas locaux, dont les bornes uint64, passent
avec les régressions SSZ/comité dans `attestationConflictQualification`.
Référence : [is_slashable_attestation_data](https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#is_slashable_attestation_data).

Cela n'applique aucune pénalité : les deux signatures, l'intersection des indices,
l'éligibilité à la sanction, sa comptabilité et sa persistance restent requis.
Les cas de test sont construits localement ; aucune conformité au corpus complet
d'opérations de slashing n'est revendiquée.

## Preuves de slashing authentifiées — 24 septembre 2026

`verify_attester_slashing` relie le prédicat de contradiction aux deux signatures
BLS, avec les snapshots/domaines historiques fournis par l'appelant. Les indices
sont vérifiés, leur intersection est calculée et chaque index commun doit garder
la même clé publique entre les deux contextes. Une intersection vide, une clé
réaffectée, une signature invalide ou des votes non contradictoires sont rejetés.
Trois preuves positives et dix rejets adversariaux passent, avec la régression
comité, dans `authenticatedSlashingQualification`.

L'appelant doit encore fournir les contextes issus d'un état consensus authentifié.
Le résultat identifie des signataires contradictoires ; il ne prouve pas leur
éligibilité actuelle à une sanction et ne modifie aucun solde. Le traitement
exactement une fois, le retrait différé, les règles économiques, la persistance
atomique et l'intégration au réacteur demeurent requis.

## Transition de slashing Phase0 — 24 septembre 2026

`process_phase0_attester_slashing` consomme la preuve authentifiée et un registre
courant fourni par l'appelant. Il vérifie la continuité des clés, filtre les
validateurs non encore sanctionnés entre activation et retrait, programme leur
sortie selon la file et le churn, prolonge le délai de retrait, enregistre le
solde effectif dans le bucket de slashing, puis applique pénalité et récompenses.
Les paramètres économiques sont explicites ; aucun profil réseau n'est activé.
La transition utilise les règles
[Phase0](https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#slash_validator),
pas les règles économiques Electra/Pectra.

Les modifications sont préparées dans une copie puis échangées sans exception.
Preuve invalide, rejeu sans nouveau signataire sanctionnable, état incohérent ou
dépassement uint64 ne doivent pas laisser de modifications partielles. Les
indices de sortie sont regroupés une fois pour éviter de rescanner le registre
pour chaque sanction ; la validation et la copie restent proportionnelles au
registre (avec tri des clés), sans qualification à un million pour ce chemin.

26 scénarios locaux et les régressions preuve/comité passent dans
`slashingTransitionQualification`. Les tests couvrent les limites d'éligibilité,
les récompenses au validateur sanctionné lui-même, plusieurs sorties, les
paramètres alternatifs et le rollback après une première modification préparée.
Ce ne sont pas des vecteurs officiels complets de transition BeaconState.

Restent requis : registre financé et contextes historiques authentifiés,
persistance atomique avec le bloc, rotation des buckets à chaque époque,
pénalités corrélées différées, cycle complet entrées/sorties, profil de fork
choisi et intégration au réacteur. La transition est actuellement en mémoire ;
elle n'est pas raccordée à `NanoStaking.sol` ni au BFT actif.

## Snapshot et reprise du slashing — 24 septembre 2026

Le format local `nano-phase0-slashing-v1` conserve les règles Phase0 attendues,
l'époque, les clés/indices, soldes, dates de sortie/retrait, marqueurs de sanction
et buckets de slashing. Le décodeur vérifie les quantités hexadécimales canoniques
uint64, l'ordre strict des indices, l'unicité des clés, les types et les dates.
Les règles de configuration doivent correspondre exactement à celles fournies
par l'appelant. Ce format JSON local n'est pas une racine SSZ de BeaconState.

`slashingRecoveryQualification` couvre 11 contrôles de roundtrip/reprise et
19 snapshots invalides. Un processus enfant écrit genesis puis un bloc et son
registre sanctionné avec `ChainStore::commit`, puis appelle `_exit(0)` sans fermer
RocksDB. Le parent récupère le tip, l'état EVM et les snapshots historiques,
vérifie les sanctions/récompenses et rejoue la preuve sans double prélèvement.
Un bloc rejeté ne crée pas de snapshot et ce résultat subsiste après réouverture.
L'identité de chaîne incompatible est également rejetée par le stockage.

Le commit utilise le WriteBatch synchrone existant, inchangé. Le test est un
harness de stockage avec des blocs construits localement, sans validation du
consensus PoW/BFT ; il n'active pas le registre dans `Blockchain`. Il ne couvre
ni interruption au milieu d'une écriture, ni panne électrique, ni engagement
cryptographique du registre dans l'en-tête, ni rejeu réseau de ses transitions.
Ces points restent requis avant de revendiquer la persistance du nouveau
consensus actif. Les trois suites preuve/transition/reprise passent.

## Comptabilité du slashing par époque — 24 septembre 2026

`advance_phase0_slashing_epoch` applique la pénalité corrélée différée puis
remet à zéro le bucket de l'époque suivante et avance ce registre d'une seule
époque. L'époque attendue doit correspondre exactement à l'état fourni : le
rejeu et les sauts sont rejetés sans mutation. Les agrégats sont calculés en
entiers GMP ; l'ordre des divisions entières et le plancher du total actif
suivent [process_slashings](https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#slashings).
La pénalité précède l'effacement du bucket, conformément au traitement
[process_slashings_reset](https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#slashings-balances-updates).

Le snapshot local est désormais **`nano-phase0-slashing-v2`** : il ajoute aux
règles l'incrément du solde effectif et le multiplicateur proportionnel. La v1
historique est rejetée, sans migration implicite. Aucun registre de ce format
n'est activé sur le réseau ou dans l'image VPS ; les preuves v1 précédentes
restent des archives de leur révision. Les paramètres ne sont pas des défauts
réseau et doivent provenir de la configuration authentifiée.

`slashingEpochQualification` archive 11 contrôles et 8 rejets atomiques :
parcours depuis une preuve signée jusqu'à la pénalité différée, restauration
snapshot sans répétition, arrondis, montant nul/plafonné, absence de stake actif,
sommes dépassant uint64 et configurations invalides. Les quatre suites
preuve/transition/reprise/époque passent. Il s'agit de cas locaux, pas d'une
qualification complète des vecteurs officiels BeaconState.

Cette fonction traite seulement le registre de slashing. Elle ne remplace pas
le traitement complet des époques : récompenses, hystérésis des soldes effectifs,
entrées/sorties complètes, RANDAO, justification/finalité restent requis.
L'appelant doit ordonner ces étapes selon le fork et persister leur résultat
avec le bloc ; l'horloge de slots et le réacteur actif ne l'appellent pas encore.

## Vecteurs officiels de pénalités Phase0 — 24 septembre 2026

Les cinq cas du groupe `minimal/phase0/epoch_processing/slashings/pyspec_tests`
de [consensus-spec-tests](https://github.com/ethereum/consensus-spec-tests/tree/bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908/tests/minimal/phase0/epoch_processing/slashings/pyspec_tests)
sont importés avec fichiers Snappy originaux, SSZ bruts, licence et empreintes.
Le lecteur C++ du schéma minimal compare les 320 soldes attendus, dont 90
modifiés, et tous les champs de la projection du registre : aucun écart.
Les paramètres minimal proviennent de fichiers consensus-specs eux-mêmes
épinglés et hachés dans le manifeste local.

Le premier essai via la transition complète rejetait ces fixtures : certains
pré-états ont une date de retrait finie et une date de sortie FAR_FUTURE_EPOCH.
Ces états isolent le composant `process_slashings`. Le calcul pur est maintenant
exposé par `phase0_slashing_penalties`, utilisé aussi par la transition validée.
Les attentes officielles ne sont pas réécrites et les contrôles de cohérence
du registre restent dans `advance_phase0_slashing_epoch`. Les pré/post-états
complets d'époque (`pre_epoch`, `post_epoch`) ne sont pas inclus dans cette preuve.

`officialSlashingQualification` conserve le premier rejet, les cinq suites
passantes et les contrôles négatifs : solde attendu altéré avec empreinte mise
à jour, puis rapport avec un solde comparé manquant. Le premier est rejeté par
le binaire C++, le second par le vérificateur de rapport. Le lecteur SSZ est
spécifique au test ; il ne constitue pas le décodeur réseau d'un BeaconState.
La qualification ne couvre pas encore le consensus actif ni un cycle d'époque
complet. Le registre ouvert, RANDAO, finalité et activation restent à intégrer.

## RANDAO authentifié et graines — 24 septembre 2026

`ConsensusRandaoState` conserve l'époque, le ring de mixes et le dernier slot
ayant apporté une révélation. `process_consensus_randao` vérifie une signature
BLS sur la racine SSZ de l'époque, avec DOMAIN_RANDAO et le fork/genesis fournis,
puis applique SHA-256 de la révélation par XOR au mix courant. Les slots répétés,
antérieurs ou hors époque sont rejetés sans mutation. Une signature d'époque
peut être réutilisée à un autre slot si son proposant est autorisé : il ne faut
pas dédupliquer les signatures indépendamment des blocs. Référence :
[process_randao](https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#randao).

La rotation copie le mix courant vers l'époque suivante, avec contrôle de
séquence. `consensus_randao_seed` dérive les graines des devoirs actuels/suivants
à partir du mix retardé, du type de devoir et de l'époque encodée little-endian,
selon [get_seed](https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#get_seed).
Des devoirs historiques demandent leur snapshot historique ; cette API refuse
d'utiliser silencieusement un ring devenu trop récent. Les longueurs de ring,
slots par époque et lookahead doivent provenir du profil authentifié.

`randaoQualification` archive 13 contrôles et 19 rejets, dont quatre valeurs de
graine SHA-256 calculées indépendamment, les bornes uint64, la rotation,
l'isolation des devoirs immédiats et une permutation utilisant la graine obtenue.
Les régressions BLS (29 vecteurs officiels) et racines de signature passent.
Les scénarios RANDAO sont locaux, pas le corpus officiel d'opérations RANDAO.

L'appelant doit encore authentifier le proposant sélectionné et son éligibilité,
le fork et le genesis, valider le bloc, puis persister mix et watermark avec
l'état consensus. Ces fonctions ne sont pas appelées par le réacteur actif et
ne modifient pas `PREVRANDAO` sur le réseau. La persistance du ring, l'horloge des
slots, le registre ouvert et le rattachement aux comités/finalité restent ouverts.

## RANDAO des blocs officiels Phase0 — 24 septembre 2026

`officialRandaoQualification` utilise les scénarios `empty_block_transition`,
`empty_epoch_transition` et `skipped_slots` du corpus
[consensus-spec-tests](https://github.com/ethereum/consensus-spec-tests/tree/bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908/tests/minimal/phase0/sanity/blocks/pyspec_tests)
minimal Phase0. Les trois révélations BLS passent ; les 192 mixes finaux,
l'époque et le dernier slot correspondent aux états post officiels. Quinze
mutations locales de signature, fork/genesis, clé ou répétition de slot sont
rejetées. Les fichiers Snappy originaux et SSZ bruts sont hachés et conservés.
Le lecteur SSZ minimal de test est partagé avec la qualification du slashing.

Les indices des proposants sont lus dans les blocs officiels et leurs clés
dans le pré-état. Leur sélection n'est pas recalculée. Le test n'exécute ni les
autres opérations des blocs, ni leur signature/en-tête, ni la racine complète
du BeaconState. Les cas officiels sont positifs ; les rejets malveillants sont
locaux. La preuve ne doit pas être présentée comme conformité complète des
blocs ou des époques. Les trois suites RANDAO local/officiel et slashing officiel
passent. Un mix post attendu altéré avec empreinte mise à jour est rejeté par
le binaire ; un rapport annonçant 191 mixes au lieu de 192 est aussi rejeté.
La sélection des proposants, la persistance et le consensus actif restent ouverts.

## Sélection pondérée du proposant Phase0 — 24 septembre 2026

`phase0_proposer_index` filtre les validateurs actifs à l'époque demandée,
dérive la graine du slot depuis le RANDAO et applique l'échantillonnage pondéré
Phase0. `shuffled_validator_index` calcule un index sans construire une permutation
complète à chaque essai. La comparaison des poids utilise un intermédiaire
128 bits ; les paramètres du profil sont explicites. Référence :
[compute_proposer_index](https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#compute_proposer_index).

`proposerQualification` recalcule les trois indices des proposants officiels
utilisés par la qualification RANDAO, au lieu de les accepter comme donnée.
Il vérifie 98 comparaisons locales pondérées, 10 rejets, l'annulation et la
correspondance scalaire avec les 240 permutations officielles. Un index de
proposant falsifié dans une fixture est rejeté même après mise à jour de son
empreinte. Les trois suites passent.

La sélection locale sur registres de 100 000 et 1 000 000 entrées, toutes au poids
maximal, a pris respectivement environ 0,53 ms et 3,92 ms sur ce Mac. La mesure
inclut le filtrage du registre, exclut sa construction, la communication réseau,
la validation des signatures et les commits disque. Ce n'est ni un million de
nœuds, ni une preuve de latence sous poids hétérogènes/charge. Le parcours initial
reste O(N) et alloue une liste des actifs ; un registre d'époque authentifié
mis en cache reste à intégrer. Le callback d'annulation interrompt le travail,
mais une interruption ne doit pas devenir un verdict de bloc invalide.

Ce profil Phase0 ne retire pas silencieusement les validateurs slashed du tirage :
la validation du bloc doit encore rejeter un proposant sanctionné selon ce fork.
Les changements Electra et EIP-8045 ne sont pas revendiqués. Le registre fourni
doit être financé et authentifié ; persistance, horloge et réacteur actif ne
sont pas encore raccordés à cette sélection.

## Contrôle commun proposant/RANDAO — 24 septembre 2026

`process_phase0_selected_randao` relie le registre de validateurs, la sélection
pondérée et la révélation BLS. Le registre et le RANDAO doivent désigner la même
époque. La clé publique n'est pas fournie séparément : elle est résolue depuis
l'index sélectionné dans le registre. Le candidat doit être actif et non slashed,
son index doit correspondre au tirage, et la révélation doit passer la vérification
BLS. Le slot rejoué, l'absence de validateur et les révélations malformées sont
rejetés avant mutation. Un proposant sanctionné n'entraîne pas un nouveau tirage.

`selectedRandaoQualification` couvre trois contrôles positifs et douze rejets
locaux. Le cas déterminant utilise une signature d'époque valide d'un autre
validateur inscrit : la primitive BLS/RANDAO seule l'accepte avec sa clé, tandis
que le contrôle commun la refuse pour ce slot. Les erreurs et l'annulation
laissent le mix intact. Les trois projections officielles passent par ce même
contrôle, avec six rejets supplémentaires (mauvais index et proposant slashed).
Les quatre suites passent ; 192 mixes officiels restent identiques.

Pour la fixture qui franchit une époque, les champs du registre utilisés par
le proposant sont vérifiés identiques entre pré/post-états ; le test aligne
ensuite l'époque de la projection. Il ne prétend pas exécuter le cycle complet
de cette époque. Le registre financé/authentifié, l'en-tête, la signature de
bloc, le corps et les engagements d'état restent à valider, puis à committer
avec le mix. Le réacteur actif n'appelle pas encore ce contrôle. Cette jonction
ne vaut donc ni qualification d'un bloc complet, ni activation réseau.

## Engagement SSZ et signature du bloc — 24 septembre 2026

`consensus_beacon_header_root` calcule la racine SSZ du conteneur BeaconBlockHeader
(slot, index du proposant, parent, état, corps), distinct de l'en-tête RLP
exécution de Nano. `verify_consensus_beacon_header` vérifie la signature BLS
avec DOMAIN_BEACON_PROPOSER, version de fork et racine genesis fournis.
La clé doit provenir d'un registre authentifié ; cette primitive ne valide pas
à elle seule le devoir du proposant, le parent ou la transition d'état.

`beaconHeaderQualification` conserve cinq racines officielles et 25 mutations
de champs, trois signatures de blocs officiels et 33 rejets (champs, fork,
genesis, clé, signature altérée/tronquée et substitution par une révélation
RANDAO). Les cinq suites liées passent. Le rapport falsifié et une racine
attendue volontairement incorrecte sont rejetés. Les fixtures proviennent de
`ethereum/consensus-spec-tests` à la révision
`bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908` ; les cinq cas statiques sont
`mainnet/phase0/ssz_static/BeaconBlockHeader/ssz_random/case_0..4`.
Les valeurs importées gardent les chemins et SHA-256 des YAML sources ; la
licence MIT amont est conservée dans `tests/fixtures/consensus-randao-official/LICENSE`.

Limite explicite : pour les trois signatures, la racine du corps provient du
latest_block_header du post-état officiel d'un scénario à un seul bloc. Le test
ne recalcule pas le corps SSZ complet. Il prend la racine d'état dans le message
signé, sans prouver cette transition. Calcul du corps, vérification du parent,
transition complète, jonction atomique avec RANDAO et intégration au réacteur
restent nécessaires. Aucune activation ni qualification complète d'EIP.
Référence : https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#verify_block_signature

## Calcul complet du corps SSZ Phase0 — 24 septembre 2026

`phase0_beacon_body_root` décode strictement le corps SSZ et calcule son
engagement : révélation RANDAO, Eth1Data, graffiti, proposer/attester slashings,
attestations avec bitlists, dépôts avec preuves et sorties signées. Offsets
canoniques, tailles fixes, bornes de listes et délimiteurs de bitlist sont
contrôlés. Les bornes sont fournies par le preset appelant ; elles ne limitent
pas la population totale des validateurs. Le merkle padding n'alloue pas la
capacité maximale configurée. Le transport devra toujours borner les messages
avant ce calcul ; ce composant ne remplace pas les quotas réseau.

`beaconBodyQualification` conserve cinq racines de corps officiels aléatoires
(20 proposer slashings, 6 attester slashings, 24 attestations, 14 dépôts,
21 sorties), 25 mutations de champs fixes, 320 mutations d'octets, 261 rejets de
structures invalides et 13 cas aux limites. Six suites passent. Le décodeur et
le hachage SSZ nouvellement ajoutés sont aussi compilés avec AddressSanitizer
et UndefinedBehaviorSanitizer : même campagne, aucune erreur détectée. Cela
ne constitue pas une qualification de l'ensemble du binaire sous sanitizers.

Les trois signatures de blocs du corpus RANDAO utilisent désormais une racine
de corps calculée depuis les octets reçus, puis comparée au post-état officiel.
Cela lève la limite de la campagne historique `beaconHeaderQualification`,
conservée sans modification. Les cinq fixtures sont épinglées à la même révision
`bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908` du corpus officiel.

Périmètre : schéma Phase0 uniquement, pas les corps Altair/Bellatrix et suivants.
Une opération SSZ bien formée n'est pas nécessairement autorisée : signatures
internes, appartenance, preuves de dépôt, compteurs, transition d'état, parent,
jonction atomique avec RANDAO et activation du réacteur restent à vérifier.
Référence : https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#beaconblockbody

## Préparation authentifiée d'un bloc Phase0 — 24 septembre 2026

`authenticate_phase0_block` relie l'enveloppe SSZ signée, le corps recalculé,
le parent/slot attendus, la signature du proposant issu du registre, son devoir
pondéré et sa révélation RANDAO. Une limite d'octets fournie par la configuration
est vérifiée avant le calcul du corps. Les entrées sont constantes ; un succès
retourne `AuthenticatedPhase0Block` avec un mix candidat, jamais un commit.
Les données malformées ou non autorisées retournent `nullopt`. Les erreurs de
configuration et l'annulation restent des exceptions distinctes.

`blockAuthenticationQualification` : trois blocs officiels épinglés passent,
54 cas invalides et 404 enveloppes tronquées sont rejetés. Quatre contrôles
locaux couvrent le succès, un bloc réellement signé par un autre validateur,
un bloc réellement signé avec révélation invalide et le dépassement de taille.
Sept erreurs de configuration sont détectées. Les 21 points d'annulation
observés sont chacun déclenchés, jusqu'après le calcul du candidat, sans changer
le registre ou le RANDAO source. Six suites passent, dont `block_auth` existante.

Dans les fixtures officielles, le parent attendu vient du tableau block_roots
du post-état après traitement des slots ; ce test ne reconstruit pas encore ce
traitement. Pour le saut d'époque, les champs du registre nécessaires au tirage
sont vérifiés inchangés et la rotation RANDAO est effectuée séparément.
Le contexte, le registre financé, le fork et les limites doivent être fiables.

Ce résultat est une authentification provisoire, pas un bloc valide : signatures
et sémantique des opérations, état résultant, transition de slots/époques,
persistance atomique et raccordement au réacteur restent à intégrer. Le schéma
est Phase0 uniquement. Aucune activation réseau ni finalité nouvelle.

## Double proposition signée et sanction — 24 septembre 2026

`verify_proposer_slashing` authentifie deux en-têtes différents, signés avec la
même clé de registre pour le même slot et le même index. Le fork historique
pour l'époque de la preuve et le genesis sont fournis par le contexte fiable.
`process_phase0_proposer_slashing` vérifie l'éligibilité actuelle et applique
la même transition économique atomique que les preuves d'attestation : pénalité,
file de sortie, retrait différé, bucket et récompenses. Le code économique est
mutualisé ; les régressions d'attestation, reprise et pénalité différée passent.

`proposerSlashingQualification` couvre les quinze cas officiels Phase0 minimal,
révision `bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908` : trois succès et douze
rejets. Pour les succès, la projection complète des registres/buckets et 192
soldes concordent avec les post-états officiels. Le proposant sanctionné peut
être lui-même bénéficiaire de la récompense, conformément à la fixture.
Une paire d'en-têtes d'un slot futur peut être sanctionnable : le test officiel
le demande ; cette vérification n'ajoute pas d'interdiction temporelle arbitraire.

Un des douze rejets concerne un pré-état volontairement incohérent : retrait à
l'époque zéro avec sortie à FAR_FUTURE. Le premier passage du runner échouait
sur `invalid slashing registry` ; le contrôle de production n'a pas été affaibli.
Le test reconnaît explicitement ce cas amont et le compte séparément comme
rejet de registre, pas comme échec cryptographique.

Six rejeux (immédiats et après aller-retour du snapshot) sont refusés sans
nouveau prélèvement ; douze mutations locales de domaine/preuve sont refusées ;
trois débordements de bucket laissent le registre intact. Sept suites passent.
Le snapshot testé ici est un aller-retour du codec ; la reprise RocksDB existante
est une régression séparée. Le traitement des opérations dans le bloc, la
validation de l'état complet et l'activation réseau restent ouverts. Économie
Phase0 uniquement, pas conformité Electra/EIP-7251.
Référence : https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#proposer-slashings

## Sanctions raccordées au bloc signé — 24 septembre 2026

`prepare_phase0_proposer_operations` authentifie le bloc et traite ses proposer
slashings, dans leur ordre, sur des candidats privés de registre/RANDAO. Le
bénéficiaire des récompenses vient du proposant authentifié. Le fork d'état
(previous/current/epoch) sélectionne le domaine des preuves historiques ; le
contexte de signature du bloc doit concorder. Le résultat expose explicitement
les nombres d'attester slashings, attestations, dépôts et sorties non traités.
Il ne doit jamais être publié comme état canonique à ce stade.

`blockOperationsQualification` couvre quatre succès locaux, sept rejets,
douze annulations et un débordement après une première sanction valide.
Un second slashing invalide ou répété invalide le candidat entier, sans publier
la première sanction ni le mix. Le test inclut une ancienne version de fork,
la sanction du proposant du bloc lui-même et une sortie non validée signalée
par le résultat. Les entrées restent inchangées même après succès.

Cinq scénarios officiels de blocs Phase0 minimal épinglés complètent ces tests :
trois succès (cinq sanctions, 192 soldes et RANDAO conformes aux post-états) et
deux rejets de preuves répétées. Ces deux blocs passent d'abord l'authentification
SSZ/BLS et échouent bien au traitement des opérations. Les métadonnées confirment
un seul bloc par scénario. Six suites passent ; les contrôles négatifs du rapport
et du post-état attendu sont archivés.

Limites : ce composant ne traite pas le vote Eth1, les autres opérations,
slots/époques ni la racine d'état complète. Les parents des cas valides sont
comparés aux block_roots officiels ; les cas invalides fournissent le parent du
bloc comme contexte pour isoler le rejet tardif. Le commit durable, les autres
forks et le réacteur restent ouverts. Les copies/scans du registre par preuve
ne constituent pas une qualification de débit à un million de validateurs.

## Sorties volontaires Phase0 — 24 septembre 2026

`process_phase0_voluntary_exit` vérifie le message SSZ signé (époque/index),
DOMAIN_VOLUNTARY_EXIT, la clé du registre, l'activité, l'ancienneté minimale,
l'époque demandée et l'absence de sortie antérieure. Le fork du domaine doit
correspondre à l'époque signée et provenir du contexte fiable. La file de sortie
est partagée avec les sanctions : capacité par époque calculée depuis la
population active, délai d'activation de sortie et retrait différé. La transition
est préparée sur copie puis publiée en mémoire, sans débit ni paiement.

`voluntaryExitQualification` conserve les dix cas officiels épinglés Phase0
minimal : quatre succès, six rejets. Les 320 enregistrements de validateurs et
les buckets concordent avec les post-états officiels ; les soldes restent
inchangés. Les cas valides couvrent la sortie simple, une sortie ultérieure,
le churn minimum et le churn calculé selon le nombre d'actifs. Quatre rejeux,
24 mauvaises signatures/domaines/requêtes et quatre débordements d'époque
sont refusés sans mutation. Sept suites passent, y compris les sanctions et
leurs traitements dans des blocs officiels, après mutualisation de la file.

Ce composant ne réalise ni versement des fonds ni transition complète de bloc.
Intégration aux opérations signées, epoch processing et commit réseau restent
à terminer. Il n'implémente pas encore EIP-7044 (domaine Capella durable) ou
EIP-7002 (demande par clé de retrait) et ne change pas leur statut dans la matrice.
Référence : https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#voluntary-exits

## EIP-7044 : domaine Capella durable — 24 septembre 2026

`process_consensus_voluntary_exit` sélectionne explicitement le profil depuis
un contexte d'état/configuration fiable. Phase0 garde le domaine correspondant
à l'époque signée. Deneb fixe la version à `capella_version` configurée, sans
utiliser les versions previous/current pour signer les sorties. Genesis reste
dans le domaine. Aucun changement du moteur EVM n'est requis par cet EIP.

`denebExitQualification` couvre les seize scénarios officiels Deneb minimal
épinglés : six succès, dix rejets et 448 enregistrements conformes aux post-états.
Deux cas officiels distinguent les profils : une signature Capella reste valide
là où l'ancienne règle sélectionnerait Deneb, tandis qu'une signature Deneb
acceptée par l'ancienne règle doit être refusée. Six changements locaux des deux
versions de fork conservent la validité du domaine Capella. Ils ne simulent pas
une transition complète de fork. Six rejeux, douze mauvais domaines/genesis et
douze contextes inconnus/futurs sont refusés sans mutation. Six suites passent,
y compris les dix fixtures Phase0 via le même sélecteur de profil.

La matrice passe EIP-7044 de non commencé à partiel. Il reste à raccorder ce
sélecteur au traitement des blocs et au gossip, puis à qualifier l'activation
réseau. Le lecteur SSZ Deneb ajouté est uniquement un outil de test des champs
consommés, pas un client Deneb complet. La file actuelle ne prétend pas appliquer
les changements économiques Electra. Sur deux branches partageant genesis et
version Capella, la réutilisation d'une sortie signée reste possible par
conception de l'EIP ; ce n'est pas un transfert des fonds.
Source : https://eips.ethereum.org/EIPS/eip-7044

## Admission par dépôt prouvé — 24 septembre 2026

`process_phase0_deposit` vérifie la racine SSZ DepositData, les 33 niveaux de
preuve Merkle et la position attendue avant admission. La racine/count Eth1
et le registre doivent être authentifiés par l'appelant. Une fausse preuve
est rejetée avant parcours ou copie du registre. Une nouvelle clé exige une
preuve de possession BLS dans le domaine genesis ; un dépôt valide ajoute une
entrée inactive avec solde effectif arrondi/plafonné selon les règles explicites.
La limite de registre vient du preset SSZ, pas d'un plafond de 1 000 validateurs.

Une recharge d'une clé déjà admise modifie seulement son solde : ni les nouveaux
credentials ni une signature de recharge invalide ne remplacent l'identité
existante. Pour une nouvelle clé à signature invalide mais preuve de dépôt
valide, la position est consommée sans créer d'entrée, conformément à Phase0.
Les erreurs de preuve ne consomment rien ; les débordements ne publient aucune
modification. Une admission ne déclenche pas immédiatement l'activation.

`depositTransitionQualification` couvre les dix-sept scénarios officiels épinglés :
six admissions, cinq recharges, quatre signatures ignorées et deux preuves
rejetées. Les 966 enregistrements comparés incluent credentials et époque
d'éligibilité ; root/count/cursor concordent aussi. Quinze snapshots et rejets
de position déjà consommée, 45 mutations de branches/données, onze débordements
et douze snapshots invalides sont vérifiés. Onze suites de régression passent.

Le registre conserve désormais withdrawal_credentials et activation_eligibility_epoch.
Son snapshot local passe à `nano-phase0-slashing-v3` ; v1/v2 sont rejetés, sans
migration automatique ni modification des archives historiques. Les appels à
ce codec ne sont pas raccordés au réacteur actif. `nano-phase0-deposit-v1`
conserve ensemble registre, racine, nombre et compteur de dépôts, avec les règles
liées au snapshot. La régression de reprise RocksDB du registre v3 passe ; les
nouveaux snapshots de dépôt sont qualifiés par aller-retour du codec, pas encore
par une campagne de panne au milieu d'un bloc avec dépôt.

Restent : authentification/évolution de la racine Eth1, traitement des dépôts dans
les blocs, activation/éjection par époque, état complet et commit réseau. Le
mécanisme est Phase0 ; EIP-6110 et les files de dépôts Electra ne sont pas promus.
Source : https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#deposits

## Intégration des opérations du registre dans les blocs — 25 septembre 2026

`prepare_phase0_registry_operations` compose l'authentification du bloc/RANDAO,
les sanctions de proposants, les dépôts ordonnés puis les sorties volontaires.
Un candidat privé contient le registre, la racine/count/cursor des dépôts et le
RANDAO préparé. Aucun état d'entrée n'est modifié, y compris lors d'un échec tardif,
d'un débordement ou d'une annulation. Le nombre de dépôts obligatoire est vérifié ;
les paramètres de la file de sortie doivent être communs aux sanctions et sorties.
Les domaines de sortie Phase0 dépendent de l'époque signée, non du bloc courant.

Le résultat distingue candidat, bloc invalide et opérations non prises en charge.
La présence de sanctions d'attestateurs ou d'attestations empêche de produire un
candidat : leurs effets ne peuvent pas être sautés avant dépôts/sorties. Ce statut
ne signifie pas que le bloc est invalide selon Ethereum.

`registryBlockOperationsQualification` : huit suites passent. Les tests composés
locaux couvrent trois cas positifs, dix rejets, trois combinaisons non prises en
charge, dix-neuf annulations, trois configurations invalides et un débordement
atomique. Un bloc combine sanction, recharge, admission et deux sorties : la file
partagée respecte le churn et l'admission reste inactive. Un dépôt prouvé à PoP
invalide consomme sa position sans admettre de clé et permet les sorties suivantes.

Six scénarios de blocs officiels sont exécutés : trois valides (193 entrées du
registre comparées, RANDAO et compteur de dépôt conformes), trois invalides qui
passent l'authentification puis échouent aux opérations. Deux séquences officielles
supplémentaires sont conservées mais différées explicitement car leur second bloc
traverse une époque : `voluntary_exit`, `multiple_different_validator_exits_same_block`.
Elles ne comptent pas comme réussies.

Limites : racine/count après vote Eth1 fournis par l'appelant, votes Eth1 non traités,
attestations et sanctions d'attestateurs non intégrées, transition d'époque et
state_root complets absents, pas de commit durable du bloc ni raccordement au
réacteur actif. Le pipeline est Phase0, pas Deneb/Electra ; EIP-7044 reste qualifié
séparément. Les copies et parcours par opération ne qualifient pas le débit d'un
registre à un million de validateurs. Clôture : compléter ces étapes, débloquer les
deux séquences d'époque et qualifier le commit/rejeu réseau de l'état complet.
Source : https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#operations

## Registre à la frontière d'époque — 25 septembre 2026

`prepare_phase0_registry_updates` implémente le composant Phase0 de mise à jour
du registre : éligibilité à l'époque suivante pour un solde effectif maximal,
éjection des validateurs actifs au seuil configuré, activation différée des
entrées dont l'éligibilité est finalisée, tri par époque d'éligibilité puis index
et churn dérivé des paramètres et du nombre de validateurs actifs. Le tri partiel
sélectionne les premiers candidats sans plafond arbitraire de taille du registre.
La file de sortie est la même que pour les sanctions et sorties volontaires.
Le composant ne modifie ni les fonds ni le solde effectif.

`registryEpochQualification` : tous les quinze cas officiels Phase0 sont couverts,
quatorze états conformes (1 152 entrées comparées) et un débordement attendu rejeté.
Dix-neuf éligibilités, dix-neuf activations et trente-sept éjections programmées,
quatorze snapshots conformes, soixante-huit annulations sans mutation de la source
et dix rejets locaux de configurations/états/débordements. Neuf suites passent.

Mesure locale bornée du composant sur ce Mac, build Release : 100 000 entrées en
17 078 µs, 1 000 000 en 120 190 µs ; pics RSS du processus respectivement 56 033 280
et 491 880 448 octets. Entrées synthétiques avec clés uniques supposées authentifiées,
moitié actives/moitié en attente ; limites de churn et ordre contrôlés sur toutes
les entrées. Le temps mesure la transition, pas la construction/vérification du
jeu de données. RSS = maximum du processus, pas allocation incrémentale. Une seule
mesure par taille, sans validation BLS ni réseau : zéro nœud distribué, aucun débit
réseau ou capacité de consensus à un million de validateurs n'est démontré.

Le résultat reste un candidat privé à intégrer exactement une fois dans la
transition d'époque complète. L'époque finalisée doit être authentifiée par le
consensus ; elle n'est pas calculée ici. Ni incrément d'époque, marqueur de rejeu,
récompenses/pénalités complètes, mise à jour des soldes effectifs, state_root ni
commit durable ne sont fournis par cette fonction. Les deux séquences multi-époques
précédemment différées restent ouvertes. Profil Phase0 : pas de consolidation
EIP-7251, de churn en solde Electra ni d'activation sur le réseau Nano.
Source : https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#registry-updates

## Justification et finalisation Phase0 — 25 septembre 2026

`prepare_phase0_justification_finalization` calcule les checkpoints justifiés et
finalisés et les quatre bits de justification depuis l'état authentifié. Il
reconstruit les comités des époques précédente/courante depuis le registre actif,
les mixes RANDAO et les paramètres de shuffle ; les bitlists désignent les
participants dans ces comités. Les cibles ne correspondant pas à l'historique sont
ignorées, les participants répétés réunis sans compter deux fois leur poids et
les validateurs sanctionnés exclus du poids attestant. Le dénominateur utilise
les validateurs actifs de l'époque courante, y compris les sanctionnés, comme
Phase0 ; les attestations de l'époque précédente peuvent inclure des sortants.
Les poids utilisent des entiers larges et le seuil exact 3 × participation ≥
2 × solde actif. Les quatre règles de finalisation et les deux époques initiales
sont traitées ; aucun candidat ne modifie ses snapshots d'entrée.

`phase0FfgQualification` couvre les dix scénarios officiels du corpus épinglé :
173 attestations en attente, checkpoints et bits conformes dans tous les cas,
y compris soutien insuffisant, mauvaise cible et validateurs sortants. Contrôles
supplémentaires : dix duplications, dix exclusions des sanctionnés, dix mutations
de cibles, quatre contrôles de seuil/genesis (dont sommes au-delà de uint64),
dix-sept contextes invalides et 226 annulations sans mutation. Sept suites passent
avec les composants de shuffle, RANDAO, sélection du proposant et registre d'époque.

Frontière de confiance : les attestations en attente proviennent de l'état déjà
authentifié ; elles ne portent plus de signatures et ne sont pas une entrée réseau.
La validation BLS, source et inclusion des attestations dans les blocs reste à
raccorder avant ce composant. Finalité réseau, LMD-GHOST, transition d'époque
complète, state_root, garde de rejeu et commit durable ne sont pas démontrés.
Le composant ne remplace pas le BFT actif. Les parcours/shuffles n'ont pas fait
l'objet d'une mesure de finalité à un million de validateurs dans cette campagne.
Clôture : intégrer l'admission authentifiée, l'ordre d'époque et la persistance,
puis confronter des séquences complètes et des partitions/reprises réseau.
Source : https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#justification-and-finalization

## Admission des attestations et composition des blocs — 25 septembre 2026

`process_phase0_attestation` vérifie époque/slot, délai d'inclusion Phase0,
checkpoint source, index/bitlist du comité, présence de participants et signature
BLS agrégée dans le domaine du fork de l'époque cible et du genesis. Les clés
viennent du registre authentifié, dont financement et PoP (y compris genesis)
doivent avoir été établis lors de l'admission. `phase0_attestation_duties` est
partagé avec FFG : registre contigu, clés uniques, comités dérivés de RANDAO.
L'attestation n'apporte ni sa liste de validateurs ni leurs poids.

Seule une admission réussie ajoute à la liste précédente/courante ; le délai et
l'index du proposant sélectionné sont conservés pour les récompenses d'époque.
Les participants sanctionnés peuvent signer une inclusion valide, mais FFG exclut
leur poids. Une mauvaise cible/tête reste admissible si signée : c'est le filtrage
FFG qui écarte les cibles non concordantes. Les doublons sont admissibles dans la
liste, sans compter deux fois leur poids dans FFG. Limite de liste issue du preset.

`prepare_phase0_registry_operations` accepte maintenant un `Phase0AttestationStage` :
après authentification/RANDAO et sanctions des proposants, les attestations sont
vérifiées sur des candidats privés avant dépôts et sorties. Une signature invalide
ou une sortie invalide tardive ne publie ni attestation ni modification du registre.
Sans ce contexte, les attestations restent explicitement non prises en charge ;
les sanctions d'attestateurs restent non prises en charge dans tous les cas.

`attestationInclusionQualification` : 41 scénarios officiels, 17 états conformes et
24 rejets attendus ; 85 mutations rejetées, 17 répétitions d'inclusion conformes,
17 limites de listes, 70 annulations et 8 contextes invalides. Un bloc signé local
combine sanction/attestation/dépôts/sorties, avec deux échecs, 29 annulations et
deux configurations invalides (y compris bloc vide). Sept suites passent, dont
les dix cas officiels FFG après partage du code de comités et ajout des métadonnées.

Les séquences officielles `attestation` et `duplicate_attestation_same_block`
(slots 9 et 17) sont conservées entièrement mais explicitement différées : elles
traversent une époque et ne comptent pas comme blocs conformes. Restent les
sanctions d'attestateurs, votes Eth1, récompenses/autres opérations d'époque,
state_root et commit/reprise du bloc complet, puis raccordement au réacteur.
La reconstruction des comités/proposants par attestation n'est pas encore optimisée
par un cache d'époque ni qualifiée à un million de validateurs. Profil Phase0,
aucune activation Deneb/Electra ou finalité réseau n'est annoncée.
Source : https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#attestations

## Sanctions d'attestateurs et cinq types d'opérations — 25 septembre 2026

`process_phase0_registry_attester_slashing` authentifie les deux attestations
indexées depuis les clés du registre financé/PoP préalablement validé. Les indices
sont non vides, triés, uniques, présents et bornés par le preset SSZ ; chaque
signature utilise le fork de son époque cible. Les fenêtres d'inclusion et
l'appartenance au comité courant ne sont pas des conditions de cette preuve de
faute. Les preuves anciennes/futures restent traitées selon Phase0. Seuls les
signataires communs encore sanctionnables sont punis, dans l'ordre des indices,
avec file de sortie, pénalité, récompenses et registre des sanctions partagés.
Annulation, débordement et erreur ne publient aucune sanction partielle.

Une distinction explicite conserve la validation de l'état :
`phase0_attester_slashing_component` calcule la transition isolée sans valider la
chronologie globale. Le scénario officiel valide `already_exited_long_ago`
utilise des retraits à l'époque 2 et des sorties aux époques 5/6. Son résultat
isolé concorde, tandis que le chemin contrôlé le rejette. Le traitement des blocs
appelle exclusivement le chemin contrôlé ; aucune option de relâchement n'y est
exposée. Les contrôles de chronologie des snapshots/autres sanctions sont conservés.

`attesterSlashingQualification` : trente cas officiels isolés (neuf états conformes,
21 preuves invalides), 576 entrées comparées et 37 validateurs sanctionnés. Huit
cas valides concordent aussi sur le chemin contrôlé ; le neuvième est le cas
synthétique rejeté décrit ci-dessus. Seize rejets de rejeu avant/après snapshot,
48 mutations, huit débordements et vingt annulations sont vérifiés.

Dans les blocs, les sanctions d'attestateurs sont exécutées après les sanctions
de proposants et avant les attestations/dépôts/sorties. Quatre blocs officiels
sont qualifiés : trois projections conformes (192 entrées, cinq preuves), un
doublon rejeté après authentification. Les recouvrements partiels ne débitent pas
deux fois les mêmes participants. Un bloc local compose les cinq types d'opérations,
avec deux rejets, 36 annulations et un débordement tardif sans mutation des sources.
Onze suites de régression passent. Les compteurs de l'ancien scénario local de
registre deviennent 11 rejets/2 non-pris-en-charge : les preuves indexées vides
sont désormais rejetées ; seules les attestations sans contexte demeurent non
prises en charge. Les anciennes campagnes restent archivées sans réécriture.

Cela complète les types d'opérations Phase0 dans le pipeline partiel, pas la
validation du bloc entier : votes Eth1, slots/époques complètes, state_root,
commit/reprise durable et réacteur réseau restent ouverts. Les séquences
multi-époques précédemment différées ne sont pas promues. Aucun fork moderne ni
EIP-7549/Electra n'est déclaré activé par cette étape.
Source : https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#attester-slashings

### Soldes effectifs Phase0 — composant d'époque qualifié

`prepare_phase0_effective_balances` applique les seuils d'hystérésis du preset
épinglé, l'arrondi par incrément et le maximum effectif Phase0. Les comparaisons
par différence évitent les débordements des additions solde/seuil. Les règles
sont explicites ; ce maximum individuel n'est pas un plafond de validateurs.
Le candidat conserve les soldes réels, le registre et ses métadonnées ; une
annulation ou une configuration invalide ne modifie pas la source.

`effectiveBalanceQualification` : le scénario officiel complet compare 64
validateurs (8 changements), avec 8 cas locaux de seuils/cap/solde maximal,
66 annulations et 8 rejets. Quatre suites passent, dont les régressions du registre,
de FFG et des opérations de blocs. Les preuves incluent une altération de l'état
attendu rejetée malgré un hash de fixture recalculé.

Ce composant n'avance pas l'époque : l'orchestration des récompenses, des resets,
des historiques et de la rotation des attestations reste à intégrer, puis à
qualifier avec state_root, persistance et réseau. Aucun EIP-7251/Electra ni
consensus actif n'est déclaré complet.
Source épinglée : https://github.com/ethereum/consensus-specs/blob/fcdcf42024f33c498bff74936eabea2d96049fc2/specs/phase0/beacon-chain.md#effective-balances-updates

### Fin d'époque Phase0 — historiques et rotation

`consensus_epoch_maintenance` couvre cinq composants : reset périodique des votes
Eth1, remise à zéro de la prochaine case de sanctions, copie du mix RANDAO vers
la prochaine case, accumulation de `HistoricalBatch` SSZ (racines des blocs et
états), et rotation des attestations courantes vers les précédentes. Les entrées
sont des états authentifiés ; ces fonctions ne valident pas des votes réseau.
Les paramètres viennent du preset. Les vecteurs d'historiques utilisent sa taille
puissance de deux ; la limite de liste est vérifiée avant tout ajout.

`epochMaintenanceQualification` couvre les six scénarios officiels des cinq
composants, sept contrôles de frontières/retour circulaire, douze rejets et 128
annulations pendant la construction des historiques. Cinq suites passent avec
les régressions RANDAO, sanctions différées, FFG et soldes effectifs. Les cinq
altérations des états attendus (hashs recalculés) sont rejetées par les tests.

Les candidats conservent le slot et l'époque : l'orchestrateur doit appliquer
chaque étape une seule fois, après les calculs qui consomment les anciens votes,
sanctions et soldes effectifs, puis avancer le slot. Aucun marqueur de rejeu ni
commit durable n'est créé ici. Les récompenses, l'orchestration complète, le
hash de BeaconState et l'intégration au réseau restent ouverts. Le reset Eth1
ne remplace pas l'authentification des dépôts ou le traitement des votes Eth1.
Cette étape ne constitue pas EIP-6110, Capella/Electra, ni la finalité réseau.
Source épinglée : https://github.com/ethereum/consensus-specs/blob/fcdcf42024f33c498bff74936eabea2d96049fc2/specs/phase0/beacon-chain.md#epoch-processing

### Récompenses et pénalités Phase0

`prepare_phase0_rewards` reconstruit les comités de l'époque précédente, compte
une seule fois chaque participant non sanctionné pour source/cible/tête, et
choisit la première attestation au délai d'inclusion minimal pour rémunérer le
proposant. Le calcul utilise le solde actif de l'époque courante et la finalité
après FFG. Les pénalités d'inactivité suivent le seuil strict du preset. Les
produits/sommes intermédiaires et la racine carrée entière utilisent GMP ; les
montants finaux et additions de soldes sont contrôlés en uint64, les débits
saturent à zéro. Le candidat privé ne change aucun solde source en cas d'échec.

`rewardsQualification` : les 19 scénarios officiels concordent (1 216 entrées,
946 soldes changés, quatre scénarios de fuite d'inactivité, deux au genesis).
Dix-neuf contrôles de duplication, 275 annulations, onze entrées/configurations
rejetées, un débordement tardif atomique et 64 débits saturés sont vérifiés.
Six suites de régression passent ; une altération de solde attendu est rejetée
malgré le recalcul de son hash de fixture.

Les attestations de l'état doivent avoir été authentifiées à l'inclusion ; ce
calcul n'est pas une API de réception de votes réseau. Il ne modifie ni époque,
ni poids effectif, ni registre d'activation. L'orchestration doit l'appeler une
seule fois, après FFG et avant les mises à jour du registre et des poids. La
transition complète, son state_root, sa persistance et l'intégration réseau
restent ouvertes. Ces règles Phase0 ne sont pas les récompenses Altair/Electra,
ni les propositions EIP-8068/8363.
Source épinglée : https://github.com/ethereum/consensus-specs/blob/fcdcf42024f33c498bff74936eabea2d96049fc2/specs/phase0/beacon-chain.md#rewards-and-penalties-1

### Composition ordonnée de l'époque Phase0

`prepare_phase0_epoch` compose FFG, récompenses, registre, sanctions corrélées,
reset Eth1, soldes effectifs, resets sanctions/RANDAO, historiques et rotation
des attestations. Les paramètres communs sont contrôlés entre composants. Un
slot de fin d'époque et un état authentifié après `process_slot` sont requis.
Le candidat privé conserve slot/époque et renseigne `last_processed_epoch` : le
retraitement est rejeté. Ce marqueur doit être persisté avec l'état complet ;
son absence est réservée à l'import d'un état déjà authentifié.

`orderedEpochQualification` : six **projections** des scénarios officiels de slots
comparent registre, RANDAO, checkpoints, attestations et votes Eth1 (384 entrées,
cinq transitions d'époque). Les entrées `slots.yaml` sont conservées et hachées.
Les racines mises en cache à chaque slot ne sont pas calculées dans ces projections.
Le septième scénario `historical_accumulator` est différé, pas réussi. Aucune
racine de l'état attendu n'est utilisée comme entrée du calcul.

Les contrôles locaux vérifient cinq rejets de retraitement, l'ordre sanction /
poids effectif / reset, une composition FFG-récompenses-rotation avec votes,
594 annulations et treize rejets, dont un dépassement de capacité tardif de
l'historique sans mutation partielle. Sept suites passent. Un solde attendu
altéré est rejeté même avec un hash recalculé ; le vérificateur refuse de masquer
le scénario différé.

L'ordre complet des composants est implémenté, mais **la transition de slots et
la racine BeaconState ne sont pas qualifiées**. Restent `process_slot`, le hash
SSZ complet, la reprise durable du marqueur, les séquences signées multi-époques
et le raccordement au consensus réseau. Les anciennes limites des composants
isolés restent dans leurs campagnes historiques ; aucune activation moderne ou
qualification de production n'est déduite de cette composition.
Source : https://github.com/ethereum/consensus-specs/blob/fcdcf42024f33c498bff74936eabea2d96049fc2/specs/phase0/beacon-chain.md#epoch-processing

### Racine SSZ du BeaconState Phase0

`phase0_beacon_state_root` calcule `hash_tree_root` sur les 21 champs de l'état
SSZ complet : parties fixes, offsets/listes, validateurs et soldes, historiques,
RANDAO, sanctions, attestations en attente et checkpoints. Les limites du preset
font partie du calcul ; les branches nulles sont complétées sans allouer la
capacité maximale des listes (notamment le registre à 2^40). Les encodages non
canoniques des offsets, booléens et bitlists sont rejetés.

`beaconStateRootQualification` : les 123 vecteurs officiels du preset minimal
sont conformes, dans les sept groupes `ssz_lengthy`, `ssz_max`, `ssz_nil`,
`ssz_one`, `ssz_random`, `ssz_random_chaos`, `ssz_zero` (1 194 781 octets décodés).
Vingt et une mutations vérifient la contribution des champs ; 33 encodages ou
configurations invalides et 585 annulations sont vérifiés. Trois suites passent,
avec les régressions d'époque/historiques. Une racine attendue altérée est rejetée
même avec son hash de fichier recalculé.

La validité SSZ est distincte de la validité consensus : les vecteurs aléatoires
peuvent avoir des dates incohérentes ou des listes validateurs/soldes de tailles
différentes. Cette fonction valide leur schéma et leur racine ; l'admission de
l'état doit vérifier leurs invariants, clés et origine. Aucun état aléatoire
n'est admis par cette qualification dans le registre ou le réseau.

Le hash complet est désormais disponible mais reste à relier à `process_slot`,
au codec d'état modifiable et à la vérification des blocs/époques. La séquence
`historical_accumulator` précédemment différée n'est pas encore qualifiée.
Le preset mainnet, les états post-Phase0, la persistance et le réacteur réseau
ne sont pas couverts par cette campagne.
Source : https://github.com/ethereum/consensus-specs/blob/fcdcf42024f33c498bff74936eabea2d96049fc2/specs/phase0/beacon-chain.md#beacon-state

### Transitions de slots avec état SSZ complet

`prepare_phase0_slots` relie désormais le hash BeaconState au cache des racines
par slot, au remplissage de la racine de l'en-tête précédent et à la composition
des époques. Le codec natif reconstruit les champs consommés par l'époque et
réencode ses listes/offsets, en conservant les autres champs. Il contrôle la
cohérence des longueurs validateurs/soldes. L'entrée reste un état authentifié,
pas un mécanisme d'admission d'un genesis ou snapshot arbitraire.

`fullSlotsQualification` : les sept scénarios officiels du preset minimal,
100 slots, concordent **octet par octet sur les états complets** (107 223 octets
comparés), racines comprises. `historical_accumulator`, précédemment différé,
est maintenant qualifié dans cette nouvelle campagne. Les anciennes preuves de
projection restent inchangées. Sept contrôles comparent exécution groupée et
slot par slot avec le même marqueur ; sept rejets de retraitement de cible,
sept points d'annulation, dix entrées invalides et une rotation de records
non vides sont vérifiés. Six suites passent. Une racine historique attendue
altérée est détectée malgré un hash de fixture recalculé.

La limite de travail `maximum_slots` est un budget explicite du demandeur, pas
une limite du protocole : le rattrapage peut se faire par lots, avec les mêmes
états finaux. Les octets et `last_processed_epoch` restent des candidats privés
à persister atomiquement. Le marqueur n'appartient pas au schéma SSZ Ethereum.

Cette qualification ne couvre pas encore les blocs signés complets, les votes
Eth1/dépôts composés avec eux, l'authentification des snapshots, leur commit et
reprise durable ou le réseau actif. Les historiques sont recalculés depuis les
états complets ; aucune performance de nœud à un million de validateurs n'est
déduite de ces sept scénarios. Les profils post-Phase0 restent distincts.
Source : https://github.com/ethereum/consensus-specs/blob/fcdcf42024f33c498bff74936eabea2d96049fc2/specs/phase0/beacon-chain.md#beacon-chain-state-transition-function

### Transition complète des blocs signés Phase0

`prepare_phase0_state_transition` relie slots/époques, parent dérivé de l'état,
proposant/signature, RANDAO, votes Eth1, les cinq types d'opérations et la
vérification finale du `state_root` annoncé. Il n'existe pas d'option pour sauter
la vérification de racine. Les votes Eth1 comparent les trois champs et exigent
une majorité stricte sur la période entière ; les opérations consomment ensuite
le nombre de dépôts déterminé par cette décision. Les données de parent et
registre ne viennent pas du bloc à valider. L'authentification est actuellement
répétée par le composant d'opérations ; aucune mesure de débit réseau n'est
inférée de cette implémentation.

`fullBlocksQualification` : tous les 45 scénarios officiels `sanity/blocks`
Phase0 minimal sont importés, avec leurs blocs et métadonnées. Les 29 séquences
valides concordent octet par octet sur les états complets (530 412 octets) ; les
16 invalides sont rejetées. Les séquences exécutent 97 transitions acceptées,
avec attestations, dépôts, sorties, sanctions, votes Eth1, sauts d'époques et
historiques. Les séquences multi-époques auparavant différées sont couvertes dans
cette nouvelle campagne ; leurs anciennes preuves restent inchangées.

Le bloc officiel à racine erronée est rejeté malgré une authentification de
signature/RANDAO réussie. Sept points d'annulation, huit mutations/troncatures,
quatre erreurs de configuration/budget et une altération d'état attendu avec
hash recalculé complètent les contrôles. Cinq suites de régression passent.

L'état de départ doit être authentifié. Le résultat demeure un candidat privé :
ni commit durable atomique octets/marqueur, ni récupération après panne, ni
fork choice ou finalité réseau ne sont fournis par cette API. Les 45 scénarios
ne prouvent pas à eux seuls toute la conformité Phase0, les profils modernes,
l'interopérabilité de réseau ou une capacité en charge. Cette étape ne bascule
pas le consensus actif et ne clôt aucun objectif de production.
Source : https://github.com/ethereum/consensus-specs/blob/fcdcf42024f33c498bff74936eabea2d96049fc2/specs/phase0/beacon-chain.md#beacon-chain-state-transition-function

### Enveloppe de persistance SSZ et reprise locale

`phase0_slot_snapshot` / `phase0_slot_from_snapshot` définissent l'enveloppe
canonique `nano-phase0-slot-v1`, contenant SSZ, racine, profil complet de règles
et marqueur d'époque. Après l'époque zéro, le marqueur doit être présent et
correspondre à l'époque précédente. Aucun historique n'est inventé à la lecture.
La restauration exige une racine attendue obtenue d'un checkpoint authentifié,
et une limite d'allocation SSZ ; l'appelant doit aussi borner le JSON avant de
le parser. Le profil inclut les limites opérationnelles : leur modification
nécessitera une migration explicite de l'enveloppe.

`slotSnapshotQualification` vérifie douze propriétés et rejette 25 enveloppes
altérées/non canoniques, dont profils divergents, marqueurs absents, état modifié,
racine recalculée par l'altération, dépassement de taille et mauvais checkpoint.
Un processus enfant enregistre le checkpoint et le premier bloc d'une séquence
Ethereum, puis appelle `_exit` après les commits synchrones RocksDB. Après
réouverture, SSZ et marqueur concordent ; le bloc suivant traverse une époque
et reproduit exactement l'état officiel. Le retraitement des blocs est rejeté.
Une seconde réouverture conserve cet état et l'absence d'un commit rejeté.
Quatre suites passent avec les régressions de slots/blocs et de sanctions.

**Portée exacte** : le test utilise l'emplacement consensus du lot atomique
existant `ChainStore`, associé à des blocs d'exécution synthétiques de support.
Il ne prouve pas encore la liaison sémantique entre état Beacon et état EVM,
ni un commit certifié du nouveau consensus. La racine de référence provient des
blocs officiels déjà authentifiés, pas de l'enveloppe stockée. Le checkpoint
initial du test est au slot 8 ; son marqueur est explicitement importé depuis
cette référence de confiance. L'enveloppe ne constitue pas un protocole
d'authentification de snapshots reçus du réseau.

L'arrêt testé survient **après** le commit, sans destructeur RocksDB. Il qualifie
cette reprise WAL locale, pas une coupure physique ni un arrêt au milieu de
l'écriture. Les fautes pendant le commit, le raccordement au fork choice et au
réacteur et les migrations de profils restent ouverts. Aucun état réel du VPS
ni fonds n'a été modifié.

### Erreur de synchronisation WAL et récupération des snapshots

`snapshotSyncQualification` utilise l'interposition macOS de test existante,
absente du binaire Nano, pour faire échouer `fsync` sur le WAL temporaire choisi.
L'erreur est observée après son append et avant le succès du commit ; les lectures
du magasin encore ouvert restent au commit précédent. Le processus sort sans
fermer RocksDB. Le parent teste trois dispositions du seul suffixe non synchronisé :
conservé, entièrement retiré, ou tronqué à sa moitié. Les tailles observées sont
consignées et vérifiées. Les octets plus anciens ne sont pas modifiés.

Dans le cas conservé, la réouverture retrouve le nouveau commit entier ; dans
les deux cas de perte, elle retrouve l'ancien entier. Bloc, état d'exécution de
support, SSZ et marqueur restent cohérents. La reprise effectue les transitions
manquantes une seule fois, reproduit l'état officiel et le conserve après une
seconde réouverture. Cinq suites passent, incluant les régressions existantes
Shanghai d'erreur de synchronisation et de nouvelle tentative.

Cette preuve couvre une erreur de synchronisation et trois dispositions de fin
de WAL sur ce Mac. Elle ne simule pas toutes les positions de déchirure, les
pertes du contrôleur/disque, ni une coupure physique. Le retrait/tronquage du
suffixe intervient uniquement après sortie du processus dans ses répertoires
de test. Comme dans `slotSnapshotQualification`, les blocs d'exécution restent
des supports synthétiques : la liaison sémantique EVM/Beacon et le commit
certifié du nouveau consensus réseau ne sont pas qualifiés. Le test est limité
à macOS et n'est pas une qualification Linux du VPS.

### Choix de tête Phase0 sur snapshot authentifié — 25 septembre 2026

`phase0_fork_choice_head` implémente le calcul LMD-GHOST sur une vue fournie
par l'appelant : poids des derniers votes du registre au checkpoint justifié,
exclusion des membres inactifs/slashés/équivoques, propagation aux ancêtres,
bonus temporaire du proposant, filtrage des feuilles par justification/finalité,
et départage lexicographique des racines. Les membres ajoutés après le checkpoint
ne contribuent pas. Le dénominateur du bonus conserve les membres actifs slashés
et le plancher d'un incrément. La racine nulle désactive le bonus.

Référence : [Phase0 fork-choice au commit fcdcf420](https://github.com/ethereum/consensus-specs/blob/fcdcf42024f33c498bff74936eabea2d96049fc2/specs/phase0/fork-choice.md).
Les sommes utilisent des entiers larges et le parcours est itératif. La vue doit
être authentifiée : cette API ne valide ni signatures ni transitions de blocs,
ne traite pas `on_attestation`, `on_block`, `on_tick` ou les preuves d'équivoque,
et ne persiste pas le Store. Les checkpoints réalisés/non réalisés, le registre
historique et l'éligibilité temporelle du bonus restent à fournir par ces handlers.

`consensus_phase0_fork_choice` couvre égalités, descendance, exclusions, registre
post-checkpoint, bonus et plancher, dépassement uint64, sources réalisées/non
réalisées, limite des deux epochs, parents invalides, annulation et une branche
locale de 20 000 blocs. Cent arbres générés déterministes comparent 10 000 poids
avec une définition directe par recherche d'ancêtres, puis la tête résultante.
Ce sont des tests locaux, pas des vecteurs officiels de fork choice ni un réseau
à 20 000 nœuds. Les suites de blocs officiels et de récupération de snapshots
passent aussi après recompilation. Journaux et empreintes locaux :
`build-cmake/qualification/fork-choice-snapshot-20260925/`.

Clôture encore requise : importer/rejouer les événements officiels, implémenter
les handlers et leur état temporel, authentifier les snapshots d'entrée, lier
Store, exécution et commit certifié, puis qualifier partitions/reprises sur le
réseau actif. Ce composant ne constitue pas une activation du nouveau consensus.

### Admission des votes de fork choice — 25 septembre 2026

`process_phase0_fork_vote` ajoute les vérifications `validate_on_attestation`,
l'authentification BLS et la mise à jour des derniers messages. Les comités et
clés sont dérivés du registre/RANDAO au checkpoint cible fourni par l'appelant.
La cible doit être un ancêtre connu à la frontière d'epoch du bloc voté ; le vote
ne peut agir qu'à partir du slot suivant. Le gossip est limité à l'epoch courante
ou précédente. L'exception pour les votes inclus dans un bloc nécessite un bloc
déjà authentifié par l'appelant. Un doublon ou conflit dans la même epoch conserve
le premier message ; seuls des votes d'epoch strictement supérieure le remplacent.
Les indices déjà marqués équivoques ne sont pas mis à jour. La signature est
vérifiée même si aucun message ne doit changer. L'admission ne marque pas à elle
seule une équivoque et n'exécute pas de sanction.

Référence identique au commit Phase0 indiqué ci-dessus, sections `on_attestation`
et `update_latest_messages`. Les règles d'inclusion d'une attestation dans un bloc
ne sont pas substituées aux règles de fork choice : notamment, cette admission
ne compare pas la source au checkpoint justifié courant du Store.

Le test local `consensus_phase0_fork_vote` utilise des clés BLS générées, vérifie
le changement de tête après admission, les répétitions/conflits, exclusions,
remplacement par epoch, 11 rejets et 70 points d'annulation sans mutation publiée.
Les suites de choix de tête et d'inclusion des attestations officielles passent
également. Journaux : `build-cmake/qualification/fork-vote-local-20260925/`.
Ce test n'est pas un replay des événements officiels de fork choice.

Il reste à dériver/authentifier le contexte cible depuis le SSZ stocké (avec les
slots sautés), traiter les preuves d'équivoque, les événements blocs et horloge,
les files de votes différés et le Store persistant, puis raccorder le réseau.
L'appelant doit garantir les preuves de possession des clés du registre, la liaison
entre contexte et racine cible, et l'accès exclusif à la vue. Aucun vote réseau
brut ne doit pouvoir fournir lui-même ce contexte ou le drapeau de bloc vérifié.

### Preuves d'équivoque dans le choix de branche — 25 septembre 2026

`process_phase0_fork_slashing` implémente la mutation de l'ensemble des
équivoques décrite par `on_attester_slashing` dans la référence Phase0 épinglée.
Les deux attestations indexées doivent être contradictoires (double vote ou
surround dans le sens prescrit), leurs indices non vides, triés, uniques et dans
la limite configurée. Les deux signatures agrégées sont vérifiées avec le domaine
de chaque epoch cible et les clés du **block state justifié**, fourni authentifié
par l'appelant. Ce contexte diffère du checkpoint state avancé utilisé pour les
votes. Seule l'intersection des signataires rejoint l'ensemble des équivoques.
Une preuve valide sans intersection ou déjà traitée est sans effet. Le registre,
les soldes et les derniers messages ne sont pas mutés : les pénalités financières
restent du ressort des opérations de bloc. L'ensemble doit être conservé au moins
depuis le dernier checkpoint finalisé, y compris pendant la synchronisation.

Les tests locaux vérifient deux signatures BLS, intersection partielle de groupes
agrégés, absence d'intersection, double vote/surround, ancien domaine de fork,
doublons, indices invalides, borne configurée et annulation à sept points. Une
preuve acceptée retire le poids du signataire et change la tête attendue. Deux
suites locales passent. Artefacts :
`build-cmake/qualification/fork-slashing-local-20260925/`.

Cette étape ne qualifie pas la réception P2P, le replay officiel d'événements,
la dérivation du block state justifié, ni la conservation durable de l'ensemble.
Elle ne déclenche aucune activation réseau du nouveau consensus.

### Contextes de fork choice dérivés du SSZ — 25 septembre 2026

`prepare_phase0_fork_state_contexts` relie le post-state immédiat d'un bloc
**déjà authentifié** à son en-tête : racine SSZ annoncée, slot, dernier en-tête
avec state_root nul, racine du bloc du checkpoint et marqueur d'epoch. Il extrait
le contexte du block state pour les preuves d'équivoque et avance les slots
sautés jusqu'au début de l'epoch cible pour le contexte des votes. Registre,
RANDAO, fork et racine de genesis proviennent du SSZ ; les deux registres ne sont
pas confondus. Un budget de slots limite le travail demandé par l'appelant.

`consensus_phase0_fork_state` rejoue les 29 séquences valides du corpus officiel
de blocs Phase0 et compare leurs états finaux aux fichiers officiels. Il dérive
ensuite les contextes, dont 24 nécessitent des slots sautés. Les projections sont
comparées au décodeur indépendant des tests ; l'avancement est comparé à
`prepare_phase0_slots`, déjà qualifié séparément par les vecteurs officiels.
Neuf rejets (racines, marqueur, en-tête, overflow, budget) et sept annulations
sont vérifiés. Les quatre suites blocs/slots officiels, votes et contextes passent.
Artefacts locaux : `build-cmake/qualification/fork-state-context-20260925/`.

Cette API ne peut pas transformer un en-tête arbitraire en ancre de confiance :
l'appelant doit avoir validé le bloc et son ascendance, ou disposer d'une ancre
initiale authentifiée. Les preuves de possession du registre initial restent
requises. La récupération du Store, l'horloge, l'admission des blocs dans le
choix de branche et son raccordement au consensus réseau restent ouverts.

### Horloge du choix de branche — 25 septembre 2026

`prepare_phase0_fork_tick` fournit une transition privée horloge/vue : expiration
du bonus au premier nouveau slot, mise à jour de l'epoch courante et promotion
des checkpoints non réalisés à la traversée d'une frontière d'epoch. La promotion
de justification nécessite le contexte authentifié du checkpoint et remplace
également le registre utilisé pour les poids. Les checkpoints de même epoch ne
sont pas remplacés. L'ensemble des équivoques et les derniers votes sont conservés.

Référence : `on_tick`, `on_tick_per_slot` et `update_checkpoints` dans la
spécification Phase0 épinglée ci-dessus. Comme aucun événement ne s'intercale
pendant l'appel, les checkpoints non réalisés sont constants : les frontières
suivantes n'ont aucun effet supplémentaire. Le rattrapage utilise donc un calcul
constant en nombre de slots manqués (la copie de la vue reste proportionnelle à
sa taille). Un temps local antérieur à celui du Store est rejeté par l'API.

`consensus_phase0_fork_clock` compare mille rattrapages déterministes à une
référence locale slot par slot, vérifie le seuil exact, le cas sans changement de
slot, le rejeu temporel, le remplacement du registre, huit rejets, deux points
d'annulation et un rattrapage jusqu'à UINT64_MAX. Les quatre suites du fork choice
passent après recompilation. Artefacts locaux :
`build-cmake/qualification/fork-clock-local-20260925/`.

Restent à fournir la source d'horloge opérationnelle, le calcul/authentification
des checkpoints non réalisés lors des blocs, la publication durable conjointe de
l'horloge et de la vue, puis le replay officiel et l'intégration réseau. Cette
transition ne fait pas tourner un nœud et ne qualifie pas sa synchronisation.

### Admission des blocs dans le Store en mémoire — 25 septembre 2026

`Phase0ForkStore` rassemble la vue/horloge, les blocs et leurs états SSZ,
les indicateurs de ponctualité et les justifications non réalisées.
`prepare_phase0_fork_block` relie le traitement `on_block` Phase0 : parent connu,
slot non futur, descendance du checkpoint finalisé, transition complète et
state_root, tête antérieure, insertion privée, bonus du premier bloc ponctuel
avec même racine dépendante du mélange, puis checkpoints réalisés/non réalisés.
Le calcul FFG anticipé est dérivé du SSZ sans mutation du post-state. Un bloc
reçu dans une epoch ultérieure applique également ses checkpoints anticipés.
Une promotion de justification dérive son registre depuis le bloc du checkpoint.
Le candidat entier doit être publié/commité ensemble par l'appelant.

La qualification locale utilise les 27 fixtures officielles de blocs qui démarrent
au slot zéro : 15 séquences valides, 12 invalides, 16 blocs acceptés. L'horloge est
avancée avant chaque admission ; les états finaux valides égalent les post-states
officiels. Les 18 fixtures démarrant sur un état intermédiaire sont exclues de ce
test de Store, sans être retirées de la qualification complète de transition de
blocs. Sont aussi testés les doublons, le bloc futur, le parent inconnu, la
signature corrompue, le seuil exact de ponctualité et sept points d'annulation.
Les six suites de régression passent. Artefacts :
`build-cmake/qualification/fork-store-blocks-20260925/`.

Il ne s'agit pas encore d'un replay des fixtures officielles **fork_choice** :
les arbres concurrents, changements de justification/finalité au cours d'un tel
replay et les arrivées adverses restent à qualifier. Le Store est copié en mémoire
et ne démontre ni une consommation mémoire bornée en fonctionnement prolongé,
ni la durabilité. Restent l'import d'ancre opérationnel, les adaptateurs votes/
équivoques/horloge sur ce Store, son format durable et sa reprise, puis le
raccordement EVM/P2P. Aucune activation réseau ou qualification production.

### Événements liés au Store et périmètre officiel — 25 septembre 2026

Les adaptateurs `prepare_phase0_store_vote`, `prepare_phase0_store_slashing` et
`prepare_phase0_store_tick` dérivent leurs contextes depuis les blocs SSZ du
Store. L'API de vote gossip n'expose ni contexte fourni par l'émetteur ni drapeau
permettant de contourner la fenêtre temporelle. Les preuves utilisent le contexte
du block state justifié sans avancer ses slots. L'horloge récupère le checkpoint
nécessaire à la promotion et avance son état privé si des slots ont été sautés.
Les opérations retournent des candidats complets ; leur commit reste à raccorder.

Cinq suites passent. Les séquences genesis passent désormais par l'adaptateur
horloge. Un genesis synthétique avec clés BLS générées vérifie vote accepté,
signature invalide, vote prématuré, preuve contradictoire, conservation des
équivoques après vote répété/changement de slot. Une promotion sur métadonnées de
checkpoint **synthétiques** vérifie l'extraction automatique du registre ; elle
ne prétend pas que des slots vides produisent une justification.

La consultation du dépôt épinglé `consensus-spec-tests` a confirmé l'absence du
répertoire `fork_choice` sous [minimal/phase0](https://github.com/ethereum/consensus-spec-tests/tree/bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908/tests/minimal/phase0)
et mainnet/phase0. Le corpus existe sous
[minimal/altair/fork_choice](https://github.com/ethereum/consensus-spec-tests/tree/bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908/tests/minimal/altair/fork_choice).
Il ne faut pas décoder ces états comme du Phase0 ou retirer leurs champs pour
annoncer une conformité. Le replay fidèle de ce corpus ajoute un prérequis ouvert :
SSZ et transitions Altair (participation, inactivity scores et sync committees),
ou des vecteurs Phase0 générés avec une provenance et un périmètre explicitement
distincts. Aucun de ces replays n'est actuellement qualifié.

Artefacts locaux, listings consultés et journaux :
`build-cmake/qualification/fork-store-adapters-20260925/`.
Durabilité, admission des ancres, mémoire bornée, ordonnancement des votes différés
et raccordement EVM/P2P restent ouverts.

### État SSZ Altair — 25 septembre 2026

`altair_beacon_state_root` implémente le schéma complet à 24 champs : les deux
listes de participation remplacent les attestations pendantes Phase0 ; scores
d'inactivité, comité de synchronisation courant et suivant sont ajoutés. Chaque
clé de 48 octets est hachée comme un vecteur SSZ, puis les clés et la clé agrégée
forment le conteneur SyncCommittee. Le format Altair est sélectionné explicitement.
Le code partage la mécanique Merkle avec Phase0 sans réinterpréter ses états.

Référence : [Altair beacon-chain au commit épinglé](https://github.com/ethereum/consensus-specs/blob/fcdcf42024f33c498bff74936eabea2d96049fc2/specs/altair/beacon-chain.md).
Tous les 123 vecteurs BeaconState du preset minimal épinglé correspondent aux
racines officielles (sept groupes, 1 401 532 octets SSZ décodés). Sept mutations
des nouveaux champs affectent la racine, 25 encodages/configurations invalides
sont rejetés, sept points d'annulation sont vérifiés. Les 123 racines Phase0 et
les transitions complètes de blocs Phase0 restent conformes à leurs tests.

Corpus : `tests/fixtures/consensus-altair-state-roots-official/`, manifeste,
Snappy originaux, SSZ décodés, racines et preset avec empreintes conservés.
Journaux : `build-cmake/qualification/altair-state-root-20260925/`.
Cette qualification est **SSZ seulement** : les octets de participation et les
clés sont des valeurs de schéma, pas une preuve de validité de consensus/BLS.
Les transitions Altair, le calcul des récompenses/participation/inactivité,
les sync committees et le replay officiel fork_choice restent ouverts.

### Scores d'inactivité Altair — 25 septembre 2026

`prepare_altair_inactivity_updates` implémente l'opération isolée
`process_inactivity_updates` sur les octets SSZ : epoch genesis inchangée,
éligibilité de l'epoch précédente, participation cible à temps des membres
non slashés, incrément du biais en cas d'absence, décrément saturé et récupération
hors fuite d'inactivité. L'addition uint64 est contrôlée avant récupération.
Les paramètres proviennent du preset/configuration, sans valeur réseau par défaut.
Seuls les scores changent dans le candidat ; aucune epoch/slot n'est avancée.

Les 21 états post officiels de l'opération correspondent intégralement, y compris
les octets hors scores. Trois suites passent (inactivité, SSZ Altair, récompenses
Phase0). Six entrées/configurations invalides, dont overflow de score et compte
incohérent, et sept points d'annulation sont vérifiés. Corpus et empreintes :
`tests/fixtures/consensus-altair-inactivity-official/`. Journaux :
`build-cmake/qualification/altair-inactivity-20260925/`.

Deux fixtures (`randomized_state`, `randomized_state_leaking`) contiennent cinq
validateurs dont l'activation est UINT64_MAX mais la sortie finie. Comme dans la
spécification de cette opération, le calcul applique les prédicats d'éligibilité
sans revalider tout le cycle de vie. Ces fixtures **ne prouvent pas l'admission**
de tels états. Les contrôles d'admission/authentification Altair restent à
implémenter avant tout raccordement réseau. La composition d'epoch, les autres
récompenses, les sync committees et le replay fork_choice demeurent ouverts.

### Récompenses et pénalités Altair — 25 septembre 2026

`prepare_altair_rewards` implémente `process_rewards_and_penalties` sur SSZ :
récompense de base par incrément, participation source/cible/tête pondérée,
absence de récompense de participation pendant une fuite d'inactivité et pénalité
d'inactivité basée sur le score. Le dénominateur utilise les soldes effectifs
actifs de l'epoch courante ; les participants éligibles sont ceux de l'epoch
précédente, hors slashés pour les crédits. Les calculs intermédiaires utilisent
des entiers larges, les additions de soldes sont contrôlées en uint64.

Les catégories sont appliquées dans l'ordre source, cible, tête, inactivité :
chaque débit est saturé à zéro **avant** les catégories suivantes. Ce n'est pas
une unique soustraction de la somme des deltas. Le test d'un solde nul qui reçoit
ensuite une récompense de tête vérifie ce point. L'appel doit suivre la mise à
jour d'inactivité dans une future composition d'epoch ; il ne la refait pas.

Les 15 post-states officiels complets correspondent. Six entrées/configurations
invalides (dont overflow de crédit), sept points d'annulation et l'ordre de
saturation sont vérifiés. Deux suites passent : récompenses et inactivité Altair.
Corpus : `tests/fixtures/consensus-altair-rewards-official/`.
Journaux : `build-cmake/qualification/altair-rewards-20260925/`.

La fonction reste une opération isolée, sans admission d'état, protection contre
le double traitement, commit ni réseau. Les pondérations 14/26/14 et le
dénominateur 64 sont les constantes du profil Altair, fournis explicitement dans
les règles. La composition d'epoch, les autres opérations Altair et le replay
fork_choice demeurent ouverts.

### Justification et finalisation Altair — 25 septembre 2026

`prepare_altair_justification_finalization` calcule les poids cibles à partir
des indicateurs de participation et du registre SSZ. Les participants doivent
être actifs dans l'epoch concernée et non slashés. Le dénominateur retient tous
les membres actifs de l'epoch courante. Les deux premières epochs restent
inchangées ; ensuite sont appliqués le décalage des bits, les seuils inclusifs
de deux tiers et les quatre règles de finalisation. Les racines sont extraites
de l'historique au slot du checkpoint, avec contrôle de la fenêtre disponible.
Les comparaisons de poids utilisent des entiers larges.

Les dix post-states officiels complets correspondent. Trois tests locaux
vérifient le seuil exact, un poids synthétique à un wei en dessous et l'exclusion
d'un participant slashé. Le cas à un wei est un test arithmétique synthétique,
pas une preuve d'admission d'un solde effectif hors incrément. Quatre rejets et
sept points d'annulation sont vérifiés. Trois suites passent : finalité,
récompenses et inactivité Altair.

Corpus : `tests/fixtures/consensus-altair-ffg-official/`.
Journaux : `build-cmake/qualification/altair-ffg-20260925/`.
La signature et l'authenticité des indicateurs doivent être établies en amont.
Cette opération ne qualifie pas la finalité réseau : composition d'epoch,
comités de synchronisation, transitions de blocs et replay fork_choice Altair
restent à compléter avant le raccordement au réseau.

### Rotation des sync committees Altair — 25 septembre 2026

`prepare_altair_sync_updates` réalise la rotation à la frontière configurée :
l'ancien comité suivant devient courant, puis le nouveau suivant est sélectionné
par mélange RANDAO/domaine sync et échantillonnage pondéré par solde effectif.
Les indices répétés sont conservés. Les clés sélectionnées sont agrégées en G1
avec vérification d'encodage/sous-groupe ; cela ne remplace pas les preuves de
possession et l'admission du registre en amont. Le SSZ hors comités reste inchangé.

La taille du comité, la période, le lookahead et les tours de mélange viennent du
preset. Le nombre maximal d'essais est un budget local fourni par l'appelant :
son dépassement lève une erreur, sans repondération ni résultat partiel. Il n'est
pas un plafond de validateurs. Le parcours du registre reste proportionnel à son
nombre de membres ; aucun résultat de débit distribué n'est déduit de ce test.

Les cinq post-states officiels correspondent : quatre rotations et un cas hors
frontière. Un cas local avec un seul membre actif conserve ses 32 occurrences
pour le preset minimal. Sept rejets, sept points d'annulation, clés invalides et
agrégat vide sont vérifiés. Trois suites passent : rotation, BLS et FFG Altair.
Corpus : `tests/fixtures/consensus-altair-sync-official/`.
Journaux : `build-cmake/qualification/altair-sync-20260925/`.

Cette étape ne traite pas encore `SyncAggregate` dans les blocs. Rotation de
participation, composition d'epoch, transition Altair complète, replay officiel
fork_choice et réseau restent à compléter.

### Rotation de participation Altair — 25 septembre 2026

`prepare_altair_participation_updates` copie les indicateurs courants vers la
participation précédente puis remet les indicateurs courants à zéro, après
validation SSZ et cohérence des tailles avec le registre. L'état source reste
inchangé ; seul le candidat contient la rotation.

Les douze post-states officiels correspondent intégralement, couvrant listes
nulles, remplies, aléatoires et registres plus grands. Trois rejets et sept points
d'annulation sont vérifiés ; les suites de récompenses et finalité passent aussi.
Un test confirme explicitement que la rotation répétée n'est pas idempotente :
le second appel effacerait la participation précédente. La composition d'epoch
et son marqueur devront garantir l'application unique avant raccordement réseau.

Corpus : `tests/fixtures/consensus-altair-participation-official/`.
Journaux : `build-cmake/qualification/altair-participation-20260925/`.
Cette opération ne ferme ni l'assemblage de l'epoch ni les contrôles d'admission,
la transition de bloc Altair, le replay fork_choice ou la durabilité.

### Époque et slots Altair composés — 25 septembre 2026

`prepare_altair_epoch` applique les douze opérations dans l'ordre prescrit :
FFG, inactivité, récompenses, registre, pénalités corrélées, Eth1, soldes effectifs,
slashings, RANDAO, historique, participation et sync committees. Les opérations
communes réutilisent les composants existants avec les paramètres Altair, dont
le multiplicateur de slashing. La reconstruction SSZ conserve les listes et les
offsets canoniques après remise à zéro des votes Eth1 ou extension de l'historique.
Le slot reste inchangé et le candidat porte le marqueur de l'epoch traitée.

`prepare_altair_slots` calcule les racines de l'état/en-tête pour chaque slot,
met à jour l'historique, appelle l'epoch à la frontière, puis incrémente le slot.
L'appel direct de l'epoch exige que les racines du slot aient déjà été mises en
cache, comme dans la spécification. L'état initial reste soumis à admission
externe ; octets et marqueur doivent être publiés et persistés ensemble.

Les sept séquences officielles Altair de slots correspondent intégralement,
soit 100 slots. Les sept comparaisons entre traitement groupé et slot par slot
reproduisent aussi les marqueurs. Sont vérifiés : rejeu d'epoch, mauvaise epoch,
appel hors frontière, règles incohérentes, budget de slots et sept annulations.
Un épuisement du budget de sélection au dernier composant sync ne publie aucun
changement ; la nouvelle tentative réussit. Sept suites de régression passent.

Corpus : `tests/fixtures/consensus-altair-slots-official/`.
Journaux : `build-cmake/qualification/altair-full-slots-20260925/`.
Cette qualification couvre les séquences minimal fournies, pas l'ensemble des
combinaisons adverses d'epoch ou un réseau distribué. Le traitement des blocs
signés Altair (notamment SyncAggregate), le replay officiel fork_choice et la
persistance/reprise du nouveau Store restent ouverts. Aucune activation réseau.

### SyncAggregate Altair — 25 septembre 2026

`prepare_altair_sync_aggregate` vérifie le bitvector et la signature BLS sur la
racine du bloc au slot précédent, avec le domaine sync committee et la version
de fork correspondant à ce slot. Le cas sans participant exige la signature
infinie canonique. Les clés répétées gardent leur multiplicité. Le proposant est
dérivé du registre et du RANDAO ; récompenses et débits saturés sont appliqués
dans l'ordre des positions, y compris lorsque le proposant est aussi membre.
Les additions vérifient le débordement et seul un candidat SSZ privé est retourné.

Les 24 cas officiels minimal du commit épinglé passent : 16 post-states identiques
octet pour octet et huit signatures invalides rejetées. Ils couvrent notamment
la participation vide/partielle/complète, les membres sortis ou retirables, le
proposant dans le comité et les signatures de mauvais domaine/bloc/comité.
Quatre mutations locales vérifient longueur, corruption et autre genesis ; cinq
contextes invalides ou débordements et sept annulations sont aussi rejetés.

L'opération suppose un état admis : clés avec preuve de possession, comité
et sa clé agrégée cohérents et authentifiés. Le calcul direct sur les participants
est équivalent au raccourci par clé agrégée de la spécification sous cette
précondition. Cette opération isolée n'authentifie pas le bloc enveloppant et
n'empêche pas sa réapplication : le pipeline complet doit assurer l'unicité.
Transition complète de bloc Altair, replay fork_choice, persistance et réseau
restent à terminer. Aucun changement du consensus actif.

Corpus : `tests/fixtures/consensus-altair-sync-aggregate-official/`.
Preuves : `build-cmake/qualification/altair-sync-aggregate-20260925/`.

### Attestations Altair — 25 septembre 2026

`prepare_altair_attestation` applique les indicateurs de participation et les
récompenses de proposant Altair. Le comité, le proposant et le domaine de
signature sont dérivés du registre, du RANDAO et du fork de l'état. Les contrôles
communs Phase0 (source, fenêtre d'inclusion, comité et signature BLS) sont
réutilisés sur un contexte privé ; aucune liste d'attestations en attente ni
limite Phase0 n'est ajoutée à l'état Altair.

Les indicateurs source/target/head dépendent des racines historiques et du délai
d'inclusion. Seuls les indicateurs nouvellement acquis contribuent à la récompense
avec les poids Altair 14/26/14, 8/64. Une inclusion répétée reste valide sans
paiement supplémentaire. Les états candidats restent privés jusqu'au succès,
avec vérification des cardinalités et du débordement de solde.

Les 41 opérations officielles minimal passent : 17 post-states identiques en
SSZ et 24 attestations invalides rejetées. Les 17 répétitions sont sans effet ;
85 mutations supplémentaires (signature, source, longueur de bits, participation
vide, autre genesis), cinq contextes invalides/débordements et sept annulations
sont vérifiés. Les clés doivent avoir établi leurs PoPs lors de l'admission de
l'état ; cette opération ne qualifie pas la transition complète du bloc.

Corpus : `tests/fixtures/consensus-altair-attestation-official/`.
Preuves : `build-cmake/qualification/altair-attestation-20260925/`.
Restent notamment les différences de dépôts/slashings et leur composition dans
le bloc Altair, puis le replay fork_choice et l'intégration réseau persistante.

### Dépôts Altair — 25 septembre 2026

`prepare_altair_deposit` réutilise les preuves Merkle, l'ordre des dépôts et les
signatures de possession du composant existant, puis reconstruit l'état SSZ Altair.
Les nouveaux validateurs reçoivent leurs deux indicateurs de participation et
leur score d'inactivité à zéro. Un complément de dépôt conserve ces champs et
les credentials existants. Une signature invalide d'une nouvelle clé consomme
l'index prouvé sans ajouter de validateur, conformément à la spécification.
Les sept listes variables sont réencodées avec leurs offsets canoniques ; les
cardinalités et la limite de représentation SSZ sont contrôlées.

Les 19 cas officiels minimal passent : 17 états complets identiques octet pour
octet (sept admissions, cinq compléments, cinq signatures ignorées) et deux
opérations invalides rejetées. Les 17 positions consommées ne sont pas rejouables ;
51 mutations de preuves/montants, cinq débordements et sept annulations sont
rejetés. Les racines SSZ des résultats restent calculables avec le schéma Altair.

Corpus : `tests/fixtures/consensus-altair-deposit-official/`.
Preuves : `build-cmake/qualification/altair-deposit-20260925/`.
L'Eth1 deposit root/count doit être authentifié par le traitement du bloc. Cette
opération privée ne prouve ni ce lien à la couche d'exécution, ni la persistance,
ni la finalité réseau. Slashings Altair et composition complète restent ouverts.

### Slashings Altair — 25 septembre 2026

`prepare_altair_proposer_slashing` et `prepare_altair_attester_slashing` dérivent
le proposant depuis l'état et vérifient les preuves avec les domaines de fork
correspondant à chaque signature. Le moteur commun de sorties et de sanctions
est réutilisé avec les paramètres Altair : pénalité minimale du preset Altair,
récompense du proposant 8/64 (exactement division par huit), et multiplicateur
de corrélation Altair. Les sorties, soldes et accumulateurs de slashings sont
reportés dans un candidat SSZ privé ; les autres champs restent inchangés.

Les 45 cas officiels minimal sont couverts : trois états de proposer slashing
et huit états de attester slashing correspondent intégralement au post-state ;
33 opérations invalides sont rejetées. Le neuvième cas valide d'attester slashing,
`already_exited_long_ago`, utilise une chronologie de registre synthétique
incohérente : son post-state SSZ est reproduit par le composant isolé, mais le
chemin strict rejette cet état. Ce cas n'est pas compté comme transition stricte
réussie. Le cas invalide `invalid_proposer_is_withdrawn` est lui aussi rejeté
pour incohérence du registre. Aucun contrôle d'admission n'est affaibli.

Onze rejeux ne peuvent appliquer une seconde sanction ; 22 mutations de preuves
ou genesis, onze débordements et quatorze annulations sont vérifiés. Les clés du
registre restent soumises à établissement préalable des PoPs. Cette étape ne
constitue ni une transition complète de bloc ni une activation du consensus.

Corpus : `tests/fixtures/consensus-altair-{proposer,attester}-slashing-official/`.
Preuves : `build-cmake/qualification/altair-slashing-20260925/`.
La composition des blocs Altair et leur replay fork_choice restent ouverts.

### Transition complète de blocs Altair — 25 septembre 2026

`prepare_altair_state_transition` compose maintenant les slots/époques Altair,
l'authentification du bloc et de son proposant, le parent, le RANDAO, les votes
Eth1, les sanctions de proposants puis d'attestateurs, les attestations, les
dépôts, les sorties et le SyncAggregate. Le corps SSZ Altair inclut le neuvième
champ SyncAggregate dans son hash_tree_root ; offsets et tailles des opérations
sont validés avant décodage. La racine d'état finale annoncée est obligatoire.
Aucun mode de saut des signatures ou du contrôle de racine n'est exposé.

L'état et le marqueur d'époque restent privés jusqu'à la réussite complète.
Les votes Eth1 sont comptés contre une majorité stricte de la période entière,
et le nombre de dépôts doit être exactement celui exigé par le curseur et le
compte authentifié. Les paramètres partagés des époques, sanctions, sorties,
récompenses, registres et comités sont contrôlés pour éviter les profils mixtes.

Les 52 séquences officielles minimal passent : 37 états finaux reproduits
intégralement, 15 séquences invalides rejetées, 105 blocs intermédiaires acceptés
avec contrôle de racine, 772164 octets de post-states comparés. Sont inclus les
opérations mixtes aléatoires, chevauchements de sanctions, auto-sanction du
proposant, doublons, votes Eth1, transitions d'époque, inactivité, historique et
sync committees vides/partiels/complets. Onze mutations/troncatures, cinq erreurs
de règles/budget et sept annulations sont testées. Un bloc à racine finale
incorrecte mais à signature d'enveloppe valide est rejeté ; un rejeu au même
slot l'est également.

Corpus : `tests/fixtures/consensus-altair-full-blocks-official/`.
Preuves : `build-cmake/qualification/altair-full-blocks-20260925/`.
Ce jalon compose les blocs Altair sur un état initial admis, sans implémenter
l'admission de snapshots, la migration Phase0→Altair, le fork_choice Altair,
la persistance du nouveau Store ou son raccordement EVM/P2P. Aucun changement
du consensus actif ni qualification de production distribuée.

### Fork choice Altair — get_head qualifié, 25 septembre 2026

Adaptateurs Altair ajoutés pour Store en mémoire, ancre de confiance externe,
contextes de votes/sanctions, horloge et réception de blocs complets. La réception
utilise la transition Altair et dérive les contextes depuis les états liés aux
blocs. Le proposant, les signataires et leurs poids ne viennent pas de messages
non authentifiés. L'ancre doit être approuvée extérieurement ; le constructeur
contrôle son lien bloc/état mais n'établit ni sa confiance ni les PoPs du registre.

Deux politiques de proposer boost sont explicites : la release v1.6.0-beta.0
sans filtre de dépendance du shuffling, et la référence plus récente avec ce
filtre (politique par défaut). Les dix scénarios officiels get_head sont rejoués
avec la politique historique et sa configuration temporelle exacte. Il ne s'agit
pas d'un changement du consensus actif de Nano.

**Échec initial conservé et résolu.** À l'événement 135 de
`voting_source_within_two_epoch`, le replay sélectionnait le bloc 41 au lieu du
bloc 40. Il omettait les attestations embarquées, alors qu'un événement bloc du
format officiel implique aussi leur réception. Ce n'était pas une erreur dans
les attentes de la fixture. `prepare_altair_received_block` compose désormais
on_block puis les attestations et preuves de double vote incluses. L'exception
de fenêtre temporelle des votes n'est accessible qu'après validation du bloc,
jamais via un paramètre fourni par un émetteur gossip. Un vote embarqué dont le
contexte ou l'ascendance ne permet pas l'admission au fork choice n'invalide pas
le bloc valide ; aucune file de récupération/différé n'est encore fournie.

Les dix scénarios passent : 109 événements blocs, 104 ticks, 19 votes séparés,
une preuve de double vote séparée et 1081 champs vérifiés (tête, temps,
checkpoints, bonus). Le code Python officiel, compilé dans un environnement
externe temporaire depuis le commit de release avec BLS actif, rejoue lui aussi
ces dix scénarios sans aucun écart. Les attentes publiées ne sont pas modifiées.
Un vote ancien reçu seul est rejeté ; le même vote embarqué dans un bloc validé
reste pris en compte. Signature de bloc altérée et sept annulations ne publient
aucune modification du Store d'entrée.

Échec historique : `build-cmake/qualification/altair-fork-head-20260925/failing-LastTest.log`.
Preuves corrigées : `build-cmake/qualification/altair-fork-head-fixed-20260925/`.
Restent les autres groupes officiels (on_block, reorg, withholding, ex_ante,
get_proposer_head), l'admission de snapshots, la persistance/reprise, la gestion
des ressources et le raccordement réseau. Aucun statut production n'en découle.

### Fork choice Altair — on_block et reorg qualifiés, 25 septembre 2026

Le runner événementiel commun rejoue désormais les 23 scénarios officiels
minimal `on_block` et les huit scénarios `reorg`, au même commit de corpus
bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908 et avec le profil historique
v1.6.0-beta.0. Aucun changement des attentes ni du moteur n'a été nécessaire.
Les fichiers originaux YAML/Snappy, leurs décodages et leurs empreintes sont
conservés ; les références partagées du groupe get_head sont aussi vérifiées.

Résultats : on_block traite 629 événements blocs et 638 ticks, vérifie 6319
champs et rejette les quatre événements invalides attendus (parent incorrect,
bloc futur, bloc antérieur à la finalité, branche incompatible avec le checkpoint
finalisé malgré des slots sautés). Reorg traite 257 événements blocs, 271 ticks
et huit votes séparés, vérifie 2640 champs, sans rejet attendu. Ces cas couvrent
les justifications retardées, les votes FFG suffisants/insuffisants et les branches
concurrentes. Les dix scénarios get_head restent verts avec leurs sept contrôles
d'annulation et gardes de réception ; ces gardes locales ne sont pas recomptées
pour chacun des deux nouveaux groupes.

CTest : `ethereum_official_altair_fork_head`,
`ethereum_official_altair_fork_block`, `ethereum_official_altair_fork_reorg` :
3/3 suites, 41 scénarios, 10040 champs vérifiés au total.
Preuves : `build-cmake/qualification/altair-fork-block-reorg-20260925/`
(journal CTest, compilation, résultats structurés et empreintes des sources).
Cette campagne native n'inclut pas un nouveau rejeu Python indépendant des deux
nouveaux groupes ; l'oracle antérieur concernait uniquement get_head.

Restent withholding, ex_ante et get_proposer_head, puis admission de snapshots,
persistance/reprise atomique, bornage des ressources et raccordement EVM/P2P.
Les scénarios utilisent le preset minimal et un Store en mémoire ; ils ne sont
ni un réseau distribué, ni une activation de consensus, ni une preuve de production.

### Fork choice Altair — withholding et ex_ante minimal, 25 septembre 2026

Les deux scénarios minimal de rétention de blocs (`withholding_attack` et
`withholding_attack_unviable_honest_chain`) et les trois scénarios minimal
ex_ante (vanilla et sandwich avec/sans attestation honnête) passent dans le
runner événementiel commun, sans modification du moteur ni des attentes.
Même corpus épinglé bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908 et profil
historique v1.6.0-beta.0 ; YAML/Snappy originaux conservés et empreintes vérifiées.

Withholding : 66 événements blocs, 86 ticks et 760 champs vérifiés.
Ex_ante : 11 événements blocs, 11 ticks, deux votes séparés et 110 champs
vérifiés. Aucun rejet attendu. Le replay indépendant avec l'implémentation
Ethereum officielle de release et BLS actif confirme les cinq scénarios et
870 champs sans écart. Les deux scénarios ex_ante réservés au preset mainnet
(`attestations_is_greater_than_proposer_boost_with_boost` et
`sandwich_with_boost_not_sufficient`) ne sont pas inclus dans cette campagne.

CTest : `ethereum_official_altair_fork_withholding` et
`ethereum_official_altair_fork_ex_ante`, 2/2 suites.
Preuves : `build-cmake/qualification/altair-fork-withholding-ex-ante-20260925/`
(journal natif, rapports indépendants, empreintes et scripts oracle temporaires).
Ces replays ne sont pas des attaques distribuées ni des mesures de charge.
Get_proposer_head reste à implémenter/qualifier ; snapshots, persistance/reprise,
ressources et raccordement au réseau restent ouverts. Aucune activation réseau.

### Altair — choix de branche du proposant, 25 septembre 2026

`altair_proposer_head` implémente l'optimisation optionnelle get_proposer_head
sur le Store authentifié. Les seuils de poids, délai et distance à la finalité
sont fournis explicitement. Les poids sont dérivés du registre justifié et des
votes admis, avec exclusion des votes sanctionnés/équivoques par le moteur
commun. Le solde actif utilisé pour les seuils inclut les validateurs sanctionnés,
comme la référence. Les calculs de poids utilisent des entiers sans débordement.

Choisir le parent exige simultanément : tête tardive, slot hors frontière
d'époque, checkpoints FFG compétitifs, finalité assez récente, proposant à
l'heure, réorganisation d'un seul slot, poids de tête strictement inférieur au
seuil et poids du parent strictement supérieur au sien. Un boost encore actif
sur la tête est une erreur de contexte ; le délai limite est inclusif.
La fonction ne modifie pas le Store, ne sélectionne aucun validateur et ne signe
aucun bloc. Elle suppose un état authentifié, sans admission de snapshot réseau.

Les deux scénarios officiels minimal get_proposer_head passent (312 champs),
confirmés indépendamment par le code Ethereum de release avec BLS actif.
Dix gardes natives supplémentaires couvrent délai inclusif/dépassé, tête ponctuelle,
boost résiduel, seuil de tête nul, seuil de parent UINT64_MAX, checkpoints FFG
incompatibles, plusieurs slots, frontière d'époque et annulation.
Les six groupes fork_choice minimal passent : 6/6 suites, 48 scénarios,
11222 champs officiels. Cela ne couvre pas les deux scénarios ex_ante mainnet.
Preuves : `build-cmake/qualification/altair-fork-proposer-20260925/`.

La qualification des groupes minimal ne termine pas l'intégration : admission
d'ancre/snapshots, persistance et reprise atomique, bornage et élagage du Store,
liaison EVM/P2P et finalité distribuée restent ouverts. Aucune activation réseau
ni changement du statut production.

### Altair — sauvegarde locale et reprise de transition, 25 septembre 2026

`altair_slot_snapshot` / `altair_slot_from_snapshot` ajoutent une enveloppe
canonique versionnée pour l'état SSZ vérifié et son marqueur de dernière époque.
Le profil contient tous les paramètres de transition Altair (époques, récompenses,
comités, sanctions, limites SSZ/corps, dépôts, sorties, attestations et sync).
La lecture exige une racine fournie par un checkpoint authentifié extérieur à
l'enveloppe. Modifier l'état et son hash interne ne suffit pas à être accepté.
La taille SSZ est bornée avant décodage ; le lecteur JSON doit aussi borner son
entrée. Ce format local n'est pas une admission de snapshots fournis par des pairs.

Le test utilise deux blocs successifs de la séquence officielle Altair attestation.
Après le premier bloc, un processus enfant écrit dans ChainStore/RocksDB avec
le commit synchronisé existant puis appelle _exit sans destructeurs. Le parent
rouvre la base, récupère les mêmes octets et marqueur, rejette le rejeu du bloc,
puis applique le suivant : résultat identique à l'exécution sans interruption
et au post-state officiel. Une seconde réouverture conserve ce résultat. Un
commit d'exécution invalide n'enregistre ni bloc ni snapshot ; une liaison à une
autre chaîne est rejetée.

Douze contrôles de reprise et 105 rejets de snapshots (chaque paramètre numérique
du profil, format, champs, marqueur, corruption, racine étrangère, limites) passent.
Les tests Phase0 existants et les 52 séquences de blocs Altair restent verts :
3/3 suites. Preuves : `build-cmake/qualification/altair-snapshot-recovery-20260925/`.

Limites : les blocs d'exécution de ce test sont synthétiques et servent uniquement
au transport atomique déjà disponible dans ChainStore. La liaison sémantique
EVM/consensus n'est pas qualifiée. L'arrêt survient après le commit réussi,
pas pendant une coupure électrique. Le Store de choix de branche (votes,
équivocations, branches, horloge, timeliness et checkpoints non réalisés) n'est
pas encore sauvegardé/restauré par cette enveloppe. Sa reprise complète reste
le prochain chantier ; aucune activation réseau ni validation production.

### Altair — sauvegarde du Store et reprise événementielle, 25 septembre 2026

`altair_store_snapshot` / `altair_store_from_snapshot` encodent désormais toutes
les branches/états, votes les plus récents, équivocations, horloge, checkpoints
réalisés/non réalisés, timeliness et proposer boost. Le registre pondéré du
checkpoint justifié est reconstruit depuis les états SSZ authentifiés ; les poids
ne sont pas acceptés comme une seconde copie arbitraire. L'enveloppe canonique
lie les règles de transition et le profil de boost/délais. Les tailles JSON,
SSZ individuelle/cumulée et nombres d'entrées sont bornés par des budgets locaux
explicites, sans nouveau plafond de validateurs dans les règles du réseau.

La lecture exige un digest externe de confiance, vérifié avant parsing JSON.
Un digest fourni par le même pair que le snapshot ne constitue pas cette confiance.
L'appelant doit conserver atomiquement cet engagement, sa séquence et sa liaison
à la chaîne. L'API n'assure pas seule l'anti-rollback du disque ni l'admission de
snapshots réseau ; ses limites d'allocation ne remplacent pas l'élagage du Store.

Échec initial conservé : filtered_block_tree avançait l'époque du checkpoint
justifié tout en conservant la racine de l'ancre après des slots vides. Le premier
restaurateur reprenait à tort le registre de l'époque initiale. Il reconstruit
maintenant l'état au début de la nouvelle époque. Aucune attente officielle ni
règle de vote n'a été modifiée pour corriger cet écart.

Les dix scénarios get_head et huit reorg sauvegardent leur Store à mi-parcours
puis à la fin (un scénario très court ne permet que la fin) : 35 écritures synchronisées dans ChainStore/RocksDB, suivies de
_exit du processus écrivain, réouverture et restauration. Le digest attendu est
conservé séparément par le parent. Tête, poids et branches viables sont identiques,
et les événements suivants continuent à satisfaire les attentes officielles.
Douze rejets au total couvrent corruption, champ ajouté, limites JSON/blocs/SSZ
et profil changé ; l'annulation est vérifiée dans chaque groupe. Les six suites
fork_choice ordinaires restent vertes : 8/8 suites au total.
Preuves : `build-cmake/qualification/altair-store-snapshot-20260925/`, échec initial
inclus. Tests : `consensus_altair_store_snapshot_head` et
`consensus_altair_store_snapshot_reorg`.

Les blocs d'exécution du transport de test restent synthétiques. Ce résultat
qualifie le format et la reprise après commit réussi, pas la liaison sémantique
EVM/consensus, les interruptions pendant commit, la panne électrique, la politique
d'ancrage en production, l'élagage, ou le raccordement au réacteur/P2P actif.

### Commit séquencé du Store — concurrence et erreurs disque, 25 septembre 2026

ChainStore dispose d'un emplacement dédié `fork-checkpoint/v1`, indépendant des
blocs d'exécution. `commit_fork_checkpoint` remplace atomiquement contenu et token
(séquence uint64 + SHA-256) dans un WriteBatch synchronisé. Il exige l'identité
chainId/genesis déjà liée et le token exact de la version précédente ; deux
écrivains calculant depuis la même version ne peuvent pas tous deux réussir.
Seule la version courante est conservée, sans accumulation de snapshots complets.
Le transport reste opaque : l'appelant doit préparer/valider le candidat de
consensus avant de le soumettre. Ce commit n'est pas encore atomique avec l'EVM.

Une erreur d'écriture rend l'instance de checkpoint inutilisable : lectures et
nouveaux commits de checkpoint échouent jusqu'à fermeture et réouverture.
L'appelant doit alors inspecter le résultat récupéré avant toute signature ou
publication. Cette règle traite l'issue incertaine d'une synchronisation échouée.
La lecture accepte aussi un engagement exact conservé extérieurement : une copie
ancienne de la base est rejetée si cet engagement correspond à la version récente.
Sans cet engagement extérieur, la restauration de tout le disque reste indétectable.
Ce mécanisme ne remplace pas une politique d'ancrage et de protection des clés.

Qualification : un seul gagnant pour deux écrivains concurrents, rejets des
versions périmées/digests faux/identités étrangères/tailles interdites ; six SIGKILL
pendant une campagne bornée de remplacements de 4 Mio, avec récupération d'un
checkpoint complet et conservation des séquences acquittées. Cela ne prouve pas
que chaque signal tombe à l'intérieur de l'appel d'écriture. Une injection macOS
fsync/F_FULLFSYNC EIO cible en plus exactement le WAL du commit : suffixe conservé
puis suffixe non synchronisé retiré dans une base temporaire. Les deux résultats
récupèrent respectivement le nouveau ou l'ancien couple contenu/token, puis
permettent un nouveau commit après réouverture. Le handle incertain est bien bloqué.

Les 35 reprises des 18 scénarios Altair get_head/reorg utilisent cette API dédiée,
sans blocs d'exécution synthétiques. Huit suites ciblées couvrent aussi les reprises
d'état Phase0/Altair et les injections disque historiques Paris/Shanghai.
Campagne : `build-cmake/qualification/fork-checkpoint-commit-20260925/`.
Tests ajoutés : `consensus_fork_checkpoint_commit` et, sur macOS,
`consensus_fork_checkpoint_sync_failure`. Restent les interruptions/erreurs Linux,
la coupure électrique physique, l'intégration événementielle au réacteur,
la liaison atomique EVM, l'ancrage et l'élagage. Aucun profil réseau activé.

### Coordinateur événementiel Altair durable, 25 septembre 2026

`AltairDurableReactor` relie tick, réception de bloc (messages embarqués inclus),
vote gossip et preuve de slashing au checkpoint séquencé. Chaque événement est
préparé sur un candidat privé, sérialisé, puis écrit avec CAS synchronisé avant
publication d'un pointeur d'état immuable et de son token. La publication après
succès n'appelle aucun callback et utilise des déplacements vérifiés noexcept.
Les méthodes sont sérialisées ; les callbacks d'annulation ne doivent pas entrer
à nouveau dans le coordinateur. Un bloc/vote/preuve refusé ne crée pas de commit.

Un conflit de version ou une erreur de commit rend le coordinateur indisponible,
y compris pour lire l'état courant. Les pointeurs précédemment publiés restent
immuables et inchangés. Une annulation pendant préparation/sérialisation ne modifie
ni mémoire publiée ni disque et n'interdit pas une nouvelle tentative. Le démarrage
exige soit un Store initial déjà authentifié pour une base vierge, soit un token
exact conservé extérieurement pour restaurer le checkpoint. L'identité de chaîne
et le profil sont vérifiés par les composants de stockage/restauration.

Les 18 scénarios officiels get_head/reorg passent via le coordinateur : 787 commits
synchronisés, dont 18 initialisations ; 17 réouvertures à mi-parcours, un scénario
ne contenant pas assez d'événements pour cette coupure. Les 3721 champs officiels
restent conformes après reprise. Une suite ciblée vérifie le bloc invalide,
cinq positions d'annulation, la conservation des anciens pointeurs, le conflit
avec un écrivain concurrent et, sur macOS, fsync EIO avant publication. Après
résolution de l'issue disque et réouverture, le traitement reprend. Quatre suites
passent, dont la régression get_head sans mode durable.
Preuves : `build-cmake/qualification/altair-durable-reactor-20260925/`.

Ce coordinateur est une brique intégrée de consensus/stockage, pas encore un service
P2P ni le remplacement du BFT actif. Il écrit un snapshot complet par événement :
base de correction fonctionnelle, sans qualification de débit ou d'amplification
d'écriture. Les commits incrémentaux, élagage, transport/admission, lien atomique
avec l'EVM, politique d'ancrage/signature et qualification réseau restent ouverts.

### Entrée SSZ des votes et preuves — Altair, 25 septembre 2026

Les décodeurs `decode_consensus_attestation_ssz` et
`decode_consensus_attester_slashing_ssz` sont désormais dans le code de production.
Ils traitent le schéma d'attestation antérieur à Electra, avec limite de comité
fournie par le profil. Offsets canoniques, longueurs fixes/variables, terminaison
du bitlist et nombre d'indices/bits sont contrôlés avant allocation de listes.
L'allocation dépend du message réel, jamais de la taille maximale configurée.
Un message mal formé/hors limite retourne nullopt ; configuration nulle et
annulation restent des erreurs distinctes. Les indices dupliqués/non triés,
signatures et devoirs appartiennent à la validation sémantique ultérieure.

`AltairDurableReactor::vote_ssz` et `slashing_ssz` composent ce décodage avec
l'admission cryptographique et le commit durable. Aucun champ fourni par le pair
ne permet de contourner la fenêtre gossip ou de se déclarer inclus dans un bloc.
Les messages mal formés ne modifient pas le token durable. Les replays officiels
get_head/reorg utilisent maintenant ces entrées en mode durable ; leur décodeur
historique de support n'est plus la porte d'entrée des messages séparés.

Qualification : 41 encodages d'attestations et 30 encodages de preuves du corpus
Altair officiel sont décodés avec conservation de tous les champs. Cela inclut
les opérations sémantiquement invalides du corpus : validité SSZ ne signifie pas
validité de signature ou de consensus. 768 troncatures, offsets altérés,
terminaisons et dépassements sont rejetés ; erreurs de configuration et annulations
propagent correctement. Cinq suites passent, incluant les 18 scénarios durables,
787 commits, 17 reprises et les gardes de publication/erreur disque.
Campagne : `build-cmake/qualification/consensus-message-ssz-20260925/`.

Reste l'intégration du transport P2P : cadrage, décompression bornée, négociation
de profil, files/budgets d'admission, récupération des dépendances, service réseau
et qualification distribuée. Le codec n'implémente pas le nouveau schéma Electra
(EIP-7549) et ne constitue aucune activation de fork ni de réseau.

### Enveloppe d'admission Altair préparée, 25 septembre 2026

L'examen de `network/main.cpp` confirme que le daemon actuel échange avec le BFT
actif via RPC. Il n'a pas encore de service d'événements Altair. L'adaptateur
`network/altair_message.hpp` prépare cette frontière sans enregistrer de nouveau
handler dans le daemon ni mélanger les profils actifs.

L'enveloppe JSON/hex non compressée contient exactement chainId, genesisHash,
forkDigest, kind et ssz. C'est un format Nano, pas le framing gossipsub Ethereum.
Le profil local fournit les identités et tailles maximales par type. Identités
incompatibles, champs supplémentaires, clés dupliquées, types inconnus (dont tick),
hex non canonique et dépassements sont rejetés avant décodage binaire. La limite
de frame passe par JsonFrame avant parsing ; un lecteur de flux devra appliquer
la même limite dès la réception fragmentée, pas seulement sur le buffer complet.
Le digest de fork est un filtre de compatibilité, pas une preuve d'authenticité.

L'admission reçoit une référence au budget partagé du daemon et utilise ses
leases bulk jusqu'à la fin du handler. Un message invalide consomme aussi son
budget ; une autre instance d'adaptateur ne crée pas une nouvelle réserve.
L'identité de pair doit venir du stream libp2p authentifié, jamais du JSON.
Le handler est synchrone et doit terminer son commit avant de répondre ; les
erreurs locales de consensus/disque/annulation remontent sans être converties
en verdict d'invalidité du pair. L'adaptateur ne doit pas être ré-entrant.

Test local vers le coordinateur durable : deux blocs du scénario officiel
chain_no_attestations committés avant acceptation, tête conforme aux attentes,
14 messages rejetés, 24 refus par quota sur une rafale bornée de 32 tentatives,
isolation des parts de pairs, partage du budget entre instances et libération
de lease après exception. Le framing est vérifié avec fragments de 1, 7 octets
et frame entière. Trois suites passent (wire, codec SSZ, publication durable).
Preuves : `build-cmake/qualification/altair-wire-admission-20260925/`.

Ce test n'ouvre aucun réseau libp2p. Restent l'enregistrement/negociation du
protocole, le service RPC/runtime Altair explicite, le travail hors boucle I/O
avec files bornées, la propagation/récupération des dépendances et la qualification
multi-nœuds. Pas de Snappy ni de fallback vers un autre profil acceptés ici.
Aucune activation réseau ou conformité wire Ethereum n'est revendiquée.

### Adaptateur asynchrone Stream libp2p, 25 septembre 2026

`AltairStreamRequest` raccorde l'enveloppe d'admission à l'interface Stream de
cpp-libp2p. Le PeerId vient du stream, la lease du budget partagé est acquise
avant lecture, et la limite de frame est appliquée à chaque fragment avant ajout
au buffer. JsonRead expose une lecture brute bornée pour préserver les clés JSON
dupliquées jusqu'au validateur d'enveloppe ; les interfaces JSON existantes restent
disponibles. La poursuite des lectures passe par l'exécuteur I/O, sans récursion
synchrone sur les fragments. Les erreurs du callback applicatif ne sont plus
avalées comme des erreurs de framing.

Le dispatcher externe reçoit message, drapeau d'annulation et callback de fin.
Il doit fournir une vraie file bornée et retourner false/lever seulement si aucun
travail n'a démarré. Il signale accepted uniquement après validation et commit
durable. Les callbacks de fin peuvent venir d'un worker ; leur traitement est
reposté sur l'exécuteur I/O et reste unique. Expiration ou arrêt explicite ferme
le stream et signale l'annulation. Si le worker tourne encore, la lease reste
occupée jusqu'à sa fin effective : les délais ne permettent pas de multiplier
les travaux cachés. Une fin tardive ne répond pas au stream expiré. Une erreur
locale ne produit aucun acquittement d'acceptation.

Dix cas sur streams contrôlés implémentant l'interface libp2p couvrent fragments,
fin depuis un thread worker, double callback, expiration pendant travail, arrêt
explicite, pair sans identité, lecture bloquée, dépassement, JSON invalide/dupliqué,
file refusée et erreur locale. Les neuf suites du module réseau passent. Les deux
régressions network/bft_network avec vrais processus et connexions libp2p locaux
passent aussi : elles qualifient le lecteur partagé et le transport existant,
**pas un réseau Altair actif**. Le test local enveloppe/coordinateur reste vert.
Preuves : `build-cmake/qualification/altair-async-stream-20260925/`.

Restent l'enregistrement/négociation du protocole Altair dans le daemon, le
service runtime/RPC explicite, la file worker bornée réelle et son arrêt coordonné,
la récupération des dépendances et les essais distribués Altair. L'adaptateur
n'est pas encore enregistré dans `network/main.cpp`. Pas d'activation réseau.

### File worker Altair bornée, 25 septembre 2026

`network/altair_worker.hpp` fournit désormais une file réelle, à un thread,
compatible avec le dispatcher de `AltairStreamRequest`. L'ordre FIFO sérialise
les traitements de consensus ; le nombre de travaux et la capacité allouée des
payloads SSZ sont bornés en comptant les travaux en attente **et** celui en cours.
Ces paramètres sont une politique locale de ressources, pas une limite du
registre des validateurs. Les callbacks et autres métadonnées sont bornés en
nombre, pas mesurés dans le budget des payloads.

`stop()` refuse toute nouvelle admission et signale l'annulation au travail actif
et aux travaux en attente. Ces derniers sont terminés sans entrer dans le handler.
Le handler actif doit coopérer aux checkpoints d'annulation et finir tout commit
avant son retour ; une décision déjà commitée n'est pas réécrite en rejet à cause
d'une annulation tardive. Les exceptions du handler produisent une erreur locale.
La callback de fin est appelée une fois après le traitement ; elle ne doit pas
lever d'exception. `join()` appartient au thread propriétaire, hors boucle I/O,
après `stop()`. L'exécuteur I/O reste vivant jusqu'au drainage des fins postées.
Un handler bloqué qui ignore l'annulation peut encore retarder l'arrêt : pas de
prétention de préemption forcée ou de délai d'arrêt garanti.

Les dix suites du module réseau passent, dont `altair_worker` (budgets cumulés,
capacité mémoire excédentaire, FIFO, exception, annulation et refus après arrêt).
`altair_stream` passe maintenant douze cas : deux supplémentaires utilisent le
vrai worker et vérifient la rétention de l'admission jusqu'à sa fin, la réponse
normale et l'absence de réponse après annulation. Les streams restent contrôlés,
sans connexions entre pairs. Preuves :
`build-cmake/qualification/altair-bounded-worker-20260925/`.

La file est disponible mais **pas instanciée dans le daemon**. Restent son
raccordement au service de consensus durable, l'enregistrement/négociation du
protocole, l'arrêt coordonné du runtime, la liaison atomique EVM et les essais
Altair distribués. Aucune activation réseau ni qualification production.

### Service worker–consensus durable, 25 septembre 2026

`network/altair_consensus_service.hpp` raccorde la file séquentielle au
`AltairDurableReactor` : blocs, attestations et preuves de slashing SSZ passent
par les validateurs et checkpoints d'annulation du cœur, puis par son commit
synchrone avant la callback d'acceptation. Le contrat de dispatch partagé
`altair_work.hpp` rend cette composition indépendante de libp2p : le service
est destiné au processus cœur, le daemon réseau restant séparé. Une exception
locale devient `local_failure`, sans acquittement positif. Une annulation après
commit ne réécrit pas la décision publiée. L'horloge reste contrôlée localement ;
il n'existe aucun type de message distant `tick`.

Qualification locale : les 18 scénarios officiels minimal `get_head` et `reorg`
passent avec `NANO_TEST_DURABLE_WORKER=1` : 366 blocs, 27 attestations, une preuve
de slashing, 375 ticks locaux et 3 721 checks. Les 787 commits incluent les
18 initialisations ; 17 reprises arrêtent/joinent le service avant fermeture de
la base, rouvrent le checkpoint exact puis poursuivent les événements. Aucun
résultat attendu du corpus n'a été changé. Le test d'admission wire passe aussi
par ce service et vérifie qu'un coordinateur indisponible après conflit de
checkpoint ne renvoie pas d'acceptation, puis qu'une instance restaurée traite
à nouveau les messages. Les dix suites du module réseau passent toujours.
Preuves : `build-cmake/qualification/altair-durable-worker-20260925/`.

Cette qualification utilise un vrai thread et RocksDB, mais pas une liaison
entre processus ni un réseau distribué Altair. Restent le point d'entrée privé
côté cœur, le transfert depuis le daemon, la négociation du protocole, le pilotage
de l'horloge et l'arrêt du runtime complet, puis la liaison EVM et les essais
multi-nœuds. Aucun changement d'activation réseau ni de statut production.

### Entrée RPC privée Altair, 25 septembre 2026

`nano_altairReceive(token, encodedEnvelope)` est maintenant traité par `Rpc`
uniquement lorsqu'un `AltairRpcService` a été explicitement injecté à sa
construction. Sans service, la méthode renvoie `-32601`. Avec service, le jeton
réseau est obligatoire (`-32001` sinon), `HttpServer` refuse une adresse d'écoute
non loopback et les requêtes navigateur avec Origin sont refusées. Ce contrôle
ne remplace pas la sécurité du compte hôte ni une authentification de snapshot.

Le cœur valide à nouveau l'enveloppe brute (profil, champs, doublons et tailles)
avant la file de consensus. La méthode compte dans le budget partagé des tâches
RPC coûteuses, pour HTTP et WebSocket, y compris dans les batches/notifications.
Elle libère le verrou RPC avant d'attendre le worker. Le worker Altair garde son
propre budget jusqu'à la fin réelle du travail. La réponse `accepted` suit le
commit durable ; les erreurs locales sont distinctes des rejets de consensus.
Saturation/arrêt renvoient `-32005`, erreur locale `-32603`, enveloppe invalide
`-32602`. Une échéance dépassée renvoie `-32002` et signale l'annulation au worker.
Une échéance ou une déconnexion n'est **pas une preuve d'absence de commit** ;
la stratégie de récupération après perte d'acquittement reste à raccorder au
transport du daemon. Aucun tick ne peut être transmis par cette méthode.

Les suites `rpc`, `http`, `consensus_altair_wire_admission` et
`consensus_altair_private_http` passent. Le nouveau cas utilise de vraies
connexions HTTP loopback, un worker réel et RocksDB ; deux blocs officiels sont
persistés, et le choix de tête correspond au corpus. Il teste le mauvais jeton,
la désactivation par défaut, le bind public refusé, les enveloppes incohérentes
ou dupliquées, le coordinateur indisponible et le service arrêté ; les RPC
ordinaires restent disponibles. Client et serveur sont dans le même processus
pour cette qualification : ce n'est pas encore un test daemon/cœur séparés.
Preuves : `build-cmake/qualification/altair-private-http-20260925/`.

Restent la construction du service depuis une configuration/ancre authentifiée
au démarrage de Nano, son appel depuis `nano-p2p`, la négociation de profil et
la gestion des réponses perdues, puis le runtime multi-nœuds et la liaison EVM.
La configuration courante n'injecte aucun service : pas d'activation implicite.

### Chargement explicite du profil Altair, 25 septembre 2026

Le runtime ne pouvait pas encore reconstruire les règles complètes sans les
helpers de fixtures. Le cœur expose maintenant `altair_transition_profile` et
`altair_transition_from_profile` : les 91 paramètres numériques et la version
initiale de fork sont lus sans défaut implicite, depuis le même format déjà lié
aux snapshots. Les tableaux ont une longueur exacte, chaque quantité est une
chaîne hexadécimale canonique, et toute conversion vérifie la largeur réelle du
champ cible (notamment les compteurs de rounds). Champs inconnus/manquants et
versions de fork non canoniques sont refusés. Les contrôles existants de cohérence
entre règles de bloc et d'epoch sont réutilisés ; cela ne prétend pas certifier
toutes les combinaisons économiques ou tous les paramètres opérationnels.

`load_altair_transition_profile` borne les octets, compare leur SHA-256 à un
digest fourni indépendamment, puis parse le JSON en refusant les clés dupliquées.
Le digest attendu doit provenir d'une configuration de confiance : le calculer
à partir du fichier reçu ne l'authentifierait pas. La sélection d'un profil
réseau et l'authentification de l'ancre restent des responsabilités distinctes.

`consensus_altair_explicit_profile` passe avec 502 refus de configurations
malformées/incohérentes et deux blocs officiels qui reproduisent exactement
l'état final attendu avec les règles rechargées. Les octets des snapshots restent
identiques. `consensus_altair_snapshot_recovery` passe également (105 snapshots
rejetés, reprise après sortie abrupte et résultat officiel identique).
Preuves : `build-cmake/qualification/altair-explicit-profile-20260925/`.

Il reste à composer ce chargeur avec les fichiers d'ancre vérifiés et les limites
du Store dans le démarrage de Nano, à gérer la reprise, puis à raccorder le daemon.
Aucun preset minimal n'est installé comme profil de production ; aucun consensus
réseau ni paramètre de genesis existant n'est activé/modifié.

### Import local d'une ancre vérifiée, 25 septembre 2026

`load_altair_anchor` compose maintenant les règles explicites avec deux fichiers
locaux bornés : état SSZ et bloc Beacon non signé. Les engagements attendus
(racines du bloc et de l'état, genesis_validators_root et version du fork) sont
fournis séparément par l'appelant. Le chargeur vérifie le format SSZ, le domaine
de fork, la liaison bloc/état et ces racines avant de rendre un Store candidat.
Il reconstitue le marqueur d'epoch à partir du slot authentifié et conserve
l'horloge du slot d'ancrage. Il ne touche pas au stockage.

La lecture refuse les fichiers absents/non réguliers et les tailles excessives ;
la taille est aussi surveillée pendant la lecture. Les checkpoints d'annulation
couvrent lecture, validation et retour du candidat. Les contrôles de cohérence
existants des règles de bloc/epoch sont réutilisés, avec validation explicite des
paramètres de timing et du profil de proposer boost.

`consensus_altair_trusted_anchor` passe : ancre de départ et ancre officielle au
slot 17 donnent exactement les Stores attendus ; onze altérations de racine,
domaine, données, taille, chemin ou règles sont refusées. Quatre annulations à
différentes étapes ne créent aucun checkpoint. Le candidat accepté peut ensuite
être publié par le coordinateur durable au numéro de séquence 1.
Preuves : `build-cmake/qualification/altair-trusted-anchor-20260925/`.

Les engagements doivent provenir d'une source indépendante de confiance. Cette
fonction ne choisit pas le checkpoint, ne prouve ni sa fraîcheur ni sa finalité
et ne revalide pas les preuves de possession historiques. L'admission de snapshots
de pairs, la politique de bootstrap/reprise et la composition dans `run_node`
restent à terminer. Aucune activation réseau ou liaison EVM n'est revendiquée.

### Composition dans `nano run`, 25 septembre 2026

`start_altair_runtime` compose profil, ancre, Store durable et service RPC avec
une configuration exacte, sans defaults de consensus. Les identités de chaîne
proviennent du genesis d'exécution ; le fork digest dérive de l'ancre. Les budgets
worker/transport sont vérifiés pour pouvoir transporter les messages du profil.
Les handles du service et du réacteur retiennent un propriétaire commun, qui
draine le worker avant de fermer la base.

`nano run` peut injecter ce runtime via le champ `altair`, uniquement en RPC
loopback, sans signataire BFT historique configuré et avec une base distincte.
Le mode initial refuse tout checkpoint déjà présent. La reprise exige un token
exact et vérifie aussi profil et ancre, sans retour implicite à un Store neuf.
Le journal `altair-runtime-ready` expose le token initial et précise explicitement
l'absence de protocole réseau Altair actif et de liaison EVM.

Les tests couvrent les configurations refusées, le maintien des durées de vie par
le handle RPC, l'initialisation et la reprise aux séquences 1 puis 2, et le refus
d'un token devenu ancien. Le cas géré lance les vrais exécutables `nano` et
`nano-p2p`, vérifie le RPC authentifié, arrête puis relance avec le token attendu.
Ce sont des processus locaux, pas un réseau de validateurs Altair.

Configuration et limites : `docs/ALTAIR-RUNTIME.md`. Restent le pilotage local de
l'horloge, l'appel/négociation depuis le daemon, le suivi des engagements après
commit, la récupération des acquittements perdus et la liaison EVM. Aucun fichier
de déploiement n'active ce runtime par défaut. Preuves :
`build-cmake/qualification/altair-managed-runtime-20260925/`.

### Horloge locale et statut du checkpoint, 25 septembre 2026

`AltairLocalClock` est désormais composable dans le runtime via une section
`clock` explicite. Activée, elle échantillonne l'heure système UTC, effectue les
ticks périodiques sans accumuler de timers et actualise l'heure avant le traitement
des messages. Un instant inchangé ne crée pas de checkpoint supplémentaire. Les
reculs, sauts au-delà de la borne locale ou erreurs de traitement verrouillent
l'horloge en erreur jusqu'au redémarrage ; les messages suivants ne sont plus
traités. Une annulation de requête seule ne verrouille pas l'horloge.

Le statut privé `nano_altairStatus(token)` expose séquence, digest, heure du Store,
identité du profil wire et état de santé de l'horloge. Il est désactivé sans service,
authentifié comme la réception et exécuté hors verrou RPC dans la file bornée.
Cela permet de lire un engagement après commit, sans prétendre conserver cet
engagement sur un support indépendant ni détecter seul un rollback du disque.

Les cinq suites RPC, HTTP, HTTP Altair privé, runtime et runtime géré passent.
Huit gardes d'horloge utilisent une source temporelle déterministe injectée :
déduplication, annulation, progression, recul, erreur persistante, thread périodique,
actualisation avant message et grand saut refusé sans nouveau commit. Le runtime
géré conserve une horloge désactivée pour son replay déterministe ; son test ne
qualifie donc pas la dérive des horloges de plusieurs hôtes. Le contrôle d'accès au
statut est aussi testé sur HTTP loopback.

Preuves : `build-cmake/qualification/altair-local-clock-20260925/`. Chaque tick
réel persiste encore le Store complet : cadence, amplification d'écriture et
performances sous charge restent ouverts. Les messages P2P Altair, les pertes
d'acquittement et la liaison EVM restent à raccorder ; aucune activation réseau.

### Tickets bornés et perte de réponse HTTP, 25 septembre 2026

Le service Altair expose un ledger de tickets par session de processus, borné par
le nombre de jobs configuré. Une reprise du même numéro/payload retrouve la
réponse sans redispatch. Un numéro réutilisé avec un autre contenu est refusé.
Les résultats restent présents jusqu'à retrait explicite et aucun travail actif
n'est évincé. Un watermark interdit la réadmission des numéros retirés/anciens.
L'annulation d'un numéro pas encore admis le réserve aussi contre un Submit HTTP
retardé. Pour les travaux actifs, la décision de fin reste distincte de la demande
d'annulation, et un commit déjà fait n'est jamais converti en rejet.

Les endpoints privés Submit/Job/Cancel/Forget/Jobs complètent Status. Le callback
de fin publie atomiquement le résultat, y compris avec une fin synchrone ou un
callback doublé. Un résultat local_failure n'est pas présenté comme un rejet
consensus. Le ledger ne prétend pas persister les reçus : une nouvelle session
après redémarrage implique un résultat inconnu à rapprocher du Store durable.

Les suites RPC, HTTP, admission wire, HTTP Altair privé et ledger passent. Le test
HTTP envoie un bloc, vérifie son admission depuis une autre connexion, ferme la
première sans lire sa réponse, puis resoumet le même ticket et récupère son
résultat. Le contrôle de séquence du checkpoint prouve un seul commit de ce bloc.
Les contrôles d'accès des cinq nouveaux endpoints sont vérifiés. Le test natif
couvre saturation, refus d'effacement actif, retry, conflit de contenu, annulation,
callback doublé, nouvelle session et admission retardée après Cancel.

Preuves : `build-cmake/qualification/altair-receipt-recovery-20260925/`.
Le daemon n'appelle pas encore ce protocole IPC. La récupération après changement
de session, l'admission P2P complète et la liaison EVM restent ouvertes.

### Client IPC avec tickets côté daemon, 25 septembre 2026

`network/altair_core_dispatch.hpp` fournit maintenant un dispatcher asynchrone
compatible avec le Stream Altair et `Core::call`. Les messages admis localement
restent bornés, les admissions sont ordonnées et une réponse Submit perdue est
résolue par recherche du même ticket. Une réponse Forget perdue ne perd pas la
décision déjà confirmée. Les doubles callbacks sont ignorés. Une réponse de
mauvaise séquence ou une indisponibilité dépassant le budget de tentatives bloque
le client en conservant ses admissions ; aucune fausse fin de worker n'est émise.

Les onze suites du module réseau passent. Les huit scénarios du dispatcher
couvrent pertes Submit/Forget, ticket inconnu puis retry identique, annulation avec
admission incertaine, saturation, cœur indisponible, réponse incohérente et arrêt
avec requête locale en attente. Le test `consensus_altair_core_client` utilise le
vrai client HTTP `Core::call` contre le serveur et le service durable : deux blocs
officiels sont commités une fois chacun, malgré une réponse d'admission supprimée
à la frontière du callback client. Le test distinct HTTP privé ferme réellement
une connexion sans lire sa réponse et reste vert.

Preuves : `build-cmake/qualification/altair-core-dispatch-20260925/`.
Ce composant n'est pas encore instancié par le daemon. Restent découverte et
vérification du profil, récupération des tickets initiaux, handler P2P et essais
entre plusieurs nœuds. Aucun protocole Altair réseau n'est activé par cette étape.

### Identité sémantique du runtime, 25 septembre 2026

Le statut privé expose une empreinte SHA-256 de l'identité de chaîne, du profil
complet de transition, du choix de branche et des engagements d'ancre, ainsi que
les limites wire du cœur. Douze modifications de règles/identité/ancre changent
l'empreinte ; un treizième scénario vérifie sa stabilité avec un autre chemin,
d'autres budgets de worker et un fichier de profil reformatté. Le redémarrage
avec un checkpoint plus récent conserve aussi l'empreinte.

Quatre suites passent : runtime, HTTP privé, client IPC réel et démarrage/reprise
des processus Nano/P2P. Preuves :
`build-cmake/qualification/altair-consensus-identity-20260925/`.
La découverte par le daemon, la négociation P2P, la récupération initiale des
tickets et l'intégration du handler restent ouvertes. Aucun fork ni réseau public
activé ; aucune clôture de conformité EIP revendiquée.

### Clôture des sessions IPC avant reprise, 25 septembre 2026

Le RPC privé `nano_altairRotateSession` change la session attendue sous le même
verrou que Submit. Les anciennes requêtes retardées sont refusées ; les tickets
actifs restent visibles, annulables et non effaçables avant leur fin. Les décisions
commitées et le watermark sont conservés. Une répétition avec l'ancienne session
échoue sans autre rotation.

Cinq suites passent (ledger, HTTP privé, client IPC, runtime et processus gérés),
avec 64 courses Submit/rotation et des requêtes d'ancienne session sur HTTP réel.
Preuves : `build-cmake/qualification/altair-session-fence-20260925/`.
Cela prépare la récupération du daemon ; ni son orchestration, ni un réseau Altair
actif ne sont démontrés. La clôture porte sur les tickets, pas sur l'ancien appel
synchrone Receive. Le propriétaire unique et le passage exclusif de l'admission
réseau par les tickets restent des exigences d'intégration.

### Récupération IPC raccordée au daemon, 25 septembre 2026

`network/main.cpp` découvre désormais le service Altair du cœur et exécute
`AltairCoreRecovery` avant de construire son client à tickets. Cette récupération
vérifie chaîne/profil/limites, confirme une rotation de session, attend et retire
les anciens tickets, puis revérifie identité et session. Un résultat prêt exige
un inventaire vide. Une rotation dont la réponse est perdue doit être confirmée
à nouveau après lecture du statut. Les erreurs sont bornées par une échéance
globale et un budget de transport ; aucune expiration ne vaut fin de worker.

Les douze suites réseau passent, avec seize scénarios de récupération contrôlée.
Quatre suites Altair passent sur le cœur : runtime, HTTP privé, client IPC et
processus gérés. Le client réel récupère un ticket antérieur malgré une réponse
de rotation supprimée à sa callback ; Nano et son vrai daemon annoncent
`altair-ipc-ready` au démarrage et après reprise du checkpoint. Le test réseau
historique passe également avec les nouveaux binaires.

Preuves : `build-cmake/qualification/altair-daemon-recovery-20260925/`.
Cette préparation IPC n'enregistre pas le handler libp2p Altair, ne signe pas de
blocs et ne lie pas le consensus à l'EVM. Le propriétaire unique du daemon, la
supervision après échec et la configuration opérateur des budgets de récupération
restent à finaliser avant activation. Le chemin synchrone Receive reste hors de
la clôture des tickets ; aucune admission réseau ne doit l'utiliser.

### Admission Altair sur libp2p, 25 septembre 2026

Le mode explicite `altair.networkAdmission: true` exige l'horloge locale, ferme
l'ancien chemin synchrone Receive et permet au daemon d'enregistrer le handler
`/nano/altair/1.0.0` après récupération IPC. Le pair doit avoir effectué le handshake
sur sa véritable identité libp2p et annoncer le même profil complet. Les messages
passent ensuite par le worker et le Store durable ; le profil annoncé ne remplace
aucune validation SSZ/BLS. Le budget partagé n'est acquis qu'une fois et n'est
libéré qu'après fin effective du traitement.

Treize suites réseau et six suites cœur/intégration passent. Le nouveau trajet
réel Noise/libp2p → daemon → HTTP → réacteur transporte le payload maximal de
8 Mio (invalide, rejeté par le cœur), importe deux blocs officiels et contrôle leurs
checkpoints/heads. Neuf identités de pairs distinctes couvrent refus sans handshake,
absence/différence de profil, enveloppes invalides, rejets du cœur et panne locale.
L'horloge est celle des fixtures ; aucune finalité distribuée n'est revendiquée.

Le profil de 8 Mio révélait un dépassement du JSON-RPC de 16 Mio avec l'encodage
hex. L'IPC du dispatcher utilise désormais une enveloppe objet base64 canonique,
sans modifier le format réseau, le profil ni la limite HTTP. Un retry hex/base64
du même payload conserve le même ticket. La capacité totale est vérifiée avant
ouverture de la base ; les profils plus grands nécessitant des fragments restent
refusés explicitement et à implémenter.

Preuves : `build-cmake/qualification/altair-libp2p-ingress-20260925/`, y compris
l'échec initial et les réussites après correction. Aucun déploiement n'active le
mode par défaut. Restent propagation sortante, récupération des dépendances,
duties/signatures de validateurs, supervision multi-processus et liaison atomique
avec les blocs EVM, puis qualification distribuée des pannes/finalité/charge.

### Archive signée atomique pour la propagation, 25 septembre 2026

Le réacteur persiste désormais les octets SSZ signés des nouveaux blocs dans le
même commit que le Store. Les archives immuables ne sont publiées qu'après succès
de l'écriture synchronisée. Le format durable v2 enveloppe le Store et les blocs ;
les checkpoints v1 sans octets signés restent lisibles sans données inventées.
Une racine déjà connue ne remplace pas une signature archivée et ne remplit pas
une lacune historique par le raccourci des doublons.

Deux RPC privés, `nano_altairInventory` et `nano_altairBlock`, fournissent une
pagination bornée et les payloads compacts à partir de la même vue publiée.
Le test libp2p exporte les deux blocs réellement reçus et compare leurs octets.
La restauration vérifie digest, SSZ, en-tête, racine et signature ; cinq archives
altérées sont refusées même après recalcul du digest externe dans le test.

Huit suites passent. Les parcours officiels head/reorg couvrent 18 scénarios,
366 admissions de blocs et 17 reprises. Le test d'archive ajoute annulation juste
avant commit, erreur de synchronisation WAL injectée, absence de publication sur
échec, reprise sans séparation état/archive, réouverture et lecture v1. Ce n'est
pas une qualification de coupure électrique sur stockage de production.

Preuves : `build-cmake/qualification/altair-signed-archive-20260925/`.
La diffusion automatique reste à raccorder. Les archives font encore partie des
snapshots complets : stockage incrémental, pruning, réhydratation authentifiée des
anciens blocs manquants et débit durable restent ouverts, ainsi que la liaison EVM.


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


### Journal anti-slashing avant émission de signatures — 25 septembre 2026

Le nouveau composant `BeaconSigningJournal` est distinct du journal BFT
hauteur/tour. Il conserve atomiquement les dernières réservations par clé avant
signature BLS ; les conflicts, retours arrière et instances périmées sont refusés.
La qualification couvre 18 refus avant callback, signatures concurrentes,
réouverture, panne de synchronisation et crash avant retour de signature.
Voir `docs/BEACON-SIGNING-JOURNAL.md` et la campagne
`build-cmake/qualification/beacon-signing-journal-20260925/`.

Reste à raccorder les tâches authentifiées du validateur et la gestion des clés.
Ajouter l'import/export EIP-3076 v5 à la clôture de production : racines absentes,
fusion d'historiques et protection des zones sans historique. Ce format n'est
pas encore implémenté. Le journal local ne protège pas une clé utilisée ailleurs
ou un quorum de bridge compromis ; aucune finalité réseau n'est activée.

## Qualification multi-hôte après correction des propositions expirées

Révision b1ccbac : finalité commune 2→3→4→5 en 145763 ms, trois
validateurs sains et actifs (136/133/128 attestations locales admises).
Mac natif + Docker ARM64 + VPS AMD64, deux machines, 64 clés synthétiques.
La campagne précédente avec validateur arrêté reste classée en échec.
Les partitions WAN, l’endurance et le grand registre restent ouverts ; aucun
statut EIP ou production global ne découle de cette fenêtre. [Preuves](qualification/multihost-20260926/with-expiry/README.md).

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

### Diagnostic des transitions FFG après reprise WAN

Le checkpoint natif de la campagne ci-dessus montre 60,9 % de poids de cible
précédente au slot 64, puis quorum au slot 66 ; justification 8 au slot 72
et finalisation 8 au slot 80. Investigation suivante : arrivée et inclusion
des attestations avant la frontière d’époque. Aucun délai assoupli.
[Analyse reproductible et limites](qualification/multihost-20260927/ffg-recovery-analysis/README.md).

### Reprise WAN horodatée — redistribution à investiguer

Campagne `80493b7` : seuil 180 s encore échoué (187542 ms), puis trois avancées
8→11 en 141066 ms. Six votes inclus tard montrent 17,6–18,2 s entre fin du
relais natif→VPS et fin du premier relais VPS enregistré. Les mesures d’horloge
ne suffisent pas à expliquer cet écart ; analyser les files et le parcours
d’inventaire, sans assimiler cet écart à une latence réseau pure.
[Preuves et limites](qualification/multihost-20260927/timed-recovery/README.md).

### Relais : réutilisation des pages inconnues

Correction du gaspillage d’inventaire : 32 votes inconnus utilisent une seule
page RPC tout en conservant 32 exports et accusés indépendants. Contrôle
négatif sur l’ancien code, perte d’accusé et invalidation après export
couverts. Compilation du daemon réussie ; effet WAN encore non mesuré.
[Preuves](qualification/relay-page-cache-20260927/README.md).

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

### 2026-09-27 — Preuve de slashing après suppression de son ancien état

Nouveau test natif passé en 41,98 s : preuve BLS de double vote conservée malgré
la suppression de l’ancien état référencé, checkpoint v3 et réouverture RocksDB.
L’index équivoque et les octets signés restent présents ; resoumission inversée
sans nouvelle écriture. Le bloc suivant applique le flag `slashed` et produit
les mêmes états Beacon/EVM sur les stores élagué et non élagué. La preuve reste
archivée tant que son inclusion n’est pas finalisée. Aucun correctif moteur requis.
**Partiel** : preuve livrée localement avec clés synthétiques ; pas de qualification
de partition, réorganisation de l’inclusion ou retrait final de l’archive.
[Scénario et preuves](qualification/capella-pruning-slashing-20260927/README.md).

### 2026-09-27 — Retrait des preuves de slashing après finalisation

Publication corrigée : une preuve est retirée seulement si tous les validateurs
qu’elle incrimine sont marqués slashed dans l’état finalisé. Une inclusion sur
la tête non finalisée ne suffit pas ; les indices équivoques restent conservés.
Nouveau test passé en 69,18 s : 72 blocs, 519 votes, finalité époque 7, six élagages,
conservation avant finalité puis retrait, resoumission sans recréer l’archive et
réouverture des deux owners. Régression de preuve non finalisée également verte.
**Partiel** : un seul validateur fautif testé ; preuves multi-validateur, sanctions
partiellement finalisées, réorganisation et panne au retrait restent à qualifier.
Aucun proposer refusé observé dans ce run ; pas de qualification de ce cas.
[Implémentation et preuves](qualification/capella-slashing-retirement-20260927/README.md).

### 2026-09-27 — Consommation d’un dépôt natif après élagage

Nouveau scénario `consensus_capella_deposit_consumption` : transaction de dépôt
sur le contrat natif après élagage, réouverture, puis votes natifs jusqu’à la
majorité de la période suivante. Deux owners doivent rester identiques ; le
curseur ne passe de 0 à 1 qu’une fois. Le solde du validateur existant est vérifié
exactement, dépôt et pénalités du comité de synchronisation inclus ; le rejeu ne
change pas le checkpoint, puis les deux bases sont rouvertes.
L’assertion initiale ignorait les pénalités de participation sync vide : échec
conservé, correction du test sans changement des règles de consensus.
**Partiel** : top-up d’un validateur existant, pas admission/activation d’une
nouvelle clé, ni EIP-6110 Ethereum, bridge, WAN ou panne au commit du dépôt.
Résultat : passé en 87,06 s ; consommation au slot 80, poursuite jusqu’au slot 89,
pénalité exacte de 11 180 gwei, solde attendu/observé de 33 007 719 982 gwei.
Réouverture des deux bases et rejeu idempotent vérifiés.
[Scénario et preuves](qualification/capella-deposit-consumption-20260927/README.md).

### 2026-09-27 — Admission et activation d’une nouvelle clé native

Nouveau scénario `consensus_capella_deposit_activation` : dépôt signé au seuil
de solde effectif pour une clé absente du genesis, après élagage et réouverture.
Le registre doit passer de 64 à 65 entrées seulement lors de la consommation du
dépôt ; identité, credentials et solde initial sont vérifiés. La clé en attente
ne doit pas signer ; son époque programmée doit rester stable, puis ses votes
doivent être acceptés sur les deux owners et son activation survivre au restart.
Passé en 159,63 s : dépôt consommé au slot 80, activation époque 17, trois
votes de la nouvelle clé acceptés, finalité époque 18 au slot 161. Réouverture
des deux bases vérifiée ; régression top-up passée (85,92 s). Aucun correctif
du runtime requis. L’époque exacte n’est pas comparée à un client de référence.
**Partiel** : scénario local à clés synthétiques ; pas churn massif, signature
de dépôt invalide, partition, reorg ni panne au commit d’activation. Ne clôture
pas EIP-6110 ni l’ensemble du consensus de production.
[Scénario et preuves](qualification/capella-deposit-activation-20260927/README.md).

### 2026-09-27 — Dépôt natif à preuve de possession invalide

Nouveau scénario : dépôt payé et preuve Merkle cohérente pour une nouvelle clé,
mais signature BLS produite par une autre clé. Le curseur doit avancer sans
admettre le validateur. Les deux reactors doivent refuser ses demandes de
signature avant tout appel au signataire, sans changement de checkpoint ;
rejeu, égalité Beacon/EVM et absence d’admission après réouverture sont vérifiés.
Passé en 87,21 s : consommation au slot 80, poursuite jusqu’au slot 89, registre
à 64 conservé et finalité époque 9. Runner officiel passé en 0,38 s ; résultats
et empreintes archivés. Aucun correctif runtime requis.
**Partiel** : une signature par mauvaise clé, pas toutes les variantes malformées,
reorg, injection de votes hostiles ou panne au commit. Aucun changement des règles
runtime ; EIP-6110 et la préparation production globale restent ouverts.
[Scénario et preuves](qualification/capella-invalid-deposit-signature-20260927/README.md).

### 2026-09-27 — Agrégats sync dans le proposant Capella natif

`submit_sync_aggregate` vérifie un agrégat complet contre l’état authentifié,
le slot courant et la tête sélectionnée. Un seul agrégat éphémère est retenu,
avec préférence pour davantage de positions participantes. L’assemblage local et
`propose_current` l’incluent uniquement pour le même slot/parent, puis la transition
complète revalide le bloc. Signature seule et application des récompenses sont
séparées pour ne pas appliquer prématurément la comptabilité avant les retraits.
Nouveau scénario alternant pleine participation et agrégat vide, avec corruption,
slot futur, checkpoint périmé, rejeu et annulation. Fixtures officielles sync
également contrôlées avec un cas de soldes saturés. Résultat : 35,37 s,
40 blocs dont 20 pleinement participatifs, 272 votes, finalité époque 3 et
deux réouvertures ; 16 fixtures officielles valides et 8 invalides conformes.
**Partiel** : collecte/signature des messages individuels, sous-réseaux,
agrégation de contributions partielles, transport RPC/P2P et raccordement au
scheduler restent à implémenter ; invalidation par reorg à qualifier séparément.
[Code et preuves](qualification/capella-local-sync-20260927/README.md).

### 2026-09-27 — Fusion des participations sync disjointes

Le cache d’agrégats du proposant fusionne désormais deux participations non vides
et disjointes, vérifiées pour le même slot/parent. Les signatures G2 sont sommées,
les bits réunis et l’agrégat résultant revalidé avant publication. Les chevauchements
ne sont jamais sommés ; un sous-ensemble ne peut gonfler la participation.
La primitive BLS rejette encodages invalides, mauvais sous-groupes et points à
l’infini, y compris une somme qui s’annule. Test local renforcé avec deux moitiés,
ordre alterné, doublons et sous-ensembles ; comparaison à une signature complète
construite indépendamment. Passé en 36,01 s : 40 blocs, 20 agrégats fusionnés,
272 votes, finalité époque 3 et deux réouvertures. Runner BLS : 12 cas officiels
conformes, mutations existantes et nouvelles assertions de somme validées.
**Partiel** : cache toujours borné à un agrégat, pas fusion optimale de recouvrements,
ni collecte/signature individuelle, P2P ou scheduler. Les bascules de branche et
redémarrages entre contributions restent à qualifier.
[Implémentation et preuves](qualification/capella-sync-merge-20260927/README.md).

### 2026-09-27 — Admission individuelle des messages sync

Le reactor accepte maintenant un message `(slot, racine Beacon, index, signature)`
pour le slot de proposition suivant. Il vérifie l’état/branche, l’appartenance au
comité et la signature BLS dans le domaine approprié. Seules les positions non
encore couvertes sont ajoutées, avec multiplicité des clés répétées ; le rejeu
n’augmente pas le poids. Aucune écriture durable lors de la collecte.
Scénario renforcé : complétion d’une moitié agrégée par messages individuels,
comparaison à l’agrégat complet indépendant et rejets adversariaux. Passé en
38,33 s : 40 blocs dont 10 complétés par messages individuels, 272 votes,
finalité époque 3 et deux réouvertures. Empreintes et logs conservés.
**Partiel** : API locale uniquement ; format réseau, P2P, signature et scheduler
restent ouverts. Dérivation du contexte répétée par message : coût à mesurer et
optimiser avant toute revendication à 100k/1M validateurs.
[Implémentation et preuves](qualification/capella-sync-message-20260927/README.md).

### 2026-09-27 — Encodage et admission réseau des messages sync

Codec SSZ strict de 144 octets ajouté avec vecteur connu d’ordre/endianness.
Le type `sync_committee_message`, activé seulement pour Capella, traverse les
enveloppes hex/base64, le dispatcher privé et le worker borné jusqu’au reactor.
L’admission SSZ choisit son checkpoint sous verrou ; tailles et identité réseau
sont contrôlées avant traitement. Scénario local renforcé avec reçus asynchrones
réels du service et alternance hex/base64, plus régression dispatcher/stream.
Passé : 38,76 s pour 40 blocs/272 votes, finalité époque 3 et deux réouvertures ;
dispatcher/stream verts en 0,52 s. Binaire nano-p2p recompilé, empreintes vérifiées.
**Partiel** : pas encore de diffusion sortante ni d’échange libp2p réel de messages
sync ; l’adaptateur RPC est exercé directement, pas via HTTP. Signature locale,
scheduler, export éphémère et qualification WAN restent ouverts.
[Implémentation et preuves](qualification/capella-sync-wire-20260927/README.md).


### Diffusion sortante sync Capella — 27 septembre 2026

Les messages sync individuels validés sont exportables via catalogue figé et relais
P2P privé, avec un cache mémoire borné au comité et au contexte slot/branche.
Le daemon traite ce cinquième type ; le relais vérifie le hash du contenu.
Qualification : 40 blocs / 272 votes / finalité 3 / deux réouvertures (41,36 s),
export vers un second réacteur, doublons, budgets et invalidation après changement
de tête ; trois tests réseau passent. **Partiel** : aucune nouvelle preuve de
transport libp2p réel ni de livraison avant échéance. Signature automatique,
ordonnancement, sous-réseaux et performances restent ouverts ; aucune clôture EIP
ou production. [Preuves](qualification/capella-sync-export-20260927/README.md).


### Calendrier de précollecte sync — 27 septembre 2026

Correction d’un écart découvert en préparant la signature automatique : les
messages sont désormais admis et exportables pendant leur slot de signature,
puis pendant le slot d’inclusion. Deux contextes bornés conservent séparément
l’agrégat courant et le suivant. Test nouveau de diffusion anticipée vers un second
réacteur, franchissement du slot et inclusion exacte ; coexistence des deux
contextes vérifiée. 40 blocs / 272 votes / finalité 3 / deux réouvertures, 44,95 s.
**Partiel** : signature automatique, calendrier réel, rotation de comité et livraison
libp2p dans les délais restent ouverts. Cette correction remplace la rétention à
un seul contexte de la tranche précédente, sans clôturer une EIP ni la production.
[Spécification et preuves](qualification/capella-sync-precollection-20260927/README.md).


### Signature sync locale et planificateur — 27 septembre 2026

`sync_current` authentifie comité/branche/domaine et journal de clé, contrôle la
signature du fournisseur et l’échéance, puis repasse par l’admission normale.
Le planificateur effectue ce devoir après les attestations et expose le compteur
`syncMessagesAdmitted`. Les messages déjà collectés ne sont pas resignés.
**1280 messages sync**, 40 blocs, 272 attestations et finalité 3 dans le scénario
planifié, avec redémarrage et 64 journaux réouverts (38,21 s). Comparaison à des
signatures indépendantes, mauvaise clé, annulation, signature invalide, dépassement
de slot et seuil du tiers de slot qualifiés séparément (48,46 s).
État **partiel réseau/consensus** : clocks synthétiques, livraison libp2p/WAN dans
les délais, rotation de comité et performance restent à qualifier. Aucune clôture
EIP ou production. [Preuves](qualification/capella-sync-local-signing-20260927/README.md).


### Sync entre vrais pairs et lecture après élagage — 27 septembre 2026

Deux nœuds libp2p locaux (32 clés distinctes chacun) atteignent une finalité commune
d’époque 2. **8 blocs finalisés** contiennent chacun 6–10 positions sync distantes,
avec égalité des blocs observés sur les deux pairs et vérification BLS indépendante.
Le premier run échouait sur une lecture Beacon ; correction de l’exclusion
systématique de l’ancrage dans l’archive signée, sans fabriquer de signature.
Régression d’élagage passée (67,76 s), puis confirmation réseau réussie.
**Partiel** : seulement 21–27/32 positions incluses, 365 tentatives de relais sync
incomplètes contre 11 complètes (pas un taux de perte par message). Priorité à
l’efficacité de livraison ; WAN, adversité, rotation et production restent ouverts.
[Preuves, échec conservé et limites](qualification/capella-sync-distributed-20260927/README.md).


### Relais sync : suppression des échos — 27 septembre 2026

Les admissions acceptées alimentent maintenant le bon cache de possession du pair
pour les attestations et messages sync Capella. Les catalogues consultent ce cache
avant l’export ; les limites de débit, délais et règles de consensus restent inchangés.
Quatre tests réseau passent (33 cas pour le catalogue), dont rejet, changement de
session, pagination et budgets. Sur deux vrais pairs locaux : finalité commune 2,
**32/32 positions sync dans les 8 blocs finalisés contrôlés**, contre 21–30 avant.
Exports blocs 5981→81, attestations 19863→237, sync 3736→1092 sur ces deux runs.
**Partiel** : 119 livraisons sync sans accusé confirmé subsistent ; ces nombres ne
sont pas des taux de perte ni une garantie générale. Attribution côté réception,
rotation, WAN et adversité restent ouverts. [Preuves et limites](qualification/capella-sync-noecho-20260927/README.md).


### Attribution des interruptions réseau — 27 septembre 2026

Instrumentation optionnelle de la réception : fin unique, cause, type si décodé,
état du worker ; erreurs du diagnostic sans effet sur l’admission. Sept nouveaux
cas dans la suite stream ; quatre CTests passent. Deux pairs locaux : finalité
commune 2 et 32/32 positions sync sur huit blocs finalisés vérifiés. Le récepteur
observe **169 refus du budget avant décodage**, sans erreur locale du cœur ni
échec d’écriture d’ACK. L’émetteur observe 132 livraisons sync non confirmées.
**Partiel** : pression d’admission prouvée, correspondance individuelle et cause
fine du budget non établies. Prochaine étape : attribution débit/concurrence puis
ordonnancement sortant borné ; aucune hausse des limites. Rotation, WAN et
production restent ouverts. [Preuves](qualification/capella-receive-attribution-20260927/README.md).


### Régulation sortante Capella — 27 septembre 2026

Les messages consensus utilisent le sous-budget bulk (4/s par pair, rafale 8),
plus restrictif que le budget général. Les limites réceptrices restent inchangées.
Les catalogues partagent maintenant un rythme sortant de 4/s avec rafale bornée
à 4 ; envoi différé gardé par la session et la durée de vie du catalogue. Les refus
récepteurs exposent leur cause précise. Cinq suites passent, dont 1000 envois
simulés contre le vrai budget. Une première variante trop espacée réduisait la
participation (28/32 minimum) : preuve conservée. Variante finale, deux pairs
locaux : finalité commune 2, 32/32 dans huit blocs finalisés contrôlés, aucun refus
de budget, une livraison sync non confirmée (132 auparavant), 18 refus explicites.
**Partiel** : ces derniers refus restent à attribuer ; pas de garantie statistique,
WAN, rotation ou grande population. [Preuves et limites](qualification/capella-send-pacing-20260927/README.md).


### Causes des refus sync du cœur — 27 septembre 2026

Compteurs bornés, saturants et éphémères ajoutés au statut RPC authentifié et au
reçu d’arrêt ; aucune modification des décisions ni des checkpoints. Le scénario
local (45,95 s) vérifie les causes et distingue annulation/checkpoint périmé d’un
refus de message. Deux pairs réels locaux : finalité commune 2, 32/32 positions
dans huit blocs finalisés. Les 16 refus sync du run correspondent à **14 racines
inattendues, un slot expiré et un slot déjà atteint par la tête** ; aucun échec au
contrôle BLS observé. Cela ne valide pas les signatures rejetées avant ce contrôle.
Zéro refus de budget ; deux livraisons sans accusé confirmé restent ouvertes.
**Partiel** : attribution agrégée, pas causalité réseau par message ; ne pas relâcher
ces contrôles. Prochaine qualification : rotation des comités et réseau asymétrique.
[Preuves](qualification/capella-sync-admission-causes-20260927/README.md).


### Rotation sync native avec reprise — 27 septembre 2026

Nouveau scénario du planificateur sur le preset minimal inchangé : **216 blocs,
trois frontières (64/128/192), deux changements de composition et 215 agrégats BLS
vérifiés à 32/32 positions**. Redémarrage propriétaire/réacteur et 64 journaux au
slot 127, puis inclusion complète au slot 128. Finalité époque 25 ; élagage actif,
maximum 31 états de blocs retenus. Deux CTests passent (209,76 s), dont le scénario
court existant. L’échec initial, dû à l’hypothèse erronée que les deux comités
initiaux de la fixture différaient, est conservé ; le test étendu exige bien deux
changements réels. **Partiel** : horloge simulée, un processus natif, pas de transport
réseau ni de crash physique. Rotation distribuée, asymétrie et partitions restent
à qualifier. [Preuves et référence](qualification/capella-sync-rotation-20260927/README.md).


### Rotation sync distribuée — 27 septembre 2026

Deux nœuds libp2p réels avec 32 clés distinctes chacun, slots de 6 s et preset
inchangés : **finalité commune époque 17 après changement effectif du comité**.
Les deux bases réouvertes via leurs témoins de reprise contiennent le même état
finalisé au slot 136 ; son engagement SSZ et son nouveau comité sont vérifiés.
Huit blocs finalisés (129–136) portent des contributions distantes BLS vérifiées,
avec **28–32 positions sur 32**, et non 100 % partout. Zéro refus de budget observé ;
80 refus sync du cœur (65 racines, 15 slots déjà atteints), deux livraisons sans ACK
confirmé et neuf exports indisponibles restent tracés. **Partiel** : deux pairs
sur un hôte, réseau stable ; asymétrie, partitions à la rotation, reprise après
crash et grande population restent ouverts. [Preuves](qualification/capella-distributed-sync-rotation-20260927/README.md).


### Reprise sync après coupure du transport — 27 septembre 2026

Le nouveau mode `--sync-transport-outage-cli` passe sur deux pairs libp2p réels
(32 clés distinctes chacun). Après 96 s de SIGSTOP des seuls démons P2P,
les branches divergent sans finaliser au-delà de la borne pré-coupure, puis
retrouvent une finalité commune époque 4. Quatre blocs finalisés aux slots 29–32
contiennent des signatures distantes BLS vérifiées pour des slots supérieurs
aux têtes observées à la reconnexion (pas une preuve horaire d’émission) ;
le slot 28 n'en contient aucune. Participation observée : 15–31/32 positions,
pas une reprise complète instantanée. Zéro refus de budget observé ; 29 refus
sync du cœur (19 racines, 10 inclusions déjà atteintes) et un ACK sync non confirmé
restent tracés. **Partiel** : coupure de processus transport sur un seul hôte,
sans rotation pendant la partition ni crash disque. Aucun nouveau statut de
conformité EVM/EIP ni validation de production. [Preuves](qualification/capella-sync-outage-recovery-20260927/README.md).


### Rotation sync pendant une coupure — 27 septembre 2026

Le mode `--sync-rotation-outage-cli` passe : deux pairs, 32 clés distinctes chacun,
coupure des démons P2P pendant 96 s commencée avant le slot128, franchissement
de la rotation sur deux branches isolées, puis finalité commune époque18.
Les deux bases réouvertes donnent le même état SSZ engagé au slot144 et un comité
explicitement différent de l'initial. Cinq blocs finalisés (140–144) ont leurs
32 positions BLS vérifiées, dont12 ou20 distantes ; le slot139 n'a que12 positions
et aucune distante. Reprise progressive. Zéro refus de budget observé, mais
67 refus sync (56 racines/11 inclusions atteintes), un échec d'écriture ACK,
deux livraisons non confirmées et six exports indisponibles restent archivés.
**Partiel** : outage de processus sur un hôte, sans qualification WAN, pertes
asymétriques, crash sous charge ou grande population. Aucun nouveau statut EVM
ou production. [Preuves](qualification/capella-sync-rotation-outage-20260927/README.md).


### Contre-exemple actif : retard asymétrique — 27 septembre 2026

**ÉCHEC** sur l'image du commit `c369693` : retard netem de500ms sur la seule
sortie d'un pair, deux démons actifs/32clés chacun. Finalité commune2 avant
perturbation, puis3, sans atteindre4 dans la fenêtre96+144s. Têtes divergentes,
processus/horloges sains ; aucune finalité contradictoire observée. Ce résultat
réouvre la qualification de progression sous latence, malgré les reprises de
partition réussies. Les catalogues d'attestations prennent jusqu'à27s près de
l'arrêt ; cause précise à établir, sans relâcher les règles ni les échéances.
La reprise sans retard n'a pas été qualifiée après cet échec. Ressources dédiées
nettoyées et absence vérifiée. [Preuves](qualification/capella-asymmetric-delay-20260927/README.md).


### Essai rejeté : livraison de deux votes simultanés — 27 septembre 2026

Le candidat passe quatre suites ciblées, mais échoue au même scénario500ms :
finalité commune3 sans atteindre4. Il introduit aussi des refus de concurrence
bulk absents du témoin. Les modifications de production sont retirées ; leur
patch, les nouveaux tests et toutes les traces sont conservés pour reproduction.
Le blocage reste actif ; prochaine analyse : ordonnancement des catalogues et
livraison des votes récents/dépendances, sans relâcher les limites.
[Expérience rejetée](qualification/capella-vote-pipeline-20260927/README.md).
