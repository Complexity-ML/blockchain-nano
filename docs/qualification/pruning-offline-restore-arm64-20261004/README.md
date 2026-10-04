# Restauration hors ligne d’une base élaguée ARM64

Scénario ajouté et passé à 3358d27b359edaf311c4fd8799f01421eae0d105,
branche feature/finalized-retention-recovery, runtime identique à 4734f285.
Seul le nouveau programme de test a été compilé/linké contre libnano_core.a
qualifié ; commandes CMake/flags exacts conservés. Les quatre programmes
précédents restent inchangés. Compilation/link/test exit 0. Nouveau programme
re-haché indépendamment, sources propres et manifeste courant conservé.

Six blocs canoniques, budget de suppression deux états : frontières 3, 5, 6.
Budget zéro ne supprime rien ; dépassement d’octets refusé avant publication,
ancien checkpoint et états conservés. Copie physique hors ligne de RocksDB,
hashes source/sauvegarde/copie vérifiés, réouverture read-only sans mutation,
reprise du pruning et commit après frontière complète. Remplacement sous
l’ancre refusé ; genesis/head, blocs, archives et reçus restent disponibles.
Sauvegarde source non modifiée par la restauration/reprise sur la copie.

Échec préflight conservé : l’image épinglée ne contient pas python3 (exit 127),
aucune compilation ni test lancé à ce stade. Contrôle adapté en bash, sans
ajout de dépendance ni reconstruction d’image. compile-run.py conserve la
première tentative ; compile-run.sh est la commande réellement exécutée.

Qualification de stockage local ; snapshot de test synthétique, aucune
preuve de restauration entière de consensus/signers/témoins indépendants,
de downgrade, de réseau WAN ou de panne électrique physique. La restauration
WAN efbeb9bc qualifiée auparavant ne qualifie pas une base élaguée 4734f285.
AMD64 et restauration de consensus avec pruning restent ouverts. Runtime
non promu sur main, production non qualifiée.
