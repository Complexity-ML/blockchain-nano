# Qualification Mac natif + Docker Mac + Docker VPS

## Images candidates identifiées — 30 septembre 2026

`NANO_TEST_CANDIDATE_MANIFEST` sélectionne deux identités Docker immuables
revues, une par architecture. Le lanceur refuse de combiner cette option avec
`NANO_TEST_BUILD_VOLUME=1`, vérifie révision/architecture/utilisateur 10001 et
les deux empreintes depuis chaque image avant de créer les fixtures. Quatre
tests couvrent sélection invalide, provenance divergente et binaires altérés.
Le volume `/build` n’est pas monté et les exécutables `/usr/local/bin` sont
utilisés. Le relais P2P VPS reste en loopback derrière le tunnel SSH ; aucun
RPC public. Exemple borné depuis la racine du dépôt, sortie obligatoirement
nouvelle :

```sh
NANO_TEST_CONSENSUS_PROFILE=capella \
NANO_TEST_FINALITY_MONITOR=1 \
NANO_TEST_CANDIDATE_MANIFEST=docs/qualification/candidate-images-20260930/manifest.json \
node tests/multihost/run.mjs ubuntu@145.239.199.166 \
  ca780d5002c72b2486833f8dbe881649787a6c4f /tmp/nano-packaged-network-fresh
```

Cette sélection permet le test de réseau/moniteur autonome. Les modes de
restauration et d’audit de rotation nécessitent encore le volume de compilation
et refusent cette combinaison. Les manifestes d’intégrité ne constituent pas
une configuration de production : le lanceur génère les fixtures synthétiques
64 clés juste avant démarrage et les répartit 22/21/21. Les trois nœuds
représentent deux hôtes physiques. Tout résultat reste borné au profil activé.
Les exemples et résultats historiques ci-dessous restent conservés par révision.
La première campagne des images finales passe : finalité commune 2→5 en
142,963 s, 24 observations saines et trois signataires terminés sains.
[Preuves bornées](../../docs/qualification/packaged-network-20260930/manifest.json).

Cette campagne utilise trois processus de nœud sur deux machines physiques.
Elle n’est pas une qualification à trois domaines de panne indépendants.
Les images ARM64 et x86-64 sont construites depuis le commit Git `b7e2748` ;
le moniteur et la préparation des fixtures sont versionnés séparément.

`prepare.mjs` transforme une sortie fraîche de `--export-kurtosis` en une
partition disjointe des 64 clés **publiques synthétiques**, réparties entre les
instances déclarées dans le tableau JSON de topologie. Chaque entrée déclare
`name`, `host` (domaine de panne), `mode` (`native` ou `docker`), `rpcPort` et
`p2pPort`. Une entrée native fournit aussi `p2pBinary` et `runtimeDirectory`
absolus. L’outil refuse un dossier de sortie existant et vérifie chaîne 1337,
identité des clés synthétiques, unicité des indices et partition non vide.

```sh
node tests/multihost/prepare.mjs exported-fixtures fresh-output topology.json
```

La configuration de consensus, le genesis et le snapshot sont copiés sans
modification. Seuls les chemins opérationnels et la répartition des clés changent.
Les bootnodes doivent être renseignés avec le véritable peer ID observé au
lancement. Les RPC restent sur loopback ; les observations Docker passent par
`docker exec` ou un transport privé. Ne jamais financer ces comptes.

Le moniteur `tests/assertoor/stability.mjs` exige maintenant l’accord de toutes
les instances fournies, sans limiter la comparaison aux deux premières. Les
13 tests passent, dont troisième instance en retard et divergence d’exécution.
Ce sont des tests du moniteur, pas un résultat du réseau réel.

Vérification préparatoire réalisée avec le véritable exporteur natif :
64 indices uniques distribués 22/21/21, deux domaines de panne, fichiers
préparés sans lancement réseau. Ces fixtures de vérification doivent être
régénérées juste avant la campagne : le genesis dépend de l’horloge.

## Réutiliser une compilation isolée

En mode `NANO_TEST_BUILD_VOLUME=1`, `NANO_TEST_BUILD_SUBDIRECTORY` sélectionne
un sous-dossier existant de `/build` (par exemple
`production-evm-c10426b2-20261001`). Le binaire Nano, le marqueur de révision
et l'auditeur de rotation sont lus dans ce même dossier ; le relais P2P reste
`/build/network/nano-p2p`. Le volume demeure en lecture seule. Le chemin est
consigné dans la preuve du banc et les empreintes sont collectées avant démarrage.
Un sélecteur de dossier est refusé hors du mode volume ou s'il contient un
chemin, un espace ou un saut de ligne. L'absence du sélecteur conserve l'ancien
chemin `/build/nano`.

