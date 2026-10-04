# Pruning AMD64 terminal 4734f285

Build/link, CTest et contrôleur exit 0 : sept noms attendus, 7/7 sans échec/skip,
242,91 s. Revue indépendante : 16 495 sources re-hachées sur le VPS et
quatre programmes re-hachés dans le volume ; manifestes avant/après identiques.
Ancienne candidate efbeb9bc non remplacée. Image/volume existants réutilisés,
aucun build d’image. LastTest.log conserve les sorties complètes.
Même périmètre que les sept tests ARM64 : états historiques élagués et rejeu,
RPC historique explicite, fsync/WAL atomique, onze corruptions et régressions.
Cette révision n’inclut pas les deux nouveaux tests de restauration 3358d27b/94535766.
Portabilité locale qualifiée pour ce lot ; aucune preuve WAN/power-loss,
ni qualification complète de production. Runtime pruning non promu.
