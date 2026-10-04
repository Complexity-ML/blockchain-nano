# Tunnel privé SSH corrigé — 4 octobre 2026

La tentative WAN `attempt-1791126559961829000` a passé les gates 427 + réseau21 + signé4 sur les deux architectures et les relectures de sources/programmes. Elle a démarré le seul nœud VPS puis échoué avant le démarrage des deux nœuds Mac : `bind [::ffff:127.0.0.1]:30307: Can't assign requested address`, code du pilote 1. Les logs et l'attestation sont conservés dans `failed-wan/`; les artefacts runtime restent inchangés. Aucun restore, quorum ou progrès WAN qualifié. Les conteneurs de cette tentative ne sont plus actifs.

Le wrapper antérieur imposait `ssh -6`, ce qui imposait aussi la famille des sockets d'écoute locales. Le nouveau wrapper utilise `AddressFamily=any` avec la destination IPv6 littérale autorisée, `StrictHostKeyChecking=yes` et le même `HostKeyAlias`. Les overrides de famille, clé d'hôte et proxy sont refusés. Le changement concerne le contrôle SSH local, pas le C++ ni le protocole.

15 tests du wrapper passent. Contrôle négatif sur l'ancien wrapper : six assertions échouent. Test live borné : l'ancien wrapper échoue à écouter sur `127.0.0.1:29307`, le nouveau obtient la bannière du service SSH via le forwarding vers `127.0.0.1:22` du VPS. Les deux processus de test ont été arrêtés. Aucune requête blockchain ni exposition publique ; cela ne prouve pas encore la réussite de la restauration WAN.

Le lanceur pointe vers `/tmp/nano-wan-dual-family-transport-control-20261004/bin` et réserve un nouveau dossier `/tmp/nano-rpc-wan-restore-d1597d9a-20261004-b`. La tentative échouée reste intacte. Les images, volumes, binaires, contrôles terminaux et délais sont identiques. Une nouvelle tentative n'est autorisée qu'après les mêmes gates indépendants.
