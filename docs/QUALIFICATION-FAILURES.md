# Registre des échecs et corrections de qualification

Ce registre doit être revu avant toute conclusion de préparation production. Il recense les campagnes en échec actuellement identifiées dans le manifeste ; **la revue exhaustive des incidents, y compris BFT, stockage, RPC, Solidity et CI, reste à effectuer**. Une campagne verte ultérieure ne clôt pas automatiquement toutes les causes de ses échecs antérieurs.

Pour chaque incident, la revue doit relier : cas original et journal, cause, correction exacte, test de régression, même cas rejoué et résultat, puis périmètre restant. Vérifier aussi les empreintes, la version du code testée, les exclusions et les faux positifs du pilote.

Les états à utiliser sont : ouvert ; correction à vérifier ; résolu sur un périmètre précisé ; régression rouverte. Une interruption ou un timeout reste un résultat incomplet. Les échecs historiques ne sont jamais remplacés par les réussites.

## État du diagnostic Anvil EIP-7702

L'écart initial de 12 500 gas est expliqué sur le périmètre Anvil 1.5.1 local :
sa base mémoire distingue les comptes absents autrement que le modèle Nano.
Les mêmes 75 transactions rejouées séquentiellement depuis un état commun
concordent avec Geth v1.16.7, y compris leurs racines d'état. La correction du
pré-état RPC sur preuve d'absence est vérifiée séparément. Les anciennes
campagnes restent en échec dans leur configuration originale ; les paragraphes
ci-dessous décrivent les étapes historiques de l'enquête. Ce diagnostic ne
clôt ni l'admission RPC Shanghai ni la qualification globale des profils modernes.

## Phase courante : qualification EVM sans flood

À la demande de l'utilisateur, aucun flood réseau ni campagne de chaos pendant
cette phase. La campagne CTest globale en cours a été arrêtée (code 143) et
l'absence de processus de test résiduel a été vérifiée. Son rapport partiel reste
conservé ; il ne vaut pas réussite des 53 suites.

Utiliser `ctest --test-dir build-cmake -L '^qualification_evm$' --output-on-failure`
pour les vérifications fonctionnelles de cette phase. Une charge contrôlée peut
être lancée séparément avec `-L '^controlled_load$'`, si nécessaire. Les scénarios
BFT/flood restent hors de cette campagne temporaire, et leur qualification globale
reste ouverte. Ne pas relancer CTest sans sélection pendant cette phase.

## Revue des preuves historiques

L'outil `node tests/qualification/audit-evidence.mjs` vérifie les empreintes,
les identités de cas et variantes, les résumés, les racines, le gas et les motifs
de rejet. Ses sept tests vérifient notamment les faux résultats verts, la corruption,
les doublons et les journaux tronqués. Quatre rapports Cancun retrouvés dans `/tmp`
ont été copiés dans les journaux locaux et épinglés après contrôle des compteurs.
Les 19 campagnes de fixtures référencées retrouvent tous leurs anciens cas en
échec/non pris en charge dans les rapports corrigés. Cette preuve historique ne
remplace ni une revue de cause, ni un nouveau passage du binaire courant.

Deux incidents ont été revus et référencés dans `qualificationIncidentReview` :
- `block_auth` attendait « invalid PoS gas limits » après le passage au diagnostic partagé « invalid block gas limits ». Les assertions de limites strictes, de minimum genesis et de non-publication du bloc rejeté sont conservées. La suite Release rejouée passe.
- L'ancien build ne connaissait pas encore la cible `nano-request-tests`. La cible existe dans la configuration Release, sa construction explicite et la suite `execution_requests` passent.

Le passage ciblé donne 2/2 suites en 2,26 secondes. Journaux initiaux et corrigés,
ainsi que les empreintes des deux binaires, sont conservés. Cela ne transforme pas
l'ancienne campagne globale de 49 suites en réussite et ne prouve pas la reproductibilité.

L'outil de revue avait lui-même un défaut : son extraction du fork capturait
`fork_transition` dans le nom du fichier ou de la fonction avant le vrai paramètre.
Le rapprochement initial manquait donc 34 cas de transition. L'extraction est
maintenant limitée au paramètre `[fork_...` ou `-fork_...` ; un test protège aussi
contre le rapprochement avec un fork différent. Les rapports bruts sont inchangés,
et le nouveau contrôle retrouve les 260 cas. L'ancien audit reste conservé dans
`transitions-evidence-audit.json`.

**Transitions : 260/260 cas concordants en Release**, après 260 unsupported initiaux.
Le rapport `transitions-block-initial.jsonl` est conservé ; le résultat corrigé
figure sous `transitionBlockCorrectedResults`, avec empreinte du binaire.
La revue des causes et des limites continue ; deux exécutions intermédiaires
arrêtées restent explicitement incomplètes.

## Inventaire initial des campagnes

Les identifiants ci-dessous désignent les entrées de [ethereum-official.json](../tests/fixtures/ethereum-official.json). Les résultats ultérieurs sont des pistes de preuve, pas une clôture de revue.

| Campagne en échec | Écarts / erreurs / non pris en charge | Résultat ultérieur à auditer | État de revue |
| --- | --- | --- | --- |
| `transitionBlockInitialResults` | 0 / 0 / 260 | `transitionBlockCorrectedResults` | Passage complet concordant ; revue des causes à terminer |
| `cancunBaselineResults` | 0 / 0 / 10777 | `cancunStateResults` | À reprendre cas par cas |
| `cancunPartialResults` | 293 / 913 / 0 | `cancunStateResults` | À reprendre cas par cas |
| `cancunBlobResults` | 256 / 0 / 0 | `cancunStateResults` | À reprendre cas par cas |
| `cancunKzgResults` | 52 / 0 / 0 | `cancunStateResults` | À reprendre cas par cas |
| `pragueInitialResults` | 1235 / 770 / 0 | `pragueStateResults` | À reprendre cas par cas |
| `pragueBlsResults` | 47 / 768 / 0 | `pragueStateResults` | À reprendre cas par cas |
| `pragueAuthorizationInitialResults` | 1 / 0 / 0 | `pragueStateResults` | À reprendre cas par cas |
| `osakaShanghaiAnvilComparison` | — / — / — | Aucun passage Shanghai entièrement concordant | Ouvert : différence d’admission RPC |
| `transactionInitialResults` | 16 / 0 / 0 | `transactionResults` | À reprendre cas par cas |
| `blockExecutionInitialResults` | 0 / 0 / 779 | `blockWithdrawalResults` | À reprendre cas par cas |
| `blockRejectionResults` | 0 / 0 / 50 | `blockWithdrawalResults` | À reprendre cas par cas |
| `blockParentHeaderResults` | 0 / 0 / 39 | `blockWithdrawalResults` | À reprendre cas par cas |
| `cancunBlockInitialResults` | 57 / 19 / 177 | `cancunBlockCorrectedResults` | À reprendre cas par cas |
| `blobHeaderInitialResults` | 27 / 0 / 4 | `blobHeaderCorrectedResults` | À reprendre cas par cas |
| `blobHeaderAdapterErrors` | 0 / 4 / 0 | `blobHeaderCorrectedResults` | À reprendre cas par cas |
| `pragueBlockInitialResults` | 106 / 14514 / 71 | `pragueBlockCorrectedResults` | Passage complet concordant ; revue des causes à terminer |
| `osakaBlockInitialResults` | 17 / 174 / 1 | `osakaBlockCorrectedResults` | Passage complet concordant ; revue des causes à terminer |

## Incidents de qualification récents

- **Prague interrompu** : `pragueBlockInterruptedResults` conserve 11 945 cas terminés et une dernière ligne tronquée. Aucune réussite globale déduite ; nouveau passage `prague-block-system-resumed.jsonl` terminé : 15 799 concordances, zéro divergence/erreur/exclusion.
- **Test natif blob** : parent de test sans withdrawalsRoot, corrigé ; journaux `blob-header-native-initial.log` et `blob-header-native-corrected.log`. Les quatre suites ciblées passent ensuite. Revoir que la correction ne masque aucun rejet valide.
- **Pilote des blocs Cancun** : quatre fixtures sans rlp_decoded causaient des erreurs JSON. Décodage de leur RLP brut ajouté ; 234 cas ciblés puis 12 114 cas complets concordants. Vérifier les catégories exactes de rejet.
- **Timeout de la suite de blocs embarquée** : 30 secondes dépassées après extension du corpus. Six autres suites passent. Délai porté à 180 secondes ; le nouveau passage termine en 28,92 secondes. Les deux journaux sont conservés. Cela ne constitue pas une preuve de performance sous charge.
- **Admission RPC Anvil Shanghai** : Nano refuse les initcodes excessifs tandis qu’Anvil retourne un hash puis les exclut du bloc. La comparaison conserve son code de sortie 1 ; ne pas affaiblir la règle Nano pour effacer cet écart. Revoir la preuve sur la version Anvil épinglée.

## Prochaine revue exhaustive

1. Inventorier aussi les journaux absents du manifeste et les essais interrompus ; signaler les preuves manquantes.
2. Grouper les échecs par cause sans supprimer les cas ni leurs identifiants.
3. Rejouer les cas corrigés sur le binaire courant, puis contrôler les régressions et le périmètre réellement couvert.
4. Reprendre explicitement les écarts restants : transitions de fork, admission RPC, blobs/PeerDAS, BFT et reprise/stockage/charge, parcours Solidity/OpenZeppelin/pool.
5. Ne clore la préparation production qu’après cette revue et les autres exigences du goal ; aucun déploiement ni fonds réels autorisés par ce registre.

## Shanghai : limites d'initcode par type de transaction

Le comparateur signe maintenant les créations de 49 151 et 49 152 octets pour
chacun des types 0, 1 et 2, et exige leur réussite. Il vérifie aussi le type et
le hash de transaction de chaque reçu. Le passage Release enregistré dans
`shanghaiTypedBoundaryComparison` donne 27 exécutions concordantes, dont ces
six frontières valides (252 716 et 252 720 unités de gas respectivement).
Les trois créations invalides de 49 153 octets restent refusées par Nano,
admises puis exclues du bloc par Anvil 1.5.1. Les états canoniques restent
inchangés et les remplacements valides au même nonce réussissent des deux côtés.
Le code de sortie 1 et le désaccord d'admission sont conservés ; aucune règle
Nano n'a été assouplie. La limite provient de [EIP-3860](https://eips.ethereum.org/EIPS/eip-3860).

Le premier essai, bloqué par l'interdiction sandbox d'écouter sur loopback,
est conservé séparément : il ne constitue aucun résultat EVM. Le passage
suivant utilisait uniquement deux nœuds locaux, sans flood.

Le contrôle de régression Paris avec le même comparateur termine à 14/14
transactions concordantes, code 0 (`parisReceiptTypeComparison`).

## Contexte EVM des lectures du staking : PREVRANDAO

La revue du chemin de lecture du registre a trouvé un contexte différent du
moteur transactionnel et du RPC : `staking_call` fournissait toujours la
`difficulty` à l'opcode 0x44, y compris après Paris. Un contrat de registre
qui consulte cet opcode pouvait donc produire une réponse différente selon
le chemin d'appel. Le contexte utilise désormais `block_randomness(header,revision)`,
comme les autres chemins : difficulty avant Paris, mixHash à partir de Paris.

Le test ajouté à `test_prevrandao.cpp` utilise un contrat sonde qui retourne
0x44, avec difficulty=7 et mixHash=123, sur les sept profils NanoExperimental,
Berlin, Paris, Shanghai, Cancun, Prague et Osaka, et vérifie l'absence de mutation.
Il échoue avant correction (`staking-randomness-before.log`, code 8).
Après correction, `staking`, `staking_paris` et `prevrandao` passent (3/3).
Le journal initial reste conservé. Ce correctif ne crée aucune source aléatoire
pour Nano et n'active aucun fork réseau. Il modifie le résultat des registres
personnalisés utilisant 0x44 après Paris ; aucune migration d'historique n'est
réalisée. Le contrat NanoStaking fourni ne consulte pas cet opcode.

La campagne fonctionnelle `qualification_evm` passe ensuite à 19/19 en Release.
Les preuves sont référencées dans `stakingRandomnessCorrection`. Aucun test de flood exécuté.

## Rejeu intégral des fixtures d'état avec le binaire Release courant

Après le correctif du contexte staking, les six corpus d'état de l'archive
épinglée tests@v20.0.0 ont été rejoués : Berlin 1 963, Paris 2 250, Shanghai
2 441, Cancun 10 777, Prague 13 074 et Osaka 13 739. Total : **44 244 cas
concordants**, zéro divergence, erreur ou unsupported ; six codes de sortie 0.
L'analyse des rapports ne relève ni doublon d'identité ni résumé incohérent.

Le contrôle de provenance a comparé les **7 603 fichiers** utilisés, octet pour
octet via SHA-256, aux membres correspondants de l'archive dont l'empreinte est
épinglée. Aucun fichier manquant, supplémentaire ou modifié dans ces six groupes.
Le manifeste conserve chaque rapport, son empreinte, le binaire et l'inventaire
sous `currentReleaseStateReplay`. Les anciens rapports restent inchangés.

Ce passage concerne les fixtures d'état de ces six profils, pas tous les groupes
de l'archive, ni un nouveau rejeu des blocs, ni la disponibilité des blobs ou la
qualification BFT. `forkConformanceProven` reste faux. L'admission Anvil Shanghai
et la préparation production globale restent ouvertes.

## Rejeu Release des blocs et transitions après correction du staking

Les sept passages complets terminent avec code 0 : Berlin 2 439, Paris 2 734,
Shanghai 2 975, Cancun 12 114, Prague 15 799, Osaka 16 549 et transitions 260.
Total : **52 870 cas concordants**, zéro divergence, erreur ou unsupported.
Les résumés, identités uniques et engagements/rejets des rapports ont été
contrôlés ; les 7 972 fichiers sélectionnés sont tous présents dans le rejeu
et leurs octets correspondent à l'archive épinglée. Le manifeste
`currentReleaseBlockReplay` conserve les rapports et empreintes, ainsi que
les nombres de blocs exécutés et rejetés par campagne.

Ces preuves portent sur les engagements d'exécution et les catégories de rejet
prises en charge par le pilote, pas sur la validation intégrale du consensus
Ethereum. Elles n'activent aucun profil réseau supplémentaire. Les rapports
antérieurs restent conservés ; le désaccord d'admission Anvil reste ouvert.

## Contrôle négatif du pilote d'état : gas du reçu

Le nouveau test `node --test tests/qualification/runner-controls.test.mjs`
modifie uniquement des attentes dans des copies temporaires de fixtures : racine,
logs, reçu RLP, gas et motif de rejet. L'original reste accepté. Le premier passage
reproduit un faux vert du pilote pour un cumulativeGasUsed corrompu alors que le
RLP du reçu reste correct : 5 contrôles passent, 1 échoue. Le pilote compare
désormais aussi ce champ numériquement, même en l'absence du RLP attendu.

Après correction, 7/7 contrôles et la suite embarquée de fixtures d'état passent.
Les journaux sont épinglés sous `stateRunnerGasControl`. Le moteur C++ n'a pas
changé ; le binaire du pilote a changé. Le précédent rejeu complet garde son
ancienne empreinte, et n'est pas présenté comme exécuté avec ce nouveau pilote.
La vérification a posteriori des 41351 lignes de gas de ce rejeu ne trouve
aucune contradiction : l'auditeur contrôlait déjà ces champs indépendamment.

## Contrôles négatifs du pilote de blocs

La suite `runner-controls.test.mjs` compte maintenant 12 contrôles réussis.
Le pilote accepte le bloc témoin intact, refuse un postState ou lastblockhash
corrompu, refuse un motif de rejet différent et garde un fork inconnu comme
unsupported avec code 1. Les fixtures officielles ne sont pas modifiées ; les
mutations utilisent des copies temporaires supprimées après chaque essai.
Preuves : `blockRunnerNegativeControls`. Aucun changement du moteur.

## Hash de transaction dans les reçus des fixtures d’état

Le pilote exécutait déjà txbytes directement, mais ne comparait pas le champ
transactionHash du reçu attendu, absent de son encodage RLP. Une mutation de ce
champ produisait un faux vert. Le pilote vérifie maintenant que le hash du reçu
correspond au keccak des octets signés et au champ attendu lorsqu’il existe.
Le contrôle négatif reproduit le défaut avant correction ; les 21 tests Node
et la suite embarquée d’état passent ensuite. L’auditeur refuse aussi un statut
matched accompagné de gasUsedMatches=false ou transactionHashMatches=false.
Preuves : stateRunnerTransactionHashControl. Le moteur reste inchangé.

Le rejeu complet avec ce pilote renforcé termine à **44244 cas concordants**,
zéro erreur, divergence ou unsupported sur les six profils. Les identités et
variantes correspondent exactement au précédent rejeu. 41351 transactions
exécutées ont leur hash contrôlé ; les autres cas sont des rejets attendus.
Les rapports séparés et l'empreinte du nouveau pilote sont conservés dans
`strictReleaseStateReplay`.

## BLOBBASEFEE dans les lectures staking et simulations RPC

Les deux chemins laissaient blob_base_fee à zéro dans leur contexte EVM.
Ils utilisent désormais le calcul partagé à partir de excessBlobGas et de la
révision, à partir de Cancun. Le test de lecture staking échoue avant correction.
La sonde d'opcode 0x4a vérifie après correction les deux chemins, avec excès
5 007 716 : valeur 4 pour Cancun et 2 pour Prague/Osaka, sans mutation d'état.
Les profils RPC sont construits en mémoire par le test : la configuration réseau
continue de refuser ces profils, aucune activation publique n'est introduite.

La campagne fonctionnelle termine à 19/19 (28,26 s). Journaux et empreinte de
la sonde sont épinglés sous `blobContextCorrection`. Le calcul de frais utilisé
par l'exécution transactionnelle et les appels système n'a pas changé. Les
anciens rejeux complets conservent les empreintes de leurs propres binaires.

## Simulation RPC des transferts de valeur

27 assertions supplémentaires couvrent NanoExperimental, Paris et Shanghai.
Un contrat sonde retourne CALLVALUE et SELFBALANCE : un appel de valeur 5 vers
un contrat possédant 10 observe bien 5 et 15. Le même appel fonctionne au gas
estimé ; une transaction signée exécutée sur une copie confirme le solde final
et le coût. Les appels sans fonds et les appels qui écrivent puis revert sont
refusés par eth_call et eth_estimateGas. Les soldes, nonces et le stockage de
la chaîne restent inchangés après toutes les simulations. La suite RPC passe
(1,52 s), sans correction du moteur. Preuves : rpcValueSimulationQualification.

## Préfixe 0xEF des créations simulées après Shanghai

Le moteur signé rejetait correctement les créations retournant un runtime
commençant par 0xEF après Berlin, mais le RPC ne vérifiait que Paris/Shanghai.
Le test reproduit la divergence sur un profil successeur construit en mémoire.
Le RPC utilise maintenant revision >= Paris. Les profils réseau autorisés
ne changent pas. 21 combinaisons (sept profils, préfixes 00/EE/EF), soit 100
assertions supplémentaires, comparent eth_call, eth_estimateGas et transaction
signée, avec contrôle de non-mutation. Les 19 suites EVM passent ensuite
(27,65 s). Preuves avant/après : rpcCreationPrefixCorrection.

## Plancher calldata Prague dans les simulations RPC

Le RPC omettait le plancher EIP-7623 et pouvait retourner une estimation que
l’exécution signée refuserait. Le test reproduit cette divergence. simulate
vérifie maintenant le plancher avant exécution ; la recherche du gas minimal
part du maximum entre gas intrinsèque et plancher. Le budget opcode reste gas
moins gas intrinsèque, confirmé par une sonde GAS (le plancher n’est pas prépayé).

60 assertions couvrent Cancun témoin, Prague et Osaka : données nulles, non
nulles et mixtes, estimation comparée à une transaction signée, limite exacte,
limite moins un, budget opcode et absence de mutation. Les 19 suites EVM
passent, puis la suite RPC enrichie de la sonde GAS passe à nouveau. Preuves
sous rpcCalldataFloorCorrection. Aucun nouveau profil réseau activé.

## Création RPC et SELFDESTRUCT délégué depuis Cancun

Le RPC omettait contract_creation dans ExecutionContext. Un constructeur
appelant via DELEGATECALL un helper ADDRESS/SELFDESTRUCT puis SELFBALANCE
observait encore sa valeur transférée après Cancun, alors que le compte créé
dans la transaction doit suivre les règles de suppression. La transaction
signée appliquait déjà le bon contexte. Le test reproduit la divergence avant
correction ; le RPC marque maintenant explicitement la création.

Huit cas (création/compte existant sur Shanghai, Cancun, Prague et Osaka),
32 assertions, comparent résultat RPC, solde après exécution signée, suppression
ou conservation du code et état canonique inchangé. Les 19 suites EVM passent
(27,87 s). Preuves : rpcCreatedAccountCorrection. Les profils modernes restent
diagnostiques ; aucun changement de leur activation réseau.

## Simulation RPC vers les comptes délégués EIP-7702

Le RPC exécutait l’indicateur EF0100 au lieu du code délégué. La divergence est
reproduite avant correction. La simulation suit maintenant un seul pointeur,
préchauffe la cible et traite la cible précompilée comme du code vide, comme
l’exécution signée. Adresse, valeur et stockage restent ceux du compte autorité.

15 cas sur Cancun témoin, Prague et Osaka couvrent cible normale (avec lecture
de balance préchauffée et écriture de stockage), cible absente, précompile,
boucle de délégation et revert. Ils comparent statut, retour, gas estimé et
exécution signée et vérifient l’absence de mutation canonique. Les 19 suites
EVM passent (30,62 s). Preuves : rpcDelegationCorrection. Les transactions
type 4 restent refusées par le RPC public avant activation ; aucun profil
réseau supplémentaire n’est activé.

## Comparaison indépendante Anvil des profils modernes : écart EIP-7702 ouvert

Un nouvel outil hors ligne `nano-signed-execution-runner` exécute une enveloppe
signée avec un état et un header explicites ; il n'ouvre aucun serveur et n'est
pas installé comme nœud. Le script `tests/hardhat/scripts/compare-modern-anvil.mjs`
lance Anvil sur loopback, envoie la même enveloppe et compare reçus et comptes/
slots explicitement suivis. Il n'établit pas une égalité de racine globale.

Le premier passage conserve 14 cas concordants (7 Cancun, 7 Prague : créations,
stockage transitoire/MCOPY, BLOBBASEFEE, revert et calldata type 2), puis échoue
sur l'autorisation type 4 Prague : Nano 68 341 gas contre Anvil 55 841,
différence 12 500. L'autorité est vide dans l'état RPC reconstruit ; l'existence
réelle du compte et le remboursement d'autorisation doivent être investigués.
Aucune attribution au moteur ou à Anvil n'est encore démontrée. Osaka n'a pas
été atteint par ce passage. Entrées exactes, états, reçus et hashes sont conservés
sous `modernAnvilInitialComparison`. Ne pas remplacer cet échec par un passage
qui évite le cas. L'ancien désaccord d'admission Shanghai reste distinct.

## EIP-7702 : matrice d'autorités et preuve d'inclusion

La campagne `modernAnvilAccountMatrix` termine 75 comparaisons sur des instances
Anvil isolées : 71 concordances et 4 divergences conservées, exit 1. Trois états
sont comparés sur Cancun, Prague et Osaka : autorité touchée sans valeur,
autorité sans transaction préalable, autorité créditée de 1 wei par transaction
signée. Les quatre divergences concernent les autorisations Prague/Osaka des
deux scénarios vides : Nano 68 341 gas, Anvil 55 841. Les autorités financées
concordent à 55 841 gas. Osaka est désormais atteint, dont création/appel CLZ.

La preuve de l'ancien passage `modern-anvil-proof` établit l'inclusion d'un
compte vide à l'adresse autorité dans la racine du bloc parent. Le vérificateur
contrôle les hashes, le chemin keccak de l'adresse, la feuille et le lien parent.
Deux contrôles négatifs (adresse et racine altérées) sont rejetés. Il vérifie
l'inclusion uniquement, pas les preuves d'absence.

