# Objectif : Nano C++ de niveau production

## Objectif prioritaire fixé le 29 septembre 2026

**Livrer et démontrer le parcours complet de sortie d'un validateur sur le
réseau Nano : soumission signée → propagation → inclusion → finalité → retrait.**
Ce jalon fait partie de l'objectif production existant ; il ne remplace aucun
critère EVM, consensus, exploitation, bridge ou lancement économique.

État de départ vérifié : `c1606fd`. La sélection locale et la file durable des
sorties sont implémentées et qualifiées sur des scénarios locaux. Le parcours
RPC/P2P, la finalisation et le retrait complet ne sont pas encore démontrés.

### Critères de clôture du jalon

1. Soumission via RPC/service réseau avec authentification de chaîne, clé/index,
   domaine BLS et éligibilité ; rejets invalides sans mutation et ressources bornées.
2. Diffusion par le vrai P2P vers d'autres nœuds, inclusion automatique,
   idempotence et reprise des envois après déconnexion.
3. Conservation de la demande avant finalité, réinclusion si son bloc est
   réorganisé, absence d'application au mauvais validateur après réindexation.
4. Finalité commune de la sortie ; retrait exact vers les credentials autorisés,
   conservation comptable, aucun double paiement ni signature après inéligibilité.
5. Reprise cohérente après arrêt brutal ou erreur d'écriture, avec vérification
   du registre, de la file et de l'état EVM ; élagage sans perte d'opération utile.
6. Preuve de bout en bout Mac/VPS avec sources et binaires identifiés, puis
   régression pertinente sur cette même version. Conserver les échecs initiaux,
   résultats, limites et commandes de reproduction dans la matrice EIP.

Les sous-étapes ne valent pas clôture du jalon. Aucun délai ou pourcentage
arbitraire annoncé, aucun fonds réel ni déploiement public autorisé par ce jalon.
La qualification globale de production reste nécessaire après sa réussite.


## Mise à jour de l'objectif — 27 septembre 2026

