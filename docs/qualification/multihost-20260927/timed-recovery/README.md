# Reprise WAN avec relais horodatés

Cœur/images `80493b7ade8d617d0d5f667943419790d8cff59d`, banc `398148d`.
Trois instances sur deux hôtes, 64 clés synthétiques, slots 6 s, epochs 8 slots.
Commande : `NANO_TEST_NETWORK_TRACE=1 NANO_TEST_WAN_PARTITION=1 node tests/multihost/run.mjs ubuntu@145.239.199.166 80493b7ade8d617d0d5f667943419790d8cff59d /private/tmp/nano-wan-timed-20260927`.

## Résultat

Finalité initiale commune 2, coupure configurée 96 s. Reconnexion à
1790465492639 ms ; nouvelle finalité commune 8 observée à 1790465680181 ms,
soit **187542 ms** : **échec du seuil initial 180 s**, sortie globale 1.
La fenêtre tardive **8→9→10→11 en 141066 ms** passe et lie les checkpoints à
l’exécution. Audit de 209 observations : aucun conflit de racine par epoch
ni régression enregistrée ; cela ne prouve pas seul l’ascendance.

Trois boucles saines : attestations 265/257/253, propositions 33/37/35,
cycles maximum 635/613/4046 ms, aucun au-delà du slot. Après terminaison,
les deux conteneurs `nano-qualification-{mac,vps}-muj0r7p7` sont `exited`
et le PID natif 16416 est absent. Bases et journaux conservés.

## Nouvelle information sur le retard

`inclusion-timing.json` corrèle 477 des 519 inclusions de la branche finalisée
à 2546 événements de relais datés. Deux événements portant `root: 0x` sont
des remises à zéro du curseur d’inventaire : ils sont comptés séparément,
pas interprétés comme des transferts. Aucun événement daté manquant n’est
inventé. Le parseur a initialement refusé ces deux événements ; sa correction
porte uniquement sur cette distinction explicite.

Six exemples du slot 69 (`forwarding-examples.json`) montrent un écart de
17597 à 18171 ms entre la fin du relais natif vers le VPS et la première fin
de relais du VPS enregistrée pour les mêmes octets d’attestation. Les votes
avaient été émis aux slots 61–63. L’intervalle d’écart VPS/Mac échantillonné
est [-9,+353] ms avant partition et [-10,+355] ms avant arrêt. La VM Docker
Mac donne [-6,+54] puis [-8,+47] ms. Ces mesures bornées ne garantissent pas
l’absence de sauts d’horloge entre échantillons.

Ce délai recouvre attente de redistribution, transferts, réception et
admission distante ; il n’est pas une mesure de latence réseau pure ni une
preuve du moment de disponibilité chez le proposant. Il oriente la prochaine
investigation vers le parcours d’inventaire et l’ordonnancement des relais
VPS après rattrapage. Aucune cause unique ni correction WAN démontrée.

Reproduction de la corrélation depuis la racine du dépôt :
```
node tests/multihost/inclusion-timing.mjs docs/qualification/multihost-20260927/timed-recovery/native-checkpoint.json.gz docs/qualification/multihost-20260927/timed-recovery
node tests/multihost/audit-history.mjs docs/qualification/multihost-20260927/timed-recovery
```
L’archive contient des états synthétiques publics, pas les clés de signature.
Décodage diagnostique des blocs admis, sans revalidation SSZ/BLS. Pas de
charge utilisateur, de fonds réels, d’endurance ni de qualification production.
