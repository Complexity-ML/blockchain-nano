# Runtime Altair expérimental

Le champ optionnel `altair` de la configuration de `nano run` initialise un
service de réception Altair durable. L'admission P2P Altair reste désactivée par
défaut ; `altair.networkAdmission: true` l'autorise explicitement après récupération
IPC. Elle ne lie pas les décisions Altair aux blocs EVM et ne démarre aucun
signataire ni propagation automatique de messages Altair.

Le service exige `rpc_host: "127.0.0.1"`, l'absence de configuration de signataire
`bft`, et un répertoire RocksDB distinct de la base EVM. Sans champ `altair`, le
comportement existant est conservé. Aucun fichier de déploiement ne l'active par
défaut. Les méthodes IPC sont protégées par le jeton éphémère du daemon ; ce jeton
n'est pas écrit dans le journal de démarrage. `nano_altairReceive` est interdit
lorsque l'admission réseau est autorisée : seuls les tickets sont alors utilisables.

## Configuration exacte

Chaque objet ci-dessous refuse les champs inconnus ou requis manquants.
`networkAdmission` est le seul champ optionnel interne présenté ici (défaut `false`). Les
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
| `networkAdmission` | Booléen optionnel ; exige `clock.enabled: true` |

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
des tickets. Le handler libp2p est enregistré uniquement en mode réseau explicite ;
la reprise après blocage du client reste à raccorder. Les tests ne qualifient pas le débit d'un réseau distribué ; un seul
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
une identité déduite du seul fork digest. En mode réseau, le daemon compare cette
identité à celle annoncée par le pair. Elle ne prouve ni son honnêteté, ni la
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
n'est pas couvert par cette clôture et est interdit en mode réseau. Le superviseur
doit assurer un propriétaire unique. La récupération au démarrage est décrite
ci-dessous ; le handler P2P utilise exclusivement les tickets.

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
récupération. `networkProtocolActive` y indique l'enregistrement du handler
d'admission, pas une finalité distribuée. Le statut RPC du cœur renvoie `null`
pour ce champ lorsque l'admission est autorisée, car seul le daemon connaît son
enregistrement effectif. Aucune signature, propagation automatique ou liaison EVM
n'est activée ici.
`altair-ipc-recovery-failed` ne signifie jamais que les anciens workers sont finis.
Il n'y a pas de boucle de reprise automatique après cet échec ; le diagnostic et
la politique de supervision restent à compléter avant activation réseau.

## Admission libp2p explicite et qualification

Après récupération, le daemon enregistre `/nano/altair/1.0.0` si le cœur autorise
l'admission réseau et annonce une horloge saine. Le champ `altair` du handshake
contient exactement `protocol`, `consensusIdentity` et `forkDigest`. Un pair doit
être connu par son identité libp2p authentifiée, appartenir à la même chaîne et
annoncer le même profil. L'absence ou la différence de profil interdit seulement
son accès au handler Altair, sans prétendre que cette annonce prouve sa loyauté.
Le cœur vérifie toujours SSZ, signatures et transitions.

Le handler partage le budget du daemon et conserve sa réservation jusqu'à la fin
du worker et du ticket. Une expiration ou une panne locale ferme le stream sans
acquittement de consensus. Un dispatcher bloqué retire l'annonce Altair lors de
la mise à jour du statut et refuse immédiatement les nouvelles admissions.

Sur le trajet privé `nano_altairSubmit`, le quatrième paramètre peut être un objet
`chainId`, `genesisHash`, `forkDigest`, `kind`, `sszBase64`. Le daemon utilise ce
format compact à base64 canonique ; l'ancien paramètre chaîne/enveloppe hex reste
lisible pour les tests et appelants privés. Les deux encodages d'un même payload
produisent le même digest de ticket. Le format réseau reste exclusivement hex :
`sszBase64` n'y est pas accepté. Les alphabets, padding, bits inutilisés et bornes
sont vérifiés avant allocation du payload décodé.

