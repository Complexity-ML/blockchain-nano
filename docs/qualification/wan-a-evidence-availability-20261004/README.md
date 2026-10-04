# Disponibilité de la preuve WAN A — 4 octobre 2026

Tentative d’inspection non mutatrice du volume historique référencé par
`capella-partition-load-20260930/attempt-a-forensics/summary.json` :
`nano-qualification-mac-munabbxp-data`. Il n’était plus disponible.
Docker a créé un nouveau volume vide à 19:39:27 UTC lors de la sonde ; lecture
`/evidence/core/CURRENT` échouée, code 1. Aucune base historique lue, aucun audit
canonique lancé, aucune signature ou reprise de duties. Le paramètre chain ID
fourni à cette première sonde était aussi incorrect (7668 au lieu de 1337) ;
il n’a pas été atteint puisque l’ouverture de la base manquante échoue d’abord.
Ne pas interpréter cet échec comme corruption de Nano ou comme anomalie de replay.

La date du nouveau volume et la liste vide de ses fichiers ont été vérifiées.
Seul ce volume vide créé par la sonde a été retiré, exit 0. Aucune autre image,
base ou qualification active modifiée. À l’avenir, inspecter l’existence du volume
avant `docker run --mount type=volume` : un montage read-only ne prévient pas
la création implicite d’un volume absent par Docker.

Les logs, acceptations, manifestes et 44 logs RocksDB archivés de l’incident
restent conservés dans `capella-partition-load-20260930`. La sauvegarde locale
de Docker du 4 octobre contient le build ARM64 et deux volumes de campagnes
plus récentes, pas ce volume historique. La cause initiale n’était pas retenue
par le runtime original. Aucun diagnostic de cause n’est déduit des traces seules.

**WAN A reste ouvert.** Un audit du dernier checkpoint de cet incident n’est
plus réalisable depuis ce volume ; rechercher une autre copie vérifiée avant
inspection ou obtenir une reproduction bornée avec diagnostics actuels. Aucun
nouveau test de réseau/charge n’est lancé dans cette étape, pour ne pas perturber
les campagnes Docker existantes. Ce constat ne clôt aucune exigence de production.
