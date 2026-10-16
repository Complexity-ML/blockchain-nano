# Sauvegardes arrêtées : qualification physique bornée

Le nouveau CLI `scripts/operations/stopped-backup.mjs` sauvegarde deux racines
entières, contrôle les octets/empreintes et restaure exclusivement vers un dossier
neuf. Les preuves d'arrêt et engagements sont des documents externes épinglés,
opaques au transport ; ils n'accordent aucune autorité de signature.

Résultats effectivement exécutés par Root : 26/26 tests filesystem sur macOS et
26/26 dans l'image ARM Linux existante avec un volume Docker Linux. Les contrôles
incluent corruption, troncation, chemins, collisions, mutation, erreurs fsync,
crash de processus avant publication et budget global. La régression de mutation
de l'entrée de l'API échoue sur le prédécesseur (0 PASS/1 FAIL), puis passe dans la
suite candidate ; le CLI et son format ne changent pas entre ces deux sources.

L'essai Docker avec un dossier partagé depuis macOS reste conservé : 21/26 PASS,
5 FAIL sur métadonnées/permissions. Cette voie n'est pas qualifiée. Les comparaisons
de neuf champs et SHA n'ont pas été réduites ou rafraîchies pour la faire passer.
Le premier refus du runner avant tout Node/Docker, dû à un chemin relatif mal
interprété, est conservé séparément ; ce n'était pas un échec du CLI.

La preuve réelle copie deux bases RocksDB synthétiques fermées à hauteur genesis,
puis exécute create, verify, restore et deux audits C++ sur les copies restaurées.
Les deux audits retrouvent exactement les hashes et racines canoniques des
originaux. Les 20 fichiers/127153 octets des deux racines sources restent inchangés
en neuf champs et SHA ; leurs montages de sauvegarde sont RO et aucune autre vue
RW de ces sources n'est montée. Le volume de test neuf a été supprimé après les
lectures indépendantes. Aucun volume existant, clé ou service de campagne n'est
touché ; aucun build d'image ou compilation C++ n'est effectué.

La seconde base appelée witness est une DB d'exécution indépendante de test,
pas un témoin de consensus authentifié. Ceci qualifie la copie et le rejeu C++
au genesis, pas une restauration de validateur, une panne électrique réelle,
une reprise des duties, une couverture de backups hors hôte ou la production.
L'authentification des derniers engagements anti-double-signature et les audits
de consensus restent nécessaires avant toute reprise autorisée.
