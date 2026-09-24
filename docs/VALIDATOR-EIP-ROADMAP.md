# EIP et consensus : périmètre demandé pour Nano

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
