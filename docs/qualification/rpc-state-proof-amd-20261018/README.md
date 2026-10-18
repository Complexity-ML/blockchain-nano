# RPC AMD64 : continuation C6 et vérification indépendante des preuves

Le 18 octobre 2026, Root a terminé la continuation AMD64 avec quatre tests C++, neuf contrôles d’arité/aide sur huit exécutables retenus, puis une vérification Node indépendante des 24 vecteurs de preuve effectivement produits. La [sortie terminale du dispatcher](evidence/continuation-dispatch/dispatch-exit-public.json), le [résultat de l’hôte](evidence/continuation/result-public.json) et le [résultat indépendant](evidence/independent-proof/result-public.json) sont trois reçus distincts. Les échecs antérieurs restent inchangés dans ce dossier.

Le code C++ compilé et identifié est le candidat `c6cdf780c68eab8b06306d76257bda9028928ea3` (C6). Le main observé lors de la qualification est `4388f77b9e9c1b3b94ba5da6f3e486eaa13a3f5c` ; C6 n’y est pas encore importée dans ces reçus. Le futur commit de publication/import n’est pas connu ici. Cette livraison ajoute des preuves et des textes proposés ; elle ne modifie aucun C++, CI, index runtime, admission de campagne ou ancien résultat.

## Résultats réels et réemploi limité

Le [premier lot AMD](history/failed-build-85431/root-failure-audit-public.json), dispatcher 85431, a échoué par expiration du budget de 1800 secondes au premier CXX du test de preuve. Il avait achevé 143 scans Core, 44 compilations Core, une archive, huit assemblages runtime avec liens/ldd et un scan supplémentaire du test de preuve : 152 M, 52 CXX achevés, huit liens et huit ldd. Aucun test ni CLI n’avait tourné. L’interruption n’établit pas un diagnostic d’échec sémantique du compilateur. Le contrôle post-cleanup a vérifié les 265 produits d’origine sans changement. Ce lot global reste FAILED.

La [continuation 99111](evidence/continuation-dispatch/dispatch-exit-public.json) a terminé avec exit 0 en 654,819 secondes. Elle a conservé ce Core et ces huit exécutables, sans recommencer les scans/compilations Core, l’archive ou les assemblages runtime. Son [worker](evidence/continuation/outputs/result-public.json) a effectué uniquement quatre M, quatre CXX, quatre liens, quatre ldd et quatre tests ; les quatre comparaisons M/MD normalisées passent. L’hôte a ensuite exécuté les neuf CLI une première fois : `nano []`, `nano --help`, puis `[]` pour `checkpointExporter`, `bootstrapImport`, `bootstrapAudit`, `stoppedCapture`, `depositAudit`, `slashingFixture` et `lifecycleSubmit`. Les exits attendus de refus d’arité sont qualifiés par leurs sorties exactes ; neuf CLI passées ne signifie pas neuf exits zéro.

| Test candidat | Oracle public observé |
| --- | --- |
| `candidate-proof` | 30 lignes JSON : 24 vecteurs, cinq rapports et un résumé de cinq groupes, 1619 checks, 63 refus attendus |
| `candidate-cost` | sept checks, quatre refus ; état synthétique de 128 comptes × 16 slots, annulation et RPC peu coûteux pendant la pause |
| `candidate-state-readers` | cinq groupes, 650 checks, 179 refus attendus |
| `candidate-legacy-rpc` | dernière ligne `1053 RPC checks passed` |

Chaque cas conserve séparément son reçu, son exit, stdout et stderr dans `evidence/continuation/outputs/`. Les quatre stdout sont les sorties réelles des tests, pas des sorties générées depuis un résultat candidat. Les définitions d’oracle préexistantes sont archivées dans [test-batch.json](source/closure/test-batch.json) et [runtime-cases.json](source/closure/runtime-cases.json).

