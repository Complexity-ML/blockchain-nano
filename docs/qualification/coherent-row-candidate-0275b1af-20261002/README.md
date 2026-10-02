# Candidat cohérent 0275b1af — builds et sélection pertinente en attente

Fusion f9a3ee1a + main919b7998 ; **diff runtime src/include/network/third_party et deux tests modifiés vide** contre f9a3ee1a, aucun changement de la correction de sérialisation. Contrôleurs WAN main réunis sans promotion du runtime main. Git push branche qualification/checkpoint-row-dump-20261002 puis Git fetch/checkout détaché sur VPS, code non transféré par SCP.

Source Mac `/tmp/nano-checkpoint-row-dump-candidate-20261002`, VPS `/home/ubuntu/nano-checkpoint-row-dump-candidate-20261002` ; **15 313 fichiers / manifest SHA-256 26eb9c4befda4c41e7e4ede68e4137f18d6917e61bf548c789433c475c1b4d22 identique** observé sur les deux hôtes. Source propre. Aucune identité binaire ou qualification de ce nouvel ensemble encore déduite.

Neuf cibles CMake Release : nano, transition Capella, helpers SSZ hex/ascendance/JSON/hex, core et rotation audit. **Builds confirmés actifs** : ARM session33066, conteneur nano-coherent-0275b1af-arm64-build, un CPU/2 GiB ; AMD PID525910 et conteneur nano-coherent-0275b1af-amd64-build, deux CPU/3 GiB. Répertoires `/build/coherent-0275b1af-{arm64,amd64}-20261002` frais ; anciens répertoires qualifiés jamais écrits. Images/volume existants réutilisés, aucun build d'image, réseau none/root readonly/capdropALL/no-new-privileges ; scratch exec Linux distinct, /tmp noexec. Manifestes source avant/après et hashes des neuf programmes exigés.

Sélection **16 cas** préparée (15 parent + nouveau sérialiseur), exact registre/résultats, zéro skip et TIMEOUT originaux requis, neuf programmes/sept entrées statiques/source avant/après inchangés. Le helper f9a3ee1a partiel ne remplace pas les assertions intégrées des tests reconstitués. AMD contrôleur PID526676 confirmé en attente du build ; ARM session57995 attend à la fois build et ancienne régression complète ARM64 2d2764de pour éviter la concurrence des scénarios. Une interruption/échec build ne déclenche aucune relance : contrôle terminal bloque CTest. L'ancienne régression ne valide pas automatiquement le nouveau runtime, quelle que soit son issue.

Résultats attendus : Mac `/tmp/nano-coherent-0275b1af-arm64-{build,test}-reports-20261002`, VPS `/home/ubuntu/nano-coherent-0275b1af-amd64-{build,test}-reports-20261002`. Contrôles archivés, bash -n passé ; aucun résultat terminal de ces builds/tests acquis ici. Linux builder sans Node n'enregistre pas les contrôles Node facultatifs ; aucune réussite de ceux-ci attribuée à ces campagnes.

Ancien AMD14/15 et son timeout360,05 conservés. Réindexation originale360 s doit passer sur le candidat cohérent avant régression AMD64 complète/porte WAN. Mémoire du cache, reprise/journaux/finalité WAN, autres exigences de production et revue indépendante restent ouverts. Aucun fonds, vente, nouvel hôte ni exposition publique.

## Suite terminale ARM64

Build/controller/source-compare **0**, neuf programmes identifiés, sources avant/après identiques. Registre **426** (builder sans Node), sélection exacte de seize noms et TIMEOUT originaux vérifiée contre le registre. Rapports archivés. Aucun test de ce nouvel ensemble encore exécuté ; sélection ARM reste en attente de l'ancienne régression. AMD64 compilation encore active.

## Compilation AMD64 terminale et préparation native

Compilation AMD64 **0**, neuf programmes identifiés, sources avant/après inchangées et identiques au manifest ARM64 (**26eb9c4befda4c41e7e4ede68e4137f18d6917e61bf548c789433c475c1b4d22**). Registre **426**, présence exacte des seize noms sélectionnés et délais originaux vérifiés. Le conteneur `nano-coherent-0275b1af-amd64-test` est confirmé actif ; aucun résultat terminal de cette sélection encore acquis. Main et VPS ont récupéré `082ca290` par Git, sans promotion du runtime candidat.

Le propriétaire macOS natif requis pour la future reprise à trois nœuds doit utiliser le même nouveau runtime : compilation CMake neuf cibles active dans `/tmp/nano-checkpoint-row-dump-candidate-20261002/build-release`, session95937, deux tâches. Ses neuf contrôles core/helpers/contrôleurs, puis quatre audits signés en lecture seule du fixture historique, sont préparés et mis en attente (session13589), conditionnés à la réussite et l'intégrité du build. Ces audits ne prouveront ni de nouveaux duties ni une nouvelle finalité WAN. Contrôles archivés ; aucun résultat natif nouveau déduit ici. L'ancienne régression Docker ARM64 2d2764de et la sélection ARM64 en attente restent séparées du candidat.

## Résultats macOS natifs terminaux

Compilation neuf cibles **0**, source identique aux deux manifests Docker, P2P inchangé. Les **9/9** cas core/helpers/contrôleurs passent (**3,103274 s cumulées**, zéro skip, registre et TIMEOUT originaux exacts) ; **200** fichiers binaires/objets/archives/dépendances existantes + Node/P2P identiques avant/après, puis encore identiques après l'audit. Les bibliothèques système du cache dyld non matérialisées ne sont pas prétendues hashées.

**4/4** audits natifs réels passent : histoire signée **99 blocs / 97 transactions**, ascendance finalisée externe requise, checkpoint/digest/sequence exacts ; trois demandes invalides rejetées (head non finalisé, epoch hors bornes, root mal formée). Temps respectifs **1,950774 / 1,656257 / 1,602002 / 0,021908 s**, délai180 conservé. Enfant sandboxé `deny network*`/`deny file-write*`, fixture/source/binaire avant/après inchangés ; hash Nano **ceadd4ee2c4e03cb73a5a058cc7ed7ab42c627ed80ceac5387d7f815c4ac24c1**. Fixture historique identique au précédent, nouvelle qualification du runtime snapshot modifié ; aucune nouvelle finalité WAN ni reprise de duties. Aucun cycle validateur, fork réseau additionnel, qualification globale ou état production déduit de ce périmètre.
