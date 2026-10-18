# C8 : copies physiques à froid et rejeu canonique

Base documentaire et sources : main `5923aede2944228836e058cdc19a0dc34a7bc1e8`. Le C++ de production demeure `a8eee9d6d3a35b8e14e8a7f7a6cd1b9e5ee2acd7`. Seul le lecteur de qualification `tests/qualification/physical_cohort_portability.cpp` est étendu (`0e54222c1d3da23136013a35455200d4c3945e503b0ff8304dd925d68995e4b9`, 42 508 octets). Aucun Core, programme installé ou image n'est reconstruit par ce lot.

Le lecteur appelle réellement `audit_capella_canonical_history`. Il reconstruit une ancre singleton depuis la publication authentifiée en lecture seule, vérifie les archives canoniques puis confronte checkpoint, racines, compteurs et inventaires au manifeste extérieur. L'appel positif n'impose pas une finalité historique non disponible. Un second appel doit refuser exactement le checkpoint extérieur d'époque zéro, antérieur à l'ancre conservée au slot un : `observed finalized checkpoint outside replayed ancestry`.

Les deux cohortes publiques synthétiques alpha/beta sont celles déjà produites et signées, pas de nouvelles identités. Elles contiennent huit bases (primaire, témoin, journal signataire et son témoin pour chacune), cinq archives et quatorze anciennes signatures de préparation. Ce lot n'appelle jamais `produce`, ne signe rien et ne charge aucune clé. Ses blocs d'exécution sont vides : zéro transaction ne démontre aucun scénario DeFi supplémentaire.

## Résultats des trois environnements

| Destination | Source physique | Image finale conservée | Contrôleur terminal | Pic RSS du lecteur |
| --- | --- | --- | --- | --- |
| Docker ARM64/Mac | Archive Linux AMD64 | `44b81420…` | 7862, sortie 0 | 23 760 Kio |
| Docker AMD64/VPS | Archive Linux ARM64 | `3eeb59d6…` | 91770, sortie 0 | 24 884 Kio |
| Mac natif ARM64 | Archive Linux AMD64 | Mach-O C8 `0a490634…` | 32580, sortie 0 | 21 408 Kio |

Les images sont celles du lot [Docker complet C8](../complete-docker-c8-20261018/README.md), recette `27b52941`. Le lecteur supplémentaire est monté seul en lecture seule ; il ne constitue pas un sixième programme livré dans l'image. Les [résultats ARM](arm/result-public.json) et [AMD](amd/result-public.json), les états terminaux des conteneurs et l'[audit terminal Root](root-docker-terminal-audit.json) sont conservés séparément.

Chaque architecture réutilise son Core C8 et son image de compilation existants : une seule nouvelle unité C++ et un lien par architecture, aucun Core recompilé. Les dépendances sélectionnées M/MD et les anciens produits sont vérifiés avant et après. Les preuves n'affirment pas que les 1 047 lignes de l'ancien arbre source soient toutes inchangées : sur ARM, treize documents avaient un ctime différent sans changement de contenu, hors des 425 headers sélectionnés. Aucun ancien engagement n'est rafraîchi.

Chaque import écrit une seule copie neuve dans un volume nommé conservé. Les propriétaires USTAR sources sont explicitement distincts (ARM `501:20`, AMD `1000:1000`) ; les copies nouvelles appartiennent à `10001:10001`. Les anciens volumes et fichiers ne sont ni chown ni modifiés. Avant Docker, les 69 entrées USTAR, onze répertoires, huit inventaires portables et le hash du manifeste extérieur sont vérifiés ; liens, devices, chemins sortants et extensions sont refusés. Les vues d'identité hôte/invité restent distinctes.

Les copies sont admises après trois captures identiques couvrant au moins deux secondes. Après cette première admission, leurs neuf métadonnées et SHA restent stricts avant/après le lecteur et après fermeture. Tous les porteurs RW sont supprimés avant les lectures RO. Neuf conteneurs possédés sont retirés par architecture, avec identités vérifiées ; aucun volume ni image n'est supprimé. Le lecteur fonctionne sous UID10001, réseau `none`, racine et volume RO, sans capacité, port, tmpfs ou healthcheck, avec bornes 512 Mio/2 CPU/64 PID. CLI0 et State.ExitCode0, non-OOM, sont vérifiés séparément.

Chaque check réalise deux passes RO indépendantes. Alpha rejoue trois blocs dont l'ancre ; beta quatre. Checkpoint primaire/témoin, journaux/floors, signature de l'ancre et cinq archives sont authentifiés. Les compteurs de signatures nouvelles restent nuls. Les inventaires de fermeture et fichiers d'entrée originaux restent exacts ; aucune reprise automatique ou duty n'est exécutée.

## Lecture native macOS

