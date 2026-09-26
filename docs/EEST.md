# Qualification Nano avec Ethereum Execution Spec Tests

Le parcours `consume direct` officiel sélectionne les fixtures et publie les
résultats pytest. Le plugin `tests/eest/nano_consumer.py` enregistre Nano sous son
propre nom et appelle les runners C++ liés à `nano_core`. Il ne substitue ni Geth,
ni Anvil, ni l'interpréteur de référence au moteur testé. Le Python appartient au
banc EEST, pas au binaire ni au déploiement du nœud.

Versions épinglées dans `dependencies.lock.json` : framework EEST
`10eaa63d5da2f50b63d4359968f36542212f9f50`, corpus stable `v5.4.0`.
L'empreinte SHA-256 du corpus a été comparée au digest publié par GitHub :
`92cf1b47ad12fb27163261fc3c1cea5df72439cab507983d06b56c94f8741909`.

## Préparation

Depuis la racine du dépôt, avec CMake, Git, uv, curl et tar disponibles :

```sh
git clone https://github.com/ethereum/execution-spec-tests.git .deps/execution-spec-tests
git -C .deps/execution-spec-tests checkout 10eaa63d5da2f50b63d4359968f36542212f9f50
(cd .deps/execution-spec-tests && uv sync --frozen)
cmake --build build-release --target nano-ethereum-state-runner nano-ethereum-block-runner -j 4
curl -fL https://github.com/ethereum/execution-spec-tests/releases/download/v5.4.0/fixtures_stable.tar.gz -o /tmp/nano-eest-v5.4.0-stable.tar.gz
printf '%s\n' '92cf1b47ad12fb27163261fc3c1cea5df72439cab507983d06b56c94f8741909  /tmp/nano-eest-v5.4.0-stable.tar.gz' | shasum -a 256 -c -
mkdir -p .deps/eest-v5.4.0
tar -xzf /tmp/nano-eest-v5.4.0-stable.tar.gz -C .deps/eest-v5.4.0 fixtures/.meta fixtures/state_tests fixtures/blockchain_tests
```

Le corpus extrait occupe environ 2,35 Go. L'index conserve aussi les références
aux formats Engine API et transaction : ils ne sont pas annoncés comme supportés
par ce consumer et leurs fichiers ne sont pas extraits par cette commande.

## Exécution

```sh
mkdir -p build-cmake/qualification/eest
scripts/run-eest.sh --input "$PWD/.deps/eest-v5.4.0/fixtures" \
  -m 'state_test and (Paris or Shanghai or Cancun)' -n 2 --maxfail=30 \
  --tb=short --no-html --json-report \
  --json-report-file="$PWD/build-cmake/qualification/eest/report.json" \
  --dump-dir="$PWD/build-cmake/qualification/eest/cases"
```

Enlever `--maxfail=30` pour une exécution sans arrêt anticipé sur échecs. Le nombre
sélectionné n'est pas le nombre exécuté. Un arrêt anticipé laisse les cas restants
non exécutés ; les forks et formats désélectionnés ne sont pas qualifiés.
`-m 'blockchain_test and Paris'` permet une campagne de blocs distincte.
`NANO_EEST_ROOT` et `NANO_EEST_BUILD` remplacent les chemins par défaut.

Chaque appel passe un identifiant exact avec `--case`. Les erreurs, divergences,
formats ou forks non pris en charge, timeout de 120 secondes par cas, et absence
de cas exécuté font échouer le test. Les sorties JSONL de Nano sont conservées
avec la commande dans `--dump-dir`, sous un sous-dossier identifié par le hash
du chemin et de l’identifiant complet (EEST peut tronquer ses noms de dossiers). Les traces opcode ne sont pas encore exposées.

Contrôles négatifs de l'adaptateur :

```sh
PYTHONPATH="$PWD/tests/eest" .deps/execution-spec-tests/.venv/bin/python \
  -m pytest tests/eest/test_consumer_contract.py -q
```

Ils exigent un succès sur un vecteur officiel ECRECOVER, puis un échec pour une
fausse racine, un identifiant absent et un fork non implémenté.

## Limites de preuve

Les tests d'état comparent la racine d'état, les logs, le hash de transaction,
les erreurs de validation et les champs de reçu disponibles dans la fixture.
Les tests de blocs passent par l'exécution C++ et contrôlent les engagements
exposés par le runner ; celui-ci ne revendique pas la validation complète de tous
les en-têtes et du consensus. Ce parcours ne teste pas RPC/P2P, la finalité Beacon,
les pannes disque, la reprise distribuée, ni les activations réseau. Il ne donne
pas, à lui seul, une qualification de production.

Sources : [EEST](https://github.com/ethereum/execution-spec-tests),
[corpus v5.4.0](https://github.com/ethereum/execution-spec-tests/releases/tag/v5.4.0).
