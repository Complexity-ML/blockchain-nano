# Supervision locale de la finalité

`scripts/operations/finality-monitor.mjs` effectue une seule observation en
lecture seule. Il ne signe, ne redémarre et ne modifie aucun nœud ; ses seuls
fichiers sont l'historique local de supervision et son verrou. Le moteur Nano
reste natif C++ ; cet outil d'exploitation séparé utilise Node.js.

Le fichier de configuration doit fournir les valeurs **revues indépendamment**
issues du genesis et du profil déployés. Ne pas apprendre automatiquement
l'identité attendue depuis le serveur surveillé. Exemple à compléter :

```json
{
  "chainId": "1337",
  "genesisHash": "REMPLACER_PAR_LE_HASH_EXECUTION_GENESIS_REVU",
  "configName": "REMPLACER_PAR_CONFIG_NAME_DU_PROFIL_REVU",
  "staleMs": 120000,
  "timeoutMs": 5000,
  "maximumResponseBytes": 1048576,
  "maximumEpochs": 256,
  "minimumPeers": 1,
  "nodes": [
    {"name": "local", "url": "http://127.0.0.1:8545/"},
    {"name": "remote-tunnel", "url": "http://127.0.0.1:9545/"}
  ]
}
```

Ces seuils illustratifs doivent être ajustés au profil et au déploiement réel.
`maximumEpochs` borne les racines retenues si un observateur tarde à avancer ;
prévoir au moins deux entrées et la marge de retard autorisée. Ce budget de
supervision ne limite pas le registre des validateurs. Un seul observateur est
possible, mais ne vérifie aucune concordance entre nœuds. Les URLs restent en loopback. Par défaut `kind: "http"` interroge directement
l'endpoint local. Avec `kind: "docker"` et `container: "nano-node"`, le lecteur
utilise `docker exec` et curl. Avec `kind: "ssh-docker"`, ajouter par exemple
`ssh: "ubuntu@145.239.199.166"` : il utilise SSH BatchMode puis sudo Docker exec.
Deux conteneurs distincts peuvent ainsi utiliser le même port local sans être
confondus ; aucun port RPC n'est publié. L'opérateur doit disposer des droits
Docker/sudo correspondants. Un tunnel privé reste possible avec le mode HTTP. Aucune exposition publique du RPC.

Depuis la racine du checkout, avec un répertoire d'état privé appartenant à
l'opérateur :

```sh
node scripts/operations/finality-monitor.mjs /etc/nano/monitor.json /var/lib/nano-monitor/state.json
```

Codes : `0` observation sans alerte, `2` alerte, `1` erreur d'exécution ou d'état.
La sortie JSON peut être collectée par journald ou un système local d'alertes.
Planifier des observations à un intervalle inférieur au seuil `staleMs` avec
le superviseur de l'opérateur ; aucun minuteur n'est installé automatiquement.
Deux exécutions simultanées sont refusées. Un SIGKILL peut laisser le verrou :
vérifier que l'ancien processus est terminé avant de le retirer. Ne pas effacer
le fichier d'état pour rendre une alerte verte.

L'outil vérifie chaîne/genesis, `CONFIG_NAME`, nombre de pairs, racine Beacon
et checkpoint finalisé lu par **racine d'état immuable**. Une erreur RPC, un
profil différent, un budget de réponse dépassé, des pairs insuffisants ou une
finalité inchangée au-delà du seuil produisent une alerte. Les conflits de
racines à la même époque, régressions et saturation de l'historique restent
verrouillés dans l'état même si les observations suivantes semblent normales.
Une réinitialisation exige une analyse et une décision explicite de l'opérateur.
Une horloge locale qui recule ou un état incompatible provoque une erreur.

La première observation établit une référence : un résultat vert ne prouve
pas encore une avancée de finalité. Une observation manquée ne reçoit aucun
crédit de progression. Le moniteur ne remplace pas la fenêtre stricte de trois
époques du banc de qualification et ne vérifie pas le rattachement de chaque
checkpoint au bloc d'exécution. Il ne surveille pas encore disque/RocksDB,
ressources, horloge du validateur, duties ni la remise externe des alertes.
L'historique persiste entre exécutions ordinaires par remplacement de fichier,
sans qualification de perte d'alimentation ou de falsification locale.

## Qualification actuelle

Vingt et un tests locaux passent, dont cinq exécutions réelles du CLI face à deux
serveurs HTTP synthétiques : panne/stagnation, reprise, conflit et conservation
de l'alerte. Les tests vérifient aussi identité, lecture par état immuable,
budget de réponses et d'historique, régression, verrou et horloge. Cela qualifie
l'outil et son lecteur. La qualification sur réseau vivant est désormais
conservée ci-dessous ; la livraison effective des alertes reste ouverte.


Le banc multihôte accepte `NANO_TEST_FINALITY_MONITOR=1` avec Capella et les
volumes de compilation existants, sans autre injection de panne. Il attend la
finalité de démarrage, exécute le vrai CLI avec une identité genesis incorrecte
(dont le refus est obligatoire), puis observe la configuration correcte en
parallèle de la fenêtre stricte de trois époques. Genesis et profil proviennent
de la fixture locale exportée avant démarrage. Ce scénario passe sur les trois environnements : 25 observations et trois
avancées communes des époques 2 à 5, avec refus du mauvais genesis.
[Preuves et limites](qualification/finality-monitor-live-20260930/manifest.json).
La disponibilité sous panne et l'exploitation complète restent à qualifier.

## File persistante de livraison — 1 octobre 2026

