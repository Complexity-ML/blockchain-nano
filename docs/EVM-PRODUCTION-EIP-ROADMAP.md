# EIP pour l’EVM et la production Nano

## Sanctions multi-validateurs et pannes d’écriture — 30 septembre 2026

La [finalisation partielle des preuves](qualification/capella-slashing-multi-20260929/README.md)
est qualifiée sur Mac, Docker ARM64 et VPS x86-64, avec la régression mono-auteur.
Le [nouveau test de panne WAL](qualification/capella-slashing-sync-20260930/README.md)
passe sur Mac (137,75 s), ARM64 (119,86 s) et VPS (447,12 s) : huit cas par
plateforme, retrait individuel puis agrégé, WAL principal/témoin, suffixe
conservé/perdu, reprise exacte et rejeu idempotent. Les 24 cas sont vérifiés et
archivés ; époque finalisée 8 sur les trois plateformes. Aucun code du cœur
n’a changé. Les validateurs absents du
registre finalisé et les autres critères de production restent ouverts.

## Sanctions après réorganisation qualifiées — 29 septembre 2026

Le [nouveau scénario](qualification/capella-slashing-reorg-20260929/README.md)
passe sur Mac (5,14 s), Docker ARM64 (3,99 s) et VPS x86-64 (16,87 s) : deux
changements de branche signés, preuve conservée puis réincluse automatiquement,
sept réouvertures avec état exact et absence de double pénalité. La régression
du scénario partagé passe sur les trois plateformes. L’échec initial du test
(horloge non avancée) est conservé ; aucun code du cœur n’a changé.
Le critère borné mono-auteur est fermé. Multi-auteurs, finalisation partielle et
échecs d’écriture restent ouverts, ainsi que l’objectif global de production.

## Rotation WAN qualifiée avec audit corrigé — 29 septembre 2026

La [campagne de rotation](qualification/capella-wan-rotation-20260929/README.md)
traverse le slot 128 sous partition : coupure 97,460 s, reprise commune en
131,132 s puis finalité **18 → 19 → 20 → 21**. Les trois bases rouvertes donnent
le même état et le nouveau comité ; **19 blocs finalisés** portent des agrégats
BLS valides avec contributions distantes. L'échec du premier auditeur, qui
refusait les agrégats vides canoniques, reste conservé ; son correctif est validé
sur les mêmes données Mac/ARM64/VPS, sans modifier le cœur ni rejouer le réseau.
Ce scénario borné ferme l'écart de rotation WAN correspondant. Ressources
échantillonnées et limites sont documentées ; sanctions/cycle complet, charge,
exploitation, EVM/RPC, bridge et revue indépendante restent ouverts.

## Rotation WAN : nouveau banc préparé — 29 septembre 2026

Le [collecteur et l’auditeur de rotation](qualification/capella-wan-rotation-harness-20260929/README.md)
sont raccordés au scénario Mac/Docker/VPS : partition traversant le slot 128,
capture SSZ avant élagage et vérification des agrégats après reprise sur les
bases rouvertes. Compilation native, 13 tests nouveaux du collecteur et contrôle
négatif d’une ancienne base passés. **Campagne WAN et chemin positif de l’audit
encore à exécuter** ; aucun statut EIP ni critère de production fermé par ce seul
outillage. Le cœur reste inchangé.

## WAN asymétrique qualifié avec lecture immuable — 29 septembre 2026

La [campagne sur `940736f`](qualification/capella-wan-pinned-20260929/README.md)
passe sur Mac natif, Docker ARM64 et Docker VPS x86-64 : 500 ms sortants,
finalité 2 → 4 en 146,980 s sous défaut, puis rattrapages 4 → 6 → 8 et fenêtre
**8 → 9 → 10 → 11**. Observation après restauration 271,743 s sous la borne
inchangée de 288 s ; 114 lectures par état, trois validateurs sains à l'arrêt.
Le correctif Beacon passe aussi en tests natifs sur les trois plateformes.
Les deux échecs précédents restent conservés. Ce scénario borné est qualifié ;
rotation WAN, sanctions/cycle complet à consolider, charge, exploitation,
écarts EVM/RPC, bridge et revue indépendante restent dans l'objectif global.

## Lecture Capella par état implémentée — 29 septembre 2026

Le [correctif de lecture immuable](qualification/capella-pinned-beacon-20260929/README.md)
sert root/fork/finalité par state root retenu et vérifie l'engagement SSZ. Le
moniteur associe le header à cet état, sans exiger un head immobile. Racines
invalides, inconnues/élaguées, conflits et régressions restent des erreurs.
Test natif passé (5,42 s), 24 tests du moniteur passés ; compilation native
réussie. La qualification Linux/WAN du nouveau binaire reste à faire. Aucun
quorum ni délai assoupli ; les échecs précédents et l'objectif global restent ouverts.

## Lecture Beacon cohérente encore manquante — 29 septembre 2026

La [nouvelle campagne WAN](qualification/capella-wan-asymmetric-settle-20260929/README.md)
atteint 2 → 4 sous retard en 159,741 s, puis échoue : le head VPS alterne entre
deux branches pendant les trois lectures, malgré un checkpoint finalisé identique.
Les validateurs restent sains, mais la stabilité n'est pas qualifiée. L'API ne
fournit actuellement cette finalité que sous `states/head` ; prochaine correction :
lecture par state root identifié, tests de concurrence/élagage/engagement, puis
nouvelle qualification. Aucun retry supplémentaire ou seuil assoupli. Les deux
runs restent en échec dans leurs preuves ; le cœur n'a pas été modifié.

