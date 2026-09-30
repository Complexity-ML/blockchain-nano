# Ouverture du stockage en lecture seule

`ChainStore::OpenMode::ReadOnly` utilise RocksDB OpenForReadOnly, sans créer
le dossier ni le schéma. Le comportement par défaut reste ReadWrite.
Le test natif passe en 0,47 s : un enfant écrit un checkpoint synchronisé puis
sort via `_exit` sans fermeture RocksDB. Le lecteur retrouve ce checkpoint via
le WAL, refuse une identité différente et une tentative de commit. Les noms et
SHA-256 de tous les fichiers restent identiques après lecture, refus et réouverture.
Une réouverture ReadWrite réussit le commit suivant. Un chemin absent n'est pas créé.

La base doit être arrêtée ou provenir d'un snapshot cohérent : ce test ne qualifie
pas une inspection concurrente avec un nœud actif. Qualifié également sur Linux ARM64 (0,03 s) et VPS x86-64 (0,04 s) ;
voir `../capella-offline-audit-20260930/linux/manifest.json`.
Cette API est un préalable à l'audit Beacon ; elle ne réalise pas son rejeu.
Le premier appel de build a trouvé la nouvelle cible inconnue ; après régénération
CMake, compilation et test réussis. Aucun résultat de test n'a été remplacé.