La [spécification EIP-7702](https://eips.ethereum.org/EIPS/eip-7702), étape 7,
conditionne le remboursement au compte non vide. Nano utilise `!current.empty()`.
La matrice n'établit pas encore la cause du comportement Anvil : les lectures RPC
de préparation et la version épinglée de son moteur restent à isoler. Ne pas
modifier la règle Nano pour obtenir artificiellement l'égalité, ni classer ces
échecs comme résolus. La comparaison s'arrête au premier champ divergent pour
chaque transaction, conserve les états complets suivis et poursuit les autres
cas ; toute divergence laisse le processus en échec. Aucun profil réseau activé.

## EIP-7702 : isolation des lectures RPC et identifiants

`modernAnvilRpcIsolation` termine 50 cas : 46 concordants et 4 divergents,
exit 1. Supprimer `eth_getProof`, puis toutes les lectures préalables de
l'autorité, ne supprime pas l'écart Prague/Osaka de 12 500 gas. Les lectures
RPC de préparation ne sont donc pas nécessaires pour reproduire le défaut.

Les hashes des 75 transactions de la matrice précédente ont été recalculés
comme keccak256 des octets signés : ils correspondent tous aux reçus Nano et
Anvil, y compris les cas divergents. Le comparateur vérifie désormais aussi
immédiatement l'identifiant retourné par `eth_sendRawTransaction`.

Le code revm à la révision exacte utilisée par Anvil, conservé avec son SHA256,
emploie `!(is_empty() && is_loaded_as_not_existing_not_touched())` pour le
remboursement (lignes 236–239). Cette condition dépend de l'existence et de
l'état interne touched ; Nano teste le compte non vide. La divergence de
conditions est établie, mais le chemin précis du chargement Anvil reste à
tracer avant d'attribuer définitivement le défaut. Les échecs restent ouverts.

## EIP-7702 : origine identifiée dans la référence Anvil épinglée

La chaîne de sources à révisions exactes explique le remboursement observé :

- Foundry `EmptyDBWrapper::basic_ref` renvoie `Some(AccountInfo::default())`
  même pour une adresse absente (`in_memory_db.rs`, lignes 81–106).
- Le journal revm transforme `Some` en compte ordinaire ; seul `None` crée un
  compte marqué `LoadedAsNotExisting` (`journal/inner.rs`, lignes 707–710).
- La conversion `From<AccountInfo>` initialise le statut à `AccountStatus::empty()`, sans le
  marqueur d'absence (`state/src/lib.rs`, lignes 276–283).
- La condition de remboursement revm épinglée accorde donc 12 500 gas à ce
  compte vide. Elle diffère du critère non vide de l'EIP-7702 actuelle.

Ceci explique les résultats des scénarios avec et sans lectures préalables,
ainsi que la concordance des autorités financées. Périmètre : Anvil 1.5.1,
base mémoire locale sans fork. Aucun changement du moteur Nano n'est justifié
par cette différence de référence. Les campagnes restent en échec et visibles ;
« expliqué » ne signifie ni égalité Anvil ni qualification EVM complète.

Six régressions Nano ajoutées (Prague/Osaka × absent/entrée vide/financé)
vérifient gas 68 341/68 341/55 841, frais du payeur, nonce et solde de l'autorité,
code délégué et stockage. La suite `authorization` passe (0,34 s).
Sources exactes et preuves : `modernAnvilEmptyAuthorityDiagnosis`.
La contre-vérification avec un autre moteur Ethereum reste à effectuer.

## Contre-vérification Geth v1.16.7 des mêmes transactions signées

L'outil officiel `evm t8n` a été compilé depuis le module Go épinglé v1.16.7,
avec empreintes du module et du binaire conservées. La première compilation
hors ligne échoue par dépendances absentes ; la compilation avec récupération
des dépendances réussit. Aucun nœud ni réseau Ethereum n'est démarré.

Le comparateur transmet les enveloppes signées exactes sous liste RLP, sans
resigner, et reprend l'environnement sauvegardé. Première campagne : 75 échecs
de comparaison, notamment type legacy omis et logs null dans le JSON Geth.
L'adaptateur normalise ces représentations, puis couvre aussi les adresses
créées dans le post-état Nano. Les campagnes corrigées donnent :

- matrice initiale : 71 concordances, 4 divergences ;
- isolation RPC : 48 concordances, 2 divergences.

Pour une autorité réellement absente de l'alloc (scénario `unqueried`), Geth
confirme Nano : 68 341 gas, sur Prague et Osaka, avec reçu et état suivis
concordants. Une autorité financée donne 55 841 gas dans les trois moteurs.
Les six divergences restantes avec Geth portent sur des entrées vides explicites
injectées par la reconstruction RPC : Geth v1.16.7 teste `StateDB.Exist`, Nano
le compte non vide. Les champs RPC balance/nonce/code ne prouvent pas l'existence
d'une feuille ; il faut traiter cette distinction dans l'adaptateur et ses
preuves, sans supprimer silencieusement les campagnes précédentes.

Cela confirme le diagnostic Anvil pour le cas absent, mais ne ferme pas la
question des pré-états explicitement vides. Pas de comparaison des racines
globales ni de validation consensus ici. Toutes les campagnes, entrées et
sorties sont référencées sous `gethSignedTransactionCrosscheck`.

## Pré-état Geth corrigé uniquement sur preuve d'absence

Un vérificateur de preuves de comptes contrôle racine, hashes des nœuds,
chemin keccak, branches et feuilles. Il distingue feuille présente vide,
feuille différente prouvant l'absence et branche vide ; une preuve tronquée,
une racine altérée ou des nœuds supplémentaires sont rejetés. Neuf tests passent.
La racine utilisée est celle du header parent sauvegardé et son hash doit
correspondre au parent de la transaction comparée.

Avec `NANO_USE_ACCOUNT_PROOFS=1`, six preuves d'autorité sont vérifiées sur
la matrice initiale : deux absences et quatre présences. Seules les deux entrées
vides dont l'absence est prouvée sont retirées de l'alloc Geth. Les deux comptes
présents vides et les deux comptes financés sont conservés. Les 75 transactions
signées originales donnent alors 73 concordances et 2 divergences, exit 1.
Les campagnes précédentes ne sont ni écrasées ni réétiquetées.

Les deux divergences restantes concernent le compte vide conservé dans le trie
Anvil après le transfert de valeur zéro. L'EIP-161 impose qu'un compte touché
et vide soit absent à la fin de la transaction :
https://eips.ethereum.org/EIPS/eip-161 (règles c/d).
Nano exclut les comptes vides de sa racine (`State::root`) ; le header Anvil
prouve une représentation différente dans ce cas. Un pré-état commun reconstruit
par exécution séquentielle reste à qualifier avant de fermer le diagnostic.
L'EIP-7523 consultée est encore affichée « Last Call » ; elle n'est pas employée
ici comme justification pour ignorer les cas.

Preuves et résultats : `gethProofAwarePrestate`. Aucune modification du moteur
Nano ni de l'activation des profils réseau.

## Rejeu séquentiel Nano–Geth depuis un état initial commun

`gethSequentialCommonState` termine 75 transactions, 9 séquences, 0 divergence,
exit 0. Les transactions signées originales de la matrice Anvil sont reprises
sur Cancun, Prague et Osaka. Chaque séquence commence avec un état synthétique
commun explicitement enregistré (comptes non vides du premier pré-état).
Cet état n'est pas présenté comme la racine globale du nœud Anvil.

Ensuite, chaque moteur consomme exclusivement son propre post-état précédent :
aucune resynchronisation depuis Anvil ou depuis l'autre moteur. La comparaison
vérifie hash des octets signés, statut, type, gas, gas cumulé, bloom, logs,
tous les comptes et slots du post-état fourni, et sa racine calculée. Le runner
Nano exporte désormais cette racine, sans modification des règles d'exécution.

Les deux anciens cas `touched-empty` Prague/Osaka concordent à 68 341 gas.
Le diagnostic d'écart de référence est ainsi confirmé par un second moteur
en séquence ; les campagnes Anvil et Geth avec pré-état artificiellement vide
restent conservées avec leurs codes d'échec. Ce résultat ne doit pas remplacer
leur historique par un « 100 % » global : le périmètre est ces 75 transactions,
sans validation des headers, du consensus, des sidecars ou du réseau public.

Sources, binaires, état initial, entrées/sorties de chaque étape et racines sont
indexés avec SHA256 dans `modern-geth-sequential/artifact-index.json`.

## Cycle EIP-7702 signé : révocation, invalidité et revert

La matrice est étendue de 54 nouveaux cas, sur Prague/Osaka et trois états
initiaux d'autorité. Elle couvre appel sans nouvelle autorisation, révocation
préservant le stockage, mauvais chain-id, doublon appliqué une seule fois,
autorisation persistante après revert, nouvel appel utilisant cette délégation,
révocation après revert, nonce périmé et autorisation chain-id zéro.

Le contrat de revert écrit maintenant 43 alors que le stockage autorité vaut
42 avant l'appel : le test vérifie réellement le retour à 42, ainsi que nonce,
code de délégation et statut, sur Nano et Anvil. Chaque preuve pré-transaction
est sauvegardée sous son nonce pour éviter l'écrasement entre autorisations.

Résultats terminaux :

- Anvil : 125 concordances / 129, les quatre écarts initiaux connus sur les
  comptes vides, exit 1 ; les 54 nouveaux cas concordent.
- Geth en séquence depuis un état initial commun : 129 concordances / 129,
  9 séquences, reçus, état fourni complet et racines identiques, exit 0.

Preuves : `authorizationLifecycleSignedComparison`, avec index SHA256 des
entrées/sorties. Les rapports précédents et leurs échecs restent inchangés.
Ce périmètre ne valide pas encore tous les contrats, le réseau, les sidecars,
le consensus ni la préparation production ; aucun profil public activé.

## Admission d'initcode : contre-vérification indépendante Geth t9n

Nouvelle matrice de 90 enveloppes signées identiques fournies à Nano et Geth
v1.16.7 : Paris, Shanghai, Cancun, Prague, Osaka × types 0/1/2 × tailles
31, 32, 33, 49 151, 49 152 et 49 153 octets. Les 90 cas concordent, exit 0.
Les 78 créations valides réussissent dans Nano, avec hash des octets signés
et gas intrinsèque concordants avec Geth. Les 12 cas dépassant 49 152 octets
sur Shanghai et après sont rejetés avec le motif attendu dans les deux outils.
Paris constitue le témoin où 49 153 octets restent admissibles.

Le runner hors ligne accepte maintenant Paris/Shanghai explicites et exporte
le gas intrinsèque ; aucune règle du moteur ni du RPC n'est modifiée.
La nouvelle matrice est indépendante de la session RPC historique : elle
confirme le rejet EIP-3860 de Nano, mais n'est pas présentée comme un nouveau
test de mempool ou de préservation d'état au rejet. Les vérifications RPC
historiques (nonce/solde inchangés et remplacement valide au même nonce)
restent leurs preuves propres. Le renvoi initial d'un hash par Anvil demeure
une différence d'admission documentée, pas une raison d'assouplir Nano.

Entrées RLP signées, contexte, sorties et index SHA256 : `initcodeGethValidation`.

## Blobs signés et correction de deux faiblesses du pilote

Le générateur de référence Geth v1.16.7 produit un blob non nul, son engagement,
sa preuve KZG et son hash versionné, puis vérifie lui-même la preuve. Anvil
reçoit l'enveloppe réseau avec sidecar ; Nano hors ligne et Geth t8n reçoivent
le même corps signé canonique. Le hash retourné doit correspondre à ce corps,
pas au paquet contenant le sidecar.

La campagne corrigée Cancun/Prague comprend 14 transactions dont 6 type 3 :
un blob, deux blobs, puis exécution annulée. Les 14 concordent avec Anvil et
les 14 avec Geth en séquence, état fourni complet et racines inclus. Les sondes
vérifient BLOBHASH (indices 0, 1 et hors limites), BLOBBASEFEE et le paiement
des frais de blob même en cas de REVERT.

Deux faiblesses ont été découvertes en inspectant le bytecode réellement déployé :

- Le premier pilote blob omettait un PUSH0 devant REVERT : il produisait une
  erreur de pile consommant tout le gas. Son résultat initial 14/14 est conservé
  mais ne prouve pas un REVERT. Le pilote corrigé exige aussi un gas consommé
  inférieur à la limite et l'absence de l'écriture de 43 dans le stockage.
- Contrairement au texte de la section précédente, la première campagne du
  cycle EIP-7702 écrivait encore 42, pas 43 : la substitution de texte n'avait
  pas modifié ce bytecode. Le contrôle ne démontrait donc pas l'annulation
  d'une valeur différente. Une campagne de remplacement utilise explicitement
  `602b6001555f5ffd`, qui écrit 43 puis exécute REVERT(0,0).

Ces corrections concernent le pilote, pas le moteur Nano. Les résultats initiaux
restent référencés. Preuves blob : `blobSignedReferenceComparison`.
La disponibilité, le transport et la rétention des sidecars par Nano ne sont
pas couverts ; Osaka/sidecar v1 reste à vérifier séparément.

Le rejeu du cycle EIP-7702 avec le bytecode de stockage effectivement corrigé
est terminé : Anvil 125/129 (uniquement les quatre écarts connus), Geth en
séquence 129/129, avec racines identiques. Les vérifications explicites portent
maintenant sur le retour de 43 à 42 et sur un vrai REVERT à gas non épuisé.
Preuves de remplacement : `authorizationLifecycleStorageCorrection` ;
l'ancienne campagne n'est pas écrasée.

## Osaka : sidecar version 1 refusé par Anvil, vérifié par Geth

Le générateur de référence Geth produit et vérifie 128 preuves de cellules par
blob. Un premier lancement hors du module Go échoue, le lancement dans le
module épinglé réussit ; les deux journaux sont conservés.

Anvil 1.5.1 Osaka accepte les quatre transactions de préparation, puis refuse
le premier paquet type 3 avec sidecar v1 : RPC -32602 « Failed to decode
transaction ». Le rejet est reproduit avec journal détaillé. Aucun paquet v0
n'est substitué pour présenter la campagne v1 comme réussie.

Le décodeur de transactions de Geth v1.16.7 lit exactement ce paquet, reconnaît
la version 1, vérifie les 128 preuves, les hashes des engagements et l'identité
du corps signé/hash. Une preuve remplacée par 48 octets nuls est rejetée au
stade de vérification des cellules. Cela écarte une simple erreur du format de
paquet testé vis-à-vis de cette implémentation Geth.

Indépendamment du rejet RPC Anvil, sept transactions du scénario blobs sont
rejouées dans Nano/Geth avec les règles Osaka : 7 concordances, état fourni
et racines compris. Le jeu de données réutilise explicitement les corps signés
et le contexte sauvegardé Prague, en changeant seulement le sélecteur de fork ;
il ne prétend pas constituer des blocs Osaka validés par consensus.

Preuves : `osakaBlobSidecarV1Reference`. Le rejet Anvil reste visible et la
qualification du transport/stockage/PeerDAS des sidecars par Nano reste ouverte.

## Intégrité des index de campagnes et de leurs pièces

L'auditeur vérifie désormais aussi les `artifactIndex`/`artifactIndices` du
manifeste : empreinte de l'index, empreinte de chaque pièce, fichier manquant,
doublon, pièce non indexée et chemin sortant du répertoire de campagne. Les
liens symboliques de l'inventaire sont signalés. Quinze tests de l'auditeur
passent, dont altération, suppression, index modifié, échappement de chemin et
intégration au manifeste.

Le contrôle terminé valide 13 index et 4 942 pièces, sans anomalie détectée.
Les références de journaux et les 19 rapprochements historiques restent
vérifiés. Le rapport conserve `complete: false` : l'intégrité d'un journal ne
prouve pas sa réussite ni la justesse du logiciel testé. Les rejets Anvil,
faiblesses historiques de pilotes et limites de périmètre restent enregistrés.

Preuves : `artifactIndexIntegrityAudit`. Le hash du manifeste audité est celui
précédant l'ajout de cette entrée, afin d'éviter une référence circulaire.

## RPC : estimation limitée par les fonds disponibles

Le nouveau test reproduit une estimation acceptée alors que le compte manque
d'un wei pour payer 21 000 gas au plafond de frais et la valeur transférée.
La correction borne la recherche par (solde - valeur) / plafond de frais ;
EIP-1559 utilise maxFeePerGas, et non le prix effectif. Les frais omis ou
explicitement nuls restent des simulations sans plafond financier inventé.

Douze scénarios Paris/Shanghai × types 0/1/2 × solde exact/moins un wei
vérifient les limites, l'exécution signée au solde exact, les simulations sans
frais et la préservation de l'état canonique. Les 19 suites fonctionnelles
qualification_evm passent après correction (32,18 s). Le code Geth v1.16.7
a été consulté ; aucune comparaison RPC externe de ces cas n'est encore faite.

L'historique conserve la compilation initiale échouée (surcharge ambiguë),
le test ensuite passé sur un ancien binaire — exclu des preuves de régression —,
la compilation corrigée, l'échec effectivement reproduit et les vérifications
après correction. Référence : rpcEstimateGasAffordability dans le manifeste.

## RPC : comparaison externe des fonds disponibles

La campagne locale Nano–Anvil produit 30 concordances et 30 divergences sur
60 estimations (Paris/Shanghai, types 0/1/2). Anvil accepte les transferts
simples au solde insuffisant, utilisant tout le solde, ou limités à 20 999 gas ;
Nano les rejette. Les soldes et nonces des douze comptes restent inchangés.

La source Anvil épinglée explique ce résultat : do_estimate_gas_with_state
renvoie directement MIN_TRANSACTION_GAS pour un transfert simple vers une
adresse sans code, avant le calcul du plafond financier ou de gas.
Source : https://github.com/foundry-rs/foundry/blob/b0a9dd9ceda36f63e2326ce530c10e6916f4b8a2/crates/anvil/src/eth/api.rs#L2853-L2905

Le code de sortie 1 et les 30 divergences restent conservés. Aucune transaction
signée n'est envoyée dans cette campagne : le comportement d'admission ne peut
pas être déduit d'une estimation. Référence : rpcAffordabilityAnvilComparison.

## RPC : contre-vérification avec l'estimateur Geth 1.16.7

Le helper Go de référence appelle directement gasestimator.Estimate depuis
le module Geth épinglé, avec les mêmes 60 demandes et soldes que la campagne
Nano–Anvil. Il obtient 54 concordances et six divergences (sortie 1 conservée).
Les refus pour fonds insuffisants concordent. Les six écarts sont les demandes
finançables contenant gas=20999 : Geth ignore un gas fourni inférieur à 21000
(lignes 57–60 de gasestimator.go), tandis que Nano refuse de dépasser ce plafond.

Ce résultat vérifie l'estimateur Geth, pas son adaptateur HTTP ni l'admission
de transactions signées. Les soldes et nonces d'entrée restent inchangés.
Le choix de compatibilité pour ce plafond RPC reste ouvert et ne justifie
aucune modification des règles de validation des transactions signées.
Référence : rpcAffordabilityGethEstimator.

## RPC : correction de l'indication gas inférieure à 21 000

Le chemin DoEstimateGas / CallDefaults / ToMessage de Geth 1.16.7 confirme
que l'estimateur reçoit les valeurs inférieures à 21 000 sans rejet préalable.
Nano adopte ce traitement uniquement pour eth_estimateGas : il recherche
le gas nécessaire tout en conservant le plafond financier. eth_call et
l'admission RPC des transactions signées continuent d'appliquer le gas fourni.

La régression échoue avant correction. Après correction, les 60 demandes
concordent avec l'estimateur Geth, et 36/60 avec Anvil ; les 24 divergences
Anvil restantes portent sur les fonds insuffisants. Les 19 suites EVM passent.
Les tests couvrent les indications 0, 1 et 20 999, les soldes exacts/insuffisants,
les types 0/1/2 et Paris/Shanghai, avec rejet des appels et transactions signées
au gas insuffisant et préservation de l'état canonique.

Les campagnes précédentes restent conservées. Ce résultat ne couvre ni
le serveur HTTP de Geth exécuté en direct ni toute la compatibilité RPC.
Référence : rpcSubIntrinsicGasCompatibility.

## RPC : validation des arguments de simulation

Le test reproduit l'acceptation de paramètres incohérents dans les simulations.
Une validation commune à eth_call et eth_estimateGas rejette désormais le
mélange gasPrice/frais dynamiques, le pourboire supérieur au plafond (y compris
un pourboire positif sans plafond), le chainId étranger et les types supérieurs
à 2 avant leur conversion en unsigned. Ce dernier contrôle évite notamment
la troncature de type=0x100000000 en type 0.

Les 24 demandes invalides (six formes × deux méthodes × Paris/Shanghai) sont
rejetées sans modifier l'état canonique. L'échec avant correction est conservé ;
les 19 suites EVM passent après correction. Les règles Geth v1.16.7 ont été
inspectées dans CallDefaults et stateTransition.preCheck, mais ces nouvelles
demandes n'ont pas encore fait l'objet d'une comparaison RPC externe.
Référence : rpcSimulationArgumentValidation.

## RPC : GASPRICE implicite corrigé après comparaison Anvil

Quarante demandes externes couvrent les six formes invalides et quatre formes
de frais omis/nuls, sur eth_call/eth_estimateGas et Paris/Shanghai. Avant
correction, 32 concordent : Nano expose 1 Gwei pour gasPrice absent ou null.
La simulation utilise désormais zéro dans ces cas, sans changer les valeurs
par défaut des transactions signées. Le chemin Geth CallDefaults/ToMessage
confirme cette valeur par défaut.

Après correction, 36/40 demandes concordent. Les quatre écarts restants
concernent gasPrice + maxFeePerGas : Anvil accepte ce mélange, Nano applique
le rejet explicite de Geth. Les tests externes comparent l'acceptation des
estimations, pas leur gas exact ; eth_call compare la valeur GASPRICE renvoyée.
Les soldes, nonces et code de sonde restent inchangés. Les tests natifs vérifient
aussi un contrat dont la réussite dépend du prix zéro, puis l'exécution de
l'appel au gas estimé. Les 19 suites EVM passent.

Les deux campagnes externes conservent leur sortie 1 et leurs divergences.
Référence : rpcZeroDefaultGasPrice.

## RPC : les frais null ne sélectionnent plus EIP-1559

La matrice externe étendue reproduit un GASPRICE nul malgré un gasPrice
explicite à 2 Gwei lorsque maxFeePerGas est présent avec la valeur null.
Le parseur de transaction déduisait le type de la présence du champ.
Une adaptation propre aux simulations supprime les trois champs de frais
nuls avant le parsing ; elle est commune à l'exécution et à l'estimation.
Le parseur des transactions signées reste inchangé.

Avant correction : 52/60 concordances externes ; après : 56/60. Les quatre
écarts conservés restent le mélange de frais non nuls accepté par Anvil.
Les tests natifs couvrent neuf formes de frais sur Paris/Shanghai, le prix
retourné, le contrat conditionnel au prix et son exécution au gas estimé.
Les 19 suites EVM passent, sans modification canonique par les simulations.
Périmètre externe : résultat de eth_call et succès de l'estimation, sans
exiger l'égalité numérique des estimations. Référence : rpcNullFeeNormalization.

## RPC : sélection de calldata identique à Anvil/Geth

La comparaison externe reproduit 16 divergences sur 36 demandes : Nano
privilégiait data au lieu de input et rejetait plusieurs aliases null.
Le parseur propre aux simulations traite désormais null comme absent et
privilégie input, même vide, conformément à TransactionArgs.data de Geth
1.16.7 et aux observations Anvil. Les deux champs non nuls sont validés
comme octets avant sélection. Le contrôle de conflit du wallet signataire
reste séparé et inchangé.

Après correction, 36/36 demandes externes concordent : neuf formes × deux
méthodes × Paris/Shanghai. La comparaison vérifie les octets renvoyés par
eth_call et le succès de eth_estimateGas, pas l'égalité de ses valeurs de gas.
Les tests natifs exécutent aussi chaque appel au gas estimé et contrôlent
l'état canonique inchangé. Les 19 suites EVM passent.
Référence : rpcCalldataAliasCompatibility ; échec initial conservé.

## Comparaison HTTP réelle avec Geth 1.16.7

Le nouveau banc relance Nano et le binaire Geth épinglé avec les allocations
initiales correspondantes, sans validateur Nano ni pairs Geth. Les hauteurs
restent zéro ; soldes, nonces et code des sondes restent inchangés.
Il compare 156 demandes des trois campagnes RPC : budget financier, arguments
et valeurs par défaut de frais, puis aliases de calldata. Résultat : 152
concordances et quatre divergences conservées, sortie 1. Les quatre demandes
contiennent type=0x100000000 : TransactionArgs de Geth n'a pas de membre type
et ignore le champ ; Nano le rejette avant toute conversion pouvant tronquer
sa valeur. Les autres contrôles de frais et calldata concordent.

Périmètre : résultats eth_call et acceptation eth_estimateGas ; pas d'égalité
numérique exigée pour toutes les estimations, ni validation de transactions
signées, consensus, sidecars ou production. Le premier essai (30 demandes)
a échoué sur une assertion de solde parce que le validateur Nano produisait
des récompenses ; le second (60 demandes) s'est arrêté au Genesis Geth suivant
qui exigeait un solde zéro explicite pour les comptes avec code. Ces essais,
ainsi que la compilation hors ligne échouée puis réussie avec dépendances
épinglées téléchargées, sont conservés. Référence : gethHttpRpcComparison.

## Shanghai : admission des mêmes créations signées contre Geth

La comparaison HTTP de pools vérifie six transactions signées identiques :
pour chacun des types 0/1/2, une création de 49 153 octets est rejetée pour
la taille de l'initcode, puis une création de 49 152 octets au même nonce
est acceptée par Nano et Geth 1.16.7. Les hashes retournés correspondent
à Keccak(raw), et les nonces en attente passent de 0 à 1 uniquement après
l'admission valide. txpool_content confirme l'absence du hash rejeté et
la présence du hash valide sur les deux nœuds. Soldes/nonces canoniques
et hauteur restent inchangés. Résultat : 6/6, sortie 0.

Les deux premiers essais restent en échec : Geth signale une indexation
historique indisponible au bloc zéro, même après cinquante attentes bornées.
Le dernier essai conserve ces réponses et vérifie l'admission via les pools ;
il ne transforme pas l'erreur d'index en reçu null. Cette campagne ne prouve
ni inclusion, ni exécution, ni finalité. L'écart d'admission Anvil précédemment
observé reste conservé ; Geth confirme ici le rejet immédiat pratiqué par Nano.
Référence : gethShanghaiSignedAdmission.

## Couverture actuelle du corpus officiel transaction_tests

Le vérificateur d'archive confirme l'empreinte épinglée de tests@v20.0.0 et
compare chaque fichier transaction_tests au dépôt. Les 24 fichiers sont
identiques octet par octet ; aucun fichier manquant, supplémentaire ou modifié.
Le rejeu Release actuel produit 110 concordances, sans erreur, divergence
ni cas unsupported. Les identités exactes (cas, fork), les exceptions attendues
et la préservation de l'état sont contrôlées : aucune omission ni duplication.

Cette exhaustivité porte seulement sur transaction_tests de cette archive :
55 cas invalides EIP-7702 Prague et 55 Osaka, aucun cas positif. Elle ne
transforme pas le runner spécialisé en qualification générale des transactions.
Les anciens échecs et résultats restent conservés.
Référence : currentCompleteTransactionCorpus.

## BFT local : correction d'une assertion de reprise trop large

L'extension Paris/Shanghai du test SIGKILL passait initialement avec une
assertion attrapant toute exception. Le signataire brut utilisé ne correspondait
pas au mode consensus du journal. Exiger le motif exact de refus a fait échouer
les trois profils. Le test utilise désormais sign_consensus_vote avec une
proposition alternative valide, et vérifie le refus de double vote avant
signature, le vote original inchangé et la reprise exacte de sa signature.
Les trois profils passent, neuf arrêts brutaux au total ; seules ces frontières
locales avec un validateur d'amorçage sont couvertes.
Référence : localBftCrashRecovery ; aucun résultat intermédiaire supprimé.

## BFT : instrumentation de connexion au seuil exact de deux tiers

La première campagne du seuil exact échouait sur « two-thirds pair not
connected » pour Paris et Shanghai ; les deux anciens scénarios pondérés
passaient. Le contrôle lisait net_peerCount en mode serve sans rapport réseau
authentifié du superviseur. Ce compteur ne prouvait donc pas la connexion
des relais lancés directement par le test. Le contrôle utilise maintenant
les événements peer de chaque relais et vérifie l’identité du pair attendu.
Les deux scénarios passent après correction ; le journal initial avec ses
deux échecs est conservé dans localBftStrictTwoThirds. Ce résultat ne constitue
pas une preuve d’absence générale de défaut de consensus.

## Synchronisation : corrections du mécanisme de panne de test

La première interception fsync ne déclenchait aucune panne car le chemin
observé utilise fcntl(F_FULLFSYNC). Les deux profils échouaient explicitement
sur l’absence d’injection. L’extension a ensuite rencontré un nom de constante
incorrect à la compilation, puis deux segfaults du mécanisme d’interposition
avec résolution dlsym. Le passage aux références directes des symboles dans
la bibliothèque d’interposition permet les deux scénarios complets.
Tous les builds et campagnes sont conservés dans localStorageSyncFailure.
Ces erreurs de test ne sont pas présentées comme des attaques ou des défauts
de consensus Nano. La bibliothèque n’est pas liée au binaire nano, ce qui
est vérifié et enregistré dans les artefacts.

## Charge BFT : absence de confirmation pendant la fenêtre courte

Les deux campagnes à 4 × 20 transactions espacées de 250 ms échouent sur
« no observed finality while senders were still active ». Les campagnes
étendues à 4 × 60 au même rythme passent avec reçus/certificats/état/reprise
vérifiés ; elles ne ferment pas l’écart de latence initial. Deux échecs de
compilation du nouveau test (conversions GMP et JSON) restent également
conservés. Référence : localBftConcurrentLoad. Le profil CPU conservé
identifie un réaudit complet de l’historique dans le parcours de ronde ;
aucune optimisation du moteur n’a encore été appliquée dans cette campagne.

## Charge courte toujours en échec après réutilisation de l’audit

Les tests bft_load_short_paris_network et bft_load_short_shanghai_network
restent en échec sur le critère de confirmation pendant les envois. Les
scénarios prolongés passent et les régressions de sécurité restent vertes ;
aucun de ces succès ne transforme les essais courts en résultats résolus.
Référence : bftCertifiedAuditReuse. L’optimisation enlève la réexécution
répétée du préfixe déjà vérifié, mais le diagnostic de latence continue.

## Charge courte : passages instrumentés non retenus comme résolution

Après avancement BFT avant réponse RPC, quatre essais avec lectures de
diagnostic passent. Leur répétition sans ces lectures échoue sur les deux
profils : le résultat dépend encore de l’ordonnancement et de la fenêtre
d’observation. Les premiers passages ne sont pas retenus pour fermer
le défaut de latence. Le manifeste conserve les campagnes avant/après
et instrumentées/non instrumentées : bftImmediateReplyDiagnostics.

## Suivi du défaut de confirmation courte

Après séparation du cycle BFT du relais à 500 ms, quatre répétitions
non instrumentées passent avec le critère original. Les six tests du relais
et les quatre scénarios quorum/reprise passent également. Cette preuve
locale est enregistrée dans bftRelayIndependentCadence ; les précédents
échecs instrumentés et non instrumentés ne sont ni supprimés ni réétiquetés.
La distribution de latence, le WAN et la charge longue restent ouverts.

## Arrêt brutal pendant la charge : reprise encore intermittente

Le scénario `--load-crash` arrête le relais puis tue le cœur du premier
validateur avec SIGKILL pendant quatre séries de transactions espacées de
250 ms. Une transaction témoin est envoyée seulement après sa mort confirmée.
Les trois survivants doivent la certifier avant la fin des envois ; le
validateur redémarre avec sa base et son journal, mais une nouvelle identité
P2P de test. Les clients restent connectés aux trois survivants.

La campagne de 240 transactions échoue sur Paris (plus d'envois actifs au
redémarrage) et passe sur Shanghai. Celle de 360 transactions échoue sur
les deux profils : aucune confirmation de la transaction témoin n'est
observée avant la fin des envois. Allonger la charge n'a donc pas résolu
le défaut. Ces échecs ne démontrent pas une falsification ou un vol.

Le test relève désormais les hauteurs, tours, propositions et votes des
survivants après l'échec du critère, sans prolonger sa fenêtre d'acceptation.
La concurrence entre gossip et consensus dans le budget entrant partagé
reste une hypothèse à vérifier, pas une cause établie. Référence du manifeste :
`localBftCrashDuringLoad`. Aucun passage isolé ne clôt ce défaut intermittent.

La répétition avec diagnostic uniquement après échec passe sur les deux
profils (360 transactions chacun, quatre bases rouvertes et auditées).
Paris confirme le témoin à 9,13 s et Shanghai à 8,30 s depuis le début des
envois. Aucun changement de production ne sépare cette répétition de la
campagne précédente en échec : le défaut reste ouvert, sans cause prouvée.

Une nouvelle répétition Paris échoue à 23,65 s : deux survivants sont au
bloc 3, le troisième au bloc 2 avec seulement deux pré-engagements pour
le bloc 3. Les trois cœurs et relais sont vivants. Une campagne avec
`NANO_TEST_NETWORK_TRACE=1` échoue aussi à 24,50 s. Les compteurs montrent
des refus du budget partagé pour BFT, blocksync et blockannounce, pendant
que les trois survivants restent connectés. Cette seconde capture les
montre au bloc 5, tour 1 : elle ne prouve pas un blocage permanent.

La contention du budget est donc observée ; son rôle exact dans le délai
de la transaction témoin reste à isoler. La diffusion des transactions
sans nouvelle tentative et les éventuels trous de nonce doivent aussi
être examinés. Les limites et règles BFT sont inchangées. Le diagnostic
réseau est désactivé par défaut, limité à huit relevés par relais ; les
six tests unitaires réseau passent. Rapports et captures :
`localBftCrashDuringLoad` dans le manifeste.

## Réserve réseau après observation de contention

Le budget des transactions/annonces/handshakes est maintenant un sous-budget
du budget total original ; BFT, chainsync et blocksync conservent une capacité
réservée. Aucun plafond total, seuil de quorum ou contrôle cryptographique
n'a été augmenté ou retiré. Les nouveaux tests vérifient la réserve sous
épuisement des jetons, concurrence et rotation d'identités ; les six tests
du relais passent.

La campagne sans instrumentation de reprise après SIGKILL passe sur Paris
et Shanghai avec 360 transactions par profil, certificats, reçus et quatre
bases rouvertes. La transaction témoin est confirmée à 4,48 s puis 21,59 s :
la variabilité et la diffusion sans reprise restent ouvertes. Les précédents
échecs restent conservés. Preuves : `bftReservedInboundBudget`.

Les scénarios de quorum strict passent également sur les deux profils :
4/6 des voix ne finalisent pas, 5/6 finalisent, puis tous les pairs convergent.

## Diffusion v2 avec perte d’accusé

La campagne avec perte volontaire de réponse après traitement passe sur
Paris et Shanghai (360 transactions chacun), avec arrêt brutal et retour
du validateur, vérification des certificats et réouverture des quatre bases.
Le test exige une injection effective dans chaque relais survivant. Les
tests RPC vérifient l’idempotence avant et après inclusion, les limites et
les éléments invalides ; les six tests réseau passent.

Cette convergence ne remplace pas une assertion directe de nouvelle tentative
côté émetteur. Expiration du journal, redémarrage du cœur, rejets temporaires
et budget CPU des lots restent ouverts. Référence : reliableTransactionGossip.

## Preuve directe de reprise des accusés

Le suivi `gossipAcknowledgementProof` ajoute la vérification du curseur côté
émetteur face aux accusés absents, malformés, obsolètes et répétés. Les sept
tests du relais passent. Dans la campagne distribuée Paris/Shanghai avec
arrêt brutal, les trois pertes injectées chez les survivants sont chacune
reliées à une reprise confirmée par le bon émetteur, vers le bon récepteur
et depuis le curseur attendu (3/3 par profil). Les 720 transactions,
certificats et quatre bases rouvertes par profil sont vérifiés.

Cette preuve complète le contrôle indirect de convergence de la campagne
précédente. Elle ne ferme pas les limites de rétention du journal, de
redémarrage, de rejets temporaires ou de budget CPU des lots.

## Budget des éléments de diffusion

Le suivi `gossipTransactionWorkBudget` limite les éléments transmis au cœur
par le relais, en complément des quotas de requêtes. Les sept tests réseau
passent, notamment pour les rafales, changements d’identité, réouvertures
de flux et horloge monotone. La campagne Paris/Shanghai avec arrêt brutal
et pertes d’accusés passe : 360 transactions par profil, correspondance
des pertes et reprises, certificats et quatre bases rouvertes.

La limitation du relais ne couvre pas les appels HTTP directs et ne constitue
pas une mesure de temps CPU absolu. Ces deux points restent ouverts, ainsi
que l’expiration du journal et les rejets temporaires d’admission.

## Admission RPC partagée et adaptation des clients de charge

Le budget commun protège les lots HTTP et les envois unitaires avant
admission. Les tests HTTP (nouvelles connexions et tableaux JSON-RPC) et RPC
(rafale, remplissage monotone, crédit borné) passent. Les deux anciens tests
de charge échouent sur le refus temporaire -32005 ; ces résultats restent
conservés. Les clients adaptés ne réessaient que ce code explicite, jamais
une erreur de transport, avec délai global de 60 s et lectures de santé.
Ils conservent les exigences initiales : 400 transactions valides et 400
rejets pour mauvaise chaîne par profil, puis reçus, racines, audits et
réouverture. Les deux profils passent (1363 reprises Paris, 1378 Shanghai).
Le débit nominal historique du rapport exclut ces requêtes supplémentaires.

Référence : rpcTransactionAdmissionBudget. La régression BFT distribuée
sous ce budget commun, l’équité entre clients et les gros payloads restent
à qualifier. Une erreur initiale de script d’édition est aussi conservée ;
la compilation suivant cette erreur portait encore sur les anciennes sources RPC.

## Régression distribuée et passage HTTP vers WebSocket

Avec le budget commun du cœur actif, la campagne Paris/Shanghai passe :
360 transactions par profil, arrêt brutal et retour du validateur, pertes
d’accusés reliées à leurs reprises, certificats et quatre bases rouvertes.
Les clients BFT conservent leur cadence et leur comportement originaux ;
aucune attente de surcharge n’a été ajoutée à ce scénario.

Le test HTTP vérifie aussi une nouvelle connexion WebSocket après épuisement
du budget par HTTP : elle reçoit -32005 pour l’admission, mais une lecture
eth_chainId réussit. Preuves : rpcBudgetIntegration. Cela ne démontre pas
l’équité sous attaque soutenue ni la reprise d’un journal expiré.

## Inventaire après expiration du journal

Le test natif franchit la borne réelle de 10 000 événements, parcourt le
pool par pages et vérifie la couverture de chacune des 10 000 transactions
encore présentes. Une insertion intercalée derrière le curseur de hash est
retrouvée par le flux d’événements depuis la barrière initiale. La réouverture
du cœur invalide la session et le curseur de page précédents.

Les tests RPC et les sept tests du relais passent. La régression BFT
Paris/Shanghai passe aussi avec 720 transactions, arrêt brutal et retour
du validateur, 3/3 accusés perdus reliés aux reprises par profil, certificats
et quatre bases rouvertes. Cette campagne réseau ne franchit pas elle-même
la borne des 10 000 événements : ce scénario distribué reste à qualifier.
Le pool n’est pas rendu persistant et un redémarrage distant conservant
l’identité P2P nécessite encore un mécanisme de resynchronisation.
Preuves : gossipInventoryRecovery. La première compilation en échec est
conservée (appel à un auxiliaire aléatoire interne non exposé).

## Historique du pool et réadmission

Le nouveau test échoue initialement : un hash évincé reste définitivement
connu et empêche sa réadmission même avec une place libre. Le cache est
désormais limité aux hashes minés récents, borné par la capacité du pool ;
les doublons actifs sont vérifiés dans les entrées elles-mêmes. Un second
test expose une réadmission moins rémunératrice chassant une meilleure
transaction d’un pool plein. Ce cas est désormais refusé ; les frais égaux
conservent la règle d’arrivée précédente.

Les cinq suites finales passent (historique du pool, cœur, chaîne, RPC et
inventaire après expiration). Le replay d’une transaction minée reste refusé
après remise à zéro du cache, par le nonce canonique persistant. Les deux
échecs intermédiaires sont conservés dans boundedPoolHistory. La limite
agrégée des octets du pool et l’équité sous churn restent à traiter.

## Pool : plafond agrégé des octets sérialisés

Le test initial de 65 transactions signées proches de 1 Mio échoue avec
« default pool exceeds 64 MiB serialized transaction budget » (exit 8).
La limite en nombre ne suffisait donc pas à borner ce volume à 64 Mio.
Le pool applique désormais ce budget en plus des plafonds existants et
prépare toutes les évictions avant mutation. Un refus, y compris celui
d'un remplacement qui demanderait une autre éviction trop chère, préserve
les entrées et leur compteur. Les retraits et inclusions libèrent les octets
exactement une fois. Le test compare le compteur à une sérialisation
indépendante de l'inventaire et couvre les remplacements plus petits/grands.

Après correction, les six suites `core`, `chain`, `rpc`, `gossip_recovery`,
`txpool_history` et `txpool_bytes` passent. La campagne est hors réseau.
Les logs avant/après et empreintes des sources/binaires sont conservés
sous `boundedPoolBytes`. Cela ne prouve pas une borne RSS globale,
l'équité entre clients ni la stabilité sous saturation distribuée.

## Charge RPC : comptage réel des requêtes des clients

L'ancien champ `mixedHttpRequestsPerSecond` excluait les nouvelles tentatives
après limitation et les lectures de santé associées. Les rapports historiques
restent inchangés. Le nouveau rapport expose `submissionAttempts`,
`workerHttpRequests`, `workerHttpRequestsPerSecond`, `admissionSeconds`,
`nominalOperationsPerSecond` et `acceptedTransactionsPerSecond`. Le débit
des requêtes inclut les refus de quota ; il ne mesure pas le débit de blocs.
Le périmètre exclut l'éventuel flood, la lecture de récupération finale et
le rejeu direct des blocs. Des assertions vérifient les identités comptables
entre tentatives, backoffs et lectures réussies.

Les campagnes Paris et Shanghai passent avec 8 clients et 400 transactions
valides plus 400 transactions étrangères chacune, sans flood. Elles vérifient
aussi les hashes retournés, reçus, soldes, rejeu sur un pair, audit et réouverture.
Preuves : `rpcLoadRequestAccounting`. Cette mesure locale ne qualifie pas
la capacité d'un réseau BFT distribué ni l'équité entre clients.

## Gossip : détection de session du cœur distant

Le statut `nano_networkStatus` expose désormais `gossip_epoch`, l'identifiant
aléatoire du processus déjà utilisé par les pages locales. Le relais compare
ce champ lors des handshakes et réponses chainsync ; un changement remplace
l'objet de progression du pair afin de recommencer l'inventaire. Les callbacks
en vol gardent l'ancien objet et ne peuvent avancer le nouveau curseur.
Un statut ancien sans ce champ reste compatible mais ne fournit pas cette
détection. Un champ présent doit être un hexadécimal minuscule de 16 octets.

Sept tests natifs du relais et la régression RPC passent. Le test de progression
vérifie les sessions inchangées, modifiées, disparues et les réponses tardives.
La retransmission réelle après redémarrage du cœur avec identité P2P conservée
reste à tester entre processus ; elle n'est pas déclarée qualifiée ici.
Preuves : `gossipRemoteSessionDetection`.

## Gossip : reprise réelle après crash du cœur destinataire

Le test `gossip_remote_restart` lance deux cœurs Shanghai et deux relais
locaux. Une transaction signée arrive d'abord au destinataire. Les relais
sont brièvement suspendus pendant le SIGKILL et la réouverture de son cœur
sur les mêmes fichiers et port RPC : le test constate le nouvel epoch et
l'absence de la transaction dans le pool volatil avant leur reprise.
Sans nouvel envoi utilisateur ni bloc produit, la transaction revient.
Le diagnostic corrèle les deux epochs avec l'identité P2P initiale, et
les deux processus relais restent vivants. Le pool contient une seule entrée.

Le premier passage échoue sur une assertion de format (`"0x1"` attendu,
`1` renvoyé par `txpool_status`), après la retransmission réussie. Le passage
corrigé réussit ; le format RPC n'a pas été changé et sa compatibilité reste
à examiner séparément. Tous les logs sont conservés sous
`gossipRemoteRestartIntegration`. La pause contrôlée ne qualifie pas les
courses d'un redémarrage sans pause ni un inventaire de plusieurs pages.

## Gossip : reprise de plusieurs pages sans pause des relais

Le scénario `gossip_remote_restart_live_pages` envoie 75 transactions signées
et attend leur présence dans le pool distant avant le SIGKILL de son cœur.
Les deux processus relais continuent de tourner pendant la réouverture
sur la même base et le même port. L'epoch change, la trace de remise à zéro
conserve la même identité P2P, et les 75 transactions reviennent en attente
sans nouvel envoi utilisateur ni bloc produit. Chaque hash est contrôlé
et le compte final est exactement 75. La limite de page de 50 implique
plusieurs lots pour retransmettre cet ensemble.

Ce passage réussit ainsi que le scénario précédent avec pause contrôlée.
Preuves : `gossipRemoteRestartLivePages`. Un seul calendrier de crash sans
pause a été exercé : cela ne qualifie pas toutes les courses de réponses
en vol, les expirations de journal distribuées ni les refus temporaires.

## RPC : quantités hexadécimales dans txpool_status

La référence [Execution APIs](https://ethereum.github.io/execution-apis/api/methods/txpool_status/)
attend des compteurs hexadécimaux. Le test initial refuse les nombres JSON
renvoyés par Nano. La frontière RPC encode désormais `pending` et `queued`
en quantités (`"0x0"`, `"0x1"`), sans modifier les compteurs internes.
Un premier build échoue sur un constructeur GMP ambigu avec uint64_t ;
la conversion décimale explicite corrige ce problème de portabilité.

Les deux tests de reprise gossip passent après correction de leurs attentes,
mais la suite RPC révèle aussi une ancienne fixture Python avec compteurs
numériques. Cette fixture reste intacte : son attente est remplacée
explicitement dans le test par la forme actuelle, et la suite RPC passe
ensuite. Les tests couvrent les pools vide et non vide. Tous les échecs
sont conservés sous `txpoolStatusRpcQuantities`. Le classement des nonces
à trous entre pending et queued reste à comparer aux clients de référence ;
cette correction ne prétend pas qualifier tout l'espace txpool.

## Pool : classement des nonces à trous

Le nouveau test échoue initialement car le nonce 2 seul est annoncé pending
alors que le compte est au nonce 0. Le classement utilise désormais le nonce
canonique du compte et trie les entrées par nonce numérique : la suite
contiguë est pending, les entrées après un trou sont queued. Le compteur
utilise des pointeurs vers les entrées sous verrou, sans sérialiser leurs
payloads. Le contenu détaillé applique le même classement.

Les tests Paris et Shanghai vérifient 2 seul, puis ajout de 0, inclusion de 0,
ajout de 1, promotion de 1/2 et exécution dans cet ordre, avec audit final.
Les quatre suites chain, RPC et reprises gossip passent. L'échec initial et
les builds sont conservés sous `txpoolNonceGapClassification`. Cette preuve
porte sur les trous de nonce ; les règles de solvabilité cumulée et de frais
restent à comparer aux clients de référence. Le nettoyage des alternatives
de même nonce après inclusion externe d'une transaction reste à qualifier.

## Pool : alternative obsolète après inclusion concurrente

Le test Paris/Shanghai reproduit une entrée locale conservée alors qu'un
autre nœud a inclus une transaction différente au même nonce. Après
validation et enregistrement du nouvel état canonique, les chemins
d'importation et de production retirent désormais les entrées dont le nonce
est inférieur à celui du compte. Les octets correspondants sont libérés
une seule fois. Les transactions de nonce supérieur restent dans le pool.

Le test refuse d'abord un bloc à racine altérée et vérifie que rien n'est
retiré, puis importe le bloc valide, constate le retrait de l'alternative,
le rejet de sa réadmission et l'exécution du nonce suivant. L'audit final
passe pour Paris et Shanghai. Le test de budget vérifie aussi les octets
libérés et l'idempotence. Les quatre suites chain, RPC, historique et octets
passent ; l'échec initial reste conservé sous `txpoolStaleNonceCleanup`.
Cela ne qualifie pas encore le scénario BFT distribué avec gossip concurrent.

## Régression BFT après durcissement du pool

Les binaires actuels passent les deux campagnes Paris/Shanghai à quatre
validateurs avec 360 transactions chacune, quatre clients espacés de 250 ms,
crash/reprise du quatrième cœur et perte volontaire d'accusés de gossip.
Un transfert témoin est finalisé par les trois survivants pendant l'absence
du quatrième et avant la fin des envois. Les 360 reçus sont ensuite comparés
sur chaque nœud, avec soldes exacts, certificats, état commun et réouverture
des quatre bases. Les traces corrèlent les pertes d'accusés aux reprises.

Preuves : `poolHardeningBftRegression`. Il s'agit d'une régression locale
sans flood ; elle ne cible pas deux transactions concurrentes de même nonce
et ne remplace pas la qualification globale. Les échecs historiques des
campagnes antérieures restent conservés dans leurs sections.

## Gossip : réconciliation périodique bornée

Un accusé `accepted:false` avance le curseur de traitement ; sans autre
mécanisme, une transaction refusée temporairement pouvait ne jamais être
proposée à nouveau. Le relais planifie désormais un inventaire complet
60 secondes après la fin du précédent. Les limites existantes de pages,
requêtes simultanées et admission restent actives. Le timer ne réinitialise
pas un inventaire en cours, une requête active ou un lot en échec.
Les événements continuent de circuler entre inventaires.

Les tests à horloge contrôlée vérifient échéance, pagination longue et
protection des requêtes en vol. Les sept tests du relais et les deux reprises
réseau passent. Preuves : `gossipPeriodicInventory`. Le scénario complet
refus temporaire puis admission après 60 secondes réelles reste à exécuter,
ainsi que la charge BFT traversant plusieurs cycles. Seules les transactions
encore conservées par un émetteur peuvent être reproposées.

## Gossip : refus temporaire suivi d'une reprise réelle

`gossip_temporary_rejection` utilise deux instances de cœur/RPC en processus
et deux processus relais réels. Chaque cœur conserve une transaction du
même compte et nonce, à 2,0 et 2,1 Gwei : ni l'une ni l'autre ne satisfait
le remplacement de 10 %. Les deux accusés de refus sont observés. Le test
retire ensuite l'obstacle par un `restore` interne au même tip, sans RPC
de mutation de production, changement d'epoch ni redémarrage.

La transaction originale est admise après 60,2 secondes, sans nouvel envoi
source ; les deux pools ont exactement une entrée et aucun bloc n'a été
produit. Le test passe en 62,1 secondes. Preuves et empreintes :
`gossipTemporaryRejectionRecovery`. La levée d'obstacle est contrôlée ;
une résolution naturelle par financement ou changement de capacité, ainsi
que plusieurs cycles sous charge BFT, restent à qualifier.

## Gossip : refus de capacité résolu par un bloc signé

`gossip_capacity_recovery` remplit le quota du compte destinataire avec
100 transactions. Le nonce 100 reste à la source et est réellement refusé
par le destinataire ; le relais observe son accusé de traitement avec refus.
Un premier bloc signé inclut les 100 transactions, libère la capacité et
est propagé à la source. La transaction revient après 59,97 secondes,
sans nouvel envoi ni changement de session. Un second bloc signé l'exécute.

Les deux cœurs concordent sur les reçus, la racine d'état et les audits
d'historique ; leurs pools finissent vides. Le test passe en 63,29 secondes
avec deux instances de cœur/RPC et deux processus relais locaux. Aucun
restore, vidage artificiel ni redémarrage n'est utilisé. Preuves :
`gossipCapacityRecovery`. Les blocs sont signés par le validateur bootstrap :
ce scénario précis ne démontre pas encore la reprise sous quorum BFT.

## BFT : charge traversant la réconciliation périodique

`bft_reconcile_paris_network` et `bft_reconcile_shanghai_network` passent
avec quatre validateurs et 360 transferts chacun, envoyés par quatre clients
à un transfert par seconde pendant environ 90 secondes. Les traces de
réponses d'inventaire sont horodatées avec l'horloge monotone et comparées
à l'intervalle réel des envois : 12 lots périodiques non vides sont observés
sur chaque profil pendant cet intervalle. Le test exige leur présence.

Les reçus, soldes, certificats, racines communes et quatre bases rouvertes
sont vérifiés par profil. Les sept tests natifs du relais passent aussi.
Preuves : `bftPeriodicInventoryLoad`. La campagne totale dure 486,78 secondes,
contrôles finaux compris. Elle traverse un premier cycle pendant les envois,
mais ne qualifie pas plusieurs cycles actifs, une partition simultanée
ou l'équité à la taille maximale du réseau.

## Pool : transactions signées comparées à Anvil

`compare-txpool-nonces.mjs` envoie les mêmes 24 transactions signées
(types 0/1/2, Paris/Shanghai), dans l'ordre de nonces 10, 2, 0, 1, sans
mining. Les hashes RPC correspondent tous au keccak des enveloppes.
Sur 30 instantanés, les compteurs et nonces pending concordent 30/30.
La comparaison complète conserve 24 divergences et sort à 1 : Anvil
place tous les objets de transaction dans `txpool_content.pending`,
y compris les nonces comptés comme queued dans son propre statut.

La [source épinglée d'Anvil](https://github.com/foundry-rs/foundry/blob/b0a9dd9ceda36f63e2326ce530c10e6916f4b8a2/crates/anvil/src/eth/api.rs#L2725-L2734)
explique cette observation : les deux boucles des transactions prêtes
et futures insèrent dans `content.pending`. Nano conserve son classement
par continuité du nonce. Rapport brut, enveloppes, sorties et empreintes
sont conservés sous `txpoolNonceAnvilComparison`. Une comparaison
indépendante Geth reste à réaliser ; aucune égalité globale avec Anvil
n'est déclarée. Aucun changement de production n'a été fait sur ce constat.

## Pool : vérification indépendante Geth des nonces à trous

`compare-txpool-nonces-geth.mjs` reprend exactement les 24 enveloppes
signées du rapport Anvil conservé. Geth 1.16.7 et Nano concordent sur
30/30 instantanés pour les types 0/1/2 en Paris/Shanghai : quantités
pending/queued, nonce pending et répartition exacte des hashes entre
les deux catégories de contenu. Les hashes d'admission correspondent
au keccak des mêmes octets ; les soldes et nonces canoniques restent
inchangés. Aucun pair public n'est connecté.

Rapports, genesis, sorties et empreintes sont sous
`txpoolNonceGethComparison`. Le résultat Anvil précédent reste 6/30 avec
24 divergences expliquées, sans remplacement ni conversion en succès.
Ce passage ne couvre pas encore la solvabilité cumulée, toutes les
politiques de remplacement ni tous les champs de métadonnées txpool.

## Pool : remplacement EIP-1559 sur les deux plafonds

La comparaison de mêmes transactions signées avec Geth 1.16.7 révèle
10 divergences sur 14 étapes Paris/Shanghai : Nano ne vérifiait que la
hausse du plafond total et acceptait un pourboire inchangé ou abaissé.
Cela pouvait aussi bloquer ensuite le remplacement correct. Le contrôle
exige désormais que les deux plafonds augmentent strictement et atteignent
le seuil de 10 % arrondi à l'entier inférieur, comme la
[source épinglée Geth](https://github.com/ethereum/go-ethereum/blob/v1.16.7/core/txpool/legacypool/list.go#L280-L299).

La campagne corrigée concorde 14/14 sur l'admission et les hashes restant
dans le pool, avec exactement les mêmes enveloppes avant/après. Les quatre
suites chain, RPC, budget et historique passent. Un test natif vérifie
l'arrondi à quelques wei et la conservation du compteur lors d'un refus.
Les rapports et l'échec initial sont conservés sous `txpoolReplacementGeth`.
La matrice des remplacements entre types de transactions et les autres
frontières entières restent à qualifier.

## Pool : matrice des remplacements entre types 0/1/2

`compare-txpool-replacement-matrix.mjs` vérifie les neuf directions de
remplacement entre les types 0, 1 et 2 sous Paris et Shanghai. Chaque
frontière utilise un nonce distinct : plafond un wei sous le seuil,
pourboire un wei sous le seuil, puis seuil exact arrondi à l'entier inférieur.
Les anciennes valeurs finissent à un wei pour exercer l'arrondi. Pour
les types 0/1, gasPrice représente les deux plafonds ; le scénario de
pourboire inférieur ne change donc pas leur gasPrice admissible.

Les 108 étapes concordent avec les attentes d'admission et Geth 1.16.7.
Les hashes des mêmes octets signés et les ensembles de hashes restant
pending sont comparés après chaque étape ; soldes et nonces canoniques
restent inchangés. Preuves : `txpoolReplacementTypeMatrix`. Aucun changement
de production supplémentaire n'a été nécessaire. Pourboire nul, limites
uint256 et concurrence avec l'inclusion d'un bloc restent à qualifier.

## Pool : solvabilité cumulée des transactions admises

La campagne signée Paris/Shanghai types 0/1/2 ne concorde initialement
que 6/30 avec Geth : Nano admet deux transferts consommant chacun 60 %
du solde. Le second bloque ensuite un transfert plus petit au même nonce.
L'admission réserve désormais la somme valeur + gas × plafond de frais
des autres entrées du compte. L'entrée de même nonce est exclue pour
permettre son remplacement sans compter deux fois son coût. Ce calcul
parcourt les entrées sous verrou sans copier leurs payloads.

Après correction, les mêmes enveloppes concordent 30/30 sur les décisions
et les partitions de hashes. Les quatre suites chain, RPC, octets et
historique passent. Le test natif vérifie aussi un remplacement autorisé
après exclusion de l'ancien coût, puis un remplacement trop coûteux qui
préserve le pool et le solde canonique. Preuves avant/après :
`txpoolCumulativeAffordability`. Le rafraîchissement après variation du solde
par exécution de bloc et les interactions avec les évictions restent à tester.

## Pool : réservation limitée à la suite pending

L'extension aux nonces 10, 20, 20, 0, 0 révèle que le premier contrôle
cumulé était trop restrictif : il réservait aussi les transactions futures.
La campagne initiale ne concorde que 6/30 avec Geth. La réservation parcourt
désormais la suite contiguë depuis le nonce canonique et s'arrête au premier
trou ; l'ancienne entrée remplacée est exclue du coût sans casser la suite.
Cela suit les [callbacks de Geth épinglé](https://github.com/ethereum/go-ethereum/blob/v1.16.7/core/txpool/legacypool/legacypool.go#L519-L532).

La campagne future corrigée et la régression contiguë passent chacune 30/30
pour Paris/Shanghai types 0/1/2. Les quatre suites natives passent aussi.
Preuves : `txpoolFutureAffordability`. Cette section corrige explicitement
la portée de réservation de la section précédente ; ses échecs restent
conservés. La promotion d'un suffixe devenu contigu mais trop coûteux
après remplissage du dernier trou reste à qualifier.

## Pool : promotion d'un suffixe au coût cumulé excessif

La campagne `txpoolGapPromotionGeth` envoie d'abord un nonce 1 consommant
60 % du solde, puis le nonce 0 consommant également 60 %, puis un nonce 2
à 5 %. Nano et Geth 1.16.7 concordent 18/18 pour les types 0/1/2 sous
Paris/Shanghai. Les deux premières transactions sont admises ; la fermeture
du trou les rend pending même si leur coût cumulé dépasse le solde.
La troisième est refusée par la réservation désormais présente.

Les enveloppes, partitions de hashes et réponses sont conservées. Soldes
et nonces canoniques restent inchangés puisqu'aucun bloc n'est exécuté.
Ce résultat ne signifie pas que toute la suite pourra être exécutée :
pending décrit ici la continuité des nonces, pas une garantie de solvabilité
globale après promotion. Le comportement d'exécution et le rafraîchissement
du pool après ce bloc restent à vérifier explicitement.

## Pool : nettoyage après baisse du solde canonique

Les tests Paris/Shanghai types 0/1/2 admettent les nonces futurs 1 (60 %
du solde) et 2 (5 %), puis 0 (60 %). L'exécution du premier bloc signé
n'inclut que le nonce 0 : le transfert non finançable n'est pas exécuté.
Le test initial échoue ensuite parce que l'entrée du nonce 1 reste au pool.
Le nettoyage après état canonique validé retire désormais aussi les entrées
dont valeur + gas × plafond de frais dépasse individuellement le solde actuel.

Après correction, le nonce 2 reste queued. Une nouvelle transaction au
nonce 1, de valeur 5 %, est admise puis exécutée avec le nonce 2. Les six
variantes vérifient l'ordre, l'absence de reçu pour le transfert non financé,
le total reçu de 70 % et l'audit final. Le test d'octets contrôle le retrait
exact et idempotent du coût de stockage. Les quatre suites natives passent.
Preuves : `txpoolPostBlockBalanceRefresh`. La comparaison d'exécution avec
un client de référence et la régression BFT distribuée restent à compléter.

## Pool après bloc : divergence Anvil isolée par retransmission

La campagne `txpoolPostBlockAnvilComparison` conserve le résultat initial
**0/12 concordances complètes** (code de sortie 1). Sous Paris/Shanghai,
pour les types 0/1/2, le premier bloc a les mêmes soldes, nonce et reçus.
Nano conserve le nonce 2 finançable en queued, tandis que le pool Anvil est
vide. Après remplacement du nonce 1 non finançable, Nano exécute 1 puis 2,
Anvil seulement 1 : les différences suivantes portent donc sur des blocs
contenant des ensembles de transactions différents.

Une campagne distincte active `NANO_TEST_REBROADCAST_SUFFIX=1` : elle
retransmet uniquement à Anvil l'enveloppe originale du nonce 2 avant le
second bloc. Les six premiers états exécutés concordent et les six
divergences de pool sont explicitement vérifiées et conservées. Les six
états après récupération concordent intégralement : soldes, valeur reçue,
nonce canonique, reçus normalisés (hash, statut, gas, prix effectif) et pool.
Le résultat JSON demeure 6/12 pour les snapshots complets ; le succès du
mode diagnostic signifie que les assertions spécifiques à cette
intervention passent, pas que le comportement naturel des pools est égal.

Cela isole une différence de rétention du pool dans ce scénario, sans
évidence de falsification des hashes ou d'exécution de la dépense non
finançable. Le mécanisme interne exact d'éviction Anvil n'est pas établi
par cette campagne. La comparaison après bloc avec Geth et la régression
BFT des dernières modifications restent ouvertes. Le code de production
Nano n'a pas été modifié pour reproduire l'éviction Anvil.

## Session gossip : valeurs malformées

`gossipMalformedEpochQualification` vérifie 14 valeurs malformées de
`gossip_epoch`, chacune du côté local puis distant (28 rejets), dont un
octet NUL dans une chaîne de longueur correcte. Les pairs sans ce champ
optionnel restent compatibles ; deux sessions valides différentes sont
acceptées. Les suites natives `peer_status` et `gossip_progress` passent.
Aucune modification du protocole ou du code de production ; cette preuve
couvre la validation native, pas une injection réseau de ces 14 valeurs.

## Régression BFT après réservation et nettoyage du pool

`poolPostBlockBftRegression` recompile le cœur et le relais puis exécute
Paris et Shanghai : 2/2 tests réussis en 224,94 s. Chaque profil utilise
quatre validateurs locaux et quatre clients envoyant 360 transferts au
total, à intervalle de 250 ms par client. Le quatrième validateur subit
un SIGKILL ; les trois survivants certifient une nouvelle transaction
après sa mort confirmée, pendant que les clients continuent à envoyer.
Le validateur redémarre et rejoint le réseau pendant les envois.

Les reçus complets concordent sur les quatre nœuds, le bénéficiaire
reçoit exactement 360 unités, les certificats sont vérifiés et les racines
finales concordent. Les quatre bases sont rouvertes et auditées pour
chaque profil. Les pertes d'accusés gossip injectées sont corrélées aux
reprises. Les empreintes de sources et binaires ont été vérifiées avant
l'enregistrement des résultats.

Cette régression couvre les derniers changements de frais de remplacement,
de réservation et de nettoyage après bloc avec des transferts séquentiels
finançables. Le scénario de suffixe non finançable et de trou de nonce
n'est pas encore exécuté ici en BFT distribué. Le débit local borné ne
constitue pas une qualification de saturation publique ni une validation
production globale.

## BFT : suffixe non finançable et récupération du pool

`bftPoolBalanceRecovery` ajoute les cibles CTest
`bft_pool_balance_paris_network` et `bft_pool_balance_shanghai_network`.
Les deux passent (13,33 s au total). Pour chaque type 0/1/2, un compte
distinct des validateurs soumet les nonces 1 (60 %), 2 (5 %) puis 0 (60 %).
Cet état initial est installé par RPC dans chaque cœur avant la connexion
des relais, afin de contrôler l'ordre initial sans course avec le consensus.

Après finalisation du nonce 0, les quatre nœuds ont le même solde reçu
(60 %), aucun reçu pour le nonce 1 non finançable et le nonce 2 en queued,
identifié par son hash exact. Le remplacement du nonce 1 à 5 %, aux mêmes
frais, est ensuite soumis uniquement au nœud 0. Les vrais relais le
propagent et le BFT finalise les nonces 1 et 2. Les quatre nœuds ont
les mêmes reçus, nonce canonique 3, pools vides et valeur reçue de 70 %.

Les certificats pondérés, les hashes de blocs et les racines finales
concordent. Les huit réouvertures de bases (quatre par profil) passent
l'audit. Au total : 18 transferts exécutés et six dépenses non finançables
sans reçu. L'erreur initiale de compilation du test est conservée, puis
corrigée par conversion explicite du JSON en chaîne ; aucun changement
de code de production.

La constitution du pool initial par gossip concurrent et une panne pendant
ce scénario précis restent hors périmètre. Le test ne remplace pas la
comparaison après bloc avec Geth ni la qualification production globale.

## Pool après bloc : comparaison Geth avec production de blocs

`txpoolPostBlockGethComparison` exécute les mêmes enveloppes signées que
la campagne Anvil sur Nano et Geth 1.16.7, types 0/1/2 sous Paris/Shanghai.
Résultat : **12/12 snapshots concordants**, sans retransmettre le nonce 2.
Le banc pilote Geth via Engine API V1 (Paris) ou V2 (Shanghai), exige des
statuts VALID et conserve les douze payloads. Il vérifie l'identité de
l'ordre des hashes inclus et des base fees calculées par les deux clients.
Geth fonctionne sans découverte ni pairs externes ; Nano produit des
blocs bootstrap signés via son RPC dev limité à loopback.

Après le premier bloc, les deux clients ont exécuté seulement le nonce 0,
retiré le nonce 1 non finançable et conservé le nonce 2 queued. Le
remplacement du nonce 1 à valeur réduite, aux mêmes frais, est admis et
les deux transferts finançables restants sont exécutés. Les soldes, nonces,
reçus normalisés (hash/statut/gas/prix effectif) et partitions du pool
concordent. La valeur totale reçue atteint 70 % du solde initial ; le
transfert non finançable n'a jamais de reçu. Les enveloppes sont vérifiées
identiques à celles de l'échec Anvil conservé.

Ce résultat étaye le diagnostic de différence de rétention du pool avec
Anvil dans ce scénario. Il ne transforme pas l'échec Anvil en succès.
Les hashes de blocs et racines globales ne sont pas comparés entre les
architectures distinctes ; la preuve reste limitée aux transferts et aux
champs explicitement contrôlés, sans affirmation production globale.

## Régression native élargie : 52 suites et correction du test gossip

`currentNativeRegression` compile toute la configuration Release, puis
exécute 52 suites CTest en deux processus parallèles, sans flood. Le
premier passage donne **51 réussites et un échec** (`gossip_recovery`),
en 81,13 s ; le rapport de code 8 est conservé. Les campagnes nécessitant
les vrais relais et le parcours d'installation sont exclus explicitement
dans le fichier de sélection. Les tests d'état/blocs CTest sont des
sous-ensembles de régression, pas une nouvelle exécution du corpus complet.

L'assertion échouée comptait seulement la partition pending pour exiger
10 000 entrées. Après la classification correcte des trous de nonce,
l'éviction d'un nonce transforme sa suite en queued. Le test corrigé
vérifie les deux partitions, l'unicité des hashes, les compteurs et la
couverture intégrale par inventaire + événements. Il passe en 14,05 s :
9 901 pending + 99 queued = 10 000 entrées couvertes, 200 pages,
10 002 événements et insertion tardive bien récupérée. Le changement
concerne uniquement le test ; les empreintes des 51 autres exécutables
sont inchangées et leur réussite initiale reste valable pour ce bilan.

Ce bilan regroupe donc 51 suites inchangées réussies et la suite corrigée
réexécutée avec succès ; il n'est pas présenté comme un nouveau passage
global 52/52. Il ne clôt ni le catalogue OpenZeppelin exhaustif ni la
qualification réseau publique ou la préparation production globale.

## Hardhat : parcours développeur, WNANO et routeur sur les deux profils

`currentSolidityDeveloperPaths` relance `publish.ts`, `wrapped-nano.ts`
et `pool-router.ts` sur des nœuds Nano temporaires Paris et Shanghai.
Le passage initial donne 4/6 succès : ERC20 passe sur les deux profils,
WNANO et le routeur passent sur Shanghai et échouent sur Paris au déploiement
de WNANO (`Opcode unavailable in selected EVM revision`). L'artefact
`contracts/WrappedNano.json` cible explicitement Shanghai ; ces échecs
confirment une incompatibilité de cible du banc, pas une raison d'activer
les opcodes Shanghai sur Paris. Les deux rapports restent conservés.

Le helper `scripts/helpers/wrapped-artifact.ts` compile le même source
WNANO avec solc épinglé pour Paris quand ce profil est demandé. Pour
Shanghai il retourne l'artefact existant, inchangé. Les deux parcours
Paris corrigés et les deux régressions Shanghai passent. Le script ERC20,
non modifié, avait déjà réussi sous les deux profils. Le libellé du réseau
dans le résultat du routeur reflète désormais le profil testé.

Les assertions contrôlent approbations, réserves, sorties natives nettes
du gas, collatéral WNANO, rollback de l'état, tentative de réentrance et
restitution complète. Les appels négatifs utilisent encore le rejet ethers
et les assertions d'état ; les reçus status=0 ne sont pas systématiquement
vérifiés explicitement. Chaque nœud utilise un seul validateur bootstrap
BFT : ces parcours ne remplacent pas une qualification distribuée des
contrats. TESTQ est synthétique, sans USDC réel, bridge ou cotation économique.

## Solidity : rejets exécutés et reçus explicitement vérifiés

`solidityFailedReceiptQualification` renforce les parcours WNANO et
Router02 avec le helper `failed-receipt.ts`. Les quatre exécutions
Paris/Shanghai réussissent. Dix transactions échouées sont enregistrées :
transfert WNANO non autorisé, retrait supérieur au solde, remboursement
refusé, slippage et échéance expirée, chacun sur les deux profils.

La soumission doit retourner une transaction ; un refus RPC ne suffit
plus. Le helper exige l'erreur ethers liée au reçu exact, puis relit le
reçu RPC status=0 sans logs, vérifie l'inclusion au bon index dans le bloc,
le nonce consommé et un gasUsed positif inférieur à la limite de transaction.
Les vérifications d'état existantes sont conservées, avec assertions
supplémentaires sur les soldes et l'offre WNANO après les deux nouveaux
échecs forcés. Les chemins valides et restitutions finales passent aussi.

Cette preuve ferme la limite des simples `assert.rejects` pour ces cinq
cas précis, sans qualifier tous les rejets Solidity. Un gasUsed inférieur
à la limite ne prouve pas à lui seul la cause d'un éventuel échec imbriqué.
Les nœuds temporaires ont un validateur bootstrap, pas un réseau distribué
pour cette campagne. Aucun changement du code de production Nano.

## Hardhat : déploiement et parcours NFT OpenZeppelin

`hardhatNftDeveloperQualification` ajoute les fixtures Solidity
`DeveloperNFTs.sol` et le parcours `scripts/nfts.ts`. Hardhat compile
les sources avec OpenZeppelin 5.0.2 et solc 0.8.28 ciblant Paris, puis
les déploie par transactions sur Nano Paris et Shanghai. Les deux parcours
passent ; les trois artefacts compilés sont archivés avec empreintes.

ERC721 : propriété initiale, transfert non autorisé, approbation/révocation,
rejet du receveur avec restauration de sa propre écriture et de
l'approbation, transfert autorisé avec événement exact et approbation
effacée, puis réception valide par contrat. ERC1155 : opérateur non
autorisé/révoqué, refus du receveur, lot dont le second élément dépasse
le solde après un premier élément valide, puis lot accepté avec soldes
conservés et événement TransferBatch exact.

Les 14 échecs attendus ont des reçus status=0 vérifiés : inclusion au bon
index dans le bloc, nonce consommé, absence de logs et gas positif sous
la limite. Chaque profil utilise un seul validateur bootstrap avec
certificats BFT ; cette preuve n'est pas une qualification NFT distribuée
ou celle de tout le catalogue OpenZeppelin. Ces contrats de test ne sont
pas destinés au déploiement public et ne mettent aucun fonds réel en jeu.

## Hardhat : signatures ERC20Permit et séparation des domaines

`hardhatPermitDeveloperQualification` compile et déploie deux tokens
OpenZeppelin ERC20Permit sur chaque nœud temporaire Paris/Shanghai.
Les deux parcours passent. Le séparateur EIP712 lu sur chaîne égale
celui calculé par ethers ; deux adresses de contrat ont des domaines
différents. Les signatures pour une autre chaîne, un autre contrat,
un montant altéré ou une échéance expirée sont refusées, sans mutation
du nonce de permis, de l'autorisation ni des soldes du token.

Le permis valide, relayé par le dépensier, émet l'Approval attendu et
crée une autorisation de 40 unités. Son rejeu échoue. Une dépense de 15
laisse 25 d'autorisation, puis un nouveau permis signé à zéro révoque
le reste ; la dépense suivante échoue. Les 12 reçus status=0 sont
vérifiés explicitement. Le nonce de transaction du relayeur est consommé
même quand le nonce interne du permis reste inchangé après rollback.

Le domaine de chaîne étrangère est signé localement : aucun fork de
réseau n'est créé. Cette preuve n'empêche pas un utilisateur de signer
volontairement un permis valide malveillant sur le bon domaine ; elle
qualifie les contrôles contractuels testés, pas une défense contre le
phishing ni le catalogue OpenZeppelin entier. Les contrats et fonds
restent fictifs et locaux.

## Hardhat : proxy UUPS, stockage et migration atomique

`hardhatUupsDeveloperQualification` compile des fixtures utilisant
Initializable, UUPSUpgradeable et ERC1967Proxy d'OpenZeppelin 5.0.2.
Le premier passage Paris/Shanghai échoue dans Hardhat, qui ne trouve pas
l'artefact importé `ERC1967Proxy`. Une fixture `DeveloperProxy` hérite
du proxy et transmet uniquement les paramètres du constructeur. Les
deux journaux initiaux sont conservés ; les deux parcours corrigés passent.

La lecture RPC du slot EIP1967 confirme l'implémentation active. Une
migration V2 écrivant puis échouant restaure ce slot, les anciennes
données et le nouveau champ, sans logs persistants. Une migration
valide conserve propriétaire/valeur, initialise le champ V2 et émet
l'événement Upgraded exact. Les écritures du proxy n'affectent pas le
stockage des contrats d'implémentation.

Les 16 rejets ont des reçus status=0 vérifiés : initialisation directe
interdite, upgrade non autorisé, UUID incompatible, migration échouée,
contextes proxy/direct invalides, réinitialisation et écriture non autorisée.
Les layouts V1/V2 ont été rendus compatibles pour la fixture : le test
ne prouve pas la sécurité des layouts d'upgrades arbitraires. Il concerne
les smart contracts locaux, pas une mise à niveau du protocole Nano,
et n'utilise ni contrats publics ni fonds réels.

## Hardhat : faille de marketplace et continuité après épuisement du gas

`hardhatMarketIsolationQualification` porte le contrat volontairement
vulnérable de `tests/contracts/TestMarket.sol` dans le banc Hardhat local.
Paris et Shanghai passent. L'absence de vérification du vendeur permet
au compte de test attaquant de voler effectivement le NFT séquestré :
reçu status=1, événement Transfer exact et nouvelle propriété vérifiée.
La liste est consommée et une seconde tentative échoue sans duplication.

Une transaction bornée à 150 000 gas appelle ensuite la boucle infinie
de la fixture. Le reçu status=0 consomme exactement cette limite, ne
conserve aucun log ni écriture du marché, et facture exactement
`gasUsed × effectiveGasPrice` au compte attaquant. Son nonce avance
d'une unité. Les soldes/offre ERC20 et soldes ERC1155 contrôlés restent
inchangés. Une transaction valide dans un bloc ultérieur restitue le NFT
au vendeur, avec présence de certificats BFT sur les blocs temporaires.

Ce test démontre ces comportements applicatifs et la continuité du
nœud dans ce scénario précis. Il n'est ni un test de saturation ni une
campagne multi-validateur de marketplace. Le contrat vulnérable est
exclusivement une fixture locale ; aucun marché public ou fonds réel
n'est utilisé et le code de production Nano n'est pas modifié.

## Pool V2 : flash swaps, remboursement et verrou de réentrance

`hardhatV2FlashSwapQualification` passe sous Paris et Shanghai avec
les bytecodes V2 officiels épinglés. Un callback forgé, un remboursement
absent et un remboursement inférieur d'une unité au minimum produisent
six reçus status=0 au total. Les soldes, réserves et écritures du callback
sont restaurés. Un remboursement exact de 10 030 091 unités pour
10 000 000 empruntées produit 30 091 unités de frais et l'événement
Swap exact. L'appel imbriqué sync échoue avec `UniswapV2: LOCKED`,
alors que le flash swap payé réussit ; un swap ultérieur réussit aussi.

La preuve est limitée à V2 et au remboursement dans le même actif
synthétique. Elle ne valide pas V3 ou V4. La demande V3 ouvre un
périmètre distinct : factory/pool, ticks et liquidité concentrée, routeur,
positions NFT, perception des frais et retraits, puis scénarios adversariaux.
Aucun composant V3 n'est déclaré qualifié par les tests V2.

## Uniswap V3 : premier cycle de position concentrée

`hardhatUniswapV3Lifecycle` épingle les paquets officiels v3-core 1.0.1
et v3-periphery 1.4.4 (intégrités npm conservées). Le parcours Hardhat
`pool-v3.ts` passe sous Paris et Shanghai. Factory, pool, SwapRouter,
NonfungiblePositionManager, NFTDescriptor et le descripteur lié sont les
artefacts upstream ; WNANO et TESTQ sont les actifs de test.

Le prix initial normalise 18/6 décimales pour 10 TESTQ/NANO, avec racine
carrée entière Q96. La position couvre 1 200 ticks autour du prix initial
(frais 3 000, spacing 60), et ses liquidités aux bornes sont vérifiées.
Le mint débite les montants exacts des événements ; le NFT et son URI
JSON/SVG sont lisibles (pas de contrôle visuel du SVG). Un swap de
1 TESTQ reçoit du WNANO et déplace le prix dans le sens attendu.

Le NFT est transféré à Bob, qui retire toute sa liquidité, collecte les
montants exacts des événements puis le brûle. Les frais TESTQ collectés
sont positifs et bornés par les frais de l'entrée (2 999 unités observées).
Six reçus status=0 contrôlent retrait non autorisé, burn prématuré et
slippage. Leurs états contrôlés sont conservés.

Il s'agit d'un premier parcours V3, pas d'une qualification exhaustive :
franchissements de ticks initialisés, positions superposées, oracle,
callbacks hostiles, flash et référence indépendante des arrondis de frais
restent ouverts. Nœuds bootstrap locaux uniquement, sans USDC réel.

### Uniswap V3 dual-asset flash qualification

`hardhatV3FlashQualification`: Paris and Shanghai both pass. Four negative transactions per profile have persisted status-0 receipts and restore both asset balances, fee growth, liquidity, slot0 and callback writes. Valid dual-asset repayment records exact Flash fees; nested flash returns `LOK` without failing the correctly repaid parent. A following one-unit loan on each asset charges one unit per asset, with independent Q128 fee-growth arithmetic.

This adds eight failed-receipt checks and four successful loans. It covers one local pool and one position, standard tokens and zero protocol fee; it does not close tick crossing, multiple positions, oracle, distributed workload or production readiness. Sources, binary and borrower artifact are fingerprinted; raw logs retained under `build-cmake/qualification/hardhat-v3-flash-*`.

### V3 overlapping ranges and open gas-estimation issue

`hardhatV3CrossingQualification`: initial Paris/Shanghai runs fail on an automatically estimated router transaction. Diagnostic Paris records estimate 230526, reverted receipt consuming 227434, then retry estimate 234976. Reverted nested calls need not consume the entire outer transaction limit. With explicit gas for swaps, all three crossings pass but a manager transaction still fails. The final bounded lifecycle (2000000 gas for swaps, decrease and collect) passes both profiles. All five failing logs and both successful logs are retained; baseline, diagnostic and bounded scripts are archived.

The successful runs cross the narrow position boundaries in both directions, verify exact price limits, active liquidity and token deltas, return to the starting price/tick, collect fees from both positions, burn both NFTs and clear the four initialized ticks. Impossible slippage restores the inspected state with a mined status-0 receipt. A latent test cleanup issue was also removed: both NFTs are burned after both collections, so snapshots do not query an already burned position.

**Open:** the unbuffered RPC estimate workflow is not qualified by these bounded runs. The final third swap consumes 140787 gas versus estimate 139822. `src/rpc.cpp` simulates the selected latest header; the official V3 `Oracle.write` skips its storage write at an unchanged timestamp. Timestamp context is a hypothesis to isolate, not a proven root cause. Geth v1.16.7 also defaults estimates to latest (`internal/ethapi/api.go`, EstimateGas); changing Nano to pending without a differential test would not establish compatibility. Next action: reproduce against a fixed parent and next-block timestamp with the same signed transaction on Anvil/Geth, then choose the evidenced fix or documented estimation policy.

Independent exact swap/fee arithmetic, oracle history and distributed V3 remain open. No production source change or production-readiness approval follows from this campaign.

### RPC pending estimation: timestamp context isolated and V3 path recovered

`rpcPendingEstimationQualification` follows the V3 failure above. A minimal timestamp-dependent storage probe demonstrates latest gas 23122 versus next-block gas 31039. The same signed insufficient-gas transaction fails on Nano and Anvil, with identical receipt status/gas and unchanged storage; a correctly funded gas limit succeeds on both at the same inclusion timestamp. This supports a context issue rather than an EVM gas-accounting divergence. Anvil's default/latest/pending estimates themselves differ; the diagnostic preserves those values and does not claim full RPC equivalence.

`Blockchain::pending_execution` constructs a read-only candidate under the chain lock, using the next timestamp, base fee, number and selected producer, and executes eligible pool transactions on a copied state. `eth_call` and `eth_estimateGas` now accept explicit `pending`. Native tests prove the candidate contains pool effects without consuming the pool or changing canonical state/events. Default and explicit `latest` retain their existing semantics. Other pending RPC endpoints are not enabled by this change.

Paris and Shanghai V3 crossing/withdrawal now pass using actual per-transaction pending estimates, not the previous two-million gas cap. The previously failing second swap uses 234976 versus latest230526; the third uses144273 versus latest139822. Both native RPC/chain suites and four post-change same-raw Nano/Anvil probe transactions pass. All initial failures remain archived.

Pending is a candidate, not a promise of future inclusion context: new transactions, time or proposer/round changes can still alter execution. Full-mempool pending RPC load remains to qualify. No blanket production approval.

### Pending estimation with a full pool: timeout reproduced and corrected

`rpcPendingFullPoolQualification` preloads10000 legacy transfers across100 senders and runs4 local HTTP clients against Paris/Shanghai. Before optimization both profiles hit the client's10-second request deadline (combined60.99seconds). Initial target build also required CMake regeneration; its failure log is retained.

Block preparation previously recovered sender addresses repeatedly while scanning ineligible nonces and continued scanning when the block could accept no further transaction. Pool snapshots now carry the sender authenticated at admission; actual execution still validates signatures. Preparation stops below21000 remaining block gas. Fee/arrival ordering and nonce checks remain.

The **unchanged** workload now passes both profiles. Eight full-pool simulations plus four identity reads take about0.63seconds per profile on this host, leaving state/tip/events/pool untouched. A second bounded phase runs alongside the HTTP timer's single-bootstrap BFT reactor:500 transfers committed on Paris and400 on Shanghai, while exactly9500 and9600 respectively remain in the pool. The admitted set is the exact disjoint union of committed and pending hashes. Certificates, history audits and reopened state pass. Maximum observed estimate/call/identity triplets were about2.42s and2.21s during block production; these are host observations, not production SLOs.

Chain185 and RPC873 checks pass, followed by core, txpool history/bytes and both four-validator pool-balance regressions (five suites). The network regression is separate from the single-validator estimation workload. Remaining scope: expensive contracts, large-byte pools, sustained clients and distributed estimation load; HTTP execution still serializes expensive work.

### Expensive RPC execution and measured-gas verification

`rpcExpensiveSimulationQualification` runs four bounded local clients on Paris and Shanghai. A storage write followed by gas exhaustion at the 30-million limit returns the expected RPC error, leaves canonical storage unchanged and permits single-bootstrap BFT progression. That case passed before optimization.

A successful 600,000-iteration computation exposed a separate problem: binary gas search repeatedly executed the expensive code. Both profiles hit transport timeouts; completed request groups took about 14.6 seconds. The unchanged test now passes at approximately 2.24–2.26 seconds for the observed groups. These are host observations, not latency guarantees.

The estimator now checks the gross gas measured during its initial successful run as a candidate. It returns that number only after a fresh simulation with that budget succeeds. A failed candidate keeps the binary search. Two native EIP-150 fixtures prove that consumed gas can be insufficient for a nested call and that the fallback still finds a working boundary. All 887 RPC checks pass.

All four costly-call suites verify storage isolation, the signed transfer and certificate, observed BFT height, history audit and reopened state. V3 crossing and withdrawals on both profiles retain their calculated pending gas limits. The same-raw Nano/Anvil timestamp comparison also passes. Initial failures, test versions, binaries and source hashes are retained.

Remaining scope includes costly nested computations that still use binary search, sustained clients, expensive precompiles, distributed load and HTTP scheduling. This optimization does not alter consensus gas rules or establish global production readiness.

### Nested expensive gas estimation: verified approximation

`rpcNestedEstimationQualification` extends the same successful computation to one and four nested CALLs. All four Paris/Shanghai baseline cases timed out under four bounded local clients; surviving request groups took approximately 15 seconds. Those failures and their source snapshots remain retained.

The estimator now uses a failed measured-gas trial as a known lower bound, tries an overflow-safe headroom candidate within the existing allowance, and stops binary search when the bracket is no wider than 1.5% of the verified successful upper bound. No unexecuted gas quote is returned. This changes quotation precision, not EVM gas charges. It replaces the previous exact-minimum fallback expectation. Small native EIP-150 fixtures independently locate the call boundary, check the approximation bound, and verify tight caller caps. A GMP/uint64 comparison compile failure was corrected with explicit integer conversions; its log is retained.

The unchanged nested load fixture passes after this change: groups are approximately 3.3 seconds at depth one and 4.6–4.9 seconds at depth four on this host. Nine suites pass, including 889 native RPC checks. The eight expensive-call variants verify canonical storage isolation, single-bootstrap BFT progress, the signed transfer and certificate, history audit and reopen.

A separate Nano/Anvil campaign passes eight identical signed transactions at both depths and profiles, using quotes from each provider. Both providers' quotes are first executed on both engines. Final gas consumption, logs, sender balance/nonce and storage match; quotation numbers need not match. The V3 crossing/collection/withdrawal regression also passes both profiles using pending estimates.

The Geth v1.16.7 gas estimator provides a reference for verified headroom candidates and bounded search precision: https://raw.githubusercontent.com/ethereum/go-ethereum/v1.16.7/eth/gasestimator/gasestimator.go . Nano's implementation is separately qualified by the evidence above. Remaining scope includes sustained/distributed RPC load, deeper or differently structured contracts and precompile extremes. There is still no universal RPC cancellation/deadline guarantee or global production approval.

### Distributed BFT during expensive RPC estimation

`bftRpcCostQualification`: both Paris and Shanghai pass (two CTests, 40.37 seconds total). Four local validators communicate through cpp-libp2p. Four clients on validator zero run three rounds each of latest/pending calls and estimates around a computation behind four nested CALLs. Each profile executes six estimates, twelve expensive calls including quote verification, and twelve identity reads without client errors.

A signed transfer is certified while all four RPC workers are still active. In both observed runs its certificate contains the other three validators, excluding the loaded validator. Shanghai advances to round one for this block. A following transfer submitted through the loaded node succeeds; all peers converge on receipts, balances and state roots. Simulated storage writes remain absent. Four databases per profile reopen and pass their history audit. Shanghai's post-load certificate also includes the formerly loaded validator.

Maximum observed client round durations are approximately 7.06 seconds (Paris) and 5.21 seconds (Shanghai); estimate rounds include executing the quote and an identity read. These are host measurements, not service guarantees. An initial test compilation error passing a GMP height to a uint64 certificate API was corrected with explicit conversion; the failed build log remains archived alongside successful results and source/binary fingerprints.

This closes the bounded distributed scenario only. Sustained traffic, overload of several validators, public-network behavior and universal RPC cancellation remain unqualified. Database reopen follows graceful shutdown here, not a crash. No production-readiness approval follows from this test.

### Longer distributed RPC workload

`bftRpcCostSustainedQualification` extends the existing four-validator fixture to 24 rounds per client, with a healthy-peer height sample approximately every second while clients remain active. Both Paris and Shanghai pass: two CTests take 257.07 seconds combined. Each profile completes 48 estimates, 96 expensive calls (including quote execution) and 96 identity reads. All four clients finish without errors.

Paris records 124 progress samples from height 3 through 50; Shanghai records 108 from height 3 through 52. Observed fixture durations are 131.65 and 121.46 seconds respectively; maximum client round durations are 7.34 and 6.89 seconds. These include multiple requests for estimation rounds and are host observations, not RPC service guarantees. The workload still verifies the during-load certificate, successful post-load transfer through the loaded node, all-peer receipt/root convergence, unchanged probe storage and eight total database reopen audits.

The new test requires at least twenty active-load observations and five blocks of progression, so initial finality alone cannot pass the longer variant. Sources, binaries, per-round timings and progress samples are retained with hashes. No production code was changed in this campaign.

This is approximately two minutes per profile on one loaded validator, not a long endurance test, simultaneous overload of multiple validators or a public network. Height observations are from a healthy peer; all-peer convergence is checked after load. Reopen remains graceful, not crash recovery. Global production readiness remains unproven.

### Expensive RPC work on all four validators

`bftRpcCostAllValidatorsQualification` places one concurrent client on each of the four local cpp-libp2p validators. Each client rotates through latest/pending calls and estimates; each returned estimate is executed with the quoted limit. The first 24-round runs complete without client errors but both fail the inherited minimum of twenty progress observations: distributing the work across processes makes this workload too short. Their logs, source snapshots and binary fingerprints are retained. The workload is extended to 96 rounds per client without lowering the progress assertions.

The longer Paris/Shanghai runs both pass, taking 278.77 seconds combined in CTest. Per profile: 192 estimates, 384 costly calls including estimate verification, and 384 identity reads. All clients finish without errors. Paris records progress from height 3 through 25 and Shanghai from 3 through 24 while workers remain active. The fixture takes approximately 136.19 and 138.61 seconds; maximum observed client round durations are 2.33 and 2.36 seconds. Each node receives all four method/context combinations. These are host observations, not latency guarantees.

During-load marker finality, post-load submission through node zero, all-peer matching receipts and roots, cryptographic certificate verification, unchanged probe storage, and four reopened database audits per profile pass. Production code is unchanged. Initial failures and complete per-round/progress results remain visible in the manifest and hashed evidence.

This covers one client per validator and one fixed expensive contract on a single host, not saturated queues, many clients per validator, precompile extremes or a public network. Progress is sampled through loaded node one; all-peer convergence is verified after load. Reopen is graceful, not crash recovery. Global production readiness remains unproven.

### Open: consensus availability under sixteen costly RPC clients

`bftRpcCostManyClientsQualification` places four clients on each of the four local validators, with 24 rounds per client. The total work matches the preceding four-client/96-round campaign; concurrency increases from four to sixteen. Paris and Shanghai both fail because the control RPC exceeds its unchanged five-second deadline. The initial test unwound before reporting worker results; the diagnostic version now joins and reports all clients even after control failure. Both original failures remain archived.

The Paris diagnostic also fails. All sixteen clients complete all 24 rounds without errors under their ten-second request deadlines (192 estimates, 384 costly calls including quote execution, 384 identity reads). Maximum observed client round duration is 9.65 seconds. Immediately after the workload, all four nodes still report height 1, the same block hash/root, null marker receipts and zero probe storage. Thus simulation isolation remains intact in the inspected state, but this workload does not maintain consensus availability. The snapshot does not establish permanent deadlock or eventual recovery, and the failed run does not reach the reopened-database checks.

Source inspection identifies shared scheduling constraints: `HttpServer::Session::respond` executes `rpc_.parse` synchronously on the same event loop as the validator timer; `Rpc::handle` also holds the mutex used by `validator_tick` throughout simulation. Merely adding HTTP threads cannot remove the shared-mutex contention. These constraints are a concrete candidate cause, not an experimentally isolated proof of the sole cause.

**Unresolved production blocker:** isolate costly simulations from consensus scheduling with coherent state snapshots and bounded work admission, then replay the same deadlines, finality and state assertions. Do not lower deadlines, bypass the failed tests or infer production readiness from the lower-concurrency passes. No production code was changed in this diagnostic campaign. Sources, binary fingerprints, failures and post-load observations are retained.

### HTTP simulation isolation corrects the sixteen-client failure

`rpcSimulationIsolationQualification` follows the open failure above without changing the BFT workload, request deadlines or finality assertions. RPC calls/estimates now capture their state, header, chain rules and BLOCKHASH ancestor window while synchronized, then release the RPC mutex for EVM execution. HTTP requests containing simulations run on a dedicated single-worker queue; at most eight envelopes are running or queued per node and surplus receives HTTP 503. Socket operations and admission counters remain on the event loop, which continues serving control requests and validator ticks. Batch items remain sequential within their envelope, but other requests may run between them.

Both unchanged sixteen-client scenarios pass. Paris progresses from observed height 3 through 130 under load; Shanghai from 4 through 133. All clients finish without errors, certificates and matching receipts/roots pass, probe storage remains zero, and all eight reopened databases pass audit. Fixture durations are approximately 137.58 and 140.00 seconds. The combined initial RPC/HTTP/network regression takes 286.43 seconds and includes 889 native RPC checks. The original failures and diagnostic state observations remain preserved.

An additional HTTP queue test submits sixteen envelopes of four expensive calls each: eight are accepted and eight return 503. An identity read completes in approximately 0.16 milliseconds on this host while work remains queued. Canonical storage remains unchanged; a mixed batch preserves notification suppression and parameter-error responses. Eight same-signed nested-computation transactions still match Anvil on status, gas, logs, balance, nonce and storage, with each provider's quotes verified on both engines.

Remaining scope is explicit: WebSocket simulations still execute on their event loop and do not use this HTTP queue; pending candidate preparation and ancestor capture remain synchronized; the queue counts envelopes rather than cumulative batch gas; cancellation/deadlines, large-state snapshot costs and long/multi-host endurance remain unqualified. Wallet-triggered internal estimates retain their enclosing request lock. This correction closes the demonstrated HTTP availability failure only, not global production readiness.

### WebSocket shares HTTP simulation isolation and capacity

`rpcWebSocketIsolationQualification` closes the transport gap identified above for the bounded workload. A shared `SimulationQueue` now owns the single worker and eight-envelope capacity for both HTTP and WebSocket. WebSocket processes mixed batches sequentially: simulations run on the worker, while subscriptions, timers and socket operations remain on the event loop. Full capacity returns JSON-RPC -32005 on WebSocket and HTTP 503; notifications produce no response.

A mixed burst of eight HTTP and eight WebSocket clients accepts eight envelopes and rejects eight. Each envelope contains four expensive calls. The identity read takes approximately 0.13 milliseconds on this host; successful simulations preserve canonical storage. Mixed notification/error batches agree between transports, and a WebSocket batch containing subscribe, simulation and unsubscribe passes.

The four-validator workload runs with sixteen WebSocket clients,24 rounds per client and an asynchronous ten-second deadline for each request (no retries). Paris and Shanghai both pass with all clients error-free, during-load marker finality, post-load transaction acceptance, certificate verification, matching peer receipts/roots, unchanged probe storage and eight reopened database audits. Observed heights progress from3 to133 on Paris and4 to133 on Shanghai; fixture durations are139.70 and141.62 seconds. Maximum multi-request round durations are10.02 and10.15 seconds; these include multiple individually bounded requests and do not imply a single-request deadline failure. All three CTests pass in295.29 seconds.

The limit still counts envelopes rather than cumulative batch gas. Cancellation deadlines, expensive pending preparation, subscription notifications concurrent with block production, disconnect races, long endurance and multiple hosts remain to qualify. No EVM gas-rule changes were made. This is scoped transport/consensus evidence, not global production approval.

### Batch-aware shared simulation capacity

`rpcSimulationBatchBudgetQualification` replaces the eight-envelope capacity with eight reserved/running simulation operations across HTTP and WebSocket. Each top-level call/estimate inside a batch counts, including notifications. A batch reserves its entire simulation count before any item executes; its reservation remains held until completion. Counting saturates above the limit, avoiding overflow and unnecessary scanning once rejection is certain. Nested arrays remain invalid JSON-RPC items and cannot execute simulations.

The mixed sixteen-envelope burst, with four expensive simulations per envelope, now accepts two and rejects fourteen; the identity read remains available (approximately0.13ms on this host). Additional checks reject nine-simulation batches over both transports, including notification variants. A filter-creation request placed before those simulations does not execute on rejection. Exact-capacity batches of eight calls succeed after previous completions/rejections, proving reuse of capacity. Mixed subscription and notification/error behavior continues to pass.

HTTP integration and both short Paris/Shanghai four-validator RPC-cost regressions pass, three CTests in43.15seconds. The network cases retain their finality, peer convergence, simulation isolation and reopened-history checks. Sources, binary fingerprints and logs are retained. The prior sixteen-client campaign was not rerun for this accounting-only change.

This bounds the number of simulations even inside batches; it is not a cumulative gas or wall-clock budget. Gas estimation may execute the same bytecode multiple times. Execution cancellation, long-running precompiles, expensive pending preparation and large-state snapshot costs remain open. No global production approval.

### Cooperative RPC execution cancellation

`rpcSimulationCancellationQualification` adds an optional host checkpoint to `ExecutionContext`. RPC simulation creates one five-second steady-clock deadline after preparation/dequeue and shares it across all estimation trials. Nested contexts inherit the checkpoint. Code analysis and the interpreter check periodically (every1024 instructions), with checks at frame entry/commit and before/after precompiles. Consensus contexts leave the callback empty; gas charging rules are unchanged.

Cancellation has a distinct exception path: frame snapshots are restored and the exception escapes nested callers. It does not become an EVM failure that an outer contract can ignore. RPC returns -32002. Deterministic clocks test direct/nested cancellation on Paris and Shanghai, rollback on the actual EVM state, latest/pending calls and estimates, cumulative estimator deadlines, ordinary execution with no callback, and a fresh budget on the next request. Native suites pass3483 core,185 chain and931 RPC checks plus the deep-frame fixture. Twelve forced HTTP/WebSocket cancellations return the expected error and allow subsequent calls, with no persisted simulation writes or leaked queue capacity.

Both unchanged sixteen-client HTTP/BFT scenarios pass with the real default deadline: clients finish error-free, finality progresses, receipts/roots and certificates agree, and eight databases reopen successfully. The transport/network campaign takes284.91seconds. The rebuilt official state runner matches5387 cases from317 retained fixture files (including505 expected validation rejections), with zero mismatches/unsupported/errors. Eight same-signed nested computations still match Anvil; both providers' quotes execute on both engines.

This is cooperative cancellation, not a hard request deadline. A native precompile or expensive single opcode already executing can only be checked when it returns; queue wait, state preparation and pending execution are outside this timer. Wallet-triggered internal estimates retain their enclosing RPC lock. Whole-request deadlines, shutdown/disconnection cancellation and costly native-operation interruption remain open. The317-file replay is the local regression corpus, not a fresh full historical-corpus campaign. All evidence and source/binary fingerprints are retained; no global production approval.

### Cooperative shutdown with active and queued simulations

`rpcSimulationShutdownQualification` adds an atomic stopping flag shared by both transports. Explicit stop, the signal handler and server destruction set it before worker join. RPC worker requests check cancellation before starting each batch item, and active simulations inherit the check through their execution checkpoints. Capacity reservation rejects work after stopping.

A deterministic test queues a batch of eight simulations plus a later filter-creation mutation. Its clock hook pauses the first simulation at the twentieth checkpoint; the test requests server stop, releases the hook, joins the event loop and destroys the server. Both HTTP and WebSocket record exactly twenty clock polls: no further simulated computation or queued simulation starts. The later mutation is skipped, the canonical state remains unchanged and the client connection closes. Observed shutdown durations are0.98ms and0.47ms on this host, below the local two-second test bound. These measurements do not establish a universal latency guarantee.

All five selected suites pass in46.62seconds:931 native RPC checks, HTTP integration, the new shutdown test and short Paris/Shanghai four-validator RPC-load regressions with their finality, state convergence and database reopen assertions. Logs and source/binary fingerprints are retained.

The test exercises the shutdown API; it does not inject process signals or power loss. A native precompile already running, expensive snapshot preparation or an already-started non-simulation mutation is not forcibly interrupted. Cancellation on client disconnection remains to qualify separately. No global production approval.

### WebSocket disconnect detection during simulations

`rpcWebSocketDisconnectQualification` first reproduces the failure before changing production code. The client sends eight simulations and a later filter mutation, then requests identity and close while the first calculation is paused at a known checkpoint. Previously the server stopped reading until the batch finished: close could not complete during the pause,58576 clock polls were observed, and the later mutation executed. This failed baseline and its source/binary fingerprint remain retained.

WebSocket now keeps its read active while simulation work runs. Detection of close/error sets an atomic disconnected flag read by the worker checkpoint. The event-loop continuation drops the rest of the abandoned batch and releases its reservation. Distinct requests may complete out of order with their JSON-RPC IDs; batch items remain sequential. Socket writes and subscription state stay on the event loop.

The unchanged diagnostic passes for normal close and TCP RST. Identity and close complete while calculation remains paused; the late mutation is skipped, canonical state remains unchanged, and a fresh eight-call HTTP batch reuses the entire capacity. Both corrected cases record52 clock polls including the successful follow-up calls. HTTP integration and shutdown regressions also pass.

Both sixteen-client WebSocket Paris/Shanghai campaigns pass with no client errors, during-load finality, matching receipts/roots, verified certificates and eight successful database reopen audits. Results, baseline failure, sources and binary fingerprints are retained.

This campaign does not alter HTTP disconnect behavior: a receive-side EOF may represent a valid HTTP client half-close while it still expects a response. WebSocket cancellation begins after server detection and remains cooperative; native precompiles and expensive preparation are still not forcibly preempted. No global production approval.

### HTTP reset cancellation without breaking half-close or pipelining

`rpcHttpDisconnectQualification` first records a failing baseline. Valid send-side half-close and pipelined half-close already succeed, but a TCP RST does not stop the simulation batch:46952 clock polls occur and the late filter mutation executes. That baseline remains archived.

While HTTP simulation work is active, a non-consuming asynchronous socket peek now detects definite socket errors and sets an atomic cancellation flag. The worker uses the existing cancellation path. Completion cancels an outstanding peek before returning to ordinary HTTP processing. EOF alone is deliberately preserved as a possible valid half-close; bytes belonging to the next request are never consumed by the watcher.

The unchanged three-case test passes after correction. Both valid half-close variants receive their complete responses, including the pipelined identity response. The RST case drops to52 clock polls including the successful follow-up calls, skips the late mutation, preserves canonical state and releases all eight simulation reservations for reuse. HTTP integration, WebSocket disconnect and server-shutdown regressions also pass.

Both sixteen-client HTTP Paris/Shanghai network campaigns pass with no client errors, finality during load, matching peer receipts/state roots, verified certificates and eight successful database reopen audits. Source snapshots, binary fingerprints, the initial failure and corrected results are retained.

Limits remain precise: a normal FIN cannot distinguish a client that still expects a response from full abandonment. If the peek finds pipelined bytes, it leaves them for the parser and stops watching; a later reset can remain undetected until the next read/write. Native routines and expensive preparation are not forcibly preempted. These scoped results do not establish global production readiness.

### Current cross-component regression after RPC lifecycle changes

`currentCrossComponentRegression` rebuilds all Release targets in the core and network build trees, then executes70 selected local project suites and all7 network component suites sequentially. All77 pass. The project campaign takes375.17seconds and the network component campaign0.05seconds. The initial CTest inventory attempt required repository write permission for its temporary log; discovery was rerun successfully after permission, before the builds/tests.

Coverage includes core/chain/RPC, HTTP/WebSocket disconnect and shutdown, installation and CLI paths, staking, storage write/sync fault injection, finality crash/recovery audit, ERC20/OpenZeppelin isolation, mempool capacity/history, gossip restart/retry/recovery, bounded RPC load/full-pool/nested computation, consensus-round logic and local official EVM/transaction/block fixtures including modern execution features. Network component tests cover bootstrap selection, frames, peer status, inbound budget, timed replies, JSON streams and acknowledgement progress.

The retained selection lists every included and excluded suite. The distributed `*_network` campaigns and standalone `load` suite are excluded here; their earlier scoped evidence remains separate. A snapshot of146 source files, executable fingerprints, CTest metadata and runtime fixture/contract fingerprints records the current build context. Neither a new full historical-corpus run nor public-network endurance is claimed.

The network configuration still accepts only Paris or Shanghai (`Genesis::from_json` in `src/chain.cpp`). Passing diagnostic execution tests for Cancun/Prague/Osaka does not activate those forks on the network. These77 passing suites establish a current regression baseline after RPC changes, not global production readiness, an independent audit or cross-host build reproducibility.

### RPC estimation resumed: latest and pending differential coverage

`rpcEstimateLatestPendingRequalification` extends the nested-call Nano/Anvil comparison from eight latest-only signed transactions to sixteen transactions across latest/pending, Paris/Shanghai, one/four nested CALLs and quotes from each client. Each quote executes successfully on both engines in the selected context. All sixteen signed transactions match on transaction hash, successful receipt status, gas consumed, logs, sender balance/nonce and leaf storage. Canonical balance/nonce/storage snapshots remain unchanged after estimation and calls. This fixture has no pending transactions when quoting; it does not add differential coverage of a populated pool.

The timestamp-dependent comparison also passes four identical signed transactions: the two latest-derived quotes fail after the timestamp-dependent execution path changes, while the two pending-derived quotes succeed. Nano and Anvil match on status, gas and storage. This expected failure remains explicit; a quote cannot guarantee execution after context changes. Five native RPC/HTTP/WebSocket shutdown/disconnect suites pass (11.57 seconds). No production code changed in this follow-up. Source snapshots, binary fingerprints and raw results are retained. These scoped results do not establish complete RPC parity or global production readiness.

### Current Shanghai admission: hashes verified, divergence reproduced

`shanghaiAdmissionCurrentRequalification` replays the current Nano binary against pinned Anvil 1.5.1. The 27 ordinary/boundary execution gas comparisons pass. For each of types 0/1/2, Nano rejects a 49,153-byte creation immediately; Anvil returns exactly Keccak-256 of the signed transaction, then leaves it without a receipt after explicit mining. Both canonical accounts remain unchanged and the same signed valid recovery transaction executes at the same nonce. The comparison deliberately retains exit code 1 and `referenceValidationMatches:false`. Added assertions verify each returned oversized-transaction hash against the original signed payload; a returned hash alone does not prove inclusion.

A fresh independent Geth v1.16.7 HTTP campaign passes all six admission cases: three oversized creations rejected and three 49,152-byte creations accepted at the same nonce. Pool membership, pending nonces and canonical state are checked. This supports Nano's rejection rule in `src/engine.cpp`, consistent with [EIP-3860](https://eips.ethereum.org/EIPS/eip-3860). No production rule was changed. The Geth campaign does not test inclusion or finality; no complete Anvil RPC parity or production readiness is claimed. Logs, exact results, source snapshots and binary fingerprints are preserved.

### Current full supported state corpus replay

`currentFullSupportedStateReplay` freshly verifies the pinned tests@v20.0.0 archive SHA-256 and all 7,603 extracted files for Berlin through Osaka. The current Release runner matches 44,244 cases including 2,893 expected validation rejections, with zero mismatches, errors or unsupported cases in those six groups. Per-group counts remain 1,963 / 2,250 / 2,441 / 10,777 / 13,074 / 13,739. A separate verifier checks every file/case/fork/variant identity against the input fixtures and detects omissions even when report totals are adjusted.

The first orchestration failed before EVM execution because the earlier three cached groups use a different directory layout; its failed log and source are retained. Corrected paths are byte-verified against the archive. All six successful raw reports, source snapshots, runner fingerprint and exact inventory are archived. This excludes earlier-fork groups from the 8,117-file archive inventory and does not qualify headers, RPC, networking, blob availability or production readiness. Cancun/Prague/Osaka are diagnostic execution profiles only.

### Current full block and fork-transition replay

`currentFullSupportedBlockReplay` verifies the pinned archive and all 7,972 selected fixture files, then replays the current Release block runner. All seven campaigns pass: Berlin 2,439, Paris 2,734, Shanghai 2,975, Cancun 12,114, Prague 15,799, Osaka 16,549 and four timestamp transitions totaling 260 cases. Overall: 52,870 matched cases, 60,257 accepted blocks and 4,940 expected rejected blocks; zero mismatches, unsupported cases or errors in the selected groups.

A separate verifier rehashes inputs and reports, checks exact file/case/fork coverage and verifies each fixture's accepted/rejected block counts and ordered rejection expectations. It also checks post-state and last-block-hash commitments through the report integrity parser. A negative control decrements one accepted-block count in an otherwise matched report and updates its checksum: the verifier rejects it. Source snapshots, runner fingerprints, archive-member inventory, raw reports and verification results are retained.

This qualifies execution commitments under the fixture adapter, which explicitly trusts externally validated Ethereum consensus inputs and permits historical signatures. It does not make Nano an Ethereum consensus client, activate modern network profiles, validate real blob availability, or establish global production readiness. Earlier-fork groups and engine API fixtures are outside this campaign.

### RPC deadline now includes pending preparation

`rpcPendingPreparationDeadline` fixes an uncovered interval: pending candidate preparation previously executed admitted transactions before the RPC clock started. A deterministic Paris/Shanghai regression queues a storage-writing loop transaction, asks for a cheap pending call/estimate and expects host cancellation. The baseline fails this assertion. The deadline now starts before snapshot preparation and is shared through candidate selection, both candidate execution passes, ancestor capture and all estimation trials. A copied ExecutionEngine carries the optional host checkpoint into transaction/system-call contexts; ordinary consensus engines retain an empty checkpoint.

953 RPC assertions pass, including direct cancellation rollback of fees/nonce/storage, block/state atomicity, unchanged canonical root/events/pool, recovery with a latest request and subsequent normal signed block production. The initial follow-up fails an older synthetic five-tick budget because preparation adds three checkpoints; changing that test budget to eight ticks preserves its cheap-call success and cumulative-estimation-expiry checks. The production timeout remains five seconds. All failures and logs remain archived.

Twelve regression suites pass in 108.62 seconds: core/chain/RPC, HTTP/WebSocket shutdown/disconnect, both local four-validator RPC-cost profiles, both full-pool pending-load profiles and the bundled official EVM regression corpus. Sixteen same-signed nested-call Nano/Anvil comparisons pass in latest/pending for Paris/Shanghai. This is not a new full-archive replay. Source snapshots, binary fingerprints and exact results are retained.

The budget remains cooperative. Queue waiting and initial RPC-lock acquisition are outside it; state copies, storage access, roots and native routines can only be checked at the next checkpoint. Pending preparation remains synchronized. No hard wall-clock guarantee, full RPC isolation or global production readiness claim.

### Modern signed requalification with all-field differences

`currentModernSignedRequalification` replays 129 same-signed transactions on Cancun/Prague/Osaka after the pending-budget engine change. Anvil 1.5.1 remains 125 matched / four mismatched (exit 1). Geth v1.16.7 independent sequential execution from a common synthetic state matches all 129 across nine sequences, including supplied post-state and computed roots (exit 0).

The new diagnostic inspects all selected receipt fields and the union of supplied account/slot values even after a mismatch, with 3,714 comparisons. Only the same four initial EIP-7702 cases differ: gasUsed/cumulativeGasUsed and sender/miner balances. Each gas delta is 12,500; every balance difference equals the corresponding effective-gas-price or priority-fee charge. Hash, status, type, logs/bloom, nonce, code and supplied storage values agree. A negative control adds a nonce difference to an already failing gas case: it is detected and the fee-only explanation becomes false. This strengthens diagnostic coverage without converting mismatches into passes.

The original first-difference report and new detailed/accounting reports are all preserved. Per-transaction Anvil prestates are RPC reconstructions, not full global roots or trie-existence proofs. Geth consumes its own preceding state independently, starting from a shared explicitly synthetic state. Neither comparison validates modern Nano network activation, sidecars, consensus, arbitrary contracts or global production readiness.

### Pending candidate execution detached from RPC and chain locks

`rpcPendingSnapshotIsolation` fixes a reproduced availability gap: with execution deliberately paused inside an admitted pending transaction, a concurrent eth_blockNumber request could not complete. Pending now captures the parent header, state, authenticated pool candidates and ancestor hashes under the chain lock, then returns a self-contained execution closure. Top-level RPC releases its mutex before candidate selection and execution. The engine reads captured ancestors, never the live store. Production and simulation share the candidate execution helper; consensus still has no RPC deadline.

969 RPC assertions pass, including concurrent canonical reads during paused pending work, cancellation without canonical/pool mutation, and replay of the same captured pending state/header after a development rewind replaces its ancestor. The first rewind fixture correctly encountered durable proposal reuse by the local signer. Its revision supplies a separately signed alternate block only inside the local rewind test. A missing block_auth.hpp caused a build failure; the immediately following old-binary test failure is retained but does not qualify the revised code. After fixing the include, the intended fixture passes. No signing protection was changed.

All twelve selected suites pass in 109.31 seconds, including core/chain/RPC, HTTP/WebSocket shutdown/disconnect, Paris/Shanghai distributed BFT with expensive RPC, both full-pool pending-load tests and bundled official EVM regressions. Sixteen latest/pending same-signed Nano/Anvil comparisons also pass. Full CTest output, failed/successful logs, source snapshots and binary fingerprints are retained.

State/pool/ancestor capture still holds locks; copying, disk access and native routines remain cooperatively bounded only at checkpoints. Wallet-internal estimates retain their enclosing RPC mutex. The snapshot cannot guarantee eventual inclusion, and these local scoped tests do not establish global production readiness.

### BFT recovery requalification after pending snapshot isolation

`currentBftRecoveryRequalification` runs eight existing adversarial/recovery scenarios against the current binaries after candidate preparation was refactored. All eight pass in 363.87 seconds, on Paris and Shanghai. Two isolated halves do not finalize; after all four cores are killed and restarted, each reproduces its exact reserved vote and the reconnected network converges. The weighted fixture confirms that participating 4/6 stake cannot finalize, while 5/6 can; the isolated validator subsequently imports the same certified block/state.

Both bounded load-crash fixtures admit 360 signed transfers. A validator is killed after load begins; a later marker is certified exclusively by the three survivors before the validator restarts. All 720 transfers across the two profiles finalize without loss or duplication, receipts/certificates agree across peers, and eight peer databases are reopened and audited. The two persistence fixtures exercise six acknowledged SIGKILL boundaries across reservation, commit and journal recovery, reject conflicting votes after restart, and finish with certified-state audit success.

Raw CTest output, extracted structured records, executed staking contract artifact, source snapshots and binary fingerprints are preserved. These are same-host, bounded process-failure tests; they do not establish physical power-loss tolerance, multi-host/endurance qualification, slashing, modern network activation or global production readiness. No flood or public network was used.

### V3 oracle arithmetic and current NFT/market lifecycles

`solidityV3OracleQualification` adds oracle assertions to the overlapping-position V3 lifecycle. The pool expands observation capacity to eight. Each of three swaps per profile verifies tick cumulative and seconds-per-liquidity cumulative from the prior observation, prior tick/liquidity and actual inclusion timestamp. An intervening native transfer ensures a genuine interior timestamp. A block-pinned observe call checks four instants per profile, including independently calculated interpolation. The slippage-revert snapshot now includes all eight observations, so callback/oracle writes must roll back with the swap.

Paris and Shanghai both pass, including pending-estimated swaps/decrease/collect, fee collection, all four position NFTs burned and ticks cleared. Four additional current-binary runs pass the OZ 5.0.2 NFT and deliberately vulnerable marketplace lifecycles. Fourteen NFT failure receipts verify authorization/revocation, receiver-write rollback and batch rollback. Two intentional application-level NFT thefts remain possible in the deliberately unsafe market; two duplicate attempts and two OOG transactions fail, unrelated assets stay unchanged, and later legitimate transfers finalize. These results do not claim that the chain prevents vulnerable-contract theft.

Six raw logs, structured results, source snapshots, 385 runtime-input fingerprints, native binary hash and matching implementation snapshot are retained. The oracle campaign covers three updates and four query instants per profile, not ring/timestamp wraparound, OLD rejection or long zero-liquidity history. Exact independent swap/fee mathematics and the full OpenZeppelin catalogue remain outside this scope. Tokens and funds are synthetic, with one local bootstrap validator; no bridge or production approval.


### RPC estimates under bounded load: signed execution and durability

`rpcEstimateSignedLoadRequalification` extends the expensive HTTP fixture beyond verifying quotes with eth_call. After four clients finish latest/pending work, each successful Paris/Shanghai scenario signs and submits the costly computation with exactly the returned pending gas allowance, without a manual margin. Depths zero, one and four cover six signed executions. Before submission storage is still zero; afterwards receipt status is successful, gas used does not exceed the estimate, sender nonce is two, storage is one and the block has a certificate. Full history audit and reopening the database with the same root pass.

All eight CTests pass, including both OOG scenarios. Only the qualification fixture changed; production estimation code is unchanged. Raw CTest output, source snapshots, binary fingerprint and results are indexed. This is a bounded single-bootstrap workload, not a new Anvil differential, a full corpus replay or production approval. Wallet-internal estimate locking, native-operation cancellation limits and wider contract/load coverage remain open.


### Wallet-internal RPC estimation isolation and signing revalidation

`rpcWalletEstimateIsolation` reproduces a canonical-read stall while an unlocked wallet's implicit gas estimation is paused inside EVM execution. An initial regression compile failed because its wallet config was out of scope; after fixing the fixture, the baseline fails the concurrent-read assertion. Both failures are retained.

Wallet and staking commands now receive an estimator that captures simulation state under the RPC lock, releases the lock for computation and reacquires it on success or exception. The transport cancellation callback reaches this estimate. Wallet transaction and staking methods reserve the shared bounded simulation queue, including explicit-gas calls, batches and notifications. Before signing, the wallet is looked up again and an implicit nonce is selected under the reacquired lock. Explicit nonces remain caller-controlled.

The final bounded-computation regression covers Paris/Shanghai signing and sending cancellation, concurrent canonical reads, wallet removal before signing, and a concurrent explicit-gas send causing the estimated transaction to select the next nonce. Canonical state remains unchanged. Fourteen suites pass in 61.52 seconds, including native RPC, HTTP, shutdown/disconnect, CLI and all eight costly simulation variants. The six estimated-gas signed executions retain their receipt/certificate/history/reopen checks. Full build, source and binary fingerprints, failed and successful logs are indexed.

The deterministic wallet scheduling checks use direct Rpc::handle; existing transport regressions and queue classification assertions do not exhaust wallet-specific HTTP/WebSocket interleavings. Snapshot capture and native routines remain cooperatively bounded, queue/lock waits are not a hard request deadline, and changes after quoting can still invalidate an estimate. No new full-corpus, Anvil differential or global production approval is claimed. This section supersedes the earlier wallet outer-lock limitation.


### Wallet estimates over HTTP/WebSocket: disconnect and half-close qualification

`rpcWalletTransportCancellation` adds real transport coverage for the prior wallet isolation change. Twenty wallet scenarios cover Paris/Shanghai and eth_signTransaction/eth_sendTransaction: eight WebSocket normal-close/reset cases and twelve HTTP reset/half-close/pipelined-half-close cases. Each request contains eight wallet commands and a later filter mutation. Calculation is paused inside the first EVM estimate; WebSocket identity replies on the same connection and HTTP identity reads on another connection remain available.

All twelve cancelled wallet batches leave the pool and canonical state unchanged, skip their later filter mutation, stop before 1000 checkpoints and release all eight simulation reservations. Valid HTTP half-closes complete their batches: 32 raw signed transactions decode to the expected sender, destination and nonce; 32 sent transactions exist in their isolated pools with contiguous nonces zero through seven. The latter are admitted only, not mined; canonical state remains unchanged. Subsequent eight-call batches reuse the released capacity in every scenario.

Four CTest suites pass in 12.05 seconds: the 20 wallet scenarios and five retained eth_call transport scenarios. An initial fixture compile failure from a malformed output initializer is retained along with the successful build, full CTest output, source snapshots and binary fingerprints. No production code changed in this campaign. Controlled loopback schedules do not exhaust arbitrary disconnect timing; in particular, pipelined bytes followed by a reset remain outside this coverage. A disconnection after admission cannot retract a transaction already accepted. This is scoped transport evidence, not global production approval.


### Full pinned EVM replay after RPC and pending-engine changes

`postWalletFullEvmRequalification` reruns the current state/block binaries after optional engine checkpoints, detached pending snapshots and wallet RPC isolation. The pinned tests v20.0.0 archive SHA-256 and every extracted input are reverified, with exact file inventories and case/fork/variant coverage. All six supported state groups (Berlin through Osaka) pass: 44,244 cases / 7,603 files / 2,893 expected transaction rejections. The six block groups plus four transition profiles pass: 52,870 cases / 7,972 files / 60,257 accepted blocks / 4,940 rejected blocks. No mismatch, unsupported case or adapter error occurs in this selected scope.

The state-report verifier now anchors reported expected/actual state roots, logs, receipt transaction hashes/gas and rejection categories directly to the archive-verified fixture values, and checks the expected rejection count. A valid control passes; two internally self-consistent fabricated reports (equal-but-wrong state roots and an invented rejection category) fail for the intended reasons. Block coverage verification checks each case's accepted/rejected block counts and ordered rejection expectations.

Sixteen current-binary same-signed nested-computation comparisons against Anvil also pass across Paris/Shanghai, latest/pending, depths one/four and quotes from each client. Both clients execute each quote; receipts, gas, logs, nonce, balances and leaf storage agree, with simulations leaving canonical state unchanged. Source and binary fingerprints remained unchanged during replay; reports, coverage results, orchestration and negative controls are indexed.

These are execution-adapter fixtures, not full Ethereum consensus or modern-fork network activation. The earlier Shanghai RPC admission disagreement and four modern Anvil differences remain open and retained. No bridge, real pool backing, public deployment or global production approval follows from these passing results.


### Actual filesystem ENOSPC at vote reservation and certified commit

`storageActualEnospcQualification` complements the earlier RLIMIT_FSIZE/EFBIG tests with real lack of free filesystem space. A temporary 64 MiB HFS+ disk image is filled until write returns ENOSPC; the fixture refuses the host filesystem or a volume above 128 MiB before creating any filler. A negative control on the host volume exits with the intended guard error and creates no filler. Docker was unavailable. The first hdiutil create command used an incompatible -format option; the corrected command created the isolated image.

Paris and Shanghai both pass at vote reservation and certified-block commit. RocksDB reports “No space left on device” on the expected database WAL, with partial appends of 4057 bytes for reservation and 1865 bytes for commit. No signature escapes a failed reservation; canonical height/state remain at genesis, and a failed commit does not advance the journal's voting position. The child exits without destructor flush while space remains exhausted. After removing only the filler, a fresh process reopens the databases, verifies no partial canonical indexes, retries and commits exactly one transfer. The already reserved precommit signature is preserved across commit failure. A third process verifies height one, sender nonce one, recipient balance 123, receipt, certificate, voting position two and a successful history audit.

All four ENOSPC scenarios and all 28 existing partial-write EFBIG scenarios pass. The image was detached successfully. Raw failures/recovery/reopen logs, environment/image/binary fingerprints, source snapshots and orchestration are indexed. No production storage code changed. This establishes scoped macOS HFS+ recovery after actual disk exhaustion; Linux, physical power loss, in-process recovery, compaction/manifest rotation and sustained disk pressure remain outside this evidence.

Manual fixture interface: `nano-storage-write-failure-tests <Paris|Shanghai> <isolated-volume/case> <reservation|commit> <fail|recover|verify> enospc`. Start with a fresh case directory on a small disposable mounted image; remove its `enospc-filler` only after the failed child exits, then run recover and verify. Never use a production database directory.


### Same-process retries after synchronization errors

`storageSameProcessSyncRetryQualification` extends the test-only macOS fsync/F_FULLFSYNC EIO fixture. After observing the initial synchronization error at reservation, canonical commit or journal progression, the injector is disabled. The same process retries the relevant operation (including recover_voting_progress after a durable canonical commit), then attempts a conflicting vote for the same slot. The fault counter must remain unchanged, so retry rejection cannot be attributed to another injected error.

All twelve new cases pass across Paris/Shanghai and retained/lost unsynced WAL suffixes. RocksDB continues rejecting the retry, no signature escapes the failed reservation or conflicting attempt, and live height/state/events/pool remain unchanged by those attempts. On restart, the existing recovery checks retain the certified block/precommit when appropriate and produce a successful history audit with the transfer applied once. The twelve original sync-failure cases also pass: four CTest suites / 24 cases / 11.14 seconds. Exact error strings, logs, test-only injector/source/binary fingerprints and results are indexed.

No production code changed. This establishes fail-closed behavior for the exercised synchronization errors and requires process/database reopening for these recoveries; it does not establish automatic in-process recovery. It is separate from the actual-ENOSPC campaign and does not prove same-process disk-full recovery, physical power-loss behavior or Linux qualification.


### Uniswap V3 flash protocol-fee split, donations and authorized collection

`v3FlashProtocolFeeQualification` extends the earlier dual-asset flash fixture on Paris and Shanghai. With protocol denominators four/five, independent integer arithmetic checks protocol fee accumulation and Q128 LP fee growth for a dual-asset loan, a zero-borrow donation of 17/23 units and one-unit loans on each side separately. Pool/borrower balances, callback observations, liquidity, slot0 and Flash event amounts agree. Owner collection transfers 10/15 units, leaves one unit of each protocol fee and does not alter LP fee growth. A following zero collection preserves the snapshot.

Both profiles pass: twelve successful flash calls, including eight new protocol-fee cases, and sixteen persisted failure receipts. Negative cases cover forged callbacks, absent/short repayment, unauthorized fee setting/collection and invalid denominator bounds. Reverted receipts contain no logs and snapshots preserve token balances, protocol/LP fee accounting and callback state. Every returned network block has a BFT certificate. Only tests changed; all assets and liquidity are synthetic.

Arithmetic was checked against the official [V3 source reference](https://raw.githubusercontent.com/Uniswap/v3-core/v1.0.0/contracts/UniswapV3Pool.sol); deployed bytecode remains the separately fingerprinted npm core 1.0.1/periphery 1.4.4 artifacts. The unavailable source tag v1.0.1 is recorded rather than treated as fetched evidence. Raw logs, compiled borrower/token artifacts, dependency inputs and source/binary fingerprints are indexed. The scope is one pool/position and selected fee settings, not all token behaviors, collection of the resulting LP fees, a bridge, real USDC backing or production approval.


### Uniswap V3 LP payout after flash protocol-fee accounting

`v3FlashLpFeeCollectionQualification` completes the preceding fee-collection path on Paris and Shanghai. With one active position and unchanged ticks/price, the fixture computes each expected LP payout independently as floor((global growth - position's last inside growth) * liquidity / 2^128). The first collection pays exactly 67/128 atomic units; a second collection pays zero. Owner and pool balances, emitted collection amounts, zero owed balances, updated position growth checkpoints, unchanged NFT ownership/liquidity and retained protocol-fee balances all agree.

All four collection transactions use the returned pending eth_estimateGas allowance without a manual margin and succeed. Both profiles also retain the twelve successful flash calls, sixteen expected failed receipts and certified-block checks from the previous fixture. Only the test script changed. Prior native/contract sources, dependency inputs and binary hashes were reverified unchanged; their existing artifact index is referenced alongside the new script, results and logs.

This closes the prior LP-withdrawal gap for this single-position flash fixture. It does not establish fee attribution across crossing ticks or multiple positions, arbitrary token behavior, real USDC backing or production readiness.


### BFT live stake change: healed minority and reopened chain histories

`bftStakeChangeHealedReopenQualification` extends the existing four-validator stake-change scenario beyond the weighted majority's first commit. The staking transaction is certified by the old [1,1,1,1] set. After weights become [4,1,1,1], the isolated 4/7 signer cannot finalize the new transfer during the observed twelve-second interval, whereas a connected 5/7 pair certifies it with two signatures.

The test then heals all cpp-libp2p links without restarting the cores. Both minority nodes obtain the same successful receipt. All four nodes verify the new-weight certificate and agree on block hash, historical state root and a recipient balance of one at that fixed height. After clean shutdown, each chain database is reopened independently; the historical root/receipt, current stake weights and full history audit are verified. Paris and Shanghai both pass: eight chain-database reopens, two CTests, 56.95 seconds.

An initial fixture build failed on implicit JSON/string comparisons; explicit conversions fixed it before execution. Failed/successful build logs, full test output, source/staking fixtures and binary fingerprints are retained. No production code changed. This is bounded single-host loopback evidence, not WAN qualification, a flood test, post-heal reactor/journal restart, slashing coverage or global production approval.


### BFT restart with persisted journals after live stake change

`bftStakeChangedReactorRestartQualification` extends the healed-partition scenario through real process restart. All four cores reopen their original node config, chain database and vote journal; the relays reconnect them. A former minority signer submits a fresh two-unit transfer. Every peer obtains the same successful receipt at a height beyond the earlier one-unit transfer, and each certificate verifies against the new [4,1,1,1] weights. The recipient has exactly three units and the earlier finalized receipt is unchanged.

After another clean shutdown, all four chain databases and journals are opened independently. The new historical state root survives; every journal's active height equals its own canonical tip plus one and its validator-set hash equals the new weighted set. Full chain history audits pass. Paris and Shanghai both pass: eight restarted reactors, eight journal checks and eight post-restart chain audits, two CTests in 59.31 seconds. The earlier pre-restart/healing checks remain in the same scenario.

Only the native test changed. Sources and dependency inputs from the preceding campaign were reverified; current core/test/relay binary hashes, full CTest logs and results are indexed. This closes the earlier post-heal reactor/journal restart gap for clean local restarts. It does not combine a stake transition with SIGKILL, simulate physical power loss, or establish multi-host/WAN, slashing or global production readiness.


### BFT abrupt all-core restart after live stake change

`bftStakeChangedAbruptRestartQualification` adds separate Paris/Shanghai CTests using --stake-change-crash. The original clean variant remains available. After the live [1,1,1,1] to [4,1,1,1] transition, weighted-quorum partition and four-peer catch-up, all four core children receive SIGKILL; every wait result must explicitly report -SIGKILL. Original configs, chain databases, journals and consensus keys are reused on restart.

Both variants pass: eight confirmed abrupt exits, eight restarted reactors and eight post-restart journal checks. A fresh two-unit transfer from a former minority node finalizes on all four peers with a new-weight certificate. The prior one-unit transfer remains finalized, the recipient has exactly three units, historical state roots survive and final chain audits pass. Journals align with their respective canonical tip plus one and the current weighted set. Two suites complete in 65.24 seconds; logs, source/staking fixtures, runtime binary hashes and results are indexed.

Only tests/CMake registration changed. This covers a crash after the transition and healed transfer have already finalized, not a kill precisely inside the activation write. It is process-crash evidence, not physical power-loss/page-cache loss or multi-host/WAN qualification. No production approval is inferred.


### Uniswap V3 exact fee attribution across two crossing positions

`v3CrossingPositionFeeQualification` extends the overlapping narrow/wide-position scenario to four swaps per profile. An additional price-limited swap stays entirely outside the narrow range while inside the wide range. The narrow position's inside growth and accrued fees must remain exactly unchanged, while the active wide position earns input-asset fees. Other swaps leave and re-enter the ranges; existing balance, liquidity, slippage rollback and oracle arithmetic/interpolation checks remain active.

Before withdrawing each position, off-chain BigInt arithmetic derives fee growth inside its lower/upper ticks from global/outside counters with uint256 subtraction semantics, subtracts the position checkpoint and applies liquidity/Q128 rounding. The collected amounts less returned principal must equal those predictions exactly for both assets, replacing the previous positive-fee-only assertion. Paris and Shanghai pass eight swaps, eight asset-payout comparisons and four position closures; all positions are burned and ticks cleared. Actual pending gas quotes continue to fund swaps and withdrawals.

Only the Hardhat test changed. Results, source/contract/dependency inputs, the pinned Tick/Position arithmetic references and native binary hash are indexed. This validates per-position accounting against the observed on-chain counters, not an independently reconstructed complete swap-step execution from inputs. Shared-range NFT rounding, extreme fee-growth wraparound and arbitrary token behavior remain outside this campaign; no bridge or global production approval follows.


### RPC exact gas quotes for typed nested transactions

`rpcTypedNestedEstimateQualification` extends the expensive nested-call differential harness with selectable transaction types. Type 1 and type 2 both use an access list covering the leaf storage slot. Paris/Shanghai, latest/pending, depths one/four and both quote providers yield 32 signed transactions. Each provider quote also succeeds in eth_call on both engines. No gas margin is added. Receipt status, gas consumed, effective gas price, transaction hash, logs, sender balance/nonce and leaf storage agree; estimates and calls leave canonical state unchanged.

The initial attempt passed eight type-1 transactions before failing the sender-balance comparison on the first type-2 transaction, despite equal successful status and gas consumed. Its log and initial harness are preserved. The corrected harness explicitly aligns the Anvil mined block base fee with Nano, in addition to the existing timestamp alignment, and asserts block base fee and effective gas price equality. All 32 cases then pass. This isolates estimation/execution from the clients' independently scheduled block histories; it does not qualify independent base-fee evolution.

Only the test harness changed. Runtime/source hashes, both attempts and final results are indexed. This remains local scoped qualification with an initially empty transaction pool, not a guarantee after state/fee changes or production approval.


### Independent RPC fee evolution and empty-block diagnosis

`rpcEstimateIndependentFeeEvolutionQualification` checks the live Nano chain against an off-chain BigInt implementation of the post-London [EIP-1559 formula](https://eips.ethereum.org/EIPS/eip-1559). Every header between successive signed estimates is fetched, parent linkage verified and base fee independently derived. Sixteen type-2 transactions pass on Paris/Shanghai, covering 63 header transitions: 14 increases and 49 decreases. Fee-history base fees, projected next fee, utilization and reward percentiles agree with headers/receipts; the sender debit equals gas consumed times independently computed effective price. Both clients still execute the exact same signed transaction at the quoted gas without extra margin.

The retained traces explain the prior balance mismatch: Nano finalizes empty blocks while expensive estimates run, whereas Anvil is manually mined once per submitted transaction. The first Paris transfer lands at Nano height four after four zero-gas parents, with base fee 0x22f06c09; Anvil's natural next fee from genesis is 0x342770c0. All sixteen histories differ before the explicit comparison override. This is observed scheduling/context divergence, not evidence of an incorrect Nano EIP-1559 formula. The prior unaligned failure remains preserved.

Only the test harness changed. The indexed evidence includes all fetched parent/current headers, fee histories, receipts, hashes and runtime/source fingerprints. Scope excludes London activation, varying gas limits, all fee-rounding boundaries and global production approval.


### Anvil 1.8.3 reference refresh: admission corrected, gas divergences remain

`anvil183ReferenceRequalification` uses the official immutable v1.8.3 Darwin ARM64 release (commit cae51ad458f6abb64852b7709eb784352429825d). Archive SHA-256 562f9c2f9094e512f1efc1e005c79de7642c27ffc1e7e9dcf8e31baa54577d6e matches the retained GitHub release API digest before extracting Anvil. It is isolated under qualification; the pinned global 1.5.1 binary is untouched. Release metadata, archive, binary, test sources and runtime hashes are indexed. No signature-attestation verification is claimed.

Shanghai now rejects all three oversized 49153-byte creations (types 0/1/2), matching Nano, with unchanged canonical state and successful same-nonce recovery. However the overall campaign exits 1: 21/27 ordinary gas comparisons match and six near-limit creations differ. At 49151/49152 bytes Nano consumes 252716/252720 gas, while Anvil consumes 512600/512610. Independent arithmetic derives Nano's values from Shanghai intrinsic/initcode costs plus four execution gas; the reference values equal the EIP-7623 calldata floor. An initial scratch calculation counted the two PUSH0 operations as six gas; this was corrected to four before accepting the retained analysis.

Modern comparison also exits 1: 122/129 match. Four existing first-authorization empty-account refund mismatches remain in Prague/Osaka. Three new Cancun calldata cases consume 21512 gas in Nano versus 22280 in Anvil, again matching pre-floor versus EIP-7623 floor arithmetic. The exhaustive difference report covers 3714 receipt/account/slot checks. These figures suggest a reference fork-gating issue but do not establish its source-level cause. All failures remain visible; Nano gas rules are unchanged, the newer reference is not promoted as a universal oracle, and global readiness is unproven.


### Anvil 1.8.3 stale gas table: source-level diagnosis

`anvil183GasTableSourceDiagnosis` traces the nine new pre-Prague floor discrepancies to standalone Anvil initialization at the tested commit. In crates/anvil/src/config.rs:1379-1380, CfgEnv::default() is followed by direct assignment to cfg.spec. Pinned revm-context 43.0.2 initializes gas_params from the default spec, and pinned revm-primitives 43.0.0 defaults to Osaka. Changing the public spec field does not rebuild this gas table. REVM's handler consumes that stored table for intrinsic/floor costs and applies its floor after execution. This explains the observed Shanghai/Cancun floor costs even though opcode/validation fork checks can still use the selected older spec.

The separate fork-from-RPC initialization uses set_spec_and_mainnet_gas_params at config.rs:2020; it is not the standalone path used by these campaigns. A one-line candidate patch uses that same setter during standalone setup. The patch is archived only: it has not been built or executed, and neither the official reference binary nor Nano production code is modified. The four empty-authority authorization divergences remain separate.

All seven fetched Foundry source/lock files match Git blob identities in the exact commit tree; the four fetched REVM crate archives match Cargo.lock SHA-256 checksums. Source files, dependency archives, diagnosis and unapplied candidate patch are indexed. Initial source inventory retrieval failed on an unquoted shell question mark and was retried with a quoted URL; no source was accepted from that failed request. This closes the source explanation, not patched-reference validation or global production readiness.


### Independent Geth replay of Anvil 1.8.3 cases

`anvil183IndependentGethSequenceQualification` replays all 129 captured raw transactions through Nano and pinned Geth 1.16.7. Nine Cancun/Prague/Osaka sequences begin with a common explicit state, then each engine independently consumes its own preceding poststate. All 129 gas, supplied poststate and state-root comparisons pass, including the seven transactions differing from Anvil 1.8.3. This strengthens the conclusion that Nano should not adopt the observed reference gas/refund discrepancies.

Sources, binary fingerprints, complete per-step results and comparison tool are indexed. This does not validate Anvil's global state root, network consensus or header rules, and does not substitute for testing the separately prepared Anvil gas-table patch.


### Anvil 1.8.3 empty-authority refund source reconfirmed

`anvil183AuthoritySourceDiagnosis` checks the exact newer reference source rather than assuming the 1.5.1 explanation remained unchanged. EmptyDBWrapper::basic_ref in crates/evm/core/src/backend/in_memory_db.rs:104-106 still returns Some(default account) for an absent address. REVM's journal converts Some into a normal account; only None creates the not-existing marker. Its EIP-7702 handler refunds when an authority is not simultaneously empty and loaded-as-not-existing/untouched. Thus the wrapper can grant the existing-account refund to an absent authority.

All four Prague/Osaka differences in the retained 1.8.3 campaign are exactly 12500 gas, with the comprehensive report accounting for every differing fee balance. The relevant source excerpts and four observed cases are separately indexed; their upstream/dependency provenance was verified in the source campaign. The single gas-table candidate currently being compiled does not change this wrapper. No authority fix is claimed or applied to Nano.


### Anvil gas-table correction confirmed by compiled replay

`anvil183PatchedGasRuntimeQualification` compiles the isolated v1.8.3 source archive with Rust 1.96.0, the unchanged upstream Cargo.lock and the single previously archived setter correction. Compilation exits zero. The initial Rust 1.94.1 dependency-version failure remains archived; the default compiler remains 1.94.1.

Shanghai now passes all 27 ordinary gas comparisons and all three oversized-initcode rejection/recovery cases. Modern replay returns 125/129 matches, with exactly the four previously diagnosed Prague/Osaka empty-authority differences remaining. All 129 signed transaction hashes and case identities match the official 1.8.3 campaign. The three Cancun and six Shanghai stale-floor discrepancies disappear. This confirms the gas-table explanation by intervention; the overall modern command still exits 1 and is recorded as a failure.

The diagnostic binary reports 1.8.3-dev but its embedded Git SHA was inherited from the surrounding Nano checkout, not from Foundry. Provenance therefore uses the exact downloaded archive, 2441 verified upstream Git blobs, unchanged lockfile, sole source patch, compiler/build log and binary SHA-256. The binary, source patch, runtime fingerprints and complete results are indexed. No official reference binary or Nano execution rule was changed. A locally patched reference is not promoted to an official oracle, the authority issue remains separate, and global readiness is not established.


### Missing-authority intervention resolves only fresh cases

`anvil183MissingAuthorityIntervention` compiles a second diagnostic reference with EmptyDBWrapper returning None rather than Some(default), retaining the separately verified gas-table correction. A complete tree comparison confirms exactly those two changed source files among 2441 checked Git blobs. Compilation succeeds with Rust 1.96 and unchanged lockfile. All 129 case identities and signed hashes match the prior campaign.

The intervention yields 127/129 matches: the fresh-authority Prague/Osaka cases now match, while both touched-empty cases still differ by the refund. The command correctly exits 1. This qualifies the earlier source explanation: the absent-account wrapper is sufficient to explain the fresh cases but changing it alone does not repair previously touched empty accounts. The REVM CacheDB commit_account path stores touched, non-selfdestructed accounts as present without empty-account deletion; Anvil's commit delegates into that cache. A separate lifecycle intervention is needed before claiming that remaining cause is repaired.

Both binaries and all earlier failures remain archived. The new binary, two-file provenance, patch, build log, complete replay and retained cache-source excerpt are indexed. This is diagnostic causal testing, not a general Foundry fix, official reference promotion, or Nano production modification.


### Touched-empty cleanup intervention closes the diagnostic replay

`anvil183TouchedEmptyIntervention` adds a third isolated source change: after building the receipt, before CacheDB commit, post-Spurious-Dragon touched accounts with empty account info receive the cache's deletion marker. This diagnostic adapter models EIP-161 cleanup without executing a SELFDESTRUCT opcode. It is not a general Foundry patch; state-change publishing and other backends are outside scope. Full-tree verification identifies exactly three changed files among 2441 checked blobs, with the original lockfile retained.

Compilation and both test commands exit zero. All 129 modern transactions now agree; all case identities and signed hashes equal the prior campaign, and the exhaustive receipt/account/slot verifier finds no differences. The two touched-empty Prague/Osaka cases are the only newly resolved cases after the prior 127/129 result. Shanghai remains 27/27 for gas comparisons, with all three oversized creations rejected and same-nonce recovery passing.

This intervention completes the causal chain for these observed reference discrepancies: stale gas table, absent-account wrapper and touched-empty cache persistence. Official 1.5.1/1.8.3 reports and all intermediate failures remain intact. The exact diagnostic binary, patches/source hashes, build log and full results are indexed; its embedded ancestor Git SHA is explicitly disclaimed as Foundry provenance. Agreement with a locally altered reference is not independent proof of correctness; the separate official fixtures and 129/129 independent Geth sequences remain the stronger qualification evidence. No Nano execution changes, network activation or global production approval follow.


### Broad native regression after Anvil diagnosis: four failures retained

`postAnvilBroadNativeQualification` rebuilds the release core and cpp-libp2p transport, then runs all 124 selected CTests with concurrency two. The explicit bft_restart_flood_network scenario is excluded. In 1691.13 seconds, 120 pass and four fail; CTest exits 8. JUnit/inventory/log cross-checking verifies every selected test appears exactly once, none skipped, with 765 unchanged input fingerprints. Missing/duplicate-case negative controls are rejected. The first report-reader pattern missed CTest's unspaced ***Failed padding; the corrected reader and original version are retained, without changing results.

Three failures are client deadlines: bft_rpc_cost_many_paris_network and bft_rpc_cost_websocket_{paris,shanghai}_network. Each records worker 2 timing out while the marker finalizes with 16 active workers and consensus heights continue advancing. Both clients enforce a ten-second request deadline. The simulation execution deadline begins after queue wait; source inspection therefore identifies a possible contributing wait, not a demonstrated cause. No timeout has been enlarged and no retry masks these failures.

The fourth failure is bft_stake_change_crash_shanghai_network: activation, partition/heal and post-restart transfer finalize, but the final reopened journal-alignment assertion fails. Certified RPC import advances the chain separately from reactor journal reconciliation, which occurs when constructing the next consensus round. This suggests a shutdown-window/test-assumption issue but remains unproven without a targeted reproduction. The same Paris scenario and clean Shanghai restart pass in this campaign.

Full logs, JUnit, original sources/fixtures, binary fingerprints, the core/transport/BFT-test binaries, verifier and negative controls are indexed. No runtime source was changed during the campaign. The four failures remain open; scoped successes, including the 19 EVM regression suites, do not establish production readiness.

### RPC request-level timing: isolated HTTP and WebSocket follow-up

The 2026-09-24 follow-up adds per-request method, block tag, sequence number, elapsed time and failure recording to the bounded four-node RPC workload. Previously a recorded round could contain estimate, quote verification and identity read, so its duration was not a single-RPC latency. Client deadlines remain ten seconds, with no retries. Core and cpp-libp2p binary hashes match the broad campaign; only the test driver was rebuilt.

Isolated Paris runs pass over both transports: each completes 960 requests (192 estimates, 384 calls and 384 identity reads), 384 worker rounds, marker finality during load, post-load transfer and four database reopen audits. HTTP maximum estimate/call/identity durations are 6876.14/5125.63/12.28 ms; WebSocket maxima are 6968.81/5355.58/23.52 ms. These scoped successes do not close the earlier concurrent-campaign timeouts. The request verifier rejects a negative control with a missing identity request.

Evidence maintenance incident: the rebuild initially reused `rpc-estimate-resume-build.log`, overwriting the older build-only log referenced by `rpcEstimateLatestPendingRequalification`. No identical backup was found. Its original filename/hash/recorded status are retained explicitly as unavailable historical evidence in the manifest; the new log is named `rpc-estimate-request-trace-build-20260924.log` and is not presented as the old evidence. Historical test results and indexed source snapshots remain intact.

With both WebSocket profiles scheduled concurrently (`ctest -j2`), Paris reproduces the timeout: worker 6, request 37, `eth_estimateGas(latest)`, 10002.14 ms. Paris completes 937 successful requests plus that failure and cannot reach its reopen checks. Shanghai completes all 960 requests and reopen checks, with maximum estimate latency 8336.13 ms. Thus the issue is not specific to worker 2 or `pending`. The paired result demonstrates sensitivity to concurrent host load; it does not separately measure server queue wait, lock wait or EVM execution time.

The 18 `rpc_cost` wall-clock qualification tests now declare CTest `RUN_SERIAL`: each retains its internal concurrent clients while other CTest campaigns cannot compete for its CPU budget. This is test scheduling isolation, not a Nano runtime speedup or a solution to overloaded public RPC. All client deadlines and success requirements remain unchanged. The concurrent failure is retained, and end-to-end RPC queue latency under host contention remains an operational qualification gap.

The final `-j2` rerun with `RUN_SERIAL` demonstrably schedules Paris then Shanghai. Both pass (150.45 and 149.00 seconds): 1920 verified requests, including 384 estimates. Maximum individual estimate durations are 8635.60 ms and 8993.24 ms. Host scheduling still varies across isolated runs; this does not establish a deterministic latency bound. `rpcEstimateRequestLatencyQualification` indexes the complete request traces, successful/failed runs, scheduling inventory, source snapshots, driver binary and missing-request negative control. The next runtime investigation must measure queue admission/start, RPC lock acquisition and execution separately.

### RPC queue, lock, preparation and execution measured separately

`rpcPhaseTimingQualification` adds opt-in local instrumentation controlled by `NANO_RPC_TIMING_DIR` (an existing writable directory, set before starting the process). Files contain only an internal request number, phase, monotonic start time and duration; no request parameters, keys or transaction payloads. Logging errors are ignored. File I/O adds observer overhead, so enable this only for bounded diagnostic campaigns; no automatic rotation is provided.

Two concurrent four-node WebSocket campaigns pass this instrumented repetition: Paris/Shanghai each complete 960 client requests, including 192 estimates, with finality during load and four reopen audits. The eight node traces contain all 1152 queued simulations. The verifier requires 144 simulations per node, five phases per simulation and four nodes per profile; removing a phase is rejected. Two native RPC/HTTP regression suites also pass.

The maximum queue waits are 5609.15 ms (Paris) and 5623.90 ms (Shanghai); maximum execution times are 2559.65/2593.16 ms, preparation 5.11/4.91 ms and RPC-lock wait 284.97/292.13 ms. Paris's slowest request spends 5609.15 ms queued and 2420.36 ms executing. Shanghai's slowest spends 5431.93 ms queued, 283.28 ms acquiring the lock and 2454.17 ms executing. Queue plus worker totals peak at 8034.55/8173.84 ms, excluding wire delivery and client processing.

These observations locate the dominant delay in the measured tail, but do not reproduce or completely explain the prior ten-second failures. No timeout, queue policy, estimation algorithm or execution semantics changed. Queue-wait bounding and explicit overload handling remain to be implemented and tested, including batch/notification capacity accounting, cancellation and wallet-side effects. Full traces, the instrumented binary, source snapshots, verifier versions and negative control are indexed; old failures remain visible.

### Queued RPC expiration without late execution

The simulation lane now limits waiting before a job starts to four seconds by default. HTTP returns 503 with `RPC simulation queue wait exceeded`; WebSocket returns JSON-RPC -32005 for the expired item and unexecuted batch suffix. Notifications produce no JSON-RPC reply. Work that already started is not cancelled by the queue timer and remains subject to existing execution/disconnect/shutdown checks. For WebSocket batches, earlier completed items are not rolled back; only the unexecuted suffix is abandoned. HTTP dispatches the whole simulation envelope as one queued job, so expiry prevents that envelope from starting.

An atomic waiting/running/expired transition arbitrates the worker and timer. The worker also checks the deadline itself, so a delayed event loop cannot revive overdue work. A reservation remains charged until the worker discards the expired job, preventing timed-out requests from accumulating an unbounded executor backlog. Response completion is confined to the event loop and emitted at most once. The queue deadline does not bound work already running, RPC mutex contention or a blocked event loop; it is not a hard end-to-end latency guarantee.

Initial focused validation passes seven suites (17.64 seconds), including eight controlled HTTP/WebSocket cases with regular/notification batches and unlocked test wallets. With an injected 50 ms queue deadline and a deliberately paused running simulation, expiry responds before that worker is released, a seven-slot request remains rejected while expired reservations are outstanding, and all eight slots become usable after draining. No expired wallet submission enters the pool and no expired suffix creates its filter. Running work completes normally. Existing shutdown and HTTP/WebSocket wallet disconnect tests remain passing. The additional delayed-event-loop case and the unchanged sixteen-client throughput gates are recorded separately below once completed.

The final rebuilt version passes ten focused suites in 39.48 seconds, including Paris/Shanghai successful depth-four estimates. The new delayed-event-loop test holds the executor past its deadline before running the timer loop: zero work executions, exactly one expiry delivery and exactly one reservation release. All eight transport/notification/wallet combinations pass again. Full release build succeeds; an initial attempt to build the new target before CMake regeneration failed (`No rule to make target`) and is retained alongside the successful configured build.

The unchanged sixteen-client throughput gates fail explicitly: WebSocket Shanghai completes 614 requests with six `RPC simulation queue wait exceeded` errors at 4000.97–4004.64 ms; HTTP Paris completes 610 requests with six non-OK HTTP replies at 4002.31–4012.70 ms. The HTTP test client discards status/body, so those six responses are not individually established as queue-expiry 503s by this load trace; the controlled HTTP tests verify that response directly. No client transport deadline expires in these two runs. Marker finality is observed with all sixteen clients active, and heights continue from 3 to 99 (Shanghai) and 4 to 103 (Paris). These failing gates stop before their post-load transfer/reopen checks.

`rpcQueueExpiryQualification` preserves these failures and indexes both campaigns, source/binary snapshots, race/transport verification and build logs. This is bounded rejection under overload, not a throughput fix. The original all-success load requirement remains open; no client deadline or test acceptance condition was relaxed.

### Two bounded RPC workers: throughput improves, contention remains explicit

The simulation lane now has two workers, retaining the eight-simulation shared HTTP/WebSocket reservation limit, four-second queue expiry and five-second cooperative execution budget. State/header snapshots remain captured under the RPC lock and execution uses independent copies. This permits two simultaneous computations per node and increases CPU/memory concurrency; it does not provide a hardware-independent throughput guarantee. The local qualification host reports 15 logical CPUs.

The queue-expiry fixture now pauses every worker independently inside execution before submitting work that must expire. All eight HTTP/WebSocket, notification and wallet combinations pass with both workers paused. The delayed-event-loop case also occupies both worker slots before checking that an overdue job never starts. Twelve rebuilt regression suites pass in 65.37 seconds, including both 10,000-transaction pending-load profiles and successful depth-four estimates. The HTTP qualification client now preserves non-OK response status/body in its diagnostic instead of a generic transport message.

The first two unchanged no-error load gates pass: HTTP Paris completes 960 requests in 89.29 seconds (maximum estimate 4807.87 ms), and WebSocket Shanghai completes 960 in 92.03 seconds (maximum estimate 5569.53 ms). Each checks 192 estimates, quote execution, marker finality while sixteen workers are active, post-load transfer, certified history and four reopened databases. Previous one-worker queue-expiry failures remain preserved.

Running both four-node WebSocket networks concurrently still exposes a limit: Shanghai passes 960 requests; Paris records 945 requests including two queue-expiry errors, for pending call/estimate at 4001.71/4002.30 ms. No client transport deadline expires. Paris does not reach the post-load/reopen checks, so this paired no-error gate remains failed. The remaining HTTP Shanghai and WebSocket Paris isolated results are recorded below when complete; no global production claim follows.

The remaining gates pass: HTTP Shanghai in 90.77 seconds, WebSocket Paris in 90.79 seconds, with maximum estimates 5310.39/5349.08 ms. Thus the isolated Paris/Shanghai × HTTP/WebSocket matrix completes 3840 requests and 768 estimates without error using the original success criteria, plus finality, post-load transfers and reopen audits. The paired-network Paris failure remains separate and unresolved.

Sixteen same-signed type-2 comparisons against the unchanged official Anvil 1.5.1 binary pass on the current Nano binary (Paris/Shanghai, latest/pending, depth one/four, quotes from each client). Both estimates execute on both engines; selected receipt fields, gas, effective fees, storage, nonce and sender balance match, and simulations preserve canonical state. Timestamp/base fee are explicitly aligned for comparison; this is not equality of block hashes, full headers or global state. The independent Nano parent-fee checks remain enabled. `rpcTwoWorkerQualification` indexes all passing/failed campaigns, sources, binaries and reference fingerprint. An initial aggregate log reader attempted to parse unrelated multiline JSON in native output; restricting extraction to the eight queue-test records corrected the reader without changing test results.

### Certified-import shutdown window: journal recovery invariant corrected

`bftJournalImportWindowQualification` reproduces the overly strict journal-alignment assertion without changing runtime code. A validator reserves a nil prevote at height one, then imports two valid certified blocks signed by other validators, including a staking-weight transition. The chain and journal are closed before another round is created. On reopening, the canonical tip is two while the journal remains at height one with the old validator-set hash. `ConsensusRound` construction invokes existing certified recovery and advances the journal to height three/current weights before a new vote is released.

The native regression verifies preservation of the old reservation, refusal to replace its nil prevote with a conflicting authenticated proposal, current-height voting and exact-signature replay on a second reopen. A separate rollback control presents the height-three journal to a genesis-only chain: recovery must refuse the ahead journal without resetting its position or reserved vote. The complete consensus-round suite passes.

The network test now invokes and checks the same recovery operation after final shutdown, recording each journal before/after instead of requiring immediate raw alignment. It still requires the correct certified weights, audited history, unchanged canonical tip/state during recovery and matching historical transfers. Four local network scenarios pass (Paris/Shanghai × clean/SIGKILL), together with the native suite: five passing suites and sixteen verified reopened journals. The clean Paris run actually records one lagging journal (height ten beside canonical tip ten) advancing to eleven. Other peers may have stopped at different valid certified suffixes; each recovered journal is checked against its own tip, not an invented common final height.

This demonstrates a legitimate state that invalidates the former assertion. The original broad-run failure did not retain raw journal/peer values, so its exact lost process state is not reconstructed. Its failure remains archived. The Nano executable is byte-identical to the previous RPC qualification; only tests/documentation changed in this step. Source/binary fingerprints, full logs, JUnit and verification results are indexed. This scoped recovery evidence is not full multi-host/power-loss qualification or a global production approval.

### Broad native campaign after RPC and journal recovery changes

`postRpcBftBroadNativeQualification` records 124 passes and one failure across 125 selected CTests (126 registered; the user-excluded restart-flood scenario was not run). Both release/network builds passed. CTest exited 8 after 1878.74 seconds. All 998 frozen source/fixture/binary inputs retained their hashes. Inventory, JUnit, complete logs, source/binary copies and negative evidence controls are indexed; missing/duplicated testcases and an unknown testcase status are rejected by the verifier.

All 18 serial costly-RPC gates pass, including Paris/Shanghai HTTP/WebSocket with sixteen clients. The four client traces independently account for 3840 requests, 768 estimates, no errors, marker finality with all clients active, post-load transfer and four reopened peers per network. Maximum estimate durations are 5430.79 ms (Paris HTTP), 5986.42 ms (Paris WebSocket), 4873.82 ms (Shanghai HTTP) and 5744.30 ms (Shanghai WebSocket). This does not erase the earlier paired-network queue-expiry failure, nor establish a universal throughput guarantee.

The remaining failure is `bft_lagging_paris_network`, at 4.84 seconds: the fixture immediately requires an isolated vote once the majority has advanced four heights. The runtime permits the isolated peer to wait five seconds for a proposal before its nil prevote. Majority progress is not synchronization for that vote; Shanghai/default variants passed. The failed temporary journal is no longer available, so its exact timer state cannot be reconstructed. A bounded wait for the actual height-one vote is prepared; SIGKILL, exact-signature recovery, certified catch-up and renewed quorum participation must remain tested. This campaign is neither a full Ethereum corpus replay nor a production approval.

### Isolated-validator vote readiness before crash recovery

`bftLaggingVoteReadinessQualification` replaces the immediate vote assertion with a bounded ten-second wait for an actual height-one vote. Every poll requires the isolated core/relay to remain alive and the isolated chain to remain at genesis. The test still kills the core, requires byte-identical vote replay before reconnecting, verifies four missed certified heights and the historical transfer, then removes another peer so renewed finality requires the recovered validator. No consensus timeout or runtime implementation changed.

All three profiles (Paris, Shanghai, default NanoExperimental) pass three consecutive executions each: nine complete recovery scenarios in 166.58 seconds. Full logs retain all nine reserved-vote and finality records. One Paris repetition waits 742.58 ms for its vote; the other eight already have a vote at the first poll. This directly exercises the readiness condition omitted by the former test. All eight recorded inputs retain their hashes; Nano is byte-identical to the preceding broad campaign. The archived 124/125 result and original failure are retained, and no new whole-suite pass is claimed. Earlier paired-network RPC queue expirations and the outbound WebSocket byte-budget review remain separate open work.

### WebSocket serialized-response byte budgets

`websocketOutputBudgetQualification` adds 8 MiB per-connection and 64 MiB shared-server serialized-payload ceilings alongside the existing 256-message limit. A bounded stream serializer stops before appending bytes beyond the remaining budget. The in-flight front message stays reserved and its buffer remains alive until the async-write callback; shutdown discards pending tails immediately, and completion/error/destruction releases the remaining reservation. The server closes the connection on output overflow. A response-side close does not undo transactions already submitted.

Native boundary checks cover exact local/shared limits, escaped JSON expansion, oversized serialization cleanup, message count, cross-connection reuse, destructor cleanup, and active-buffer pointer stability while pending output is discarded. Three bounded live clients each stop reading replies and are disconnected after nine accepted 1 MiB-ID requests. Other HTTP/WebSocket clients remain responsive, and the canonical tip/mempool remain unchanged. Nine RPC/HTTP/queue/disconnect/shutdown suites pass in 22.56 seconds.

The new binary also passes both sixteen-client WebSocket network workloads (Paris 90.86 s, Shanghai 90.29 s): 1920 requests, 384 estimates, no request errors, marker finality with clients active, post-load transfer, and four reopened peers per network. The detailed request verifier and thirteen recorded input hashes pass. Sources, binaries, logs and JUnit are indexed. These payload ceilings do not bound pre-serialization JSON, allocation capacity, operating-system buffers, or process RSS. Aggregate limits are tested at the queue level; the live workload is three sequential local slow readers, not proof against distributed saturation. The earlier paired-network queue expirations remain unresolved, and no whole-suite or production approval is claimed.

### Explicit RPC overload and recovery on two simultaneous local networks

`rpcExplicitOverloadQualification` adds a separately named bounded scenario, leaving the existing no-error gates strict. Paris and Shanghai each run four validators, 48 WebSocket clients and 12 rounds per client. Only the exact -32005 capacity/queue-expiry errors are permitted as overload outcomes. Transport timeouts, execution failures, incorrect simulation outputs, and failed identity reads remain errors. Each round makes a control read, and every successfully returned gas quote must execute; if its immediate verification call is refused, that quote is explicitly verified after the load drains. Fresh latest/pending estimates and executions must also succeed on every node without retries.

Both simultaneous scenarios pass with unchanged runtime code: Paris records 1270 requests, 310 capacity refusals and 27 queue expirations; Shanghai records 1275 requests, 306 refusals and 17 expirations. All 1152 identity reads succeed. There are 241 successful in-load estimates and four deferred quote verifications. After load, sixteen fresh estimate/call pairs pass. Finality advances from heights 4 to 71 in Paris and 3 to 71 in Shanghai during observations; the marker is certified with 34/40 clients still active. Subsequent transfers, certified history, canonical state isolation and all eight reopened-node audits pass. Maximum recorded request times are 7189.35/7233.19 ms, below the unchanged ten-second client deadlines.

The historical strict Paris WebSocket scenario separately passes 960 requests and 192 estimates in 91.84 seconds after the fixture refactor. The overload verifier rejects a missing control read, an execution error substituted for an allowed refusal, and a missing recovery/finality result. All thirteen snapshotted inputs retain their hashes, and Nano remains byte-identical to the WebSocket-budget campaign. Sources, binaries and successful/negative logs are indexed.

This qualifies explicit refusal and recovery under this finite local workload. It does not turn the earlier paired no-error failure into a pass, establish unlimited throughput or distributed long-term resilience, bound whole-process memory, or approve production.

### Newly reproduced valid large-block failures (not corrected)

`largeBlockFailureQualification` exposes two internal size-limit problems independently on Paris and Shanghai. Every transaction is signed, below the existing 1 MiB pool limit, funded and admitted. Five transactions with 450000 zero calldata bytes each consume 9105000 of 30000000 block gas. The resulting block passes proposal execution, receives a BFT certificate and passes canonical replay audit, but its 9003887-byte JSON representation exceeds the block-page loop's 8 MiB threshold: `nano_networkBlocks(0)` returns an empty block array even though height one exists. A lagging reader cannot obtain that block through this page.

Five transactions with 900000 zero calldata bytes each consume 18105000 gas, still within the same block limit. The 18003890-byte proposal passes execution validation but cannot reserve its BFT vote: the journal rejects proposals above 16 MiB. The initial two-profile experiment stopped at that earlier failure; its logs and executable are preserved. Expanded tests independently reproduce both failure modes: four failures, zero passes, with candidate/page diagnostics and JUnit. These fixtures use a native single-validator certificate to isolate storage/export behavior; no live multi-node attack or failure recovery has yet been demonstrated.

Transaction JSON includes both `data` and `input`, duplicating calldata; transport, journal and pagination limits consequently disagree with valid aggregate block execution. The relay frame/core-client limits are also 16 MiB, while the native RPC client accepts 8 MiB responses. Simply applying a new HTTP output cap, raising one bound, or treating an empty page as success would not establish correct large-block synchronization. The runtime is unchanged in this step. Block representation, vote recovery and bounded transport need a coherent correction, followed by real large-block finality/catch-up tests. These failures remain open and block a global production-readiness claim.

### Compact signed-transaction block representation: four native failures resolved

`compactBlockTransportQualification` introduces the explicit `transactionEncoding: signed-rlp-v1` tag for internal BFT proposals, new voting-journal proposal entries and exported network blocks. Transactions carry their canonical signed RLP bytes once. Expanded legacy blocks remain readable; external Ethereum JSON stays unchanged. Existing journal proposal bytes are matched against the exact legacy serialization and retained, while subsequent reservation/commit can resume the identical signature. Unknown encoding versions, mixed object/raw payloads, unversioned raw arrays and noncanonical signed bytes are rejected.

Block and event pagination now treats 8 MiB as a batching target. A larger first item can occupy one bounded page below the 16 MiB envelope, with reserved framing margin. This prevents an existing in-range first block or event from producing a permanently empty page. The native RPC client's response ceiling is aligned to the relay's 16 MiB ceiling. Journal storage limits are not removed.

The four Paris/Shanghai regressions that previously failed now complete certified vote, audited execution, block export, advancing event pages and matching follower import/audit. Their old 9003887/18003890-byte expanded blocks produce 4503470/9003471-byte RPC pages. The final eleven targeted suites all pass in 39.60 seconds, including codec/legacy-journal recovery, chain/RPC, block authentication, finality crash and consensus-round checks. Signed bytes for types 0 through 4 roundtrip in the codec; this does not enable types 3/4 on the network.

The initial build exposed incorrect fixture usage of the signature/RocksDB APIs, subsequently corrected. The first targeted campaign had fourteen passes and one codec-fixture failure caused by its synthetic producer address; a second diagnostic isolated checksum-case normalization. Those build/failure reports are retained along with the successful normalized fixture. Four ordinary multi-process BFT recovery scenarios passed before the event-pagination adjustment; those are not evidence of large-block libp2p operation on the final binary.

The four original native failures are resolved within their tested sizes. Compact blocks beyond 16 MiB, combined current/previous large BFT proposals, actual multi-process large-block propagation, and mixed-version upgrades remain open. This is not a complete native/Ethereum replay or a production approval. See `BLOCK-TRANSPORT.md` for format and compatibility boundaries.

### Compact large-block transport over real libp2p (2026-09-24)

Two new Paris/Shanghai gates pass: three of four real local validators certify a five-transaction large block while the fourth remains isolated, then the fourth imports it through libp2p. Compact page size: 9,004,650 bytes. Matching receipts and state plus eight reopened database audits are verified. Evidence is in `build-cmake/qualification/large-block-network-20260924`. No runtime change in this campaign. The previous four native large-block failures remain preserved in their baseline campaign. Above-16-MiB and composite BFT envelope limits remain open; this is not production approval.

### Large BFT round envelope: reproduced and paged (2026-09-24)

Paris and Shanghai regressions first fail with `JSON frame too large`: 18,009,077 bytes after a retained-lock round transition, versus the 16,777,216-byte limit. Failed evidence and executable remain in `large-bft-envelope-20260924`. The relay now requests complete single-round pages through `nano_bftExchangePage`, independently alternating current/previous pages for each peer. Corrected deterministic tests preserve every vote and the locked block, feed both pages through the real framing parser, accept the previous page on a lagging reactor, then certify and audit the retained block at round one. Page sizes are 9,002,809 and 9,006,288 bytes.

Eight targeted suites passed. During that first run the relay was refined to track alternation per peer; those initial network results are supplementary. All four local Paris/Shanghai large-block and lagging/crash scenarios were replayed against the final frozen binaries and passed in 87.17 seconds. Fifteen final inputs are hashed and unchanged. Evidence: `large-bft-pages-20260924`. The aggregate diagnostic RPC can still exceed the frame limit. Single oversized proposals, multi-host endurance, mixed versions and end-to-end delayed large-round transitions remain unqualified.

### Single compact block over 16 MiB: four unresolved boundaries (2026-09-24)

`oversized_block_paris` and `oversized_block_shanghai` both fail. This is an explicitly configured 40,000,000-gas genesis, accepted by Nano, not a claim that the default 30,000,000-gas block already reaches this size. Ten transactions each contain 900,000 zero calldata bytes and individually remain below the pool byte ceiling. The authenticated proposal executes within the gas budget: 36,210,000 gas, 18,003,843 compact JSON bytes.

Four boundaries refuse this valid block: the 16 MiB voting journal proposal value, `nano_networkBlocks` single-page ceiling, `nano_networkEvents` single-event ceiling, and the 16 MiB libp2p JSON frame. The local vote reservation fails without a published vote or canonical block. An independently constructed **fixture certificate**, signed with the synthetic bootstrap key, then isolates the import/storage path: certified import and canonical replay audit succeed. This is not evidence of successful live finality.

The two gates remain red, without expected-failure annotations or relaxed consensus rules. Evidence: `build-cmake/qualification/oversized-block-20260924`, including failed JUnit, full output, source/binary copies and verification identifying all four boundaries. A coherent fragmented transfer and durable proposal representation is still required; raising only one limit cannot fix this lifecycle.

### Oversized proposal journal resolved; transport still red (2026-09-24)

The new `nano/proposal/chunks-v1` journal representation stores large compact proposals in 1 MiB fragments with a manifest and complete Keccak digest. Proposal fragments, reservation, lock and watermark share one synchronous RocksDB batch. Existing inline journals are not rewritten.

Paris/Shanghai large-proposal recovery tests pass: 18 fragments reconstruct the exact proposal, the reserved signature resumes identically, eight corruption/incompleteness cases per profile refuse recovery without advancing the watermark, and finality plus canonical audit succeed. Existing legacy codec and consensus-round checks pass. Initial campaign: six passes and two remaining failures.

The two `oversized_block_*` gates now reach real local journal-backed finality (`externalCertificateFixture: false`); export, block event and libp2p frame still fail, so the gates remain red. Both large-journal SIGKILL campaigns also pass at reservation, canonical commit and recovery boundaries: six large-case abrupt stops, identical signatures, conflicting-vote rejection and a final two-block audit. Two small-journal SIGKILL regressions were replayed with the updated fixture and pass. This tests process death after acknowledged durable phases, not power loss during a disk write.

Evidence: `build-cmake/qualification/chunked-voting-proposal-20260924`; first run 6/8 passed in 43.12 seconds, crash run 4/4 passed in 23.12 seconds. Full reconstruction memory, fragmented network transfer, mixed-version operation and hardware power-loss qualification remain open.

### Bounded HTTP export fragments for oversized blocks (2026-09-24)

Two read-only endpoints now export a canonical block manifest and transaction byte fragments: `nano_networkBlockManifest` and `nano_networkTransactionFragment`. The manifest supplies ordered transaction hashes and remains non-executable; fragments are scoped to the requested block and signed transaction and limited to 256 KiB before hexadecimal encoding. No transfer sessions or partial imports are created.

Paris/Shanghai fixtures each export the actual locally finalized 18,003,843-byte compact block over HTTP in 40 fragments. Serialized reply size is at most 524,569 bytes in the test; manifest size is 3,042 bytes. Exact signed bytes and transaction root are reconstructed, eight invalid queries per profile are refused, and incomplete/reordered/corrupted assemblies are rejected by the existing direct block-import RPC before an intact reconstruction imports and audits. The final import is **in process**, not an oversized HTTP submission.

Four initial native/RPC suites pass in 21.27 seconds; the two fragment suites then pass over real local HTTP in 14.25 seconds. Initial test compilation errors (GMP integer conversion and JSON/string comparison) and the corrected build are preserved. Evidence: `build-cmake/qualification/block-fragment-export-20260924`, with eleven frozen final inputs.

This qualifies HTTP export only. The relay still uses aggregate block/event paths, local-core ingress remains monolithic, and BFT proposal fragmentation is not connected. The previous oversized lifecycle gates therefore remain unresolved. Very large manifests require pagination, and total reconstruction memory plus concurrent transfer budgets still need implementation and qualification.

### Private HTTP fragment ingress and bounded staging (2026-09-24)

`nano_beginBlockImport`, `nano_appendBlockImport`, `nano_finishBlockImport` and `nano_abortBlockImport` now provide a private relay-to-core receiving path. The existing local network token is required; an empty token disables it. Staging admits at most four sessions and 64 MiB of serialized manifest/raw-byte payload with a fixed 60-second lifetime. This caps retained payload, not process RSS, and cannot yet assemble a valid block whose retained payload exceeds that local budget. No consensus size rule was changed.

The budget regression passes byte exhaustion/reuse, session exhaustion, incomplete finish, gaps, wrong identity, conflicting duplicates, duplicate idempotence, expiry without keepalive extension, final digest corruption cleanup and abort. Two actual local HTTP-server campaigns, Paris and Shanghai, transfer 40 source fragments to each of two receiving assemblies. One has a valid certificate and the other an empty certificate. No assembly changes canonical state before finish; the bad certificate fails, the good block imports and audits, and reopening its database preserves the audited history. Authentication, default-disabled behavior, session quota and expiry are also checked.

Five suites pass in 24.65 seconds, including existing RPC and HTTP regressions. Evidence: `build-cmake/qualification/block-fragment-ingress-20260924`, with fourteen frozen inputs. This removes the prior need for a monolithic HTTP import in the fragment test. The existing relay still uses the old paths: libp2p forwarding, block-event references and BFT proposal fragments remain unconnected. Lost-final-ack recovery, partial-assembly restart behavior, very large manifests, larger-than-budget assemblies and sustained transfer load remain unqualified.

### Real managed libp2p catch-up for an oversized certified block (2026-09-24)

Managed relays now connect the fragment export and private core ingress through `/nano/blockparts/1.0.0`. One transfer runs per relay, paced at 350 ms between fragment requests and bounded by a 55-second deadline. Failure aborts staging and applies a peer cooldown. A canonical hash lookup confirms the result after finish, including the intended lost-finish-reply case (fault injection remains pending). Existing peer handshakes preserve active sync/page state instead of resetting it.

Paris and Shanghai each pass a two-managed-node scenario: a block is first certified by the real local journal/consensus path, then both nodes start with the source at height one and the follower at genesis. The old aggregate export is explicitly refused. The follower nevertheless receives all 40 fragments through actual libp2p and private HTTP forwarding, reaches the same block/state roots, matches all ten receipts and applies the recipient balance exactly once. Both managed relay children are reaped, and both databases pass reopened audits. The source block is pre-certified before networking: this is **not** evidence for distributed oversized BFT proposal finality.

Four suites pass in 70.60 seconds, including RPC and managed-supervisor regressions. The fragment network cases take 30.11 and 29.65 seconds. Evidence: `build-cmake/qualification/libp2p-fragment-sync-20260924`, fifteen hashed inputs. Oversized proposal distribution, aggregate event announcements, lost-ack injection, interrupted-transfer recovery, concurrent/adversarial peers, larger/slower valid transfers and mixed-version operation remain open.


### RPC estimation current-binary requalification — 2026-09-24

`rpcEstimateCurrentBinaryQualification` records a fresh rebuild and ten passing serial suites (59.55 seconds): RPC semantics, queue expiry, Paris/Shanghai 10,000-transaction pending pools, costly out-of-gas calls, successful costly estimates and depth-four nested execution. No runtime change was needed.

The unchanged official Anvil 1.5.1 reference passes all 48 same-signed comparisons: types 0/1/2, Paris/Shanghai, latest/pending, depths one/four, and quotes from either client. Both quotes execute on both engines; selected receipt fields, gas consumption, effective fees, transaction hash, storage, sender nonce and balance match. Simulations leave canonical state unchanged. Timestamp and base fee are aligned explicitly; this is not full-header or global state-root equality. All sixteen input hashes remain unchanged. Sources, binary fingerprints, complete logs and exact matrix verification are retained in `build-cmake/qualification/rpc-estimate-refresh-20260924-a`. Earlier strict concurrent-load failures remain preserved; this does not qualify all RPC inputs, all EVM forks, sustained load, or production readiness.


### Fragment transfer interruption and lost commit response — 2026-09-24

`libp2pFragmentFaultQualification` records five passing suites in 77.55 seconds. Paris and Shanghai each inject one actual libp2p stream reset at the second fragment, observe failure after one accepted fragment and restart the complete forty-fragment transfer. The destination then resets the actual local HTTP connection after commit but before its reply. The relay observes the missing reply and confirms the canonical hash instead of blindly importing again. Exactly one failure and one successful import are required. Both profiles compare ten receipts, exact balances, block/state commitments and four total reopened database audits. All sixteen source/binary fingerprints remain unchanged. Complete logs, source snapshots and parsed results are indexed under `libp2p-fragment-faults-20260924`.

This closes the two specified connection-reset cases for local certified-history catch-up only. Process death during assembly, larger/slower valid blocks, oversized BFT proposals, event references, concurrent adversarial peers and multi-host endurance remain open. No production approval follows.


### Unfinalized BFT proposal fragments — 2026-09-24

`bftProposalFragmentExportQualification` adds read-only current/previous proposal manifests and 256 KiB signed-byte fragments. The previous proposal is retained as a structured block instead of a full JSON body; legacy snapshots preserve their votes and contents. The ordinary receiver refuses a hash-only proposal before voting. Unknown identities, unaligned/out-of-range offsets and stale committed-height requests fail.

The final five suites pass in 30.01 seconds: RPC regression, existing round-page scenarios and Paris/Shanghai oversized fragmentation. Each oversized profile reconstructs current and previous envelopes exactly (eighty fragments), checks seven rejection cases, preserves canonical state while serving and finalizes/audits the locked proposal in the next simulated round. Additional direct RPC checks validate selector/offset rejection and exact fragment export. Twelve final source/binary hashes remain unchanged. Initial native tests and the subsequent RPC-fixture phase passed too; a final source adjustment postpones previous-proposal replacement until successful round transition, and the full five-suite set was rebuilt/rerun.

Evidence is indexed under `bft-proposal-fragment-export-20260924`. No live network BFT fragment path is connected yet. Private proposal assembly, live round/retention timing, interrupted/adversarial downloads and distributed oversized finality remain open. The 16 MiB aggregate frame limit is unchanged; production readiness is not established.


### Private BFT fragmented ingress — 2026-09-24

`bftProposalIngressQualification` retains two failed initial profiles, a diagnostic RPC response (`invalid or missing PoS producer signature`), two intermediate fixture failures and their corrections. The initial preflight attempted to verify Nano producer signatures using an empty transaction body even though the signature domain includes the full signed bytes. The fix preserves the signature domain and performs authentication only through the ordinary receiver after assembly. The intermediate assertion compared checksum-cased and normalized addresses; the fixture now normalizes both while still decoding/verifying the vote cryptographically.

Final Paris/Shanghai ingress tests pass in 32.76 seconds; six regressions passed on the same core binary. Only the fixture source/binary changed between those phases, verified by hashes. Fifteen final inputs remain unchanged. Eight unique suites pass across the two final phases; neither earlier eight-suite run is represented as all-green.

Each profile forwards forty fragments to three assemblies, rejects twelve invalid requests, checks shared block/proposal session quotas and purpose isolation, prevents staging or false execution roots from producing votes, obtains a validated local prevote, then commits with signed quorum votes and audits the reopened database. A complete assembly finished after commit is rejected. Unit checks additionally account for retained envelope metadata and expiry cleanup. The HTTP certified-block import regressions pass. BFT ingress itself is tested through native RPC dispatch; live HTTP/libp2p, lost BFT acknowledgement, changing live round deadlines, process interruption and adversarial concurrency remain unqualified. No production claim follows.


### Live local BFT proposal fragments and retry timing — 2026-09-24

`bftFragmentNetworkQualification` preserves an initial Paris failure (106.38 seconds including setup/cleanup): the network remained at height zero through the unchanged 100-second finality gate. Eight transfers stopped after 12–28 fragments as short rounds outlived their proposal context. The transport now uses manifest/vote exchange plus private proposal assembly; the new live test had exposed the old fixed five-second proposal wait as insufficient for paced large transfers.

A round-dependent proposal wait (5/10/20/40/80/160 seconds, capped at 300) is applied both after transition and on round recovery. Invalid durations cannot partially replace coordinator state. Vote-phase waits, quorum rules, signature domains, admission limits, fragment pacing and the test deadline were not relaxed. Coordinator and Paris network tests pass, followed by six RPC/ingress/connection-reset/Shanghai regressions: eight unique suites. Both first network successes were independently checked for full target-block downloads rather than empty later proposals.

The final fixture itself now requires forty fragments at two distinct receivers for the actual certified block. Strict Paris/Shanghai replays pass in 79.35/78.11 seconds, both at certificate round two, with ten matching receipts per peer, exact balances and eight total reopened database audits. Each retains five failed intermediate downloads; these are not hidden. Core and relay binaries are identical to the earlier corrected regressions; only the fixture source/binary changed for the stricter replay. Eighteen final input fingerprints remain unchanged. The verifier rejects missing target transfers and duplicate receiver identities. Logs, all source phases and negative controls are indexed under `bft-fragment-network-20260924`.

Scope: four validator instances share one core process, four HTTP listeners and four real libp2p processes on loopback. This is not independent-process or multi-host qualification. Live BFT fragment faults, partitions, crash recovery, larger/slower assemblies, mixed versions, block-event references and liveness after repeated proposer/signing-journal cycles remain open. No production approval follows.


### Block event references — 2026-09-24

The new reference path initially failed to build because its helper included `nlohmann/json.hpp` instead of the project's vendored `json.hpp`. Both compiler logs are retained. After correcting the include, RPC/events and both fragment-fault regressions passed. Both new network scenarios successfully imported the referenced oversized block, then failed their native event-ring exhaustion setup: 10,000 transactions from one sender exceeded the existing 100-per-account admission limit. The fixture now uses 100 funded accounts with 100 transactions each, preserving runtime admission limits. The original six-suite run remains recorded as four passes and two failures.

The corrected eleven-suite replay passes in 158.74 seconds: both reference profiles and nine queue, full-pool and costly/nested gas-estimation suites. Combined with the four initial passing regressions, fifteen distinct suites pass on identical core/relay binaries. Only the new test fixture and its executable changed after the initial run. All twenty-two final source/binary fingerprints remain unchanged after the replay.

Each reference profile transfers the 18,003,843-byte block in forty fragments, compares ten receipts, audits two reopened databases, paginates fifty-two blocks with a 6,630-byte maximum page, rejects seven malformed references and recovers an expired event ring after 10,000 admissions across 100 accounts. The verifier rejects missing profile evidence, a mismatched received hash and incomplete fragment counts. Evidence is indexed under `block-event-references-20260924`. Two chain instances share one core process and use two real loopback relay processes; periodic synchronization also runs. Full independent-process/multi-host BFT, broader resource/fault behavior and production readiness remain unqualified.


### BFT fragments with independent core processes — 2026-09-24

Paris and Shanghai both pass (81.54 and 78.89 seconds including fixture setup/shutdown). Both certify an 18,003,850-byte compact block at round two; three Paris receivers and two Shanghai receivers download all forty fragments of that exact block. Four/five failed intermediate downloads remain recorded. Ten receipts agree across the four peers in each profile, transfers apply exactly once and all eight reopened databases audit successfully. Twelve captured input fingerprints remain unchanged during the run. CMake regenerated/recompiled KZG and relinked the core archive during setup, so this campaign does not claim byte identity with the preceding archive. The relay binary and compared runtime sources are unchanged.

Evidence: `bft-fragment-processes-20260924`. The verifier rejects duplicate/missing core process identities and duplicate target receiver identities. This closes the shared-process limitation for this one large-proposal lifecycle on loopback. It does not qualify multi-host timing, deployment through the production supervisor, extended proposer cycles, partitions, process crashes during proposal assembly, or larger/slower transfers.


### Returning BFT proposer stalls after a full rotation — 2026-09-24

`bft_proposer_cycle_paris` and `bft_proposer_cycle_shanghai` both fail deterministically with `proposal created outside current round`. Four equal-stake validators are registered by executing the staking contract. The round-zero proposer operates a real reactor and journal; three other validators supply correctly signed nil prevotes/precommits for rounds zero through three. These certificates advance rounds without committing any block. At round four the same proposer is selected again. Its tick reloads its reserved round-zero proposal and the coordinator correctly refuses that proposal as outside the current round.

The reservation key in `ChainStore::reserve_signature` and `signing_proposal` contains chain, genesis, producer and height but no BFT round. `BftReactor::tick` unconditionally resumes that height's proposal. This is a liveness defect, not evidence that conflicting blocks were committed or signatures forged. It contradicts any general claim of recovery after arbitrarily many failed proposer rounds. The passing round-two large-block campaigns did not exercise it.

The regression is intentionally red and the runtime is not corrected yet. Evidence: `bft-proposer-cycle-20260924` (two failed profiles, 1.19 seconds). A correction must scope BFT producer reservations by round, preserve within-round conflict rejection, safely handle existing height-only reservations and prevent an older binary from silently bypassing newer reservations. Non-BFT signing behavior and BFT vote locks must remain protected. Qualification must also cover restart and a later actual quorum commit; removing the current-round proposal check would not be an acceptable fix.


### Returning BFT proposer correction and journal recovery — 2026-09-24

The original two failed profiles remain unchanged in `bft-proposer-cycle-20260924`. BFT producer reservations now include the proposal round under the existing height domain. Repeating the same reservation is idempotent; a different digest in that round is refused. Non-BFT reservations remain height-scoped. Vote signatures, vote locks, current-round proposal checks and quorum thresholds are unchanged.

A legacy height reservation is validated against its retained proposal and recovered only in that proposal's original round. Its digest still forbids a conflicting reservation in that round. A legacy reservation without proposal bytes is ambiguous and refuses migration. New height domains atomically store a marker in the old location alongside their round reservation, so the old height-only write path cannot silently bypass them. These tests exercise the legacy API/storage layout, not a rolling deployment of older binaries. Readers and idempotent writers both refuse missing guard, missing body, wrong digest, altered body and orphaned body records.

Seven initial corrected suites pass in 55.58 seconds. After adding corruption refusal checks, all nine final suites pass in 218.49 seconds: RPC, producer authentication, consensus rounds, two proposal-ingress profiles, two independent-core large-proposal profiles and both proposer-cycle profiles. Each cycle fixture advances through four signed nil quorums, creates a current-round proposal, reopens the reactor with its exact reserved proposal/vote, commits only with a signed quorum at round four and audits the fully reopened chain. Seventeen rejection checks per profile include both reading and re-reserving each of five corrupted journal variants.

Both final large-proposal network regressions retain four independent fixture core processes and four libp2p relay processes, target-fragment evidence and eight reopened audits in total. Twenty final input hashes remain unchanged. Exact logs, source phases, fingerprints and verifier negative controls are indexed under `bft-proposer-cycle-corrected-20260924`. This closes the reproduced proposer-cycle defect, not all BFT liveness, power-loss atomicity of the new journal layout, multi-host behavior, mixed-version operation or production readiness.


### BFT round reservation sync failure and modeled WAL loss — 2026-09-24

`bft_signing_sync_paris` and `bft_signing_sync_shanghai` inject a real macOS `fsync`/`F_FULLFSYNC` EIO against the owned temporary signing-journal WAL, after an observed append. Each profile tests fresh round-aware storage and an existing durable height-only reservation, with both retained and deliberately removed unsynced WAL suffixes. The failed child exits without destructors; loss modeling truncates only the measured append after that child has exited. This is controlled WAL-loss modeling, not a physical power-cut experiment.

All eight new scenarios pass. No producer signature is emitted following the failed sync. After disabling the injector, the errored live store still refuses the same reservation and a conflicting one. Raw recovery checks require digest/body atomicity and, for fresh storage, matching presence of the legacy-path guard. Existing legacy reservation bytes must survive unchanged. Retained round reservations reject conflicts; both dispositions can subsequently reserve durably, sign successfully and recover the same proposal after a second reopen. The height-only writer is refused and the canonical chain remains at genesis throughout: signature recovery does not manufacture a commit.

Four existing sync-failure/retry regressions also pass, for six suites in 14.63 seconds. Eleven snapshotted input fingerprints remain unchanged. No runtime code changed in this campaign. The verifier rejects missing injection, incorrect WAL disposition, missing legacy coverage and an escaped signature. Evidence is indexed in `bft-signing-sync-20260924`. The test-only interposition library is linked only into fault-test executables. Physical power loss, filesystem/controller durability guarantees and other operating systems remain outside this evidence.


### Broad native regression after round-scoped signing — 2026-09-24

All release targets rebuild successfully. A serial selection of 96 tests finishes in 374.60 seconds: 94 pass and two fail. The exact selected/excluded inventory is archived; 71 live-network, relay/supervisor or gossip scenarios are outside this pass, including the user-excluded flood. This is not a full CTest pass and not a replay of the complete official Ethereum corpus.

The only failures are `oversized_block_paris` and `oversized_block_shanghai`. Both still produce and audit the valid 18,003,843-byte block through a real local voting journal, then fail the three legacy aggregate stages: `nano_networkBlocks`, `nano_networkEvents` and the 16 MiB JSON frame. These failures remain failures. Separate fragment/reference paths pass their selected native tests, but do not make the old standalone aggregate path support that block.

Passing coverage includes native EVM and embedded official regressions, ERC20/OpenZeppelin isolation and fee scenarios, wallet/CLI/staking, RPC shutdown/disconnect/queue/load/pending estimates, storage write/sync failures, crash recovery, block/proposal assembly, round coordination and the new proposer-cycle/sync-failure regressions. There are no additional failures within this selection. Exact case identities match the selection, with no skipped or duplicated tests. Both dedicated BFT evidence verifiers pass against the full CTest log.

The source archive contains 204 source/configuration files. All 275 captured source/binary fingerprints remain unchanged after the tests. Build context, full logs, JUnit, known-failure payloads and coverage verification are indexed under `post-signing-native-20260924`. No runtime code changed in this campaign. Broader network and full-corpus replay remain separate gates; production readiness remains unproven.


### Full supported Ethereum corpus after BFT signing changes — 2026-09-24

The newly rebuilt release state/block runners replay all selected fixtures in pinned `tests@v20.0.0`: the Berlin, Paris, Shanghai, Cancun, Prague and Osaka groups plus the four existing block transition profiles. Before execution, all 7,603 state files and 7,972 block files are checked byte-for-byte against archive SHA-256 `b183702a5b447b465873865357ced9eb342315922e52e31b714e2de115dd0bb4`; the selected archive file sets exactly equal the replay inventories. Inputs use read-only staging aliases and reports retain their precise file/case/fork/variant identities.

All 44,244 state cases match, including 2,893 expected validation rejections. All 52,870 block cases match, with 60,257 accepted blocks and 4,940 rejected blocks. There are zero mismatches, unsupported cases or errors within this scope. Independent coverage verifiers check exact identities, expected rejection categories, state/log commitments, available receipt gas/hash fields, accepted/rejected block counts and final state/block-hash results. Every selected case is present exactly once. Eight directly captured replay inputs and all 275 source/binary fingerprints from the rebuilt native campaign remain unchanged.

The immutable evidence bundle is `post-signing-full-evm-artifacts-20260924`; controller execution and input aliases remain under `post-signing-full-evm-20260924`. This is a current-runner full replay for the six supported groups and four transitions, not all fork groups in the upstream archive, full Ethereum consensus/network validation or production approval. It does not activate diagnostic modern forks on the public Nano network. No fresh Anvil comparison was run in this campaign, and the two legacy aggregate-transport failures remain open.


### Current-binary Anvil and Geth refresh after BFT signing changes — 2026-09-24

The unchanged official Anvil 1.5.1 reference passes all 48 Paris/Shanghai same-signed nested-execution comparisons: transaction types 0/1/2, latest/pending, depths one/four and quotes from either provider. Both providers' estimates execute on both engines. Receipts, consumed gas, effective fees, transaction hashes, selected storage, sender nonce and balance match; simulations preserve canonical state. Timestamps and base fees are aligned explicitly. This does not compare complete block headers or global state roots.

The official, unpatched Anvil 1.8.3 reference yields 202 matches and 13 differences across the current 215-case Cancun/Prague/Osaka matrix. The script now defaults to five account-query scenarios, adding `fresh-no-proof` and `unqueried` to the historical three. All 129 overlapping case identities retain identical status and Nano/Anvil gas results, including the original seven differences. The 86 additional cases expose six further differences in the same calldata-floor/authorization categories; all remain recorded as mismatches and the comparison exits nonzero.

A new independent sequential Geth 1.16.7 replay matches Nano on all 215 cases across fifteen sequences. Each engine starts from the same explicit synthetic nonempty-account state and then consumes only its own preceding full poststate. The verifier confirms identical signed bytes, receipt gas and hashes, complete supplied poststate and computed roots, and equality between these Nano gas results and the corresponding Anvil-diagnostic Nano results. This supports Nano execution for these inputs; it is not proof of equality with Anvil's global prestate/trie.

The refreshed Shanghai Hardhat campaign also preserves exactly its historical 27 case results: 21 gas matches, six gas differences. Both clients now reject the three 49,153-byte initcode transactions (types 0/1/2), preserve canonical state and execute the same signed recovery transaction at the same nonce. The historical oversized-initcode admission disagreement is not reproduced with official Anvil 1.8.3. The overall Hardhat comparison still exits nonzero for the six gas differences.

Thirteen captured source, artifact and executable fingerprints remain unchanged. No runtime or reference implementation was modified. Exact logs, signed inputs, both clients' outputs, Geth sequence evidence, reference versions and coverage checks are indexed under `post-signing-anvil-20260924`. All launched local clients are stopped by the scripts. Full RPC/fork parity, network activation of modern forks, the legacy aggregate transport limits and production readiness remain separate open gates.

### Independent initcode gas boundaries across Shanghai–Osaka — 2026-09-24

A new offline matrix executes 60 signed creations through the current Nano runner and pinned Geth 1.16.7: Shanghai, Cancun, Prague and Osaka; transaction types 0/1/2; initcode lengths 31, 32, 33, 49,151 and 49,152 bytes. Twelve sequences evolve each engine's own state independently. All receipt gas/status/hash/log fields, complete supplied poststates and roots match. An independent verifier decodes each signed input, checks exact matrix coverage and derives intrinsic gas plus the four gas consumed by two PUSH0 instructions. The EIP-7623 floor applies only to Prague/Osaka.

At the two large boundaries, both clients consume 252716/252720 gas before Prague and 512600/512610 from Prague onward. This independently supports the previously diagnosed explanation of the six official Anvil 1.8.3 Shanghai differences. Those official-reference failures remain recorded; this does not replace them with a patched oracle or change Nano gas rules.

The initial generator incorrectly retained a template header hash after changing its synthetic context; Nano refused it with `header hash mismatch`. That failed run and generator are retained. The corrected generator omits the stale optional hash and strips Cancun-only header fields for Shanghai. These are execution contexts, not a header/consensus qualification. Missing-case and false-match negative controls fail as expected. Evidence is indexed under `initcode-gas-forks-20260924`; binary hashes match the prior campaign. No runtime code changed. RPC estimation/admission, modern network activation and global production readiness remain outside this new test.

### ERC20Votes / TimelockController exposes context-sensitive estimation — 2026-09-24

A new OpenZeppelin 5.0.2 governance-primitives integration first fails at compilation because the fixture omits an explicit EIP712 import. After that fixture-only correction, the default ethers estimation path twice produces a mined out-of-gas ERC20Votes transfer: gas limit and consumption are both 67083. The immediately preceding delegation writes a checkpoint at the current block number. An explicit latest estimate can price updating that checkpoint, while inclusion at a later block creates another checkpoint. The explicit pending quote is 89173 and the transfer succeeds at that budget. The failure is retained, not classified as a passing malicious-call check.

After switching vote operations to explicit pending estimates, a later timelock transaction also fails with the default quote: consumption is 30763. Tracing all successful calls identifies cancellation as another context-sensitive path: latest quote 30763, pending quote 30801, successful pending-budget receipt consumption 26001 after refund. Timelock state classification depends on the block timestamp. These results expose a developer integration limitation: a successful latest-context estimate is not necessarily enough for future inclusion. They do not demonstrate incorrect historical execution or compromised consensus. Default estimation policy and comparison with other clients remain open; no engine/RPC behavior was changed to hide the failures.

The final explicit-pending lifecycle passes on Paris and Shanghai. It checks signed delegation, replay/expiry refusal, historical vote/supply checkpoints, shared-delegate transfers, timelock access control, payload tampering, delayed execution, cancellation and duplicate-execution refusal. Sixteen deliberately reverted transactions have persisted receipts, no logs, consumed sender nonces and gas consumption below their supplied limits. Eighteen successful calls execute with their selected pending quotes. A too-early eth_call is pinned to the scheduling block to avoid racing the live validator clock. Target state ends at 42, historical votes remain unchanged, and retrieved blocks carry BFT votes.

Evidence: `oz-governance-primitives-20260924`, including all initial failures, source phases, artifacts, both final profiles and ten checked final input fingerprints. This qualifies ERC20Votes and TimelockController primitives on local Nano, not the complete Governor proposal/voting lifecycle, production governance configuration, a multi-validator deployment or all OpenZeppelin contracts. No runtime code changed. The use of pending in this fixture is explicit and does not close the default-wallet estimation issue.

### Default gas-estimation context corrected and reference checked — 2026-09-24

Official unmodified Anvil 1.5.1 and 1.8.3 are probed independently on Paris and Shanghai with the new ERC20Votes artifact. For each profile the deployment and initial delegation are followed by three snapshot-isolated transfer branches. Both references quote 89173 gas when the block argument is omitted or `pending`, and 67083 for explicit `latest`. All eight default/pending branches succeed, while all four latest-budget inclusion branches fail out of gas with preserved token balances and a consumed transaction nonce. The reference script records signed bytes and receipts; it is an Anvil-only context probe, not global Nano/Anvil chain-state equivalence.

Nano previously selected `latest` when the block argument was omitted. `Rpc::prepare_simulation` now uses the existing isolated pending snapshot for an omitted `eth_estimateGas` block. Explicit `latest`, numbered/historical blocks and the `eth_call` default retain their previous context. There is no arbitrary gas padding, opcode change or removed gas cap. Pending execution remains under the same queue, cancellation and shared request deadline.

The new native number-dependent storage regression fails before the fix and passes afterward. The final native executable passes 991 checks, including default/explicit context separation, pending pool storage effects and canonical-state/nonce/pool preservation. Existing price-dependent tests now explicitly request the genesis/latest context used by their execution oracle. Diagnostic modern-fork tests likewise explicitly select their supplied historical context: their private revision override does not activate modern pending-header construction, and the intermediate `missing Cancun header fields` failure remains archived. These changes preserve the intended diagnostic checks instead of manufacturing network activation.

Both Hardhat governance profiles pass with `NANO_GOVERNANCE_GAS_TAG=default`: ethers obtains gas itself and the script supplies no gas override for the eighteen successful quoted calls. Sixteen intentional access/replay/expiry failures still have persisted reverted receipts. Ten additional suites pass in 54.12 seconds: queue expiry, shutdown, RPC disconnect, wallet WebSocket disconnect, both 10000-transaction pending-pool profiles, both costly-success estimates and both depth-four nested estimates. No flood is run.

Evidence indices: `votes-estimate-reference-20260924` and `rpc-default-pending-20260924`. Initial failures and fixture-context adjustments are retained. This closes the reproduced omitted-block context defect for these paths; a pending quote cannot guarantee success after arbitrary later state changes, reordering or different timestamps. It does not qualify every wallet, full Governor governance, modern-fork public activation or production readiness.

### Same-signed default-estimate ERC20Votes replay — 2026-09-24

The current Nano binary and official Anvil 1.5.1 pass all eighteen transfer comparisons: Paris/Shanghai, transaction types 0/1/2, and budgets supplied by Nano's default quote, Anvil's default quote or explicit latest. The twelve default-budget branches succeed on both engines. Six deliberately latest-budget branches run out of gas identically, preserving token/vote balances while consuming the sender nonce and native fees. Signed hashes, receipt status/type/gas/effective price/logs, selected native/token balances, delegates and vote totals agree. Snapshot restoration is checked between branches. Deployments and initial delegations also use identical signed bytes.

Initial comparator runs fail on checksum-versus-lowercase address formatting, then on Anvil's first-block base-fee initialization. Both failures are retained. The final comparator compares address bytes, controls timestamps and explicitly sets Anvil's next base fee to Nano's. Nano's fee is checked independently against its parent using the EIP-1559 formula. This is explicit execution-context alignment, not full header/global state-root equivalence or production BFT qualification. Local development mining is used.

Official Anvil 1.8.3 passes all nine Shanghai transfer branches with the same checks. Its Paris run stops at the first deployment: Nano consumes 1792011 gas and Anvil 1792639. The 10039-byte initcode spans 314 words; the 628-gas difference exactly equals Shanghai's initcode word charge. A separate same-signed replay from common supplied state matches Nano and Geth 1.16.7 completely (poststate and computed root) on both Paris and Shanghai, yielding 1792011 and 1792639 respectively. Thus Anvil's Paris result matches the Shanghai cost for this input. Attribution to the previously diagnosed stale reference gas table is an inference here; no new source intervention was performed and the official Paris mismatch remains a failure.

The Geth sequential adapter now omits withdrawals for pre-Shanghai Paris/Berlin contexts instead of always supplying an empty withdrawal list; the new Paris/Shanghai replay verifies this execution path. No Nano runtime rule changed. Evidence is indexed under `votes-signed-default-20260924`, including raw signed inputs, per-step receipts/headers, failed runs, source phases and exact final inputs. The independent result verifier checks all case identities, signed transaction hashes, quote selection and fee accounting; missing-case and forged-hash negative controls fail. This strengthens the prior default-pending fix and does not close full Governor behavior, all RPC parity, modern-fork network activation or global readiness.

### OpenZeppelin Governor lifecycle, snapshots and quorum — 2026-09-24

The new `DeveloperGovernor` fixture composes official OpenZeppelin 5.0.2 GovernorVotes, GovernorCountingSimple and GovernorTimelockControl. The local token supply is one million, proposal threshold 100000 and quorum 500000; these are fixture settings, not Nano consensus parameters. Timelock proposer/executor/canceller roles are transferred to the Governor, the original owner's roles are revoked and its admin role is renounced. Timelock self-administration remains the standard OpenZeppelin mechanism.

Both Paris and Shanghai pass two complete proposal outcomes. The first proposal reaches Pending, Active, Succeeded, Queued and Executed and changes the protected target to 73 through the timelock. Voting weight uses the snapshot: after the owner votes with one million, transferring 750000 tokens and delegating the recipient leaves counted votes at one million. The recipient can cast a vote but has zero historical weight. Both historical checkpoints remain unchanged. A second proposal receives no votes, becomes Defeated and cannot be queued.

Twelve mined rejection checks cover below-threshold proposing, duplicate voting, queuing without quorum, execution before queueing, direct timelock bypass and repeated execution. Each has a persisted failed receipt, no logs, consumed sender nonce and gas below its supplied limit. Early voting and early timed execution are additionally rejected by eth_call pinned to the relevant historical block, avoiding wall-clock races. Successful calls use ethers' default estimation without gas overrides. Normal local signed transactions advance block clocks; no public chain or funds are used.

Evidence: `oz-governor-lifecycle-20260924`. Fourteen captured source/configuration/artifact/binary inputs remain unchanged, including the full Solidity compiler input for this fixture. The report verifier checks both profiles, lifecycle identities, receipt coverage, snapshot ordering and vote totals; missing-profile, post-snapshot extra-weight and quorum-bypass negative controls fail. No Nano runtime changes or qualification failures occurred in this campaign. It covers this Governor composition and these outcomes, not every extension, signed Governor ballots, cancellation/concurrent proposal races, governance upgrades, multi-validator operation or global production readiness.

### BFT finality under omitted-block estimate load — 2026-09-24

The new separately named `bft_rpc_default_overload_paris_network` and `bft_rpc_default_overload_shanghai_network` gates run four local validators/relays with 48 WebSocket clients, each bounded to twelve rounds. One estimate mode omits the block argument entirely, exercising the corrected default-pending path; another explicitly selects pending. Calls still exercise latest and pending. Returned quotes must execute, including quotes whose immediate verification call encounters admission refusal. Existing strict no-error gates are not relaxed.

Both networks pass, run sequentially on the same host: Paris 97.59 seconds, Shanghai 95.40 seconds. Across 2533 traced requests, all 1152 identity reads succeed. Only 657 exact capacity refusals and 46 exact queue-wait expirations occur; no transport, execution or control failures are admitted by this gate. Of 288 omitted-block estimate attempts, 113 return successful quotes. All 229 returned estimates across both modes are checked immediately or through nine deferred verifications. Twenty-four fresh recovery estimate/call pairs include omitted-block estimates on every node.

A signed marker is certified while 33/32 load workers remain active. Observed heights advance from 3 to 60 and 4 to 59 during the respective workloads. During-load and post-load transfer receipts agree across peers; both commit certificates are cryptographically verified against the validator set. Recipient funds apply exactly once, RPC trial writes leave canonical contract storage untouched, and all eight reopened databases have matching persisted snapshots and verified histories.

The test driver and CMake cases were extended; no core runtime source changed. Binaries were rebuilt, and fifteen captured inputs remain unchanged during the campaign. The updated verifier also accepts both historical explicit-tag overload reports. Negative controls replacing an omitted tag, substituting an execution-error code and removing a default recovery probe all fail. Logs, request traces, JUnit, summaries and source inputs are indexed under `bft-default-estimate-load-20260924`.

This is finite local overload/recovery evidence for the default-estimate correction. It does not establish multi-host endurance, host-wide CPU/memory exhaustion protection, a public latency SLA or resolution of the earlier separately retained simultaneous-network strict no-error failure. No sustained flood or public network was exercised; global production readiness remains open.

### Current-binary BFT partition, crash and membership recovery — 2026-09-24

Eight scoped cases pass on the same current binaries as the default-estimate load campaign: Paris/Shanghai partition plus SIGKILL restart, finite transaction load with one crashed validator, stake-weight change followed by SIGKILL, and an exactly-two-thirds voting partition. The initial campaign was interrupted after the complete Paris restart result and during the following load-crash case. On resume, its process handle was absent and the process inventory confirmed no campaign processes remained. The completed result is preserved; the incomplete load-crash attempt is not counted as a pass. Seven remaining cases then complete with separate CTest logs, JUnit and process exit records.

The restart cases observe no finality in the two isolated halves, abruptly kill all four cores and require each restarted reactor to reproduce its exact reserved vote before the network reconnects and finalizes a common block. Weighted cases observe no finality with exactly four of six stake units for twelve seconds after an actual pair handshake; a five-of-six signed quorum subsequently finalizes and peers reconnect. These are bounded observed network behaviors, not an exhaustive proof over all schedules.

Both load-crash cases complete 360 signed transfers. A fresh transfer finalizes on the surviving three validators after confirmed death of the fourth and while senders remain active. The restarted peer rejoins before all transfers have been submitted; final receipts, balances, commitments and reopened histories agree. Across both profiles, 720 transfers are verified without loss or duplication.

Stake-change cases execute the staking contract to move weights from [1,1,1,1] to [4,1,1,1], verify the appropriate old/new certificates, heal the weighted partition and confirm four SIGKILL exits. Restarting the original databases/configuration/journals preserves the new weights and finalizes another transfer. Eight vote journals are then recovered to their certified chain tips plus one without altering canonical state, with the new validator-set hash. Load and stake cases account for sixteen distinct peer histories reopened and audited; stake cases additionally audit again after the final restart.

All 102 captured inputs remain unchanged. The exact selected case set matches the observed outcomes; the seven resumed JUnit cases contain no failure/error/skip. Verifier controls reject a missing case, a journal left at the tip rather than tip plus one, and reverted stake weights. Sources, initial partial logs, per-case resumed logs and results are indexed under `bft-recovery-refresh-20260924`. No runtime changes or completed-case failures occurred. Physical power loss, multi-host timing/endurance, larger proposal-assembly crash cases and global production readiness remain outside these results.

### Assemblage BFT interrompu : défauts du nouveau test puis six cas réussis (2026-09-24)

Campagne `bft-assembly-crash-20260924`, manifeste `bftAssemblyCrashQualification`.
Le premier essai échoue sur une hypothèse du test : avant initialisation
paresseuse du round, `nano_bftManifest` peut renvoyer null. Le test accepte
maintenant ce résultat vide et exige toujours l'absence de proposition/vote.
Le second essai échoue sur le solde : la fixture réutilisée transférait zéro,
alors que le nouveau contrôle attendait dix. Les transactions du seul mode
crash transfèrent maintenant une unité chacune. Les deux journaux rouges et
les sources intermédiaires sont conservés ; aucun correctif runtime.

Le verdict final couvre quatre suites : six points SIGKILL sur Paris/Shanghai
et deux régressions d'import, tous réussis. Les 82 empreintes d'entrée restent
inchangées pendant le verdict final. Un vérificateur exige les six identités
et les suites réussies ; ses contrôles négatifs refusent un point absent,
un point dupliqué et un vote fantôme déclaré. Il vérifie la couverture du
rapport, pas indépendamment les calculs de consensus. Voir FINALITY.md pour
les limites de transport et de déploiement encore ouvertes.

### Reprise après SIGKILL pendant téléchargement libp2p (2026-09-24)

Campagne `bft-download-crash-20260924` : deux tests locaux Paris/Shanghai,
quatre validateurs chacun, réussis dès le premier lancement (171,84 secondes).
Le destinataire et son relais meurent après un fragment effectivement reçu
et acquitté par le cœur. La reprise conserve les chemins de stockage/journal
et l'identité réseau, refuse l'ancienne session puis rejoint la chaîne
certifiée commune. Deux audits avant commit et huit après réouverture passent.
102 empreintes de sources/configurations/binaires sont inchangées pendant
l'exécution. Le vérificateur refuse un profil absent, un signal de panne
substitué et une session périmée déclarée acceptée. Les téléchargements
intermédiaires qui échouent lors des changements de round restent visibles
dans le journal complet ; ils ne sont pas comptés comme réussis.

La campagne utilise de vrais relais cpp-libp2p mais des cœurs fixtures : le
superviseur de production et les autres limites listées dans FINALITY.md
restent ouverts. Aucun défaut consensus n'a été corrigé dans cette campagne.

### Relais orphelin après SIGKILL de nano run : corrigé (2026-09-24)

`supervisor-parent-crash-20260924` conserve l'échec initial (relais encore vivant
après six secondes), les deux sources runtime avant correction et le verdict
final des quatre suites. `src/node.cpp` transmet le PID du parent en écrasant
la valeur héritée ; `network/main.cpp` refuse un parent périmé au démarrage et
surveille le parent direct pendant son exécution. Aucun mécanisme de consensus
ni résultat d'exécution EVM n'est changé.

Le nouveau test passe, ainsi que le superviseur existant et les deux transferts
fragmentés avec pannes Paris/Shanghai. 104 empreintes restent inchangées pendant
le verdict final. Les contrôles négatifs du vérificateur rejettent un scénario
absent et un relais déclaré survivant. Le contrôle d'intégrité/rapport complète
les assertions natives, sans être une réexécution indépendante du protocole.
Voir FINALITY.md pour les limites, notamment la cadence liée à la boucle et le
scénario BFT distribué sous superviseur encore ouvert.

### Scénario BFT distribué sous superviseur : deux profils réussis (2026-09-24)

La campagne `bft-managed-large-crash-20260924` conserve le premier appel de
build sans régénération CMake et les deux erreurs de comparaison JSON/string
du nouveau test, puis le build corrigé et les deux exécutions réussies.
Paris/Shanghai passent en 222,80 secondes au total avec admission RPC, SIGKILL,
arrêt automatique du relais orphelin et reprise sans réinjection de mempool.
103 empreintes restent inchangées pendant les essais. Le vérificateur exige
les deux profils et refuse un profil absent, une réinjection déclarée ou un
nombre nul de nœuds gérés. Les assertions natives vérifient réellement les
reçus, soldes et audits ; le vérificateur ne réexécute pas le consensus.

### Première mesure de passage à l'échelle des votes (2026-09-24)

`vote-scaling-20260924` conserve le premier échec du nouveau benchmark (adresse
du signataire omise), corrigé avant la mesure de référence. Le premier build
de l'optimisation échoue sur l'inférence des types Json dans une fonction
modèle ; des types Json explicites corrigent la compilation. Ces deux échecs
ne sont pas des succès de qualification et restent archivés.

La référence valide et la version optimisée couvrent les mêmes cinq tailles,
jusqu'à 1 000 membres. Cinq suites finales passent : benchmark/invariants,
finality, consensus_round et deux réseaux stake-change/SIGKILL. 90 empreintes
restent inchangées pendant les réseaux. Le vérificateur refuse une taille
manquante et l'absence déclarée des contrôles de votes invalides. Les gains
observés sont locaux au collecteur ; les signatures ne sont pas court-circuitées,
le plafond n'est pas relevé et le débit réseau n'est pas qualifié par ce test.

### Compteurs de participation : référence et régressions (2026-09-24)

`vote-participation-20260924` conserve les mesures et le modèle de référence
avant modification, puis six suites finales réussies. Aucun échec de cette
campagne n'a été écarté. Les 92 empreintes d'entrée sont inchangées pendant les
vérifications finales. Le vérificateur exige les cinq tailles, les 776 contrôles
du modèle et les deux réseaux de seuil strict ; ses contrôles négatifs refusent
un modèle absent et la suppression déclarée des rejets de signatures invalides.
Le contrôle des rapports ne remplace pas les assertions natives. Cette
optimisation porte uniquement sur la participation par étape, pas sur la
preuve qu'un bloc dispose d'un quorum ni sur la bande passante de diffusion.

### Votes différentiels et réparation après panne (2026-09-24)

`bft-vote-delta-20260924` conserve cinq suites réussies : suivi de connaissance
BFT, progression gossip, frames JSON et deux réseaux `nano run` avec grande
proposition, panne et reprise. Les 107 empreintes restent inchangées pendant
les réseaux. Le vérificateur réutilise la vérification des blocs/reçus/audits
et exige des réductions observées dans les traces ; ses contrôles négatifs
refusent une réduction nulle et un délai de réémission altéré.

Les votes omis sont connus du pair ou acquittés par une réponse de même
contexte ; ils sont régulièrement renvoyés en entier. Les tests unitaires
couvrent notamment réponse perdue, contexte différent, votes différents,
éviction d'un contexte, remplacement de session et limites de mémoire.
Les signatures et quorums restent vérifiés dans le cœur. Les métriques sont
échantillonnées et les limites de FINALITY.md restent ouvertes.

### Enveloppe IPC Altair à la limite de bloc (2026-09-25)

Échec observé dans `consensus_altair_runtime` et `consensus_altair_managed_runtime`
pendant l'activation explicite de l'admission P2P : un payload de 8 Mio devenait
une requête JSON-RPC supérieure à 16 Mio avec hex et enveloppes. Le refus avant
ouverture du Store était correct ; le transport IPC ne couvrait pas le profil.

Correction : objet IPC avec `sszBase64` canonique, validation de longueur/padding/
bits inutilisés avant allocation, identité de ticket commune aux encodages hex
et compact. Le format P2P reste hex. Le profil de 8 Mio et la limite HTTP de 16 Mio
restent inchangés. Les deux suites passent maintenant, ainsi que le transfert réel
de 8 Mio via libp2p/HTTP (SSZ invalide rejeté par le cœur) et les blocs officiels.
Les profils plus grands restent conditionnés à la capacité IPC ; fragmentation
encore ouverte. Preuves initiales et finales dans
`build-cmake/qualification/altair-libp2p-ingress-20260925/`.

### Tâche attester sur bloc du créneau courant (2026-09-25)

Pendant la nouvelle intégration locale des tâches Altair, le test positif sur
un bloc reçu au slot d'attestation a échoué avec `incoherent Altair epoch/slot
state`. Le préparateur appelait `prepare_altair_slots` avec un état déjà au slot
cible, alors que cette primitive exige une avancée strictement positive.

Correction : réutiliser l'état authentifié lorsque le bloc est déjà au slot
cible, avancer uniquement les créneaux manquants. Le test construit maintenant
explicitement une tâche valide pour la nouvelle tête, y compris avant le délai
de secours lorsqu'un bloc courant est présent. Il exige ensuite que le refus
de signer une seconde tête provienne précisément du journal anti-slashing ; une
erreur générique de préparation ne peut plus faire réussir ce contrôle.

Preuves : `build-cmake/qualification/altair-attester-duty-20260925/current-slot-failure.log`
pour l'échec, `duty-final.log` pour le scénario corrigé. Le reste du périmètre
(planification autonome, propositions, binding d'exécution et finalité réseau)
reste ouvert.


## 2026-09-25 — recul d'horloge pendant une tâche d'attestation

Le contrôle vérifiait le créneau et la borne basse du Store, sans comparer les
lectures successives. Le test `altair-duty-time-20260925/before-fix.log` échoue
avec `rejected duty reached signer or mutated journal` : une horloge qui recule
de 1 ms dans le même slot peut encore atteindre le fournisseur.

Correction : observations non décroissantes pendant une tentative et contrôle
final après la vérification BLS du journal, avec annulation. La réservation est
conservée si le résultat doit être retenu après sa création. La campagne
`build-cmake/qualification/altair-duty-time-20260925/tests.log` passe 4/4 suites,
dont quatre injections après réservation avec réouverture du journal et refus
d'un vote conflictuel. La politique d'horloge entre opérations et après longue
absence reste à implémenter avec le planificateur autonome.
