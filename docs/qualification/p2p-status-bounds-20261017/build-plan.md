# P2P status : compilation ciblée depuis les SDK retenus

Lecture source/publique uniquement : aucun SDK, objet, exécutable ou descripteur privé ouvert ; aucun Git, compilateur, test, Docker, SSH, réseau ou fichier R exécuté/modifié. Les chemins privés ci-dessous sont des locators à joindre par Root, pas des autorités relues ou réadmises par cet avis.

## Périmètre exact

Local prépare `network/main.cpp`, `peer_status.hpp`, `json_stream.hpp` et les deux TU existantes `test_peer_status.cpp` / `test_json_stream.cpp`. Leurs nouveaux modes sont `--status-bounds-self-test`. `network/CMakeLists.txt:173–176,191–194,207–212` suffit :

| Cible | TU | Lien |
| --- | --- | --- |
| `nano-p2p` | `main.cpp` | `p2p::p2p`, `FMT_CONSTEVAL=` |
| `nano-peer-status-tests` | `test_peer_status.cpp` | aucun `p2p::p2p`, includes third_party |
| `nano-json-stream-tests` | `test_json_stream.cpp` | `p2p::p2p`, `FMT_CONSTEVAL=` |

Toutes sont C++20. Aucune ne lie `nano_core`. Le minimum candidat est donc **3 M, 3 CXX, 3 liens**, sans Core/ar/SDK rebuild. Les deux nouvelles TU contre les anciens headers peuvent ajouter deux M/CXX/liens de causalité séparés. Le changement de main et de ses headers impose un nouvel objet main : aucun réemploi de l’ancien M/MD ou objet ne prouve le candidat.

## Recettes et autorités localisées

**Natif.** Root confirme le build retenu `R/.deps/p2p-source-20261016-native-02/source-sdk-consumer-qualification-v1/arm64`, `compile_commands.json` à 38 entrées et `CMakeFiles/<target>.dir/link.txt`. Le contrôleur public `E/qualify-native02-source-sdk-consumer-root-20261016-v2.py` donne cette route et l’ordre réel des 101 archives du consommateur. Sa preuve publique `E/native02-sdk-consumer-result-public-20261016.json` lie `inputs-post-fence.private.json` SHA `9864349325fb58a86504ba3c20b922f030f8c57bd32e91132eb40cc47c1ac163` /363727 ; Root rapporte 902 entrées à préserver. Ne pas réutiliser le proxy compilateur avec son ancien callRoot : Root sélectionne sa vraie clang déjà retenue. SDK à fournir à la configure neuve : `R/.deps/p2p-source-20261016-native-02/deps`, architecture arm64 et sysroot/compilateur explicitement retenus.

**Docker ARM.** Producteur extérieur à l’image : `E/fullnode-cmake-package-arm64-20261015/network-build/`. Les recettes attendues sont `compile_commands.json`, `CMakeFiles/{nano-p2p,nano-peer-status-tests,nano-json-stream-tests}.dir/{flags.make,link.txt}` ; l’ancien main est `CMakeFiles/nano-p2p.dir/main.cpp.o` et son `.o.d`. Leur présence actuelle n’est pas affirmée ici. Une commande réelle indépendante est déjà dans `receipts/sdk-network-build.stdout.log` **SHA05940d3aa1d43eefd8c91acee6e7b6a78612eb72f99d26b3b8be99b0fb7bda01 /20309**, lignes151 (compile) et154 (lien ordonné complet). Terminal `sdk-network-build-terminal.json` : `46354965bddc9165333c42f69365af30575bce3cd91682f514f19e6701fc16a6` /309. Le SDK immuable est l’image `sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb`, UID501/GID20.

Le log réel prouve les préfixes **`/src/.deps/install/{include,lib}`**, et `/src/.deps/hunter/_Base/15ca502/dff9242/9613f97/Install`, dont Boost `-a64`. Ce n’est pas `/src/.deps/libp2p-install`. Le cache SDK `/src/.deps/libp2p-build/CMakeCache.txt` fournit Boost_DIR au CMake actuel ; ce cache n’est pas une base de recettes du consommateur main.

