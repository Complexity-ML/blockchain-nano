# Démarrage Capella et récupération des fichiers SST

Le correctif C++ du commit `68d64a5330d16c971dd603b56e0f14c88c1459c8` et les deux nouveaux modes de test passent dans Docker ARM64 sur Mac et Docker AMD64 sur le VPS. Les images existantes sont réutilisées. La qualification porte sur ces scénarios ; la production et la nouvelle candidate complète restent ouvertes.

## Correctif de démarrage

Les trois booléens de maintenance sont maintenant validés avant toute lecture du profil, ouverture du témoin de reprise ou publication d’un checkpoint. Leurs valeurs par défaut et leurs applications sont conservées.

Sur chaque architecture, les trois nouveaux groupes vérifient 31 refus attendus, sans démarrage du runtime ni création de témoin. Le même objet de test lié à l’ancien Core reproduit le défaut : exit 1, stdout vide et stderr exact `maintenance refusal created witness or execution checkpoint` suivi de LF. Les cinq anciens groupes ne sont pas rejoués.

Chaque qualification initiale effectue quatre analyses de dépendances `-M`, quatre compilations avec vrais MD, une mise à jour d’archive, quatre liens dont celui du contrôle ancien Core, et trois `ldd`. M et MD concordent littéralement : ARM 518/529/530/551 dépendances ; AMD 516/527/523/552. Seul `consensus_capella_runtime.cpp.o` est remplacé parmi 143 membres ; les 142 autres restent identiques à l’octet près. Les archives originales restent inchangées. `storage.cpp` et son API sont conservés.

| Nouvelle archive | SHA256 | Octets |
|---|---|---:|
| ARM64 | `35779b74ff3894d8857d06732caa801cb340381f4cf6dc5801074c7e372dbdf6` | 41053984 |
| AMD64 | `1687f1876ea784006b0d687db0f94e3a49b01f8edaa3a599fdf942a9fc4d1525` | 40820160 |

Voir les résultats [ARM](reports/arm/qualification.json), [AMD](reports/amd/qualification.json), leurs sorties de démarrage et leurs contrôles négatifs dans les mêmes dossiers.

## Rétention et compactage

Chaque invocation produit 64 blocs et transactions signées avec une clé de fixture publique : 65 536 vrais SSTORE réécrivent 1 024 slots. Après compactage initial, deux copies indépendantes reçoivent chacune 16 publications de checkpoint. La branche avec rétention supprime 63 états historiques, par lots de quatre au maximum, avec une ancre au bloc 64. Les états genesis et 64, les blocs, transactions, reçus, racines, tokens de checkpoint conservés séparément et toutes les autres lignes attendues sont vérifiés, y compris après réouverture en lecture seule. Les fichiers et neuf matériaux de la source initiale restent inchangés.

Chaque invocation utilise 64 signatures ECDSA de fixture et trois appels à la CLI existante de compactage hors ligne. Aucune signature opérationnelle n’est créée. Les seuils fixés avant exécution restent identiques : baseline SST d’au moins 1 Mio, gain SST et allocation d’au moins 64 Kio, durée de 120 s, RSS de 128 Mio, inventaire logique de 32 Mio/4 096 clés.

| Support des bases de test | Environnement | Durée SST (ms) | RSS (Kio) | Gain SST (octets) | Gain d’allocation des fichiers (octets) |
|---|---|---:|---:|---:|---:|
| tmpfs de 256 Mio | ARM64 | 2319 | 114156 | 4934734 | 4935680 |
| tmpfs de 256 Mio | AMD64 | 6398 | 112884 | 4934734 | 4935680 |
| Volume Linux, statfs `ext2/ext3` | ARM64 | 2463 | 77104 | 4934734 | 4935680 |
| Volume Linux, statfs `ext2/ext3` | AMD64 | 6621 | 78904 | 4934734 | 4935680 |

Les deux essais sur volume reprennent les deux exécutables déjà admis, montés en lecture seule et vérifiés avant/après par leurs neuf matériaux GNU et SHA256. Ils exécutent seulement le mode SST : aucune compilation ni répétition des 31 refus. Le nouveau montage de `/tmp` est vérifié comme volume Linux local, avec un type de filesystem différent de tmpfs/ramfs. Les résultats sont disponibles pour [ARM](reports/disk-arm/result.json) et [AMD](reports/disk-amd/result.json).

Les douze compactages conservent chacun leur engagement logique ordonné complet ; leur WAL final est nul. La comparaison avec le contrôle compacté distingue la suppression des états du seul nettoyage du WAL. L’allocation mesurée est `st_blocks * 512`. Ces essais démontrent la diminution des fichiers sur les supports testés ; ils ne garantissent pas la réduction du fichier disque Docker/APFS, une capacité NVMe suffisante en production ou une résistance à une perte électrique. Les répertoires temporaires sont vides à la fin, les conteneurs de test sont supprimés et les volumes détenus sont conservés.

## Portée de la livraison

Les sources d’orchestration exécutées sont regroupées sous `source/`. Les propositions gelées gardent leur statut d’auteur SOURCE ONLY ; les résultats Root dans `reports/` attestent séparément leur exécution. Les contrôleurs nécessitent leurs autorités et inputs sélectionnés d’origine. Ce dossier publie les résultats, sorties synthétiques et engagements ; les binaires, bases et admissions privées restent chez leurs propriétaires. `MANIFEST.json` engage les octets de chaque fichier livré.

Les preuves bd11, le manifeste d30e et les trois anciennes admissions sont conservés sur leurs identités historiques. Elles ne sont pas réattribuées aux nouveaux Core. Les 112 vérifications/20 refus RPC antérieurs restent acquis dans leur périmètre et ne sont pas rejoués. Aucun profil EVM, fork ou statut EIP n’est promu.

Restent ouverts : assemblage cohérent du nouveau Core Native et des programmes Nano/admissions ; campagne fraîche autorisée, restauration complète et reprise des duties avec finalité commune ; cycle des validateurs, partitions/WAN/charge ; fonctionnement prolongé du pruning ; relation Base–NANO, bridge et conditions réelles du lancement Doppler ; CI complète et revue de sécurité indépendante. V37/campagne08, journaux Mac absents, refus du NAS et tous les échecs antérieurs restent conservés. Le lot ne qualifie pas CMake global, tout le SDK, le DAG ou la fermeture complète du loader.

La [CI du commit source](https://github.com/Complexity-ML/blockchain-nano/actions/runs/38915047914) refuse les jobs `changes` et `operations` pour facturation avant toute étape ; `core` et `network` sont ignorés. [Observation conservée](reports/ci.json), sans relance ni modification des dépenses. L’objectif de [production](../../PRODUCTION-GOAL.md) reste actif.
