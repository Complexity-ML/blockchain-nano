# Frontières après v13 — diagnostic hors ligne Docker ARM

Session originale 92763, sortie 0 ; compilation Docker et liaison avec le cœur 5787915e qualifié, empreinte de bibliothèque concordante. Image et volume existants réutilisés. Sources gelées vérifiées ; bases Mac arrêtées montées read-only et empreintes de tous les fichiers inchangées avant/après.

Les graphes contiennent 453 et 356 blocs (limite 1024). Les trois frontières à parent connu restent les mêmes qu’après v12 : slots 10751/10753 et 10752, parent 10574. Écarts de 177/179/178 slots. Les 16 votes récents de chaque source (32 total) référencent des blocs absents du destinataire ; la classification s’arrête avant vérification BLS.

Cela ne prouve pas une signature invalide ni la cause complète du réseau. Le contre-exemple de continuations par parent/slot est corrigé et qualifié séparément ; v13 reste FAILED. Prochain diagnostic : sélection et reprise du relais/catalogue sur les archives réelles, budget/ordonnancement des tentatives et admission des frontières. Ne pas assouplir les règles cryptographiques ni augmenter une limite sans cause démontrée. Production, WAN, ressources et revue indépendante restent ouverts.
