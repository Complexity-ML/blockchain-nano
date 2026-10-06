# Reprise v10 : export retenu sans verrou d'admission

Sources post-v9 auditées séparément, propriétaires 16/16/33, engagements et journaux conservés sans réinitialisation ni fusion. VPS arrêté séparément par IPv4 après échec original du nettoyage IPv6 ; la provenance complémentaire est conservée. Copies Mac locales seulement, sans peer.key ; volume VPS conservé sur son hôte. Core bc6d1794 qualifié sur Mac natif et Docker ARM64/AMD64, relais 76bb2468 inchangé.

Limites (1024 blocs), profil, finalité de départ 1284 et critère de trois avancées distinctes en 900 secondes inchangés. Les contrôles de source/binaire empêchent d'utiliser un core antérieur. Le transport SSH est explicitement IPv4 avec vérification habituelle de clé hôte ; aucun fallback implicite.

La correction fait servir les blocs retenus depuis une vue immuable authentifiée. Elle ne démontre pas la cause WAN v9, la saturation ou la reprise réseau. Préparation seule ; production ouverte. Aucun transfert inter-hôtes de base ou de clé propriétaire, aucun fonds réel.
