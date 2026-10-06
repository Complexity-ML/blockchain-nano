# Reprise v11 : continuations de cibles partagées

Sources post-v10 auditées séparément, engagements et journaux 16/16/33 conservés sans réinitialisation ni fusion. Trois arrêts originaux propres/sans OOM. Copies Mac locales seulement, sans peer.key ; volume VPS conservé sur son hôte. Core 504c9df8 qualifié sur Mac natif et Docker ARM64/AMD64, relais 76bb2468 inchangé.

Limites 1024 blocs, profil, finalité de départ 1284 et critère de trois avancées distinctes en 900 secondes inchangés. SSH explicitement IPv4 avec vérification habituelle de clé hôte. Sources/binaire et fichiers audités vérifiés avant toute reprise.

Le défaut d'effacement de continuations lors de blocs frères alternants est reproduit et corrigé localement. Il n'est pas établi comme cause de v10. Préparation seule, finalité et production ouvertes. Parents distincts, coût des validations/publications, pression du cache et pruning restent ouverts. Aucun transfert inter-hôtes de base ou de clé propriétaire, aucun fonds réel.
