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
