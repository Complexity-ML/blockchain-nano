# Import macOS Native04 → lecteur Linux AMD64 retenu

Proposition **SOURCE_ONLY**, non exécutée par l’auteur. Ce paquet reçoit la cohorte publique créée par Native04, vérifie son archive et ses engagements indépendants, crée une copie Linux puis tente **une seule commande C++ `check`**, en lecture seule. Le contrôle Root exemple demeure `ready:false` ; ses lignes FD9 ne sont pas inventées.

## Entrées fermées et provenance

Les cinq carriers publics ont été annoncés par Root après son export terminal, sans lecture de ces fichiers par l’auteur :

| Fichier | SHA-256 | Octets |
|---|---|---:|
| `cohort.tar` | `7c6f556ab3514bc2115db8fb095dc9b1f5feaf1f0beaad955b81e286aebebf96` | 5 283 840 |
| `cohort-manifest-public.json` | `b8dfb4a9b2144c037f9f85a2137d0c4047bd0f344a739290b22c9242369246e6` | 12 445 |
| `export-inventory-public.json` | `c931751ec594069eca6ce70452c8670bb3ca07bcd4e31fade85e402ddea9a5bf` | 18 838 |
| `export-external-commitment-public.json` | `3144122f7872df305c47ee538a3db7f6a3d5f7c21274b163c07ab7d80137a203` | 1 157 |
| `result-public.json` | `7d55ffa8313961489970a02a27b41a4ebefa7a6a1774c1ba4bc0d7ee98385496` | 6 406 |

Ils sont sélectionnés littéralement sous `/home/ubuntu/nano-physical-native-import-public-root-20261018-01`. Le sixième carrier `source-current-observation-public.json`, s’il a été transporté, n’est pas une entrée dynamique : son pin est lié entre engagement et résultat export, sans lecture ni montage. L’engagement lie également les trois pins publics historiques Native04 (terminaux produce/check et résultat whole FAILED). Tous les anciens résultats et autorités restent intacts.

Le lecteur est l’ELF AMD64 conservé `/home/ubuntu/physical-cohort-amd-root-20261018-02/outputs/physical-cohort-portability`, SHA-256 `824bd6a9ac7ce93fbfecbeeca23fcc71322d8e4f4979d16b96e5c815ab4a753a`, 6 408 336 octets. Sa vraie ligne FD9 originale provient de l’audit public `61763fd153906581389d0123c606bedad41efbf0f3fd573171f3eea2e5d877ca` / 3 392 octets. Les **dix** anciens fichiers logiciels (six bibliothèques, Core, Docker, Git et Sudo) sont relus aux lignes originales du contrôle connu `689c5951523ed44f281023b163d0c0654006f49d188b09e458066084f3ca05ba` / 185 358 octets. Aucune ligne originale n’est recalculée ni remplacée. Ces autorités privées ne sont jamais exportées ni montées dans le conteneur.

## Contrat d’exécution réservé à Root

Après lecture et gel des sources, Root renseigne seulement les `originalRow` du modèle depuis ses autorités d’origine, conserve les pins, fixe `ready:true` dans un contrôle neuf et épingle extérieurement ce contrôle. Les descriptors ont exactement `file`, `pin`, `originalRow`, cette dernière avec les neuf champs FD9 (`dev`, `ino`, mode, nlink, UID, GID, taille, mtime_ns, ctime_ns) et SHA-256. Les vrais noms Python `st_*` figurent dans le code. UID:GID hôte imposés : **1000:1000**. Commande future, non exécutée ici :

```text
python3 /ABS/runner.py /ABS/CONTROL.json CONTROL_SHA CONTROL_BYTES RUNNER_SHA RUNNER_BYTES
```

Le programme refuse un output préexistant. Aucun `mkdir` n’a lieu avant validation des pins des cinq carriers, du USTAR, du manifeste et des huit inventaires portables, de leur engagement export, du résultat terminal, du lecteur et des dix logiciels originaux. Le helper `cohort_common.py` est une copie byte-exacte `09422e11287ce5f0357e3b3e9e5219d6c492ac2a3d3815aa6bbb6aa2ade7943c` / 10 200 octets du parent public. Il est chargé depuis ses bytes vérifiés et sa ligne FD9, avec un nom non-main ; aucune résolution `.pyc` ou import de projet/SDK.