Les totaux bruts des deux exécutions sont 156 M, 56 CXX achevés, une archive, 12 liens/ldd, quatre tests et neuf CLI. Le total M inclut l’ancien scan de preuve avant le CXX interrompu ; il ne faut pas le remplacer par le budget prospectif de 155 M. La continuation réussie ne transforme pas le premier lot en succès.

## Core, produits et identités

Le Core retenu mesure 40 988 476 octets, SHA-256 `3ef6c2cdd75c793b8cec88f3268ab0309e2ebc30aba3be86c1fb9125b13dc652`. La [preuve d’archive initiale](history/failed-build-85431/outputs/core-member-proof.json), également jointe au résultat hôte de continuation, conserve les 143 membres dans le même ordre : 44 membres sélectionnés, 99 membres hors sélection inchangés et huit corps effectivement différents. Huit corps changés sur AMD ne remplace pas le décompte distinct du Core Native. Le parseur d’archive ne revendique pas une validation sémantique des tables de symboles.

Le [résultat hôte](evidence/continuation/result-public.json) et sa [clôture finale](evidence/continuation/host-final-fence-public.json) attestent les 265 lignes de produits d’origine et les 16 nouveaux produits des quatre tests. Les 16 lignes M/objet/MD/exécutable sont jointes à l’autorité du worker avant les neuf CLI, puis tenues jusqu’aux fences après cleanup et vérification finale de l’image. L’objet de preuve interrompu du premier lot n’est pas réemployé. Le conteneur possédé est absent à la clôture ; aucune image ni volume n’a été purgé.

L’autorité JSON de contenu `866e3f3d…` n’avait pas sa propre ligne physique historique disponible. La [préparation](evidence/staging/result-public.json) distingue explicitement son ancien pin d’octets de sa nouvelle admission physique : `authority866HistoricalFD9Verified:false` et `authority866NewInputAdmissionOnly:true`. Cela n’invente pas une ancienne métadonnée pour ce JSON et ne remplace aucune des 265 lignes de produits originales qu’il contient. Les observations de collecte du catalogue sont également distinctes des autorités d’origine.

## Vérification indépendante des 24 sorties effectives

Le [reçu indépendant Root 28073](evidence/independent-proof/result-public.json) indique exit Node 0, processus rejoint et 24 vecteurs vérifiés. Le [reçu du processus](evidence/independent-proof/independent-proof.exit-public.json) donne 320 ms, aucun timeout, stdout 4765 octets et stderr vide. Le contrôle mécanique des 56 fichiers publics, des quatre tests et des neuf CLI a été refait hors ligne ; aucun SSH, CXX, test C++ ni ancien groupe de huit unités JavaScript n’a été rejoué.

Le flux [test-candidate-proof.stdout.log](evidence/continuation/outputs/test-candidate-proof.stdout.log) mesure 41 688 octets, SHA-256 `f8c059c07a766da1afc84e9c557e9167c64a5336786f0334f906a9e777241e94`. Ses octets sont identiques aux flux Native et ARM déjà obtenus ; la vérification indépendante porte bien sur cette copie AMD. Le [pilote](source/closure/verify-rpc-proof-vectors.mjs) et les [vérificateurs compte/storage](source/closure/tests/qualification/rpc-proof.mjs) vérifient inclusion/exclusion, existence et valeurs attendues contre les racines fournies par les tests C++.

L’[admission hors ligne](evidence/independent-proof/admission-summary-public.json) conserve six lignes originales et admet 72 nouvelles entrées aux pins d’octets connus. La [clôture](evidence/independent-proof/final-input-fence-public.json) vérifie les 78 fichiers sans rafraîchir les anciennes lignes. Le résultat C++ conserve historiquement `rootIndependentProofVectorsVerified:false` ; le reçu indépendant ultérieur est ajouté séparément.

## Échecs et propositions conservés

La chronologie ne retire aucun refus :