Le runtime refuse avant ouverture du Store un profil dont le plus grand payload
ne tient pas dans une requête IPC compacte de 16 Mio. Le profil de qualification
à 8 Mio tient sans réduction de taille et sans augmenter la limite HTTP. Les
profils dépassant cette capacité nécessitent encore un transport fragmenté ; leur
activation n'est pas silencieusement tronquée.

`consensus_altair_libp2p` lance un vrai daemon et des pairs Noise/libp2p distincts
contre le cœur HTTP durable. Il vérifie les refus de pairs/profils/enveloppes,
transporte 8 Mio de SSZ volontairement invalide jusqu'au rejet par le cœur et
importe deux blocs officiels avec contrôle du checkpoint et du head attendu.
L'horloge suit les fixtures : ce n'est ni un réseau de validateurs contemporains,
ni une preuve de finalité distribuée, ni un test de charge soutenue. Le binaire
`nano-altair-probe` sert uniquement à cette qualification et n'est pas installé.

## Blocs signés conservés avec le checkpoint

Le réacteur conserve maintenant les octets SSZ originaux des nouveaux blocs
validés. L'état et cette archive sont écrits dans le même checkpoint RocksDB,
avant publication de leurs références immuables. Les votes et ticks conservent
ces références ; un doublon de racine ne remplace jamais la signature archivée.
Cette précaution compte car le choix de branche peut accepter une racine déjà
connue sans revérifier la signature du doublon.

Un checkpoint sans archive conserve exactement le format `nano-altair-store-v1`.
Dès qu'un nouveau bloc est archivé, le format `nano-altair-durable-v2` enveloppe le
Store et `signedBlocks`. La restauration vérifie d'abord le digest externe, puis
les limites, le corps SSZ, l'en-tête, la racine et la signature BLS de chaque bloc
par rapport à son état authentifié. Ce chemin reste un chargement de checkpoint
local de confiance, pas une admission de snapshots provenant de pairs.

Les anciens checkpoints restent lisibles avec une archive vide. L'ancre est un
bloc non signé de confiance et n'est pas exportée comme un bloc signé. Les octets
absents d'un ancien checkpoint ne sont pas reconstitués par le raccourci des
racines déjà connues ; une réhydratation historique authentifiée reste à prévoir.
Après écriture du format v2, un ancien binaire ne comprenant que v1 ne peut pas
reprendre ce checkpoint.

Deux lectures privées alimentent la propagation :

- `nano_altairInventory(token, afterRoot, limit)` : `afterRoot` vaut `"0x"` au
  début, puis le curseur `next`. La limite est une quantité hex positive. Les
  racines archivées portent parent et slot. Le catalogue initial est trié par
  slot puis racine (parents avant enfants) ; les nouveaux blocs sont ajoutés
  à la fin, sans déplacer les curseurs existants. Un tick seul conserve le catalogue.
  Le budget de réponse réduit la taille d'une page, jamais le nombre total de
  blocs. `missingSignedBlocks` signale les états connus sans payload signé,
  hors ancre. Les pages portent séquence, digest et `catalogEpoch`. Un changement
  d'époque du catalogue impose de reprendre depuis `"0x"`. Le dernier root reçu
  reste utilisable pour découvrir des ajouts après une page dont `next` est null.
- `nano_altairBlock(token, root)` : retourne les octets archivés dans l'enveloppe
  IPC compacte. Un payload absent renvoie `available: false`, avec distinction
  bloc connu/ancre. Un payload trop grand pour l'export échoue explicitement.

Ces lectures utilisent les références d'un même état publié et la file RPC
bornée, en dehors du verrou global RPC. Le daemon diffuse automatiquement les
blocs aux pairs dont l'identité de consensus est compatible, uniquement lorsque
l'admission Altair est explicitement activée. Les archives restent incluses dans les snapshots
complets, soumises à `limits.envelopeBytes` et à la taille de bloc du profil :
stockage incrémental, élagage et débit d'écriture restent à qualifier.


### Diffusion automatique des blocs, 25 septembre 2026

