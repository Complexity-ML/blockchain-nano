# CI et limites de qualification

Le workflow `Nano qualification checks` ne déploie rien, ne publie aucune image et n'accède ni au VPS ni aux clés de validation. Il utilise des actions officielles épinglées par commit complet et `contents: read`, conformément aux [recommandations GitHub](https://docs.github.com/en/actions/reference/security/secure-use). Checkout ne conserve pas les credentials.

Sur push main/PR touchant sources, dépendances, recettes de build, tests, scripts ou workflow, il exécute les tests autonomes de `scripts/operations/*.test.mjs`, `scripts/ci/*.test.mjs` et `tests/multihost/*.test.mjs`. Node est fixé à 25.9.0, version déjà qualifiée par les campagnes locales. Aucun daemon, Docker ou SSH vivant dans ce job. Logs et révision sont archivés sept jours, y compris en cas d'échec.

Le classement des changements par diff Git déclenche automatiquement le job C++ pour les sources, headers, réseau, fixtures, tests C++, dépendances, recettes et workflows. Un changement de documentation seul ne déclenche pas le workflow ; un changement du harness JS seul évite la compilation C++ inchangée. Une baseline invalide ou un checkout non lié au SHA attendu échoue explicitement. Le déclenchement manuel avec `core=true` reste disponible. Le job compile Nano et les exécutables de neuf contrôles C++ : core, chain, expiration de file RPC, vérification/agrégation BLS, checkpoint de stockage, état/payload Capella couplés, transition signée et horloge Capella. Le registre CTest doit contenir exactement les neuf noms attendus ; aucun résultat vide ou test manquant ne vaut réussite. Dans un conteneur Ubuntu 24.04 épinglé par digest (2 CPU/3 GiB), sans construire une image Nano supplémentaire. Build deux tâches, tests série, budgets CTest inchangés, job limité à 30 minutes. JUnit, liste des tests, versions et logs conservés. Pas de nouvelle image Docker à chaque push.

Commandes locales depuis un checkout :

```sh
sh scripts/ci/operations.sh
CC=clang CXX=clang++ bash scripts/ci/core.sh build-ci
```

Ces contrôles ne sont pas la CI complète de production : P2P compilé avec cpp-libp2p, matrice Linux ARM64/AMD64 complète, EEST exhaustif, release reproductible, campagnes WAN/restauration/charge longues et revue indépendante restent des gates séparés. Apt, runner et outils système ne sont pas figés en snapshots : la version compilée est identifiable, pas prouvée reproductible bit à bit. Les logs de configuration, compilation et tests sont diffusés en direct avec `tee` et `pipefail` : un échec ne peut pas être masqué par l’archivage. Les SHA-256 des neuf exécutables et les versions des paquets de build sont conservés. Le routage automatique ne remplace pas une protection de branche. Les paramètres de protection de branche et le statut obligatoire des checks ne sont pas configurés par ce fichier.

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
