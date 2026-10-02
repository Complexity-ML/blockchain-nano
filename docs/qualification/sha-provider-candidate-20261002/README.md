# SHA-256 SSZ : fetch OpenSSL réutilisé, candidat non promu

Révision cac8d719, parent 2db1840c, branche de qualification publiée puis récupérée sur VPS par Git. `consensus_signing_root` conserve le SHA-256 des 64 octets object||domain ; sélection OpenSSL effectuée au premier appel une fois, méthode immutable référencée et libérée à la destruction statique. EVP_Digest conserve un contexte privé par appel, vérifie succès et longueur 32 ; absence de fournisseur ou échec entraîne une exception. Aucun hash maison, domaine, BLS ou règle de consensus remplacé. OpenSSL >=3.0 déclaré dans CMake pour l’API EVP_MD_fetch. Configuration OpenSSL au premier appel ; pas de mutation de providers/default-properties dans le runtime du dépôt.

Deux nouveaux contrôles exécutent la vraie source du helper : 4 098 comparaisons à l’entrée legacy SHA256, chemin de référence distinct mais même bibliothèque OpenSSL (pas une contre-implémentation cryptographique indépendante), 16 threads synchronisés au premier appel ; puis fournisseur indisponible refusé et initialisation réessayée après restauration de la configuration dans un autre processus de test. ARM64 direct : les deux passent, sources/programme inchangés, container 0. Portée helper réel, pas un nouveau nœud ARM64 complet ni une régression. Code et manifestes conservés.

Qualification AMD64 réelle en cours dans `nano-sha-provider-candidate-amd64-test`, rapports `/home/ubuntu/nano-sha-provider-candidate-control-20261002-reports`. Compilation module changé / nouveaux tests puis relink cœur/nano/contrôles existants, headers identiques, pas de nouvelle image. Six cas : racines officielles, concurrence, fournisseur indisponible, chaîne signée, partition, stale-writer du cache. Délais CMake originaux ; bénéfice performance et résultats non acquis. Aucun runtime main promu, timeout et critères de production conservés.

Référence API primaire : https://docs.openssl.org/3.0/man3/EVP_DigestInit/ (fetch référence comptée à libérer ; EVP_Digest succès 1).

## AMD64 terminal — six contrôles passent

6/6, zéro skip, CTest/container/post-manifeste 0, sources/programmes/bibliothèques identiques avant/après. Racines officielles passent, tests de premier appel concurrent et d’indisponibilité passent, chaîne signée passe 5,462 s, stale-writer passe 0,536 s. Partition passe 221,908 s sous le timeout original 240 s : slot 112 atteint, finalité commune 12, six avancements consécutifs et assertions finales exécutées. Deux cœurs en un processus, clock synthétique, 64 validateurs, sans P2P : aucune clôture du réseau réel/WAN. Première réussite du candidat sur ce gate ; pas un pourcentage de gain calculable sur les précédents timeouts censurés.

Les sorties finales JSON proviennent de LastTest.log complet ; system-out du JUnit est tronqué et sa lecture JSON initiale a échoué. Les résultats JUnit et audits de manifestes étaient indépendamment vérifiés ; aucun test relancé pour ce problème de lecture. Rapport et résumé dans `terminal-amd64/`.

ARM64 : source propre cac8d719 figée dans `/tmp/nano-sha-provider-arm-source-20261002`, mêmes six cas. Le nouveau worktree a renouvelé les timestamps des sources : le build log montre une recompilation complète du cœur, pas seulement le module SHA malgré les headers inchangés. Même image/volume, pas de nouvelle image. Compilation terminée, racines officielles passent, partition en cours ; aucun résultat final nouveau revendiqué. Contrôleur conservé.

Cinq gates Capella précédemment en timeout démarrent sur le même cœur VPS, source inchangée : dépôt/activation, restart, auto-registration, activation après réindex, sortie après réindex. Target Capella relinkée, délais originaux 360/360/360/360/480 s. Rapports `/home/ubuntu/nano-capella-sha-provider-control-20261002-reports`, container `nano-capella-sha-provider-amd64-test`. Aucun résultat encore acquis, pas de replay des six succès sans changement ; ces cinq anciennes défaillances et l’ensemble des exigences WAN/production restent ouverts.

## ARM64 terminal — même source, six contrôles passent

6/6, zéro skip, CTest/container/post-manifeste 0. Racines officielles, premier appel concurrent, indisponibilité/reprise, chaîne signée et stale-writer passent. Partition passe 58,372 s sous sa limite originale 240 s. Manifeste source byte à byte identique à celui de la campagne AMD64 ; programmes et bibliothèques inchangés avant/après sur chaque architecture. Le cœur et nano ont bien été recompilés avec la source cac8d719 sur ARM64.

Ces deux campagnes utilisent les mêmes six noms et cas d’entrée, mais n’établissent ni l’endurance, ni le WAN, ni une régression globale. Les sorties finales ne publient pas le root de finalité : aucune égalité de root final entre architectures déduite des seules époques. Une vraie convergence multi-nœud sur la nouvelle version restera nécessaire. Résultats dans `terminal-arm64/` ; les cinq Capella VPS continuent séparément, sans promesse de réussite ou hausse des timeouts.
