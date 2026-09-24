# Objectif : Nano C++ de niveau production

Décision utilisateur du 21 septembre 2026 : le portage doit corriger ce qui
n'était pas adapté à la production dans le Python. La référence Python reste
intacte. La parité avec ses défauts n'est plus un critère de réussite.
Le prototype ne doit plus imposer de dépendance d’exécution ni de pilote de
tests Python à Nano ; les tests du projet sont natifs C++.

## Décision prioritaire du 24 septembre 2026 — validateurs

La cible demandée devient une architecture inspirée du consensus Ethereum :
registre ouvert sous conditions de staking, comités tournants par slots/epochs,
signatures BLS agrégées avec signataires identifiables, règles de finalité et
choix de branche explicites. Cette décision remplace les propositions antérieures
qui conservaient exclusivement le BFT à votes individuels comme cible finale.
Elle ne vaut pas validation ni implémentation de cette architecture.

Avant activation : spécifier et versionner les domaines cryptographiques,
l'aléa et la sélection des comités, le calcul des poids, les transitions du
registre, les sorties et sanctions, le choix de branche, la finalité et les
conditions de synchronisation sûre. Tester les partitions, équivoques,
agrégats forgés, doubles comptages, reprises et changements d'epoch.
Réutiliser les spécifications et primitives éprouvées ; ajouter des groupes
au BFT existant ne suffit pas à reproduire la sécurité d'Ethereum.

Les tests de votes individuels à 100 000 et un million de validateurs mesurent
l'implémentation actuelle. Ils ne qualifient ni le futur protocole ni un réseau
distribué de cette taille. Le lancement public reste conditionné aux preuves
techniques et aux paramètres opérationnels de déploiement.

## Cible retenue

Un réseau public Nano en C++, avec EVM native, cpp-libp2p, RocksDB et staking
par transactions signées vers un smart contract. Les nouvelles règles corrigées
priment sur la reproduction exacte des historiques Python divergents. Toute
rupture de format ou de règles doit être versionnée et documentée ; les données
existantes ne doivent jamais être réécrites silencieusement.

## Critères de clôture

1. Consensus et blocs authentifiés : signature du producteur, séparation des
   domaines de signature par chaîne et version, contrôle de sélection, rejet
   du rejeu et des altérations. Définir et vérifier les règles de concurrence
   de branches, de finalité et de sanctions applicables au réseau public.
2. Exécution déterministe : règles EVM et gaz définies, rejeu identique sur les
   pairs, invariants de soldes et de dépôts, annulation atomique des échecs.
3. Staking : inscriptions, ajouts et retraits signés, règles de verrouillage et
   de sortie explicites, protections adversariales et revue de sécurité.
4. Persistance : atomicité, récupération après arrêt brutal, détection des
   corruptions, sauvegarde/restauration et évolution de schéma vérifiées.
5. Réseau et RPC : contrôle des entrées, limites de ressources, résistance aux
   pairs malveillants, synchronisation vérifiée, séparation des clés et des
   interfaces publiques, comportement opérationnel documenté.
6. Livraison : compilation reproductible, dépendances et licences recensées,
   tests fonctionnels et adversariaux, essais multi-nœuds prolongés, procédures
   d'exploitation et limitations connues. Toute validation externe nécessaire
   mais non obtenue reste explicitement un point ouvert.

7. Entrée et sortie des actifs : bridge bidirectionnel qualifié vers au moins
   une chaîne choisie, avec identification des chaînes et contrats, protection
   contre le rejeu, règles de finalité et modèle de confiance explicites.
   Vérifier la conservation de la couverture des actifs représentés, les retraits,
   les doublons, les pannes de relais, les réorganisations admises par le modèle
   et la procédure de reprise. Le bridge et la pool de liquidité sont distincts :
   traverser le bridge ne fournit pas automatiquement de liquidité à une pool.
   La cible retenue est Base–Nano avec USDC ; les contrats sources exacts, le mécanisme de vérification et son modèle de confiance restent à qualifier.