1. Le [lot initial 85431](history/failed-build-85431/root-failure-audit-public.json) reste FAILED pour budget ; sa preuve Core et ses produits achevés sont seulement réemployés sous leurs identités originales.
2. La [proposition de continuation V1](source/continuation-v1/manifest.json) reste archivée. La V2 [source](source/continuation-v2/manifest.json) ferme avant exécution le manque de fence des 16 produits frais après les CLI.
3. Le [refus préstart du manifest](history/refusals/amd-rpc-continuation-staging-prestart-manifest-refusal-root-20261018.json) précède namespace et SSH. Il concerne le pin de manifest avant/après gel final ; les octets des cinq payloads n’avaient pas changé. Le [manifest précédemment relu](history/refusals/amd-rpc-continuation-reviewed-manifest-before-freeze-root-20261018.json) est conservé. Ce refus n’est pas présenté comme une mutation pendant une tenue déjà acquise.
4. Le [collecteur V1](history/refusals/amd-rpc-continuation-collector-source-origin-join-refusal-root-20261018.json) refuse avant SSH/Node : il joignait les lignes des origines aux copies projetées. Les octets des quatre copies correspondaient ; aucune mutation récente du module n’est établie et aucune ancienne FD9 de ces copies n’est inventée.
5. Le [collecteur V2](history/refusals/amd-rpc-continuation-collector-new-input-admission-refusal-root-20261018.json) lit et valide les 56 fichiers puis refuse avant Node sur la stabilité de nouveaux inputs. Ses échantillons en échec n’ont pas été conservés ; le champ divergent et l’acteur restent inconnus. Son [root-audit-public.json](evidence/collector-v2/root-audit-public.json) reste en échec, sans requalification par le succès hors ligne ultérieur.
6. Les [trois sources de wrapper hors ligne](source/offline-wrappers/) et leurs [refus de préparation](history/refusals/amd-rpc-offline-proof-source-prestart-review-root-20261018.json) sont conservés. Les deux premiers prédécesseurs n’ont pas été exécutés. La [source finale figée](history/refusals/amd-rpc-offline-proof-final-source-freeze-root-20261018.json) est `d611b329…`, effectivement utilisée pour Root 28073.

## Portée et publication proposée

EIP-1186 est qualifiée localement sur les profils Paris et Shanghai par ces scénarios, avec les quatre lecteurs EIP-1898, la capture/racine bornée, l’annulation et la rétention/reorg exercés par les tests. Cela ne revendique pas tous les états orphelins ni tous les profils/forks. Les parcours O(N), budgets logiques, attente non préemptible des mutex/I/O et annulation coopérative restent les limites documentées. Les racines MPT sont engagées dans les headers d’exécution ; le vérificateur indépendant n’authentifie pas encore un checkpoint finalisé par le consensus.

Les CLI ne lancent aucun service `main/serve`, P2P, duty ou campagne. Les listeners HTTP/WS synthétiques des tests sont inclus dans leur portée locale ; le HTTP du vrai main Native/AMD reste non qualifié. Le SDK entier, le graphe CMake global, tous les chargeurs, la finalité réseau, CI globale et la production ne sont pas promus. Les qualifications Native/ARM et leurs échecs restent des preuves séparées. Campagne 03 reste FAILED et consommée ; aucune campagne 04 n’est autorisée par cette livraison. Les autres critères L1, exploitation, audit externe, Base–Nano/bridge et fair launch Doppler restent ouverts.

Le [MANIFEST.json](MANIFEST.json) inventorie uniquement des copies publiques exactes et ce README. Les scripts archivés gardent leurs chemins et pins historiques ; leur présence n’est pas une promesse d’exécution directe depuis ce répertoire. Aucune autorité privée, SDK, clé, binaire ou `node_modules` n’est copié. Les quatre appendices proposés sont composés par Root après les corps documentaires actuels, avec hashes avant/après ; aucun corps historique n’est réécrit. L’auteur du paquet n’a exécuté aucun test ou opération et n’a modifié aucun fichier du dépôt.
