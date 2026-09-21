# Wrapped Nano (WNANO)

`WrappedNano.sol` transforme le NANO natif en ERC-20 pour son utilisation dans
les contrats de pool. Un dépôt d'une unité native crée une unité WNANO ; un
retrait brûle les unités du demandeur et lui restitue le même montant natif.
Les décimales sont 18. Ce contrat ne crée pas de cotation contre un autre actif
et ne transfère rien entre chaînes : il ne constitue ni une pool ni un bridge.

Le contrat utilise ERC20 et ReentrancyGuard d'OpenZeppelin 5.0.2. Il n'a ni
propriétaire, ni proxy, ni création administrative de jetons, ni retrait
administrateur. Un échec du transfert natif annule aussi la destruction des
WNANO. Une arrivée forcée de fonds peut produire un surplus : l'invariant est
`solde natif >= totalSupply`, pas nécessairement une égalité. Aucun mécanisme
administrateur de récupération de ce surplus n'est prévu.

## Compilation et tests locaux

Depuis la racine du dépôt :

```sh
npm ci --prefix tests/hardhat --ignore-scripts
node scripts/build-wrapped-nano.mjs
NANO_TEST_EVM_FORK=Shanghai NANO_BINARY="$PWD/build-cmake/nano" \
  node tests/hardhat/run-local.mjs scripts/wrapped-nano.ts
```

Le compilateur solc 0.8.28 produit du bytecode Shanghai avec optimisation à
200 passages. `WrappedNano.json` contient ABI, bytecodes, paramètres et hashes
du source et des dépendances importées. Un réseau Nano doit sélectionner
Shanghai pour ce bytecode ; le test utilise un genesis temporaire, pas celui
du mainnet livré. Les clés des tests sont publiques et n'utilisent aucun fonds réel.

Le parcours Hardhat vérifie les deux formes de dépôt, transferts, autorisations,
retraits, absence de retrait au-delà du solde, remboursement refusé dans une
transaction effectivement exécutée, tentative de réentrance effectivement
déclenchée, restitution intégrale et certificats BFT. Ces scénarios ne sont
pas une preuve exhaustive. Le contrat n'est déployé sur aucun réseau public
par cette procédure et son intégration à une pool reste à réaliser.

Exécution vérifiée : le parcours Hardhat WNANO passe sur Nano Shanghai, ainsi
que les 41 tests CTest après reconstruction (148,17 secondes). Le parcours
Hardhat est distinct de ces 41 tests. Deux compilations successives de WNANO
dans le même environnement produisent le même SHA-256 d'artefact.
