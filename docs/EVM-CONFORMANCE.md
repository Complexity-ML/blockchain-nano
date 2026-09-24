# Qualification complète de l'exécution EVM

Le [registre des échecs et corrections](QUALIFICATION-FAILURES.md) doit être
repris cas par cas avant toute conclusion de préparation production.

## Point de situation au 23 septembre 2026

Le dernier replay complet est `postWalletFullEvmRequalification` :
44 244 cas d’état et 52 870 cas de blocs/transitions concordants après les
corrections RPC de concurrence et d’estimation. Ces nombres couvrent les groupes
épinglés pris en charge ; ils ne prouvent ni l’activation réseau des profils
modernes ni le consensus Ethereum complet.

Les comparaisons récentes séparent désormais les références suivantes :

| Référence et campagne | Résultat observé |
| --- | --- |
| Geth officiel 1.16.7, `anvil183IndependentGethSequenceQualification` | 129/129 transactions, séquences indépendantes, états fournis et racines concordants |
| Anvil officiel 1.8.3, `anvil183ReferenceRequalification` | 122/129 modernes ; Shanghai 21/27 pour le gas et trois rejets de créations trop volumineuses conformes |
| Anvil modifié localement, `anvil183TouchedEmptyIntervention` | 129/129 modernes et 27/27 Shanghai ; neuf écarts de plancher et quatre écarts d’autorisation expliqués par interventions successives |

La variante locale corrige la sélection de la table de gas, le signalement des
comptes absents et la suppression des comptes vides touchés dans le cache. Elle
sert à confirmer les causes, pas à remplacer une référence officielle. Les
échecs initiaux et intermédiaires restent conservés dans le registre. Nano n’a
pas été modifié pour suivre ces comportements d’Anvil.

Pour l’estimation RPC, `rpcTypedNestedEstimateQualification` couvre 32
transactions signées types 1/2 aux quotes exactes, sans marge ajoutée.
`rpcEstimateIndependentFeeEvolutionQualification` ajoute 16 transactions
EIP-1559 et 63 transitions de frais vérifiées indépendamment. Les blocs vides
produits par Nano pendant les simulations expliquent des différences de frais
avec Anvil miné manuellement ; les comparaisons d’exécution alignent leur
contexte. Cela ne garantit pas une inclusion après changement d’état ou de frais.

La qualification globale BFT, stockage, reprise, réseau et charge reste ouverte.
Le bridge USDC et une mise en service publique ne sont pas validés par ce bilan.

## Nouveau replay intégral des six groupes pris en charge

Le replay antérieur `currentFullSupportedStateReplay` vérifie à nouveau l'archive épinglée et les
7 603 fichiers extraits, puis exécute le runner Release actuel : **44 244 cas
concordants**, dont **2 893 rejets attendus**, sans divergence, erreur ou cas
non pris en charge dans ces six groupes. Le vérificateur recoupe chaque fichier,
nom de cas, profil et variante ; aucun cas omis. Les groupes antérieurs à Berlin
ne sont pas inclus. Les profils Cancun/Prague/Osaka restent diagnostiques.

Le replay antérieur des blocs (`currentFullSupportedBlockReplay`) couvre également
7 972 fichiers vérifiés : **52 870 cas concordants**, transitions comprises.
Un contrôle indépendant recoupe chaque bloc accepté/rejeté et chaque motif de
rejet attendu ; son contrôle négatif détecte un compte de blocs altéré.
Ce résultat porte sur les engagements d'exécution du pilote, pas sur une
validation complète du consensus Ethereum ni l'activation réseau des profils.

## Vérifications fonctionnelles et comparaisons signées actuelles

La régression native élargie `currentNativeRegression` recompile Release
et couvre 52 suites : 51 réussissent au premier passage, puis le test
`gossip_recovery` est corrigé pour compter pending et queued et repasse
séparément. Le rapport initial échoué reste conservé. Les suites EVM
CTest sont des sous-ensembles ; ce passage ne remplace pas le corpus complet.

La compilation Release complète et les 19 suites CTest `qualification_evm`
réussissent après correction de l’estimation des frais (32,18 s), sans flood ni chaos.
La régression et ses preuves sont enregistrées sous `rpcEstimateGasAffordability` ;
la vérification précédente reste sous `currentFunctionalEvmVerification`.

Une correction RPC antérieure est vérifiée sous `rpcSubIntrinsicGasCompatibility` :
19 suites réussies et 60/60 demandes concordantes avec l’estimateur Geth ;
24 écarts d’estimation financière Anvil restent visibles sur 60 demandes.

Le corpus `transaction_tests` complet de l’archive épinglée est aussi revérifié :
24 fichiers identiques, 110 rejets attendus Prague/Osaka, aucune omission.
Il ne contient aucun cas positif (`currentCompleteTransactionCorpus`).

Les comparaisons signées récentes complètent le corpus officiel, sans le remplacer :

| Campagne | Résultat et limite |
| --- | --- |
| Cycle EIP-7702 corrigé | Geth 129/129, état fourni et racines ; Anvil 125/129, quatre écarts connus sur les comptes vides |
| Blobs Cancun/Prague | Anvil et Geth 14/14, six transactions type 3 ; transport des sidecars Nano non couvert |
| Blobs Osaka | Nano/Geth 7/7 en exécution ; sidecar v1 valide pour Geth rejeté au décodage par Anvil 1.5.1 |
| Limites d'initcode | 90/90 validations Nano/Geth t9n de Paris à Osaka, dont 12 rejets attendus |
| Intégrité des nouvelles preuves | 13 index, 4 942 pièces et 157 références de journaux vérifiés |

Les scénarios de rollback ont été corrigés après inspection du bytecode réel ;
les premiers rapports restent conservés avec leurs limites dans le registre.
Les profils modernes restent diagnostiques : ce bilan n'active ni genesis
Cancun/Prague/Osaka, ni transport/stockage des blobs, ni PeerDAS sur Nano.
La préparation production, le catalogue OpenZeppelin complet, BFT/reprise et
les essais réseau prolongés restent des chantiers distincts ouverts.

## Bilan courant — tests d'état seulement

| Groupe officiel épinglé | Fichiers | Cas concordants | Rejets vérifiés | Divergences / erreurs / non pris en charge |
| --- | ---: | ---: | ---: | --- |
| for_berlin | 127 | 1 963 / 1 963 | 150 | 0 / 0 / 0 |
| for_paris | 129 | 2 250 / 2 250 | 280 | 0 / 0 / 0 |
| for_shanghai | 146 | 2 441 / 2 441 | 290 | 0 / 0 / 0 |
| for_cancun | 2 332 | 10 777 / 10 777 | 583 | 0 / 0 / 0 |
| for_prague | 2 466 | 13 074 / 13 074 | 785 | 0 / 0 / 0 |
| for_osaka | 2 403 | 13 739 / 13 739 | 805 | 0 / 0 / 0 |

## Bilan Osaka courant

Les 13 739 cas du groupe `for_osaka` de la même archive épinglée concordent,
dont 805 rejets vérifiés. Le sous-ensemble spécifique Osaka compte 1 231 cas
concordants dans 56 fichiers. Aucun cas ignoré, aucune divergence ni erreur.
Les résultats sont conservés dans `osakaStateResults` du manifeste et les
journaux locaux dans `build-cmake/qualification/osaka-state-full.jsonl` et
`osaka-specific.jsonl`.

Le moteur versionne CLZ, les bornes et nouveaux coûts MODEXP, le plafond de gas
par transaction, le plafond de six blobs par transaction et le précompile P-256
à l'adresse 0x100. Les opérations de courbe P-256 utilisent OpenSSL. Le test
natif `osaka` vérifie notamment CLZ pour zéro et chaque bit, les frontières de
gas et longueurs MODEXP, les signatures P-256 high-s, les entrées invalides et
la non-activation avant Osaka. Il passe après compilation.

MODEXP calcule désormais le coût avec des entiers arbitraires avant conversion
des longueurs et allocation. L'ancien plafond arbitraire de 1 Mio est supprimé
avant Osaka ; Osaka applique ses bornes explicites de 1 024 octets.

**Osaka reste refusé dans le genesis réseau.** Ces tests d'état ne qualifient
ni les blocs complets, ni les transitions, ni la disponibilité des données
PeerDAS, ni la propagation des blobs, ni la préparation production globale.
`forkConformanceProven` reste faux. Après ces changements, les **47 suites
CTest passent en 295,40 s**, dont les 317 fichiers officiels embarqués, les
scénarios BFT/reprise/charge existants et les tests cryptographiques. Le journal
est conservé dans `build-cmake/qualification/osaka-ctest.log`.

La requalification intégrale Berlin (1 963), Paris (2 250), Shanghai (2 441)
Cancun (10 777) et Prague (13 074) ne présente aucune divergence, erreur ni cas ignoré.
Les empreintes des journaux figurent dans `osakaRegressionResults` du manifeste.
Les cinq profils antérieurs sont ainsi revérifiés après les changements Osaka.

## Transitions Paris → Osaka : corpus d’exécution concordant

Le pilote reconnaît les quatre profils officiels `AtTime15k` : Paris vers Shanghai,
Shanghai vers Cancun, Cancun vers Prague et Prague vers Osaka. Il sélectionne
les règles pour chaque bloc à partir de son horodatage RLP : ancienne version
avant 15 000, nouvelle version à partir de 15 000. Le moteur reçoit explicitement
cette révision ; les parents antérieurs à l'activation peuvent ne pas avoir les
champs blob. Cela ne configure pas de calendrier de mise à jour du réseau Nano.

Le contrôle distingue le format d'en-tête et de corps attendu pour chaque version.
Les contrats système obligatoires absents sont distingués des appels qui échouent.
Les 12 cas dédiés au déploiement de ces contrats concordent. Les 19 suites
fonctionnelles sélectionnées passent en 69,53 secondes, sans flood ni chaos réseau.

