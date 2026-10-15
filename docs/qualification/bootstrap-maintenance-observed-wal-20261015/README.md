# Reprise CLI/WAL publique sur copies possédées

Deux CTests sont qualifiés sur chacun des profils Linux AMD64 et ARM64 ci-dessous : six interruptions SIGKILL cold/fragmented et trois fautes fsync ciblées. Les trois surcouches modifient uniquement le consumer de test, son shim d’observation/faute et CMake (deux sélecteurs Linux et Threads pour le shim). Le code de la CLI de production, les 143 unités core, leurs formats et leur ABI restent inchangés. Ce résultat ne qualifie pas une restauration complète ou un redémarrage du nœud original.

| Profil réel | Résultat admis | Lignes / appels CLI / SIGKILL joints | RSS mesuré |
| --- | --- | --- | --- |
| Linux AMD64, image de dépendances existante sur VPS | 2 CTests PASS ; admission post-test distincte 64213 | 11 / 36 / 6 | 25372 KiB |
| Linux ARM64, nouveau volume local Linux Docker possédé | 2 CTests PASS ; contrôleur 84928 EXIT0 | 11 / 36 / 6 | 23804 KiB |

Les trois campagnes ont réellement exécuté six CTests : les quatre PASS qualifiés ci-dessus et deux échecs historiques ARM sur Mac bind. Ces échecs dans la copie des fixtures DB restent FAIL, cause inconnue. Le probe public de fichiers a réussi sur Mac bind et volume Linux ; il ne reproduit ni n’explique l’échec du workflow DB. Le profil volume Linux est qualifié séparément, sans qualifier le profil Mac.

AMD a réellement passé les deux CTests lors de 88543, mais le contrôleur initial est resté EXIT1 car JUnit tronquait les lignes sémantiques. Le finaliseur distinct 64213 a vérifié les deux résultats JUnit et les onze lignes complètes du LastTest log sans rejouer les tests. ARM 84928 a terminé en 32.70345737505704 secondes, avec 68001 anciens matériaux/SHA inchangés, les nouveaux octets relus après la fence des anciens inputs et le volume/conteneurs vérifiés absents. Son usage disque maximal observé est 6924 KiB sur treize observations : ce n’est ni un pic exhaustif ni un quota dur. Le lancement antérieur 62891 reste EXIT1 avant toute création de volume ou tout test, sur le seul ctime de sa nouvelle enveloppe ; sa correction garde stricts tous les anciens inputs.

Deux compilations de consumer/shim et deux liens ont effectivement terminé par architecture. AMD a produit un terminal build réussi. Le worker ARM a réussi puis son contrôleur a refusé sur les métadonnées d’une nouvelle sortie ; ROOT a admis distinctement ces sorties courantes sans recompilation. Le test ARM sur volume Linux réutilise ces ELF qualifiés : aucune nouvelle compilation ou lien n’a accompagné le changement de profil. Les builds vérifient les véritables dépendances M/MD, les entrées de liens/SDK et la CLI ELF retenues du lot OPS. Les 143 unités core, l’archive, P2P et les images ne sont pas reconstruits ; OPS et les treize anciens scénarios ne sont pas rejoués.

La provenance par architecture est fixée dans scoped-results.json : trois sources gelées `9c36f942…`, contrat `8d8a0a6d…`, constructeurs AMD `884daa42…` et ARM `4e585224…`, nouveau contrôleur de test ARM `12684965…`, images de dépendances immutables ARM `e30ce85e…` et AMD `fd9b0bb9…`. Le SHA `4a53f54d…` identifie le CPP de la CLI, jamais son ELF. Les hashes ELF/SDK complets et fermetures de compilation restent dans leurs reçus qualifiés locaux ou VM ; les seules synthèses ROOT publiques filtrées sont dans evidence/. Les statuts PENDING des propositions d’origine sont historiques et ne remplacent pas les résultats actuels de ce rapport.

Pour reproduire la sélection avec CMake sous Linux, dans un environnement disposant des dépendances du projet :

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target nano-bootstrap-archive-maintenance-tests --parallel 2
TMPDIR=/chemin/vers/scratch-linux-possede ctest --test-dir build -j 1 \
  -R '^(storage_capella_bootstrap_archive_owned_copies_crash|storage_capella_bootstrap_archive_retention_fsync)$' \
  --no-tests=error --output-on-failure \
  --test-output-size-passed 1048576 --test-output-size-failed 1048576
```

Cette commande reproduit la sélection, pas les admissions de provenance ROOT. Un scratch Linux neuf, privé et possédé est requis ; les CTests restent bornés à 130/70 secondes, les processus à moins de 120 secondes et 131072 KiB, les fixtures à 64 MiB logiques. Le profil ARM mesuré utilise deux CPU, mémoire Docker 512 MiB, réseau absent, racine/sources/build en lecture seule et capacités retirées.

Les scénarios utilisent des fixtures publiques et des preuves déjà signées sur des copies neuves : aucune nouvelle signature, clé privée opérationnelle, donnée de chaîne privée, RPC, réseau ou duty n’est utilisé. L’observation du WAL appartient au SDK réellement lié ; la perte de suffixe volatile est une simulation bornée. Les fautes injectées ne prouvent pas une perte électrique physique, une file non vide, la sauvegarde/restauration complète des originaux ou la finalité. Le lot n’active aucun fork/EIP et ne qualifie pas l’exploitation ni la production.

history.json conserve les refus avant compilation, les corrections de contrôle des entrées/sorties et les admissions distinctes. Aucun échec n’est réétiqueté PASS. Les quatre corps documentaires entiers et le CSV restent byte exacts, avec les anciennes limites mémoire et preuves partielles. NAS non rétabli, originaux Mac à conserver, anciennes clés arrêtées, V37 FAILED, finalité à trois nœuds, duties privées, audit externe et CI limitée par la facturation restent ouverts selon leur historique. Le diagnostic privé 28722 demeure inconnu et n’a pas été relu ou contourné.

La livraison fournit les trois sources à leurs chemins du dépôt. Le petit paquet documentaire contient leur patch, le contrat, de petites revues statiques et des reçus publics typés. Les répertoires source/, documents/ et preserved/ du dossier de livraison sont des artifacts d’import ; ils ne sont pas dupliqués sous le paquet Git. Aucun contrôleur ROOT, table de parents, inventaire privé, métadonnée VM brute, SDK, ELF ou journal privé n’est publié. Chaque document ajoute seulement un préfixe à son corps historique intégral ; l’unique CSV est inchangé. Ce rapport fixe les sources par leurs hashes ; la livraison Git se vérifie séparément.

La revue indépendante des résultats ARM compare JUnit, LastTest et les onze lignes sauvegardées au contrat. Les observations physiques de conservation et de nettoyage restent celles de ROOT sous les gardes relues ; le reviewer n’a pas rescanné tous les fichiers physiques. Pour AMD, seules les attestations ROOT filtrées sont utilisées : les durées du finaliseur ne sont pas des durées des CTests, qui restent inconnues dans ce rapport.
