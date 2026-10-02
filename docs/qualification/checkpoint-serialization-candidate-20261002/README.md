# Candidat de sérialisation — Git, non fusionné dans main

Commit 4834a014, branche `qualification/checkpoint-serialization-20261002`, parent 1ac57e25. Six fichiers changés : deux modules de sérialisation, nouveau helper interne/test, déclaration du test CMake et pair de qualification Altair corrigé. Tous les autres fichiers et headers existants sont identiques au parent qualifié. La branche est poussée et récupérée par Git sur le VPS dans un worktree dédié.

Le helper appelé directement par le cœur réutilise le JSON canonique du store, vérifie la limite sans overflow et refuse une clé extérieure qui déplacerait store dans l’ordre canonique. Les six tableaux locaux sont transférés ; le store authentifié reste const. Pas d’instrumentation de profil dans ce candidat.

Docker ARM64 : le test réel du helper passe, 2 048 comparaisons byte à byte des formats v2-v5, 3 076 rejets de limites/clés non canoniques, 1 024 annulations propagées. Données synthétiques, pas une qualification complète de la reprise ni de la production.

AMD64 : sélection de huit cas lancée avec le nouveau test et reprise témoin, slashing local, sortie, dépôt, coalescence, headers/reorg, partition. Réutilisation de l’objet de test et des bibliothèques 1ac57e25 après contrôle d’équivalence des sources ; deux objets candidate recompilés. Programme de test ainsi composé identifié par manifeste. Pas de qualification d’un nouveau binaire nano complet ou d’une nouvelle image.

Première préparation arrêtée pendant la compilation, avant tout test : le contrôleur accordait par erreur 90 s à la coalescence, dont le délai original est 60 s. Tentative et scripts conservés sur VPS sous `...-timeout-preflight-*`. Contrôleur corrigé : 60 s, autres délais originaux conservés. Aucun résultat ne provient de cette tentative.

Résultats AMD64 terminaux : huit cas exacts, sept passent, un échoue, zéro skip. Partition timeout 240,027 s ; dernière observation slot 96, finalité commune epoch 10 et quatre observations consécutives de tête identique, assertions finales non atteintes. Manifeste sources/programmes/bibliothèques inchangé avant/après, post-manifest à zéro. Aucun runtime main promu, aucun délai assoupli, aucun critère de production clos. Les échecs historiques restent conservés.
