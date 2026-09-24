# Sources KZG utilisées par Nano

- c-kzg-4844 v2.1.8, commit `e125905e5e01186e6ccb7a0ced4845bf7eddbcfe`, dépôt https://github.com/ethereum/c-kzg-4844.
- blst, commit `e7f90de551e8df682f3cc99067d204d8b90d27ad`, sous-module épinglé par cette version.
- Sources nécessaires à la compilation copiées sans modification ; empreintes individuelles dans `UPSTREAM.json`.
- Licences Apache-2.0 conservées dans `LICENSE` et `blst/LICENSE`, également installées avec Nano.

`CMakeLists.txt` est l'intégration propre à Nano. Elle compile statiquement les bibliothèques C et assembleur, avec le mode portable de blst. Le cœur reste en C++20. La compilation a été vérifiée sur macOS arm64 avec CMake et Make ; les autres plateformes restent à vérifier.

Le fichier public `src/trusted_setup.txt` provient du même commit c-kzg. Son SHA-256 est `d39b9f2d047cc9dca2de58f264b6a09448ccd34db967881a6713eacacf0f26b7`. CMake refuse une empreinte différente et génère les tableaux intégrés au binaire. Aucun fichier de paramètres, téléchargement ou secret n'est requis à l'exécution. Modifier les paramètres est un changement de règles cryptographiques à examiner et versionner.

Nano utilise `verify_kzg_proof` pour le précompile Cancun d'évaluation ponctuelle. Cette intégration n'implémente pas à elle seule la disponibilité, la conservation ou la propagation des blobs. La présence des sources EIP-7594 dans la bibliothèque ne signifie pas que cette EIP est activée dans Nano.

Le même blst épinglé est utilisé directement par `src/bls.cpp` pour les sept
précompiles Prague EIP-2537. Les multiplications multiscalaires utilisent
Pippenger (multiplication directe pour un seul point), les coordonnées sont
validées avant conversion et les contrôles de sous-groupes sont appliqués aux
MSM et appariements. Cette utilisation ne dépend pas du setup KZG et ne modifie
aucune source amont. Son activation est limitée au profil de qualification Prague.
