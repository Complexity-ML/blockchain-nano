# Préparation multi-hôte — 2026-09-26

Cœur : commit `b7e27489b3dc6ff5870af507615e83ba28f04569`.
Le Mac natif, Docker ARM64 Mac et Docker AMD64 VPS doivent être comparés.
Deux machines physiques, trois instances et 64 clés synthétiques distinctes.

Docker Mac : build réussi, image
`sha256:ced09365b61834ee2a4989d20fb66cf891f305c3123d4abcc035489d9c9a38e4`,
étiquette de révision vérifiée, `nano --help` passé sans réseau.
VPS : build `t87f4chgr9f66cdghlajlljl0` terminé avec succès, image AMD64
à la même révision ; `vps-build.json` conserve le résultat Buildx.
Ce document ne constitue pas une réussite réseau.

Le moniteur généralisé passe 13 tests, dont les refus d’un troisième nœud
retardé ou d’une divergence de son exécution. Le préparateur a été exécuté
sur les fixtures de l’exporteur natif : 22/21/21 clés, tous les indices 0–63
présents exactement une fois. Le lanceur `tests/multihost/run.mjs` est préparé
et sa syntaxe vérifiée ; les résultats des campagnes réelles figurent ci-dessous.

## Première campagne réelle : échec de lancement

`startup-without-init/` conserve la tentative arrêtée avant connexion des trois
instances. Le relais refuse `NANO_SUPERVISOR_PID=1` (`Invalid supervisor PID`).
Le lanceur avait omis `--init`, présent sous `init: true` dans Compose.
Correction : utilisation de `--init` pour les deux conteneurs et capture des
deux flux de logs. Aucune règle de consensus ni image modifiée. Nouvelle
campagne exécutée avec fixtures fraîches ; résultat ci-dessous.

## Campagne avec init : accord observé, continuité non qualifiée

Trois instances connectées (Mac natif, Docker ARM64 Mac, Docker AMD64 VPS),
deux machines et 64 clés synthétiques réparties 22/21/21. Checkpoints communs
observés : 2 → 3 → 4 → 6. Le moniteur refuse le saut 4 → 6 : le critère
de trois avancées consécutives n’est pas atteint. Aucun conflit de finalité
n’est observé ; un saut ne prouve pas à lui seul une violation de FFG.
Les trois instances terminent sur la même racine d’epoch 6.

Propositions admises : 22/22/20 ; attestations admises : 163/150/151.
Maximum du temps de proposition : 309/393/2571 ms ; maximum du poll
validateur : 584/652/3924 ms, d’après les compteurs internes à l’arrêt.
Ces maxima ne mesurent pas la propagation P2P. Les timestamps des lectures
séquentielles SSH/Docker ne la mesurent pas non plus. Les événements
`head-changed-during-finality-read` proviennent du moniteur JavaScript.

Tous les processus de campagne sont arrêtés ; données et conteneurs arrêtés
conservés pour diagnostic. Prochaine analyse : participation par epoch et
branche, délais de diffusion/inclusion et coût des propositions sur le VPS.
Ni charge supplémentaire ni assouplissement des seuils effectué.

## Diagnostic du checkpoint natif arrêté

Extraction RocksDB en lecture seule (`read-checkpoint.cpp`), empreinte du
checkpoint dans `participation-diagnostic.json`. Inspection SSZ ad hoc
paramétrée pour cette fixture (64 validateurs), pas un vérificateur indépendant.
Branche de slot maximal suivie par liens parents. Au slot 47 (fin epoch 5),
34 indicateurs de cible courante sont présents ; après transition, 48 au slot
48, 50 au slot 49 et 64 au slot 50 pour cette même epoch devenue précédente.
La justification reste à 4 au slot 48 ; elle atteint 6 au slot 56, puis la
finalité atteint 6 au slot 64. Les attestations sont donc incluses avec retard
pour certaines frontières. Ces nombres sont des comptes de flags, pas une
preuve indépendante du quorum pondéré. Départ des votes, diffusion et sélection
par le proposant restent à départager avant correction du code réseau.

## Campagne instrumentée : réussite bornée

Même cœur b7e2748 et même topologie, fixtures neuves, traces existantes activées
via `NANO_TEST_NETWORK_TRACE=1`. Le moniteur observe 2 → 3 → 4 → 5 sur les
trois instances, liées à leurs blocs d’exécution, en 199002 ms. Lanceur terminé
avec code 0, processus arrêtés. Résultats et logs dans `with-trace/`.

Maximum de proposition : natif 374 ms, Docker Mac 266 ms, VPS 5490 ms ;
maximum de poll VPS 5963 ms pour des slots de 6000 ms. Opérations de relais
tracées jusqu’à 5000 ms sur VPS, dont réponses négatives ; une tentative
échouée ne signifie pas une perte définitive. Le relais global est séquentiel.
Ces observations justifient une mesure détaillée des étapes et de l’attente
des verrous ; elles ne prouvent pas encore une cause unique.

La réussite de cette fenêtre ne résout pas la variabilité observée dans la
campagne précédente, ni l’endurance, les partitions WAN, la charge ou la
production. Aucun réglage de consensus ni assouplissement du moniteur.

## Campagne avec mesures par étape (5131589)

La fenêtre de trois avancées 2 → 3 → 4 → 5 réussit en 143494 ms sur
les trois instances. Lanceur terminé avec code 0. Mesures et logs conservés
dans `with-phases/`. Aucun autre changement fonctionnel que l’instrumentation.

Les traces ne comprennent que les tentatives lentes (>500 ms) ou échouées.
Sur VPS, une récupération locale de vote atteint 2198994 µs ; sur Mac natif,
l’attente d’acquittement d’un vote atteint 1562563 µs. Ces observations
localisent des délais côté cœur VPS, mais ne séparent pas encore attente
du verrou et calcul sous verrou. La construction locale réexécute une
transition complète pour chaque attestation candidate : piste à optimiser
avec contrôle de validité conservé et comparaison avant/après.

Deux fenêtres réussies et un échec antérieur ne clôturent pas le retard
intermittent ni la qualification d’endurance.