8. Pool de lancement : permettre l’échange de NANO (ou de sa représentation
   bridgée) contre un actif de contrepartie choisi. Définir le réseau hôte et
   le DEX, puis vérifier l’ajout/retrait de liquidité, les swaps, les décimales,
   les autorisations et les limites de slippage. Vérifier le parcours complet
   bridge vers pool puis retour, sans émission non couverte ni double retrait.
   La liquidité initiale et le prix initial nécessitent des montants décidés
   explicitement ; aucun apport de fonds réel n’est autorisé par le développement.

## Positionnement de lancement

Décision utilisateur : bridge bidirectionnel et pool de lancement. Le produit
ne cible pas un coffre-fort ou une fonction de gestion de trésorerie. Aucun
contrat de coffre supplémentaire n’est requis par ce positionnement.
La cible de bridge retenue est Base–Nano avec USDC. Le réseau hôte et la configuration définitive de la pool restent à arrêter.
Le bridge facilite le passage entre réseaux ; seuls les dépôts de liquidité
alimentent la pool. La présence du bridge ne garantit ni demande ni cotation
sur un service tiers.

Des suites vertes ne suffisent pas à déclarer le réseau prêt pour la production.
Aucun déploiement public ni engagement financier n'est autorisé implicitement
par cet objectif de développement.

## Priorité immédiate

Terminer la qualification d’exécution EVM à partir des fixtures Ethereum
épinglées et de transactions signées identiques comparées à des clients de
référence. Conserver les divergences officielles même lorsqu’une variante
diagnostique locale en explique la cause. Poursuivre ensuite la qualification
BFT, reprise, stockage, RPC et réseau ; vérifier les parcours Solidity/pool avant
le développement du bridge. Voir `EVM-CONFORMANCE.md` pour le bilan récent.

L’audit externe payant et l’industrialisation sont reportés à la demande de
l’utilisateur ; leur report ne constitue pas une preuve de préparation
production. Aucun déploiement public ni usage de fonds réels n’est autorisé.

### Étape antérieure : authentification des producteurs

Authentifier les blocs PoS et vérifier le rejet des blocs falsifiés sur le RPC,
l'import, le stockage et le réseau. Le format C++ versionné et les producteurs signés sont maintenant intégrés
(voir `BLOCK-SIGNATURES.md`). Les genesis livrés activent cette règle ; leurs versions historiques sont
conservées séparément (voir `GENESIS-VERSIONS.md`). La protection locale contre la double signature est
implémentée avec les limites décrites ci-dessous.

## Audit de stockage

La commande `nano audit` rejoue la chaîne canonique depuis le genesis dans une
base temporaire et compare états, registres de validateurs, reçus et index.
Les tests couvrent une preuve historique corrompue et un reçu altéré. Voir
`STORAGE-AUDIT.md` pour le périmètre exact ; cela ne remplace ni la finalité,
ni les essais de perte d’alimentation, ni une revue indépendante.
Le test du validateur distant couvre désormais un SIGKILL après réservation
et signature, avant réception du bloc : la reprise conserve le même bloc et
la même preuve. Les pannes disque et pertes d’alimentation restent à éprouver.

### Qualification OpenZeppelin demandée

Couvrir tout le catalogue OpenZeppelin, y compris ERC721/ERC1155 et leurs
extensions, contrôle d’accès, cryptographie, gouvernance, proxies et compositions
de marché. La première version figée est Contracts v5.0.2. Le suivi par composant
est décrit dans `OPENZEPPELIN-QUALIFICATION.md` ; compilation et quelques scénarios
réussis ne constituent pas une validation complète. Le paquet séparé
Contracts Upgradeable doit également être inventorié avant d’en revendiquer la
couverture. Les contrats propres aux marketplaces, au bridge et à l’AMM demandent
des tests de composition et de sécurité supplémentaires.

### Point ouvert RPC / gouvernance ajouté le 2026-09-24

