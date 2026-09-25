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

## Rattrapage par de vrais démons libp2p

`consensus_altair_generated_chain_libp2p` réutilise la chaîne générée avec les
64 clés, puis initialise un second coordinateur depuis l'ancre. Les deux
coordinateurs ont des bases distinctes et deux services HTTP privés. Deux
processus `nano-p2p` échangent sur loopback via le protocole Altair existant :
le test ne copie pas les blocs dans le Store destinataire et n'appelle pas
son admission directement.

Le récepteur doit obtenir les 33 blocs signés et retrouver le même checkpoint
finalisé, la même justification, la même tête et exactement le même état SSZ
à cette tête. Tous les octets de blocs archivés sont comparés à la source.
Après arrêt des démons et drainage des services, un nouveau coordinateur restaure
le checkpoint disque du destinataire et doit conserver sa finalité et son archive.
Cette restauration réutilise l'instance de base ouverte ; la fermeture/réouverture
complète de la base source reste couverte par le scénario de génération.

Campagne : `build-cmake/qualification/altair-generated-p2p-20260925/final-tests.log`.
Le test n'est inscrit par CMake que si `NANO_NETWORK_EXECUTABLE` est fourni.
Une build qui ne fournit pas ce binaire n'a pas qualifié ce scénario réseau.

Portée précise : finalité distante reconstituée par rattrapage d'une chaîne
produite localement, et non finalisation collaborative en temps réel par deux
groupes de validateurs. Les horloges sont pilotées et le récepteur est placé au
slot final avant le rattrapage. Les opérations et attestations incluses dans les
blocs sont validées normalement ; la convergence exacte de toutes les dernières
attestations gossip n'est pas exigée ici. Les deux cœurs HTTP résident dans le
processus de test, les démons réseau dans deux processus séparés. Aucune partition,
perte réseau ou panne brutale n'est injectée dans cette campagne.

Le premier passage (`tests.log`) a reçu 33 blocs et restauré l'époque finalisée 2
en 87,37 secondes, génération locale incluse. La version finale du harness draine
les sorties des deux processus pendant l'attente et journalise la progression
du nombre de blocs reçus ; `final-tests.log` qualifie cette version. Ce temps
est celui du scénario borné et du relais séquentiel actuel, pas une mesure du
débit maximal ou une garantie de synchronisation en production.

## Interruption du transport pendant le rattrapage

`consensus_altair_generated_chain_libp2p_restart` ajoute une perte d'ACK après
admission puis tue le démon récepteur par `SIGKILL` après au moins huit blocs
persistés, avant la fin du rattrapage. Le test exige l'événement confirmant la
perte d'ACK et le code de terminaison par signal. Il arrête ensuite proprement
l'émetteur, observe un Store stable pendant l'interruption et relance les deux
démons avec leurs clés P2P existantes et de nouveaux curseurs de relais.

Tous les blocs du préfixe doivent être conservés à l'identique. La retransmission
depuis le début doit rejoindre les 33 blocs sans doublon d'archive, puis retrouver
la même tête, le même état et le même checkpoint finalisé que la source. Les
assertions finales et la restauration du checkpoint destinataire sont identiques
au scénario sans interruption. Campagne :
`build-cmake/qualification/altair-generated-p2p-restart-20260925/tests.log`.

La panne concerne le processus P2P récepteur, pas le cœur de consensus ni sa base.
Le cœur HTTP reste actif pendant la reconnexion. Le test ne simule pas une panne
disque, une partition entre groupes de validateurs autonomes ou un consensus
qui continue à produire pendant la coupure. L'émetteur est relancé explicitement
avec la nouvelle adresse du récepteur ; la redécouverte automatique d'adresse
n'est pas qualifiée par ce scénario.

## Branche concurrente correctement signée après finalisation

`consensus_altair_adversarial_finality` produit d'abord les 33 blocs et leurs votes,
puis utilise volontairement les clés de test hors du journal anti-slashing pour
construire deux blocs concurrents. Chaque candidat est soumis séparément à la
transition complète signée : proposant sélectionné, RANDAO, signatures BLS,
corps et racine d'état doivent être valides avant le test de choix de branche.

Le premier candidat est au slot courant et descend directement de l'ancre,
contournant donc le checkpoint finalisé de l'époque 2. Il est accepté par un
Store témoin encore ancré au genesis, mais refusé par le coordinateur ayant
finalisé l'époque 2. Le token durable et l'archive restent inchangés. Le parent
est connu et le slot n'est pas futur : le scénario ne se contente pas de vérifier
le rejet d'un paquet malformé ou d'une dépendance absente.

Le second candidat descend du parent de la tête, après le checkpoint finalisé.
Il diffère du bloc déjà produit au même slot et est également signé hors journal.
Son admission doit réussir et ajouter exactement un bloc concurrent à l'archive,
sans modifier le checkpoint finalisé. Le consensus conserve ainsi le traitement
normal des branches non finalisées ; la prévention locale des doubles signatures
n'est pas utilisée comme substitut à la validation des blocs reçus.

Le test ferme puis rouvre la base du coordinateur avec son engagement attendu.
La branche incompatible doit encore être refusée, sans changement de checkpoint
ni perte du bloc concurrent admissible. Campagne réussie :
`build-cmake/qualification/altair-adversarial-finality-20260925/tests.log`.

Cette preuve est locale et porte sur un checkpoint déjà authentifié et persisté.
Elle ne démontre pas une résistance à un quorum de validateurs compromis avant
finalisation, à un bootstrap utilisant une ancre hostile, aux attaques de longue
portée ou à une partition distribuée. Elle ne qualifie pas non plus l'inclusion
d'une preuve de slashing ou sa sanction économique pour les clés du scénario.
