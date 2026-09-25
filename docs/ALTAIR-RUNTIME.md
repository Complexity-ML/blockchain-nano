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
à un stockage bloqué. La propagation des messages entre daemons et les essais
distribués restent ouverts ; les reprises IPC qualifiées sont décrites ci-dessous. Ne pas utiliser ce
runtime comme preuve de finalité de fonds EVM.

## Tickets de traitement et réponses perdues

Le daemon utilise le protocole privé suivant, toutes protégées par
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

Le ledger et son trajet HTTP sont qualifiés. Le daemon utilise ces appels pour
récupérer les tickets au démarrage ; la propagation P2P reste à intégrer.

## Client IPC destiné au daemon

`AltairCoreDispatch` utilise `Core::call` pour Submit/Job/Cancel/Forget. Sa file
locale est bornée, ses numéros sont croissants et ses échanges sont sérialisés.
Une réponse Submit perdue conduit à rechercher le même ticket. Si le cœur indique
qu'il est encore inconnu, seul ce numéro/payload peut être resoumis. Une réponse
Forget perdue est récupérée par répétition du retrait ; `retired` confirme alors
le retrait du résultat déjà observé.

La callback de fin n'est délivrée qu'après confirmation de fin et nettoyage du
ticket, ou confirmation que le numéro ne peut plus être admis. Les requêtes en
attente locale annulées avant envoi sont terminées sans contacter le cœur. En
cas d'épuisement des tentatives de transport ou de réponse incohérente, le client
bloque les nouvelles admissions et conserve les callbacks/capacités réservées :
il ne prétend pas qu'un worker éventuellement actif s'est terminé. L'appelant doit
reconcilier les tickets avant de reconstruire ce client, et fournir la session et
le highWater observés. Il doit aussi fournir les échéances/annulations des requêtes.

Le client est qualifié avec réponses contrôlées et avec le vrai `Core::call` sur
HTTP loopback. Il est instancié dans `network/main.cpp` après récupération réussie
des tickets. Le handler libp2p et la reprise après blocage du client restent à
raccorder. Les tests ne qualifient pas le débit d'un réseau distribué ; un seul
aller-retour IPC est actif à la fois.

## Identité de consensus exposée au daemon

Le runtime fournit `consensusIdentity` dans le statut RPC privé : SHA-256 du JSON
compact canonique (clés ordonnées, quantités hexadécimales) de format
`nano-altair-consensus-identity-v1`. Il engage le chain ID, le genesis d'exécution,
le profil de transition complet, les quatre paramètres de choix de branche et
les quatre engagements d'ancre. Le fork digest seul ne distingue pas ces règles.
Les chemins, la présentation du fichier de profil, les budgets locaux, la session
IPC et la séquence de checkpoint ne modifient pas cette identité. L'identité
reste conservatrice : deux ancres différentes exigent une réconciliation explicite,
même si elles pourraient appartenir à la même chaîne.

`wire` expose aussi les quatre limites d'enveloppe/payload effectivement utilisées
par le cœur. Un service de test construit sans identité annonce `null`, jamais
une identité déduite du seul fork digest. Ce statut est préparatoire : le daemon
ne le négocie pas encore avec les pairs. Il ne prouve ni leur honnêteté, ni la
finalité de l'ancre, ni la liaison atomique avec l'exécution EVM.

## Clôture d'une ancienne session d'admission

`nano_altairRotateSession(token, expectedSession)` remplace atomiquement la
session des tickets et renvoie `session` et `highWater`. La validation de session
et l'admission Submit partagent le même mutex : une requête retardée de l'ancienne
session ne peut plus démarrer, quel que soit son numéro. Les tickets existants
et leur watermark restent conservés ; leurs annulations sont signalées.
Les anciennes requêtes Job/Cancel/Forget sont également refusées.

La rotation ne prouve pas la fin des workers. Sous la nouvelle session, le daemon
doit encore consulter les tickets, attendre `completed`, récupérer la décision,
puis retirer chaque résultat. Une acceptation déjà commitée reste une acceptation.
Une seconde rotation avec l'ancienne session attendue échoue sans mutation. Si sa
réponse a été perdue, relire le statut avant toute décision ; ne jamais déduire
que les anciens travaux ont été rejetés, ni les rejouer automatiquement.

Cette primitive ne coordonne pas plusieurs daemons autorisés concurrents. Elle
porte sur le chemin à tickets ; l'ancien appel synchrone `nano_altairReceive`
n'est pas couvert par cette clôture. Avant activation réseau, le superviseur doit
assurer un propriétaire unique et l'admission réseau doit utiliser exclusivement
les tickets. La récupération au démarrage est décrite ci-dessous ; le handler P2P
reste à raccorder.

## Récupération réellement exécutée au démarrage du daemon

Lorsque le cœur annonce `altairIpc: true`, `network/main.cpp` démarre une seule
récupération authentifiée. `AltairCoreRecovery` vérifie l'identité de chaîne,
l'empreinte du consensus, les limites wire et la capacité de tickets du cœur.
Il confirme une rotation, consulte les anciens tickets, demande l'annulation des
actifs et retire les terminés. Il ne produit un résultat prêt qu'après inventaire
vide et nouvelle vérification de l'identité et de la session.

Une réponse de rotation perdue entraîne une nouvelle lecture du statut puis une
nouvelle rotation confirmée. Une réponse Forget perdue est récupérée en répétant
le retrait. Les réponses incohérentes, changements de profil/session, compteurs
qui reculent ou sont épuisés bloquent la récupération. Une échéance totale borne
l'attente même si le transport ne rappelle jamais sa callback ou si un worker
reste actif. Le daemon utilise actuellement trois erreurs de transport maximum,
100 ms entre échanges et 30 secondes au total : ce sont des budgets IPC locaux,
pas des limites de validateurs. Leur configuration opérateur reste à exposer.

Le journal `altair-ipc-ready` indique que le client IPC a été construit après cette
récupération. Il conserve `networkProtocolActive: false` : aucun handler Altair
libp2p, signature de validateur, propagation ou liaison EVM n'est activé ici.
`altair-ipc-recovery-failed` ne signifie jamais que les anciens workers sont finis.
Il n'y a pas de boucle de reprise automatique après cet échec ; le diagnostic et
la politique de supervision restent à compléter avant activation réseau.
