# Qualification de la candidate sur les trois environnements — 13 octobre 2026

Les compilations cohérentes et les contrôles ciblés sont terminés sur Mac natif, Docker Mac et Docker VPS. Les sources de production correspondent à `c672a30d` ; `d7cc1bdb` livre uniquement la correction de type du test Darwin (`44febdb9`). Les six sources corrigées et CMake (`28fe657b`) restent identiques. Aucun profil EVM ou règle de consensus n'est activé par ce lot.

| Environnement | Archive du cœur | Binaire nano | Périmètre exécuté |
| --- | --- | --- | --- |
| Mac natif Darwin ARM64 | `90ee449b` | `2b4856b1` | Trois CTests réels sans skip, puis prepared-block direct |
| Docker Linux ARM64 sur Mac | `161a3450` | `64ca2a4b` | Disponibilité, local-sync, prepared-block et 25 gardes de capture |
| Docker Linux AMD64 sur VPS | `1b5a8673` | `31887383` | Les mêmes quatre sélections ciblées, avec logs et JUnit réels |

Chaque archive est liée aux 139 unités du cœur recompilées et à leurs sources. Les consommateurs et bibliothèques de fault-injection sont identifiés séparément. Ces contrôles ne constituent ni la sélection quotidienne complète, ni une qualification générale des dépendances ou de la reproductibilité.

Le compilateur Apple Clang a d'abord refusé le test pour une déduction de types incompatible. Cette erreur reste conservée. La correction ne change qu'une expression du test ; les consommateurs natifs ont été terminés sans recompiler le cœur déjà achevé. Sur Linux ARM64 et AMD64, une compilation de cette expression donne exactement le même objet et le même exécutable que le test déjà exécuté. Cette identité a été mesurée avant la décision de réutilisation : aucun test identique n'a été rejoué pour ce port.

Les trois nouveaux scénarios utilisent des données synthétiques. Le dernier admet 48 blocs normalement, puis retrouve une préparation disponible à la nouvelle tête ; ses 144 signatures sont celles de la fixture. La clé locale est en attente d'inscription et le fournisseur local n'est jamais appelé. Ce résultat ne démontre aucune reprise de duty signée, de réseau restauré ou de finalité commune. Sur AMD64, il termine en 33,296 s avec 32 512 KiB RSS (31,75 MiB), sous les limites internes inchangées de 60 s et 128 MiB. L'ancien refus du format plat à 137 524 KiB reste ouvert.

Les paquets suivants sont conservés à l'identique. Le paquet Mac a été constitué avant la fin AMD64 : sa mention historique de ce prérequis ouvert est remplacée par le présent état courant, sans modifier son manifeste.

- [Mac natif et Docker ARM64](mac-platforms-public/README.md), manifeste `8881105c` : 278 payloads publics.
- [Docker AMD64 et portabilité](amd64-public/README.md), manifeste `e9dec5b8` : 237 payloads publics.
- [Revue technique interne Mac/ARM](mac-platforms-public/independent-review/terminal-review.json) et [complément des liens générés](generated-links-and-d7-source-addendum.json).
- [Revue technique interne AMD64](independent-amd64-review/terminal-review.json), manifeste `5047c38a`.
- [Exclusion des données privées](amd64-private-exclusion-review/terminal-review.json), manifeste `1c0e5e4c`.
- [Refus actuel de la CI source](source-ci-refusal-d7/terminal-review.json), manifeste `508f7262`.

La CI GitHub du commit `d7cc1bdb`, run [38325619830](https://github.com/Complexity-ML/blockchain-nano/actions/runs/38325619830), est refusée avant toute étape. GitHub indique un paiement récent échoué ou une limite de dépenses, sans préciser lequel. Zéro étape ou test CI exécuté ; aucun réglage de facturation modifié.

Les compilations et les preuves publiques nécessaires au futur garde de reprise avancent ; le garde v2 reste une préparation en workspace. Sa conversion en autorité réelle doit encore lier les objets et sorties des consommateurs et les bytes des membres d'archive. Les observations `ar-t` ne sont pas présentées comme une comparaison `ar-p` indépendante. L'allowance administrative proposée de 4 h n'est pas activée.

Les quatre preuves privées VPS restent sur le VPS, en attente d'autorisation d'export. Aucun binaire, objet, base, journal privé, clé, environnement ou capsule n'est livré dans ces paquets. Les sources de restauration et leurs témoins ne sont pas remplacés par GitHub.

V37 FAILED, reprise réelle des duties, finalité commune entre trois environnements, partitions et ressources, pruning, exploitation, CI/release, Base–Nano/USDC/Doppler et revue de sécurité externe restent ouverts. Les matrices EIP et tous les critères historiques restent inchangés. Aucun fonds réel, vente ou déploiement public ; objectif de production actif.
