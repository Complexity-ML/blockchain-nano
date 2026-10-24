# C8 : démarrage Docker et supervision de la clôture

Candidate `ed651411285dcbd653f158d1d3bc73186331d5f9`. Le pilote crée désormais chaque runtime Docker avec `--restart=no`, retient son CID, vérifie image/UUID/labels/état/politique avant démarrage, puis démarre ce CID et relit l’état. La promesse START est jointe même si la gate échoue ; l’arrêt du conteneur possédé reste possible après une dérive. Les inspections ajoutées projettent les seules métadonnées utiles, sans lire `Config.Env`.

Le superviseur externe accepte un contrat C8 typé avec budget/réserve concordants à la spec et aux bindings. Le mode historique v1 garde son contrat7h. C8 transmet le vrai flag `--closeout-reserve-ms`, conserve la réserve dans la durée globale et reconnaît le reçu de clôture lié au PID/source/spec/budgets. L’intention du sampler est écrite et fsync avant CREATE ; son cleanup utilise l’identité possédée. Les résultats C8 restent explicitement provisoires avant observation externe ; synchronisation tardive, interruption ou enfant non joint refusent la qualification.

## Vérifications réellement exécutées

- Image AMD64 existante, Node18.19.1, Docker sans réseau : **13 groupes passent, 0échec, 15 ignorés**, sortie Docker et contrôleur0. Huit groupes nouveaux couvrent le parser/coordinator et le superviseur C8 ; trois régressions ciblées couvrent le superviseur partagé, puis le garde réel du driver et l’inventaire transitif. Les53 anciens tests ne sont pas rejoués.
- Le vrai moteur du VPS vérifie avec `sleep` le parcours create/inspect/start/inspect, puis refuse un conteneur `always` encore créé, jamais démarré. Les deux CID exacts sont supprimés ; Root observe séparément leur absence et celle du lecteur de tests. Aucun Nano, clé, mount privé, volume, image ou réseau créé, aucune image reconstruite/téléchargée.
- Le contrôleur clôture ses31 FD tenus (sélection des sources, helpers et19 providers Node). Le bundle Git lie les sources testées à la candidate ; la fermeture complète des providers système et de tous les fichiers non tenus n’est pas revendiquée.

Les scénarios de budget utilisent des horloges/handles injectés, des fichiers synthétiques Linux et des fonctions du vrai pilote. Ils ne prouvent pas9h réelles, fsync Darwin, absence universelle d’acteurs après crash, rattrapage des descendants, ni une reprise de validation ou une finalité commune. Les contrôles de partition restent hors exécution de ce lot. L’ancienne politique Docker Mac03 demeure inconnue ; le moteur Mac n’a pas été démarré.

Les propositions source NO_GO et préparations refusées restent conservées : état pré-start trop permissif, promesse START non jointe, validations de stop après dérive, cleanup sampler trop strict, verdict tardif, garde d’ordre absent, image Volumes via champ manquant, et première hypothèse de compteur23 face aux28 groupes présents. Les deux dernières préparations ont échoué avant tout conteneur de test ; aucun ancien reçu n’est modifié.

Voir les [résultats](result-public.json), le [TAP](stdout.log), l’[observation terminale Root](root-terminal-observation-public.json), les [cas réels Docker](daemon-policy-proof/result-public.json) et la [proposition04](CAMPAGNE-04-PROPOSITION.md). La documentation Docker confirme le rôle des [politiques de redémarrage](https://docs.docker.com/engine/containers/start-containers-automatically/) ; les comportements revendiqués ici ont aussi été observés sur le moteur du VPS.

Campagne03 signée/échouée, accord consommé ; aucune04 autorisée. Paris/Shanghai seul profil activé, aucun EIP promu. La copie Native hors ligne qualifiée précédemment reste distincte des duties/restaurations des trois nœuds encore à démontrer. CI/reproductibilité/exploitation et revue indépendante non réalisée gardent leurs exigences ; Base–Nano1:1 reste proposé/non adopté, bridge/Doppler/T0 non qualifiés. Aucun fonds, vente, déploiement public ni dépense ; objectif actif, production non qualifiée.


## Accord recu apres les tests

L’utilisateur a ensuite [autorisé distinctement04](campaign04-authorization-public.json), pour neuf heures maximum clôture comprise, y compris le démarrage contrôlé de Docker Desktop. Les mentions «04 non autorisée» précédentes et la proposition décrivent leur état avant cet accord ; elles ne sont pas des refus actuels. Aucun domaine frais n’a encore été créé par ce reçu et les précontrôles restent obligatoires.
