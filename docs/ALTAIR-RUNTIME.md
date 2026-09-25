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
| `clock` | `enabled` (booléen), `intervalMs`, `maximumForwardStepMs` |

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
initial, pas un export continu des engagements après chaque commit. La méthode
privée `nano_altairStatus(token)` permet de lire le token actuellement publié,
l’heure du Store et `clockHealthy` (`null` si désactivée). Elle exige le même jeton
que la réception et passe par la file RPC bornée. La conservation externe durable
des tokens reste à intégrer. Sans engagement
conservé indépendamment, un rollback complet du disque n'est pas détectable.

Le service, son worker, le réacteur et la base partagent un propriétaire : les
handles RPC gardent ces ressources vivantes, et le worker est drainé avant la
destruction du stockage.

## Horloge locale

Avec `clock.enabled: false`, l'heure reste au checkpoint importé/restauré (mode
utile aux replays déterministes). Avec `true`, le runtime lit l'heure système UTC
en millisecondes, effectue un premier tick puis lance un thread périodique. Il
actualise aussi l'heure juste avant chaque message traité par le worker. Un même
instant n'est pas commité deux fois. L'heure distante ne peut pas être injectée
par RPC ou par l'enveloppe d'un pair.

Un recul par rapport à l'heure persistée, un écart supérieur à
`maximumForwardStepMs` ou une erreur locale bloque l'horloge jusqu'au redémarrage.
Le worker refuse alors les traitements Altair avec une erreur locale ; le RPC de
statut expose `clockHealthy: false` tant que le réacteur reste lisible. Une simple
annulation de requête n'invalide pas l'horloge. Il faut résoudre l'horloge/le
checkpoint avant de reprendre : augmenter aveuglément la borne ne constitue pas
une vérification de fraîcheur de l'ancre.

Chaque tick effectif conserve encore un snapshot complet du Store. La cadence
`intervalMs` et les performances sous charge ne sont pas qualifiées pour la
production ; l'absence de backlog de timers n'élimine pas cette amplification
d'écriture. L'arrêt attend la fin du travail courant, sans garantie de délai face
à un stockage bloqué. Les messages du daemon, la récupération après perte
d'acquittement et les essais distribués restent ouverts. Ne pas utiliser ce
runtime comme preuve de finalité de fonds EVM.

## Tickets de traitement et réponses perdues

Le daemon pourra utiliser les méthodes privées suivantes, toutes protégées par
le même jeton. `session` provient de `nano_altairStatus` (`admissionSession`) ou
`nano_altairJobs`, et `sequence` est une quantité positive canonique.

| Méthode | Paramètres après le token | Effet |
| --- | --- | --- |
| `nano_altairSubmit` | session, sequence, enveloppe encodée | Admet un travail ou retrouve son ticket existant |
| `nano_altairJob` | session, sequence | Lit le résultat sans relancer le travail |
| `nano_altairCancel` | session, sequence | Signale l'annulation ou interdit une admission encore retardée |
| `nano_altairForget` | session, sequence | Retire uniquement un résultat terminé |
| `nano_altairJobs` | aucun | Liste bornée des tickets et numéro le plus élevé réservé |

Les admissions utilisent des numéros croissants. Réutiliser un numéro existant
avec le même payload retrouve son résultat ; un contenu différent est refusé.
L'appelant doit ordonner ses admissions : un numéro ancien absent de la table
ne peut pas démarrer après un numéro plus élevé. Les états sont `pending`,
`completed`, `limited`, `unknown` ou `retired`. Un ticket terminé distingue
`accepted`, `rejected` et `local_failure`. `retired` signifie que ce numéro ne
peut plus être admis, **pas** que son effet historique est connu.

La table est bornée par `worker.jobs`. Les tickets terminés continuent à compter
jusqu'à leur retrait explicite ; les tickets actifs ne sont jamais évincés. Un
retrait perdu peut être réitéré. Un Cancel sur un numéro encore inconnu empêche
un Submit HTTP retardé de démarrer ce numéro après l'annulation. Pour un travail
déjà actif, Cancel n'est pas une preuve de fin : il faut attendre `completed`.
Une annulation après commit ne réécrit pas une acceptation en rejet.

La connexion Submit peut disparaître sans annuler le travail admis. Rechercher
ou resoumettre **le même ticket**, au lieu d'inventer un nouveau numéro, permet
de récupérer la décision après perte de réponse. Les tickets sont locaux au
processus et ne sont pas persistés. Après redémarrage, la session change ; les
anciens tickets sont refusés et leur résultat doit être rapproché de l'état
consensus durable. Ne pas interpréter un changement de session comme un rejet,
et ne pas rejouer aveuglément les anciens messages sous de nouveaux numéros.

Le ledger et son trajet HTTP sont qualifiés, mais le daemon n'utilise pas encore
ces appels. La politique de récupération après changement de session et la
propagation P2P restent à intégrer.
