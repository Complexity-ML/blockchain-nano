# Finalité continue après incident — 26 septembre 2026

Deux processus Nano Release et deux démons cpp-libp2p sur un seul Mac,
64 clés synthétiques réparties 32/32. Slots de 6 secondes, époques de 8 slots.
Aucun fonds réel, aucun déploiement VPS. Les empreintes sont dans `manifest.json`.

## Nouveau critère

Les cibles `consensus_bellatrix_partition_sustained` et
`consensus_bellatrix_crash_sustained` prolongent la reprise existante par trois
avancées consécutives de finalité commune. À chaque avancée, les blocs Beacon
et les blocs d’exécution doivent correspondre sur les deux nœuds. Une
régression, une racine finalisée contradictoire, une tête immobile trois slots,
une finalité commune immobile deux époques ou un retard supérieur à quatre
époques provoque un échec. Les observations sont périodiques, une par slot.
Cette fenêtre ne démontre pas une stabilité indéfinie.

## Partition : échec du critère renforcé

Campagne de 521,06 secondes. Pendant 96 secondes de coupure des deux processus
P2P, les branches divergent mais la finalité ne progresse pas. Après reconnexion,
les nœuds retrouvent une tête commune et une finalité qui avance : l’ancien
critère de reprise passe. La sonde supplémentaire échoue ensuite avec
`Finality lag exceeds four epochs`.

Les snapshots fermés, vérifiés par leurs engagements d’arrêt indépendants,
contiennent chacun 89 blocs, un slot maximal 88 (époque 11), une finalité à
l’époque 6 et une justification à l’époque 10. Les racines finalisées sont
identiques. Voir `partition-stopped-summary.json`. Ce résultat ne démontre ni
finalités contradictoires ni corruption ; il laisse ouverte la récupération
suffisamment rapide de la finalité après partition. Ne pas supprimer ce résultat
au profit du succès plus étroit de convergence.

Bases synthétiques retenues localement :
`nano-assertoor-core-failure-F08t65/snapshot` dans le répertoire temporaire macOS.
Copies exportées hors ligne : `/private/tmp/nano-recovery-diagnosis-F08t65`.
Ces bases ne sont pas incluses dans Git. Les traces ne montrent pas de panne du
validateur ni de dispatch durablement bloqué ; la cause exacte reste ouverte.

## Crash distribué

**Réussi en 325,89 secondes.** SIGKILL du processus portant la moitié du stake,
sans reçu d’arrêt propre ; reprise à partir des témoins durables et conservation
du bloc observé avant la panne. Les nœuds finalisent ensuite les époques
**2 → 3 → 4 → 5**, soit trois avancées consécutives en 145,438 secondes,
avec les mêmes hashes d’exécution. Les reçus d’arrêt confirment la même finalité
5 et la justification 6. Preuves : `crash/result.json` et `crash/samples.jsonl`.

La campagne complète termine avec **1 réussite / 2 scénarios**, en 846,96 secondes.
Les six tests déterministes du moniteur passent. `campaign-summary.log` conserve
les événements utiles ; le journal brut local est identifié par `raw-log.json`.
Ces résultats ne couvrent ni crash machine physique, ni plusieurs hôtes,
ni charge applicative, ni l’ensemble des points d’injection pendant une écriture.

## Contre-épreuve du délai de relais

Une salve limitée à quatre tentatives réussies par tick a été essayée sans
changer les règles de consensus ni les seuils de test. La convergence initiale
intervient peu après 96 secondes après reconnexion, mais le contrôle prolongé
échoue en **501,40 s** : `Common finality stalled for two epochs`. Les observations
et le patch expérimental sont dans `bounded-sweep/`. Ce résultat ne permet pas
d’attribuer le défaut au seul intervalle de relais. La modification est retirée ;
les règles antérieures du relais sont restaurées. Examiner maintenant les
attestations et leurs sources/cibles dans les branches produites après partition.

## Diagnostic des attestations incluses

`nano-bellatrix-checkpoint-export` accepte maintenant `--attestation-report`
(après le chemin du fichier de configuration). Il travaille sur une copie de
base arrêtée, vérifie l’engagement externe du snapshot, calcule la tête par le
fork-choice natif, puis décode les attestations et retrouve les indices via
les comités de leur état cible. Il ne signe ni ne publie de transition.