Le [contrôleur natif](native/result-public.json) et l'[audit passif Root indépendant](root-native-passive-audit.json) passent. Une seule invocation `check`, deux passes RO, huit bases/cinq archives et zéro signature nouvelle ; les deux records de cohortes sont strictement identiques à ceux de Docker ARM qui lit la même archive AMD. Les 69 entrées nouvellement restaurées demeurent exactes après fermeture du groupe possédé. Root les réobserve ensuite indépendamment et refait les fences de onze descripteurs originaux (lecteur, archive, contrôleur/helper, Core et six providers). Le contrôleur tient douze fichiers en comptant sa configuration.

Le lecteur natif est lié au Core C8 `ec2d1206…`, sans recompiler ce Core. Native02 réutilise l'objet compilé par Native01 et corrige uniquement le driver de lien ; le refus Native01 reste FAILED. Les sept providers déclarés (Core et six bibliothèques d'origine) ne constituent pas une mesure indépendante de toute la fermeture dyld. Le périmètre ne revendique pas une provenance historique globale des headers SDK/auxiliaires de lien.

L'import manuel écrit exclusivement une arborescence neuve sous `/private/tmp`, fichiers0600/répertoires0700, UID501:GID20. Les seuls nouveaux objets reçoivent ce GID avant leurs ancres d'identité ; le brouillon sans ce réglage n'a jamais été exécuté et son [refus de préparation en source](failures/native-source-preflight.json) reste distinct d'un échec runtime. Les identités de création et octets sont vérifiés avant première admission ; seul le ctime de la nouvelle copie peut se stabiliser avant l'admission. Aucune ancienne autorité n'est rafraîchie.

Ce cas démontre la lecture RO de **cette cohorte Linux RocksDB8.9.1 sur macOS RocksDB11.8.1**. Il ne démontre pas une compatibilité générale de versions, un downgrade, un writer natif, une panne physique ou une restauration opérateur. Aucun montage OS RO natif ni fermeture réseau par le noyau n'est revendiqué : les APIs C++ sont RO, le programme ne démarre aucun réseau et les fences détectent les mutations. Budgets distincts : contrôleur300s, appel C++180s, réserve de fermeture60s.

## Limites et refus conservés

Le Genesis historique de cette fixture utilise un `extra_data` de 31 octets fixé après lecture. Le codec Genesis Beacon/Shanghai le normalise à 32 octets : cette fixture ne peut donc pas être passée telle quelle au `nano audit-capella` installé. Ce lot ne modifie pas le codec, ne refabrique pas son Genesis ni ses signatures et **ne qualifie pas cette CLI installée**. Le rejeu Core vérifié et l'interface opérateur restent des périmètres distincts.

La finalité d'époque zéro précède l'ancre conservée ; `externalFinalizedAncestorVerified=false`, `requiredFinalized=null` dans le succès canonique. Le refus explicite du second appel n'est pas une preuve positive d'ascendance finale historique. Aucun fork choice historique complet, vote autonome, duty, progression de finalité commune ou reprise opérateur n'est qualifié.

Les premières compilations Docker 01 ont refusé avant compilation sur un chemin de dépendance contenant `../support` ; la correction utilise son chemin physique déjà engagé, sans rafraîchir les autorités. Native01 a compilé son TU mais son lien a échoué parce que le contrôleur avait perdu le mode de driver C++ en résolvant `clang++` vers `clang`. Native02 réemploie le même objet, ajoute `--driver-mode=g++` et lie avec succès, sans recompiler. Les six auxiliaires de lien et les headers SDK sans ancienne identité FD9 restent explicitement hors qualification de provenance historique globale.

Le premier contrôleur proposé n'a jamais été exécuté : Root a refusé en source deux hypothèses erronées (absence Docker affichée `[]`, propriétaire d'archive unique). La proposition 02 corrige ces deux points avec le helper commun inchangé. La première copie Mac du contrôleur a ensuite refusé avant tout Docker sur un ctime qui s'était stabilisé après sa première lecture ; ce refus demeure [conservé](failures/arm-source-prestart.json). Une copie neuve distincte est stabilisée avant sa première admission ; aucune ancienne autorité n'est actualisée. Tous les anciens échecs restent des échecs.

La restauration de véritables anciens journaux opérateur, les floors indépendants récents, les duties et la finalité commune avançante Mac natif/Docker Mac/VPS restent ouverts. Les anciens journaux privés Mac manquent ; la récupération NAS a été refusée. La campagne privée03 est FAILED et son accord consommé ; aucune04 n'est autorisée. Le runtime réseau `1925eee…` est inchangé et n'est pas réévalué ici.

Paris/Shanghai reste le profil réseau activé ; aucun nouveau fork/EIP n'est promu. Consensus/WAN/partitions/cycle complet/rotations/sync/sanctions, charge bornée, pruning, CI/reproductibilité, exploitation et revue de sécurité indépendante restent ouverts selon leurs critères normatifs. L'économie Base–Nano reste un brouillon non adopté : offre/ratio, réserve/conversion, bridge USDC, termes Doppler et visibilité T0 ne sont pas décidés/qualifiés. Aucun fonds réel, vente, dépense ou déploiement public. L'objectif général demeure actif, production non qualifiée.
