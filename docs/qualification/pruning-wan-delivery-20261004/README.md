# Livrables et préflight WAN pruning 092a153c

Sources figées Git 092a153ca1d203434beb14fe2102e3a092dc454c sur les deux hôtes.
Runtime src/include/network/third_party/cmake identique à 4734f285. Les deux
entrées nano Docker sont compilées/linkées contre les bibliothèques existantes
qualifiées ; nouveaux répertoires, ancien cœur/programmes inchangés. nano-p2p
copié depuis efbeb9bc avec hash exact, source network identique. Aucun rebuild
d’image ni de cœur Docker. Sources avant/après et livrables re-hachés par le
préflight avant lancement, chaînes de provenance explicitement conservées.

Mac natif : build neuf du cœur, nano et programme Capella ; nouveau scénario
pruned physical restore exit 0 sur macOS, 64 journaux avancés et finalité 2 → 7.
P2P natif efbeb9bc réutilisé après comparaison des sources et SHA-256. Sources
et binaires contrôlés, compiler/CMake et dépendances dynamiques conservés.
Pas de reconstruction hermétique depuis les sources de toutes dépendances.

Campagne privée désormais réellement démarrée : Mac natif, Docker Mac,
Docker VPS, 64 clés synthétiques sur deux hôtes via SSH. Profil Capella avec
pruning explicite et double audit de rejeu exigé. Contrôleur exact et préflight
archivés ici ; sortie terminale, restauration WAN et stabilité encore attendues.
Rapports live : /tmp/nano-pruned-wan-restore-092a153c-20261004 ; contrôle :
/tmp/nano-pruned-wan-restore-control-20261004. Aucune nouvelle production
publique, transaction de fonds réels, image construite ou qualification de
production complète. Source candidate efbeb9bc et anciens résultats préservés.
