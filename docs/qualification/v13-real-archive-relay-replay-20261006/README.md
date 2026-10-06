# Rejeu du relais sur archives post-v13 réelles

Lecteur/admission liés au cœur 5787915e et en-têtes du relais identiques au checkout 23272de4. Docker ARM, image/volume existants, réseau désactivé. Catalogue et export C++ réels ; réception par le réacteur réel dans une copie locale des fichiers chain uniquement. Aucune clé ni journal de signature copié. Bases originales read-only, empreintes inchangées après chaque diagnostic. Pas de nouvelle image.

Première compilation du diagnostic : sortie 1 (variable maximum redéfinie), conservée ; correction du nom, session 72274 sortie 0. Rejeu 32 cycles : 91 racines ACK, 21 tentatives refusées sur la frontière 10753, zéro sur 10751, aucune publication nouvelle. Ce résultat borné ne prouve pas un blocage définitif.

Rejeu distinct sur copie neuve, borne 128 cycles, session 35530 sortie 0 : frontière 10753 admise à sa 23e tentative, 81 publications réelles et 172 racines ACK. Frontière 10751 jamais envoyée. Les 77 cycles terminés avec head-admission-unconfirmed s'arrêtent avant le catalogue ; fin du diagnostic sur la même priorité, sans finalité réseau démontrée. Les tentatives peuvent encore contenir de la préparation incrémentale ; pas de preuve de blocage infini ni d'attribution complète à la campagne WAN.

Le transport est modélisé ; HTTP/tickets, pacing, concurrence, nouvelles propositions et horloge live ne sont pas reproduits. Le rejeu ne qualifie ni le WAN ni la production. Prochaine étape : déterminer les dépendances de la tête refusée et qualifier un accès borné au catalogue lorsque sa priorité est non confirmée, sans faux ACK ni assouplissement de consensus. Caches, ressources, finalité commune et revue indépendante restent ouverts.
