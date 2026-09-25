# Checkpoint atomique du consensus et de l'exécution

`ChainStore::commit_execution_checkpoint` est une primitive de stockage pour un
candidat **déjà validé** par le consensus. Elle remplace dans un seul WriteBatch
RocksDB synchronisé :

- le suffixe canonique après un ancêtre commun, ses états et ses reçus ;
- les index bloc/hash et transaction/reçu, la hauteur et les compteurs ;
- le snapshot opaque du consensus, son numéro de séquence et son digest ;
- une liaison persistante entre ce digest et le hash de la tête EVM.

Le suffixe peut être plus court que la branche remplacée. Un suffixe vide permet
un checkpoint de votes/horloge sans changement de tête, ou un retour à l'ancêtre
commun. La suppression des anciens index et l'insertion des nouveaux font partie
du même batch, y compris lorsqu'une transaction réapparaît dans la nouvelle branche.

Deux engagements protègent contre les auteurs obsolètes : le jeton exact du
checkpoint précédent et le hash de la tête d'exécution précédente. La base doit
être liée au chain ID et au genesis fourni, et contenir le genesis canonique.
La méthode vérifie les liaisons de parenté, racines d'état/transactions/reçus,
les métadonnées des reçus et le gaz/bloom. Les budgets fournis bornent le nombre
de blocs remplacés/insérés et la taille du batch sérialisé ; ils ne constituent
pas une borne stricte de mémoire maximale pendant la sérialisation d'un candidat.

Après la première écriture couplée, les APIs ordinaires `commit`, `rewind` et
`commit_fork_checkpoint` refusent de séparer les deux moitiés. La conversion d'une
chaîne portant les certificats du consensus historique est refusée. Une erreur
de Write/sync rend le handle inutilisable en lecture comme en écriture jusqu'à
réouverture. Il ne faut pas tenter de signer à partir d'un résultat incertain.
La lecture du checkpoint vérifie également sa liaison persistante à la tête EVM.

## Qualification

`storage_execution_checkpoint` couvre remplacement par une branche plus courte,
nettoyage des index, checkpoint sans changement de tête, huit refus, deux auteurs
concurrents (un gagnant), et quatre interruptions SIGKILL suivies de réouverture.
Le contenu du checkpoint est comparé au bloc, à l'état et aux reçus récupérés.
Les acquittements observés doivent être conservés après réouverture.

Sur macOS, `storage_execution_checkpoint_sync` injecte une erreur de synchronisation
sur le WAL, puis vérifie séparément les deux dispositions : suffixe conservé ou
suffixe non synchronisé supprimé. Le checkpoint et l'exécution doivent être tous
deux anciens ou tous deux nouveaux. Une écriture après réouverture est vérifiée.
Cette injection ne constitue pas un test de coupure électrique physique.

Preuves locales : `build-cmake/qualification/execution-checkpoint-20260925/`.

## Raccordement restant

Le snapshot est opaque pour RocksDB : cette API ne prouve pas qu'il représente
un Store Bellatrix valide, ne décide pas de la finalité et ne rejoue pas l'EVM.
L'appelant doit authentifier/valider le candidat, interdire les réorganisations
sous le checkpoint finalisé et conserver les enveloppes nécessaires à la reprise.
Les lectures successives par plusieurs getters ne sont pas un snapshot transactionnel
pour les lecteurs concurrents ; le reactor doit sérialiser leur publication.

La sérialisation/revalidation du Store Bellatrix complet est qualifiée dans
[BELLATRIX-STORE-RECOVERY.md](BELLATRIX-STORE-RECOVERY.md). Son reactor durable
et son raccordement réseau restent à intégrer. Cette étape ferme la séparation des
écritures au niveau de la primitive de stockage, pas la qualification de production.
Le journal anti-slashing de signature reste distinct et doit être synchronisé avant
émission. Détecter une restauration ancienne de l'ensemble du disque nécessite un
engagement indépendant du dernier jeton ; un digest stocké sur le même disque ne
peut pas détecter seul ce retour arrière.
