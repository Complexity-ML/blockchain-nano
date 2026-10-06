# Cause de terminaison du catalogue — qualification ciblée ARM64

Le premier refus reste dans reason/failureStage. terminationReason/terminationStage identifient séparément la cause d’arrêt. catalogExhausted distingue le parcours complet de closed (libération confirmée du handle). Aucun ACK, admission, quota, signature ni règle de consensus n’est modifié.

Contre-exemple sur l’en-tête 7c6c1fd3 : sortie 1 attendue, export-unavailable masqué par dependency-depth-limit. En-tête corrigé : cinq scénarios passent (export indisponible, réponse absente, limite d’entrées, catalogue épuisé avec refus, expiration). Les 46 scénarios existants du catalogue passent aussi. Image ARM64 existante réutilisée, sans build d’image ni réseau, après arrêt v15. Sessions 18894 et 64191 : sortie 0.

L’essai initial 60169/sortie 1 est conservé : l’inclusion entre guillemets sélectionnait involontairement l’en-tête corrigé pour le baseline. Ce résultat initial ne valide aucun contre-exemple. L’inclusion explicite a été corrigée avant le baseline probant.

Qualification CMake/SDK, Mac natif/AMD64 et intégration des nouveaux binaires réseau restent à faire. La cause WAN de v15 et la convergence commune restent ouvertes ; aucun test réseau n’est validé par ce contre-exemple.
