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
