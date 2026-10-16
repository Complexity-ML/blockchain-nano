# Sauvegarde arrêtée de deux racines

Un CLI Node standard, sans package, commande enfant, ouverture RocksDB ou accès réseau.
Voir les [preuves exécutées](qualification/stopped-backup-20261016/README.md) pour le périmètre exact.

Le produit conserve deux racines ENTIERES et indépendantes : `data` et `witness`,
y compris les fichiers présents de journaux et d’identité P2P. Ces noms désignent
des rôles de transport : le CLI n’invente ni témoin de consensus ni relation entre
deux bases. Deux DB d’exécution synthétiques peuvent qualifier la copie physique,
sans qualifier l’authentification de signataires ou d’un témoin.

## Successeur source v2

Cette version conserve la proposition v1 gelée (manifeste SHA256
`6b7bc0ee2de64f84739b4213843fafd43c5b5ee2d21fbf6ffbb11cbaf715354e`).
Le seul changement produit est une copie profonde de l’entrée de createBackup,
faite avant validation et avant le premier await. Une mutation ultérieure de l’objet
fourni par l’appelant ne peut donc changer ni les assertions déjà validées ni les
chemins, limites ou pins retenus par l’opération. Le format CLI/manifeste reste v1.

La régression modifie sourcesMountedReadOnly sur l’entrée originale
pendant le checkpoint new-backup-root : la copie retenue garde l’assertion true,
créer un backup vérifiable avec son pin et préserver les neuf champs des sources.
Le défaut v1 était dans cette API asynchrone, pas une preuve de corruption des données.
Root a exécuté les tests et la copie physique décrits dans la qualification ci-dessous. La proposition source v2 et ses statuts historiques sont conservés dans le paquet de preuves.

## Utilisation

```
node scripts/operations/stopped-backup.mjs create CONTROL.json CONTROL_SHA256
node scripts/operations/stopped-backup.mjs verify BACKUP MANIFEST_SHA256
node scripts/operations/stopped-backup.mjs restore BACKUP MANIFEST_SHA256 NEW_DEST
```

Le contrôle de création est lui-même fourni par chemin physique et SHA externe :

```json
{
  "format": "nano-stopped-backup-create-v1",
  "roots": {"data": "/stopped-data", "witness": "/stopped-witness"},
  "destination": "/destination/backup-01",
  "proofs": {
    "stop": {"path": "/controls/stop.json", "sha256": "SHA256_HEX64", "bytes": 123},
    "commitments": {"path": "/controls/commitments.json", "sha256": "SHA256_HEX64", "bytes": 456}
  },
  "limits": {
    "maximumEntries": 10000,
    "maximumFileBytes": 134217728,
    "maximumTotalBytes": 134217728,
    "maximumElapsedMs": 120000
  },
  "preconditions": {"allWritersStopped": true, "sourcesMountedReadOnly": true}
}
```

Remplacer les placeholders par les octets/pins réels indépendamment retenus.
Ces deux preuves sont opaques et copiées telles quelles ; le CLI n’interprète pas
leurs champs ou tokens et ne les dérive jamais des données sauvegardées. Genesis,
profil, configuration et identités de signataires restent conservés et authentifiés
indépendamment pour les audits C++ ultérieurs. Aucune clé n’est cherchée dans
l’environnement, aucun chemin implicite ou glob de données n’est utilisé.

**Avant create**, arrêter TOUS les writers puis monter les deux racines réellement
en lecture seule dans le namespace de l’outil. Les deux booléens sont des assertions
de l’opérateur, pas une mesure de quiescence ou de mountflags. Un LOCK présent,
un chmod ou un inventaire stable ne prouve pas qu’un writer inactif est absent.
Root prévoit une intégration Node Linux dans l’image ARM existante, deux montages
RO et un nouveau montage RW, sans image rebuild. Les tests filesystem macOS sont
une preuve distincte de cette intégration et ne prouvent pas un mount RO.

## Format et intégrité

Le nouveau dossier contient exactement `data.nbak`, `witness.nbak`,
`proofs/{stop,commitments}.json` et `manifest.json` après réussite. Les archives
internes versionnées ne sont pas des tar : magic `NANO-STOPPED-ROOT-V1\n`, puis
pour chaque fichier, longueur uint64 big-endian et octets. Aucun nom, lien ou mode
issu d’une archive n’est exécuté ou extrait. Le manifeste épinglé fournit les noms
relatifs, types, tailles, SHA et neuf attributs des sources (atime exclu). Chaque
frame doit correspondre exactement, sans troncation ni octet supplémentaire.

