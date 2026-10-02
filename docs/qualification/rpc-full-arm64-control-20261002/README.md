# Qualification cohérente ARM64 après reconstruction — contrôleur préparé

Même source candidate `d1597d9a6987d0839ed2f0c7ded268aead35d071` que le VPS. Attente du processus réel de dépendances Mac PID 83863, marqueurs de commande vérifiés pour refuser un PID réutilisé. Un processus absent/zombie n'est pas une réussite : l'image finale doit exister, être ARM64 et produire un digest immuable. Manifest des 15 315 sources comparé avec le début du build.

Une seule image et un volume neuf nécessaires après remise à zéro Docker Mac. Le script Git existant `scripts/ci/full-docker.sh` compile le P2P/helpers et le cœur dans `rpc-full-d1597d9a-arm64`, capture la sélection et les programmes puis exécute toute la régression sans traces RPC opt-in. Timeouts CTest inchangés, limites Docker 2 CPU/3 Go, garde globale 4 h, arrêt uniquement des conteneurs détenus en cas de dépassement. Aucun lancement public ou de fonds réels.

Contrôleur syntaxiquement vérifié, exécution/qualification complète non encore acquises. Logs en `/tmp/nano-rpc-full-arm64-control-20261002/controller.log`, puis `/tmp/nano-rpc-full-arm64-reports-20261002/prepare.log` et `tests.log`. Les erreurs historiques, supplément réseau, audit de restauration/duties, finalité WAN et autres critères production restent ouverts. Candidate non promue sur main.
