# Reprise v19 : échec de disponibilité IPC du VPS

Contrôleur original 81355, checkout fc704b9f ; cœur d8d5b46c et relais f11250ae. Sortie terminale 1. Préflight vérifié, configuration et environnement dérivés sur le VPS sans export depuis le Mac. Le VPS restaure son état mais signale altair-ipc-recovery-failed, puis le délai de disponibilité IPC de 180 secondes expire. Les deux propriétaires Mac ne sont pas lancés. Le critère de trois finalités communes en 900 secondes n'est donc pas exécuté et reste ouvert.

Arrêt VPS normal, sortie 0 sans OOM. Un reçu indépendant est conservé. Audit read-only : 33 clés, engagements et journaux/témoins comparés au reçu ; historique canonique et ancêtre finalisé vérifiés par le reader qualifié, 2170 fichiers inchangés pendant l'audit. Aucun transfert de base ou de clés entre hôtes. Les copies Mac post-v18 ne sont pas exécutées par cette campagne.

La revue de source montre que les erreurs transport/RPC deviennent nullptr et que l'événement IPC confond fin de récupération nulle et exception de configuration. L'étape/cause précise n'est pas établie. Le parcours d'ancre signée intervient après cette récupération : son correctif ciblé ne qualifie pas ce démarrage. Un brouillon de diagnostic redigé et testé séparément n'est pas encore intégré.

Aucun rebuild d'image, compilation concurrente, flood, fonds ou déploiement public. Tous les échecs antérieurs, critères de production, bridge/lancement et revue indépendante restent ouverts.
