# Cache des octets publiés — candidat non qualifié

Commit fa390c3b, branche `qualification/checkpoint-serialization-20261002`, poussé puis récupéré par Git sur le VPS. Une chaîne privée garde les octets canoniques du dernier checkpoint seulement après une publication durable réussie ; son transfert après commit est statiquement garanti sans exception. Les comparaisons exactes des octets remplacent le hash redondant de détection des doublons. À la réouverture, cache vide et repli cryptographique original conservé.

Même en cas de doublon, les vérifications du checkpoint sur disque, de son token et du témoin éventuel restent exécutées ; un échec rend le réacteur indisponible. Aucun checkpoint, token, format, règle de consensus, WAL ou écriture synchrone modifié. Un seul snapshot supplémentaire est conservé en mémoire, borné par la limite d’enveloppe : empreinte mémoire et comportement à qualifier.

Nouveau scénario `consensus_bellatrix_publication_cache` : retransmissions idempotentes puis modification du checkpoint couplé par un second writer ; la retransmission doit refuser le token devenu stale et fermer le réacteur. Résultat non acquis.

Un champ privé modifie la taille du réacteur : recompilation complète du cœur et des cibles nano/Bellatrix/Capella/tests stockage et témoin, sans réutiliser les anciens objets compilés avec le header précédent. Build `/build/published-bytes-fa390c3b-20261002`, image AMD64 existante ; pas de nouvelle image. Contrôleur sélectionne douze tests pertinents avec les délais CMake d’origine. P2P existant séparément identifié, pas de qualification globale d’une nouvelle image réseau.

Compilation réellement active constatée dans `nano-published-bytes-candidate-control-amd64-test`. Rapports `/home/ubuntu/nano-published-bytes-candidate-control-20261002-reports`. Attendre les résultats et les manifestes avant toute revendication de gain ou promotion. Le parent conserve 7/8 réussites et un timeout de partition au slot observé 96 ; tous les échecs historiques et critères de production restent ouverts.

## Échec de compilation conservé et reprise

Le build fa390c3b termine avec code 2 : cœur et binaire nano compilés, mais nouveau test incorrect (`record->payload` au lieu de `record->snapshot`). Aucun cas CTest exécuté ; aucune réussite ou échec runtime à déduire. Sources inchangées avant/après. Rapports complets conservés dans `build-failure-fa390c3b/`.

Correction test seulement 090e95df publiée sur la branche et récupérée par Git sur le VPS. `git diff --name-only fa390c3b HEAD` limité à `tests/test_bellatrix_transition.cpp`. Runtime et headers identiques : reprise dans le même répertoire de build, sans refaire les objets du cœur ni une image. Contrôleur et rapports distincts : `retry-control-090e95df/` et `/home/ubuntu/nano-published-bytes-candidate-retry-control-20261002-reports`. Container `nano-published-bytes-candidate-retry-amd64-test` observé actif, compilation du test en cours. Résultats des douze cas encore non acquis ; limites d’origine conservées.

## Résultats terminaux 090e95df

Douze cas exacts, dix réussites, deux échecs, aucun skip ; CTest et container code 8, contrôle post-manifeste 0. Sources, programmes (nano et P2P explicitement inclus) et bibliothèques identiques avant/après. Sources figées et cœur entièrement recompilé après changement de layout, puis test corrigé seulement ; aucune combinaison avec les anciens headers.

Passent : coalescence de l’horloge, checkpoint atomique couplé, erreur de synchronisation disque, reprise avec témoin, runtime avec témoin, slashing local, sortie volontaire, dépôt, reorg des headers Beacon et encodage canonique. Portées exactes du registre et sorties conservées dans `retry-terminal-amd64-090e95df/`.

Partition : timeout 240,053 s ; dernier slot 96, finalité commune epoch 10, quatre avancements consécutifs. Assertions finales non atteintes, même dernier slot que le parent : gain insuffisant pour clôturer le délai. Scénario du cache : arrêt en 0,008 s sur `unknown Bellatrix qualification arguments`, donc assertions du cache non exécutées. Ce défaut de harness est corrigé par bad3e0c0 (admission de son flag uniquement) ; aucun runtime/header modifié.

Rejeu limité à `consensus_bellatrix_publication_cache`, délai original 90 s, contrôleur `case-control-bad3e0c0/`, rapports `/home/ubuntu/nano-publication-cache-case-control-20261002-reports`. Container `nano-publication-cache-case-amd64-test` démarré ; résultat encore en attente. Pas de rejeu des dix succès, pas de nouvelle image. La production et tous les autres écarts historiques demeurent ouverts.

## Nouveau scénario bad3e0c0 — réussite bornée

`consensus_bellatrix_publication_cache` : 1/1 passe en 0,833 s, aucun skip, CTest/container/post-manifeste 0. Sources/programmes/bibliothèques inchangés avant/après. Sortie structurée : trois retransmissions conservent le token ; une modification du checkpoint par un second writer est détectée et le réacteur refuse de poursuivre. Assertion réellement exécutée après correction de l’admission du flag ; résultat et premier échec conservés distinctement.

Recompilation test seulement avec runtime et headers identiques, plus régénération CMake/recompilation automatique de kzg.cpp et relink du cœur visibles dans le build log ; pas de nouvelle image ou changement de source cryptographique. Rapports dans `case-terminal-amd64-bad3e0c0/`. Les dix succès de 090e95df restent ceux de cette campagne, pas dix nouvelles exécutions sur bad3e0c0. Partition 240,053 s/slot 96 toujours non résolue, pas de résultat global 12/12 ni promotion runtime main. Prochaine piste : copie évitable de la ligne JSON du store pour sa comparaison au cache, en conservant l’égalité de tous les champs sérialisés et les gardes de corruption existantes.
