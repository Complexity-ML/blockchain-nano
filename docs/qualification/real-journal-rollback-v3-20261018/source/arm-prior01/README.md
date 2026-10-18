# ARM Linux retained-Core journal rollback — SOURCE ONLY DRAFT

Ce couple est une proposition non exécutée. Root fournit un **nouveau** contrôle externe et ses SHA256/octets, vérifie les autorités sur place et décide de son usage. Le modèle `ready:false` est inutilisable. Aucun contrôle effectif, SDK, journal, cache, preuve ou DB n’a été lu par l’auteur. Aucun programme proposé n’a été importé ou exécuté ; seule l’analyse AST a été faite.

## Programme et contrôle exacts

`python3 host.py CONTROL_FILE CONTROL_SHA256 CONTROL_BYTES`

Le contrôle fermé contient exactement **13 champs** : `format`, `ready`, `actualHead`, `oldControl`, `sourceAuthority`, `caseRoot`, `outputRoot`, `hostPin`, `worker`, `volume`, `fixtureInventory`, `timeoutSeconds`, `recipe`. Format `nano-real-journal-rollback-arm-control-v1` ; `ready` doit être le booléen vrai ; `actualHead` est un SHA Git complet explicite, vérifié avec le Git d’origine et un checkout propre. Les identifiants de publication ne servent pas d’autorité pour les octets Core/source.

- `oldControl={file,pin}` : pin Root connu `3a1697c4… /186873`. Son contenu fournit repo/Git, Docker, Core, six bibliothèques, les neuf lignes logicielles d’origine et l’ancien root `/work`. Le programme garde ces neuf **anciennes** lignes FD9+SHA, jamais des statistiques de remplacement. Le résultat FAILED historique du contrôleur physique reste FAILED.
- `sourceAuthority={file,pin}` : ancien échantillon `0b4b3c84… /623344`, `{directories,elapsedMs,rows}` avec 1050 lignes. Sélection exacte des **1047** lignes sous l’ancien `sourceRoot.root`, exclusion des trois anciennes feuilles `/case`. Aucune réadmission de ces fichiers.
- `caseRoot={root,pins}` : nouveau chemin physique, disjoint source/output ; exactement les deux feuilles TU CppV3 `2e5ef451… /28801` et helper `d3e2ddd7… /20319`. Nouvelle admission connue d’octets, huit identités fixes et trois observations FD9 stables durant au moins deux secondes, sous trente secondes. Toutes les observations sont persistées avant décisions/délais ; ensuite les identités admises sont strictes.
- `hostPin={sha256,bytes}`, `worker={file,pin}` : sources Root nouvellement admises, fermées avant tout `ModuleType/exec`. Python tourne **sur le Mac**, pas dans l’image. Le worker n’a ni CLI ni Python invité.
- `fixtureInventory={file,pin}` : export public d’origine connu `19d3c604… /18623`, 69 noms sous `public-cohort`, contenus/types/modes/UID/GID/mtime-secondes et huit engagements portables. Il ne fournit **pas** d’anciennes FD9 invitées.
- `volume` doit être exactement l’objet du modèle : volume local existant `nano-physical-public-64712f46d71f4587809d58a1bf7c26a3`, label d’origine. Aucun volume créé, supprimé, nettoyé ou réinitialisé.
- `recipe={compilerFlags,linkPrefix,linkTail}` : argv explicites Root par rôle, aucune commande shell composée ni remappage automatique. Clang est `/usr/bin/clang++`; un Core `/retained/libnano_core.a` et six providers d’origine requis. `timeoutSeconds` : entier 300–600.
- `outputRoot` : nouveau dossier `real-journal-rollback-arm-root-20261018-NN`, parent physique existant ; refus de tout chemin déjà présent. `hostOutput/outputs` reçoit seulement les quatre nouveaux produits M/objet/MD/ELF et les diagnostics.

## Compilation et logiciel effectivement consommé

Core Linux ARM conservé : `65c91631… /41306456`, **143 membres**, aucune compilation Core ni mise à jour d’archive. Image ARM Linux `e30ce85e…` existante, aucun build/pull d’image. UID/GID **501:20**, réseau none, rootfs RO, aucune capability ajoutée, no-new-privileges, deux CPU/2 Gio/128 PID/tmpfs64 Mio.

