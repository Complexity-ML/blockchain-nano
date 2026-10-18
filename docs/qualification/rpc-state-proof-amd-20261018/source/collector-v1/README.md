# Collecteur/audit de continuation AMD — source Root non exécutée

Cette nouvelle source adapte `collect-and-verify-amd-rpc-candidate-root-20261018.py`
(`113284fc…` /10 803 octets) sans modifier son parent. L'auteur n'a exécuté ni
importé le projet/collecteur, lancé aucun parse/test, Docker, SSH, C++ ou Node,
et n'a lu aucune autorité privée ou SDK. Les seules lectures portent sur les
sources et reçus **publics** nécessaires à la conception. Le manifeste est livré
dans son état final ; aucune mutation après GO/pins communiqués.

## Cible exacte et contrat hôte

* Source C++ séparée : C6 `c6cdf780c68eab8b06306d76257bda9028928ea3`.
* HEAD documentaire/JS attendu : `4388f77b9e9c1b3b94ba5da6f3e486eaa13a3f5c`.
* Sortie distante unique : `/home/ubuntu/rpc-candidate-amd-continuation-root-20261018-01`.
* Dispatcher local : `E/amd-rpc-continuation-dispatch-root-20261018-01`.
* Nouvelle sortie locale exclusive : `E/rpc-candidate-amd-continuation-public-audit-root-20261018-01`.
  E est le workspace `qualification-resume-20261012`, codé explicitement : le
  collecteur reste dans ce sous-dossier source, pas à déplacer en supposant que
  son parent immédiat serait E.

L'hôte proposé corrigé `amd-rpc-continuation-source-v2-20261018/host.py`
(`582389ce…` /39 355 octets) est lu comme **source**, jamais importé. Son résultat
doit rapporter `retainedProductRows:265` et
`newWorkerProductRowsHeldThroughCleanup:16`, tous deux entiers exacts, ainsi que
les clôtures sources/produits après CLI, cleanup et inspection finale de l'image.
Le collecteur ne lit/transfère pas ses autorités privées et ne leur substitue pas
ses propres observations. Root conserve séparément les preuves de staging et
l'authentification du code/contrôle réellement exécuté.

## Lecture publique : exactement 56 fichiers

L'allowlist est statique, sans découverte depuis les valeurs reçues :

* Quatre fixes : result-public.json, host-final-fence-public.json,
  prepared-public.json et outputs/result-public.json.
* Quatre cas candidats × quatre : case-public, exit, stdout et stderr sous
  outputs/ pour proof/cost/state-readers/legacy-rpc.
* Neuf CLI retenues × quatre : retained-case-public, retained-test exit,
  stdout et stderr à la racine distante.

Aucun nom contenant `.private` ni chemin hors allowlist n'est accepté. La
lecture distante impose avant allocation une somme de tailles ≤16 Mio, puis
vérifie chaque FD/path neuf champs/SHA et refait la clôture des 56 fichiers avant
de les transmettre. Base64/JSON wire ≤32 Mio ; le lecteur local applique la
limite **pendant** le drainage du processus, avant décodage/copie. Stderr ≤1 Mio.
Le consommateur refuse toute ligne inconnue, dupliquée, manquante, hors taille,
SHA incohérent ou ligne FD9 mal typée. Les rows de readback sont explicitement
des observations actuelles, pas des autorités historiques.

Une seule lecture SSH bornée120s est prévue, sur la machine déjà autorisée et
avec host-key stricte. Aucun Docker, image, runtime ou test C++ n'est lancé par
le collecteur. Les rapports ne sont pas réécrits pour modifier leurs flags.

## Audits mécaniques et sémantiques

Le nouveau worker doit être retained, coreProof:null et exactement quatre
M/CXX/liens/ldd/tests, zéro Core/runtime répété, mmd4 avec égalité M/MD et quatre
produits candidats non réutilisés. Les neuf résultats runtime sont joints
séparément depuis `host.retainedRuntimeCases` et leurs fichiers/exit/stdout/stderr
réels. Le host/worker/case-public et les exit codes doivent concorder.

Les politiques stdout/stderr des quatre tests et neuf CLI sont reprises depuis
les deux documents publics originaux épinglés172315… et aa613… . Les flux réels
sont contrôlés à nouveau sans exécuter de C++ : empty, exact, JSON typé,
last-line et forme du flux de preuve. La vérification cryptographique de ce
dernier est une étape indépendante suivante, pas la confiance dans un flag PASS.