Le nouveau parcours OpenZeppelin ERC20Votes/TimelockController révèle des transactions échouant avec le gaz estimé sur le contexte `latest`, puis réussissant avec une estimation explicite `pending`. Les coûts varient avec les checkpoints de numéro de bloc et l’état temporel du timelock. La campagne `oz-governance-primitives-20260924` valide les primitives sur Paris/Shanghai avec pending, conserve tous les échecs et ne modifie pas le moteur. Il reste à comparer ce comportement aux clients de référence, définir puis qualifier la politique d’estimation par défaut pour les wallets, sans altérer la sémantique d’un bloc explicitement demandé. Le cycle complet de Governor reste distinct et non qualifié par ce test.

### Correction du défaut d’estimation omise — 2026-09-24

Le point précédent est résolu pour le défaut de sélection du contexte : comme Anvil 1.5.1/1.8.3, Nano sélectionne désormais `pending` quand `eth_estimateGas` ne reçoit aucun bloc. Les profils de gouvernance passent avec ethers sans override de gaz, avec 991 vérifications RPC et dix suites complémentaires réussies. Les blocs explicitement demandés et le défaut `eth_call` sont préservés. Les devis sensibles à une évolution ultérieure de l’état, le cycle complet de Governor et la qualification globale de production restent distincts. Preuves et limites : `rpcDefaultPendingQualification` dans le manifeste Ethereum et les deux indices associés.

### Cycle Governor : périmètre local démontré le 2026-09-24

Le cycle principal GovernorVotes/CountingSimple/TimelockControl est désormais testé de bout en bout sur Paris et Shanghai, avec votes historiques après transfert, proposition sans quorum et refus des doubles votes/exécutions. Le manifeste `ozGovernorLifecycleQualification` conserve les preuves. Restent notamment les autres extensions, votes Governor signés, annulation/concurrence, mises à niveau et configuration réelle du réseau ; ce résultat ne clôt pas la préparation production globale.

### Finalisation après correction RPC : charge locale vérifiée (2026-09-24)

`bftDefaultEstimateLoadQualification` ajoute deux réseaux locaux successifs Paris/Shanghai avec quatre validateurs, 48 clients et douze tours bornés. La finalisation reste active, les devis sans bloc demandé sont effectivement exercés, les refus de capacité restent explicites, les RPC récupèrent et huit bases rouvertes passent leur audit. Cela vérifie le nouveau défaut pending sous cette charge ; les limites multi-hôtes, d’endurance et des précédents scénarios stricts restent ouvertes.

### Reprise BFT avec les binaires actuels (2026-09-24)

Le manifeste `bftRecoveryRefreshQualification` référence huit cas réussis : partitions/redémarrages, panne sous charge, changement de poids suivi d’arrêts forcés et seuil strict des deux tiers, sur Paris et Shanghai. Les tests vérifient 720 transferts, seize historiques de pairs et huit reprises de journaux de vote. Ils complètent les preuves après correction du journal de signature et des estimations RPC ; les limites de coupure électrique, réseau multi-hôte, endurance et crash pendant assemblage de grandes propositions restent ouvertes.

### Contrainte de lancement précisée le 2026-09-24

Le budget personnel annoncé est de 200 € au total pour la liquidité initiale et
les frais du bridge/pool. Réserver les frais avant de déterminer la somme
convertie en USDC ; aucun montant de frais n'est encore chiffré. Dans un modèle
lock-and-mint, les USDC bloqués sur la chaîne source garantissent les jetons
bridgés placés dans la pool Nano : ces deux positions ne sont pas deux réserves
indépendantes à financer. Cette contrainte ne vaut ni autorisation d'engager
les fonds, ni preuve de sécurité du bridge, ni financement de l'exploitation
continue. Le modèle de confiance et le parcours de retrait restent à qualifier
avant toute ouverture aux dépôts externes.

### Assemblage BFT interrompu : premier périmètre qualifié (2026-09-24)