## Saut FFG expliqué, moniteur de récupération corrigé — 29 septembre 2026

Le [rejeu du checkpoint réel](qualification/capella-wan-ffg-analysis-20260929/README.md)
authentifie la base par le digest du reçu terminal et reproduit exactement
4 → 6 au passage du slot 71 à 72. Les poids de cibles 54/64 et 47/64 satisfont
les conditions FFG ; le saut n'est pas en lui-même une erreur de consensus.
Le mode de récupération du moniteur recommence sa fenêtre après un saut sans
lui donner de crédit, sans effacer l'historique de sécurité et sans prolonger
le délai absolu. Les trois avancées consécutives liées à l'exécution restent
exigées. 19 tests du moniteur passent, dont six nouveaux. Le cœur est inchangé ;
l'échec historique reste conservé et une campagne distincte qualifie le correctif.

## WAN asymétrique : défaut testé, stabilité encore ouverte — 29 septembre 2026

La [nouvelle campagne réelle Mac/Docker/VPS](qualification/capella-wan-asymmetric-20260929/README.md)
applique 500 ms de retard sortant au seul conteneur VPS. Les trois nœuds
progressent de la finalité 2 à 4 en 159,156 s sous défaut (2 092 paquets mesurés).
Après retrait, le moniteur échoue sur le saut 4 → 6 avant ses trois avancées
consécutives : **campagne globale en échec**, malgré les trois validateurs sains.
Les traces internes confirment les observations 4 aux slots 56–71 puis 6 aux
slots 72–73 ; cause à reconstruire depuis justification/participation, aucun
seuil relâché. 15 nouveaux cas de sécurité du banc, 41 cas au total, passent.
Rotation WAN, charge, exploitation, bridge et production restent ouverts ;
les preuves de rotation locale existantes sont référencées dans l'audit candidat.

## Nouvel objectif : candidat production — 29 septembre 2026

L'[audit de départ](qualification/production-candidate-20260929/README.md)
conserve les acquis et cible d'abord la finalité WAN sous partition. Le cas
local à 500 ms est déjà passé ; l'ancien WAN à 189,2 s reste un échec face au
seuil de 180 s. Le préparateur de topologie prend désormais en charge Capella
avec trois instances et 22/21/21 clés synthétiques. Configuration et 26 tests du banc vérifiés. La [campagne Capella WAN](qualification/capella-wan-partition-20260929/README.md)
passe : coupure observée 97,239 s, reprise commune en 136,691 s (seuil 180 s),
puis trois avancées 7 → 8 → 9 → 10 avec accord des blocs d’exécution. Trois
groupes de validateurs sains à l’arrêt. Les anciens échecs sont conservés ;
asymétrie WAN, charge et qualification globale de production restent ouvertes.

## Jalon sorties : qualification technique acquise — 29 septembre 2026

L'[audit des six critères](qualification/capella-exit-audit-20260929/README.md)
relie chaque exigence à ses sources, tests et limites : admission authentifiée,
livraison autonome après redémarrage, inclusion/réinclusion, finalité et paiement
sur Mac/VPS, cohérence après WAL/SIGKILL, retrait des archives et élagage.
La dernière sélection passe (3/3, 16,45 s), le réseau local après reprise passe
(1/1, 427,44 s), et les seize cas WAL passent aussi sur Linux (3/3 tests,
476,02 s). Les échecs initiaux et les versions exactes des bancs sont conservés.

**Cela termine la qualification technique de ce jalon, pas la production de
Nanobees.** Les étapes globales EVM/RPC, consensus sous partitions/charge,
exploitation, bridge et lancement économique restent ouvertes. La publication
Git du livrable est contrôlée séparément avant clôture de l'objectif actif.
Les sections suivantes sont le journal historique des étapes et de leurs
limites à la date du run ; l'audit ci-dessus fixe l'état consolidé du jalon.

## Audit des sorties en cours — 29 septembre 2026

Les [huit rejets explicites de contexte BLS](qualification/capella-exit-authentication-20260929/README.md)
passent sur Mac (deux tests, 10,92 s), sans mutation du checkpoint ni de la file.
La qualification WAN existante démontre la propagation de la sortie dans les
blocs ; ses journaux ne démontrent pas une livraison autonome `voluntary_exit`.
Cette livraison autonome est prouvée dans le scénario local RPC/libp2p. Son
[extension hors connexion](qualification/capella-exit-offline-relay-20260929/README.md)
passe en 427,44 s : redémarrage sans nouvelle soumission, livraison autonome,
retrait finalisé et contrôle des deux bases rouvertes. Ne pas confondre
ces deux portées ni assimiler cette extension à une partition WAN.

## Sortie entre Mac et VPS qualifiée — 29 septembre 2026

Le [scénario Mac/VPS](qualification/capella-exit-multihost-20260929/README.md)
passe avec 32 validateurs par hôte et le vrai cpp-libp2p dans un tunnel SSH
privé : soumission sur le VPS, propagation, inclusion, SIGKILL avant finalité,
reconnexion et retrait finalisé identique au bloc 55 (époque 8). Un seul paiement
du capital ; comptabilité exacte comparée sur les deux RPC. La réouverture du
Mac confirme l’archive retirée, la cohérence EVM/consensus et l’absence de
proposition/attestation après l’époque de sortie. Les deux échecs du banc sont
conservés. La réouverture hors ligne du VPS passe sur `24971b8`, avec les
mêmes montants, finalité et archive que le Mac ; le nouveau test d’élagage
passe également sur Linux (1/1, 89,17 s).

