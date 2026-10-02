# Régression complète après optimisation RPC — contrôleur préparé

Candidate `d1597d9a6987d0839ed2f0c7ded268aead35d071`, image/volume AMD64 réutilisés. Le script Git existant `scripts/ci/full-docker.sh` effectue une compilation cohérente fraîche dans `rpc-full-d1597d9a-amd64` (P2P/helpers et cœur), capture les programmes et les sources, puis tous les cas CTest enregistrés sans instrumentation RPC. Dossier de preuves vide exigé, aucune ancienne campagne reprise/écrasée. La régression est justifiée par le changement de recherche de gaz et les nouveaux tests ; aucune image reconstruite sur le VPS.

Exécution rootless ubuntu, outils d'audit hôte, sudo limité au Docker CLI, limites conteneur 2 CPU/3 Go et timeouts CTest d'origine ; garde globale 4 h arrêtant uniquement les deux conteneurs détenus si atteinte. Le rapport conserve aussi les audits lorsque CTest échoue. La sélection exacte et le résultat terminal restent à vérifier, puis supplément réseau 21 cas et autres gates avant réutilisation WAN.

Contrôleur préparé et syntaxiquement vérifié ; aucune réussite complète revendiquée. La candidate n'est pas promue sur main. Mac Docker doit reconstruire une seule image de dépendances après le reset puis qualifier la même source ARM64 ; aucune qualification deux architectures ou production acquise.
