# Reprise v9 : résultat original et nettoyage complémentaire — 6 octobre 2026

Contrôleur original session 37036, checkout 48715f39 ; core 05649987, relais 76bb2468. Résultat FAILED, sortie 1, aucune avancée distincte de finalité commune depuis le checkpoint 1284. Pas de restart du contrôleur ni de prolongation de l’échéance 900 s. La campagne a terminé prématurément sur un timeout HTTP ; son nettoyage distant a ensuite échoué : IPv6 SSH « No route to host ».

Le VPS était encore actif après cette erreur. Son conteneur ORIGINAL a été arrêté séparément par IPv4 SSH, sortie 0 et sans OOM. Ce nettoyage complémentaire ne transforme pas le résultat en succès. result.json conserve exactement deux reçus originaux ; le troisième reçu, vps.log.state.json et supplemental-vps.log proviennent explicitement de cet arrêt complémentaire. supplemental-cleanup-provenance.json distingue ces sources. Aucun nouveau conteneur lancé, aucun journal réinitialisé ou fusionné.

Les trois boucles de validation étaient saines à leur propre arrêt ; tous les 65 engagements de propriétaire ont avancé (16/16/33). Cela ne prouve ni activation du propriétaire encore pending ni participation à une finalité commune. Aucun changement du critère de trois avancées communes distinctes.

Traces terminales : 134+134 envois de blocs Mac→VPS acceptés, aucun envoi VPS→Mac observé ; aucune transition transport-suspended, transport-resumed ou protocol-blocked. Ce run ne démontre donc pas la résolution de l’écart par le correctif de dispatcher. Catalogues expirés et admissions non confirmées conservés. La cause de l’asymétrie et du timeout reste ouverte.

Deux relevés de 2 s montrent de fortes écritures dans nano et un relais nano-p2p peu actif. Le relevé kernel VPS conserve memory.peak=2162774016 octets au moment de la lecture, limite 4 Gio, zéro événement OOM ; ce n’est pas le pic final de la campagne. CPU, attente, cgroup et erreurs Beacon 503 sont bruts. Ils orientent l’analyse de contention/persistance, sans preuve causale.

Audits post-arrêt séparés, en lecture seule : fichiers avant/après inchangés, engagements exacts du core et des signers, historique d’exécution canonique et ancêtre finalisé requis vérifiés. Les journaux Mac inactifs restent conservés séparément. Historique de votes/fork-choice non rejoué. La première tentative native a refusé les deux reçus originaux ; le guard a ensuite exigé la provenance du troisième arrêt complémentaire, sans réécrire le résultat original.

La base VPS reste sur le VPS ; aucune base/clé privée transférée entre hôtes. Sources/binaires et outils de capture qualifiés réutilisés. Les inspections archivées omettent l’environnement. Production, reprise réseau, pruning/saturation, WAN A et portabilité physique restent ouverts.