**Statut : actif, production non démontrée.** Le périmètre complet de l'objectif
est conservé. La [matrice EIP](EIP-IMPLEMENTATION-STATUS.md), la
[feuille de route validateurs](VALIDATOR-EIP-ROADMAP.md) et celle de
[l'exécution](EVM-PRODUCTION-EIP-ROADMAP.md) restent les références de suivi.
Une preuve ciblée ferme seulement le scénario qu'elle couvre.

### Acquis récents vérifiables

- Résolution automatique des validateurs par clé publique ; contrôle de la
  paire clé/index avant signature et refus des index périmés après reorg.
- Deux branches à dépôts inversés : réindexation 64→65, activation à l'époque
  13, neuf attestations et finalité commune époque 20 conservée après réouverture.
  [Preuves](qualification/capella-reindexed-activation-20260927/README.md).
- Panne WAL au vote de bascule : état ancien ou nouveau cohérent, fermeture des
  accès incertains et rejeu qualifiés sur macOS, Linux ARM64 et désormais sur le
  VPS Ubuntu x86-64 (99,87 s). [Preuve VPS](qualification/vps-deposit-reorg-sync-20260927/README.md).
- Recherche de clés dans des états SSZ de 100 000 et un million de validateurs,
  avec cache lié à la racine authentifiée. Ces mesures ne représentent pas des
  réseaux de cette taille. [Preuves et coût restant](qualification/capella-registration-ssz-scale-20260927/README.md).

### Travail restant, dans l'ordre de priorité

1. Achever le cycle des validateurs intégré : entrées, activation, sorties,
   retraits, sanctions, réorganisations et reprise. Étendre les preuves aux
   points d'interruption non couverts et aux témoins indépendants ; contrôler
   les signatures et la finalité après reprise, pas seulement le redémarrage.
2. Qualifier la finalité durable via le vrai transport entre Mac et VPS :
   partitions asymétriques, rotation des comités, reconnexion et charge bornée.
   Mesurer mémoire, disque, latence et coûts de registre ; traiter les limites
   implicites sans les masquer par un plafond arbitrairement augmenté.
3. Fermer les écarts du profil EVM/réseau retenu et de ses dépendances. Séparer
   forks activés, runners de conformité et propositions expérimentales ; conserver
   les preuves Ethereum/Anvil/Geth, RPC, OpenZeppelin et Uniswap. Les succès EEST
   ciblés ne prouvent pas l'activation réseau complète de Cancun/Prague.
4. Exécuter une qualification de version candidate cohérente, avec empreintes
   des mêmes sources/binaires, régression pertinente, pannes et ressources,
   sauvegarde/restauration et procédures d'exploitation. Conserver tous les
   échecs, exclusions et limites ; aucune somme de petits runs ne vaut une
   campagne complète non exécutée. La revue indépendante reste un point ouvert.
5. Finaliser le lancement Base–Nano : utilité et relation du token Base avec
   l'actif natif, couverture USDC, confiance et finalité du bridge, retraits,
   allocation et migration de liquidité/fair launch. Qualifier le parcours
   complet avant une ouverture de vente. Aucun référencement tiers garanti.

Les travaux de bridge et de lancement restent dans l'objectif, sans fonds réels
ni déploiement public implicites. Le budget de préparation de 200 euros n'est
pas une autorisation de dépense. Le code, les preuves et l'historique doivent
être préservés ; push sur main après revue et qualification autorisé.


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

## Priorité historique — remplacée par la mise à jour du 27 septembre

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


### 2026-09-26 — Kurtosis native Beacon qualification

Two real Docker Nano nodes, 32 synthetic validators each, connected and finalized
execution block 15 with the same hash. Evidence: `qualification/kurtosis-20260926/summary.json`.
The initial harness needed RPC-loopback and assertion corrections; the corrected
finality checks passed on the same live nodes. This qualifies a bounded native
Bellatrix/Paris scenario, not ethereum-package/Lighthouse interoperability.
Engine capability RPC returns -32601. Priorities: container restart/partition,
bounded load, accurate Beacon supervision, and completing the selected fork
lifecycle. Assertoor and chaos scenarios remain unqualified by this campaign.

La suite des campagnes et leurs critères de clôture sont suivis dans
`SYSTEM-TEST-PLAN.md` : crash atomique, partitions/quorum, charge bornée,
invariants rejoués sur Nano et intégration Assertoor conditionnée aux API réelles.

### 2026-09-26 — Reprise Docker Beacon sous écritures

Campagne bornée réussie : SIGKILL (137), reprise du même volume avec témoins,
24 transactions SSTORE, reçus réussis, nonce/compteur exacts et finalité commune
au bloc 45. Preuves : `qualification/kurtosis-crash-20260926/`. Aucun test de
coupure électrique ni de frontière exacte d'écriture. Limites mesurées :
~236 s après réouverture avant clôture de finalité ; ~4,9 Go sur un nœud.
Analyser l'amplification de stockage et la latence avant toute conclusion
opérationnelle. Assertoor/Foundry/Chaos Mesh restent des campagnes distinctes.

### Correction d'allocation et comparaison qualifiée (2026-09-26)

`allow_fallocate=false` dans ChainStore supprime la réservation anticipée,
sans désactiver WAL ni sync. Les trois tests ciblés de durabilité passent ;
le rejeu Kurtosis SIGKILL/24 transactions passe aussi (bloc 39 commun).
Empreinte finale : ~114/51 Mio au lieu de ~4,9 Go par nœud. Reprise RPC
~3,2 s dans les deux essais ; finalité après reprise 188 s versus 236 s,
sans attribution causale à partir d'un seul essai. Preuves :
`qualification/rocksdb-allocation-20260926/README.md`. Prochaine priorité :
interfaces Beacon pour Assertoor, décrites dans `ASSERTOOR-INTEGRATION.md`.
Le pruning et la maîtrise de l'amplification d'écriture restent ouverts.

- 2026-09-26 : lectures Beacon head/root, fork et checkpoints intégrées au runtime
  et testées sur HTTP réel avec refus d'accès et indisponibilité. Détails et
  limites : `docs/qualification/beacon-reads-20260926/README.md`. Raccordement
  Assertoor toujours partiel, aucune clôture de la qualification système.

- 2026-09-26 : NodeSyncing natif implémenté avec observations P2P de slots distinctes
  des hauteurs EVM, expiration et refus des annonces futures. Deux processus et
  démons P2P testés avant/après redémarrage ; preuves dans
  `docs/qualification/beacon-sync-20260926/README.md`. Qualification Assertoor complète
  toujours ouverte (notamment corps de blocs et événements).

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

## Échec ouvert : finalité avec Assertoor — 2026-09-26

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

- 2026-09-26 — Reprise prolongée : **SIGKILL distribué réussi** (325,89 s),
  finalité commune 2→3→4→5. **Partition prolongée en échec** (521,06 s) :
  sûreté conservée pendant 96 s d’isolement et convergence ultérieure, mais
  retard de finalité supérieur à quatre époques au début du contrôle prolongé.
  Le succès du redémarrage ordonné ne clôt donc pas la reprise après partition.
  [Preuves, critères et limites](qualification/recovery-sustained-20260926/README.md).

- 2026-09-26 — Contre-épreuve après partition : un relais par salves bornées
  améliore la convergence initiale dans cet essai mais échoue au contrôle
  prolongé (501,40 s, finalité immobile deux époques). Hypothèse insuffisante,
  patch expérimental retiré. Prochaine analyse : attestations incluses et
  checkpoints source/cible après reconnexion.
  [Preuves](qualification/recovery-sustained-20260926/README.md#contre-épreuve-du-délai-de-relais).

- 2026-09-26 — Diagnostic hors ligne ajouté (`--attestation-report`) et vérifié
  sur les deux bases arrêtées : rapports identiques, participation utile divisée
  entre branches après partition. Les votes inclus avec une mauvaise cible sont
  distingués des votes utiles. Prochaine correction à cibler : convergence des
  branches / inclusion des attestations, sans modifier quorum ni slashing.
  [Détails](qualification/recovery-sustained-20260926/README.md#diagnostic-des-attestations-incluses).

- 2026-09-26 — Cache borné des acquittements de votes par session ajouté :
  tests de réessai/perte de réponse/session/éviction réussis, retransmissions
  répétées réduites dans le scénario réel. La finalité après partition reste
  en échec (503,39 s). Ne pas assimiler cette correction de transport à une
  résolution du choix de branche ou à la préparation production.
  [Preuves](qualification/recovery-sustained-20260926/README.md#acquittements-de-votes-mémorisés-par-session).

- 2026-09-26 — Nouveau témoin déterministe de partition à livraison directe :
  réussi en 122,04 s, six avancées communes 6→12 après deux époques d’isolement.
  Deux cœurs durables, BLS/EVM/RocksDB réels, horloge simulée et aucun P2P.
  Le contraste avec l’échec réseau cible désormais l’ordre et les délais de
  livraison/rattrapage ; le critère réseau n’est ni remplacé ni assoupli.
  [Preuves](qualification/recovery-sustained-20260926/README.md#témoin-à-livraison-directe-et-horloge-contrôlée).

- 2026-09-26 — Observabilité des délais du validateur ajoutée. Le témoin
  instrumenté passe (134,92 s), sans cycle de six secondes. Une copie arrêtée
  reprend ensuite avec horloge réelle et passe de la finalité 6 à 9 sans pair
  pendant une courte observation. L’échec réseau caractérise un délai excessif
  selon le critère strict ; il ne prouve pas un blocage définitif. Critère intact.
  [Mesures](qualification/recovery-sustained-20260926/README.md#mesure-des-échéances-du-validateur).

- 2026-09-26 — Coalescence des observations d’horloge sans changement de phase
  ajoutée : équivalence d’état et protections d’horloge testées, quatre cibles
  réussies (29,38 s). Reprise courte : 33 publications contre 167, même finalité.
  La partition prolongée échoue en 519,24 s : retard de finalité supérieur
  à quatre époques. Aucun cycle validateur ne dépasse six secondes ; la réduction
  des écritures ne clôt pas le défaut de convergence/finalité.
  [Preuves](qualification/recovery-sustained-20260926/README.md#coalescence-des-observations-dhorloge-bellatrix).

- 2026-09-26 — Diagnostic borné des votes locaux ajouté, inactif par défaut.
  Test dédié réussi (8,56 s) : votes réellement admis, éviction, désactivation
  et accord des deux cœurs. Partition instrumentée en échec (522,42 s) : saut d’époque finalisée.
  Les traces révèlent une stagnation temporaire des messages distants ;
  correction en cours du parcours des votes déjà acquittés.
  Ce diagnostic ne ferme pas le défaut de finalité.
  [Preuves](qualification/recovery-sustained-20260926/duty-trace/README.md).

- 2026-09-26 — Parcours asynchrone borné des votes déjà acquittés ajouté.
  Le test atteint un vote nouveau derrière 70 connus en trois tentatives ;
  perte de réponse et réessai conservés (0,32 s). Relais P2P reconstruit,
  partition en échec (373,12 s), suspicion de course entre lectures RPC.
  La fraîcheur des votes distants progresse bien sur les traces.
  [Preuves](qualification/recovery-sustained-20260926/cached-scan/README.md).

- 2026-09-26 — Course du moniteur reproduite puis corrigée : deux lectures
  de tête encadrent le checkpoint, trois tentatives maximum. Conflits et reculs
  restent fatals pendant une course. Neuf tests passent, seuils inchangés ;
  qualification réseau relancée avec cette mesure corrigée.
  [Preuves](qualification/recovery-sustained-20260926/consistent-sampling/README.md).

- 2026-09-26 — La lecture cohérente seule ne résout pas l’échec (373,31 s).
  Diagnostic prouvé sur le checkpoint arrêté : blocs 62–63, finalité réalisée 2
  mais non réalisée 6 ; bloc 64, réalisée 6. Le scénario exige désormais
  la concordance Beacon réalisé / exécution avant de démarrer sa fenêtre,
  dans le même délai de cinq époques. Neuf tests du moniteur passent ;
  qualification réseau en cours. Les résultats précédents restent des échecs.
  [Preuves](qualification/recovery-sustained-20260926/recovery-bound/README.md).

- 2026-09-26 — **Partition prolongée réussie, 517,99 s** avec le relais corrigé
  et le seuil de reprise lié aux deux projections : coupure 96 s, reprise 95 s,
  trois finalités communes 6→7→8→9, mêmes blocs d’exécution et arrêt propre.
  Deux cœurs / deux P2P, 64 clés synthétiques, un Mac. Clôture de ce scénario
  local uniquement ; les échecs historiques et autres critères production restent
  conservés. Prochaine validation : arrêt brutal avec les binaires actuels.
  [Preuves](qualification/recovery-sustained-20260926/recovery-bound/README.md).

- 2026-09-26 — **Reprise SIGKILL et finalité continue réussies, 325,58 s** :
  deux cœurs / deux P2P, 64 clés synthétiques sur un Mac ; trois avancées
  communes 2→3→4→5 après redémarrage, liées aux blocs d’exécution finalisés.
  Empreintes des binaires inchangées depuis la partition réussie et vérifiées.
  Ce scénario ne couvre pas une panne électrique ni la rétention longue.
  [Preuves](qualification/recovery-sustained-20260926/crash-current/README.md).

- 2026-09-26 — Primitive d’archive atomique de checkpoint implémentée et testée :
  deux cibles passent en 1,49 s, avec réorganisation, SIGKILL et suffixe WAL
  conservé/perdu. **Raccordement Bellatrix/P2P et élagage non réalisés.**
  [Preuves](qualification/checkpoint-archive-20260926/README.md).

- Critère bridge ajouté : avant tout mint, vérifier l’inclusion du dépôt dans
  l’ascendance d’un checkpoint source finalisé selon le protocole choisi.
  Un seuil arbitraire de 12–64 blocs n’est pas une preuve de finalité.
  Tester dépôt réorganisé avant finalité (aucun mint), rejeu (un seul mint),
  finalité bloquée (attente), source RPC mensongère et conflit de finalité
  (arrêt de nouvelles émissions, incident explicite). Une réorganisation
  exceptionnelle après mint ne s’annule pas automatiquement sur Nano : plan
  de traitement des engagements et modèle de confiance à définir avant fonds.
  Références : https://ethereum.org/developers/docs/consensus-mechanisms/pos/faqs
  et https://docs.base.org/base-chain/specs/protocol/consensus/derivation.

## 2026-09-27 — Sélection locale des sorties Capella

Le constructeur peut désormais sélectionner des sorties volontaires candidates
par transition complète, avec budgets et annulation. Nouveau scénario : bloc
indépendant identique, quatre candidats invalides/doublons exclus, deux reorgs
et réouvertures. Test 4,42 s ; régression BLS 4,87 s. Échec initial du banc conservé.
La file persistante, RPC/P2P et le cycle jusqu'au retrait restent à raccorder ;
aucune clôture EIP/production. [Preuves](qualification/capella-local-exits-20260927/README.md).

## 2026-09-27 — File durable de sorties volontaires Capella

Admission authentifiée par clé/index et état de branche, format durable v4
avec revalidation à la reprise, doublons idempotents et sélection automatique.
Nouveau scénario : admission sans modifier le consensus, capacité pleine,
réouverture et deux reorgs rendant la sortie applicable puis non applicable.
3/3 tests passent en 13,98 s ; nouveau test 4,72 s. Le retrait après finalité et
le réancrage à l'élagage sont codés mais restent à qualifier ; RPC/P2P, pannes
WAL, réindexation de dépôts et retrait complet restent ouverts.
[Preuves et limites](qualification/capella-queued-exits-20260927/README.md).
