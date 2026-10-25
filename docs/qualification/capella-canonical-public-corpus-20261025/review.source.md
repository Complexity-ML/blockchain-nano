# Revue SOURCE du runner de corpus

Avis : **GO borné au raccord du runner**, sous réserve du chargement/pinning du corpus, relu séparément. Aucun programme, test, compilateur, Git, Docker, SSH ou acteur privé exécuté/lu pour cet avis.

Sources relues :

- `source/tests/support/capella_canonical_stream_self_test.hpp` : SHA-256 `d3bfcbf2f9da1c0e5edf0450f62a17a30c9d92e290c4cfd9e99e111fd68cb1a9`.
- `source/tests/test_capella_canonical_stream.cpp` : SHA-256 `c9503ce7890d2db7f88ce2a83d6a0b8534e509b79adde92889d6c411a30040a7`.

Les quatre groupes et neuf refus Core restent présents : quatre refus seal/mutation, chaîne positive de 1 026 blocs signés, signature ancienne/manquant/sibling/annulation, puis refus de fenêtre sur les 255 blocs stalled. Les motifs précis des refus Core, les comparaisons d'exécution et de racines, les fenêtres 256/128 et l'annulation au huitième callback sont conservés. Les quatre refus d'entrée du corpus sont comptés séparément.

La sélection générée/chargée rejoint ensuite le même scénario ; `--generate-fixtures` du thin main conserve la génération monolithique historique. Aucun reset de `started` ou du compteur de signatures n'est ajouté. `phase_started` ne pilote que les métriques ; les écritures de diagnostic consomment le budget existant. Une fin de phase indique que l'appel est revenu, pas que ses assertions ultérieures ont réussi.

Le chargement doit encore garantir avant adoption : SHA/longueur littéraux externes, mode exact, bornes de lecture, ancre/configuration équivalentes à `fixture()`, attentes `(Block, stateRoot)` figées et aucun fallback de génération. Les quatre entrées négatives doivent refuser avant la construction de `fixture()` ; sinon le nombre de signatures bootstrap ne serait plus les quatre possibles des deux chargements positifs. Les statistiques du producteur restent sous `fixtureGenerationProvenance` ; elles ne mesurent pas cette exécution.

Changement de comportement visible : le caller historique `nano-capella-deposit-audit --fresh-stream-self-test` appelle sans argument et sélectionnera donc lui aussi le corpus. Le benchmark généré reste reproductible par `nano-capella-canonical-stream-tests --generate-fixtures` ; il ne faut pas attribuer sa sémantique historique à l'ancien flag sans le préciser.

Cet avis ne qualifie ni la génération ni le rejeu, et ne clôt pas l'ancien échec génération+rejeu à 300 s. Le coût Core précédemment mesuré après génération n'est pas une mesure de rejeu froid. La limite de 300 s/128 MiB et le succès du nouveau périmètre exigent une vraie exécution terminée par Root. Aucune performance de production, finalité réseau ou CI complète n'est démontrée ici.

## Delta rapport et CMake relu

**GO SOURCE borné au delta**, avec le précédent avis conservé ci-dessus. Header courant : `c59de11ba88ea02245d911c0aa757a3371e57282e7f2d0906d0ead699ddefd1c` ; thin main inchangé `c9503ce7890d2db7f88ce2a83d6a0b8534e509b79adde92889d6c411a30040a7` ; CMake `35e8b5a82c28e7f9fe34f641784eb7b106b4c090a3886e08e34d08e10595c9ba`.

Les cinq champs ajoutés sont des observations de l'`outcome` existant : racine de tête, hash du bloc d'exécution, racine de son état et deux compteurs d'ancêtres/d'historique de dépôts. La racine de tête est conservée dans une variable locale avant ses itérateurs. Ces champs ne modifient ni transition, sélection, assertion, callback ni limite, et les compteurs ne sont pas une mesure de RSS. Le commentaire distingue désormais correctement le défaut corpus du benchmark généré explicite.

