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
