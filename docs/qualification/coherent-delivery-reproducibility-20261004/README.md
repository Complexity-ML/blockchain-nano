# Reproductibilité des exécutables efbeb9bc — 4 octobre 2026

Contrôleur terminal exit 0, ARM64 et AMD64 complets. `nano` et `nano-p2p`
recompilés dans des répertoires neufs : SHA-256 identiques aux exécutables
qualifiés sur chaque architecture. Images de dépendances épinglées réutilisées,
aucune reconstruction d'image et aucun chevauchement avec la restauration.
Sources avant/après et exécutables originaux avant/après inchangés.
Le résultat, le contrôleur exact, les journaux et manifestes sont archivés ;
gzip conserve les octets originaux et leurs SHA-256 dans le manifeste.

Ce résultat qualifie les deux exécutables de livraison avec les mêmes images
et caches de dépendances. Il ne qualifie pas la reproductibilité hermétique
des dépendances depuis leurs sources, ni une image de release finale, ni
la totalité des 208 exécutables reconstruits (seuls les deux livrables le sont).
Le pruning 4734f285 commence après ce succès et reste un changement distinct.
Main n'est pas promu à la candidate ; production toujours non qualifiée.