La preuve Core précédente est jointe au fichier public original acabca… :143
membres,44 sélectionnés,99 hors sélection inchangés et Core3ef6c…/40 988 476 B.
L'audit public e1b1f0… du parent doit toujours dire exit1/TimeoutExpired/FAILED.
Les compteurs bruts combinés sont156 M (152 anciens +4 nouveaux), et non155 :
l'ancienne première analyse proof précédait son CXX interrompu. Le cumul porte
sur deux enveloppes et ne transforme pas la première en PASS. Ses52 CXX/8liens
restent réutilisés ; seuls4 CXX/4liens supplémentaires sont attendus.

## Node : autorité locale épinglée avant lecture, puis vrais 24 vecteurs

Root doit exécuter uniquement après terminalité réelle du dispatcher exit0 :

```sh
python3 /Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012/amd-rpc-continuation-audit-source-20261018/collect.py \
  1f20ae6cde3a0de220376b260395f86e1eef025306d5154b2889fbbe705440a6 8680
```

Ce SHA/taille de `UNIT/new-input-admission.json` vient explicitement de Root ;
l'auteur n'a pas lu ce fichier. Le collecteur futur le lit **localement**, sous
ce pin connu avant lecture, puis compare chaque ancien fichier sélectionné à sa
row originale sans rafraîchissement. Node doit figurer dans l'autorité. Les
trois modules source sont aussi liés à leurs rows originales du reçu public
JS1996 B et à la correction publique1592 B conservant les huit tests réellement
passés et le refus initial de reporter. Aucun des huit tests n'est rejoué.

Les nouveaux rapports locaux, le stdout réel proof et le driver sont admis
prospectivement par trois full9 identiques sur ≥2s, SHA/taille connus à chaque
échantillon. Les anciennes rows déjà held ne sont jamais remplacées. Le driver
`94b5a31b…` /3291 B et le module `b59224b4…` /5340 B restent leurs versions
qualifiées, avec account-proof et package.json originaux. Node25.9/ethers6.16
et leurs dépendances sélectionnées restent liés à l'autorité précédente.

La seule invocation Node, bornée60s, consomme
`outputs/test-candidate-proof.stdout.log` avec son SHA réellement transféré.
Elle exige30 JSONL,24 vecteurs, cinq rapports et le résumé, et vérifie les
preuves avec le module indépendant. Le résultat doit lier son inputSha256 à
ces mêmes octets et retourner24 résultats. Aucun RPC réel ni unité JS rejouée.
Une éventuelle égalité des octets avec Native/ARM est rapportée séparément ;
elle ne constitue pas le critère de validité des preuves.

## Première erreur, processus et clôtures

Le lecteur borne stdout/stderr pendant la vie de chaque enfant, suit sa sortie
et ferme ses pipes. Une erreur initiale reste première même si cleanup, capture
de diagnostic, clôture full9 ou publication échoue. Les seuls signaux éventuels
visent le propre nouveau groupe de processus, identité contrôlée ; jamais
PID28722. Aucun retry, redémarrage de campagne ou nouveau domaine n'est créé.

La clôture finale parcourt **toutes** les entrées held sans court-circuit. Les
refus secondaires restent distincts ; le résultat public est tenté même si un
audit précédent échoue. `verifiedProofVectors:24` nécessite aussi les clôtures
intactes ; sinon il reste0, et le nombre éventuellement rapporté par le vérifieur
est conservé séparément. Le Root doit encore joindre le processus du collecteur
et vérifier son vrai exit : un JSON seul ne prouve pas la sortie terminale.

## Préflight Root et limites

Avant usage, Root doit vérifier les pins du manifeste final, relire la source,
faire parser collect.py et le programme distant généré sur les seuls octets
publics, puis contrôler les gardes56/caps/firstfailure avec cas publics injectés
si nécessaires. Rien de cela n'est exécuté par l'auteur. Les autres sources,
produits, autorités et anciennes enveloppes restent intacts.

Les données privées des anciens/nouveaux produits sont attestées par le host
Root séparé ; ce collecteur transfère zéro fichier privé. Son autorité locale
Node est l'unique fichier d'autorité explicitement lu par Root, jamais exporté.
Ce n'est pas une fermeture globale SDK/loader, ni une authentification consensus
des racines, une nouvelle image runtime ou admission de campagne. C6 reste
séparée ; aucune promotion production, devoir/finalité,04, dépense ou lancement.
