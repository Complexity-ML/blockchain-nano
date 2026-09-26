# Qualification Mac natif + Docker Mac + Docker VPS

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
