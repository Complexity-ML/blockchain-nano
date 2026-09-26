# Dimensionnement Python contre mint Uniswap V3 sur Nano

Binaire Nano de 42a3b131a672b9a13eedf08872f18359a8af1c76, empreinte jointe.
Profil Paris, réseau local temporaire, WNANO et quote synthétique six décimales.
Bytecodes officiels core 1.0.1 et periphery 1.4.4 déjà épinglés au lock npm.
Aucun fonds réel. Le correctif C++ d'expiration non publié n'est pas inclus dans
ce binaire EVM : cette expérience ne le qualifie pas.

Le script graduation-v3.ts lit le prix réel slot0 et le tickSpacing, appelle
le simulateur Python, puis passe les montants calculés au PositionManager.
Il compare exactement la liquidité et les montants de l'événement IncreaseLiquidity,
les variations de soldes, les bornes et la liquidité de la position/pool.
Le sqrtPriceX96 et le tick restent identiques après le mint.

Ce test porte sur une nouvelle position active, sans marché concurrent ni swaps.
Il ne qualifie pas le verrouillage des NFT, le contrat de vente, le bridge,
les positions à un seul actif, le dépassement de capacité par tick ou une
migration publique. Le budget d'un quote synthétique n'est pas de l'USDC réel.

Commande :
```
NANO_TEST_EVM_FORK=Paris NANO_BINARY="$PWD/build-release/nano" \
 node tests/hardhat/run-local.mjs scripts/graduation-v3.ts
```
L'option NANO_TEST_REVERSE_ASSETS=1 inverse l'ordre de déploiement des actifs.

Résultats : les deux ordres passent (codes 0). Quote token0 : liquidité
11393569353068, allocation 112944827289606681 unités NANO. Quote token1 :
10311611406682 et 92729794397778112 unités. Les plages sont centrées par arrondi
au spacing dans chaque orientation, donc elles ne sont pas des plages économiques
exactement réciproques ; ces allocations ne doivent pas être égales.
