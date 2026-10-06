# Reprise v12 : cache de votes intégré — échec terminal

Contrôleur original 70646, checkout 4612ddf8, core f077205f / relais 76bb2468. Sortie 1 après le délai original de 900 secondes : pas de trois avancées communes depuis l'époque 1284. Aucun redémarrage ni prolongation. Aucune compilation diagnostique pendant cette campagne ; compilations hors ligne seulement après sa sortie terminale.

Trois reçus d'arrêt originaux ; audits séparés read-only des engagements exacts, des journaux/témoins et de l'historique canonique. Empreintes des fichiers conservées. Cela ne qualifie ni le rejeu complet du fork-choice ni la portabilité physique entre architectures.

Diagnostic borné sur deux stockages Mac arrêtés : les 16 votes les plus récents de chaque source v11 puis v12 (32 par campagne) pointent vers des références de blocs/targets absentes du stockage opposé. Ces cas s'arrêtent avant vérification BLS ; ils ne prouvent pas une signature invalide. Les refus génériques en direct ne permettent pas une attribution complète. Priorité : expliquer les dépendances de blocs non admises, le rattrapage des branches et la propagation utile avant les fenêtres de gossip. Ne pas modifier les règles de signature pour accepter des votes sans références.

Frontières v12 Mac mesurées : 326 et 419 blocs, limite 1024 non atteinte. Premiers descendants absents de parent connu : slots 10751/10753 et 10752, parent commun 10574, soit 177/179 et 178 slots de préparation. Les graphes divergent avant les votes récents. Ces observations orientent le diagnostic des continuations longues sans prouver toutes les causes WAN.

Campagne, diagnostics, limites, erreurs et preuves préalables restent distincts. Saturation/pruning, causes WAN, finalité commune après restauration, portabilité physique, cycles/rotations, production et revue indépendante restent ouverts.