Cela ne qualifie pas les binaires : la régression complète, la provenance du
marqueur, la correspondance des sources et du binaire natif doivent être revues
avant le lancement. Ne pas créer de marqueur pour une compilation non qualifiée.
Aucune campagne ni nouvelle image ne sont lancées par cette configuration.

## Critère de campagne

Vérifier le commit et l’architecture des binaires, les connexions réelles,
la progression et trois avancées consécutives du même checkpoint finalisé
sur **les trois instances**, liées aux mêmes blocs d’exécution. Conserver
observations, latences, empreintes et logs, y compris en cas d’échec.
Les changements de connexion et crashs viennent ensuite, sur ces seules
instances isolées. Les résultats bornés sont archivés dans `docs/qualification/multihost-20260926/`.

## Lancement de la campagne isolée

Une fois les deux images construites, depuis la racine du dépôt sur le Mac :

```sh
node tests/multihost/run.mjs ubuntu@145.239.199.166 \
  b7e27489b3dc6ff5870af507615e83ba28f04569 /private/tmp/nano-multihost-fresh-run
```

Le lanceur vérifie les deux images avant de générer le genesis, conserve les
empreintes des binaires natifs, puis distribue uniquement les fixtures
synthétiques par SCP (le code du VPS vient de Git). Il publie le seul port P2P
30307 sur le VPS, sans RPC public. Le test vérifie les connexions puis la
concordance finalité Beacon/exécution avant de lancer la fenêtre de mesure.
À la fin, les processus sont arrêtés et les bases/conteneurs conservés pour
inspection. Ne pas réutiliser un répertoire de campagne existant.

Les conteneurs utilisent `--init`, comme `init: true` dans Compose : le cœur
ne doit pas être PID 1 pour le contrôle de parent du relais. Les deux flux
stdout/stderr sont archivés afin de conserver les erreurs de démarrage.

Diagnostic optionnel : préfixer le lancement par `NANO_TEST_NETWORK_TRACE=1`.
Le même réglage est alors transmis aux deux conteneurs et au processus natif.
Il active les traces existantes du relais et des devoirs, sans modifier les
intervalles, seuils ou règles de consensus. Son coût fait partie de la mesure.

## Partition WAN isolée (scénario en cours de qualification)

Préfixer la commande par `NANO_TEST_WAN_PARTITION=1`. Le lanceur crée un
réseau Docker dédié au seul conteneur VPS de cette campagne. Après finalité
commune, il déconnecte ce conteneur pendant 96 secondes, conserve les RPC sur
loopback via docker exec et contrôle zéro pair et le processus encore actif.
Il reconnecte ensuite la même adresse interne dans un bloc finally, y compris
si une observation échoue ou si la réponse SSH de déconnexion est perdue.
Aucune règle de pare-feu du serveur ni autre conteneur n’est modifié.

Les checkpoints sont comparés pendant la coupure et la reprise (refus de
régression et de racines contradictoires). Délai maximal de reprise : 180 s,
avec pairs reconnectés et un checkpoint commun supérieur à celui de départ.
Suit la fenêtre habituelle de trois avancées consécutives, liées à l’exécution,
puis le contrôle terminal des trois validateurs. Le lanceur conserve ses
empreintes et les binaires dans run.jsonl. Les réseaux arrêtés restent conservés
avec les conteneurs et bases de diagnostic, sans suppression automatique.

Ce scénario isole les 21 clés du VPS des 43 clés Mac ; il ne constitue pas une
partition équilibrée 32/32 ni trois domaines de panne. On ne suppose pas que
l'ensemble du réseau doit cesser de finaliser : cela dépend aussi des liaisons
restantes entre les instances Mac. Ce test n’injecte ni transaction utilisateur,
ni charge continue. Une restauration dans finally couvre les erreurs capturées,
pas une destruction du processus orchestrateur ou la perte totale de SSH.

Les tests unitaires de `partition.test.mjs` contrôlent l'orchestrateur et ses
refus ; ils ne prouvent pas la reprise du réseau réel. Campagne lancée le
26 septembre 2026, résultats à consigner après son arrêt.

Le relais sortant utilise quatre permissions globales par défaut, au maximum
une par pair. `NANO_CONSENSUS_RELAY_CONCURRENCY=1` restaure la sérialisation
pour diagnostic ; plage opérationnelle 1..1024. Ce budget local ne limite pas
le registre des validateurs. Le test standard conserve le défaut.

`node tests/multihost/relay-summary.mjs <dossier>` conserve les totaux par type
et ajoute `peerGroups` lorsque les traces portent `peerId`. Les anciennes
traces sont comptées dans `withoutPeerIdentity`, sans attribution inventée.
Ces statistiques couvrent seulement les tentatives lentes ou échouées ;
elles ne mesurent ni le débit total ni la seule latence réseau.

