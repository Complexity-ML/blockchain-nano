# Assertoor exécuté contre Nano natif — 2026-09-26

Le binaire officiel Assertoor, sans modification de ses sources, a exécuté avec
succès le scénario `nano-native-observation` contre deux processus Nano et deux
démons P2P locaux. Aucune émulation de réponses, aucun contrôle EL/CL désactivé.
Les deux URL d'un participant désignent le même binaire monolithique Nano.

## Preuves du premier passage

- Test englobant : 25,50 secondes, incluant synchronisation et redémarrage natif.
- Santé : **2/2 clients**, consensus et exécution, aucun client exclu.
- Synchronisation : progression du slot 3 au slot 4, distance zéro sur les deux.
- Propositions : deux corps de blocs nouveaux observés, slots 4 et 5, en-têtes,
  signatures et payloads conservés dans `run-1.json`.
- API Assertoor : deux clients online/ready, même racine Beacon et même hash EVM
  au slot/bloc 5 (`clients.json`). Nano garde son nom et son type inconnu (-1).
- Le test natif redémarre ensuite le validateur et vérifie la propagation du bloc
  6 ; cette dernière vérification appartient au harnais C++, pas à Assertoor.

`runs.json`, `run-1.json`, `clients.json`, `assertoor.log` et `first-run.log`
conservent les résultats complets. Le seul avertissement du premier passage
concerne l'API de contrôle d'Assertoor sans authentification, liée à loopback
pour ce test temporaire. Aucun jeton interne Nano n'est transmis à Assertoor.

## Passage de la cible CTest dédiée

`consensus_bellatrix_assertoor` passe en **32,83 s** avec validation explicite
que les trois tâches ont démarré, se sont terminées et ont réussi. Les preuves
préfixées `ctest-` et `ctest.log` conservent ce second passage de la cible enregistrée.

## Reproduction

Sources Assertoor : commit `89366be6cb211522249a6d9424adb47631864432`.
Compilation Go 1.26.2 darwin/arm64 ; sonde Node 25.9.0.
SHA256 du binaire compilé :
`cfa9351511b48b0bb9565dea624f1cc01c6278ac267e5ec6bd1ae54ca9f5aef5`.

```sh
# Depuis la copie du dépôt officiel au commit indiqué :
cd .deps/assertoor
go build -o ../assertoor-bin .
cd ../..
cmake -S . -B build-cmake \
  -DNANO_ASSERTOOR_EXECUTABLE="$PWD/.deps/assertoor-bin"
cmake --build build-cmake --target nano nano-bellatrix-transition-tests -j4
ctest --test-dir build-cmake -R '^consensus_bellatrix_assertoor$' --output-on-failure
```

Le build réseau doit déjà fournir `NANO_NETWORK_EXECUTABLE`. La cible CTest
Assertoor est optionnelle et n'est enregistrée que si ce binaire externe est
explicitement configuré. Node doit être disponible dans PATH.
`tests/assertoor/smoke.json` définit les trois tâches. La sonde garde ses preuves
dans un répertoire temporaire annoncé, refuse tâches manquantes/ignorées/échouées,
attend au plus 100 secondes et arrête Assertoor. Les nœuds et démons P2P sont
arrêtés par le harnais C++ à la fin. Aucun flood ni fonds réels.

## Limites et prochaine priorité

Ce passage qualifie l'observation de base par **Assertoor lui-même**, et remplace
le statut « SDK seulement ». Il ne qualifie pas son catalogue complet :

- Les 64 clés synthétiques appartiennent à un seul validateur processus ;
  l'autre nœud est observateur, sur le même Mac.
- Les blocs observés ici sont sans transactions ; pas de nouvelle preuve DeFi.
- Pas de finalité non nulle, de partition, de perte de quorum, ni de crash forcé
  dans les tâches Assertoor de cette campagne courte.
- Prochaine extension : tâche `check_consensus_finality` avec minimum d'epochs
  finalisés non nul, puis perte/rétablissement du quorum sur deux validateurs.
- REST Beacon reste partiel ; autres topics SSE et cycle des validateurs ouverts.
- Pas de nouvelle EIP activée, pas de qualification de production complète.

Référence : [Assertoor officiel](https://github.com/ethpandaops/assertoor/tree/89366be6cb211522249a6d9424adb47631864432).