Les rapports des deux copies de l’échec initial sont identiques (tête slot 88).
Les époques 1–3 retrouvent bien les 64 indices distincts. Aux époques 8 et 9,
seuls les indices 32–63 figurent avec une cible correspondant à la branche
retenue. Les indices 0–31 sont tous retrouvés sur une autre branche à l’époque
8 ; 17 d’entre eux le sont à l’époque 9. Le sort des 15 autres à cette dernière
époque n’est pas déduit de ces seuls blocs. Certains votes inclus dans la
branche retenue ont une autre cible et ne doivent pas être comptés comme
participation utile à cette cible.

Le contre-essai par salves atteint 42 indices distincts avec la cible retenue
à l’époque 8 (10 + 32), puis 64 à l’époque 9. Le défaut observé est donc lié à
la convergence des branches et à l’inclusion des attestations, et pas seulement
à l’absence de blocs ou au transport brut. Ces comptages ne remplacent pas
le calcul pondéré complet de justification/finalisation dans la transition.

Rapports : `attestations/validator.json`, `observer.json`, `summary.json` et
`bounded-sweep.json`. Les exports ont été exécutés après compilation Release ;
les contrôles d’égalité des deux rapports, de présence des 64 indices avant
partition et des seuls indices 32–63 aux époques 8–9 passent. La cause racine
et sa correction restent ouvertes ; aucun nouveau scénario réseau n’a été
relancé pour ce diagnostic hors ligne.

## Acquittements de votes mémorisés par session

Le relais garde désormais un cache FIFO borné de 2048 racines de votes
explicitement acquittées. Ce budget mémoire ne limite ni les validateurs ni
les votes transmissibles : une entrée évincée est simplement retransmise.
Le changement d’époque du catalogue conserve ces acquittements ; le changement
de session distante les efface. Refus et réponses perdues ne sont jamais
mémorisés comme acceptations. Les blocs, preuves et sorties gardent leur chemin
existant. Le cache est une optimisation de transport, jamais une preuve de
consensus.

Les nouveaux contrôles de réessai, changement de catalogue/session et éviction
passent. Sur le scénario réel, 293 et 370 envois sont évités ; 40 et 41 doublons
restent comptés sur l’ensemble des sessions, contre 206/228 dans l’ancien essai.
Les exécutions ne sont pas identiques, donc ce n’est pas une mesure isolée de
gain de débit. **La partition prolongée échoue encore en 503,39 s**, avec une
finalité immobile deux époques. Cette optimisation ne clôt pas le défaut.
Résultats et empreintes : `ack-cache/`. La correction est conservée pour les
retransmissions avérées ; les règles de consensus ne sont pas modifiées.

## Témoin à livraison directe et horloge contrôlée

Nouvelle cible `consensus_bellatrix_partition_simulation` : deux cœurs durables
Nano dans un processus, 64 clés réparties 32/32 avec leurs journaux indépendants,
vraies signatures BLS, transitions Beacon/EVM et écritures RocksDB. L’horloge
avance explicitement au début, au tiers et aux deux tiers de chaque slot.
Les blocs disponibles sont livrés par ordre de slot, puis les votes ; l’ordre
local des deux validateurs alterne entre les slots. Aucun socket ni P2P réel.

L’isolement couvre les slots 33–48. Des branches distinctes sont effectivement
produites, sans progression de finalité sur une moitié du stake. Les livraisons
reprennent au slot 49. Les têtes et états d’exécution finaux concordent ; après
la récupération du checkpoint 6, les checkpoints **7, 8, 9, 10, 11 et 12**
se succèdent sans saut. **Réussi en 122,04 s** de temps machine, 112 slots simulés.
Preuves dans `direct-delivery/`.

Ce témoin montre que ce scénario peut se réunifier avec la logique actuelle
quand la livraison et l’horloge sont contrôlées. Il oriente le diagnostic vers
l’intégration temps réel (ordre, délais, échéances et rattrapage), sans prouver
l’absence de défaut du consensus dans tous les ordres possibles. Il ne remplace
pas la cible réseau `partition_sustained`, toujours en échec, et ne constitue
pas une qualification multi-hôte ou une preuve de production.

## Mesure des échéances du validateur

