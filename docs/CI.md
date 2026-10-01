# CI et limites de qualification

Le workflow `Nano qualification checks` ne déploie rien, ne publie aucune image et n'accède ni au VPS ni aux clés de validation. Il utilise des actions officielles épinglées par commit complet et `contents: read`, conformément aux [recommandations GitHub](https://docs.github.com/en/actions/reference/security/secure-use). Checkout ne conserve pas les credentials.

Sur push main/PR touchant sources, dépendances, recettes de build, tests, scripts ou workflow, il exécute les tests autonomes de `scripts/operations/*.test.mjs`, `scripts/ci/*.test.mjs` et `tests/multihost/*.test.mjs`. Node est fixé à 25.9.0, version déjà qualifiée par les campagnes locales. Aucun daemon, Docker ou SSH vivant dans ce job. Logs et révision sont archivés sept jours, y compris en cas d'échec.

Le classement des changements par diff Git déclenche automatiquement le job C++ pour les sources, headers, réseau, fixtures, tests C++, dépendances, recettes et workflows. Un changement de documentation seul ne déclenche pas le workflow ; un changement du harness JS seul évite la compilation C++ inchangée. Une baseline invalide ou un checkout non lié au SHA attendu échoue explicitement. Le déclenchement manuel avec `core=true` reste disponible. Le job compile Nano et les exécutables de neuf contrôles C++ : core, chain, expiration de file RPC, vérification/agrégation BLS, checkpoint de stockage, état/payload Capella couplés, transition signée et horloge Capella. Le registre CTest doit contenir exactement les neuf noms attendus ; aucun résultat vide ou test manquant ne vaut réussite. Dans un conteneur Ubuntu 24.04 épinglé par digest (2 CPU/3 GiB), sans construire une image Nano supplémentaire. Build deux tâches, tests série, budgets CTest inchangés, job limité à 30 minutes. JUnit, liste des tests, versions et logs conservés. Pas de nouvelle image Docker à chaque push.

Commandes locales depuis un checkout :

```sh
sh scripts/ci/operations.sh
CC=clang CXX=clang++ bash scripts/ci/core.sh build-ci
```

Ces contrôles ne sont pas la CI complète de production : job P2P frais encore à qualifier, matrice Linux ARM64/AMD64 complète, EEST exhaustif, release reproductible, campagnes WAN/restauration/charge longues et revue indépendante restent des gates séparés. Apt, runner et outils système ne sont pas figés en snapshots : la version compilée est identifiable, pas prouvée reproductible bit à bit. Les logs de configuration, compilation et tests sont diffusés en direct avec `tee` et `pipefail` : un échec ne peut pas être masqué par l’archivage. Les SHA-256 des neuf exécutables et les versions des paquets de build sont conservés. Le routage automatique ne remplace pas une protection de branche. Les paramètres de protection de branche et le statut obligatoire des checks ne sont pas configurés par ce fichier.

État initial : 227 tests autonomes passent localement. Exécution réelle du workflow GitHub et build C++ restent non qualifiés jusqu'à leurs résultats terminaux. Aucun workflow vert ne déclare `productionReady=true`.

Premier déclenchement C++ sans conteneur annulé avant qualification (run 36886165867), recette remplacée pour respecter le build Docker demandé. Aucun succès C++ revendiqué pour ce run.

