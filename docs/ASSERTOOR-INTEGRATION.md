# Raccordement Assertoor : interfaces réellement requises

Inspection du 2026-09-26, source ethpandaops/assertoor épinglée au commit
`89366be6cb211522249a6d9424adb47631864432` (copie locale `.deps/assertoor`).
**État actuel :** la campagne de finalité à deux validateurs échoue (époque 0,
timeout). [Échec ouvert](qualification/assertoor-finality-20260926/README.md).
La campagne courte d’Assertoor officiel a exécuté trois tâches contre deux processus
Nano : santé EL/CL, synchronisation avec progression et deux propositions natives.
[Rapport et limites](qualification/assertoor-native-20260926/README.md).
Le catalogue complet, la finalité et les partitions via Assertoor restent ouverts.
Les sections chronologiques ci-dessous conservent les étapes du raccordement.

`pkg/clients/consensus/clientlogic.go` initialise le client HTTP, puis exige
version du nœud, genesis, paramètres et synchronisation. Sa boucle lit ensuite
la tête, les checkpoints, les en-têtes et corps des blocs, et souscrit aux
événements. Les appels passent par `pkg/clients/consensus/rpc/beaconapi.go`
et `github.com/ethpandaops/go-eth2-client v0.1.6` ; l'initialisation de cette
bibliothèque et la spécification Beacon API doivent aussi être vérifiées.

| Lot | Données à exposer à partir du consensus natif | Critère de clôture |
| --- | --- | --- |
| Lecture initiale | Version, genesis_time, genesis_validators_root, fork version, paramètres cohérents avec le profil, synchronisation et mode optimiste | Démarrage réel du client Assertoor ; aucune valeur Ethereum par défaut inventée |
| Suivi du réseau | En-têtes signés, blocs par racine/slot, checkpoints justifié/finalisé, événements | Les engagements SSZ et EVM concordent avec l'état natif ; suivi des deux nœuds et des réorganisations |
| Cycle des validateurs | Registre, statuts, soldes, devoirs, fork ; opérations de sortie et changements de credentials selon fork actif | Scénarios Assertoor explicitement sélectionnés et exécutés ; opérations non activées rejetées |

Nano peut servir ces API dans le même binaire ; aucune séparation EL/CL ni
Engine API ne découle de ce besoin d'observation. Conserver la protection du
RPC interne. Ne jamais exporter ses jetons aux outils ; prévoir un endpoint
Beacon avec politique d'accès explicite et des lectures bornées/snapshots
cohérents, plutôt qu'une conversion des numéros de blocs EVM en slots Beacon.

Le test de raccordement devra aussi distinguer une erreur transport, un endpoint
absent, un fork non pris en charge et un désaccord d'état. Aucun statut vert ne
doit être obtenu en simulant l'état de synchronisation ou la finalité.

Source inspectée :
https://github.com/ethpandaops/assertoor/blob/89366be6cb211522249a6d9424adb47631864432/pkg/clients/consensus/clientlogic.go

## Première intégration HTTP — 2026-09-26

Le runtime Bellatrix fournit maintenant un handler en lecture seule à `HttpServer`.
Trois GET sont implémentés pour l'identifiant **head uniquement** :

- `/eth/v1/beacon/states/head/root`
- `/eth/v1/beacon/states/head/fork`
- `/eth/v1/beacon/states/head/finality_checkpoints`

Les réponses viennent d'un même snapshot immuable du réacteur, vérifié par les
engagements Beacon et EVM. Les checkpoints proviennent du BeaconState de tête,
pas des checkpoints remontés du fork choice. Les nombres Beacon sont des chaînes
décimales. Le runtime n'admet pas d'exécution optimiste ; le booléen `finalized`
compare la tête au checkpoint finalisé du snapshot.

Les GET utilisent la file bornée existante (2 workers, 8 réservations communes
aux simulations), un délai d'attente de file et une annulation coopérative après
2 secondes de calcul. Cela n'est pas une borne temps réel de l'attente du mutex
interne. Bind loopback obligatoire, Origin refusé, aucune mutation Beacon via
POST ; exceptions masquées par une réponse 503. Les routes absentes renvoient
404, sans simuler un état de synchronisation.

