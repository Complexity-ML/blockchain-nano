# C8 — copies exécutables Docker et packaging natif CMake

18 octobre 2026, base `742626f854f0436ef6442bc5a49d9278d9f1f397`.
Les sources C++ restent `a8eee9d6d3a35b8e14e8a7f7a6cd1b9e5ee2acd7`.
Ce lot rend utilisables les copies logicielles sélectionnées et déclare dans
CMake le packaging du shim/exporter macOS. Aucune admission entière d’opérateur.

| Résultat | ARM64 Docker Mac | AMD64 Docker VPS |
| --- | ---: | ---: |
| Nouvelles copies ELF exécutables0500 | 5 | 4 |
| Octets copiés | 19 972 936 | 15 824 280 |
| Sources strictes tenues inchangées | 5 originaux compilateur | 16 anciennes copies |
| Captures full9 stables finales | 3 sur2,143s | 3 sur2,160s |
| Compilation / image / exécution des outils | 0 / 0 / 0 | 0 / 0 / 0 |

Les [reçus ARM64](arm64.json) et [AMD64](amd64.json) donnent les SHA/octet,
permissions et propriétaires. Les nouveaux dossiers sont distincts des copies
0400 et des chemins échoués. Aucun chmod d’ancien fichier. Les huit champs de
création et bytes/SHA connus sont vérifiés avant chaque observation. Les trois
captures full9 identiques sur au moins2s précèdent la première admission ; les
fences finales vérifient ensuite strictement les fichiers admis et leurs sources.

Sur Mac, source02 refuse une divergence de ctime dans les anciennes copies0400.
L’inspection Root retrouve15 fichiers dont seul ce champ diffère, avec SHA et
autres huit champs identiques ; cela ne prouve pas la cause du changement et ne
rafraîchit aucune ancienne autorité. Source03 part des cinq produits compilateur
toujours strictement conformes, mais refuse le ctime encore en stabilisation de
ses copies neuves. Source04 crée un autre dossier neuf : une variation ctime
avant la toute première admission remet seulement sa fenêtre de stabilité à
zéro. Créateurs8/SHA, délai global30s, lecteur des originaux et fichiers déjà
admis restent stricts. Quatre observations totales, une remise à zéro, puis les
trois stables requises passent. Aucun chemin échoué n’est réadmis. Les deux
échecs restent FAILED, anciennes copies conservées. AMD passe dès source02.

## Deux liens natifs conformes aux réglages CMake déclarés

Huit lignes APPLE uniquement rendent explicites l’ID `@rpath` du shim et
`BUILD_RPATH "@loader_path"` de l’exporter. Une configuration dans un nouveau
dossier termine0 ; ses sondes de compilateur peuvent compiler leurs petites
fixtures, sans reconstruire le Core ou les consommateurs de production.
Le CMake qualifié a le SHA
`8918308fd5a3f7a36a969a2ad91631203787f9f611b202312a3d7174004ae916`.

[native.json](native.json) : les sept jeux de flags normalisés sont égaux aux
recettes retenues. Six liens restent égaux ; seul l’exporter ajoute le premier
rpath déclaré. Deux liens sont réalisés avec les objets et fournisseurs
antérieurs strictement engagés : shim et exporter. Aucun des sept TU ni Core
recompilé, aucun des cinq autres outils relinké. Les53 entrées sélectionnées
restent conformes à leurs observations tenues ; les cinq commandes terminent0
et leurs groupes possédés sont fermés.

Les commandes générées subissent des bindings explicites vers les anciens
objets/fournisseurs, une relocation des sorties/rpath automatique et le maintien
du mode C++ lors de la résolution de `clang++`. Elles ne sont pas présentées
comme des argv générés littéralement inchangés. Le premier essai avait perdu
ce mode en lançant `clang` ; son linker refuse les symboles de bibliothèque
standard. Ce FAILED, le refus SOURCE sur la clôture des groupes et un échec du
lanceur Python avant initialisation restent conservés. Ce sont des échecs de
préparation/contrôleur, pas des preuves d’une cause de campagne03 ou d’un écart EVM.

Les inspections Mach-O confirment l’ID
`@rpath/libnano-test-sync-fault.dylib`, son nom de chargement dans l’exporter et
le premier `LC_RPATH @loader_path`, avec les deux produits colocalisés. Il s’agit
d’une vérification structurelle du chargement ; l’exporter n’est jamais exécuté.
Le Core ec2d1206… reste inchangé. Le shim partagé peut transmettre cet ID à ses
autres futurs consommateurs ; aucune nouvelle qualification de ceux-ci déduite.

## Portée et suites

[history.json](history.json) engage les contrôleurs et échecs conservés. Les
logs complets, produits et reçus privés restent accessibles sur leurs machines.
Les deux objets réutilisés conservent la limite publiée : leur première
observation indépendante avait été postérieure au terminal07. Aucun SDK
historique de fournisseur ou SDK/dyld global/reproductibilité n’est promu.

Restent la jointure entière24 actifs, les admissions exactes des trois opérateurs,
les compléments de provenance nécessaires et la restauration entière avec
reprise des duties/finalité commune avançante. Les pointeurs originaux P2P Docker
sont retrouvés, sans nouvelle jointure actuelle revendiquée. Runtime1925 et
Paris/Shanghai inchangés, aucun EIP/fork promu. Campagne03 FAILED/accord consommé,
aucune04 autorisée. Tous les autres critères normatifs, CI/facturation,
supervision/SIGABRT, revue indépendante non réalisée et Base–Nano proposé/bridge
USDC/Doppler/T0 restent ouverts selon leur périmètre. Aucun fonds, clé nouvelle,
vente, réseau de chaîne ou déploiement public. Objectif ACTIF, production non
qualifiée ; préfixes normatifs et historique conservés.