Une seule nouvelle unité : **1 M indépendant / 1 CXX-MD / 1 lien / 1 ldd / 1 RUN**, compteurs de tentatives incrémentés avant commandes ; les événements exposent les véritables exits et groupes de processus joints. M/MD ont le même MT `/outputs/journal.o`; égalité littérale et normalisée exigée. Les dépendances invitées sont tenues par GNU stat/hash/stat et alias canonique, distincts des FD9 Mac. Chaque entrée `/work` consommée par M puis MD doit mapper une **ancienne ligne hôte originale exacte** avant CXX puis aux fences finales. Toute dérive consommée refuse. Tous les absolus linkPrefix/linkTail et toute la fermeture résolue `ldd` sont tenus avant RUN.

L’ensemble ancien `/work` demeure fermé sur les **1047 noms, tailles et SHA** avant/après : chaque lecture doit être FD9 stable en elle-même. Ses dérives historiques de métadonnées hors closure ne sont pas réadmises ; `historicalAllSourceFD9Qualified:false` reste explicite. Aucun nombre fixe de dérives n’est un oracle et aucune politique des répertoires Native9541/1252 n’est transposée. Les nouveaux produits ont leurs observations propres et sont tenus aux fences hôte après nettoyage.

## Volume et ancien cohort

Seul un sibling inédit `/state/journal-rollback-<nonce>` reçoit le RUN. `/state` n’est pas une ancienne row ; sa modification nécessaire par création du sibling ne remet aucun floor ni Seal à zéro. **Les 69 anciennes entrées sous `/state/public-cohort` restent inchangées.** Aucun ancien C++/DB n’est rouvert ou rejoué.

Avant RUN, un scan passif GNU find/stat/hash ancré sur FD compare exactement69 noms/types et contenus à l’inventaire export original, recompute les huit digests portables et retient une observation GNU9 courante strictement cohérente FD/path. Cette observation est **nouvelle, en cours de qualification**, pas une ancienne autorité FD9 inventée. Elle doit rester exacte après RUN, puis via un nouvel auditeur volume RO après retrait du carrier principal. Les deux carriers sont exclusifs, sans autre utilisateur du volume, vérifiés par nom/CID/image/labels ; seuls ces carriers possédés sont stoppés/retirés. Le volume et les données restent conservés.

Docker Desktop : chaque Source bind inspectée doit être exactement le chemin Mac fourni ou `/host_mnt` + ce chemin, aucun autre alias. Aucune DB ne réside sur un bind Mac ; le seul stockage RUN est le volume Linux persistant. `/work`, `/case`, Core et les six bibliothèques sont RO.

## Oracle CppV3 et limites

Le record JSON unique reste fermé : **1 BLS public**, 7 authentifications Core, 4 positives, 3 refus exacts, 8 ouvertures RO sur deux passes et quatre stores. Nouvel unique champ `productionAdmissions`, deux objets fermés dans l’ordre `initial`, `post-sign`, bornes et booléens exacts issus du source CppV3. Il documente uniquement l’admission de nouveaux arbres de production, sans affaiblir les journaux/Seals déjà tenus. Les sommes des deux nombres `entries` et des deux `payloadBytes` doivent égaler respectivement `storeEntries` et `storeBytes`. Les limites de l’exécutable restent 180 s /128 Mio RSS /32 Mio de payload/4096 entrées.

Le résultat C++ provient du **vrai exit Docker exec** et d’un groupe hôte joint. L’arrêt du keepalive n’est jamais présenté comme un State.ExitCode0 du C++. Cleanup possédé, image finale, source globale et fences hôte sont tentés séparément après erreur ; première cause conservée et erreurs secondaires publiées. Les signaux sont restaurés avant construction du résultat final.

Historique : anciennes qualifications contrôleur/Native FAILED conservées ; aucune promotion à partir de pins identiques. Ce lot source n’est pas un PASS runtime. Aucun nouveau test réseau, duty, restauration opérateur, finalité commune, lancement, protection indépendante du rollback simultané des engagements, provenance historique globale du SDK ou production n’est qualifié. Root conserve la responsabilité des autorités originales et des opérations.

Bornes du parser historique conservées sur demande Root : `externalCommitmentBytes` 1..1048576, `storeBytes` 1..33554432, `elapsedMs` 0..180000. Elles diffèrent des bornes documentaires CppV3 (1..16384, 0..33554432 et 0..200000) ; aucun oracle ancien n’a été modifié dans ce nouveau transport. Les autres compteurs/schemas sont exacts.
