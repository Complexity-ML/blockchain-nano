État courant : fixture et shim Linux de test intégrés, parentmain5f2977a6 ; ARM6486820 et AMD6464866 terminaux0, trois SIGKILL et quatre EIO chacun, hashes parent/test/shim inchangés. Enregistrement CTest Linux vérifié par configure/inventaire ARM64 et AMD64, sans recompiler/rejouer les mêmes tests. Historique de préparation conservé ci-dessous.

# SIGKILL aux frontières WAL du réacteur froid — 6 octobre 2026

État courant : prototype isolé, aucun code appliqué au main5f2977a6. ARM6486820 terminal0 ; AMD6464866 actif, compiler PID3859573 confirmé vivant. Quatre fichiers publics vérifiés avant compilation sur VPS.

Le cœur qualifié reference-memo-runtime est inchangé ; seuls la fixture et le shim Linux réservé aux tests sont reconstruits. L’API arm_sync_stop cible un chemin WAL canonique exact. Après append, avant fsync/fdatasync réel, elle émet un marqueur puis SIGSTOP ; le parent posix_spawn possède et tue uniquement ce child avec SIGKILL et exige le code -9. Une troisième coupure vise l’après-commit durable, après accusé de réception. Toutes les bases sont des copies synthétiques fermées avant copie, aucun fork avec RocksDB ouvert, aucun volume/journal réseau utilisé.

ARM64 : trois frontières passent (before-primary-sync, before-witness-sync, after-durable-commit), récupération exacte parmi les deux checkpoints canoniques de l’oracle, archives signées, retry idempotent et réouverture avec token attendue. Les quatre EIO précédents sont rejoués car leur shim change, et passent aussi. Hashes core/fixture parent, nouveau test et shim inchangés avant/après. Le témoin écrit mais non synchronisé peut survivre à un SIGKILL par le cache noyau ; ce run observe new dans les trois cas, sans revendiquer une panne électrique.

Les tokens de récupération sont issus des checkpoints synthétiques de l’oracle, avec disposition déterminée par l’intent réellement rouvert. Ce n’est pas une preuve de conservation indépendante des engagements d’un validateur réel. Restent ouverts : AMD64 terminal, migration froide interrompue, coupure physique, RSS/GC, graphe1024, restauration réelle/journaux et finalité WAN. Aucun EIP, bridge, déploiement ou production promu.

Le shim de faute reste lié exclusivement aux binaires de test ; nano/nano-p2p et le core ne sont pas modifiés. Gate consensus_capella_cold_memo_sigkill_recovery, timeout240s. Trois frontières qualifiées sur copies synthétiques uniquement ; aucune panne électrique, migration froide interrompue, restauration réelle des journaux/engagements ou finalité WAN déduite. Aucun EIP, bridge, déploiement ni production promu.
