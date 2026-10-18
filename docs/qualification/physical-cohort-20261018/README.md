# Cohorte physique publique — qualification bornée du 18 octobre 2026

Ce lot qualifie une copie de stockage Linux ARM64 vers Linux AMD64 avec le lecteur C++ de destination. Il ne qualifie pas la restauration d’un validateur opérationnel ni la production générale. Les résultats et leurs empreintes ont été vérifiés par Root avant cette publication ; l’[inventaire des preuves](proof-inventory.json) identifie les fichiers publics associés.

Le scénario utilise deux cohortes synthétiques, alpha et beta : huit bases (chaîne, témoin, journal et témoin du journal par cohorte), cinq archives de blocs signés, état d’exécution et opération BLS en attente. `produce` crée quatorze signatures de fixture publique ; `check` n’en crée aucune et effectue deux passes en lecture seule. Les engagements du manifeste sont conservés hors des arbres contrôlés.

| Périmètre | Résultat vérifié | Limite |
| --- | --- | --- |
| Producteur ARM64 puis lecteur distinct | PASS sur volume Linux persistant, producteur supprimé avant création du lecteur RO ; manifeste engagé séparément. | Fixture synthétique, sans duty opérateur. |
| Producteur AMD64 puis lecteur local | PASS : une unité C++ compilée et liée avec le Core conservé ; huit bases produites et vérifiées. | Production indépendante, distincte de la copie ARM ; recette manuelle, pas génération CMake. |
| Comparaison des deux productions | Même projection sémantique après exclusion des deux champs `physical`, un par cohorte. | Les fichiers RocksDB des deux producteurs ne sont pas déclarés identiques octet par octet. |
| Copie physique ARM64 → AMD64 | PASS : export brut, transfert SSH, import dans de nouveaux fichiers, puis authentification par le véritable ELF AMD. Trois records, deux passes RO, 69 entrées inchangées après nettoyage et sortie du conteneur vérifiée séparément à zéro. | Une direction et les versions épinglées ; zéro compilation, signature, réparation ou `produce` pendant cette lecture. |

L’archive réellement transférée contient 5 160 960 octets, SHA-256 `4e834ce270dcd5ce931d6bc5ec88974d00e4075807a36ebdee1ad17312fbaab1`. Le manifeste original reste `89cf80fa311d4fd91b54094f7f1e8a08bfbc71ac4d831d98b24669703c6ffdbc`, 12 445 octets. L’export et l’import vérifient huit inventaires portables. Le lecteur C++ contrôle ensuite les engagements, archives, états, registres et journaux avec le Core de destination. Un hash trouvé uniquement dans la copie ne remplace pas l’engagement externe.

Les [records C++](arm-amd-reader-records.json) portent `networkTransport:false` parce que le lecteur travaille hors ligne. Le transport SSH de l’archive est une étape externe effective. Le [résultat du lecteur](arm-amd-reader-result.json) conserve les deux périmètres séparément : commande et conteneur terminés à zéro, RSS maximale 24 716 KiB, huit entrées tenues et les 69 entrées restaurées inchangées après nettoyage. L’export n’a pas vérifié rétrospectivement les identités physiques complètes des bases source ; il a vérifié noms, types, octets et empreintes avec le volume monté en lecture seule.

## Sources et binaires identifiés

Le Core C7 provient de la référence `9692ccd15b168c513596bc59f951d6e5bf1326f2`. Les Core et dépendances existants sont réutilisés, sans nouvelle image. Le helper ajoute des diagnostics bornés ; ses conditions de refus UID/type/mode et ses contrôles d’identité restent inchangés.

| Élément | SHA-256 | Octets |
| --- | --- | ---: |
| [Unité C++](../../../tests/qualification/physical_cohort_portability.cpp) | `1cc68bfa049fb8f924c9aacfde88c4413d28dba17cfec7557596209e6863c952` | 39 083 |
| [Helper diagnostique](../../../tools/storage-maintenance/bootstrap-readonly-replica.hpp) | `d3e2ddd78187e3c7994a709558e569a841838c487b9d266501306bca14dbc125` | 20 319 |
| Core ARM64 conservé | `65c91631ffe93515e500fd1eeacb4e6fd784fa04d431a193c40ec14733077938` | 41 306 456 |
| Core AMD64 conservé | `dc4af8d2c04e684844205d2d6b4c5d99d9ebb6074a6197148e16e489006cad8e` | 41 076 890 |
| ELF ARM64 | `d63e054a54ee40d8607dabd02904dcb2b628991498588175907619ac7bf9f960` | 6 050 664 |
| ELF AMD64 | `824bd6a9ac7ce93fbfecbeeca23fcc71322d8e4f4979d16b96e5c815ab4a753a` | 6 408 336 |

Le [raccord CMake](../../../CMakeLists.txt) ajoute trois commandes : cible `nano-physical-cohort-portability`, lien `nano_core`, macro `NANO_TEST_SOURCE`. Il est revu en source ; configuration, génération et construction CMake complètes restent à qualifier. Aucun CTest automatique ni nouveau mode C++ n’est ajouté. Les CLI restent `produce ABSENT_ROOT` et `check EXISTING_ROOT MANIFEST_SHA256`.

Ces bibliothèques et ELF de qualification ne remplacent pas le producteur réseau déployé, identifié séparément par `1925eeeabc9f7d0aef41f2c3fe43c11c31f1f352`. Une mise à jour Git des sources ne prouve pas une mise à jour du binaire.

## Échecs conservés et critères ouverts

ARM01–05, les refus de préparation/source, refus forensiques et de sonde, ainsi que NativeVolume01/02 restent des échecs historiques. Une commande terminée à zéro ne transforme pas un scénario C++ refusé en PASS. La réussite sur tmpfs ne prouvait pas la persistance. La préparation AMD01 a refusé le modèle avant compilation parce que le garde V6 attendait à tort un worker nul, alors que le modèle public avait déjà son descripteur de worker. V7 corrige ce garde et utilise de nouveaux dossiers 02, sans effacer le refus 01 ni modifier ses autorités. Aucun défaut Core n’est déduit de ces seuls refus.

Restent ouverts pour cette candidate :

- Scénario sur Mac natif et recette réellement générée par CMake.
- Copie réciproque AMD64 → ARM64 et copies adverses : rôles mélangés, blob/journal/témoin/base manquant ou altéré ; preuve de rejet d’un véritable ancien journal, sans modifier les produits originaux.
- Restauration opérationnelle entière, engagements externes de tous les propriétaires et journaux, absence de rollback, reprise effective des duties et finalité commune avançante entre Mac natif, Docker Mac et VPS.
- Pannes d’E/S, autres critères de consensus, profil EVM/RPC activé, pruning, CI/reproductibilité, supervision et revue de sécurité indépendante.
- Relation Base–Nano, bridge et conditions réelles du lancement Doppler. Aucune ouverture de vente, dépense ou campagne réseau 04 n’est autorisée par ce lot.

Les anciens PASS restent acquis à leurs révisions et périmètres historiques. Les quatre documents normatifs reçoivent uniquement un ajout ; tous leurs critères, préfixes et échecs sont conservés. `productionQualified=false` ; objectif général actif et non atteint.
