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

## État d’intégration au 26 septembre 2026

Le coordinateur est raccordé au runtime, à la boucle autonome des validateurs,
au journal anti-slashing et au transport/rattrapage P2P. Les archives durables
conservent aussi les preuves d’attester slashing et les sorties volontaires ;
les inventaires et exports correspondants existent dans `BellatrixRpcService`.
Ces intégrations ne sont plus des éléments non commencés.

La partition prolongée à deux cœurs et deux démons P2P, 64 clés synthétiques
réparties 32/32 sur un Mac, passe : coupure de 96 s, reprise en 95 s, puis
finalité commune 6→7→8→9. La fenêtre débute après concordance du checkpoint
Beacon réalisé avec le bloc finalisé d’exécution.
[Preuves et périmètre](qualification/recovery-sustained-20260926/recovery-bound/README.md).
Le test SIGKILL avec ces mêmes binaires passe en 325,58 s, suivi de trois
avancées communes 2→3→4→5 pendant 144,793 s.
[Preuves de reprise](qualification/recovery-sustained-20260926/crash-current/README.md).

Les observations d’horloge sans changement de slot ni franchissement du seuil
d’attestation sont coalescées ; les changements de phase et les événements
de consensus conservent la publication durable synchronisée. Le cache local
de validation ne dispense pas de la revalidation lors d’une restauration.

## Périmètre restant

Les snapshots complets et les états de tous les blocs restent conservés.
L’élagage des anciens états/branches, la conservation et le service de l’archive
historique après cet élagage, ainsi que la tenue au-delà des budgets de blocs
et d’enveloppe restent à implémenter/qualifier. Un catalogue de votes borné ou
une meilleure allocation RocksDB ne résout pas cette rétention des états.
Les essais locaux bornés ne prouvent ni une longue endurance, ni une capacité
multi-hôte ou à très grand registre, ni l’ensemble des critères de production.

### Dépendances de l’élagage à implémenter

L’audit du chemin actuel relève trois couplages : `durable_snapshot` exige que
chaque bloc signé archivé existe dans `Store.blocks`, `publish` exige que
l’ancre reste inchangée et `bellatrix_store_from_snapshot` reconstruit le registre
justifié depuis les états authentifiés. Une suppression directe des anciens
états rendrait donc le checkpoint ou son historique inutilisable.

Le chantier doit préserver ensemble :

1. Une archive persistante des blocs signés avec index par racine et slot,
   accessible au rattrapage P2P après retrait des états complets de la mémoire.
2. Une frontière d’élagage issue de la finalité authentifiée, avec conservation
   des contextes nécessaires aux votes, au slashing, aux sorties, aux dépôts et
   aux ancêtres d’exécution utilisés par BLOCKHASH.
3. Une publication atomique de l’archive, du checkpoint et de l’état d’exécution,
   avec migration explicite du format durable et validation à la réouverture.
4. Une qualification dépassant la capacité de l’ancien snapshot : finalité et
   tête inchangées face à un témoin sans élagage, refus des branches incompatibles,
   rattrapage d’un nouveau pair depuis l’historique, et crash aux frontières de
   commit. Mesurer mémoire, disque, temps de reprise et taille du checkpoint.

Cette liste décrit du travail restant, pas une fonctionnalité livrée. Augmenter
les budgets du snapshot ne clôt pas le critère de rétention.