Une première régression embarquée signale encore une catégorie de rejet incorrecte
pour dix blobs avant Osaka : la transaction est rejetée, mais le pilote classe
le dépassement de capacité blob du bloc comme limite de nombre par transaction.
Cette classification est corrigée ; les rapports précédents restent conservés.

Les 31 fichiers comprennent 260 cas, tous initialement unsupported. Vingt-neuf
fichiers complets sont embarqués. Les deux autres (historique des hashes et gros
blocs à la frontière de taille) restent référencés dans
`externalTransitionBlockRegressionFiles` et sont exécutés dans le passage complet.

Le profilage du cas historique long montre le recalcul des tries d'état dans le
binaire CMake sans optimisation. Un binaire distinct `build-release`, compilé en
Release, vérifie les mêmes fichiers : **260/260 cas concordants**, zéro divergence,
erreur ou exclusion ; 1622 blocs acceptés et 21 rejets vérifiés.
Les 19 suites fonctionnelles Release passent en 27,19 secondes ; les régressions
de blocs embarquées passent aussi. Les deux exécutions intermédiaires sans
optimisation ont été arrêtées après cette réussite et restent enregistrées comme
incomplètes (code 143), jamais comme réussites. Le rapport, le hash du binaire et
les empreintes des journaux figurent sous `transitionBlockCorrectedResults`.
Ces résultats n'activent pas les forks sur le réseau Nano et ne prouvent pas
à eux seuls toutes les règles de consensus Ethereum ou la préparation production.

## Blocs Osaka : règles supplémentaires en qualification

