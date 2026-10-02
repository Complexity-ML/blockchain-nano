# Supplément réseau de la candidate RPC d1597d9a

Contrôleur terminal uniquement, sans attente automatique ni reconstruction. Réutilise l’image figée et le volume existant de chaque architecture, sous-dossier `rpc-full-d1597d9a-{arm64,amd64}`. Il exige avant Docker : audit strict des 427 résultats, sources/programmes stables, délais originaux, tous les statuts terminaux nuls, révision et checkout propres. Il vérifie ensuite les empreintes de tous les programmes directement dans le volume avant les 21 cas réseau autonomes.

Même isolation : utilisateur hôte non root, réseau Docker `none`, capacités retirées, 2 CPU / 3 Go, aucun port publié. Les délais explicites du CMake restent prioritaires ; fallback historique de 120 secondes conservé. Rapports frais, codes réels, JUnit, empreintes avant/après et revue réseau indépendante sont enregistrés. Aucun WAN ou statut production n’est acquis.

Préflight ARM64 réel : refusé pour manifest après tests absent, avant création de rapport ou interaction Docker. La campagne complète reste active. Le supplément n’a pas démarré.

Après succès terminal seulement : `python3 -B run.py arm64 /tmp/nano-rpc-network-supplement-arm64-reports-20261002` sur Mac ; `python3 -B run.py amd64 /home/ubuntu/nano-rpc-network-supplement-amd64-reports-20261002` sur VPS. Le mode `--preflight-only` ne lance aucun test.
