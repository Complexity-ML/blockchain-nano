# Cycles de partitions — tentative A terminée en échec

Deux coupures effectives, de 96 puis 192 secondes demandées, sur les images
candidates finales ARM64/x86-64 avec Mac natif. Trois processus sur deux hôtes,
64 clés synthétiques, Capella/Shanghai. Le premier cycle passe en 142,925 s de
reprise observée. Le second manque le délai initial de 180 s : reprise commune
observée en 187,621 s. Le verdict de lancement est exit 1 ; ce dépassement reste
un échec malgré la réussite de la fenêtre tardive 15→18 en 138,883 s.

Les validateurs terminent sains (420/396/401 attestations admises). Les deux
conteneurs sont arrêtés, exit 0 sans OOM. Les états terminaux, relevés ressources,
observations et journaux complets sont archivés. Les volumes de données et les
journaux de signature restent conservés pour analyse. Le résumé de ressources
porte sur les échantillons, pas les maxima continus ni l’empreinte disque.

`deadline-result.json` est la première capture intermédiaire, conservée avec
son statut historique. Les archives de logs ont ensuite été complétées à la fin
de la campagne ; `terminal-result.json` donne le verdict terminal.
La cause du délai n’est pas encore établie. Aucun budget modifié, aucune relance,
aucune qualification globale de production.