Après une restauration réussie mais un délai de reprise manqué, le banc
conserve cet échec et poursuit l’observation bornée de la finalité.
`late-finality-passed` qualifie uniquement cette fenêtre ultérieure ; le run
reste en échec et n’émet pas `passed`. Les erreurs de sécurité, de RPC ou de
restauration ne déclenchent pas cette extension.

Après collecte, `node tests/multihost/audit-history.mjs <dossier>` vérifie la
cohérence des checkpoints entre démarrage, partition et observation prolongée.
Les observations manquantes ne valent pas réussite. Ce contrôle hors ligne
complète les critères de vivacité et de liaison à l’exécution sans les remplacer.

L’extension tardive attend d’abord une nouvelle finalité commune, au maximum
180 s supplémentaires, en conservant les contrôles de conflits et régressions
de la partition. Elle démarre ensuite la fenêtre de stabilité habituelle. Le
critère initial de 180 s reste échoué, même si ces phases ultérieures passent.

## Remplacement du conteneur avec pins indépendants

`NANO_TEST_PINNED_RESTART=1` exige des images candidates Capella et un scénario
autonome (sans moniteur, restauration, charge, partition ou rotation). Le VPS
est arrêté sainement ; son reçu est conservé indépendamment et le conteneur
remplacé sur le même volume avec tous les pins. Aucune ancienne sauvegarde
n’est remise en service. Même peer ID, nouvelles duties et trois avancées
communes sont exigés. Les engagements finaux doivent avancer sans régresser
pour aucun signataire. Cela qualifie le remplacement à version identique,
pas une migration de schéma ni un retour entre versions différentes.
[Campagne qualifiée](../../docs/qualification/pinned-replacement-20260930/manifest.json).

## Programme fini de partitions répétées

`NANO_TEST_PARTITION_SCHEDULE_MS=96000,192000`, avec
`NANO_TEST_WAN_PARTITION=1`, exige un scénario autonome Capella sur images
candidates (pas de charge, rotation, moniteur, restauration ou remplacement).
L’historique de sécurité est partagé entre cycles ; chaque reprise doit être
connectée et finaliser plus haut que son point de départ. Le premier échec
arrête les coupures suivantes. La fenêtre stricte de trois avancées est
toujours exigée après le programme, sans effacer une échéance ratée.
Les ressources sont relevées toutes les 20 secondes, et une erreur de collecte
reste fatale. Le programme est borné à 2–4 coupures de 1–300 secondes, avec
600 secondes de coupure cumulée maximum : budgets de test uniquement.
La campagne 96/192 secondes est en cours au moment de cette documentation ;
aucune réussite réseau n’est encore revendiquée.

## Diagnostic de reprise après partition

`NANO_TEST_FAULT_PROGRESS=1` ajoute à chaque observation de partition Capella
le head/slot, l’état immuable sélectionné, ses checkpoints justifié/finalisé
et les bornes temporelles des deux lectures. Les contrôles de sécurité utilisent
toujours les observations de finalité du lanceur ; les diagnostics ne changent
ni le délai de 180 s ni le verdict. Une lecture manquante, un état élagué ou
un format invalide échoue explicitement. Quatre tests ciblés couvrent le head
qui change, les réponses invalides et les erreurs de lecture/horloge.
La tentative A sans ces relevés a dépassé son second délai ; la cause reste
ouverte. Ce mode sert à distinguer rattrapage du head et attente de finalité.

### Connected majority island (public Capella fixture only)

Set `NANO_TEST_MAJORITY_ISLAND=1` with `NANO_TEST_WAN_PARTITION=1`,
`NANO_TEST_CONSENSUS_PROFILE=capella` and the qualified packaged candidate manifest.
This standalone finite campaign keeps Mac native and Mac Docker directly linked
through loopback port 30306 while disconnecting only the dedicated VPS test
network for 192 seconds. No runtime build is necessary for this harness change.

The gate decodes effective balances of the 64 active, unslashed validators from
hash-bound initial Capella SSZ and checks complete, non-overlapping ownership.
It requires the Mac island to hold strictly more than two thirds of total stake.
Both Mac peer IDs must remain reciprocally connected. Minority finality must
not exceed a conservative pre-cut commitment ceiling, frozen at the first
zero-peer observation; already received votes are allowed. Majority finality
must advance beyond that ceiling while the cut is effective. Reconnection
restores the original IP even on failure and must produce common finality newer
than the cut's highest observed majority epoch within the unchanged 180-second
budget. The standard stability, healthy signer and clock gates follow.
This fixed-fixture decoder does not qualify arbitrary fork schemas, validator
scales, asynchronous cross-chain bridges or production readiness.

`NANO_TEST_MAJORITY_CUT_MS=384000` selects a distinct sustained eight-epoch
cut. The default remains 192000 ms, allowed finite range 192000–600000 ms.
A longer-cut result cannot close an earlier failed shorter-cut criterion;
the reconnection deadline remains 180000 ms.
