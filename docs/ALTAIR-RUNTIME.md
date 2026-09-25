# Runtime Altair expérimental

Le champ optionnel `altair` de la configuration de `nano run` initialise un
service de réception Altair durable. Il n'active pas le consensus Altair dans le
réseau P2P et ne lie pas ses décisions aux blocs EVM. Le protocole P2P existant
reste celui de la chaîne d'exécution. Aucun validateur Altair ne signe ici.

Le service exige `rpc_host: "127.0.0.1"`, l'absence de configuration de signataire
`bft`, et un répertoire RocksDB distinct de la base EVM. Sans champ `altair`, le
comportement existant est conservé. Aucun fichier de déploiement ne l'active par
défaut. La méthode `nano_altairReceive` reste protégée par le jeton éphémère du
daemon ; ce jeton n'est pas écrit dans le journal de démarrage.

## Configuration exacte

Chaque objet ci-dessous refuse les champs supplémentaires ou manquants. Les
quantités numériques sont des chaînes hexadécimales canoniques (`"0x1"`, jamais
`1` ou `"0x01"`). Les tailles et budgets sont positifs ; `attestationDueMs` et
`proposerScoreBoost` peuvent être nuls. Les chemins sont résolus relativement au
répertoire de travail du processus ; des chemins absolus évitent l'ambiguïté.

| Champ de `altair` | Sous-champs requis |
| --- | --- |
| `profile` | `file`, `sha256`, `maximumBytes` |
| `forkChoice` | `slotDurationMs`, `attestationDueMs`, `proposerScoreBoost`, `boostProfile` |
| `anchor` | `stateFile`, `blockFile`, `blockRoot`, `stateRoot`, `genesisValidatorsRoot`, `forkVersion` |
| `database` | Chemin du répertoire RocksDB du consensus |
| `limits` | `envelopeBytes`, `blocks`, `messages`, `equivocations`, `stateBytes`, `totalStateBytes`, `replaySlots`, `syncTrials` |
| `wire` | `frameBytes`, `blockBytes`, `attestationBytes`, `slashingBytes` |
| `worker` | `jobs`, `payloadBytes`, `timeoutMs` |
| `restart` | `null`, ou objet `sequence`, `digest` |

`profile.file` contient le profil complet décrit par
`altair_transition_profile`, sans choix de preset implicite. Son digest doit
être authentifié séparément. `boostProfile` vaut exactement
`release-1.6.0-beta.0` ou `shuffling-dependent` ; le premier correspond au corpus
historique de qualification, pas à une recommandation pour un réseau public.

Les racines et digests font 32 octets en hexadécimal canonique, la version de fork
4 octets. L'identité d'exécution (chain ID et hash du genesis EVM) vient de la
chaîne ouverte par `nano run`. Le digest de fork est dérivé de la version et du
genesis_validators_root de l'ancre ; il n'est pas accepté librement du réseau.

Les limites de transport doivent permettre le bloc maximal du profil et les
messages SSZ du comité configuré. `frameBytes` doit couvrir l'enveloppe hexadécimale
complète et `payloadBytes` le plus grand payload autorisé. Ces limites sont des
budgets locaux, pas un plafond de validateurs dans les règles du consensus.

## Initialisation et reprise

- Avec `restart: null`, les fichiers de l'ancre sont vérifiés avant création du
  premier checkpoint. Un checkpoint déjà présent provoque un refus explicite.
- Avec un token `restart`, le répertoire doit déjà exister. La séquence et le
  digest attendus doivent correspondre exactement au stockage. Le profil et
  l'ancre configurés sont aussi comparés au Store restauré. Les fichiers SSZ
  initiaux ne sont pas relus dans ce chemin.
- Un token absent, périmé, un profil ou une ancre incompatibles ne déclenchent
  jamais une réinitialisation automatique. Ne supprimer ni la base ni le token
  pour contourner ce refus : résoudre l'origine du désaccord.

`altair-runtime-ready` indique la séquence et le digest au démarrage ainsi que
`executionBound: false` et `networkProtocolActive: false`. Ce journal est un état
initial, pas un export continu des engagements après chaque commit. La conservation
externe des tokens et leur export opérationnel restent à intégrer. Sans engagement
conservé indépendamment, un rollback complet du disque n'est pas détectable.

Le service, son worker, le réacteur et la base partagent un propriétaire : les
handles RPC gardent ces ressources vivantes, et le worker est drainé avant la
destruction du stockage. L'horloge reste au checkpoint importé/restauré : le
pilotage local des ticks, les messages du daemon, la récupération après perte
d'acquittement et les essais distribués sont les prochaines étapes. Ne pas
utiliser ce runtime comme preuve de finalité de fonds EVM.