Six cas SIGKILL après 1/20/40 fragments reçus sur Paris/Shanghai passent avec
reprise sans vote fantôme, réassemblage intégral, quorum signé, dix reçus et
solde exact par cas, puis audit après réouverture. Deux régressions d'import
passent également. `bftAssemblyCrashQualification` conserve les preuves et
les premiers échecs du nouveau test. Le crash pendant transfert libp2p réel
et sous le superviseur de production reste à tester ; cette campagne ne ferme
pas la qualification réseau/multi-hôte ou de coupure électrique.

### Interruption du téléchargement BFT réel : deux réseaux validés (2026-09-24)

`bftDownloadCrashQualification` complète le test d'assemblage par une panne
observée du cœur et du relais après réception du premier fragment libp2p,
sur Paris et Shanghai. La reprise et l'accord des quatre pairs, les reçus,
les soldes exacts et les bases rouvertes passent les vérifications. Restent
le même parcours sous `nano run`, les autres points d'interruption, les essais
multi-hôtes, l'endurance et la coupure électrique. Le goal reste ouvert.

### Défaut de cycle de vie du superviseur corrigé (2026-09-24)

La préparation du scénario BFT sous `nano run` a révélé puis reproduit un relais
survivant à SIGKILL de son cœur. Le relais surveille maintenant son parent
direct ; les quatre suites de supervision et transferts fragmentés avec pannes
passent. Le manifeste `supervisorParentCrashQualification` conserve le rouge
initial et les preuves finales. Le prochain parcours reste l'interruption
d'une grande proposition BFT distribuée sous le superviseur réel : la correction
du relais orphelin en est un préalable, pas son équivalent.

### Grande proposition sous le superviseur réel : périmètre acquis (2026-09-24)

`bftManagedLargeCrashQualification` valide sur Paris/Shanghai quatre `nano run`,
l'admission RPC des transactions, la panne après le premier fragment et la
reprise réseau sans réinjection manuelle de mempool. Le bloc commun dépasse
16 Mio et les audits/reçus/soldes concordent. Restent les autres points de panne,
les essais multi-hôtes, l'endurance, la coupure électrique et les autres limites
déjà documentées. Le goal n'est pas achevé.

### Passage à l'échelle des validateurs demandé (2026-09-24)

L'utilisateur remet en question le plafond de 1 000 validateurs. Il est présent
à la fois dans NanoStaking.sol, finality.cpp et storage.cpp. Le plafond ne doit
pas être supprimé isolément : mesurer le CPU, les octets de votes rediffusés,
la mémoire et la finalisation quand le nombre de votants augmente ; optimiser
les recherches et validations répétées, puis examiner un protocole de diffusion
plus efficace et ses propriétés de sécurité/vivacité. Distinguer nombre total
de nœuds, nombre de voisins directs et ensemble des validateurs votants. Les
réseaux à quatre validateurs qualifiés ici ne prouvent pas la capacité à 1 000.

### Première optimisation mesurée des validateurs (2026-09-24)

Le collecteur réutilise désormais son ensemble votant immuable au lieu de le
normaliser et le hacher à chaque vote. La mesure locale à 1 000 membres passe
d'environ 1,69 s à 0,27 s pour la réception/participation ; toutes les signatures
restent contrôlées. Cinq suites finales passent, dont les changements de stake
avec redémarrage sur Paris/Shanghai. `voteCollectorScalingQualification` garde
les preuves et limites. Restent le coût externe du réacteur, les parcours de
quorum, la diffusion réseau et des mesures à l'échelle du réseau avant toute
révision cohérente du plafond de 1 000.

### Deuxième coût supprimé : rescans de participation (2026-09-24)

Le contrôle de participation utilise maintenant des poids exacts maintenus
pour les premiers votes validés, séparément par étape. Le modèle de référence
avant/après (776 contrôles) et six suites finales passent, dont les cas réseau
à exactement deux tiers. `voteParticipationQualification` conserve les preuves
et timings locaux. Les certificats, contrôles externes du réacteur et le gossip
restent à traiter ; le plafond de 1 000 et le statut de production sont inchangés.

