# Décomposition du temps de relais

Le résultat de chaque tentative expose `stageUs` (microsecondes monotones) :
inventory, payload, ack et known selon le chemin parcouru. Le journal réseau
existant reprend ces champs pour les tentatives lentes ou refusées. `ack`
inclut l’ouverture du flux, la transmission et l’admission distante : ce
n’est pas une mesure de latence réseau seule. Les catégories répétées
s’accumulent pendant le parcours borné du cache.

Compilation locale du relais `nano-p2p` réussie. Test de progression du relais
réussi en 0,31 s, avec vérification des durées sur succès, rejet, acquittement
perdu et parcours du cache. Aucun changement de protocole ni de seuil.

Cette instrumentation n’est pas encore dans les images Docker b7e2748.
Elle ne corrige pas le retard intermittent ; prochaine campagne requise après
reconstruction des images pour départager contention locale et acquittement.
