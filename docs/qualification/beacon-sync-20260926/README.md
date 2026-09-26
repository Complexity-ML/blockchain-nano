# Observation de synchronisation Beacon — 2026-09-26

Source locale non commitée basée sur `30b0de21a1225ba55603de437c1200f3437a1ecb`.
Binaire Nano, runner et nano-p2p compilés sur macOS ARM64.

## Chemin effectivement raccordé

1. `nano_bellatrixStatus`, protégé par le jeton du superviseur, expose le slot
   de tête issu du choix de branche natif. Calcul coopérativement borné à 2 s.
2. `nano-p2p` rafraîchit ce slot et l'annonce séparément de best_block. Aucune
   conversion d'une hauteur d'exécution en slot Beacon.
3. Seuls les slots des pairs compatibles avec l'identité/profil Bellatrix sont
   transmis dans le rapport interne authentifié. Cache local et statut du pair
   expirent chacun après 5 s ; les réceptions de statuts rafraîchissent leur âge.
4. Le RPC valide le format du rapport puis alimente un tracker partagé avec le
   runtime, lui aussi expirant après 5 s sur horloge monotone.
5. `GET /eth/v1/node/syncing` compare la tête validée au plus haut slot observé.
   Les nombres sont des chaînes décimales. is_optimistic et el_offline sont false
   car ce runtime n'expose que l'exécution native validée et disponible.

Rapport absent, expiré ou sans pair compatible : 503. Une annonce de slot au-delà
   de l'horloge locale est refusée avec 503, sans avancer le consensus. Un problème
   d'horloge ou de disponibilité du réacteur échoue également sans succès simulé.

## Qualification nouvelle

- Test HTTP `consensus_bellatrix_beacon_reads` : passé. Rapport non autorisé rejeté,
  hauteur EVM artificielle 999999 sans effet sur le calcul Beacon, distance 0 puis
  2 slots, annonce future refusée, format invalide rejeté, absence de pairs,
  expiration à 5 s et horloge monotone reculant. Aucune mutation de consensus.
- `consensus_bellatrix_managed_runtime` : passé en 13,52 s. Deux processus Nano,
  deux vrais démons P2P, clés publiques synthétiques. Observations présentes via
  HTTP sur les deux nœuds avant et après redémarrage, blocs propagés et en-tête
  signé canonique disponible via HTTP. Arrêt propre des processus de test.

## Portée et limites

Il s'agit d'une **observation des pairs connus**, pas d'une preuve de quorum,
finalité globale, disponibilité des données ou absence de partition. Les annonces
restent non fiables : un pair peut affecter cet indicateur, jamais l'admission des
blocs. Un réseau isolé est explicitement indisponible pour cette observation.
Les caches successifs peuvent cumuler leurs délais de fraîcheur ; ce n'est pas
une mesure instantanée du réseau. La file Assertoor n'a pas été exécutée ici.
Les corps de blocs REST et événements, ainsi que la complétude des métadonnées de
fork, restent ouverts. Aucun nouveau fork ni EIP activé par cette instrumentation.
