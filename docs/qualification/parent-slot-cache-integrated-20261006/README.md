# Cache de préparations par parent et slot — qualification ciblée

Base du dépôt 81bcd6cb, cœur antérieur f077205f. Quatre fichiers modifiés : en-tête du réacteur, implémentation, test portable et enregistrement CMake. Les sources exactes sont identifiées par source-v2-manifest.json ; tous les consommateurs du type modifié ont été reconstruits après scan de chaque unité du cœur. Les unités indépendantes conservent les objets du cœur parent qualifié. Pas de nouvelle image Docker.

Contre-exemple réel sur clones locaux sans clés ni journaux : ancien cœur, bloc valide admis en 22 appels seul mais aucun progrès en 100 appels entrelacés avec un bloc invalide d'un autre parent ; arrêt de l'interférence puis admission en 21 appels. Correction Docker ARM : 22 appels avec ou sans interférence, état/en-tête conformes à l'oracle, rejet de l'invalide et duplicata stable. Stockages originaux inchangés.

Régression portable : ancien cœur échoue (sortie attendue 1) sur Docker ARM et AMD ; correction passe en 86 paires. État conforme à l'oracle, invalide rejeté, duplicata stable et témoin réouvert. Les quatre régressions pertinentes et ce nouveau cas donnent les mêmes résultats sur Mac natif et Docker ARM/AMD. Tests exécutés directement ; enregistrement CTest ajouté, pas de nouvelle campagne CTest globale revendiquée.

Les exécutables nano et paused-capture sont reconstruits avec le nouveau type et leurs empreintes sont conservées. Erreur native initiale de scan : en-tête secp256k1 introuvable (sortie 1), conservée ; réparation du chemin d'inclusion, réutilisation des objets déjà reconstruits avec sources inchangées, sortie 0. Ce n'est pas un échec de consensus.

Cache éphémère borné en nombre par les limites existantes, indexé par parent admis et slot ; aucune règle de signature, de quorum ou de publication modifiée. L'éviction ne supprime que du travail recomputable. Pression, coût/mémoire des caches, équité sous charge, finalité réseau, autres critères de production et revue indépendante restent ouverts. Aucun transfert de base entre Mac/VPS, fonds réels, vente ou déploiement public. Cette qualification n'établit pas encore la cause complète de v12 ni une finalité commune.