`alert-delivery.mjs` est un outil séparé, activé seulement avec un récepteur
explicitement configuré. Le monitor lui-même continue de faire des lectures
seules. Aucun récepteur public n'a été contacté pendant la qualification.

Les nouveaux rapports contiennent `identity`, dérivée de la configuration
canonique du monitor. Calculer le champ attendu depuis la configuration locale
revue, via `monitorIdentity(configuration)` exportée par `finality-monitor.mjs` ;
ne pas apprendre cette identité depuis un nœud distant. Configuration de livraison :

```json
{
  "monitorIdentity": "REMPLACER_PAR_64_CHIFFRES_HEXA_DE_L_IDENTITE_REVUE",
  "url": "https://RECEPTEUR_EXPLICITEMENT_CONFIGURE/alerts",
  "timeoutMs": 5000,
  "maximumResponseBytes": 4096,
  "maximumPending": 64
}
```

HTTPS est requis, sauf HTTP vers 127.0.0.1 pour les tests. Pas de redirection,
ni identifiant/mot de passe dans l'URL. Ces valeurs sont des paramètres
d'exploitation à revoir, pas des limites du registre des validateurs.

Avec `umask 077`, conserver la sortie JSON du monitor dans un fichier privé
et ne l'envoyer au consommateur que si le monitor a terminé avec code 0 ou 2.
Un code 1 représente une erreur d'outil/état, pas un rapport valide. Publier
le fichier par renommage pour éviter une lecture concurrente partielle.

```sh
node scripts/operations/alert-delivery.mjs delivery.json report.json queue.json
node scripts/operations/alert-delivery.mjs delivery.json --drain queue.json
```

L'état de supervision et la file utilisent écriture temporaire exclusive,
fsync du fichier, renommage atomique et fsync du répertoire. Une alerte est
persistée avant le POST, retirée après une réponse HTTP 2xx et un commit durable.
Un échec conserve le message. Le code 2 indique une livraison échouée, le code 1
une erreur locale/configuration, le code 0 une file livrée ou silencieuse.
Le drain reprend les messages existants sans créer d'observation fictive ;
il permet de libérer une file pleine. Une file saturée refuse le nouvel événement
sans supprimer les anciens : surveiller également ces erreurs locales.

Le corps et l'en-tête `Idempotency-Key` portent le même `eventId`. Une interruption
après acceptation et avant commit local peut répéter le POST avec ce même identifiant.
Le récepteur doit dédupliquer durablement cet identifiant ; il ne s'agit pas d'une
garantie universelle d'exécution exactement une fois. Un 2xx est l'accusé de réception
convenu, pas une preuve que l'opérateur a lu la notification. La reprise garde l'ordre
des événements. Une panne persistante n'est pas effacée ; le retour à une santé sans
faute produit un événement distinct. Le progrès normal et les observations identiques
ne génèrent pas de spam. Les fautes de sécurité verrouillées restent verrouillées.

Conserver file et configuration privées et leur sauvegarde cohérente. Un SIGKILL
peut laisser le verrou : confirmer l'absence de l'ancien processus avant retrait,
sans effacer la file ni l'état de sécurité. Changement d'identité/configuration ou
corruption : refus, migration explicite après analyse. Ce mécanisme ne provisionne
ni service d'alertes, ni authentification/mTLS du récepteur, ni minuteur d'exploitation.
La perte physique d'alimentation et le parcours depuis un daemon Nano en panne
jusqu'à un récepteur d'exploitation réel restent à qualifier.

Qualification de la livraison : 31 tests passent sur Mac natif, Docker ARM64
et VPS Docker AMD64, dont 10 nouveaux cas de file/livraison et 21 du monitor.
Voir les [preuves AMD64](qualification/alert-delivery-20261001/amd64/README.md).
Les récepteurs sont locaux et les rapports de livraison synthétiques ; cela
ne ferme pas le raccordement à un daemon vivant et au destinataire opérateur.

## Raccordement durable du monitor à la file

Pour l'exploitation avec livraison, utiliser le cycle intégré :

```sh
node scripts/operations/monitor-and-deliver.mjs monitor.json delivery.json state.json queue.json
```

Il effectue une seule observation. L'identité du destinataire doit correspondre
à la configuration revue du monitor ; état et file restent deux fichiers privés
distincts. Les alias symlink et collisions entre file/état/verrous sont refusés.
Le rapport à livrer est enregistré comme `pendingDelivery` dans le même commit
durable que l'état du monitor, avant la remise à la file. Au prochain cycle,
ce rapport est remis avant toute nouvelle observation ; il est retiré seulement
après la remise durable. Une panne HTTP laisse le message dans la file.
Une erreur locale de remise conserve le rapport dans l'outbox et bloque son
remplacement. Une file pleine demande un drain explicite avant reprise.
Le CLI de monitor seul refuse un état avec remise en attente.

Code 0 : observation saine et remise sans échec HTTP ; code 2 : observation
en alerte ou remise HTTP échouée ; code 1 : erreur locale/configuration.
Après SIGKILL, vérifier le processus terminé avant retrait du verrou ; ne pas
effacer l'outbox, la file ni les fautes de sécurité. Aucun service ou minuteur
n'est installé. Cette méthode ferme la fenêtre de perte entre observation et
remise ; une observation jamais effectuée ou non encore commitée n'est pas
couverte. Le récepteur doit toujours dédupliquer durablement l'eventId.

Quatre nouveaux tests de raccordement, dont un vrai SIGKILL du superviseur,
passent en natif et ARM64 (35 tests totaux). Les endpoints RPC sont
synthétiques ; la campagne depuis de vrais daemons et AMD64 restent ouverts.
[Preuves](qualification/monitor-handoff-20261001/native/README.md).
