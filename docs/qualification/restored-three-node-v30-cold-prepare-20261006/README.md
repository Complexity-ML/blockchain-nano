# V30 : reprise depuis les engagements réels après V29

Core source9e696b50, mêmes binaires core que la qualification catalog-preflight native/ARM64/AMD64 ; relais e33ad89e inchangés. Hashes vérifiés, src/include/third_party/network comparés au commit déclaré. Aucun nouveau core/image recompilé pour cette préparation.

Sources : Mac V28 intactes (1024 records configurés), VPS V29 arrêté1 avant duties avec checkpoint migré et budget4096. Les budgets sources1k/1k/4k sont déclarés explicitement et doivent correspondre aux configurations auditées. Les autres limites/profils sont identiques. Le planner applique4096/cache8/16MiB et catalogue128KiB à tous ; il ne baisse aucune capacité et ne réécrit aucun pin source. Aucun plafond de validateurs n'est introduit : ces budgets sont des ressources du graphe local.

105 tests intégrés :87 planner/helper (neuf nouveaux cas de sources mixtes),18 garde de source arrêtée. Un exit1 n'est admis que sur le VPS avec reçu d'audit hashé dans cette préparation : pins inchangés, principal/témoin migrés d'une séquence, rejeu canonique identique hors token, source arrêtée sans OOM et sans duties. Exit137 garde sa propre exigence d'audit ; exit0 reste séparé. Les invalides/live/absence de preuve ne sont pas assimilés à une reprise saine.

Préparation59335/0 : pins possédés16/16/33 et historiques16/16/0 comparés séparément, checkpoint VPS actuel lié à la publication V29. Copies APFS Mac1727/1758 fichiers vérifiés, journaux/témoins complets sans peer.key. La dérivation VPS locale correspond exactement au hash de la configuration indépendante, sans transfert de DB/configuration/clé privée.

Sauvegarde82067/0 du VPS migré :2716 fichiers, tous hashes égaux avant/après et inodes indépendants, sauvegarde0700 sur son hôte. Source V28 et sauvegarde antérieure aussi conservées. Nouvelle liaison locale readonly de l'audit VPS préparée64994/0 ; la capture déjà réussie n'est pas rejouée : les fichiers sont revérifiés avant copie des références locales et hashes config/requête retenus sur VPS. Seuls scripts/reçus publics sont archivés ici.

Préparation seule. Aucun duty ou signature lancé à ce stade. La prochaine campagne doit reprendre ces snapshots et démontrer trois nouveaux checkpoints communs en900s sur Mac natif/Docker Mac/Docker VPS. V28/V29 FAILED, grand graphe/RSS/GC/partitions/rotations/slashing, EVM/RPC dans son vrai profil, Base–Nano/Doppler et revue indépendante restent ouverts.
