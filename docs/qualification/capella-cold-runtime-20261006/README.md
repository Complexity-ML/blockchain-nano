Statut : QUALIFIÉ dans le périmètre Docker local ARM64/AMD64 ci-dessous ; production et WAN restent ouverts.

# Runtime Capella froid et pruning compact — 2026-10-06

Parent : 88591cad159f206987f02293ca7b6808c87f83d3. Archives core du lot cold-reactor ARM64/AMD64, mêmes images et volumes ; deux unités core modifiées plus fixture, pas de build de release propre ni déploiement de nœud.

## Implémentation

La configuration `coldCheckpoint` est validée immédiatement après les limites du profil, avant publication de checkpoint ou ouverture du témoin : structure exacte, quantités canoniques positives, nombre/volume dans les budgets locaux. Migration ensuite au même point que précédemment, avant clock/duties. Le pruning froid compare les manifestes de références validés du résultat attendu et du candidat, au lieu de reconstruire deux enveloppes plates complètes. Le chemin legacy conserve sa comparaison originale.

## Campagne ajoutée

Gate `NANO_TEST_COLD_RUNTIME=1` dans la fixture runtime réelle : cache 8 records, budget encodé issu des limites de la fixture. Neuf refus de configuration sans checkpoint ni répertoire witness créé ; bootstrap monotone ; démarrage avec 64 clés/journaux/témoins synthétiques ; propositions signées et transactions exécutées sur quatre époques ; graphes publiés froids et marqueur de format conservé ; redémarrage de runtime, réouverture disque owner/witness à mi-parcours ; anchor finale différente de l'initiale (pruning requis). Reprise automatique avec engagements externes et rollback refusé. Un deuxième mode ajoute l'admission durable de changements BLS non vides via RPC local.

Les comparaisons de cache/format sont effectuées après les duties ; aucune mesure RSS, capacité de grand graphe ou effet WAN n'en découle. Le nombre de clés et les plafonds sont ceux de la fixture, pas une preuve de 10000 nœuds.

Une régression legacy pruning-automatic est exécutée pour le branchement de comparaison modifié. Conserver sources/commandes/.d/hashes/logs et chaque échec ; exiger sorties terminales et intégrité parent/programmes avant toute qualification.

## Ouvert

Capacité du graphe 1024 et V28 FAILED, finalité WAN commune, graph metadata/cold records et charge réseau à grande échelle, runtime physique restauré avec journaux historiques, crashs froids aux frontières du batch, GC des blobs et RSS global, pic mémoire de récupération PRIMARY-previous. Le pruning compact valide toujours les rows une par une ; ce n'est pas un pruning O(1). Pas de nouveaux forks EVM, bridge, Doppler, revue indépendante ou production qualifiés.

## Échecs et diagnostic conservés

ARM64 source initiale, contrôleur 19991 : terminal 1, real Capella catalog relay failed. Fixture v2 avec décisions détaillées, contrôleur 47593 : terminal 1 ; blocs livrés17 en1485ms, attestations refusées au stade open sans dépassement de délai. La fixture supprimait les erreurs RPC en null. Fixture v3 utilise core_capture_busy_hint officiel du transport de production et retry de capture1ms, délai total10s conservé. Cause observée : cinq erreurs -32005 Capella inventory capture busy, puis ouverture réussie et80 attestations livrées en13ms. Les erreurs inconnues/authentification ne sont pas transformées en retry. Parcours automatique ARM64-v3 terminal0 :32 blocs,64 clés, neuf refus config, neuf vérifications engagements, pruning et reprises. Autres modes et AMD64 restent EN COURS tant que leurs sorties ne sont pas collectées. Aucun test ayant échoué n’est supprimé de l’historique.

ARM64-v3 deux parcours froids terminaux0 (contrôleur19128 terminé1 uniquement pour le warm-pruning sans fixture) ; warm-repair contrôleur37707 terminal0, mêmes hashes programmes/core/parent vérifiés. Legacy :72 blocs,6 mouvements anchor, finalizedEpoch7,64 journaux réouverts,18 records retenus finaux. Source-v4 ajoute seulement la fixture publique de dépôt manquante (5554570octets), sans changement de code ni recompilation ;1616 fichiers comparés au worktree, zéro écart. AMD64 initial13154 terminal1 même refus de catalogue. AMD64 corrigé92481 actif ; source-v3 montée immuable, il échouera encore sur la fixture warm manquante si ses deux parcours froids passent. Puis exécuter seulement warm-repair avec le même binaire et source-v4, déjà transféré. Ne pas rejouer les deux modes froids réussis pour corriger cet emballage.

Mise à jour : mode cold-automatic AMD64-v3 terminal0,32 blocs/64 clés,pruning activé,neuf refus config et neuf vérifications des engagements ; logs terminaux collectés. Contrôleur92481 toujours actif dans cold-bls ; régression warm avec fixture réparée encore à exécuter après sa sortie. Qualification complète du lot et hashes après tous modes encore ouverts.

AMD64-v3 contrôleur92481 terminal1 : cold-automatic0, cold-bls0, warm-pruning1 sur entrée JSON absente. Logs complets, commandes et hashes conservés. Réparation uniquement warm lancée sous41583, source-v4 avec la fixture de dépôt, mêmes programmes/core/parent vérifiés avant le run ; aucune recompilation. Audit1616 fichiers Mac/VPS : zéro écart. Qualification entière et hashes après encore ouverts.

Clôture du lot : réparation warm AMD6441583 terminal0,72 blocs/528 votes,finalizedEpoch7,6 mouvements anchor,64 journaux réouverts ; hashes parent/programmes avant/après inchangés. Les deux modes froids et warm réparé sont terminaux0 sur ARM64 et AMD64 ; audit du worktree source1616 fichiers sans écart. Contrôleurs échoués conservés (catalogue puis fixture absente) ; aucune image rebuild, release propre, déploiement, finalité WAN ou gate production clos. Voir qualification-audit.json.