Première exécution GitHub réelle : 227 tests autonomes réussis, run
[36885989682](https://github.com/Complexity-ML/blockchain-nano/actions/runs/36885989682).
[Preuves récupérées de GitHub](qualification/ci-20261001/operations-github/README.md).
Le job C++ et la CI globale ne sont pas qualifiés par ce résultat.

Premier Docker core : refus Git avant CMake (ownership du checkout),
run 36886531310 terminé en échec. Lecture de révision corrigée avec
exception de confiance limitée au checkout, toolchain make explicite.
[Diagnostic conservé](qualification/ci-20261001/core-first-docker-failure/README.md).
Requalification C++ toujours requise.

Le build C++ Docker corrigé a réussi : neuf tests exécutés, aucun échec/skip,
run [36887129414](https://github.com/Complexity-ML/blockchain-nano/actions/runs/36887129414).
[Artefacts réels](qualification/ci-20261001/core-docker-success/README.md).
Le sous-ensemble documenté est qualifié, pas la CI complète ni la reproductibilité.
L'archivage passe à [upload-artifact v6 Node24](https://github.com/actions/upload-artifact/tree/b7c566a772e6b6bfb58ed0dc250532a479d7789f)
épinglé, pour supprimer l'annotation de runtime obsolète ; aucun C++ runtime
ne change et aucune nouvelle image de chaîne n'est nécessaire.

Le workflow avec upload-artifact v6 a terminé avec succès, run
[36889258299](https://github.com/Complexity-ML/blockchain-nano/actions/runs/36889258299),
227 tests autonomes passés et artefacts effectivement récupérés.
[Preuves](qualification/ci-20261001/artifact-node24-success/README.md).
Aucun rebuild C++ pour ce changement CI seul ; CI globale toujours ouverte.

### Routage automatique — validation locale

232 contrôles autonomes passent localement, sans échec ni skip. Cinq nouveaux contrôles vérifient le diff réel, les renommages, les baselines, le déclenchement manuel et la propagation d’un échec de configuration à travers `tee`. La validation GitHub du nouveau routage et de ses artefacts reste requise. La sélection C++ demeure les neuf tests déjà documentés ; aucune nouvelle couverture du runtime n’est revendiquée.


### 2026-10-01 — CI C++ automatique et provenance qualifiées

Run `36890479029`, source `dad9a14`, completed/success. Routage push réel
vers Docker, 232 contrôles autonomes réussis, neuf tests C++ réussis sans
skip (27,44 s), neuf empreintes de binaires et paquets de build récupérés
et vérifiés. Job core 12m06s. Portée C++ inchangée : nouveau gate CI/provenance,
pas une nouvelle qualification complète du runtime. P2P/EEST global,
reproductibilité et critères restants de production toujours ouverts.
[Preuves](qualification/ci-20261001/automatic-core-success/README.md).

Le contrôle rapide `check-notices.mjs` vérifie les onze notices directes et tous les copyrights SPDX du header JSON ; son rapport est archivé par le job operations. Validation GitHub de cet ajout encore requise.


### 2026-10-01 — livraison des notices sur les deux architectures

Après push/pull de `930bf00`, assemblage AMD64 réel réussi sur VPS sans
compilation, téléchargements ou données de chaîne. Image `6eb8175c...` :
onze notices vérifiées, UID/GID 10001, CLI help réussi, deux binaires
strictement inchangés. ARM64 déjà vérifié (`7b3db08d...`).
Le job operations GitHub `36892286298` réussit : 232 contrôles, aucune
erreur/skip, rapport notices réel (11 empreintes/5 copyrights). Job core
encore en cours au relevé : aucun succès terminal de ce run revendiqué.
Correction de livraison qualifiée ; Dockerfile complet, SBOM transitif,
reproductibilité, régression complète et production globale restent ouverts.
[Preuves](qualification/runtime-notices-20261001/README.md).


### 2026-10-01 — job CI transport ajouté, recette ARM64 qualifiée

Routage réseau automatique et manuel séparé ; Docker Ubuntu épinglé,
2 CPU/3 GiB, builds deux tâches, registre exact des 21 tests réseau,
CTest série avec budgets existants (60 s pour un test sans budget explicite),
logs live, JUnit, révision cpp-libp2p et empreinte nano-p2p archivés.
Recette Docker ARM64 avec dépendances/volume réutilisés : 21 tests passés
sans skip, 1,02 s, binaire inchangé. Huit contrôles de routage/gate passent
localement, dont trois nouveaux. Premier refus pour Python absent conservé ;
contrôles JSON déplacés vers CMake déjà présent, sans nouvelle dépendance.
Construction fraîche GitHub et recette AMD64 restent requises. Ce gate
ne qualifie pas le WAN, la matrice EEST globale ou la production complète.
[Preuves bornées](qualification/network-ci-20261001/arm64-prebuilt/README.md).

Le job `network` compile cpp-libp2p depuis le pin contrôlé et les targets de `network/`, puis exécute ses 21 tests. Les changements de sources/headers partagés, réseau, toolchain, JSON, dépendances, recettes réseau et workflows le déclenchent ; un changement de docs ou de fixture du cœur seul évite ce build. `workflow_dispatch` permet `network=true` indépendamment de `core`.

Qualification locale avec un builder existant : `NANO_CI_REUSE_DEPS=1 NANO_DEPS_DIR=/src/.deps bash scripts/ci/network.sh /build/network`. Le mode réutilisé exige le préfixe installé et le cache CMake, vérifie le pin et l’indique dans le rapport. GitHub utilise le mode frais par défaut. Aucun cache binaire partagé ni image publiée par le job.

Le gate réseau refuse aussi les propriétés CTest de désactivation/saut.
Vérificateur final appliqué au registre réel sauvegardé, puis huit contrôles
de routage/gate réussis ; aucun rejeu des 21 tests inchangés pour cette garde.


### 2026-10-01 — CI de la correction de notices terminée

Run `36892286298`, source `930bf00`, completed/success : 232 contrôles
autonomes et neuf tests C++ passés sans échec/skip (26,91 s). Rapport réel
11 notices/5 copyrights vérifié. Neuf empreintes de binaires identiques
au run `dad9a14` : répétabilité bornée de deux builds CI, pas encore une
release entièrement figée. P2P CI du commit `85a4a7c` reste à qualifier.
Production globale et ses autres critères conservés ouverts.
[Preuves terminales](qualification/ci-20261001/notices-core-success/README.md).


### 2026-10-01 — recette transport Docker AMD64 qualifiée

Après push/pull `85a4a7c`, registre complet de 21 tests réseau vérifié,
21 tests passés sans skip sur VPS Docker AMD64 (1,24 s), binaire nano-p2p
inchangé. Builder/dépendances/volume réutilisés, aucune nouvelle image.
Deux refus d’environnement avant tests conservés : point de montage puis
propriétaire du répertoire temporaire de rapports. Protections Docker
conservées. ARM64 et AMD64 réutilisés qualifiés ; construction fraîche
GitHub encore requise. Son job operations réussit avec 235 contrôles
autonomes, dont trois nouveaux contrôles du job réseau ; core/network
encore en cours au relevé. Pas de gate WAN ou production globale clos.
[AMD64](qualification/network-ci-20261001/amd64-prebuilt/README.md),
[contrôles GitHub](qualification/network-ci-20261001/github-operations/README.md).