### Première réduction réelle du trafic de votes (2026-09-24)

Le relais BFT géré filtre par pair les votes déjà connus/acquittés, avec deux
contextes bornés, réémission complète toutes les cinq secondes et remplacement
de l'état lors du changement de session. Les cinq suites de
`bftVoteDeltaQualification` passent, dont les grandes propositions et reprises
sous nano run sur Paris/Shanghai. Des réductions sont observées dans les traces.
Ce résultat ne suffit pas à qualifier 1 000 validateurs ou un coût global
linéaire ; les coûts restants du cœur et les essais réseau à plus grande
échelle demeurent nécessaires avant révision du plafond.

### Encodage des votes et limites restantes (2026-09-24)

Le réacteur conserve désormais les messages canoniques des premiers votes
validés et indexe leurs slots par étape/adresse. Les lectures ne refont plus
l'encodage cryptographique ; les contrôles entrants restent inchangés. Six
suites locales passent : finalité, consensus, lecture répétée et cycle de
proposeur sur Paris/Shanghai. Doublons, votes invalides, équivoques, transition
de round, reprise et disparition des pages après commit sont contrôlés.
Preuves : `bftSnapshotCacheQualification`. La mesure inclut le recalcul coûteux
des hashes de transactions de la proposition ; elle ne prouve pas une capacité
réseau à grande échelle.

La cible utilisateur est une architecture sans plafond arbitraire de 1 000
validateurs. Cette limite demeure actuellement dans le contrat, le décodage
du staking, les ensembles de vote persistés et les certificats RPC. La lecture
complète du staking impose aussi 5 millions de gas par appel et 50 millions
pour la synchronisation, avec copie d'état par appel et recherche linéaire
des enregistrements précédents. Supprimer la constante seule ne supprime donc
pas le plafond pratique. Le gossip doit également traiter des pages bornées
sans tronquer l'ensemble éligible.

Les frais nécessitent une distinction : la base fee suit la formule dynamique
EIP-1559 ; `eth_maxPriorityFeePerGas` recommande encore 1 Gwei fixe et
`eth_gasPrice` ajoute ce même montant à la base fee courante. Cette heuristique
RPC doit évoluer vers une observation de l'activité, avec comportement
explicite à vide. Elle n'impose pas ce pourboire aux transactions signées.

## Liste EIP intégrée au goal — 24 septembre 2026

Le périmètre demandé comprend les EIP 7251, 7002, 7685, 7917, 8045, 8068,
8363, 7805, 7594, 7732 et 8141, ainsi que l’identification des propositions
post-quantiques pertinentes. Voir [la liste et les critères de preuve](VALIDATOR-EIP-ROADMAP.md).
Ces éléments sont des travaux requis ; cette liste ne vaut pas implémentation.

## Compléments EIP recherchés — 24 septembre 2026

Le goal inclut les deux listes : [validateurs](VALIDATOR-EIP-ROADMAP.md) et
[EVM/production](EVM-PRODUCTION-EIP-ROADMAP.md). Auditer l’existant et combler
les écarts ; une entrée dans ces listes n’est ni une activation ni une preuve
de conformité. Les dépendances du fork complet font partie du périmètre.

## Suivi prioritaire de réalisation

La [matrice EIP](EIP-IMPLEMENTATION-STATUS.md) distingue implémenté, partiel et
non commencé, avec périmètre, preuves, priorité et travail restant. Les deux
listes de sources restent des annexes. Clôturer les écarts, sans repartir de
zéro ni considérer une liste comme une implémentation.

Complément Cancun/Prague : suivre 7516, 2537, 2935, 7623, 7691, 7840 et
le périmètre de 7642 dans la matrice ; provisionnement des contrats système,
SSZ consensus distinct de RLP transactions, et eth_createAccessList à qualifier.