Qualification : `consensus_bellatrix_beacon_reads`, serveur HTTP réel attaché à
un runtime démarré depuis un checkpoint synthétique authentifié, et
`consensus_bellatrix_runtime_restart`. Résultats détaillés dans
`qualification/beacon-reads-20260926/`. Cette tranche ne qualifie ni Assertoor,
ni les performances sous charge, ni les routes complètes de ces API (autres
identifiants d'état). Elle n'active aucune nouvelle EIP ni fork réseau.

À cette première étape, restaient avant le suivi réel Assertoor : genesis et historique de forks
explicites, configuration complète du profil, version/synchronisation fiables,
en-têtes signés et blocs archivés, événements. L'ancre initiale peut ne pas avoir
de signature de bloc archivée : ne pas en fabriquer une pour satisfaire le SDK.
L'accès depuis un conteneur Assertoor reste à concevoir en conservant la frontière
du RPC interne (le serveur actuel reste loopback).

Schémas officiels consultés le 2026-09-26 :
- https://github.com/ethereum/beacon-APIs/blob/master/apis/beacon/states/root.yaml
- https://github.com/ethereum/beacon-APIs/blob/master/apis/beacon/states/fork.yaml
- https://github.com/ethereum/beacon-APIs/blob/master/apis/beacon/states/finality_checkpoints.yaml

## En-têtes signés et changements de branche — 2026-09-26

`/eth/v1/beacon/headers/{block_id}` est désormais raccordé au runtime : head,
finalized, racine et slot, à partir de l'archive signée du même snapshot que le
Store. Revérification du corps SSZ et de la signature BLS avant projection JSON.
L'absence de signature conservée reste un 404, notamment sur l'ancre importée ;
aucune signature n'est fabriquée. Le champ canonical et la sélection par slot
ont été vérifiés pendant deux changements de tête induits par des votes valides,
avec conservation des réponses d'un snapshot antérieur.

[Preuves et limites](qualification/beacon-headers-20260926/README.md).
Cette extension remplace la mention précédente d'en-têtes entièrement absents.
Les corps de blocs REST, événements, genesis, configuration et synchronisation
restent à raccorder ; Assertoor n'a toujours pas été lancé contre Nano.

## Métadonnées et prérequis SDK — 2026-09-26

Genesis, version et projection des paramètres du profil sont désormais servis
et testés en HTTP natif. La spec reste **partielle**, sans historique de forks
inventé ni fausse déclaration de preset Ethereum. Nano doit déclarer son propre
calendrier d'activation ; reproduire les migrations historiques d'Ethereum
n'est pas un prérequis en soi.

L'inspection de `go-eth2-client v0.1.6` a identifié le premier verrou réel :
`http.New` appelle `NodeSyncing` pour décider si le service est actif, avant les
lectures de genesis/spec. La prochaine interface nécessaire est donc une mesure
de synchronisation native fiable. Ne pas retourner systématiquement false,
ne pas désactiver ce contrôle pour faire passer le client.

[Résultats, provenance et limites](qualification/beacon-metadata-20260926/README.md).

## NodeSyncing natif — 2026-09-26

Le prérequis NodeSyncing est maintenant raccordé aux observations réelles de
slots de pairs Bellatrix compatibles. Le rapport P2P interne est authentifié,
les observations expirent et les annonces futures sont refusées. Un nœud sans
observation exploitable renvoie 503 ; aucune valeur is_syncing=false constante.
Le statut mesure le retard face aux pairs connus, pas une finalité globale ni
une preuve d'absence de partition.

Tests HTTP des cas adverses et test de deux processus / deux démons P2P avec
redémarrage passés : [preuves](qualification/beacon-sync-20260926/README.md).
Le SDK/Assertoor reste à exécuter ; corps de blocs et événements restent à fournir.

## Corps de blocs et première exécution du SDK — 2026-09-26

La route v2 des blocs sert désormais les archives natives en JSON et SSZ avec
version Bellatrix. Le SDK exact d'Assertoor s'est initialisé contre un vrai nœud
Nano, a lu ses métadonnées et son bloc, puis a confirmé l'égalité JSON → SSZ avec
l'archive. Les huit corps de fixtures officielles donnent aussi une égalité exacte.
[Preuves, commandes et limites](qualification/beacon-blocks-20260926/README.md).

Cette preuve remplace la mention de corps REST absents et de SDK jamais exécuté.
La suite Assertoor complète reste non exécutée ; flux d'événements, accès depuis
son processus d'orchestration et complétude des métadonnées restent à qualifier.

## Événements SSE natifs — 2026-09-26

`block` et `finalized_checkpoint` sont maintenant raccordés à la publication
après commit durable. Le SDK a reçu un événement de bloc réel et vérifié son
en-tête par sa racine. Le journal borné, les refus HTTP, la perte de curseur,
les doublons, la finalité et le redémarrage passent cinq tests ciblés.
[Preuves et limites](qualification/beacon-events-20260926/README.md).

Cela remplace la mention d'un flux entièrement absent. Les autres topics ne
sont pas implémentés ; la suite Assertoor complète, son déploiement et les
scénarios système restent ouverts. Aucun historique SSE durable n'est promis.

## Première campagne Assertoor — 2026-09-26

Le binaire officiel, sans patch, passe les trois tâches définies dans
`tests/assertoor/smoke.json`. La cible optionnelle
`consensus_bellatrix_assertoor` permet de rejouer ce scénario borné avec preuves.
Cela clôt le démarrage et l'observation de base, sans clore la finalité,
les scénarios adverses, le cycle des validateurs ni le catalogue Assertoor.
[Résultats détaillés](qualification/assertoor-native-20260926/README.md).

## Échec ouvert : finalité avec Assertoor — 2026-09-26

La nouvelle cible `consensus_bellatrix_assertoor_finality` répartit les 64 clés
sur deux processus. Santé, synchronisation et propositions passent, mais la
finalité reste à l'époque 0 : timeout après 240 s, campagne 265,84 s. Les reçus
natifs confirment ce résultat ; deux lectures Beacon 503 sont également observées.
Cause non établie. Priorité : contention RPC/consensus et propagation des votes,
avec témoin sans Assertoor ; ne pas assouplir le seuil d'époque finalisée 2.
[Preuves et diagnostic](qualification/assertoor-finality-20260926/README.md).
La finalité sous cette supervision n'est pas qualifiée ; les partitions Assertoor
et la clôture production restent ouvertes.

- 2026-09-26, diagnostic de finalité : le témoin **sans Assertoor** échoue aussi
  à retrouver des têtes communes (253,70 s), malgré un checkpoint finalisé commun
  époque 2 à l'arrêt. Un ordonnancement qui saute les catégories vides corrige
  une limite de capacité du relais, mais ne suffit pas : Assertoor reste en échec
  (269,82 s, finalisée 0). Sorties et slashing passent (10,30/5,33 s).
  [Preuves avant/après](qualification/bellatrix-relay-scheduling-20260926/README.md).
  Résolution toujours ouverte. Conservation automatique des bases et des blocs
  de tête ajoutée pour le prochain diagnostic ; aucun changement de quorum.

- 2026-09-26 — Critère de réussite continue : nouvelle cible
  `consensus_bellatrix_assertoor_sustained`, quatre avancées consécutives de
  finalité commune puis quatre après redémarrage, avec cohérence Beacon/EVM.
  Le diagnostic distingue le défaut JSON/SSZ de la sonde, le retard réel du
  relais et un build sans optimisation. Cache de validation historique ajouté
  sans modification des règles ni des checkpoints ; trois cibles Release
  réussies (9,48 s). Campagne prolongée **réussie en 616,36 s** : quatre
  avancées communes 2→6, puis quatre 7→11 après redémarrage ordonné ; 64
  observations sans écart de tête ou d’époque finalisée. Qualification locale
  bornée en Release, sans conclusion de production ou de tenue indéfinie.
  [Preuves et limites](qualification/assertoor-native-finality-20260926/README.md).

- 2026-09-26 — Reprise prolongée : **SIGKILL distribué réussi** (325,89 s),
  finalité commune 2→3→4→5. **Partition prolongée en échec** (521,06 s) :
  sûreté conservée pendant 96 s d’isolement et convergence ultérieure, mais
  retard de finalité supérieur à quatre époques au début du contrôle prolongé.
  Le succès du redémarrage ordonné ne clôt donc pas la reprise après partition.
  [Preuves, critères et limites](qualification/recovery-sustained-20260926/README.md).
