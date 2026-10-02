# SSZ : copie différée du cache — candidat 80373152

Révision **803731526c33111ea128bf6fddf5d7323fb5b220**, parent418d05cc, branche qualification/snapshot-ssz-view-20261002. Aucun runtime promu sur main.

Le chemin warm compare tous les octets SSZ à la chaîne hex canonique déjà validée, puis compare chaque autre champ de la ligne, ses justifications et le statut anchor. Le placeholder null demeure interne. Au moindre changement de champ, la chaîne SSZ complète est matérialisée avant validation et stockage. Le layout du cache ne change pas ; profils/pruning/cancellations et validation cryptographique sont conservés. Cela retire une copie lvalue du SSZ dans le cas inchangé ; aucun gain end-to-end n'est encore mesuré.

## Preuve ARM partielle terminale

**1153** comparaisons de sérialisation canonique, **2560** contrôles différentiels des autres champs (256 échantillons), **256** mutations SSZ refusées, **768** refus d'enveloppe et **384** annulations : helper exit0. Trois intégrations locales signées passent (natif, migration Bellatrix, migration avec trois slots sautés), neuf blocs chacune, roundtrip, sept familles de mutations warm/cold et **deux réouvertures SIGKILL chacune**, zéro message réseau. Sources complètes, helper/programme et sept archives/objets identiques avant/après ; exits helper/intégration/conteneur/source0.

Le module snapshot et l'intégration ont été recompilés dans un scratch neuf contre les archives2d readonly, ABI du cache inchangée. Ce lien partiel **ne vaut pas un build CMake cohérent, une qualification AMD ou une reprise WAN**. Image ARM immuable, réseau none, un CPU/2GiB, délais60/120s. Les fenêtres de benchmark du helper concernent seulement l'assemblage JSON antérieur ; elles ne mesurent pas ce delta SSZ.

## Qualification cohérente en cours, résultats non acquis

Deux builds CMake frais des neuf cibles ont été démarrés et observés actifs sur Docker Mac et VPS. Images existantes inchangées ; ARM1CPU/2GiB et AMD2CPU/3GiB. Répertoires coherent-80373152-arm64/amd64-20261002, aucun output qualifié écrasé. Sources gelées.

Deux contrôleurs des **16 scénarios originaux**, délais inchangés et aucun skip autorisé, attendent la fin vérifiée des builds. L'ARM attend aussi la régression complète2d déjà active. Le registre exact et les délais seront contrôlés avant toute exécution ; sources/neuf programmes/sept entrées sont hachés avant/après. Ce dossier contient les scripts, **pas encore leurs résultats terminaux**.

L'ancien contrôleur ARM0275 a été annulé avant démarrage de ses tests, après vérification du PID/commande et de l'absence de son conteneur. La campagne AMD0275 reste **15/16, timeout360,04s**, fullAMD refusé. Aucun échec effacé ou délai augmenté. Régression complète, audit natif/AMD, restauration des trois propriétaires et finalité commune, partitions, critères EIP/réseau, lancement Doppler et revue indépendante restent ouverts.

### Suivi vérifié du contrôleur

Build ARM neuf terminal0 ; les 16 tests ARM ont réellement démarré (trois premiers passent, activation en cours à la dernière observation). Build AMD encore actif. Deux refus préalables ARM conservés : image sans Python, puis CTest --show-only tentant une écriture de journal sur le volume readonly. Aucun CTest exécuté lors de ces tentatives ; contrôleur corrigé avec validation Python sur hôte et volume build frais writable pour le journal d'inventaire. Sources/binaire readonly ou hashés avant/après, registre comparé entre inventaire préalable et exécution. Délais/tests inchangés. Scripts corrigés archivés ; résultat16/16 non acquis.
