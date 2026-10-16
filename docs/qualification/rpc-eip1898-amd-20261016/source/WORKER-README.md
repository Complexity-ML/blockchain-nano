# Variante AMD du worker RPC

Proposition source uniquement, hors dépôt ; aucun Perl, compilateur, programme, SDK, conteneur ou accès VPS n’a été exécuté ou lu par l’auteur. ROOT doit revoir ce lot avant son gel et sa qualification réelle.

Le worker ARM figé `5c45bb5a…/14145` est repris intégralement avec trois substitutions sur deux lignes : UID réel/effectif `1000`, propriétaire de `/outputs` `1000:1000`, et format public `nano-rpc-eip1898-amd-thin-qualification-v1`. `worker.patch` contient exactement ce delta. Tous les chemins, commandes, contrôles FD/path GNU-stat, M=MD, bornes monotoniques, nettoyage et oracles restent identiques. L’original ARM n’est pas modifié.

L’entrée reste `/usr/bin/perl /controls/worker.pl`, sans argument, avec les contrôles `recipes.json` et `source-expected.private.json` sous `/controls`. `/outputs` doit être neuf, vide et `0700`, ici sur l’hôte Linux sous `1000:1000`. Les sources, le Core et les autres anciens artefacts restent en lecture seule. ROOT fournira les recettes/pins retenus, les sources Git publiques à `1f6c0d04…`, le Core AMD `3816…/40815352` et l’image immuable `fd9…` ; aucune ancienne carte SDK privée n’est réauditée ou récupérée par cette proposition.

La qualification AMD est attendue : deux TUs et deux M=MD, copie du Core 143/142 avec seul membre RPC remplacé, ancien Core refusant exactement le premier objet après la baseline chaîne (`checks:4`, sortie 1), puis cinq groupes et 58 refus. ROOT doit vérifier le résumé réel de 120 checks. Le passage ARM en 48 461 ms est un antécédent ARM, jamais une réussite AMD. Budgets et limites demeurent inchangés : 180 s par commande, 590 s pour le worker, 600 s dans son contrôleur externe.

Ce lot ne lance aucune campagne, nœud, serveur HTTP ou duty. SDK global, autres méthodes EIP-1898, branches non canoniques conservées, candidat multiarchitecture, finalité, déploiement et production restent ouverts. Aucun résultat ni financement nouveau n’est revendiqué.
