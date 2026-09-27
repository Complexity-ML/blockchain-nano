# EIP pour l’EVM et la production Nano

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
