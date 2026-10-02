# Diagnostic ciblé AMD64 WebSocket — contrôleur

Un seul rejeu du cas échoué est préparé pour distinguer file, verrou RPC, préparation et exécution, avec `NANO_RPC_TIMING_DIR` déjà présent dans le runtime. Les durées client seules (`latency-analysis.json`) ne permettent pas cette attribution. Le contrôleur a été analysé syntaxiquement ; aucun résultat diagnostique n’est encore acquis.

Il attend le propriétaire original PID 946683 en contrôlant sa ligne de commande réelle. Après disparition du propriétaire, les résultats et manifests terminaux doivent être présents, le cas 43 doit rester un échec, sources et programmes doivent correspondre au candidat 9e9ebed3. Il échoue sans lancer de test si ces conditions ne sont pas vérifiables.

Pas de compilation : image AMD64 gelée 0fa8a608… et volume conservé, sources et build montés en lecture seule. Un seul conteneur local, réseau extérieur désactivé, utilisateur 1000, capacités retirées, 2 CPU / 3 Gio. Répertoire temporaire distinct sur disque. Commande du registre d’origine, fork Paris, timeout test 480 s, limites RPC originales (file 4 s/exécution 5 s), charge et assertions conservées. SHA des trois programmes relus avant/après, sources comparées avant/après. L’instrumentation ajoute du coût d’écriture : une réussite diagnostique n’annule pas l’échec historique et ne remplace pas la qualification.

Aucun nœud WAN ni vente ni déploiement public. Le contrôleur ne contourne aucun gate de restauration. Après ces mesures, une correction doit être fondée sur une cause démontrée puis qualifiée. Aucune cause profonde prouvée à ce stade.