**VPS AMD.** Source publique de la route : `E/fullnode-cmake-amd64-init-proposal-v2-20261015/init-public-build-AMD.py:67–75` et `E/audit-fresh-amd-public-network-artifacts-root-20261015.py`. Le build réel se trouve à `/home/ubuntu/nanobees-qualification/fullnode-cmake-package-amd64-20261015/network-build`, avec les mêmes noms relatifs de recettes/objet/MD ; son source était le voisin `source/`. L’image retenue est `sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14`, UID/GID1000. Le contrôleur initial utilisait déjà `-DNANO_DEPS_DIR=/src/.deps` et construisait uniquement le consommateur P2P après sélection des bibliothèques. Les argc exacts/imports/hash Hunter AMD doivent venir de ses propres recettes ou d’une configure réseau neuve dans cette image : ne pas transformer mécaniquement la chaîne ARM `-a64` en `-x64` ni inventer son préfixe. Aucun ancien SDK map n’est nécessaire pour consulter ces seuls locators par Root.

## Route neuve si les recettes consommateur ne sont plus accessibles

La proposition Root est cohérente avec les sources : configure **network seul** dans une sortie absente, snapshot `network/` + `third_party/` connu et RO, Release, `CMAKE_EXPORT_COMPILE_COMMANDS=ON`, `NANO_DEPS_DIR=/src/.deps` sur Linux (ou le préfixe Native ci-dessus). Puis sélectionner exactement `nano-p2p nano-peer-status-tests nano-json-stream-tests`. Ce projet n’a ni FetchContent, ni ExternalProject, ni sous-projet Core. `network/CMakeLists.txt:9–28,70–73` lit le Boost_DIR retenu puis trouve ZLIB/qtils/secp256k1/libp2p dans install+Hunter. La disponibilité physique de ces configs/imports reste à vérifier par Root ; un manque doit refuser, sans téléchargement ni production d’une dépendance. Le namespace neuf peut générer les recettes des autres targets sans les construire.

Root prend les trois argv compile réels et leurs trois link.txt, conserve macros/includes/architecture/ordre/occurrences des bibliothèques, et ne remplace que les sorties par celles du nouveau namespace. Faire un vrai `-M` avec chaque contexte exact, puis la compilation canonique `-MD` et comparaison des ensembles normalisés M/MD ; les chemins littéraux restent conservés séparément. Puis lien complet de chaque consommateur concerné avec les archives importées inchangées. Aucune nouvelle configure globale de Nano ni compilation Core n’en découle.

## Fences et limites à conserver

L’audit public `E/audit-unchanged-p2p-reuse-root-20261017.py` donne les autorités des TROIS anciens P2P à préserver : Native binding `current-native-runtime-bindings-root-attempt03-20261016/binding-inputs.private.json` **7f9083e8…/2027120** avec providers/aliases ; ARM `current-arm-runtime-network-root-20261016/first-copy-admission.private.json` **dccedc2d…/1856** ; AMD `/home/ubuntu/current-amd-final-runtime-20261016-01/runtime-bundle-admission.private.json` **5c5bb592…/8589**. L’index public R `docs/qualification/rpc-runtime-assemblies-20261017/software-candidate-index.json` conserve Native65d08/13406616, ARMb46a7/19201120, AMDb87645/18964752. Ces pins restent historiques ; ils ne sont pas ceux du nouveau correctif.

Les anciens outputs restent RO, neuf champs/SHA et aliases/providers Native stricts, jamais réadmis. Dans un nouveau conteneur, les lignes inode/dev d’anciens fichiers du rootfs SDK ne sont pas une autorité physique transférable : mêmes images, pins des entrées réellement atteintes, observations prospectives de ce namespace et fences avant/après, sans revendication de récupération historique. Les nouveaux source/contrôles/produits seulement reçoivent leur première admission connue en octets, après stabilisation ≥3 observations sur ≥2s ; aucun refresh après admission. Conserver logs avant assertions de succès, premier refus et nettoyage des seuls processus/conteneurs possédés ; `volume-nocopy` sur tout volume neuf.

Les tests doivent nommer leur sélecteur et le vrai résultat causal (mauvais hash de statut, lecture4097 contre borne4096), sans présenter une erreur de compilation comme causalité. Le main peut subir son refus sans argument attendu1 avant identité/listener, si Root le demande. Aucun fichier d’identité, daemon, DB ou campagne n’est requis. Ces liens et tests ne qualifient pas WAN, duties, finalité ou production ; après succès il faudra un nouveau pin P2P par architecture, sans altérer les anciennes admissions24 ni annoncer une qualification du SDK entier.
