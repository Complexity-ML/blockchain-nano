# Contrôle terminal du candidat cohérent 9e9ebed3

Contrôleur préparé, campagnes ARM64 et AMD64 encore actives. Aucun résultat terminal accepté, aucun WAN lancé, aucun runtime promu.

`review.py` conserve les 427 commandes et leurs timeouts du registre précédent : seul le préfixe de build et le chemin réseau désormais interne au build sont normalisés. Le hash du CMake source doit être identique. Les manifests de sources et programmes avant/après, les résultats JUnit sans omission/skip/échec, les codes terminaux et les helpers réseau sont obligatoires. `/usr/bin/cmake` reste l’unique programme système permis en dehors du build.

`run-review.sh` ajoute le contrôle exact des 5 387 variantes des 317 fixtures EEST, la revue des 21 tests réseau séparés, les sources actuelles et une relecture SHA des exécutables dans le volume conservé en lecture seule. Il ne compile pas et ne démarre aucun nœud. Les entrées de build statiques ne sont pas auditées par ce contrôleur. Les contraintes de production restent ouvertes.

15 contrôles synthétiques passent : un cas positif synthétique et 14 refus. Ils n’exécutent pas Nano et ne constituent pas une qualification de consensus/EVM. L’erreur initiale sur le programme système CMake est conservée. Le test réel du contrôleur sur la campagne ARM encore active refuse correctement l’absence du manifeste terminal (exit 1).

Les scripts d’audit EEST et réseau existants sont réutilisés, sans revendication de nouvelle couverture runtime. Lancer `bash run-review.sh SOURCE FULL_REPORTS FRESH_OUTPUT /build/full-network-9e9ebed3-ARCH-20261002 IMAGE_DIGEST BUILD_VOLUME NETWORK_REPORTS` seulement après les résultats terminaux ; sur VPS, utiliser le wrapper Docker sudo existant.
