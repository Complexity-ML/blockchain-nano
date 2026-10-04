# Correction des contrôleurs de reprise — 4 octobre 2026

Échec restore-v1 conservé : écriture de production-source-revision.txt refusée,
aucun nœud WAN lancé. Les dossiers qualifiés appartiennent à 501:20 (Mac)
et 1000:1000 (VPS), mode 755, racine du volume 0:0. L’image n’a pas d’USER
configuré ; root avec ALL capabilities retirées ne contourne pas ces droits.
Repro-v2/pruning-v2 ont correctement refusé de démarrer après cet échec.
Aucun build ni nouveau test pruning exécuté dans ces tentatives.

Restore-v2 écrit le seul marqueur avec l’UID:GID propriétaire obtenu via stat
et vérifie volume/image avant montage. Binaires et sources non modifiés.
Repro-v3 est identique hormis son prérequis corrigé et l’inspection du volume.
Pruning-v3 crée son seul dossier neuf en root avec CHOWN, puis build/tests
avec l’UID:GID hôte, comme la livraison ; droits du reste du volume inchangés.
Syntaxe des trois contrôleurs vérifiée ; processus de restauration désormais
lancé et vivant, autres contrôleurs en attente. Aucun résultat de restauration,
reproductibilité ou pruning acquis ici. Images existantes réutilisées.
