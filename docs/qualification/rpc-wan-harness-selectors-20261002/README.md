# Sélection explicite des artefacts existants pour le WAN RPC

Pilote `c7f759bb` sur branche Git `qualification/rpc-wan-harness-20261002`, basé sur le runtime `d1597d9a`. Seuls trois fichiers JS changent : build-paths, ses tests et le pilote multihôte. Aucun changement de C++, CMake, réseau C++, tiers ou CI ; le runtime qualifié reste d159. Les sélections natives et P2P cohérentes préparées antérieurement sont reprises.

Nouveaux paramètres `NANO_TEST_BUILDER_IMAGE_ARM64/AMD64` et `NANO_TEST_BUILD_VOLUME_ARM64/AMD64` : les quatre doivent être explicites ensemble ; les images exigent des digests SHA-256, les volumes des noms valides. Le montage de build en lecture seule et les chemins/binaires sont traduits pour le VPS. Les montages de données de fixture ne sont pas réécrits. Les valeurs par défaut historiques restent inchangées pour les autres campagnes ; le futur lanceur d159 doit fournir les quatre valeurs actuelles.

20 tests de sélecteurs passent, dont cinq nouveaux contrôles sur images/volumes, mode, injection et traduction complète d’argv. Trois invocations réelles du pilote avec paramètres invalides sont refusées avant création de sortie et avant Docker/SSH/SCP (commandes remplacées par des pièges locaux pendant ces refus). Aucun nœud lancé. Le premier essai de tests a échoué sur le texte `require` vs `requires` attendu ; message corrigé, log initial conservé.

Syntaxe du pilote vérifiée. Régression Docker complète, suppléments, audit signé AMD64 et adaptation finale/revue du lanceur restent requis avant exécution WAN. Ce fichier ne remplace aucun gate et ne relance pas le lanceur précédemment refusé par l’approbation automatique. Code arrivé par Git, aucun transfert SCP de logiciel. Production non qualifiée.