Le [nouveau cas d’élagage d’une sortie jamais incluse](qualification/capella-exit-pending-pruning-20260929/README.md)
passe (32,56 s), avec réancrage, réouverture, doublon idempotent et inclusion
automatique. Les régressions élagage BLS/retrait d’archive passent également
(2/2, 99,92 s). La [campagne aux transitions de sortie](qualification/capella-exit-lifecycle-sync-20260929/README.md)
passe maintenant sur Mac : 16 cas WAL aux publications d’inclusion,
retrait d’archive, paiement et finalisation du paiement (130,29 s), puis
6/6 régressions ciblées (121,81 s). La campagne Linux/VPS passe également sur `2d7c6da` : 16/16 cas
(447,33 s), avec 3/3 tests au total (476,02 s). L’audit final de couverture
du jalon reste ouvert.
**Le jalon complet et la production ne sont pas déclarés terminés.**


## Qualification réseau des sorties — locale (29 septembre 2026)

Le scénario RPC/libp2p avec deux vrais nœuds locaux passe : retrait complet,
finalité commune au bloc 56, comptabilité des paiements après réouverture et
contrôle des dernières propositions/attestations du validateur sorti. Un seul
processus possède les clés des validateurs ; cela ne qualifie pas un consensus
entre hôtes indépendants. Les [quatre cas WAL de l’admission](qualification/capella-exit-wal-20260929/README.md) passent aussi sur Mac, avec la régression BLS du banc partagé (2/2 tests).
La [campagne WAL Linux/VPS](qualification/vps-exit-wal-20260929/README.md)
passe sur `ebe1d16` (2/2, 26,48 s). Le [contrôle direct de l'archive](qualification/capella-exit-retirement-20260929/README.md)
passe sur Mac : conservation avant finalité, retrait après finalité, puis
élagage et réouvertures (64,81 s). Le [cas après réindexation](qualification/capella-exit-reindex-20260929/README.md) passe également (174,47 s) : ancien index refusé, bon index admis et inclus, autre validateur inchangé. La preuve Mac/VPS acquise ensuite est décrite ci-dessus.
Voir les [preuves, échecs et limites](qualification/capella-exit-network-20260929/README.md).
La production et le jalon complet restent non démontrés.


Sélection priorisée à partir des spécifications officielles, consultées le
24 septembre 2026. Cette liste complète le goal ; elle ne prétend ni épuiser
toutes les EIP d’un fork ni prouver leur implémentation actuelle dans Nano.

Une EVM compatible active un profil cohérent avec toutes ses dépendances,
pas une sélection libre d’opcodes. Les règles historiques encore pertinentes
font partie du socle ; les changements PoW historiques ne sont pas imposés
au consensus PoS de Nano. Pour chaque ligne : auditer l’existant, corriger les
écarts et rattacher des preuves plutôt que réimplémenter ce qui fonctionne.

## Socle transactions, sécurité et frais

| EIP | Rôle / validation |
| --- | --- |
| [2](https://eips.ethereum.org/EIPS/eip-2) | Signatures canoniques et échecs de création ; volet exécution applicable |
| [155](https://eips.ethereum.org/EIPS/eip-155) | Protection des transactions legacy par chain ID |
| [1344](https://eips.ethereum.org/EIPS/eip-1344) | CHAINID accessible aux contrats |
| [2718](https://eips.ethereum.org/EIPS/eip-2718) | Enveloppes de transactions typées |
| [2930](https://eips.ethereum.org/EIPS/eip-2930) | Listes d’accès |
| [1559](https://eips.ethereum.org/EIPS/eip-1559) | Base fee dynamique, brûlage et frais prioritaires |
| [3198](https://eips.ethereum.org/EIPS/eip-3198) | BASEFEE accessible aux contrats |

## Exécution déterministe et maîtrise du coût

| EIP | Rôle / validation |
| --- | --- |
| [140](https://eips.ethereum.org/EIPS/eip-140) | REVERT et annulation des effets |
| [150](https://eips.ethereum.org/EIPS/eip-150) | Tarification des opérations et transfert borné de gas aux appels |
| [161](https://eips.ethereum.org/EIPS/eip-161) | Traitement des comptes vides |
| [170](https://eips.ethereum.org/EIPS/eip-170) | Limite de taille du code déployé |
| [211](https://eips.ethereum.org/EIPS/eip-211) | Données de retour des appels |
| [214](https://eips.ethereum.org/EIPS/eip-214) | STATICCALL et interdiction des mutations |
| [1014](https://eips.ethereum.org/EIPS/eip-1014) | CREATE2 et déploiements déterministes |
| [2200](https://eips.ethereum.org/EIPS/eip-2200) | Comptabilité SSTORE |
| [2929](https://eips.ethereum.org/EIPS/eip-2929) | Accès chauds/froids |
| [3529](https://eips.ethereum.org/EIPS/eip-3529) | Réduction des remboursements de gas |
| [3860](https://eips.ethereum.org/EIPS/eip-3860) | Taille et coût de l’initcode |

## Compatibilité Solidity moderne et forks

| EIP | Rôle / validation |
| --- | --- |
| [3855](https://eips.ethereum.org/EIPS/eip-3855) | PUSH0, Shanghai |
| [1153](https://eips.ethereum.org/EIPS/eip-1153) | Stockage transitoire, Cancun |
| [5656](https://eips.ethereum.org/EIPS/eip-5656) | MCOPY, Cancun |
| [6780](https://eips.ethereum.org/EIPS/eip-6780) | Sémantique moderne de SELFDESTRUCT, Cancun |
| [7702](https://eips.ethereum.org/EIPS/eip-7702) | Délégation de code des comptes, Prague ; autorisations et rejeu à qualifier |

## Blobs et rollups : périmètre réseau complet

| EIP | Rôle / validation |
| --- | --- |
| [4844](https://eips.ethereum.org/EIPS/eip-4844) | Transactions blobs, KZG, disponibilité et rétention |
| [7594](https://eips.ethereum.org/EIPS/eip-7594) | PeerDAS : échantillonnage, custody et reconstruction |

## Applications : standards ERC distincts du moteur EVM

Qualifier ERC-20, ERC-721, ERC-1155 et ERC-1271 par contrats Solidity,
ainsi que signatures EIP-712, permis ERC-2612 et détection ERC-165.
Sources : https://ercs.ethereum.org/ et https://eips.ethereum.org/EIPS/eip-712.
Ces standards ne sont pas des opcodes à ajouter au moteur. Tests de contrats,
callbacks malveillants, permissions, signatures, rejeu et compatibilité RPC
sont nécessaires. Aucun standard ERC ne certifie la viabilité économique.

## Critères de production qui ne sont pas des EIP

Finalité et reprise, partitions, limites de ressources, durabilité, sécurité
RPC/P2P, builds reproductibles, gestion des clés et paramètres de lancement
restent nécessaires. EIP-1559 ne garantit ni prix constant ni absence de spam.
EIP-4844/PeerDAS sont pertinents pour les rollups mais ne sont pas obligatoires
pour toute L1 EVM et ne garantissent aucune baisse de prix chiffrée.

La compatibilité doit séparer : implémentation dans les runners, admission
réseau, activation du fork et validation de bout en bout. Les extensions
Draft/Review déjà demandées restent suivies dans VALIDATOR-EIP-ROADMAP.md
et ne deviennent pas des prérequis universels à toute blockchain de production.

## Compléments de couverture Cancun / Prague

Suivre explicitement EIP-7516, 2537, 2935, 7623, 7691 et la configuration
7840. Évaluer 7642 seulement pour une éventuelle compatibilité eth/69.
Référence de composition : https://eips.ethereum.org/EIPS/eip-7600.
Leur état réel est consigné dans EIP-IMPLEMENTATION-STATUS.md ; plusieurs
sont déjà présents dans le moteur sans activation réseau. Ajouter comme
critères le provisionnement versionné des contrats système et la couverture
RPC eth_createAccessList ; préserver le traitement existant des frais dynamiques.

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


### Diagnostic mesuré du retard — 27 septembre 2026

Le scénario500ms échoue encore à la finalité3, avec instrumentation seule.
Pour les attestations, la médiane d'envoi passe de213–219ms à2893–3019ms :
environ1,4–1,5s d'ouverture du flux, puis1,5s après écriture (écriture médiane0ms).
Les traces distinguent admission/refus/issue inconnue et conservent les échecs.
Le temps post-écriture inclut transport et travail distant, pas seulement RTT.
Prochaine correction : amortir négociation et accusés par réutilisation ou lots
bornés, en préservant les décisions d'admission et les budgets. **Blocage actif**,
aucune nouvelle conformité EVM ou disponibilité de production.
[Mesures et limites](qualification/capella-send-timing-20260927/README.md).


### Primitive de réception réutilisable — 27 septembre 2026

Session séquentielle bornée à 16 messages, avec budget, validation du domaine et
accusé distincts par message. Annulation à la frontière ACK corrigée ; **11 cas
nouveaux**, 30 au total dans `altair_stream`, et `timed_reply` passent. Binaire
P2P compilé. **Non raccordée au réseau** : négociation, émetteur et invalidation
par génération restent à intégrer. Le contre-exemple de finalité à 500ms reste
ouvert ; aucun statut EIP ou production clôturé. [Preuves et limites](qualification/consensus-stream-session-20260927/README.md).


### Finalité sous retard asymétrique : scénario passé — 27 septembre 2026

Le transport Capella réutilise des flux négociés séparément, bornés à 16 messages,
avec un accusé et une admission par message. Huit nouveaux cas émetteur portent
la suite de flux à 38 ; compatibilité/quota/relais vérifiés. **Le même scénario
Docker 500ms passe désormais** : finalité commune 2 → 4 pendant 147,985s de retard,
puis 5 après retrait (43,826s pour la qualification de reprise). Médiane des
attestations sous retard ≈1,5s, contre ≈3s précédemment ; aucun refus de budget
entrant journalisé. Refus d'admission, un échec d'ACK et deux envois d'attestation
non confirmés restent dans les preuves. Ce contre-exemple local est couvert ;
WAN géographique, nombreux pairs, adversaires, charge et production globale
restent ouverts. Aucun nouveau statut EIP clôturé. [Code, mesures et limites](qualification/capella-stream-reuse-20260927/README.md).


### Crash complet, reprise avec témoin et ancre après élagage — 27 septembre 2026

Deux courses de délais sont reproduites puis corrigées : une expiration JSON déjà
en file fermait un flux terminé ; un ancien délai d'inactivité pouvait fermer le
flux réutilisé. La concurrence de neuf pairs synthétiques respecte les huit
permis bulk globaux et conserve la capacité de contrôle (pas un réseau de neuf
nœuds qualifié). 40 cas de flux et cinq cas JSON passent.

Le nouveau SIGKILL Docker révèle d'abord une configuration de reprise absente
(refus explicite du checkpoint), puis une finalité bloquée à 2 malgré le témoin.
Le relais conservait l'ancre du démarrage après élagage. Il reconnaît désormais
l'ancre actuelle renvoyée par le cœur authentifié pour terminer une recherche
d'ancêtres, sans inventer d'ACK du pair. Six cas nouveaux portent la livraison
ancestrale à 19 scénarios ; les descendants exigent toujours leur admission réelle.

**Rejeu passé** : deux nœuds, 32 clés synthétiques chacun ; SIGKILL de 96,801s,
identité/volume/checkpoint conservés, puis finalité commune 7 en 141,703s depuis
la configuration de reprise (139,655s après disponibilité vérifiée). Les trois
campagnes et les tests négatifs sont conservés. La configuration passe
explicitement de l'amorçage à la reprise par témoin ; le redémarrage autonome
sans changement de configuration, les pannes physiques, le WAN, les nombreux
pairs et la production globale restent ouverts. Aucun nouveau statut EIP clos.
[Correctifs, échecs, réussite et limites](qualification/reusable-stream-deadlines-20260927/README.md).

### Reprise automatique Capella explicite — 27 septembre 2026

Option `automaticRecovery: true` : conserver le snapshot initial dans la
configuration, initialiser seulement si les deux stores sont vierges, sinon
exiger la reprise vérifiée par témoin. Le témoin absent avec checkpoint primaire
est refusé ; aucune réinitialisation silencieuse. Deux scénarios runtime passent
(ancien mode explicite et nouveau mode automatique, 64 clés synthétiques).
Le crash Docker sans modification de configuration reste à qualifier ; la perte
ou le rollback conjoint des deux stores exige une frontière de confiance externe.
Aucun EIP clôturé, aucune qualification globale de production.
[Preuves et limites](qualification/capella-automatic-recovery-20260927/README.md).

### Crash Docker avec configuration inchangée — 27 septembre 2026

**PASS** : deux nœuds × 32 clés synthétiques, SIGKILL complet pendant 96,757 s,
reprise automatique avec le même fichier (SHA256 contrôlé), la même identité
P2P et le checkpoint conservé. Finalité commune d'époque 7 après 143,110 s
depuis la préparation du redémarrage ; disponibilité vérifiée après 2,022 s.
Ce cas clôt la limite de modification manuelle de configuration du scénario
précédent. WAN, pertes physiques, rollback indépendant, longues pannes, charge
et production globale restent ouverts. Aucun EIP clôturé ni VPS modifié.
[Preuves](qualification/capella-automatic-docker-recovery-20260927/README.md).
[Procédure de reprise](CAPELLA-RECOVERY.md).

### Coût des retraits sur grands registres — 27 septembre 2026

Six nouveaux cas Capella passent : 100 000 et 1 000 000 entrées synthétiques,
retraits denses/rares/absents, bouclage du curseur, montants/adresses/ordre,
conservation de tous les validateurs et refus de listes altérées. À un million,
préparation du candidat en 13 788–32 442 µs ; pic RSS processus 394 772 480 octets.
Sélection bornée à 16 384 entrées, mais copie du registre O(N) toujours présente.
Mesure locale unique hors SSZ/BLS/disque/réseau, pas une capacité de nœud actif.
Aucun EIP ni objectif production clôturé ; mesurer ensuite la projection SSZ
authentifiée et le runtime complet.
[Code, mesures et limites](qualification/capella-withdrawal-scale-20260927/README.md).

### Authentification SSZ à un million : coût limitant mesuré — 27 septembre 2026

Deux nouveaux cas passent avec comparaison de tous les octets et refus d'une
racine incorrecte. États synthétiques de 100 000 / 1 000 000 entrées, autres
vecteurs au preset minimal. Préparation complète des retraits SSZ : 422 669 /
4 258 573 µs ; calcul initial de racine : 215 134 / 2 068 384 µs. À un million,
état de 139 010 821 octets et RSS maximal du processus 875 020 288 octets.
La mesure inclut authentification/projection/copie/recalcul de racine, hors
BLS, reste de transition, disque et réseau. Elle démontre un coût significatif
que le benchmark registre seul masquait ; aucune capacité runtime clôturée.
Priorité : profiler et réduire les calculs complets répétés sans supprimer
l'authentification, puis mesurer le nœud complet. Aucun EIP clôturé.
[Preuves](qualification/capella-withdrawal-ssz-scale-20260927/README.md).

### Arbre SSZ Validator de taille fixe — 27 septembre 2026

Optimisation C++ : les huit champs du validateur sont hachés sur la pile, sans
vecteurs temporaires ni recalcul des sous-arbres nuls inutiles. Vérifications de
format, annulation, authentification et racine de sortie conservées. Six suites
passent ; 17 nouvelles mutations de champs. Racines avant/après identiques à
100 000 et un million d'entrées. À un million, préparation SSZ mesurée de
4 468 134 à 3 029 580 µs (~32 % de temps en moins, comparaison unique).
Le coût complet du runtime reste ouvert ; aucune conformité EIP supplémentaire.
[Preuves et comparaison](qualification/ssz-validator-fixed-tree-20260927/README.md).

### Retraits SSZ sans projection complète du registre — 27 septembre 2026

Le chemin SSZ authentifie la racine et les cardinalités, puis sélectionne les
retraits directement dans une fenêtre bornée. Règles de sélection partagées avec
le registre classique ; comparaison exacte de la liste et recalcul final maintenus.
Cinq suites passent, avec deux nouvelles annulations sans mutation. À un million,
RSS maximal du benchmark de 854 589 440 à 639 205 376 octets ; temps mesuré de
3 029 580 à 2 911 148 µs. Racines identiques. Les autres projections et le coût
runtime complet restent ouverts ; aucune clôture EIP ou production.
[Preuves](qualification/ssz-withdrawal-direct-sweep-20260927/README.md).

### Réutilisation SSZ confinée à une transformation — 27 septembre 2026

Entrée copiée et intégralement authentifiée avant transformation ; racines des
champs variables réutilisées uniquement après égalité exacte des octets et
longueurs, au sein du même appel. Champs changés et offsets revérifiés, aucun
cache externe fourni ou persistant. Sept suites passent, dont nouveaux contrôles
sur les huit états Capella officiels (modifications de champs, entrée non
authentifiée, sortie malformée, isolation et abandon). À un million, préparation
de 2 911 148 à 1 525 469 µs ; RSS 639 090 688 octets, racines identiques.
Mesures locales uniques hors runtime complet ; aucune clôture EIP/production.
[Preuves et frontière de confiance](qualification/ssz-scoped-field-reuse-20260927/README.md).

## 2026-09-27 — Régression Bellatrix : drainage des traces de reprise

La régression sur `af7f99e` expose un échec de finalité après partition
(516,39 s). Le prédicat ne drainait plus les sorties tant que la finalité
n’avançait pas : le correctif draine les deux processus inconditionnellement.
Un nouveau contrôle à deux enfants reproduit la contre-pression stdout/stderr
et vérifie la récupération intégrale après drainage. Le scénario réseau avec
ce correctif reste à rejouer ; la campagne initiale continue sur ses binaires
inchangés. Aucun délai ni critère abaissé, aucune EIP clôturée.
[Échec, contrôle et limites](qualification/bellatrix-partition-trace-drain-20260927/README.md).

## 2026-09-27 — Assertoor : défaut de base de résultats reproduit

La régression `consensus_bellatrix_assertoor_finality` échoue après 296,02 s
malgré une finalité commune d’époque 4 à l’arrêt. Le journal Assertoor signale
une table `task_logs` absente. Un reproducer à deux connexions avec son pilote
exact reproduit l’erreur du DSN mémoire ; un fichier temporaire partagé entre
connexions passe. Le probe configure désormais une base de résultats sur disque
propre à chaque run. Le rejeu réel reste à effectuer ; aucune règle de consensus
ni limite temporelle modifiée, aucun EIP clôturé.
[Échec, reproducer et correction](qualification/assertoor-result-database-20260927/README.md).

## 2026-09-27 — Bilan initial de la régression consensus

Sur les binaires `af7f99e`, **185/188 tests sélectionnés passent, 3 échouent,
aucun n’est ignoré**. Les deux partitions Bellatrix échouent avant la reprise
de finalité ; Assertoor finalité échoue avec une table de résultats absente.
Les correctifs de banc d’essai sont préparés séparément et attendent leurs
rejeux réels. La reprise après crash avec stabilité prolongée passe (325,44 s).
Les 201 autres tests CTest sont hors de cette campagne ; aucun statut EIP ou
production n’est clôturé. [Bilan, empreintes et résultats complets](qualification/consensus-regression-20260927/README.md).

## 2026-09-27 — Rejeu de partition Bellatrix après correction du banc

Le scénario simple corrigé passe : coupure demandée de 96 s, finalité commune
d’époque 6 retrouvée en 95 s, mêmes racines Beacon et bloc d’exécution sur les
deux nœuds. Les binaires Nano/P2P sont inchangés ; seul le drainage des sorties
du scénario est corrigé. Les seuils et délais restent identiques. La variante
prolongée et le rejeu Assertoor sont encore attendus ; aucune clôture EIP ou
production. [Rejeu et contre-exemples conservés](qualification/bellatrix-partition-trace-drain-20260927/README.md).

## 2026-09-27 — Rejeu Assertoor après correction de sa base

Le contrôle réel de finalité passe en 198,32 s : quatre tâches Assertoor
réussies, finalité commune d’époque 2 et même bloc d’exécution sur les deux
nœuds, vérifiés indépendamment du cache Assertoor. Plus d’erreur de table SQL
absente dans ce rejeu. Les binaires Nano/P2P et les délais restent inchangés.
Ce résultat local couvre ce défaut du banc ; il ne clôture aucune EIP ni la
production. [Échec initial et rejeu complet](qualification/assertoor-result-database-20260927/README.md).

## 2026-09-27 — Trois échecs initiaux couverts par leurs rejeux

La partition prolongée corrigée passe (565,85 s) : reprise commune d’époque 7
en 144 s après reconnexion, puis trois avancées communes jusqu’à l’époque 10
en 144,552 s. Avec la partition simple et Assertoor déjà rejoués, les trois
échecs initiaux ont chacun une preuve corrigée réussie. La campagne initiale
reste 185/188 ; ne pas présenter l’ensemble comme un unique run 188/188. Aucun
code de consensus ni seuil assoupli, aucune nouvelle clôture EIP/production.
[Campagne et rejeux séparés](qualification/consensus-regression-20260927/README.md).

## 2026-09-27 — Recommandation des frais RPC historique

Le pourboire constant de 1 gwei est remplacé par une politique configurable :
échantillons de frais effectifs récents, exclusion du producteur et cache par
hash de tête. Le portefeuille partage cette recommandation ; les frais explicites
restent inchangés et les valeurs implicites sont bornées à uint256. Quatre tests
passent (8,65 s), dont le nouveau `rpc_fee_policy` ; les trois autres couvrent
les parcours RPC/CLI existants. Aucun changement de formule consensus EIP-1559.
Coût à froid, saturation et concurrence pendant une réorganisation restent à
qualifier ; aucun statut global de production clôturé.
[Politique](RPC-FEE-POLICY.md) et [preuves](qualification/rpc-fee-policy-20260927/README.md).

## 2026-09-27 — Charge RPC et correction de l’exclusion du producteur

Le nouveau scénario détecte un défaut masqué par l’ancien test : adresse avec
checksum comparée à une adresse normalisée, donc producteur non exclu. Les deux
adresses sont désormais normalisées ; l’ancien test utilise un pourboire du
producteur inférieur aux frais externes. Cinq tests ciblés passent ; le scénario
final couvre aussi 20 blocs / 28 560 transactions. Calcul sans cache de
recommandation autour de 652 ms, sous mutex RPC ; 1 000 lectures en cache en
5,385 ms au total. Mesures locales, caches disque non purgés, sans HTTP/P2P.
Coût répété à chaque tête, fenêtre maximale, équité et saturation restent ouverts.
[Échec initial, correction et mesures](qualification/rpc-fee-load-20260927/README.md).

## 2026-09-27 — Réutilisation bornée des échantillons de frais RPC

Les pourboires sont maintenant réutilisés par hash de bloc, dans la seule fenêtre
courante ; rechargement de politique, réorganisation et expiration de fenêtre
restent contrôlés. Cinq tests affectés passent, avec nouveaux cas de branche
remplacée et expiration. Après un nouveau bloc de 1 428 transactions, mesure
locale de 33,518 ms contre 671,225 ms pour un estimateur neuf sur la même tête.
Le premier calcul, les branches non cachées, la fenêtre maximale et l’équité
sous mutex RPC restent ouverts. Aucun changement de consensus EIP-1559.
[Implémentation, mesures et limites](qualification/rpc-fee-incremental-20260927/README.md).

## 2026-09-27 — Reprise ordonnée pendant l’activation d’une nouvelle clé

Nouveau scénario `consensus_capella_deposit_activation_restart` passé en 159,53 s.
Dépôt consommé au slot 80 ; owner et journal rouverts au slot 104 pendant l’attente,
puis second owner et journal après le premier vote au slot 142. Le vote
contradictoire est rejeté avant le signataire, sans mutation du journal ; trois
votes légitimes sont admis et la finalité atteint l’époque 18. Registre à 65,
activation époque 17 et curseur unique conservés après réouverture finale.
L’échec initial de banc est conservé, aucun correctif runtime. **Partiel** :
réouvertures ordonnées, pas crash au commit, perte de témoin, WAN ou churn massif.
Ne clôture ni EIP-6110 ni le cycle validateur de production.
[Preuves et limites](qualification/capella-activation-restart-20260927/README.md).

## 2026-09-27 — Clé configurée avant inscription, résolution automatique

Le runtime accepte désormais `index: null` : clé publique dérivée du secret,
index recherché dans la tête authentifiée à chaque poll, attente sans devoir
si absente et garde d’activation inchangée. Journal, identité de chaîne et
identité du devoir restent vérifiés avant signature. Les index explicites
restent stricts ; reçus et traces identifient les clés automatiques.
Quatre tests passent : deux nouveaux (chargement/reprise avec clé absente et
dépôt jusqu’aux trois attestations automatiques) plus runtime et finalité de la
boucle existante. Dépôt slot 80, activation époque 17, finalité époque 18.
**Partiel** : projection du registre à grande échelle, reorg concurrent, crash
au commit et WAN restent ouverts. Aucun statut EIP-6110/production clôturé.
[Configuration](CAPELLA-VALIDATOR-REGISTRATION.md) et [preuves](qualification/capella-automatic-registration-20260927/README.md).

## 2026-09-27 — Recherche des clés automatiques sans projection complète

Le reactor Capella authentifie le SSZ de la tête puis ne conserve que les clés
locales demandées. Nouveau test à 100 000/1 000 000 enregistrements synthétiques :
146 ms/1,43 s avec authentification, trois correspondances exactes et une absence.
Racine incorrecte, doublon, largeur, annulation et cardinalité incohérente refusés.
La régression du runtime à 65 clés configurées passe ; 2/2 tests en 29,37 s.
Le coût linéaire sous verrou à chaque poll, les changements de branche concurrents
et la charge distribuée restent ouverts. Aucun million de nœuds ni BLS mesuré,
aucune clôture EIP ou production. [Preuves](qualification/capella-registration-ssz-scale-20260927/README.md).

## 2026-09-27 — Cache des index lié à la tête authentifiée

Cache privé à une entrée, indexé par racine SSZ et ensemble exact de clés locales,
sous verrou du reactor. Pas de snapshot conservé ; une absence n'est mémorisée
que pour la racine courante. Nouvelle couverture du scénario de réorganisation :
trois recalculs de racine, sept hits, deux bascules de branche, annulation et
changement des clés contrôlés. Runtime et dépôt/activation passent aussi :
**3/3 en 163,77 s**, trois attestations de la nouvelle clé après activation.
Le coût froid à chaque nouvel état, les indices divergents après dépôts concurrents
et la charge distribuée restent ouverts. Aucun EIP/production clôturé.
[Preuves et limites](qualification/capella-registration-cache-20260927/README.md).

## 2026-09-27 — Index automatique revérifié sous verrou de signature

Les propositions, attestations et messages sync revérifient la paire clé/index
sur la tête admise avant le signataire. Un index périmé devient un devoir à
retenter (`StaleCapellaDuty`), sans masquer une mauvaise association au journal.
Nouveau test : trois refus d'index et trois refus de configuration, zéro appel
au signataire et checkpoint/journal inchangés, puis 40 blocs et finalité époque 3
avec les clés automatiques. Régressions index fixe et dépôt/activation passent :
**3/3 en 160,16 s**. Deux dépôts concurrents réattribuant un index et disponibilité
WAN restent ouverts ; le test injecte directement un index périmé, pas un thread
concurrent. Aucun EIP/production clôturé.
[Preuves](qualification/capella-registration-signing-guard-20260927/README.md).

## 2026-09-27 — Deux branches financées, ordres de dépôts inversés

Nouveau cas dans `consensus_execution_deposit_history` : mêmes clés A/B déposées
par transactions exécutées dans les ordres AB/BA, index de A 0/1 ; quatre preuves
étrangères et quatre rejeux refusés sans mutation. Les deux restaurations
reconstruisent leurs registres. Le sélecteur natif distingue les deux ascendances
à cardinalité égale et vérifie quatre opérations obligatoires. Test passé en
0,39 s. Enveloppes Beacon synthétiques pour la sélection ; bascule réelle du
reactor, fork choice et signatures après réindexation restent ouverts.
Aucun changement de production, aucun EIP clôturé.
[Preuves et limites](qualification/competing-deposit-order-20260927/README.md).

## 2026-09-27 — Reactor réindexé par de vrais blocs à dépôts concurrents

Nouveau scénario local avec trois propriétaires RocksDB : branches AB/BA
signées et admises jusqu'au slot 49, dépôts consommés, registre de 66 membres.
Les votes de comité sélectionnent chaque branche ; la clé A passe de 64 à 65,
avec état EVM/historique cohérents et deux réouvertures vérifiées. L'ancien
index est refusé avant signature sans mutation ; le nouveau reste inactif.
Test passé en 22,09 s. Échec initial du budget de 64 blocs du banc conservé,
budget du scénario calculé à 98 sans changement de consensus. Activation après
reorg, finalité, threads concurrents et WAN restent ouverts. Aucun EIP clôturé.
[Preuves](qualification/capella-competing-deposit-reorg-20260927/README.md).

## 2026-09-27 — Activation et finalité après la réindexation 64 vers 65

Extension intégrée de la concurrence de dépôts : après sélection de BA, la
boucle automatique retrouve A à l'index 65, attend l'activation époque 13 puis
produit neuf attestations admises. Aucun appel au signataire avant activation.
Le producteur et l'observateur finalisent le même checkpoint d'époque 20 ; les
deux réouvertures conservent finalité et index. Test passé en 175,54 s, jusqu'au
slot 177. Trois bases locales, appels orchestrés sans transport WAN, journal A
resté ouvert, pas de crash au commit ni reorgs soutenues. Aucun EIP/profil ou
objectif production clôturé. [Preuves](qualification/capella-reindexed-activation-20260927/README.md).

## 2026-09-27 — Panne WAL au vote de réorganisation de dépôts

Injection macOS fsync/F_FULLFSYNC EIO dans un sous-processus au vote qui fait
passer A de 64 à 65. Cinq accès refusés après erreur. WAL conservé : état complet
65 et archive du vote ; suffixe perdu : état complet 64 sans archive, puis rejeu
vers 65. Racines EVM/têtes et enveloppes durables exactes contrôlées. Test passé
en 28,61 s. Échec initial du banc (snapshot simple versus enveloppe de votes)
conservé et corrigé. Pas de panne physique, Linux/VPS, témoins indépendants ou
WAN ; aucun EIP/production clôturé. [Preuves](qualification/capella-deposit-reorg-sync-20260927/README.md).

## 2026-09-27 — Injection WAL portée et qualifiée sous Linux ARM64

Interposition fsync/fdatasync réservée aux tests Linux, contrôlée sur fichier
ciblé, fichier distinct et après désactivation. Réutilisation de l'image de
dépendances dans Docker sans réseau, 2 CPU/4 Go. Le scénario de réorganisation
avec WAL conservé/perdu passe en 24,63 s : fermeture après EIO, état ancien ou
nouveau cohérent, archives et rejeu vers l'index 65 vérifiés. Aucun code du nœud
modifié, ni nouvelle image/VPS. Linux ARM64 local seulement : x86-64/VPS,
WAN et autres pannes restent ouverts. [Preuves](qualification/linux-deposit-reorg-sync-20260927/README.md).

## 2026-09-27 — Panne WAL qualifiée sur VPS Linux x86-64

Au commit `c379e7d`, le scénario de réorganisation de dépôts passe sur le VPS
Ubuntu 24.04/Docker amd64 en **99,87 s** : cinq accès refusés après EIO,
ancien état/index 64 si suffixe perdu, nouvel état/index 65 si WAL conservé,
puis rejeu cohérent vers 65. Sources propres, empreintes et journaux archivés.
Test local isolé sur le VPS, pas WAN ni panne physique ; autres points de panne,
charge, témoins indépendants et qualification globale restent ouverts.
[Preuves et limites](qualification/vps-deposit-reorg-sync-20260927/README.md).

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

## 2026-09-29 — Sorties Capella raccordées au service et au relais

Admission des sorties par le worker et les reçus RPC privés, catalogue/export
bornés, vérification d'empreinte et rotation de diffusion du daemon P2P ajoutés.
3 tests cœur passent en 14,92 s ; 35 scénarios de relais simulé passent en 0,45 s.
Le daemon compile. La propagation réelle Mac/VPS, HTTP, le retrait final et les
pannes du parcours complet restent à démontrer ; aucun objectif clôturé.
[Preuves et limites](qualification/capella-exit-wire-20260929/README.md).
