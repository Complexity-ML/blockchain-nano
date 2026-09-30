# Sauvegarde et restauration hors ligne Capella

Le script `tests/multihost/backup-restore.sh` reçoit le répertoire d'un nœud
explicitement arrêté, une destination neuve, le genesis, la requête d'audit et le
binaire. Il refuse les liens/fichiers spéciaux, une destination existante ou dans
la source. Il ne stoppe pas lui-même un nœud : ce préalable doit être vérifié.

Sur les volumes conservés de C, montés en lecture seule : archive tar.gz complète,
extraction dans un dossier neuf, comparaison de tous les noms/SHA-256 et audit
canonique de la restauration. Les 313 fichiers ARM64 et 314 fichiers VPS sont
identiques avant/après : chaîne, témoin principal, journaux de signature et leurs
témoins, identité P2P. Audit réussi (99 blocs ARM64, 100 VPS, 97 transactions).
Aucun réseau ni signataire lancé ; 2 CPU / 4 Go. Source et restauration demeurent
inchangées après audit. Refus des destinations existante/interne testés sur ARM64.
La seconde exécution ARM64 valide le déplacement du contrôle de destination
avant sa création. Aucun build supplémentaire.

Les archives/restaurations restent sous `/tmp/nano-backup-restore-20260930` sur
chaque hôte. Elles ne sont pas publiées dans Git ; seules les empreintes et preuves
le sont. Le répertoire privé 0700 du VPS a empêché une récupération par scp ; les
rapports ont ensuite été lus avec sudo, sans modifier cette protection. Le premier
commit de preuves contenait uniquement ARM64 ; le présent ensemble complète le VPS.

## Limites

Copie locale d'une fixture arrêtée : pas de sauvegarde en ligne/hors site, ni de
preuve de persistance fsync de l'archive sous coupure électrique. La conservation
exacte des journaux ne prouve pas à elle seule la reprise du réseau. Redémarrage
des signataires restaurés, refus des sauvegardes anciennes/incomplètes et finalité
commune après restauration restent à qualifier. Les clés de validation externes,
configuration et engagements indépendants doivent accompagner le plan de reprise ;
une archive du répertoire de données ne les remplace pas. Les limites de l'audit
canonique restent inchangées.
