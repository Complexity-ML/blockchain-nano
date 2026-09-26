# Lots de votes — finalité réussie, validateur défaillant

Révision 42a3b131a672b9a13eedf08872f18359a8af1c76, mêmes trois modes
d'exécution et 64 clés synthétiques. Le lanceur historique termine avec code 0.
La fenêtre de finalité 2 → 3 → 4 → 5 passe en 135047 ms. Le démarrage
jusqu'au checkpoint 2 prend 288444 ms après l'événement started.

Cependant Docker Mac indique healthy=false, « Proposal expired before provider »,
un seul poll, aucune proposition ni attestation admise. Le cœur continue à suivre
la chaîne. Les 43 autres clés sur Mac natif et VPS suffisent encore à dépasser
2/3 des 64 clés de poids initial égal : la finalité seule masquait cette panne.
La qualification globale de cette campagne est donc **en échec**.

Le nouveau validator-gate.mjs refuse cette campagne et accepte le témoin
with-phases. Le lanceur distingue désormais finality-passed de passed, ce dernier
n'étant émis qu'après contrôle des statuts terminaux des trois validateurs.
Un statut absent, ambigu, unhealthy ou sans attestation locale fait échouer le test.

Maximum proposition VPS : 986 ms contre 2010 ms dans with-phases ; maximum poll
1759 ms contre 2550 ms. Ce n'est pas une preuve causale de gain : un validateur
inactif change la charge et les deux exécutions n'ont pas la même durée.
Les logs et résultats d'origine sont conservés sans réécrire leur ancien passed.

Prochaine correction : gérer l'expiration normale d'un devoir de proposition
sans arrêter définitivement la boucle, tout en conservant la réservation durable
anti-double-signature et les véritables erreurs de fournisseur. Aucun déploiement
de fonds ni changement du quorum. Les processus de cette campagne sont arrêtés.