Le daemon choisit les pairs à tour de rôle, avec une seule tentative active et
un bloc par tentative. À chaque tick réseau, il lit une page privée, exporte le
SSZ durable, puis l'envoie sur `/nano/altair/1.0.0`. Le curseur propre au pair
avance uniquement après un acquittement exact `{"accepted":true}`. Une perte,
un rejet ou un délai expiré conserve le curseur. Un redémarrage du daemon
reprend le catalogue ; une nouvelle session du cœur distant remet son curseur
à zéro. La progression est volatile ; les octets source sont durables.

La qualification `consensus_altair_propagation` utilise deux vrais daemons et
deux serveurs HTTP de cœur sur loopback, avec horloge de fixture. Deux blocs
signés officiels sont diffusés automatiquement, le premier acquittement est
supprimé après traitement, puis le daemon émetteur est arrêté et relancé avant
la diffusion du second bloc. Les octets reçus et la tête finale sont vérifiés,
ainsi que la stabilité du curseur après un tick et un ajout. Ce test ne redémarre
pas les cœurs et ne démontre pas de finalité distribuée.

Limites restantes : un rejet permanent bloque la progression vers ce pair ;
la réconciliation des branches/finalités et la récupération des dépendances
restent à réaliser. Les preuves de slashing ne sont pas encore diffusées par ce
relais ; les attestations sont traitées par l’extension décrite plus bas. Il n'y a pas encore de comparaison d'inventaires distants ni de mesure
de débit à grande échelle. Le lien atomique EVM/consensus, les devoirs et signatures
des validateurs et la qualification de finalité distribuée restent ouverts.

Preuves locales : `build-cmake/qualification/altair-block-propagation-20260925/`,
14/14 tests réseau et 3/3 tests cœur (ingress libp2p, archive et propagation).


### Reprise du cœur après arrêt brutal, 25 septembre 2026

`consensus_altair_restart_libp2p` exécute le cœur récepteur dans un processus
séparé avec RocksDB et son vrai serveur HTTP ; les deux daemons utilisent
libp2p. Après acquittement du premier bloc officiel, le test arrête le daemon
récepteur, conserve à l'extérieur du cœur son engagement séquence/digest, puis
tue le cœur par `SIGKILL`. Le daemon émetteur reste actif et son cœur ajoute le
second bloc pendant l'indisponibilité du récepteur.

Le nouveau processus récepteur rouvre les mêmes bases avec cet engagement
attendu. Le test vérifie le premier bloc, sa tête, sa séquence et son digest,
ainsi qu'une nouvelle session de gossip. Le daemon récepteur redémarre avec la
même clé P2P et retrouve l'émetteur par bootstrap. Le bloc manqué arrive sans
injection directe dans le cœur récepteur ; les SSZ exportés sont comparés aux
octets signés source. Un second `SIGKILL` suivi d'une réouverture vérifie les deux
blocs persistés et la tête attendue par la fixture officielle.

Preuve : `build-cmake/qualification/altair-core-crash-catchup-20260925/verified.log`
(1/1 test, deux réouvertures après `SIGKILL`). L'horloge est celle de la fixture.
Le daemon est arrêté avant chaque capture de l'engagement de redémarrage : cet
essai ne couvre pas une interruption au milieu d'un commit ni la récupération
automatique d'un engagement externe devenu obsolète. Une panne de processus
n'est pas une coupure électrique du stockage. Finalité distribuée, durabilité
Linux et liaison atomique EVM/consensus restent à qualifier.


### Conservation des attestations signées, 25 septembre 2026

Le réacteur conserve désormais les attestations de gossip qui modifient
`latest_messages`, dans le même checkpoint que leur effet sur le choix de
branche. La structure publiée contient `signed_votes` : hash SHA-256 du SSZ
canonique vers octets immuables. Les entrées directes et SSZ passent toutes par
la validation de comité, d'ascendance, de fenêtre temporelle et de signature.
Un doublon reste vérifié, mais n'ajoute pas de copie ; un vote sans nouvel effet
n'alimente pas cette archive. Les attestations incluses dans un bloc restent
transportées par les octets signés de ce bloc.

