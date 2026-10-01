# Cache du contexte sync — Docker ARM64

Source `7bba333` : une seule entrée dérivée d’un parent admis immuable et d’un slot d’inclusion, sous mutex. État SSZ et projection communs aux messages et agrégats, remplacés quand la clé change, non persistés. Toutes les vérifications de signature, bornes temporelles, tokens, overlap et règles de quorum restent exécutées. Les compteurs contextBuilds/contextHits rendent la réutilisation observable.

Nouveaux contrôles : même clé réutilisée ; contexte partagé entre messages individuels et agrégats ; signature forgée refusée malgré le cache chaud ; remplacement lors d’un changement de slot ; retour au slot précédent ; parent changé au même slot reconstruit et ancien root rejeté, sans changement du checkpoint. La suite local-sync termine exit 0 : 40 blocs, 272 votes, finalité 3, deux stores et 64 journaux rouverts. La première compilation a été arrêtée explicitement (137) pour corriger l’ordre du nouveau test, qui réchauffait initialement le cache trop tôt ; log conservé, aucune qualification tirée de ce processus.

Le scénario scheduler-sync-rotation termine exit 0 : trois rotations, 215 agrégats vérifiés indépendamment, 216 blocs, finalité 25, reprise témoin et 64 journaux rouverts. L’audit de l’histoire canonique conserve explicitement historicalForkChoiceVerified=false et historicalStandaloneVotesReplayed=false.

Qualification locale ARM64 uniquement ; amd64 encore en cours au moment de cet archivage. Aucun gain WAN ou cause du délai H démontré. Les images réseau/natif sont antérieures au cache. Les causes WAN A/B/G, seuil 180 s et tous les critères de production restants demeurent ouverts.
