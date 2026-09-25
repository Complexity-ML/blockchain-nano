# Slots et epochs Bellatrix

`prepare_bellatrix_slots` avance un état Bellatrix authentifié par l'appelant,
met en cache les racines d'état et de bloc de chaque slot et traite chaque fin
d'epoch dans l'ordre du protocole. La racine de chaque état inclut le champ
`latest_execution_payload_header` ; une racine Altair ne lui est pas substituée.
Le marqueur `last_processed_epoch` interdit le double traitement d'une epoch.
Octets et marqueur doivent ensuite être publiés ensemble par le stockage.

`prepare_bellatrix_epoch` réutilise les opérations d'epoch communes avec Altair
par une projection privée et réversible des 24 premiers champs. Cette projection
ne sert jamais à calculer les racines historiques d'un slot. L'en-tête
d'exécution est conservé octet pour octet, puis réinséré avec les offsets SSZ
recalculés après les modifications de listes. Les racines mises en cache et
l'accumulateur historique restent celles du schéma Bellatrix complet.

`BellatrixEpochRules` contient les règles d'opérations, limites de payload et
budget local d'état. Les paramètres de pénalité doivent provenir du preset
Bellatrix authentifié. Les tests utilisent `INACTIVITY_PENALTY_QUOTIENT_BELLATRIX`,
`MIN_SLASHING_PENALTY_QUOTIENT_BELLATRIX` et
`PROPORTIONAL_SLASHING_MULTIPLIER_BELLATRIX` du preset épinglé. Aucune sélection
automatique d'un fork réseau n'est introduite.

Sources : [traitement des slots](https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#process_slots),
[changements Bellatrix](https://ethereum.github.io/consensus-specs/specs/bellatrix/beacon-chain/),
[corpus officiel](https://github.com/ethereum/consensus-spec-tests/tree/bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908/tests/minimal/bellatrix/sanity/slots/pyspec_tests).

## Qualification

Le test `ethereum_official_bellatrix_full_slots` compare les post-états SSZ
complets aux sept cas officiels du preset minimal : deux slots isolés,
franchissement d'epoch, epoch vide, deux epochs vides, changement de balance
affectant la sélection du proposant et accumulateur historique. Les 100 slots
au total sont aussi exécutés un par un : le résultat et le marqueur doivent
être identiques au traitement groupé. Le manifeste conserve les octets originaux
Snappy, les états décompressés, les tailles et SHA-256.

Les essais supplémentaires couvrent :

- cinq refus de frontière, marqueur ou configuration d'epoch incohérente ;
- budget de slots épuisé et échec tardif de rotation sync, sans modification
  de l'entrée, puis nouvelle tentative réussie ;
- sept points d'annulation répartis dans le traitement d'epoch ;
- conservation d'un en-tête d'exécution non vide et de son extra-data lors
  d'un cycle historique complet, avec engagement dans la racine du premier slot ;
- refus de croissance d'état au-delà du budget lors du rollover historique.

Preuves : `build-cmake/qualification/bellatrix-slots-20260925/`.
Les cas officiels ne constituent pas une couverture exhaustive des pénalités
Bellatrix, des changements de registre ou des scénarios adversariaux.

## Reste à fermer

Ce composant ne vérifie pas à lui seul un bloc signé complet et n'active pas le
réseau. Le [parcours de bloc Bellatrix signé](BELLATRIX-TRANSITION.md) le raccorde
désormais au header, à l'exécution, au RANDAO et aux autres opérations. Restent
le choix de branche et la persistance atomique Beacon–EVM. Le passage Altair → Bellatrix
et le bloc terminal PoW restent distincts de l'avancement d'un état déjà Bellatrix.