CMake ajoute uniquement un producteur `nano-capella-canonical-corpus` avec `EXCLUDE_FROM_ALL`, le source exporteur, le même lien `nano_core`, les mêmes includes BLS et la définition `NANO_TEST_SOURCE`. Aucun `add_test` ni dépendance de génération n'est ajouté ; le CTest existant et son timeout 300 restent identiques. Le build direct du producteur peut construire ses dépendances selon l'état de CMake ; la réutilisation sans reconstruction du Core demeure la responsabilité du contrôleur Root, pas une propriété de `EXCLUDE_FROM_ALL`.

## Composition avec le loader GEN02

**GO SOURCE borné**, après lecture du helper complet puis de son unique correctif. Le runner courant `2ab177fba24d2a4f23d3b93a170e61de334902bdb9d72875c91bae9e6fba3fb5` remplace le chemin négatif par `temporary/missing/manifest.json` : le basename est correct et le refus atteint bien l'absence. Main/CMake restent aux pins précédents.

Le helper final relu est `canonical-stream-corpus-source-02/source/tests/support/capella_stream_public_corpus.hpp`, SHA-256 `3a08a4887b3fa1747350ac0f28d5dd357fd8d58347bf930c986a939e7b84c374`. Les SHA/bytes externes, le mode et le namespace sont contrôlés avant `fixture()` ; les quatre entrées négatives ne construisent donc aucun bootstrap. `from_generated` déplace la fixture existante. Le chargement des deux cas positifs ne régénère pas les histoires et peut créer quatre signatures bootstrap, plus la vérification Core du sibling sans signature nouvelle. Les oracles `(Block, stateRoot)` proviennent des octets du producteur épinglés avant cette admission, sans être reconstruits à partir du rejeu testé.

Le helper GEN01 `e4cb93bc…` avait un défaut précis de liaison du sibling : un root déclaré égal à l'ancre préexistante pouvait satisfaire `blocks.contains(root)` après admission. GEN02 refuse ce root avant bootstrap, exige son absence du store initial authentifié puis exactement un record ajouté, son root, son slot et son parent. Ce correctif ne change ni l'exporteur, ni `from_generated`, ni le lecteur fixe, ni les données du corpus ; il ne justifie aucune régénération. L'exporteur `b2bd0f05…` et le fixture reader `d3f53d44…` ont été lus ; aucun autre défaut concret bloquant n'a été identifié dans cette interaction.

Les pins littéraux de `capella_stream_public_corpus_pins.hpp` n'existaient pas encore lors de cette première composition. Son GO ne couvrait pas leur admission après export ni les vraies exécutions.

## Pins réels et cinquième refus d'entrée

**GO SOURCE du delta final** : runner `77e8a5dcca6e5bbde42bb5a69d4f1678f492cf38d0d280d1538a981c45567b29`, pins header `a2895872ddf9a6e94ef8291e6e0a31aca89d411ee5c436dc026b161238f1e79b`. Les littéraux correspondent aux deux manifests source : complete `1abc347450dcb50b8c3d0f9ffa4327bf351ec202a35e9ea974e98a0d760c5dfd`/196 B et stalled `fecfc2fdf55bf8b486d8f1751c81094665a35772f82c6e5258f7eefacbdaf63c`/194 B. Leurs body SHA/bytes correspondent aussi aux fichiers source : `5f19c991c0624884cda0651e10646f40a6ff55b4f07c3a7da9b88f78fa3f3447`/4 832 583 B et `26f35b171ee65a6060010f0359e01cb4f8eefcf8e0f789b9e7fff79f9838edcf`/965 801 B.

Le nouveau cas copie les octets publics authentifiés dans un répertoire owned neuf, change seulement `sibling.root` pour l'ancre puis réépingle intentionnellement cette copie. Il exige le motif exact `public corpus distinct same-slot sibling` et le compteur de signatures inchangé. Les refus d'entrée passent donc de quatre à cinq, séparément des neuf refus Core ; les deux chargements positifs peuvent toujours créer quatre signatures bootstrap. Les transitions, fenêtres, oracles et limites précédemment relus ne changent pas. Aucune génération ni exécution de replay n'a été effectuée par ce reviewer ; la provenance effective des exports et les terminaux réels restent la preuve Root.
