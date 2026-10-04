# Nouveaux scénarios de restauration AMD64 terminale — 94535766

Contrôleur, deux compilations/link et deux tests exit 0. Seuls les nouveaux
programmes reconstruits contre le cœur pruning 4734f285 déjà qualifié ; image
existante réutilisée. Pas de rejeu du lot initial 7/7 ni de reconstruction d’image.
Source propre figée 94535766652e1d5eff53a7ce4f1ef3652e387262. Revue indépendante :
16 495 sources et les deux nouveaux programmes effectivement re-hachés.
Quatre anciens programmes et sources avant/après inchangés.

Stockage : copie physique hors ligne exacte, read-only sans mutation,
frontières 3/5/6, budget zéro sans suppression, budget d’octets refusé
atomiquement, commit repris après frontière complète et sauvegarde inchangée.
Capella : restauration d’un propriétaire élagué et 64 journaux, témoins retenus
indépendamment hors archive, audit/rejeu genesis, reprise avec engagements
exacts. Proposition conflictuelle refusée avant callback signataire ; 64/64
journaux avancés, finalité commune locale 2 → 7, source/sauvegarde inchangées.
Délais de scénarios inchangés 120 s stockage et 180 s Capella. Fixtures sur le
volume disque dès le départ ; l’échec tmpfs ARM64 reste dans son archive dédiée.

Deux propriétaires sur un même VPS, clés synthétiques et transport injecté.
Aucune nouvelle preuve multi-hôte, panne électrique physique, downgrade ou
conformité complète de production. Le banc WAN 092a153c est distinct et n'a
pas encore été exécuté avec pruning. Runtime non promu sur main.
