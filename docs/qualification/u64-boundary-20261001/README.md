# Conversion uint64 — nouvelle couverture ARM64

Branche qualification/u64-boundary-20261001, commit 6e5c1400 : contrôle exact signe/largeur avant conversion, sans reparsage du maximum décimal. 136 nouvelles vérifications : zéro, maximum, chaque bit et son prédécesseur, six valeurs négatives/trop larges. Le chemin unsigned long de 32 bits utilise un export natif ; ce chemin n’est pas qualifié sur une plateforme LLP64.

Docker ARM64 : core 4 005 checks, RPC 1 007 checks, EEST 317 fichiers / 5 387 correspondances d’exécution / 505 validations, zéro divergence/non supporté/erreur. La preuve EEST reste un sous-ensemble local ; forkConformanceProven=false. Les objets types et core test sont recompilés, les exécutables sont relinkés avec les bibliothèques du candidat ea9667f6 en lecture seule ; ceci n’est pas une reconstruction CMake complète de release.

Première exécution core/RPC : erreur de lecture des fixtures depuis le mauvais répertoire, conservée dans preparation-a. Même binaire rejoué depuis /work, conforme au WORKING_DIRECTORY des CTest ; empreintes inchangées. EEST réussi dès la première exécution, non rejoué.

Microbenchmark isolé de cinq millions de conversions, checksum identique : 138,034 ms avant / 8,099 ms après. Ce gain ne mesure pas le temps EVM complet ou la charge RPC AMD64. Qualification AMD64, charge, release cohérente et régression complète restent ouvertes.