Le format local `nano-altair-durable-v3` ajoute `signedVotes` au format v2.
Les formats v1/v2 restent lisibles sans inventer les signatures absentes. Si
l'archive des votes est vide, le checkpoint conserve le format précédent
approprié. Lors de la restauration, le digest externe est vérifié avant parsing,
puis chaque vote est décodé et sa signature, son ascendance et sa fenêtre sont
revérifiées contre l'état authentifié. Cette vérification ne réapplique pas le
vote au Store restauré.

Le nombre d'entrées est borné par `limits.messages` et leurs octets partagent
`limits.envelopeBytes` avec le Store et les blocs. Un dépassement échoue avant
commit, sans publier le candidat. À chaque publication, les payloads dont
l'époque cible précède l'époque courante moins un sont retirés dans le même
commit ; les derniers votes du choix de branche ne sont pas supprimés.
C'est une archive de retransmission temporaire, pas un historique complet.
Les votes sans effet sur un ancien checkpoint ne servent pas à reconstituer
rétroactivement les signatures manquantes.

La campagne `build-cmake/qualification/altair-signed-votes-20260925/` vérifie
l'encodage octet pour octet de 41 attestations officielles, la réouverture,
les doublons, l'expiration, les archives corrompues avec digest recalculé et
l'absence de publication lors d'un échec de synchronisation disque injecté.
Les suites officielles durables de choix de branche/réorganisation et les
scénarios P2P de propagation/reprise sont également relancés.

Le raccordement des attestations au daemon est décrit ci-dessous ; celui des
preuves de slashing reste à réaliser. Le nouveau format local n'active aucun profil réseau
ni aucune finalité distribuée ; le stockage utilise encore des snapshots complets.


### Diffusion des attestations, 25 septembre 2026

Deux RPC privés authentifiés alimentent le relais :

- `nano_altairVoteInventory(token, afterRoot, limit)` : catalogue paginé `votes`,
  chaque entrée portant le hash SHA-256 des octets signés (`root`), le slot et
  `beaconBlockRoot`. Les curseurs sont stables lors des ajouts. Une expiration
  reconstruit le catalogue avec un nouveau `catalogEpoch` ; un ancien curseur
  absent est refusé. Le champ partagé `missingSignedBlocks` vaut zéro pour ce
  catalogue ; il ne garantit pas la disponibilité de votes historiques absents.
- `nano_altairVote(token, root)` : export compact du SSZ signé, soumis au budget
  de l'attestation du profil et au plafond IPC. Une attestation expirée retourne
  `available: false`. Un hash de vote n'est pas un identifiant de transaction EVM.

`AltairArchiveRelay` traite blocs et attestations avec le même contrôle de
profil et d'acquittement. Chaque pair dispose de deux curseurs indépendants.
L'ordonnanceur alterne leurs tentatives, à tour de rôle entre pairs, avec une
seule tentative globale active. Un vote refusé (par exemple bloc inconnu ou
horloge distante en retard) conserve son curseur sans bloquer le tour des blocs.
La modification de session distante réinitialise les deux curseurs. Une époque
ou un curseur de catalogue devenu invalide entraîne une reprise depuis le début.

La campagne `build-cmake/qualification/altair-vote-propagation-20260925/` passe
14 tests réseau (dont 18 cas bloc/vote du relais) et 3 tests P2P réels. Le nouveau
scénario utilise la fixture officielle `shorter_chain_but_heavier_weight` :
quatre blocs arrivent au second cœur sans votes ; sa tête est différente du
résultat final. Une attestation est ensuite injectée uniquement dans le cœur
source. Sa diffusion P2P fait changer la tête distante et les derniers votes
vers les résultats attendus. La perte du premier ACK est retentée ; les octets
reçus sont identiques et l'archive reste à une entrée. Les lectures non autorisées,
la disparition des payloads expirés et l'invalidation du curseur sont vérifiées.

Ce test utilise deux daemons et deux cœurs HTTP sur loopback, avec horloge de
fixture. Il ne produit pas de nouveaux votes avec des clés de validateurs, ne
finalise pas plusieurs epochs distribués et ne mesure pas le débit d'un grand
réseau. La réconciliation des branches et les rejets permanents restent ouverts,
ainsi que les devoirs/signatures des validateurs, le gossip des slashings et la
liaison atomique EVM/consensus.
