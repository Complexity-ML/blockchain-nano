# Finalité Altair produite localement

Le test C++ `nano-altair-local-finality-tests` assemble les composants de tâche,
signature et admission existants. Il part de l'ancre officielle épinglée du cas
`shorter_chain_but_heavier_weight`, puis produit ses propres blocs et attestations.
Les blocs qui suivent ne sont pas rejoués depuis les fixtures officielles.

## Scénarios bornés

Le preset minimal fournit 64 validateurs, initialement actifs avec le même poids.
Chaque scénario couvre 33 blocs (quatre époques de huit slots, puis un slot) :

| Scénario CTest | Clés qui attestent | Résultat exigé |
| --- | --- | --- |
| `consensus_altair_local_generated_finality` | 64 | Époque justifiée au moins 3, finalisée au moins 2 |
| `consensus_altair_local_without_votes` | 0 | Justification et finalité restent au genesis |
| `consensus_altair_local_minority_votes` | 21 | Justification et finalité restent au genesis |

Tous les proposants sélectionnés continuent à produire des blocs, même dans les
scénarios sans quorum d'attestation. La minorité porte donc sur les attestations,
pas sur la disponibilité des proposants. Les clés proviennent du générateur de
fixtures public ; ce ne sont jamais des clés de production.

À chaque slot, le test admet les votes du slot précédent, choisit la tête,
dérive le proposant, génère son RANDAO, assemble le corps avec les attestations,
construit et signe le bloc via son journal dédié, puis utilise l'admission normale.
Il prépare ensuite les tâches des attesters et signe seulement les clés affectées.
Il ne fournit aucune signature de SyncCommittee ; les pénalités correspondantes
sont traitées par les transitions, sans désactiver leur validation.

Au slot 17, le coordinateur, sa base et les journaux de toutes les clés sont
fermés puis rouverts avec les engagements attendus retenus par le test. La
production continue. Il s'agit d'une réouverture propre, pas d'un SIGKILL ni d'une
simulation de perte disque. Les autres campagnes de panne restent distinctes.

Le test vérifie la monotonie de l'époque finalisée, le nombre de blocs archivés
et la cohérence du checkpoint finalisé avec celui de l'état du bloc choisi.
Au genesis seulement, il exige explicitement la racine nulle de l'état Beacon
et la racine du bloc d'ancrage dans le Store ; aux époques suivantes, il exige
l'égalité exacte des checkpoints.

## Preuves et limites

Campagne : `build-cmake/qualification/altair-local-finality-20260925/`.
`final-tests.log` contient les trois résultats et les compteurs de blocs/votes.
`genesis-assertion-failure.log` conserve un échec du premier test négatif :
l'assertion comparait directement les deux conventions de racine au genesis.
La correction porte sur l'assertion, pas sur le consensus.

C'est une qualification de l'assemblage local avec une horloge pilotée par le test.
Elle ne démontre pas la finalité d'un réseau distribué, l'indépendance des
opérateurs, la résistance aux partitions, le débit à grande échelle, un
planificateur opérationnel ou une liaison à l'exécution EVM. Les opérations sont
limitées aux attestations et au vote Eth1 fixe de l'ancre ; aucun dépôt ni retrait
réel n'est engagé. Le scénario minoritaire ne simule pas des doubles signatures.
