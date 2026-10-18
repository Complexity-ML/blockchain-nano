# C8 Native et helper RocksDB — qualification par étapes

Le C++ de production reste celui de `a8eee9d6d3a35b8e14e8a7f7a6cd1b9e5ee2acd7`. Seul le helper de tests `tests/support/rocksdb.hpp` est corrigé (`a128ab10f79c3207412dc0a8a224ee699d865139b1b0cc9fdd0da8de5aa96376 /716`) : `requires` sélectionne l’API Open disponible sans changer les options, les assertions ou le stockage de production. Root a exécuté les opérations et vérifié les reçus archivés ci-dessous.

## Résultats natifs distincts

| Lot | Résultat et périmètre |
| --- | --- |
| [Native01](native01-FAILED/result-public.json) | FAILED avant configure, zéro compilation : identité ctime refusée sur les fichiers fraîchement matérialisés. |
| [Native02](native02-FAILED/result-public.json) | FAILED : génération CMake zéro, build sortie2 sur l’ancien helper DB** incompatible avec RocksDB11.8.1. Six écarts ctime de dotfiles sont aussi conservés séparément. |
| [Native03](native03/result-public.json) | Configuration et compilation des onze cibles sélectionnées réussies, avec143 objets Core neufs. CTest : cinq PASS, deux FAILED ; le lot entier reste FAILED. |
| [Native04](native04/result-public.json) | PASS séparé sur les mêmes binaires : deux cas refusés rejoués directement, probe CLI initial, installation et observation des dépendances. Zéro build, zéro rejeu des cinq succès antérieurs. |

Les cinq CTests réussis de Native03 sont chain (185 checks), block-auth, recovery-audit Paris, recovery-audit Shanghai et Beacon gate. L’API RO refuse un changement ctime dans sa clôture d’identité ; bootstrap retourne `Invalid Capella bootstrap proof input`. Le second diagnostic ne suffit pas à identifier une cause précise. Le changement de TMPDIR entre les lots ne prouve ni acteur ni cause matérielle de ces échecs. [Sortie CTest originale](native03/ctest.log), [diagnostics initiaux](initial-diagnostics-summary-public.json).

Native04 utilise un nouveau TMPDIR canonique sous `/private/tmp`. L’API passe six groupes, onze refus attendus, deux vraies barrières d’écriture et cinq ouvertures RO ; bootstrap passe sept records. Le probe réalise18 vraies CLI : huit succès, dix refus et quinze contrôles de source. Les deux derniers tests sont des invocations directes des binaires CMake conservés, pas un nouveau reçu CTest. Huit familles sont couvertes au total : API, chain, block-auth, bootstrap, Beacon gate, recovery Paris, recovery Shanghai et probe CLI. **Aucun PASS unique de sept CTests n’est revendiqué.** API/probe/bootstrap créent zéro signature ; cela ne s’étend pas aux autres fixtures publiques du lot.

L’installation est contrôlée séparément : quatre exécutables (nano et trois maintenances), onze notices vérifiées contre le manifeste. `otool -L` liste les dépendances des quatre fichiers ; seul `nano --help` installé est exécuté, sortie0. Les trois maintenances ne sont pas exécutées. Nano-p2p n’est ni construit ni installé dans ce lot ; la documentation installée est une projection de premier niveau, pas une distribution documentaire complète. [Installation](native04/install.log), [dépendances](native04/installed-load-commands.log).

## Provenance et contrôle passif

[Audit Root](native04/root-passive-audit-public.json) : **8d9019de11224fc93e0156b61be5bae36b44f1a92328cfe9b7d59464ce1638c6 /659373**. Il observe161 unités compilées, dont143 du Core, 489 sources consommées et un fichier généré. Les9713 lignes originales de sources, logiciels et produits — dont huit produits retenus pour Native04 — restent inchangées. L’assembly BLST n’a pas de depfile émis par ce toolchain ; l’identité historique de l’ensemble des headers externes n’est pas qualifiée. Le Core est `ec2d12064b6f8606823b7d6ac53857dbd9953cca9046b800853c335a46acfc2a /29416184`. [Projection](source-projection-summary-public.json), [programmes exécutés](source/).

## Helper Linux et historique conservé

La [première probe Docker ARM](linux-helper01-FAILED/result-public.json) compile puis réussit trois Open (deux succès, un refus attendu), un Put et deux Get, mais refuse le cleanup sur un bind Mac : **FAILED conservé**. La [reprise runtime seule](linux-helper02/result-public.json), sur le même binaire et un tmpfs Linux neuf, passe opérations et cleanup sans compilation ni image reconstruite. Cette mini probe ouvre uniquement des DB synthétiques RW ; zéro Core, réseau, signature ou ancienne DB. [Source](source/rocksdb-test-helper-probe.cpp), [oracle](source/helper-probe-oracle.json).

Sur Native/RocksDB11.8.1, le wrapper est exercé par les trois cas auth/recovery. La mini probe autonome n’a été exécutée que sur Docker ARM/RocksDB8.9.1. La qualification Docker C8 AMD64 antérieure garde sa portée propre. La proposition redondante des trois licences a été [refusée en revue](notices-source-review-refusal.md) : elles étaient déjà installées par CMake ; aucun patch de notices n’est appliqué.

## Limites de clôture

Aucune portabilité physique de DB entre versions, reproductibilité bit à bit, CI globale ou nouvelle image de diffusion n’est qualifiée ici. Le runtime réseau1925 n’est pas remplacé ; restauration opérateur, duties, full Beacon ABA et progression de finalité commune restent ouverts. Paris/Shanghai reste le profil activé, Cancun n’est pas activé. La campagne privée03 FAILED est consommée ; aucune nouvelle campagne de validateurs n’est autorisée par ce lot. Revue de sécurité indépendante, critères normatifs restants et production restent ouverts.

Offre/ratio, réserve et conversion Base–Nano, bridge USDC, termes Doppler et visibilité T0 restent à décider ou qualifier. Aucun fonds réel, vente, push d’image ou déploiement public. Tous les échecs sont préservés ; objectif ACTIF.