L’import est manuel via descripteurs parents, `O_NOFOLLOW` et créations exclusives. Pas d’`extractall`, lien, PAX, device, chemin absolu, traversée ou doublon. Seuls les **69 membres** connus sous `public-cohort` sont copiés. Répertoires 0700, fichiers 0600, propriétaire 1000:1000 ; fichiers et parents synchronisés. Les nouveaux inodes sont uniques et distincts des carriers. Les SHA et tailles de chaque fichier, le profil et les **huit digests portables** doivent égaler le manifeste inchangé. Les inodes, UID et horodatages Linux ne sont pas assimilés à ceux du writer macOS.

Les huit champs de création et SHA des seuls nouveaux fichiers sont fixés au writer FD ; les répertoires sont fixés à la première capture complète fermée. Seul ctime peut varier **avant leur première admission**. Chaque observation est persistée avant décision ; trois captures full9 identiques couvrant au moins deux secondes sont requises dans trente secondes, puis la dernière capture exacte est adoptée une fois. Après cette admission, toutes les fences full9/SHA sont strictes. Aucune ancienne Seal, copie, autorité ni journal n’est réadmis. Le nouvel engagement d’import est un reçu de cette copie, hors arbre et hors montages ; il ne rétablit aucune protection historique anti-rollback.

## Lecture et clôture

L’image exacte retenue est `sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14`. Un seul conteneur à nom/label unique reçoit exactement deux bind mounts RO : nouvel arbre vers `/restore`, lecteur conservé vers `/nano-checker`. **Aucun ancien volume** n’est monté. Rootfs RO, réseau `none`, utilisateur 1000:1000, `cap-drop ALL`, `no-new-privileges`, aucun port, limites 2 CPU / 512 MiB / 64 PID, seul `/tmp` en tmpfs de 64 MiB. Aucun nouveau build, image, C++ compilé, `produce`, clé, signature ou duty.

L’unique CLI `check /restore MANIFEST_SHA` effectue déjà deux passes RO sur huit DB et cinq archives. Le contrôleur exige Docker CLI **0 et State.ExitCode 0 indépendamment**, groupe de processus fermé, stderr vide et exactement trois records JSON typés fermés. Alpha/beta doivent exposer les tokens primaire/témoin et headRoot **égaux aux engagements du manifeste indépendant**, deux/trois archives, les flags d’authentification et les deux passes. Le résumé exige huit DB, cinq archives, zéro nouvelle signature, RSS ≤ 128 MiB et les flags opérationnels/production false. Un simple schéma JSON valide ne suffit pas à la compatibilité sémantique.

Budgets : import ≤ 180 s dont admission ≤ 30 s, unique check ≤ 180 s, travail ≤ 360 s, fermeture totale ≤ 420 s ; archive/payload ≤ 128 MiB, ≤ 4 096 entrées et logs ≤ 1 MiB par commande. Les groupes sont uniquement ceux créés, PID 28722 exclu ; signaux, TERM/KILL/join, événements et nettoyages restent bornés. En succès comme en échec, fermeture du conteneur, absence exacte CID/nom, image finale et fences des entrées/arbre sont tentées indépendamment en conservant la première erreur. Aucun PASS avant ces clôtures. Les copies partielles et le nouvel arbre sont gardés ; aucun volume n’est pruné, aucune donnée supprimée.

## Portée et limites

Un succès qualifierait seulement la lecture authentifiée **de cette cohorte macOS Native04** par cet ELF Linux AMD64 et cette image, après vrai transport/import. Il ne prouverait ni compatibilité universelle des versions RocksDB, ni reprise de validateur opérateur, journaux privés, anti-rollback simultané des engagements, duties ou finalité commune. Root a signalé RocksDB macOS 11.8.1 et une ancienne fermeture Linux ARM 8.9.1 ; la version exacte du lecteur AMD ne s’en déduit pas. Toute incompatibilité réelle doit rester un échec, sans réparation, migration RW, régénération ou nouvelle signature.

Native04 produce/check CLI0 restent distincts de son contrôleur **whole FAILED**. Native05 whole FAILED, les anciens échecs physiques et les qualifications historiques ne sont pas reclassés. L’admission du nouvel export n’a pas qualifié le FD9 historique de la source macOS ; `historicalNativeSourceFD9Qualified`, `oldHostResultsRequalified`, restauration opérateur, duties, finalité commune et production restent false. Aucun fait d’exécution du futur import n’est revendiqué par ce paquet source.
