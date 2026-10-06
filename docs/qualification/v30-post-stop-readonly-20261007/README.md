# Audits indépendants après arrêt V30 — 7 October 2026

Core corrigé7329b7ad, outil de capture ressource e4ce39eb relinké sur ce core, sans modification du format. Audits originaux : local85927/0 (natif puis Docker Mac), VPS68012/0. Les trois captures ont fini0. Toutes les sources sont arrêtées0 sansOOM avant audit et aucun signer ne reprend pendant l’audit.

Checkpoint principal et témoin indépendant exacts aux reçus V30. Mac natif et Docker :32 journaux chacun, les16 pins actifs sont comparés aux reçus V30 et les16 historiques aux captures antérieures, séparément. VPS :33 pins actifs exacts, aucune clé historique supplémentaire. Les mêmes clés et engagements sont conservés ; aucune réparation par remise à zéro.

Tous les fichiers sont hashés avant/après :1796 natif,1807 Docker Mac,2784 VPS, strictement inchangés. La capture revalide l’historique canonique local, les archives et le checkpoint finalisé externe ; elle ne prouve pas de convergence entre les trois têtes. Le dernier checkpoint finalisé reste1284. Ces relectures ne reprennent pas les duties et ne ferment pas la finalité WAN.

Sources privées : /private/tmp/nano-restored-three-node-v30-cold-20261006 sur Mac et volume nano-restored-three-node-vps-20261005-data, configuration /tmp/nano-restored-three-node-vps-mux80iao sur VPS. Demandes, configurations, bases et listes complètes de fichiers privés restent sur leur hôte. Seuls capsules, reçus et scripts publics sont archivés ici.

Copies APFS complètes pour V31 préparées séparément sur Mac, hashes/inodes indépendants vérifiés, journaux/témoins préservés et peer.key exclu. Une préparation publique complète avec sauvegarde VPS et dérivation locale indépendante est encore nécessaire avant le prochain writer. Tous les critères de production, Base–Nano/Doppler et revue indépendante restent ouverts.
