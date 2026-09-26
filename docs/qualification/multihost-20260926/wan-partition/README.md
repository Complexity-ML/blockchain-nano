# Partition WAN — reprise en échec

Cœur b1ccbac9ea805afed67ac33fd7b5f180669aed22, banc b9b885b.
Trois instances / deux hôtes, 64 clés synthétiques 22/21/21. Aucun fonds réel.
Le nœud VPS est déconnecté de son réseau Docker dédié pendant 96 secondes,
puis reconnecté à la même adresse. Aucun processus de validation n'est suspendu.
La topologie en étoile perd effectivement tous ses pairs pendant la coupure ;
les trois RPC continuent à répondre, avec la même finalité d'epoch 2.

Les connexions reviennent (1/1/2 pairs). La finalité reste à 2 jusqu'au délai de
reprise de 180 secondes : lanceur terminé avec code 1. La fenêtre ultérieure
de trois avancées consécutives n'a donc pas été exécutée.

Les trois statuts terminaux sont sains, avec 198/187/185 attestations et
27/22/27 propositions admises. Aucun poll au-delà d'un slot, mais maximum VPS
5866 ms (proposition 4967 ms) pour un slot de 6000 ms. Ce résultat ne s'explique
pas par une boucle arrêtée comme dans la campagne with-batch.

`duty-targets.json` regroupe les **échantillons** de traces de devoirs, pas tous
les votes ni leurs poids. Ils montrent des cibles différentes à l'epoch 6,
un retard de convergence sur Docker Mac à l'epoch 8, puis la même cible à
l'epoch 9. À l'arrêt, justification 3 sur les Mac et 9 sur le VPS ; finalité 2
partout. L'origine du retard (diffusion, sélection/inclusion, choix de branche)
reste à isoler à partir des bases conservées. Ne pas relever le délai pour
transformer cet échec en réussite ni réinitialiser les journaux de signature.

Les logs, empreintes du banc et des binaires, topologie et checkpoints sont
conservés. Les fichiers privés de configuration et les bases ne sont pas publiés.
Le test révèle un écart réel de reprise dans ce périmètre ; il ne démontre
ni finalités contradictoires ni perte de fonds. Les contrôles unitaires du banc
(9/9) ne remplacent pas cette qualification réseau échouée.