Inventaires et lectures sont FD/path avec O_NOFOLLOW, nlink1 pour tous les fichiers,
hash borné et neuf champs inchangés. Les répertoires sont inventoriés eux aussi.
Les symlinks, entrées spéciales, noms invalides/UTF8 ambigu, hardlinks et inodes
partagés entre les racines refusent. L’énumération par opendir est bornée AVANT
collecte et tri. Les limites portent sur les deux racines, même les fichiers vides.
Les archives et preuves nouvelles sont synchronisées puis relues par SHA ; les
sources et preuves externes sont relues avant publication et dans le finally.
Aucune ancienne identité n’est rafraîchie pour contourner un refus.

## Publication et crash

Le dossier de destination est réservé par mkdir exclusif, privé0700. Ses fichiers
sont0600 et son parent doit appartenir à l’UID courant sans écriture groupe/autres.
Les deux racines, preuves, contrôle et destination sont disjoints. Un dossier déjà
présent, même vide ou incomplet, est refusé : il n’existe ni `--force` ni reprise
qui écrase une tentative précédente. Aucun dossier ancien n’est nettoyé.

Les archives, preuves, manifeste temporaire et répertoires sont fsync avant le
commit. Le commit est **le fichier manifest.json**, publié avec fs.link(temp,final)
sans remplacement, puis unlink(temp), fsync du dossier et de son parent. Il n’y a
PAS de rename de répertoire prétendu no-replace. Le dossier peut être visible mais
INCOMPLET avant ce marqueur et doit rester non restaurable.

Une erreur après link mais avant le fsync final laisse un état INCERTAIN conservé,
sans résultat de succès ni pin producteur émis. Ne pas fabriquer une autorité en
hachant soi-même ce marqueur après l’échec. Un backup n’est accepté qu’avec le SHA
retenu extérieurement du résultat create réussi. Le manifeste et ces reçus doivent
être stockés indépendamment de la copie pour détecter substitution/rollback.

Les erreurs fsync/ENOTSUP/EIO/ENOSPC refusent, comme une erreur du fsync de dossier.
Les destinations tmpfs/ramfs Linux sont refusées explicitement. Un autre type de
filesystem n’est pas automatiquement qualifié : ces appels demandent la durabilité
au système, sans prouver un contrôleur matériel, une coupure électrique, un stockage
distant ou l’espace physique du Docker VM. Root doit qualifier le filesystem retenu.

## Restauration

Verify et restore exigent le SHA du manifeste connu AVANT lecture. Les quatre fichiers
et deux preuves attendus ont leurs noms fixes ; toute entrée supplémentaire refuse.
Tous les frames/hashes sont vérifiés AVANT création de NEW_DEST. La restauration
écrit exclusivement de nouveaux `data/`, `witness/`, `proofs/`, fichiers0600 et
répertoires0700. Elle ne remet jamais les anciennes9/modes sur les nouvelles copies.
Les fichiers restaurés sont relus par SHA, les feuilles/répertoires synchronisés et
le backup d’origine recontrôlé avant son propre commit de restauration.

Ce commit reste `signingAuthorized:false`, `cppSemanticValidationRequired:true` et
`independentLatestCommitmentsRequiredBeforeSigning:true`. Il ne valide pas à lui seul
chaîne, signataires, témoin, fraîcheur ou reprise. Les contrôles existants distincts
restent requis : tokens externes et identités préparées de restore-config, audit
canonique C++ et authentification des paires/journaux par le mécanisme approprié au
réseau, puis budget d’horloge et procédure explicitement autorisée. Ne jamais restaurer
d’anciens journaux pour obtenir un démarrage vert ; aucune config de reprise, signature,
commande de lancement ou retour arrière automatique n’est produite par cet outil.

## Bornes et vérification

Create, verify et restore ont une deadline monotone unique depuis leur entrée. Celle
de verify/restore est réduite au budget du manifeste sans remettre l’origine à zéro ;
pré-vérification, extraction, readback et fences partagent ce budget. Les vérifications
ont lieu ENTRE I/O : une syscall noyau bloquée ou un fsync lent ne sont pas annulés
par Node. Prévoir un watchdog externe ; aucun hardtimeout système n’est revendiqué.
En échec, aucun succès n’est imprimé, l’état partiel est gardé et la première exception
reste prioritaire même si la fence finale échoue aussi. Les détails/valeurs de DB ne
sont jamais imprimés. Le CLI ne touche à aucun PID, service, volume ou image.

