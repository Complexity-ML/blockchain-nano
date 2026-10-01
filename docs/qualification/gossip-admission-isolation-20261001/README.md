# Isolation du gossip étranger — 1 octobre 2026

Candidate isolée ff53e4d1a42d38171d1044ad1b6da54c50d4d9dd,
branche qualification/rpc-admission-20261001, pas intégrée dans main.
Le nouveau test échoue sur le précédent runtime bf173958, exit 8 :
foreign gossip consumed local admission budget. Résultat conservé.

Après shape/count/byte checks, la prélecture bornée extrait le chainId
sans parsing récursif. Seuls les éléments dont la chaîne étrangère est
établie sont exclus du budget ; les locaux ET malformés restent facturés.
Réservation atomique de tout le travail local avant ECDSA/submit. Résultats
acceptés/refusés gardent leur ordre. Limites 50 items, 8 Mio hex/lot,
1 Mio brut/item, burst 128/refill 64/s inchangées. Conversion hex reste
linéaire avant quota, allocation locale cumulée bornée par le lot ;
aucune protection DoS universelle ou fairness inter-clients déduite.

ARM64 : quatre noms exacts passent, no skip, 46.13 s : RPC (1.25),
isolation (0.06), restart/charge legacy BFT Paris/Shanghai (22.23/22.59).
Nouveaux contrôles : 550 refus étrangers en lots (plus éléments mixtes),
250 profondément imbriqués dans les dix premiers lots ; lot mixte ordonné,
high-s/local imbriqué rejetés, lot sans capacité refusé sans écritures,
capacité résiduelle préservée, malformed limité et refill vérifié.
Contrôles direct RPC précédents toujours présents et réussis.

Trois parcours transport passent séparément, no skip, 27.94 s : network,
signed_network, bft_network (9.22/9.39/9.32). Même runtime, nouvelle source
ff53e4d1. Exécutables/recettes/sélections/LastTest/JUnit conservés.

AMD64 en cours : Git fast-forward du checkout isolé après arrêt du banc
précédent, mêmes builder/volume et onze tests pertinents ; aucune image
neuve ni déploiement. Log : /home/ubuntu/nano-gossip-admission-amd64-20261001.log.
Campagne originale 400 toujours active, runtime/sources préservés.
Beacon/WAN, coût estimation RPC, same-chain flood, matrice complète et
huit critères initiaux restent ouverts. Production non qualifiée.
