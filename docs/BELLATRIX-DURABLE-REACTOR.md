# Coordinateur durable Bellatrix

`BellatrixDurableReactor` sérialise les événements locaux bloc, vote, preuve de
slashing et horloge. Il prépare un Store privé, conserve les messages signés,
sérialise le checkpoint, calcule le suffixe d'exécution à remplacer puis appelle
`commit_execution_checkpoint`. Le nouveau Store n'est publié qu'après succès de
l'écriture synchronisée. Aucune allocation ni callback n'intervient entre ce
succès et les déplacements non levants des pointeurs/jeton publiés.

La recherche d'ancêtre commun compare les blocs d'exécution du candidat avec la
base. Les deux états et les reçus de chaque bloc proviennent du même enregistrement
Bellatrix. Le coordinateur vérifie la continuité de l'ancre et refuse une nouvelle
tête qui ne descend pas du checkpoint déjà finalisé. Le stockage historique avec
certificats ne devient pas automatiquement un stockage Bellatrix.

L'initialisation nécessite une chaîne d'exécution persistée et un Store de confiance.
Le redémarrage exige un jeton indépendant exact, revalide le snapshot, ses archives
signées et la concordance de la tête, de l'état EVM et des reçus avec RocksDB. Les
messages de blocs admis sont conservés sans remplacement par des doublons. Les
votes ayant modifié les latest messages sont conservés dans la fenêtre courante/
précédente, puis élagués. Un Store initial fourni par l'appelant peut ne pas disposer
d'archives pour les blocs qu'il contenait déjà ; aucune enveloppe n'est fabriquée.

Le verrou est partagé entre les coordinateurs utilisant le même ChainStore. Le
CAS de stockage refuse un auteur obsolète. Un échec d'écriture rend le coordinateur
indisponible jusqu'à reprise ; une annulation de préparation laisse le jeton et
l'état précédents intacts. Les callbacks ne doivent pas réentrer le coordinateur.

## Qualification locale

`consensus_bellatrix_signed_execution_chain` passe les mêmes événements au Store
pur et au coordinateur : blocs, branche concurrente, ticks et six votes BLS. Le
Store publié doit être identique. Les deux bascules de tête sont persistées par
le coordinateur, et un redémarrage conserve neuf enveloppes de blocs et six votes.
Trois annulations de tick (précoce/intermédiaire/tardive) ne publient rien. Un second
coordinateur fait avancer le checkpoint, puis le premier est refusé comme obsolète.

Sur macOS, une erreur de synchronisation du WAL est injectée pendant un tick : le
coordinateur se ferme, et le pointeur du Store précédemment publié demeure inchangé.
Les tests de la primitive atomique couvrent séparément SIGKILL et conservation/perte
du suffixe non synchronisé. Ce scénario ne coupe pas physiquement l'alimentation et
ne démontre pas une finalité sur plusieurs epochs.

Preuves : `build-cmake/qualification/bellatrix-reactor-20260925/`.

## Périmètre restant

Ce coordinateur n'est pas encore un service P2P et n'active aucun fork réseau.
Les signatures locales et leurs duties sont raccordées au journal anti-slashing :
voir [BELLATRIX-LOCAL-SIGNING.md](BELLATRIX-LOCAL-SIGNING.md). Restent la boucle
autonome, le transport/rattrapage et la qualification des pannes/partitions avec
plusieurs processus. Les snapshots complets à chaque événement
et la conservation des blocs sont un socle fonctionnel, sans qualification de
capacité ni pruning des branches. Les preuves d'attester slashing reçues mettent
à jour l'état mais ne disposent pas encore d'une archive dédiée à leur rediffusion.