Le moteur versionne désormais le calcul d'excessBlobGas défini par
[EIP-7918](https://eips.ethereum.org/EIPS/eip-7918), avec la comparaison stricte
entre le prix de réserve et le prix blob du parent. Le profil épinglé utilise
une cible de six blobs et un maximum de neuf par bloc ; le plafond par transaction
reste six. Le calcul antérieur est conservé avant Osaka.

[EIP-7934](https://eips.ethereum.org/EIPS/eip-7934) ajoute une limite de
**8 388 608 octets** à l'encodage RLP du bloc d'exécution. Le moteur vérifie la
taille lors de la validation et après construction des engagements. Le pilote
refuse aussi un RLP brut trop grand avant son décodage. La taille calculée inclut
l'en-tête, les enveloppes des transactions, les ommers et les retraits ; la preuve
de producteur Nano est distincte de cet encodage Ethereum.

Les **1 329 cas des 63 fichiers spécifiques Osaka** concordent, sans divergence,
erreur ou exclusion. Les tests natifs blobs et osaka passent (0,96 seconde), avec
frontières du prix de réserve, limites de taille et rejet sans mutation d'état.
Les fichiers de régression complets des règles de réserve, taille, plafonds de gas
et blobs sont embarqués lorsque leur taille est inférieure à 10 Mo. Cinq fichiers
plus volumineux restent référencés par chemin d'archive, taille et SHA-256 dans
`externalOsakaBlockRegressionFiles` ; ils sont inclus dans le résultat officiel
ci-dessus, mais ne font pas partie du CTest embarqué. Le journal et son empreinte
figurent sous `osakaSpecificBlockResults`.

Le passage complet Osaka initial termine avec 16357 concordances,
17 divergences, 174 erreurs et 1 exclusion
sur 2510 fichiers (code de sortie 1). Il est conservé sous
`osakaBlockInitialResults`. Le passage corrigé termine avec **16549
concordances sur 2510 fichiers**, zéro divergence, erreur ou exclusion
(code de sortie 0), 17251 blocs acceptés rejoués et
1664 rejets atomiques. Journal et empreinte sous
`osakaBlockCorrectedResults`. La suite embarquée élargie a d'abord atteint
son timeout de 30 secondes ; avec un budget de 180 secondes elle termine
en 28,92 secondes. Les deux journaux sont conservés, sans masquer le timeout. Ces résultats
n'activent pas Osaka sur le réseau et ne qualifient pas PeerDAS, le transport
ou la disponibilité des blobs. Aucune conformité globale Osaka n'est revendiquée.

## Blocs Prague : appels système et engagement des requêtes

Le profil EthereumValidated de qualification implémente désormais les traitements
d'exécution suivants, à partir de Prague :

- [EIP-2935](https://eips.ethereum.org/EIPS/eip-2935) : appel du contrat historique
  avec le hash parent avant les transactions, sans changer la sémantique de BLOCKHASH.
- [EIP-6110](https://eips.ethereum.org/EIPS/eip-6110) : extraction ordonnée des
  dépôts des logs à l'adresse Ethereum mainnet configurée dans le binaire,
  avec validation stricte de la taille, des offsets et longueurs ABI.
- [EIP-7002](https://eips.ethereum.org/EIPS/eip-7002) et
  [EIP-7251](https://eips.ethereum.org/EIPS/eip-7251) : appels réels des contrats
  de retrait et consolidation après les transactions ; une absence de code ou
  un échec invalide atomiquement le bloc.
- [EIP-7685](https://eips.ethereum.org/EIPS/eip-7685) : champ requestsHash de
  32 octets ajouté à l'en-tête, dérivé des requêtes par SHA-256, avec types
  ordonnés et omission des groupes vides. Un engagement incorrect est rejeté.

Les appels disposent de 30 millions de gas indépendants du gas du bloc et ne
créent pas de reçus de transaction. Le bytecode déployé est exécuté par l'EVM ;
les files et l'historique ne sont pas simulés par des écritures C++ spécialisées.
Le champ requestsHash est refusé avant Prague et par la politique Nano native.
Le genesis réseau reste limité à Paris/Shanghai. Aucun traitement Beacon des
requêtes ni activation de validateurs Ethereum n'est ajouté. L'adresse de dépôt
n'est pas encore configurable pour d'autres réseaux Ethereum.

Les **222 cas des 27 fichiers complets** dédiés concordent, sans divergence,
erreur ou exclusion. Les fichiers sont embarqués avec leurs empreintes. Les tests
natifs vérifient l'encodage de l'en-tête, les données ABI, le bytecode réel, les
échecs obligatoires, les limites de fork et le retour à l'état initial. Les trois
suites ciblées initiales passent (7,07 secondes). Après ajout des fichiers
embarqués et du test de rollback d'une transaction signée, les neuf suites
core, chain, rpc, block_auth, ethereum_official_regressions,
ethereum_block_regressions, withdrawals, beacon et execution_requests passent
en 49,56 secondes. Le premier passage intégral Prague avant les appels système termine en échec :
1108 concordances, 106 divergences, 14514 erreurs
et 71 cas non pris en charge, sur 2569 fichiers. Le rapport
`prague-block-initial.jsonl` est conservé et épinglé sous `pragueBlockInitialResults`.
Le passage corrigé a été interrompu après 11 945 enregistrements concordants,
sans bilan final. Son rapport tronqué est conservé sous `pragueBlockInterruptedResults`.
Après vérification de l'absence du processus, le corpus intégral a été relancé.
Il termine avec **15799 concordances sur 2569 fichiers**, aucune
divergence, erreur ou exclusion (code de sortie 0), 16500 blocs
acceptés rejoués et 1664 rejets atomiques. Le rapport
`prague-block-system-resumed.jsonl` et son SHA-256 sont référencés sous
`pragueBlockCorrectedResults`. Cela ne qualifie ni toutes les règles de consensus
des en-têtes, ni les transitions de fork, ni l'activation réseau Prague.

## En-têtes blob Cancun : corrections ciblées

Le calcul d'excessBlobGas est maintenant vérifié contre le parent, avec la
cible propre au fork. Le total des blobs des transactions doit correspondre à
blobGasUsed ; les plafonds par transaction et par bloc sont contrôlés avant
mutation de l'état. Le chemin de vérification Nano fournit désormais le parent
au moteur, sans activer Cancun dans le genesis réseau.

Le sous-ensemble des 28 fichiers complets provenant des divergences et cas non
pris en charge du premier passage Cancun atteint **234 concordances**, zéro
divergence, zéro erreur et zéro exclusion. Les quatre fixtures sans métadonnées
rlp_decoded sont lues depuis leur RLP : les transactions avec enveloppe de
propagation sont rejetées par le décodeur et la création de contrat blob par
le moteur. Le pilote vérifie le motif de rejet et l'absence de mutation.

Les rapports intermédiaires restent conservés : 27 divergences de classification,
puis quatre erreurs d'adaptation JSON, avant le résultat corrigé. Le premier
test natif des frontières échouait parce que son parent omettait withdrawalsRoot ;
l'en-tête de test est corrigé. Les tests core, block_auth, blobs et
ethereum_block_regressions passent (4/4, 8,21 secondes). Les 28 fichiers complets
sont intégrés et épinglés par SHA-256 ; la suite embarquée enrichie passe aussi.
Le passage intégral Cancun corrigé termine avec **12114 concordances
sur 2397 fichiers**, zéro divergence, erreur ou exclusion (code de sortie 0).
Il rejoue 12988 blocs acceptés et vérifie 847 rejets atomiques.
Le rapport et son SHA-256 figurent sous `cancunBlockCorrectedResults` du manifeste. Cela ne qualifie ni la disponibilité des blobs ni leur
transport, ni une activation réseau, ni les transitions de fork.

## Premier corpus de blocs Cancun et correction PREVRANDAO

Le premier passage des 2 397 fichiers de blocs Cancun se termine avec
**11 861 concordances, 57 écarts de rejet, 19 erreurs d'engagement d'exécution
et 177 cas non pris en charge**. Le rapport initial reste intact dans
`build-cmake/qualification/cancun-block-initial.jsonl`, empreinte et compteurs
sous `cancunBlockInitialResults`. Ce passage retourne 1.

Les 19 erreurs d'exécution proviennent du contexte de l'opcode 0x44 : les
transactions et simulations RPC lisaient `difficulty` même après Paris. Ils
lisent désormais `mixHash` à partir de Paris, conformément à EIP-4399 ; Berlin
et le profil expérimental conservent `difficulty`. L'adaptateur des fixtures
d'état remplit maintenant ces deux champs distinctement, au lieu de transporter
`currentRandom` dans `difficulty`. Le test natif oppose explicitement 7 et 123,
vérifie les transactions signées pour sept profils et les appels RPC pour les
profils réseau disponibles.

Les 19 fichiers complets concernés passent maintenant leurs **23 cas**, sans
erreur ni exclusion. Le nouveau rapport `randao-block-corrections.jsonl` et
son empreinte sont conservés sous `randaoBlockCorrections`. Dix-sept fichiers
sont embarqués ; deux dépassant 10 Mo restent référencés par empreinte dans
`externalRandaoBlockRegressionFiles`. Les suites RPC, fixtures d'état embarquées
et PREVRANDAO passent après correction (18,90 s). La régression de blocs
embarqués passe également après ajout des fichiers concernés. Le passage complet
Cancun corrigé est désormais consigné dans la section précédente.

Le consensus PoS Nano actuel impose un `mixHash` nul : PREVRANDAO renvoie donc
zéro dans ses blocs Paris/Shanghai. Ce correctif ne crée pas de source d'aléa.
Il change la valeur autrefois lue par l'opcode dans ces profils ; aucune base
historique n'est réécrite automatiquement. La suite native compte maintenant
52 tests ; leur passage intégral après les corrections blob et PREVRANDAO
réussit : **52/52 en 317,11 secondes**. Le journal
`build-cmake/qualification/blob-header-full-ctest.log` et son SHA-256 sont
référencés dans le manifeste. Cela couvre les scénarios existants, sans prouver
à lui seul la résistance du réseau dans toutes les conditions de production.

## Cancun : racine Beacon et appel système EIP-4788

Le format d'en-tête sérialise `parentBeaconBlockRoot` (32 octets), après les
champs de gas blob. Dans le contexte `ConsensusInputPolicy::EthereumValidated`,
le moteur appelle le contrat Beacon avec l'adresse système, les 32 octets de
racine et un budget de 30 millions de gas, avant les transactions. Cet appel ne
produit aucun reçu et ne consomme pas le gas du bloc. L'absence de code est
silencieuse ; un revert annule ses écritures et un bloc rejeté annule aussi
l'appel système. Aucun appel n'est exécuté au bloc genesis.

La politique explicite remplace le précédent nom centré sur les retraits : elle
couvre maintenant les racines Beacon et les retraits. Nano utilise toujours
`RejectExternalInputs`. Aucune activation réseau Cancun ni authentification de
racine par un client de consensus Beacon n'est ajoutée : la validation externe
reste une précondition du contexte de fixtures.

**62 / 62 scénarios EIP-4788 concordent** dans huit fichiers officiels complets
ajoutés aux régressions. Le journal est `build-cmake/qualification/beacon-specific.jsonl`,
avec son empreinte dans `beaconBlockResults`. Les six suites ciblées cœur,
authentification des blocs, blobs, régressions de blocs, retraits et Beacon
passent (9,60 s). La suite complète contient désormais 51 tests, sans nouveau
passage intégral revendiqué. Le premier passage du corpus
complet de blocs Cancun est désormais terminé ; son échec et le correctif
PREVRANDAO sont détaillés plus haut.

## Blocs Berlin–Shanghai : corpus épinglé concordant après retraits

Les 465 fichiers de blocs Berlin, Paris et Shanghai concordent désormais :
**8 148 scénarios**, zéro divergence, erreur ou cas exclu, retour 0. Ils rejouent
11896 blocs acceptés et vérifient 744 rejets atomiques. Les anciens diagnostics
incomplets restent conservés. Le résultat courant est `blockWithdrawalResults`
et le journal `build-cmake/qualification/block-withdrawals.jsonl`.

Le moteur implémente l'encodage RLP et la racine MPT des retraits EIP-4895,
indexée par leur position dans le bloc. Les montants uint64 sont convertis de
Gwei en wei et crédités après les transactions, sans EVM ni gas. Les retraits
nuls ne créent pas de compte. Le vérificateur refuse une racine incorrecte avant
mutation d'état. Les 40 cas spécifiques passent, et leurs 12 fichiers complets
rejoignent les régressions.

**Cette exécution exige `ConsensusInputPolicy::EthereumValidated`.** Le
runner choisit explicitement cette politique pour les fixtures. Le constructeur
par défaut, donc les nœuds Nano, utilise `RejectExternalInputs` et refuse tout retrait non
vide. Aucune option RPC ou genesis ne permet cette activation. La validation
Beacon/consensus externe n'est pas implémentée par cette politique : c'est une
précondition du contexte appelant, pas une preuve de dépôt ou un bridge.

Les cinq suites ciblées passent : cœur, authentification des blocs, blobs,
régressions de blocs et retraits. Le test de proposition Nano vérifie le rejet
d'un crédit non autorisé sans mutation de l'état ni du stockage canonique.
La suite complète comporte maintenant 50 tests ; aucun passage intégral de ces
50 tests n'est revendiqué à ce stade.

**La concordance ne prouve pas une conformité complète de fork.** Les sceaux,
la difficulté Ethereum, certains autres contrôles d'en-tête, les transitions,
les corpus de blocs Cancun/Prague/Osaka et la disponibilité des blobs restent à
qualifier. `forkConformanceProven` reste faux et le réseau Nano garde son BFT.

## En-têtes : limites de gas et frais par rapport au parent

Les contrôles de gas et de frais sont regroupés dans
`validate_execution_header`, utilisée par le chemin d'import Nano et le
runner de blocs. Elle vérifie le minimum configuré, la variation strictement
inférieure à `parent.gasLimit / 1024`, le gas consommé et le frais de base
calculé à partir du parent. Une base fee ne peut pas apparaître implicitement
si le parent n'en possède pas : les activations de forks demandent une règle
séparée. Les tests natifs couvrent les deux frontières strictes de variation,
le minimum et les frais manquants/incorrects.

Sur les mêmes 465 fichiers, **8 109 scénarios concordent**, avec **740 rejets
atomiques** dont neuf gas limits invalides et deux frais incorrects. Aucun
écart ni erreur ; **39 cas restent non pris en charge** : 36 scénarios de
retraits et trois contrôles de leur racine d'en-tête. Le diagnostic retourne 1.
Le manifeste `blockParentHeaderResults` conserve les résultats et l'empreinte
de `build-cmake/qualification/block-parent-headers.jsonl`. Cinq fichiers
complets supplémentaires rejoignent les régressions.

Le passage intégral des 49 suites natives donne 48 réussites et un échec
`block_auth` (357,89 s), dû à trois assertions attendant l'ancien texte
« invalid PoS gas limits ». Après mise à jour vers le texte commun, `block_auth`
repasse seul (2,46 s). Aucun code de production n'a changé entre les deux
passages. Les deux journaux et l'échec initial sont conservés dans le manifeste ;
il ne s'agit pas d'un nouveau passage intégral vert des 49 suites.
Les sceaux de consensus, les transitions de fork, les retraits et les profils
post-Shanghai restent à qualifier ; les résultats ci-dessus ne certifient pas
une chaîne Ethereum ni une préparation production globale.

## Rejets de transactions dans les blocs : 729 rejets atomiques vérifiés

Le diagnostic étendu couvre les transactions invalides directement depuis le
RLP des blocs, avec les comptes et l'ascendance des fixtures. Chaque rejet doit
avoir le motif exact attendu et préserver intégralement l'état ; le bloc refusé
ne fait pas avancer l'ascendance. Un rejet inattendu ne peut pas satisfaire un
cas négatif. Le moteur distingue désormais explicitement nonce trop bas et
nonce trop haut, sans changer leur rejet.

Sur les mêmes 465 fichiers, **8 098 scénarios concordent**, dont **729 rejets** :
674 gas intrinsèque insuffisant, 32 signatures invalides, 8 chain IDs invalids,
3 soldes insuffisants, 3 dépassements de gas du bloc, 3 nonces trop bas,
3 nonces trop hauts, 2 initcodes trop longs et 1 type 2 avant activation.
Aucune divergence ni erreur. **50 scénarios restent non pris en charge** :
14 validations d'en-tête et 36 impliquant des retraits. Le diagnostic conserve
un retour 1. La catégorisation des cas mixtes explique le passage de 35 à 36
scénarios de retraits après suppression de l'exclusion générale des blocs invalides.

Le bilan précédent reste conservé. Le nouveau résultat est dans
`blockRejectionResults`, avec l'empreinte de
`build-cmake/qualification/block-rejections.jsonl`. Vingt-quatre fichiers
supplémentaires complets avec cas négatifs rejoignent les régressions embarquées.
Les suites ciblées `core`, `authorization`, `ethereum_official_regressions` et
`ethereum_block_regressions` passent après ces changements ; journaux référencés
dans `blockRejectionResults.targetedTests`.
Les limites de qualification des en-têtes, consensus, retraits et transitions
ci-dessous restent applicables.

## Premier diagnostic des blocs : périmètre partiel et échec global

`nano-ethereum-block-runner` lit les transactions signées dans le RLP du bloc,
vérifie l'encodage des en-têtes et des oncles, l'état genesis et l'ascendance
linéaire, puis utilise `ExecutionEngine::verify_block` pour comparer les racines
de transactions/reçus/état, le bloom, le gas et le hash final. Il compare aussi
les reçus RLP fournis et l'état final du scénario. Les signatures historiques
non protégées restent autorisées uniquement dans ce contexte de fixtures.

Premier passage des 465 fichiers de blocs Berlin, Paris et Shanghai :

| Groupe | Scénarios concordants | Non pris en charge |
| --- | ---: | ---: |
| Berlin | 2 283 | 156 |
| Paris | 2 447 | 287 |
| Shanghai | 2 639 | 336 |
| Total | 7 369 | 779 |

Les scénarios concordants rejouent 11 830 blocs. Aucune divergence ni erreur
sur ces scénarios ; **le diagnostic retourne néanmoins 1** à cause des cas
non pris en charge : 744 scénarios avec rejet de bloc attendu et 35 avec
retraits non vides. Les décomptes et l'empreinte du journal sont conservés dans
`blockExecutionInitialResults`, journal `build-cmake/qualification/block-initial.jsonl`.
Six fichiers officiels complets composent une régression positive distincte,
`ethereum_block_regressions`, qui passe. Cette sélection ne masque pas l'échec
du corpus complet.

**Il s'agit de concordance des engagements d'exécution, pas de validation
complète des blocs.** La validation des en-têtes relativement au parent
(difficulté, frais, limites de gas, temps), les sceaux de consensus, les cas
invalides, les retraits, les forks ultérieurs et les transitions restent à
qualifier. Nano n'a pas de consensus Beacon Ethereum : appliquer ses retraits
à un réseau Nano nécessitera une règle explicite de consensus et ne doit pas
permettre à un producteur de créer des crédits arbitraires.

## Transactions signées malformées : 110 cas EIP-7702

Les 24 fichiers `transaction_tests` de l'archive épinglée contiennent uniquement
110 transactions EIP-7702 invalides : 55 Prague et 55 Osaka. Ils sont conservés
intégralement dans `tests/fixtures/ethereum-transactions`, avec leurs empreintes
sous `bundledTransactionRegressionFiles` du manifeste.

Le nouveau binaire `nano-ethereum-transaction-runner` vérifie le décodage réel
des enveloppes et, lorsque le décodage réussit, la validation d'exécution dans
un contexte explicitement synthétique (compte financé, nonce connu). Il exige
un motif de rejet attendu et l'état inchangé. Les autres formats, forks et cas
positifs sont signalés comme non pris en charge, avec retour non nul ; ils ne
sont jamais comptés comme réussites.

Premier passage : **94 concordances et 16 différences de motif**, toutes les
transactions étant déjà refusées. Les champs r/s trop larges et y_parity hors
format produisaient une erreur générique. Les erreurs distinguent désormais
la largeur des éléments de signature de l'encodage RLP non canonique. Les règles
d'acceptation restent identiques, notamment l'ignorance des autorisations aux
signatures non valides mais structurellement encodables selon EIP-7702.

Passage final : **110 / 110 concordances**, aucune erreur ni exclusion. Les deux
journaux restent dans `build-cmake/qualification/transaction-{initial,final}.jsonl`
et leurs empreintes dans `transactionInitialResults` et `transactionResults`.
Les quatre suites ciblées `core`, `authorization`, `ethereum_official_regressions`
et `ethereum_transaction_regressions` passent après le changement. La suite
complète comporte désormais 48 tests ; son dernier passage intégral de 47 tests
précède cette addition et ne doit pas être présenté comme un passage de 48 tests.

Cela ne constitue pas une couverture positive des types de transactions, une
qualification de l'admission réseau, ni une conformité complète de fork.

## Admission RPC Shanghai : diagnostic Anvil 1.5.1

Le profil Paris du comparateur repasse ses 14 transactions identiques sans
échec (`osakaParisAnvilComparison`, journal `osaka-paris-anvil.log`).

Le comparateur vérifie désormais les enveloppes signées de types 0, 1 et 2
avec 49 153 octets d'initcode. Pour chaque type, Nano refuse immédiatement,
Anvil renvoie un hash mais ne produit aucun reçu après minage. Les soldes et
nonces canoniques restent inchangés ; le nonce pending Nano reste inchangé.
Une même transaction de remplacement signée au même nonce est ensuite incluse
avec succès sur chaque chaîne et consomme 21 000 gas.

Les 22 autres transactions Shanghai concordent toujours en statut, logs et gas,
avec comparaisons du code déployé et des lectures d'état sélectionnées.
Le test **retourne toujours 1** pour préserver le désaccord d'admission RPC.
Le rapport et la version exacte figurent dans `osakaShanghaiAnvilComparison`
et `build-cmake/qualification/osaka-shanghai-anvil-admission.log`.

L'[EIP-3860](https://eips.ethereum.org/EIPS/eip-3860) impose l'invalidité des
transactions de création au-delà de 49 152 octets d'initcode. Dans le
[code du commit installé d'Anvil](https://github.com/foundry-rs/foundry/blob/b0a9dd9ceda36f63e2326ce530c10e6916f4b8a2/crates/anvil/src/eth/backend/mem/mod.rs#L3383-L3522),
`validate_pool_transaction_for` vérifie notamment nonce, frais, soldes et blobs,
mais ne vérifie pas cette longueur. Cette lecture explique le comportement
observé à l'admission ; l'exclusion du bloc est vérifiée expérimentalement.
Nano conserve son rejet anticipé. Ce diagnostic ne prouve ni l'égalité générale
des RPC ni la conformité complète du consensus de l'une ou l'autre chaîne.

## Bilan Prague courant après EIP-7702

Le corpus d'état complet `for_prague` concorde : **13 074 / 13 074 cas**, dont
**785 rejets vérifiés**, dans 2 466 fichiers, zéro divergence, erreur ou cas
ignoré. Le processus retourne 0. Le résultat par groupe et l'empreinte du rapport
sont dans `pragueStateResults`, indiqué par `latestPragueResult` du manifeste.
Le rapport local est `build-cmake/qualification/prague-authorization-final.jsonl`.
Les diagnostics précédents restent conservés, notamment le premier passage
EIP-7702 à 13 073 concordances et un motif de rejet différent.

[EIP-7702](https://eips.ethereum.org/EIPS/eip-7702) ajoute les enveloppes signées
de type 4, leurs autorisations JSON/RLP et reçus, les signatures d'autorisation
avec séparation de domaine, les limites de champs, les nonces et chaînes, les
remboursements, la mise en place et la révocation du code délégué. Les écritures
d'autorisation et leur remboursement survivent à un échec de l'exécution ; les
écritures du code exécuté sont annulées normalement. Le contexte de stockage
reste celui de l'autorité. Les appels suivent un seul pointeur, les pointeurs
vers les précompiles exécutent du code vide, et les accès EXTCODE restent ceux
de l'indicateur. Les adresses récupérées sont normalisées avant préchauffage.

Les 2 195 cas du sous-ensemble spécifique Prague passent aussi : 978 BLS,
489 plancher calldata et 728 EIP-7702. 72 fichiers complets EIP-7702 et sept
EIP-7623 sont embarqués ; deux fichiers volumineux restent dans l'archive
épinglée avec leurs SHA-256. Aucun cas n'est retiré du passage complet.
Le test natif `authorization` protège signature, round-trips, exécution,
persistance après revert, remboursement, révocation, nonces/chaînes invalides,
high-s, préchauffage canonique et rejet avant Prague. La validation du chain ID
précède celle de la liste d'autorisations vide, conformément au cas officiel.

Les **46 suites CTest passent**. Après le dernier correctif d'ordre de validation,
`authorization`, `rpc` et `ethereum_official_regressions` passent à nouveau,
ainsi que les cinq variantes du fichier de chain ID invalide. Le RPC refuse les
transactions de type 4 et leur simulation avant activation, sans mutation d'état
ni du nonce pending. La comparaison Anvil Shanghai retrouve ses 22 transactions
concordantes ; son désaccord d'admission d'initcode reste un échec explicite.

**Prague reste indisponible dans le genesis réseau.** La concordance de ce corpus
ne couvre pas les tests de blocs, les transitions complètes, les appels système,
les règles de propagation/mempool EIP-7702, les blobs ni la qualification globale
BFT/stockage/charge. `forkConformanceProven` reste faux. Le corpus Osaka est
extrait de la même archive épinglée ; son profil reste à implémenter.

## Bilan Prague après BLS12-381

Le passage complet `for_prague` donne **12 259 concordances sur 13 074**,
**47 divergences**, **768 erreurs** et aucun cas filtré/sauté dans 2 466 fichiers.
707 rejets concordent. Toutes les divergences restantes appartiennent au groupe
EIP-7702 ; les 768 erreurs indiquent un type de transaction non pris en charge.
Le processus retourne 1 et `forkConformanceProven` reste faux. Les résultats
par groupe et le SHA-256 du rapport sont dans `pragueBlsResults`, indiqué par
`latestPragueResult`. Le diagnostic initial ci-dessous reste conservé.
Rapport local : `build-cmake/qualification/prague-bls-state.jsonl`.

Les sept précompiles [EIP-2537](https://eips.ethereum.org/EIPS/eip-2537), adresses
`0x0b` à `0x11`, utilisent le blst déjà épinglé pour KZG, sans modification
amont ni nouveau setup : additions G1/G2, MSM, appariement et maps Fp/Fp2.
Les champs sont canoniques sur 64 octets, l'infini est accepté, les additions
vérifient la courbe sans imposer le sous-groupe, tandis que MSM et appariements
imposent le sous-groupe. Les scalaires entiers de 256 bits sont acceptés. Les MSM
utilisent Pippenger, avec multiplication directe pour un seul point. Les tables
de réduction de gas sont distinctes pour G1 et G2. Les appels invalides brûlent
leur budget et les adresses sont préchauffées seulement à partir de Prague.

**978 / 978 cas BLS officiels concordent** dans les 53 fichiers complets : zéro
divergence, erreur ou cas ignoré. 52 fichiers sont embarqués ; le fichier G2 MSM
valide de 14 Mo reste dans l'archive épinglée avec son SHA-256 au manifeste.
Le test natif `bls` vérifie aussi gas exact/insuffisant, encodages invalides,
longueurs invalides/vides, infini et activation du fork. Les deux cas officiels
à transactions de plus de 1 Mio passent après retrait de la limite arbitraire
du décodeur générique. La limite de 1 Mio est conservée comme politique locale
de mempool, rejetée tôt en RPC, et non comme règle de décodage de consensus.
Les plafonds HTTP/WebSocket restent inchangés. `chain` vérifie qu'une enveloppe
volumineuse se décode mais est refusée par la pool sans modifier le nonce.

Les **45 suites CTest passent**, dont les nouveaux cas BLS embarqués. Après
l'ajustement final de la politique d'admission, `chain` et `rpc` ont été
recompilés et relancés avec succès. Journaux locaux :
`build-cmake/qualification/bls-ctest.log` et `bls-policy-tests.log`.
L'intégration BLS ne qualifie pas encore les ressources sous charge prolongée.
EIP-7702, les règles complètes de blocs et transitions, puis Osaka restent
ouverts. Prague reste refusé par le genesis réseau.

## Premier bilan Prague : implémentation partielle

Le profil de diagnostic `EvmRevision::Prague` est distinct de Cancun et reste
refusé par la configuration genesis réseau. Le corpus complet `for_prague` de
la même archive épinglée a été exécuté : **11 069 / 13 074 concordances** dans
2 466 fichiers, **1 235 divergences**, **770 erreurs**, zéro cas filtré ou sauté.
707 rejets attendus concordent. Le processus retourne **1**. Le détail par groupe
et le SHA-256 du rapport figurent dans `pragueInitialResults` du manifeste ; le
rapport local est `build-cmake/qualification/prague-state.jsonl`.

Deux évolutions Prague sont ajoutées à ce stade :

- [EIP-7623](https://eips.ethereum.org/EIPS/eip-7623) : validation du plancher
  calldata et application après exécution/remboursement. Le plancher ne diminue
  pas le budget EVM initial. Le groupe complet donne 395 concordances et 94
  erreurs dues aux transactions de type 4, sans autre divergence. Trois fichiers
  complets de validation des types 0–3 (372 cas) sont intégrés aux régressions.
- [EIP-7691](https://eips.ethereum.org/EIPS/eip-7691) : dénominateur de frais blob
  5 007 716, maximum neuf blobs par transaction/bloc en Prague ; Cancun conserve
  ses paramètres. Le test natif vérifie qu'un même bloc signé à huit blobs est
  refusé en Cancun et accepté en Prague, ainsi que les frontières neuf/dix blobs
  et l'absence de mutation lors d'un rejet. La transition de l'excès de gas à
  partir du parent reste à implémenter et qualifier.

Les erreurs comprennent 768 types de transaction non pris en charge et deux
tailles d'enveloppe invalides. Les précompiles BLS12-381 EIP-2537 et la délégation
EIP-7702 restent à implémenter. Leurs absences affectent aussi des tests
historiques d'appels, d'accès aux précompiles et de création. Les huit divergences
`stCreate2` et celle de `stSpecialTest` restent à diagnostiquer séparément.
Aucun de ces échecs n'est transformé en succès ni supprimé du corpus.

Les cinq suites ciblées `core`, `ethereum_official_regressions`,
`evm_small_stack`, `blobs` et `kzg` passent après les changements Prague.
La suite de fixtures embarquées a ensuite été relancée avec les 372 cas ajoutés
et passe. Le résultat 44/44 ci-dessous est celui de l'étape Cancun précédente ;
la suite réseau complète n'a pas été relancée pour cette étape de moteur.

## Bilan Cancun courant : corpus d'état concordant

Le groupe `for_cancun` est parcouru intégralement : **10 777 cas concordants sur
10 777**, dont **583 rejets vérifiés**, zéro divergence, zéro erreur d'adaptateur
et zéro cas ignoré, dans 2 332 fichiers. Le processus retourne 0. Le résultat
par groupe et le SHA-256 du rapport sont dans `cancunStateResults` du manifeste,
indiqué par `latestCancunResult`. Les bilans précédents restent conservés.
Le rapport détaillé local est dans `build-cmake/qualification/cancun-state.jsonl`.

Les 52 divergences du bilan KZG précédent ont été corrigées : sortie vide
ECRECOVER en cas de signature invalide, CALLCODE avec valeur autorisé en contexte
statique, nonce maximal refusé (EIP-2681), MODEXP à longue déclaration d'exposant
avec base et modulus de longueur nulle, et extension mémoire facturée avant
allocation sans l'ancien plafond arbitraire de 1 Mio. Les attentes du prototype
corrigées restent traçables dans `tests/fixtures/precompiles.json`.

Les 40 fichiers officiels complets contenant ces divergences couvrent 246 cas
concordants. 32 sont ajoutés aux régressions embarquées ; huit fichiers aléatoires
volumineux restent dans l'archive épinglée avec chemins et SHA-256 individuels.
Aucun cas n'est retiré du passage complet. Berlin, Paris et Shanghai ont été
réexécutés après ces corrections et restent concordants aux nombres ci-dessus.

Les **44 suites CTest passent** après ces changements, y compris les régressions
Ethereum embarquées, les précompiles, les contrats OpenZeppelin et les scénarios
BFT existants. Le journal est conservé dans `build-cmake/qualification/ctest.log`.
Cela couvre ces scénarios bornés, pas une qualification générale sous charge.

La comparaison Shanghai avec Anvil 1.5.1 retrouve 22 transactions identiques en
résultat et gas. Elle reste en échec explicite sur l'admission RPC de l'initcode
de 49 153 octets : Nano le refuse immédiatement ; Anvil retourne un hash puis
l'exclut du bloc, sans mutation canonique, et accepte une transaction valide au
même nonce. Le rapport reste dans `build-cmake/qualification/shanghai-anvil.log`.

**Ce résultat ne prouve pas la conformité complète Cancun ni la préparation
production.** Les formats de tests de blocs et transactions, les transitions de
forks, l'activation réseau Cancun et le transport/stockage des blobs restent à
qualifier. Le plafond MODEXP connu lors de ce bilan a ensuite été retiré à
l’étape Osaka décrite plus haut. Les qualifications réseau, BFT, reprise et
charge restent ouvertes. `forkConformanceProven` reste faux.

## Cancun : implémentation partielle et correction des appels profonds

Le moteur de diagnostic accepte désormais `EvmRevision::Cancun`, sans accepter
Cancun comme configuration de genesis réseau. Les règles ajoutées sont
[EIP-1153](https://eips.ethereum.org/EIPS/eip-1153) (stockage transitoire),
[EIP-5656](https://eips.ethereum.org/EIPS/eip-5656) (MCOPY) et
[EIP-6780](https://eips.ethereum.org/EIPS/eip-6780) (SELFDESTRUCT).
Les écritures transitoires sont propres à une transaction, partagées suivant
le contexte CALL/DELEGATECALL et annulées lors d'un revert. SELFDESTRUCT ne
supprime plus les comptes préexistants ; les créations de la transaction
restent suivies à travers les sous-appels et leurs annulations.

Les groupes officiels complets ajoutés à CTest couvrent 123 cas de stockage
transitoire, 99 cas MCOPY et 136 cas SELFDESTRUCT : tous concordent. Les 41
fichiers sont copiés sans filtrage, avec leurs empreintes dans le manifeste.

Le corpus élargi a aussi révélé un crash de pile native dans
`call1024_balance_too_low`. Le diagnostic LLDB s'arrêtait dans
`___chkstk_darwin` ; la fonction récursive non optimisée réservait environ
9 Kio par niveau. Les appels EVM utilisent désormais des coroutines C++20 à
transfert symétrique, avec les contextes alloués sur le tas, sans threads
supplémentaires. La limite vérifie la profondeur du prochain appel suivant
la [spécification d'exécution](https://raw.githubusercontent.com/ethereum/execution-specs/master/src/ethereum/forks/cancun/vm/instructions/system.py).
Le décalage d'un niveau de l'ancienne vérification est corrigé pour CALL et CREATE.

La fixture profonde est conservée entière dans les régressions. Le nouveau
test `evm_small_stack` l'exécute deux fois dans un thread POSIX configuré avec
une pile de 256 Kio : racine d'état et reçu RLP concordent, sans crash.
Cette correction change la sémantique à la limite des appels ; elle ne doit pas
être appliquée silencieusement à un historique existant sans règles d'activation.

Les 41 tests préexistants passent après reconstruction, ainsi que le nouveau
test de pile limitée. Les groupes Berlin (1 963), Paris (2 250) et Shanghai
(2 441) restent intégralement concordants. La comparaison Anvil conserve ses
22 exécutions Shanghai concordantes et son désaccord d'admission RPC documenté.

Le premier passage intégral Cancun après suppression du crash, avant correction
du décalage de profondeur, obtenait 9 545 concordances, 319 divergences et
913 erreurs sur 10 777 cas. Le passage final complet après correction donne
**9 571 concordances, 293 divergences et 913 erreurs**, sans cas ignoré ni
crash. Les 26 divergences de profondeur corrigées ne changent pas les cas
concordants en échecs. Le processus retourne 1, car la qualification reste
incomplète. Le détail par groupe figure dans `cancunPartialResults` du manifeste.
Ces résultats ont identifié les transactions blob de type 3, le précompile
KZG, BLOBHASH/BLOBBASEFEE, la validation, les limites mémoire et d’autres écarts
du corpus historique élargi. Les étapes suivantes ci-dessous traitent une
partie de ces écarts ; ce compteur est conservé comme diagnostic historique. Les copies d'état par appel doivent aussi
être qualifiées en coût mémoire/temps ; l'absence de crash de pile ne prouve
pas une résistance complète à l'épuisement de ressources.

## Transactions blob Cancun : exécution en qualification

Le décodage, la signature, les représentations JSON/RLP et les reçus prennent
maintenant en charge les transactions de type 3. Le moteur Cancun vérifie le
destinataire, le nombre et la version des hashes, le plafond de frais blob et
la capacité financière avec les plafonds des deux marchés de gas. Le prix blob
suit l'exponentielle entière de [EIP-4844](https://eips.ethereum.org/EIPS/eip-4844).
Les frais blob effectifs sont déduits avant exécution et ne sont pas remboursés
lors d'un revert ; ils ne sont pas versés au producteur. BLOBHASH et BLOBBASEFEE
sont disponibles seulement à partir de Cancun.

Les champs optionnels blobGasUsed/excessBlobGas sont sérialisés dans l'en-tête ;
les en-têtes antérieurs sans ces champs gardent leur encodage. Le moteur de bloc
limite le total à six blobs sous Cancun et refuse atomiquement un dépassement.
Cette étape ne qualifie pas encore tous les champs et traitements Cancun des
blocs : la transition d'excessBlobGas à partir du parent, EIP-4788, la
disponibilité et la propagation des blobs restent notamment à intégrer ; la
vérification ponctuelle KZG est ajoutée dans l’étape suivante.
Le calcul défensif du prix rejette actuellement un résultat dépassant uint256 ;
cette limite reste à qualifier avec la validation des en-têtes avant activation.

Avant ajout de KZG, le sous-ensemble `for_cancun/cancun` donnait 1 276
concordances et 180 divergences,
sans erreur ni cas ignoré. Les 180 divergences concernent les fichiers du
précompile KZG alors absent. Les 16 fichiers complets de transactions blob,
BLOBHASH et BLOBBASEFEE (918 cas) sont conservés dans les régressions.
Les autres erreurs de validation sont classées par motifs exacts (frais,
expéditeur avec code, limite de gas), avec état inchangé et racine vérifiés.

Le test natif `blobs` couvre les encodages signés/en-têtes/reçus, les frais après
succès et revert, le refus pré-Cancun et le plafond de blobs par bloc. Le test
RPC vérifie le refus des envois, eth_call et eth_estimateGas de type 3 avant
activation, sans changer l'état ni le nonce pending. Cancun reste indisponible
comme configuration réseau ; le code de transport des blobs n'est pas annoncé
comme implémenté par l'ajout de ces formats.

## Précompile KZG Cancun

Le précompile `0x0a` utilise désormais c-kzg-4844 **v2.1.8**, avec son sous-module
blst épinglé. Les sources requises sont conservées avec leurs licences et
empreintes individuelles dans `third_party/ckzg/UPSTREAM.json`. Le setup public
Ethereum du même commit est vérifié par SHA-256 lors de la configuration CMake,
puis intégré au binaire : aucune dépendance à un chemin de fichier au lancement.
CMake et Make compilent les bibliothèques statiques ; les deux chemins ont été
vérifiés sur macOS arm64.

Le traitement vérifie la longueur de 192 octets, le hash versionné du commitment,
les éléments de corps canoniques et la preuve KZG. Une réussite consomme 50 000
gas ; un échec consomme le budget de l'appel. Les **180 cas officiels** de preuves,
d'entrées invalides, de types d'appels et de gas concordent. Les six fichiers
complets rejoignent les régressions CTest. Le sous-ensemble spécifique
`for_cancun/cancun` atteint **1 456 concordances sur 1 456**, sans erreur ni cas
ignoré. Cela ne remplace pas la qualification du corpus complet.

Le test natif `kzg` vérifie aussi quatre premières utilisations simultanées,
une preuve valide à l'infini, les rejets d'entrées altérées, le gas insuffisant
et l'activation par fork. Paris et Shanghai traitent toujours cette adresse
comme un compte ordinaire ; le comportement expérimental historique est
conservé. Le réseau Nano n'accepte toujours pas de genesis Cancun ni de
transaction blob. La propagation, la disponibilité et le stockage des blobs
restent distincts de ce vérificateur cryptographique.

## Vérification complémentaire Shanghai

L'inventaire intégral des 8 117 fichiers de tests d'état de la même archive
est conservé dans `tests/fixtures/ethereum-state-inventory.json`, avec son SHA-256.
Il retrouve quatre cas Shanghai dans trois fichiers du groupe
`for_shanghaitocancunattime15k`. Ces quatre cas passent, dont deux rejets de
transactions blob avant activation : **2 445 cas Shanghai concordants au total**.
Les trois fichiers sont ajoutés entiers à la régression CTest, qui passe.
Il s'agit des règles avant activation, pas d'une preuve de transition d'un
réseau existant vers Cancun.

Le pilote reconnaît désormais le rejet d'une transaction de type 3 sous
Shanghai et les alternatives exactes séparées par `|` dans les fixtures.
Le classement exige simultanément le message natif, le fork et le type brut ;
un type inconnu n'est pas assimilé à ce rejet. Trois témoins négatifs (motifs
non concordants, type 4 au lieu de 3, racine d'état altérée) échouent comme prévu.
Aucune règle du moteur ou du réseau n'a changé pour obtenir ce résultat.

La comparaison avec Anvil **1.5.1** en mode Shanghai exécute les mêmes
**22 transactions signées** sur les deux nœuds. Statuts, logs et gas concordent ;
le code déployé et les lectures d'état ciblées sont comparés aussi. Elle ajoute
PUSH0, l'accès chaud au coinbase, et les tailles d'initcode 31, 32, 33 et 49 152.
Les contrats Solidity restent compilés Paris ; le bytecode spécifique Shanghai
est fourni explicitement dans le test.

Un désaccord d'admission RPC reste visible pour 49 153 octets d'initcode.
Nano rejette l'envoi sans changer le nonce pending ; Anvil 1.5.1 retourne un
hash. Le test force ensuite `evm_mine` : aucun reçu pour la transaction invalide,
nonce et solde canoniques inchangés. Une transaction valide au même nonce est
ensuite minée avec succès. Le désaccord observé concerne donc l'admission au
RPC ; aucune inclusion du déploiement invalide n'a été observée.
Le rejet de Nano reste conforme à [EIP-3860](https://eips.ethereum.org/EIPS/eip-3860).
Aucune limite de Nano n'est relâchée pour reproduire la politique d'Anvil.

Le rapport expose `referenceValidationMatches: false` et le comparateur retourne
un échec dans ce cas. Les 22 transactions concordantes, l'exclusion au minage et
la reprise au même nonce sont des vérifications distinctes ; ce résultat n'est
pas une concordance complète des validations RPC.

Commande reproductible, avec des binaires absolus et uniquement des chaînes temporaires :

```sh
NANO_TEST_EVM_FORK=Shanghai ANVIL_BINARY=/chemin/vers/anvil \
NANO_BINARY=/chemin/vers/nano node tests/hardhat/run-local.mjs scripts/compare-anvil.ts
```

### Périmètre suivant à qualifier

L'inventaire recense notamment **10 777 cas Cancun**, **13 074 cas Prague** et
**13 739 cas Osaka**, ainsi que neuf cas Cancun dans le groupe de transition
vers Prague. Ces nombres sont un inventaire, pas des tests passés. Cancun est
désormais partiellement implémenté ; Prague et Osaka restent non pris en charge.
Le passage initial, avant implémentation, du groupe `for_cancun` comptait
2 332 fichiers, 10 777 cas `unsupported-fork`, zéro cas exécuté et un code de
sortie 1. Ce diagnostic est conservé dans le manifeste ; il ne constitue pas
une mesure de concordance Cancun. Les autres versions
historiques sont également inventoriées. Les tests de transactions et de blocs
restent un périmètre distinct à qualifier ; les formats Engine Ethereum ne
constituent pas une interface actuellement annoncée par Nano.

Shanghai est désormais sélectionnable avec `config.evmFork: "Shanghai"`.
La limite et le coût intrinsèque de l'initcode sont partagés entre moteur,
mempool, eth_call et eth_estimateGas. Le genesis engage ces règles avec un
domaine distinct de Paris ; les genesis livrés ne sont pas modifiés. Le test
`chain` vérifie la limite exacte, le rejet à un octet au-delà, PUSH0, l'égalité
estimation/reçu, le replay par un pair et la réouverture avec audit.

Après raccordement, six suites CTest (core, chain, rpc, staking, staking_paris,
ethereum_official_regressions) passent. Le parcours Hardhat `publish.ts` passe
aussi sur un nœud temporaire Shanghai : déploiement ERC20, offre de 1 000 000,
transfert de 125, refus d'un transfert non autorisé et blocs certifiés.
Ce contrat reste compilé avec la cible Paris ; le test natif de création PUSH0
couvre séparément l'instruction introduite par Shanghai.

Le diagnostic initial Shanghai avait 1 940 cas concordants et 501 divergences.
Les corrections ajoutent PUSH0, le bénéficiaire du bloc aux adresses chaudes,
et EIP-3860 : plafond de 49 152 octets pour l'initcode, deux unités de gas par
mot de 32 octets pour les transactions de création et CREATE/CREATE2. Un initcode
trop grand invalide la transaction de création avant mutation de l'état, ou
provoque un échec exceptionnel dans CREATE/CREATE2. Les règles Paris de frais,
de refus des expéditeurs contenant du code et de préfixe 0xEF sont conservées.
Références : [EIP-3855](https://eips.ethereum.org/EIPS/eip-3855),
[EIP-3651](https://eips.ethereum.org/EIPS/eip-3651),
[EIP-3860](https://eips.ethereum.org/EIPS/eip-3860).

Les huit fichiers officiels spécifiques à ces trois EIP sont intégrés sans
filtrage dans la régression CTest. Berlin et Paris ont été rejoués après ces
changements : les 1 963 et 2 250 cas restent concordants. La comparaison porte
sur les groupes de tests d'état sélectionnés ; les formats de blocs et les
transitions temporelles de règles ne sont pas couverts par ce résultat.

Ces groupes sont exécutés intégralement, sans filtres de cas, avec le pilote C++.
Les rejets comparent le motif exact mappé, l'état intégral inchangé, sa racine
et les logs vides. Trois témoins négatifs (motif erroné, rejet inattendu, racine
erronée) échouent tous comme attendu. Un message d'erreur inconnu n'est jamais
compté comme rejet conforme.

Le pilote demande explicitement la politique de signature Ethereum historique.
La politique est indépendante de la version EVM ; celle du réseau Nano exige
par défaut une signature liée à son identifiant, y compris avec Paris. Une
signature protégée pour une autre chaîne reste refusée dans les deux politiques.

Ce résultat ne couvre pas tous les tests d'état de l'archive, les tests de blocs,
les formats Engine, les versions plus récentes ni les propriétés du réseau BFT.
`forkConformanceProven` reste faux. Les sections ci-dessous conservent les étapes
du diagnostic ; leurs compteurs antérieurs ne sont pas le bilan courant.


L'objectif est une conformité vérifiable aux règles Ethereum versionnées, pas
une simple compatibilité avec quelques contrats. Nano conserve son consensus
BFT propre ; la conformité EVM ne signifie pas qu'il devient un client du réseau
Ethereum ni qu'il implémente son consensus.

## Corpus indépendant

La release officielle `ethereum/execution-specs`, `tests@v20.0.0`, est épinglée
avec URL, taille et SHA-256 dans `tests/fixtures/ethereum-official.json`.
L'archive a été téléchargée et son empreinte vérifiée. Un premier diagnostic natif du groupe Berlin est
désormais exécuté, avec le bilan incomplet détaillé plus bas. Les tests ne doivent pas être comptés comme passés simplement
parce qu'ils ont été téléchargés, parsés ou compilés.

Le pilote C++ devra recenser chaque cas/fork et comparer l'état attendu, les logs,
les erreurs d'exécution et les erreurs de validation. Un format ou fork non pris
en charge doit être comptabilisé explicitement comme couverture incomplète.
Aucun filtre ne doit rendre invisible un échec connu dans une qualification.
Les règles Paris servent de premier jalon, mais les forks plus récents font
partie du travail restant ; les définir et les activer explicitement est requis.

## Preuves acquises et limites

La suite locale de 37 tests passe après correction de la double facturation du
stockage et de REVERT. La comparaison Anvil Paris de quatorze transactions signées identiques
(ERC20 et stockage Solidity) concorde sur statuts, logs, état observé et gas. Ces résultats sont
décrits dans OPENZEPPELIN-QUALIFICATION.md ; ils ne prouvent pas la conformité
complète. Les historiques expérimentaux antérieurs ne sont pas migrés.

Les écarts restant à traiter incluent notamment la comptabilité
originale/courante de SSTORE et ses remboursements, ainsi que le traitement
économique des frais EIP-1559. La sémantique des forks,
précompiles, créations, auto-destructions et types de transactions doit également
être confrontée au corpus. La correction d'un cas ne dispense pas des autres.

## Conditions pour une déclaration de conformité

- Règles et forks annoncés explicitement, liés à l'identité/configuration du réseau.
- Corpus officiel épinglé entièrement inventorié et exécuté pour ces règles.
- Aucun échec, cas ignoré ou format non supporté dans le périmètre annoncé.
- Résultats reproductibles et comparaisons indépendantes des divergences corrigées.
- Déploiements Solidity/Hardhat et comportements des contrats OpenZeppelin testés.
- Qualification séparée du consensus, du transport, du stockage et des RPC.

La préparation d'un réseau public demeure non acquise tant que ces conditions
et les autres étapes du projet restent ouvertes.

## Premier diagnostic natif du groupe Berlin

`nano-ethereum-state-runner <fichier-ou-dossier>` exécute les transactions brutes
signées des fixtures d'état et compare la racine d'état, le hash des logs et le
reçu RLP lorsqu'il est présent. Il utilise les règles actuelles de Nano, encore
non versionnées par fork : un cas concordant ne prouve donc pas la conformité
complète au fork indiqué. Les cas à exception attendue sont pour l'instant
signalés non pris en charge plutôt que classés arbitrairement comme réussis.

Le groupe officiel `state_tests/for_berlin` contient 127 fichiers, soit 1 963 cas :
1 584 concordances, 170 divergences, 150 validations non prises en charge et 59
erreurs liées à l'identifiant de chaîne. Le processus retourne un code non nul,
ce qui est attendu tant que le corpus n'est pas entièrement compatible.
Le rapport détaillé local est conservé dans le cache des fixtures.

Les premiers correctifs couvrent la double facturation des accès aux comptes,
les frais de CALL/STATICCALL/DELEGATECALL, le retour du stipend de 2 300 gas lors
d'un appel refusé avant exécution, et le préchauffage des adresses et clés d'une
liste d'accès signée. Les précompiles 1 à 9 sont initialement chauds ; le coinbase
n'est plus préchauffé implicitement dans ces règles antérieures à Shanghai.
Onze fichiers officiels complets, soit 123 cas, sont conservés avec leur
licence CC0 et leurs empreintes. `ethereum_official_regressions` les exécute dans
CTest. Ce sous-ensemble ne remplace jamais le bilan du corpus complet.

## Corrections de création de contrats

CREATE et CREATE2 ne remboursent pas le stipend réservé aux appels avec valeur.
CREATE2 facture le hachage de chaque mot de code initial. La création refuse
le dépassement du nonce 64 bits du créateur ; une collision d'adresse consomme
le budget transférable au constructeur, conserve la réserve du parent et
avance le nonce. L'adresse de création reste chaude après une collision ou
un constructeur échoué. Les fixtures officielles de dépôt de code épuisant le
gas et de lecture EXTCODEHASH dans les constructeurs sont ajoutées entières.

Ces modifications changent potentiellement les racines d'état et nécessitent
une activation versionnée avant tout usage sur un historique existant.

## MODEXP et copies mémoire

Le tarif de MODEXP est corrigé selon EIP-2565 : carré du nombre de mots de huit
octets, itérations calculées avec la longueur et le premier mot de l'exposant,
division par trois et minimum de 200 gas. Le tarif est vérifié avant de lire
l'exposant complet et d'exécuter l'exponentiation. La limite locale de taille
reste à qualifier ; ceci ne prouve pas une prise en charge de tous les forks.

EXTCODECOPY et RETURNDATACOPY facturent maintenant trois gas par mot de 32
octets copié, en plus de l'accès éventuel et de l'extension mémoire. Des tests
couvrent 0, 1, 31, 32, 33, 64 et 65 octets, les adresses chaudes/froides et
les données renvoyées par le précompile identité. Les anciens vecteurs de
précompiles gardent leurs sorties mais utilisent les tarifs Berlin pour MODEXP.

Le diagnostic complet passe de 1 218 à 1 584 concordances sans transition d'un
cas concordant vers un échec. Les 150 validations non prises en charge et les
59 erreurs de chaîne demeurent ouvertes. Aucun contournement du contrôle de
chaîne n'a été ajouté au moteur ni au pilote.

Références : https://eips.ethereum.org/EIPS/eip-2565 et
https://eips.ethereum.org/EIPS/eip-211 ; instructions environment.py du fork
Berlin dans ethereum/execution-specs pour EXTCODECOPY.

## Comptabilité SSTORE transactionnelle

Le moteur conserve la valeur originale de chaque slot modifié pendant toute
une exécution, y compris au travers de CALL/DELEGATECALL et de leurs annulations.
Il applique les transitions original/courant/nouveau de EIP-2200, les coûts
chaud/froid de EIP-2929 et le remboursement de suppression de 4 800 gas de
EIP-3529. Les compteurs par appel peuvent être négatifs ; ils sont fusionnés
seulement si l'appel réussit. La protection gasleft <= 2 300 est vérifiée avant
le supplément d'accès froid. Le cache original est réinitialisé à chaque transaction.

La table de transitions est testée sur des slots froids et préchauffés, ainsi
que sur des appels imbriqués réussis et annulés. StorageMeter.sol est déployé
sur Nano et Anvil Paris : écritures successives, remises à zéro, restauration
et appels externes annulés concordent sur gas, statuts, logs et valeur stockée.
Les quatorze transactions incluent les trois cas ERC20 antérieurs.

Le bilan Berlin reste 1 584 concordances, 170 divergences, 150 validations non
supportées et 59 erreurs de chaîne, sans changement de statut des cas. Ce n'est
pas une conformité Berlin : ses remboursements (15 000 et plafond de moitié)
diffèrent de ceux appliqués ici. La sélection de règles par fork reste requise.
Références : https://eips.ethereum.org/EIPS/eip-2200,
https://eips.ethereum.org/EIPS/eip-2929 et https://eips.ethereum.org/EIPS/eip-3529.

## Versions de règles EVM explicites

Berlin et Paris désignent des versions de règles d'exécution, pas des réseaux
ni un fork de l'historique Ethereum. Nano reste une chaîne indépendante.
Le pilote officiel sélectionne explicitement Berlin ou Paris, et signale les
autres versions comme non prises en charge. Le diagnostic Berlin courant est
1 728 concordances, 26 divergences, 150 validations non prises en charge et
59 erreurs de chaîne sur 1 963 cas ; les bilans précédents ci-dessus concernent
le moteur expérimental non versionné. La conformité complète n'est pas acquise.

`config.evmFork: "Paris"` est une option pour un nouveau genesis Nano. Elle est
liée à son hash par un domaine distinct ; aucun genesis livré n'est modifié.
Sans cette option, le comportement expérimental antérieur reste sélectionné.
Berlin est réservé au moteur de tests, pas accepté comme configuration réseau.
Les versions sont propagées aux transactions, aux simulations RPC et aux
lectures du contrat de staking. Le profil Paris active notamment le brûlage
des frais de base, le refus des expéditeurs ayant du code et du préfixe de code
0xEF. PUSH0 reste indisponible dans Berlin et Paris ; BASEFEE seulement dans Paris.
Les remboursements Berlin sont séparés de ceux de Paris. Les règles encore
incomplètes, notamment SELFDESTRUCT et certaines validations, restent ouvertes.
Il n'y a pas encore de calendrier de transitions de versions sur une chaîne existante.

Le banc Hardhat peut sélectionner Paris uniquement pour son genesis temporaire
avec `NANO_TEST_EVM_FORK=Paris`. Cette option ne touche jamais le genesis livré.

## Destruction différée et collisions de création

Dans les versions Berlin et Paris explicites, SELFDESTRUCT garde le code et le
stockage accessibles jusqu'à la fin de la transaction, transfère le solde,
charge le bénéficiaire froid et sa création éventuelle, et annule la suppression
si l'appel est annulé. Berlin accorde son remboursement une seule fois par
compte ; Paris ne l'accorde plus. Le dépôt de code d'un constructeur ne doit
pas ressusciter un compte ainsi supprimé. Les règles expérimentales non
versionnées conservent encore leur ancien comportement.

La création est également refusée si l'adresse possède déjà du stockage,
selon la règle rétroactive EIP-7610 utilisée dans ce corpus épinglé. Ce contrôle
est appliqué aux transactions de déploiement, CREATE/CREATE2 et simulations RPC.

Le diagnostic Berlin actuel est 1 754 concordances, aucune divergence parmi
les cas exécutés, 150 validations non prises en charge et 59 erreurs de chaîne.
Le pilote retourne toujours un échec global : ces 209 cas ne sont ni validés ni
exclus du bilan. Les formats/forks du reste du corpus sont encore à qualifier.
Trois fixtures officielles complètes supplémentaires couvrent les destructions
dans les constructeurs, les sous-appels et les collisions avec stockage.
Référence : https://eips.ethereum.org/EIPS/eip-7610 ; instruction SELFDESTRUCT
Berlin dans ethereum/execution-specs.

### Transactions historiques et environnement des fixtures

Le profil Berlin réservé au diagnostic accepte les signatures historiques v=27/28
sans identifiant de chaîne ; il vérifie toujours la signature et refuse les
transactions protégées signées pour une autre chaîne. Les profils réseau Nano
(expérimental et Paris) conservent leur exigence de signature liée à la chaîne.
Cette différence de politique reste une limite explicite à une affirmation de
conformité Ethereum exhaustive du réseau Nano.

Le pilote fournit pour BLOCKHASH la convention des state tests, Keccak-256 du
numéro de bloc écrit en décimal, comme vmTestBlockHash dans les tests Geth.
Aucun historique synthétique n'est ajouté à l'exécution du réseau Nano.
Le bilan courant est désormais 1 813 concordances, zéro divergence, zéro erreur
et 150 validations non prises en charge. Le groupe contient toujours 1 963 cas
et le code de sortie reste non nul en raison de ces validations incomplètes.
Référence : https://github.com/ethereum/go-ethereum/blob/master/tests/state_test_util.go.


## Rejeu complet après les corrections du journal BFT — 24 septembre 2026

Les exécutables Release reconstruits passent 44 244 cas d'état et 52 870 cas de blocs des six groupes Berlin, Paris, Shanghai, Cancun, Prague et Osaka, avec les quatre transitions déjà prises en charge. Les fichiers d'entrée sont vérifiés octet pour octet contre l'archive épinglée `tests@v20.0.0`. La couverture est contrôlée par identité exacte, sans cas omis ou dupliqué.

Résultats : 2 893 rejets de transaction attendus dans les tests d'état, 60 257 blocs acceptés et 4 940 blocs rejetés conformément aux fixtures ; aucune divergence, erreur ou catégorie non prise en charge dans ce périmètre. Les 275 empreintes de sources et exécutables du build restent inchangées. Les preuves sont indexées dans `build-cmake/qualification/post-signing-full-evm-artifacts-20260924`.

Ce passage ne constitue pas une qualification complète du consensus Ethereum ni du réseau Nano. Les forks modernes restent des profils d'exécution diagnostiques lorsque le réseau ne les active pas. Les comparaisons Anvil et les limites de transport conservent leurs qualifications séparées.


## Comparaisons Anvil/Geth actualisées après les corrections BFT

La campagne `post-signing-anvil-20260924` confirme 48/48 comparaisons de transactions signées et d'estimation imbriquée sur Paris/Shanghai avec Anvil 1.5.1. Les 215 cas modernes contre Anvil 1.8.3 officiel produisent 202 correspondances et 13 écarts ; les 129 cas historiques sont inchangés et les deux scénarios de requêtes supplémentaires ajoutent 86 cas. Ces écarts restent comptés comme des échecs de comparaison.

Geth 1.16.7 et Nano correspondent sur ces 215 transactions rejouées séquentiellement depuis un état synthétique commun, avec propagation indépendante des états, reçus et racines calculées identiques. Le test Shanghai Hardhat conserve 21/27 correspondances de gaz, mais les trois transactions de création dépassant la limite d'initcode sont refusées par les deux clients, avec reprise au même nonce. Ce contrôle ne prouve ni l'égalité de l'état global d'Anvil ni la préparation production du réseau Nano.

### Limites d’initcode : contrôle indépendant des coûts par fork (2026-09-24)

La campagne `initcode-gas-forks-20260924` passe 60/60 créations signées identiques sur Nano et Geth 1.16.7 : Shanghai/Cancun/Prague/Osaka, types 0/1/2, tailles 31/32/33/49151/49152 octets. Les douze séquences comparent les reçus, l’état fourni complet et ses racines ; un vérificateur distinct contrôle les octets signés, la couverture et les coûts calculés. Aux deux grandes tailles, le gaz vaut 252716/252720 avant Prague, puis 512600/512610 avec son plancher EIP-7623. Cela confirme indépendamment le diagnostic des six différences Shanghai d’Anvil 1.8.3, qui restent archivées. Le premier générateur refusé pour hash d’en-tête périmé est conservé. Aucun changement du moteur ; ce contrôle hors ligne ne qualifie ni les RPC ni l’activation réseau des forks modernes.

### Votes et exécution différée OpenZeppelin — 2026-09-24

`oz-governance-primitives-20260924` valide ERC20Votes et TimelockController 5.0.2 sur Paris/Shanghai : délégation signée, checkpoints historiques, refus de rejeu/expiration, contrôle d’accès, exécution différée, annulation et rejeu d’opération. Les deux parcours comportent 18 appels réussis avec estimation explicite `pending` et 16 transactions adversariales refusées avec reçus persistés. Cela ne couvre pas encore le cycle complet de Governor.

Une limite d’intégration reste ouverte : le gaz estimé sur `latest` peut être insuffisant à l’inclusion quand le contrat change de branche selon le numéro ou l’horodatage du bloc. Le transfert ERC20Votes échoue à 67083 gas mais passe avec le devis `pending` de 89173 ; l’annulation du timelock expose aussi cet écart de contexte. Les échecs sont archivés, aucun comportement RPC n’est modifié, et ce test explicite-pending ne vaut pas correction du parcours par défaut des wallets.

### Contexte par défaut de l’estimation corrigé — 2026-09-24

Les références officielles Anvil 1.5.1 et 1.8.3 confirment que `eth_estimateGas` sans bloc explicite cible `pending`. Nano applique désormais ce choix ; `eth_call` sans bloc reste sur `latest`, et tout bloc explicite conserve son sens. Les estimations historiques ne sont pas artificiellement majorées.

Les parcours ERC20Votes/TimelockController passent sur Paris et Shanghai avec le gaz obtenu automatiquement par ethers, sans override : 18 appels réussis et 16 refus adversariaux attendus. Le test natif initialement rouge et les contrôles de pool portent le total à 991 vérifications RPC réussies ; dix suites de file, arrêt, déconnexion et charge bornée passent également. Les échecs initiaux restent archivés dans `rpc-default-pending-20260924`. Cela corrige le défaut de contexte identifié, sans promettre qu’un devis restera valable après toute évolution future de l’état.

### Devis par défaut : transactions ERC20Votes identiques — 2026-09-24

La campagne `votes-signed-default-20260924` compare les mêmes transactions signées de types 0/1/2 : 18 branches Paris/Shanghai contre Anvil 1.5.1 et 9 branches Shanghai contre Anvil 1.8.3. Les 18 branches utilisant les devis par défaut réussissent ; les 9 branches volontairement limitées au devis `latest` échouent de manière identique. Reçus, coûts, soldes et votes suivis correspondent, avec horodatages/frais explicitement alignés et vérification indépendante du calcul EIP-1559 de Nano.

Un nouvel écart reste enregistré pour le déploiement Paris sur Anvil 1.8.3 : 628 gas supplémentaires, exactement le coût Shanghai des 314 mots d’initcode. Geth 1.16.7 confirme les valeurs de Nano et les états/racines fournis sur Paris et Shanghai. Cette divergence de référence ne motive aucune modification des règles Paris de Nano. Les transactions de développement ne qualifient pas la production ni une égalité complète des en-têtes réseau.

### Cycle Governor OpenZeppelin qualifié localement — 2026-09-24

`oz-governor-lifecycle-20260924` passe sur Paris/Shanghai avec GovernorVotes, GovernorCountingSimple et GovernorTimelockControl 5.0.2. Deux propositions par profil vérifient le cycle réussi jusqu’à l’exécution et un échec faute de quorum. Transférer les jetons après le snapshot n’augmente pas le poids historique du destinataire. Douze transactions malveillantes échouent avec reçus persistés ; les appels réussis utilisent le gaz estimé automatiquement. Les rôles du timelock appartiennent au Governor, l’administrateur de test renonce à ses droits, et la cible finale vaut 73. Pas de modification du moteur ; cette composition locale ne représente pas toute la gamme Governor ni un déploiement de gouvernance en production.

### Rafraîchissement après vue EVM constante — 24 septembre 2026

Le runner Release recompilé après le changement des appels statiques et de
l'ouverture RocksDB passe les 5 387 cas présents dans les 317 fichiers de
`tests/fixtures/ethereum-official`, dont 505 validations de rejet. Aucun mismatch,
unsupported ou adapter error sur cette sélection. Le vérificateur
`verify-readonly-corpus.mjs` contrôle les identifiants exacts, les résultats et
les empreintes des fixtures ; supprimer un cas fait échouer la vérification.
`readonlyEvmCorpusQualification` référence les artefacts de cette campagne.
Ce résultat concerne le corpus versionné dans le dépôt, pas les 44 244 cas
historiquement exécutés dans d'autres sélections, ni les tests de blocs ou
l'activation d'un fork réseau. Ces vérifications plus larges restent à rafraîchir.

Le même rafraîchissement couvre les 2 412 cas des 170 fichiers de blocs
versionnés : tous correspondent, sans unsupported ni erreur. Identités et
empreintes vérifiées par `verify-readonly-blocks.mjs`, contrôle négatif par
suppression d'un cas. Référence : `readonlyEvmBlockCorpusQualification`.
Il s'agit des engagements d'exécution couverts par le runner, et non d'une
validation complète des en-têtes, de la finalité ou du consensus réseau.