Le statut du validateur expose un objet `timing` avec les nombres de cycles,
le maximum de durée d’un cycle et d’une proposition (préparation, signature et
admission), les cycles durant au moins un slot, les propositions indisponibles
et les sorties d’un cycle à une frontière de slot. Les durées utilisent une
horloge monotone ; les compteurs sont volatils, remis à zéro au démarrage et
protégés par le verrou existant. Ils ne changent aucune règle de consensus.

Le témoin déterministe instrumenté passe en **134,92 s**. Maximum des cycles :
759/1162 ms ; maximum des propositions : 448/961 ms ; aucun cycle de six secondes
ni proposition indisponible. Ces mesures concernent ce Mac et cette exécution.

Une **copie** du snapshot synthétique ACpQT4 est également rouverte avec le runtime
natif et une horloge réelle relative au checkpoint. Après initialisation et
24 secondes d’observation demandée, sa tête passe du slot 85 au slot 90 et sa
finalité de 6 à 9, sans P2P ni nouveau pair. Trois propositions et 21 attestations
locales sont admises ; maximum de cycle 661 ms, proposition 261 ms, aucune
sortie à la frontière de slot. Ce n’est ni une reproduction de la charge réseau
ni une preuve que les échéances sont toujours tenues. La copie modifiée est
`/private/tmp/nano-timing-resume-ACpQT4` ; la preuve originale est conservée.

Cette reprise tardive distingue le dépassement du critère strict de stabilité
de la preuve d’un blocage définitif. Elle ne transforme pas l’échec réseau en
réussite : le délai et l’ordre du rattrapage doivent encore être qualifiés.
Preuves dans `validator-timing/`.

## Coalescence des observations d’horloge Bellatrix

`BellatrixLocalClock` conserve en mémoire chaque observation pour détecter les
retours en arrière et sauts excessifs. `tick_if_needed` ne publie un checkpoint
que si le slot change ou si le seuil d’attestation est franchi. Les lectures du
profil Bellatrix utilisent ces deux distinctions pour les effets de l’horloge :
promotion de checkpoints/reset du boost aux frontières, disponibilité des
attestations et timeliness des blocs. L’horodatage stocké peut rester en retrait
à l’intérieur de la même phase. `tick()` explicite conserve son contrat de
publication durable ; WAL et synchronisation des écritures sont inchangés.

Le test d’équivalence compare l’état complet à des transitions à chaque
observation, en normalisant uniquement l’horodatage d’observation. **45
observations → 17 publications**, jusqu’à une frontière d’époque. Les contrôles
de saut excessif, retour en arrière au sein d’une phase et annulation passent.
Les quatre cibles (horloge, runtime/restart, chaîne signée, témoin de reprise)
passent en **29,38 s** ; les deux cas limites supplémentaires passent ensuite.
Le test du témoin utilise un `tick()` explicite pour forcer sa publication
unique, car une observation sans changement de phase n’est plus une écriture.

Sur deux copies du même checkpoint synthétique, la reprise courte conserve
3 propositions, 21 attestations, une tête au slot 90 et le même checkpoint
finalisé 9, avec **33 publications contre 167** avant modification. Ces essais
ne sont pas une mesure de débit sous charge ni une qualification multi-hôte.
La campagne `partition_sustained` échoue en **519,24 s**, avec les mêmes seuils :
`Finality lag exceeds four epochs`. La reconnexion restaure une finalité commune,
puis les onze relevés gardent le checkpoint 6 pendant que les têtes passent de
75–77 à 87. À l’arrêt, les deux nœuds ont justifié l’époque 10, sans finaliser
au-delà de 6. Aucun cycle du validateur ne dépasse le slot de six secondes
(maxima 1 430 et 903 ms). La coalescence réduit les écritures mais ne résout pas
le retard de finalité après partition. Preuves et empreintes : `clock-coalescing/`.

Le diagnostic de cette même base arrêtée retrouve, sur la branche canonique,
32 validateurs distincts avec cible correcte à l’époque 8, puis 33 à l’époque 9
(32 du nœud observateur, respectivement 0 et 1 du nœud source). À l’époque 10,
les 64 apparaissent de nouveau, avec source 7. Ces comptes expliquent pourquoi
la production continue ne suffit pas à prouver la finalité ; ils ne remplacent
pas la vérification FFG pondérée. La piste suivante est la sélection de branche
et la disponibilité des votes du nœud source pendant le rattrapage, plutôt
qu’une nouvelle optimisation des seules écritures disque. Voir
`clock-coalescing/attestation-summary.json` et le rapport brut associé.