Le test standard Node couvre des fichiers synthétiques publics : roundtrip
de deux racines avec fichier vide/répertoire vide, neuf champs sources inchangés,
préconditions/budgets, alias/hardlinks, mauvais pin, ancien dossier, corruption,
troncation, traversal même avec un manifeste repincé de test, mutation source,
crash de processus avant commit, filesystem volatile, fsync non supporté/erreur,
collision atomique du commit, état incertain après link, readback restauré, priorité
de la première exception, deadline globale par horloge injectée et énumération bornée.
Les hooks sync/clock/checkpoint sont réservés à cette injection en bibliothèque ;
aucune option CLI/env n’active une panne artificielle.

## Qualification observée le 16 octobre 2026

`node --test scripts/operations/stopped-backup.test.mjs` : 26/26 PASS sur Mac natif,
26/26 PASS dans Docker ARM sur volume Linux. Un essai sur partage de dossier macOS
via Docker reste 21/26 PASS, 5 FAIL de métadonnées/permissions : cette voie reste
non qualifiée. Les comparaisons strictes ne sont pas relâchées.

Le CLI a sauvegardé, vérifié et restauré deux véritables DB RocksDB synthétiques
fermées au genesis ; les deux audits Nano restaurés retrouvent les mêmes racines
et hashes que les originaux. Les sources étaient montées RO et sont restées
inchangées. La seconde DB appelée witness est une DB d'exécution indépendante,
pas un témoin de consensus authentifié. Aucun validateur n'est restauré ou relancé
par cette preuve ; aucune panne électrique ni durabilité hors hôte n'est qualifiée.
Les [rapports et échecs conservés](qualification/stopped-backup-20261016/README.md)
donnent le détail. La production et la reprise opérationnelle restent ouvertes.


## 16 octobre 2026 — sauvegarde des journaux et témoins de consensus arrêtés

[Preuves publiques et refus conservé](qualification/fresh-lifecycle-capture-correction-20261016/README.md). Root a sauvegardé/vérifié/restauré les deux racines entières data/witness du volume mac-docker arrêté, montées RO : 369 fichiers, 119 767 303 octets. Les captures C++ de la restauration valident 16 paires primary/witness et 34 DB, les jetons externes du second arrêt/témoins et les fermetures de blobs ; Core et états validateurs byte-identiques. Les inventaires historiques des originaux restent respectés et la capture source de 2 667 597 octets est byte-identique avant/après. Huit contrôles Linux sont nouvellement admis, stables sur 2 018 ms, puis neuf champs/SHA inchangés en fence finale. Le premier contrôle Mac refuse après create (`control-nine-changed`), sans restauration ni refresh de ses anciennes autorités. Backup privé sur le même hôte Docker Mac, sans publication des DB/capsules. Pas de reprise de duties/signature, replay historique, durabilité hors hôte ou panne électrique qualifiés ; V37, cycle complet et production restent ouverts.


## 16 octobre 2026 — VPS AMD64 : backup entier et copie lifecycle distincts

[Résultats publics et sources](qualification/lifecycle-whole-copy-vps-backup-20261016/README.md) : le CLI backup/verify/restore copie deux racines arrêtées RO entières, 743 fichiers/124 106 584 octets ; captures C++ 33 paires/68 rôles DB avec jetons externes exacts, Core/états byte-identiques, capture source finale historique égale. Les neuf champs des preuves privées sont figés dans cette opération, leurs SHA rapprochés de l'historique ; aucune ancienne table de métadonnées récupérée. Fence finale de 21 entrées dont huit contrôles nouveaux stables sur 2 003 ms ; le checksum Node HTTPS ne vaut pas signature de release.

Ce CLI n'est pas lifecycleRestore. Le générateur lifecycle corrigé (concaténation, fsync parents) garde ses causalités et son premier TAP 13 PASS/1 FAIL de projection. La vraie copie v1 échoue à create ; observations RO à 38 170 ms dépassent 30 s, erreur conservée. V2 statx 15/15 sur les deux ISA puis vraie copie VPS PASS : 743 fichiers/70 répertoires, création, capture C++ de 33 paires/68 rôles, vérification et fence source ; temps de stabilité 17 352 ms et étendue 12 198 ms, limites 175/30 s inchangées. Base c1b et candidat v2 alors hors main distincts, aucun C++/image reconstruit. Copies privées sur même VPS, pas de replay historique, duties/signatures repris, off-host/panne électrique ou production qualifiés. La prochaine tentative est autorisée mais non commencée, aucun nouveau seed/jeu de clés ; ancien domaine signé et journaux sans reset.
