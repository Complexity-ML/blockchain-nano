# CI core 9e87b63b — résultat terminal

Run 37231449042, job core completed/success. Révision exacte vérifiée,
sélection/JUnit concordants et noms attendus : 9/9 sans échec/skip.
Build Release Linux AMD64, logs/packages/compiler/CMake et hashes publiés
conservés byte-exact (gzip pour gros fichiers), archive-manifest.json.
Aucun binaire n’est contenu dans cet artefact : ses hashes publiés ne sont
pas une vérification indépendante des bytes ni une preuve de reproductibilité.

Le workflow modifié déclenche conservativement le sous-ensemble C++ et réseau.
Aucun changement runtime src/include/network/cmake/third_party/CMake/dependencies
entre 674a23e0 et 9e87b63b ; ce job n’élargit pas le périmètre des neuf cas.
Il ne remplace pas les 427 tests de la candidate efbeb9bc, la restauration
WAN, les nouveaux tests pruning ou les gates de production. Réseau encore actif.
