# Correction du banc de durée RPC — qualification réseau en attente

Les quatre échecs observés dans la campagne ARM64 c10426b2 sont conservés dans
observed-failures.json : les workers terminent leurs 3 ou 24 tours avant la
finalité observée ou les 20 observations requises. Tous les clients terminent
sans erreur ; les deux scénarios soutenus voient le marker finalisé pendant
la charge. Cela identifie une couverture de durée insuffisante, sans établir
que tous les autres critères réseau sont satisfaits.

Le banc séparé répète des requêtes réellement exécutées jusqu'à la preuve de
l'observateur, en conservant les minimums historiques. Le mode simple est
plafonné à 45 secondes et le soutenu à 80 secondes, tous deux à 1024 tours par
worker ; les délais RPC s'appliquent également à chaque requête en cours.
Ce plafond ne promet pas une terminaison à la milliseconde : une requête déjà
lancée doit revenir ou échouer à son timeout avant la prochaine vérification.
Les scénarios many-client/all-node/overload gardent leurs tours fixes.

Les critères restent : marker finalisé pendant les workers actifs, 20
observations et cinq blocs d'avance pour le soutenu, état canonique inchangé
par simulation, certificats vérifiés, reprise et audit de tous les pairs.
Le rapport ajoute les tours et tentatives effectivement exécutés, plutôt que
les seuls minimums. L'observateur libère les workers même après une erreur ;
l'erreur et les critères restent bloquants pour le résultat.

Onze contrôles du helper passent via clang++ -DNDEBUG -std=c++20 -Wall
-Wextra -Werror, dont client trop rapide, minimum préservé, observateur bloqué,
deadline exacte et plafond de tours. Ce n'est pas un succès du réseau réel.
Le banc C++ complet et ses quatre scénarios sont à compiler et qualifier dans
Docker après les campagnes c10426b2 actuellement figées et en cours.

La campagne initiale a ensuite révélé deux échecs locaux supplémentaires
`rpc_cost_paris` et `rpc_cost_shanghai` : une seule simulation OOG par worker
se termine en environ 50–100 ms, avant la première avancée BFT. Le banc local
répète ces simulations OOG jusqu'à une hauteur positive observée, sous le
même plafond de 45 secondes / 1024 tours. Chaque tour vérifie toujours OOG
et l'identité RPC ; la certification du transfert, le rollback, l'audit et
la réouverture restent requis. Les scénarios successful/nested gardent leur
unique simulation et leur exécution signée à la quote. Le nombre réel de tours
est consigné. Les six scénarios corrigés restent à qualifier dans Docker.
