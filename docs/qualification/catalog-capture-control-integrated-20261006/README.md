# Capture catalogue hors verrou de contrôle — qualification intégrée

Parent main 000c9c85, cœur parent d285db64. 349 sources figées et vérifiées avant/après sur Docker ARM64 et AMD64, puis comparées au worktree. Scan des dépendances : seuls rpc.cpp et consensus_capella_runtime.cpp reconstruits, autres objets réutilisés du cœur qualifié inchangé. Les runtimes nano, lecteurs et fixtures sont liés avec le nouvel archive. Images/volumes existants ; aucune image reconstruite. Aucun nœud de campagne ni stockage de campagne ouvert.

La capture est sérialisée par un verrou distinct. Le registre reste disponible pour génération/release/stop pendant la capture et son attente du réacteur. La capture réserve son budget restant jusqu’à son drainage ; la publication revérifie arrêt, génération, retraite de requête et lease. Les quotas restent inchangés. Une capture échouée détruit son snapshot avant de libérer la réservation.

Originaux ARM64 1980, AMD64 4618 et natif cohérent 65381 : sorties 0 vérifiées. Cinq modes concernés passent sur les trois : concurrence catalogue (32 travailleurs, quotas, doublons et 128 courses release/acquire), runtime configuré, reprise automatique, HTTP, nouveau contrôle de capture. Ce dernier vérifie séparément retrait de requête, arrêt, expiration et annulation : contrôles réactifs pendant la capture retenue, publication tardive refusée, primaire inchangé ; capacité récupérée sauf arrêt permanent.

Premiers pilotes Docker : erreur de montage, sorties 125, avant compilation ; commandes/logs/reçus conservés. Réparation uniquement du chemin de montage après terminal original, mêmes sources/images/volumes. Les pilotes natifs précédents liés au cœur parent ne sont pas utilisés comme qualification du runtime cohérent.

Ce lot ne prouve pas la cause WAN et ne qualifie aucune finalité distribuée. La file HTTP commune peut encore retenir ses deux travailleurs sur une capture et un acquéreur en attente : sa contention et la propagation des votes récents restent ouvertes avant une nouvelle campagne WAN. EVM/forks, autres critères de production, bridge, lancement et revue indépendante restent ouverts. Pas de fonds réels ni de vente.
