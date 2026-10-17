# Première admission des nouvelles sorties connues

Proposition SOURCE ONLY. Aucun programme de projet, test, campagne, processus, réseau, SDK, capsule ou entrée privée n’a été exécuté ou lu par l’auteur. Root doit contrôler et qualifier ce lot avant tout usage. Aucun fichier de R n’est modifié.

## Correctif V2 de syntaxe du fixture

Root a exécuté la tentative01 avec le guard84b671. Node25 a refusé le parseur avant tout corps de test : une fermeture `;}` du premier bloc if manquait après le retour de l’objet fake open. Aucun test causal ni groupe candidat n’a été exécuté. Le reçu public `campaign-new-output-admission-root-20261017-01/baseline-parser-refusal-public.json` et le manifest45948 original restent conservés. V2 ajoute seulement ces deux caractères dans le test ; les produits run1af106a et helper5806 restent byte-exacts. La qualification baseline puis candidat reste à réaliser par Root sur de nouvelles projections. Aucun Node ou test n’a été exécuté par l’auteur.

## Incident et cause limitée

Root a observé la campagne03 terminale exit1, après acceptation de la sortie volontaire et progression de finalité commune jusqu’à87. Cela ne qualifie ni le cycle complet ni la production. Le refus est survenu avant préparation/soumission du proposer slashing, au `save` de la première capsule mac-native. L’état consommé et les journaux restent conservés par Root ; cette proposition n’autorise aucune reprise de campagne ni nouveau domaine.

Dans la baseline `run.mjs`, `save` crée exclusivement (`wx`,0600), écrit les octets connus, fsync le fichier et son parent, puis appelle immédiatement `observe`. Le refus intra-FD/path des neuf champs survient avant le contrôle SHA/taille et avant `newFiles.set`. Ce fichier n’est alors ni une autorité held ni une sortie déjà admise. La trace identifie la seule différence ctimeNs. Elle ne prouve pas l’acteur du changement, ni que les octets originaux ont été vérifiés lors de la lecture refusée. Le diagnostic public Root ultérieur (`campaign03-failed-first-save-readonly-diagnostic-root-20261017.json`) montre trois lectures cohérentes maintenant, sans publier le SHA original attendu et sans attribution de cause. Aucune autorité ancienne n’est récupérée depuis cette observation.

## Modification

`save` refuse immédiatement tout chemin déjà dans held ou newFiles. Il copie le Buffer fourni avant tout await, borne les octets, garde la création exclusive et les fsync existants, puis capture les neuf champs du FD de création après écriture/fsync. Le nouveau helper `admitNewLifecycleOutput` est appelé uniquement pour cette sortie nouvellement créée.

Chaque lecture séparée exige chemin canonique, NOFOLLOW, fichier régulier, un seul lien, taille exacte, SHA calculé sur les octets lus égal au SHA du buffer connu, et les huit champs de création dev/ino/mode/uid/gid/nlink/size/mtimeNs inchangés. Les neuf champs FD avant/après/path sont comparés strictement. Si seul ctime rend l’échantillon incohérent, celui-ci est rejeté et ne compte pas ; une prochaine lecture peut commencer une nouvelle série avant toute admission. Une modification entre deux lectures stables remet également la série à zéro. Aucun autre champ, alias, changement d’inode, SHA/taille différente ou erreur de lecture n’est retenté.

Il faut trois échantillons stricts et identiques sur au moins2000ms avant `newFiles.set`. Le délai coopératif de30000ms est fixe, avec une attente1000ms entre lectures ; pas de nouvelle lecture une fois la limite atteinte. Une lecture OS peut dépasser le délai ; son résultat est refusé après retour. Il n’y a pas de timer qui interrompt un appel système. Les vérifications held/newFiles sont répétées avant/après chaque lecture, après chaque attente et immédiatement après le await du helper avant insertion en map. Les anciens `observe`, `fence`, règles de restauration, signatures, finalité et reçus restent inchangés. La map newFiles garde son format SHA/taille ; ce lot n’ajoute pas une nouvelle politique de conservation des neuf champs de toutes les sorties.

Cette stabilisation ajoute au minimum deux secondes à CHAQUE appel `save`, y compris publications intermédiaires et finales. Les budgets existants de campagne/actions/finalité restent inchangés ; ils peuvent donc refuser si ce coût supplémentaire les épuise. Ce lot ne démontre pas la tenue des délais d’une campagne complète.

## Régressions injectées, à qualifier par Root

Les cinq groupes `new output admission:` sont dans `lifecycle-driver-guard.test.mjs`, déjà enregistré par CTest. Aucun nouveau fichier test ni changement CMake. Le fichier `lifecycle-runtime.test.mjs` et son compteur historique after55 restent byte-exacts et ne sont pas requis pour la sélection de ces nouveaux groupes.

Le fixture extrait textuellement les vrais corps `save` ET `observe` du `run.mjs` de la phase testée, sans importer/exécuter le driver. Il injecte uniquement FD/stat/read/write/fsync, séries d’observation et temps ; aucun fichier de campagne, aucune vraie écriture FS, aucun processus ni réseau. Les variantes du helper utilisent un SHA connu et les neuf champs sous forme de chaînes ; le temps simulé ne retarde pas CTest.

1. Causal : premier échantillon ctime FD200→201, puis trois lectures strictes201. Avec le `run` et module parents, le vrai prédicat `observe` refuse avec `Public input changed during FD read`. Le candidat doit retourner les octets connus après quatre lectures/3000ms, avec sync fichier/parent avant toute lecture et sans held ajouté.
2. Changement ctime entre lectures : recommencer la série ; cinq lectures/4000ms. Chemin sans perturbation : trois lectures/2000ms.
3. Dix mutations : SHA, taille et chacun des huit champs de création ; refus immédiat à la première lecture, sans admission, même avec une dérive ctime simultanée.
4. Sept rétentions : held/newFiles déjà présents avant ouverture, présents pendant première lecture, held ajouté pendant attente ou held/newFiles posé à la résolution du helper juste avant la continuation save ; aucune ligne retenue n’est remplacée.
5. Ctime perpétuellement instable :30 lectures, refus à29000ms avant une attente qui atteindrait la limite ; erreur FD/no-follow injectée propagée par identité, sans retry.

Plan Root causal : mettre le NOUVEAU fichier de test à côté des `run.mjs`/module parents et sélectionner seulement `^new output admission: actual save` (un FAIL attendu avec le message précis, pas erreur d’import/parser). Puis utiliser les trois sources candidates et sélectionner `^new output admission:` (cinq PASS attendus). Le guard existant peut être exécuté une fois sur candidat pour vérifier ses deux pins modifiés et les refus avant seed/output. Les anciens55 ne sont pas rejoués par ce plan et ne sont pas affaiblis. Les résultats ne sont pas acquis par la présente lecture.

Limites : ces injections ne reproduisent pas l’acteur du changement ctime, le cache d’un FS, une capsule réelle, un arrêt/restart, une signature, un audit C++, une progression de duties ou la clôture d’un cycle. La première admission réelle sur ce chemin est encore à qualifier séparément par Root. Les anciens échecs restent des échecs.
