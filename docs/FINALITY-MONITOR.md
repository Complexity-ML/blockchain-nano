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
possible, mais ne vérifie aucune concordance entre nœuds. Les URLs distinctes
restent en loopback : pour un hôte distant, préparer un tunnel SSH privé ou
exécuter l'outil localement sur cet hôte. Aucune exposition publique du RPC.

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

Dix-sept tests locaux passent, dont cinq exécutions réelles du CLI face à deux
serveurs HTTP synthétiques : panne/stagnation, reprise, conflit et conservation
de l'alerte. Les tests vérifient aussi identité, lecture par état immuable,
budget de réponses et d'historique, régression, verrou et horloge. Cela qualifie
l'outil et son lecteur, **pas** la supervision d'un réseau Nano vivant. Cette
intégration et la livraison effective des alertes restent ouvertes.
