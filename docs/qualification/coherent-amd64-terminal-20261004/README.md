# Candidate efbeb9bc : campagne AMD64 terminale

427/427, aucun échec/skip ; CTest, contrôleur, audit et manifestes exit 0.
Temps CTest 12065,68 s. Image fd9b0bb9, volume existant, aucune image rebuildée.
Revue indépendante avant le lancement WAN : 16 494 fichiers source et 208
programmes réellement re-hachés, identiques aux manifestes avant/après tests.
Sources propres à efbeb9bca3d42e41edb9cb81afba98c0b953a90f.
Rapports et revue exacts, gzip pour gros fichiers, hashes dans archive-manifest.json.

EEST sélectionné : 5 387 variantes, dont 4 882 exécution et 505 validation,
317 fichiers ; pas de conformité complète des forks ni de nouvelle activation
réseau déduite. ARM64 était déjà terminal 427/427 sur cette même candidate.
Les registres de 100k/1M sont synthétiques, pas des réseaux de ces tailles.
Réactivation après réindexation 351,85 s sous 360 s : marge de performance
faible, à investiguer. Le scénario Nano/P2P des sorties passe en 428,55 s
sur le même hôte ; il ne qualifie pas la restauration WAN.

La restauration et la reprise des duties, la reproductibilité des livrables,
le nouveau pruning historique, l’anomalie WAN A et tous les autres critères
de production/lancement restent ouverts. Runtime candidate non promu sur main.
