# Pool WNANO : intégration Uniswap V2 sur Nano

Le banc Hardhat utilise les bytecodes distribués par `@uniswap/v2-core@1.0.1`
et `@uniswap/v2-periphery@1.1.0-beta.0`, avec versions exactes et intégrités npm
dans `tests/hardhat/package-lock.json`. WNANO est le contrat Nano ; TESTQ est
un ERC-20 OpenZeppelin à six décimales créé uniquement pour les tests, sans
adossement monétaire. Les sources Uniswap sont sous licence GPL-3.0 et restent
dans leurs paquets d'origine. Référence : https://github.com/Uniswap/v2-core.

## Parcours vérifiés

`scripts/pool.ts` déploie la factory et une paire avec 100 WNANO et 1 000 TESTQ.
Le ratio initial est 10 TESTQ/NANO. L'achat avec 10 TESTQ produit le montant
calculé avec les frais V2 de 0,3 %, et le ratio devient 10,200696 TESTQ/NANO
(valeur tronquée à six décimales). Il ne s'agit pas d'une cotation en dollars.
Le test vérifie les réserves, les soldes, le produit des réserves, le rejet
d'un swap sans paiement dans une transaction exécutée, le remboursement de
WNANO en NANO et le retrait proportionnel des parts LP. Les 1 000 unités LP
minimales verrouillées par Uniswap restent en circulation.

Ce premier script appelle directement les primitives de paire en plusieurs
transactions. Il sert au diagnostic local et ne doit pas être utilisé comme
parcours public : transférer des tokens à une paire avant mint/swap/burn dans
une autre transaction expose ces tokens à l'utilisation par un tiers.

`scripts/pool-router.ts` exerce le routeur officiel Router02 avec WNANO comme
actif natif enveloppé. Les noms des fonctions upstream contiennent ETH, mais
les valeurs natives sont des NANO sur ce réseau. Il vérifie le dépôt atomique,
le rejet d'un montant minimal de sortie trop élevé, le rejet d'une échéance
expirée, l'échange TESTQ contre NANO natif et le retrait atomique vers un
destinataire autorisé par le détenteur des LP. Les montants natifs sont comparés
exactement ; le destinataire de retrait est distinct du validateur de test pour
ne pas confondre remboursement et récompenses de consensus. Le hash de création
de la paire est contrôlé contre celui attendu par le routeur.

## Exécuter localement

```sh
npm ci --prefix tests/hardhat --ignore-scripts
node scripts/build-wrapped-nano.mjs
NANO_TEST_EVM_FORK=Shanghai NANO_BINARY="$PWD/build-cmake/nano" \
  node tests/hardhat/run-local.mjs scripts/pool.ts
NANO_TEST_EVM_FORK=Shanghai NANO_BINARY="$PWD/build-cmake/nano" \
  node tests/hardhat/run-local.mjs scripts/pool-router.ts
```

Ces scripts créent un nœud temporaire, un genesis et des clés publiques de test.
Ils ne déploient aucun contrat sur le mainnet ou le testnet public. La factory
de test désactive irréversiblement l'administration des frais de protocole en
choisissant l'adresse zéro ; ce choix n'est pas une décision de lancement.

## Travail restant pour la pool personnelle

Choisir l'actif réel de contrepartie et, s'il vient d'une autre chaîne, son
mécanisme de transfert et d'adossement. Définir ensuite les adresses de réseau,
les montants de liquidité, les détenteurs de parts et la politique des frais.
Un outil de déploiement vérifiant l'identité du réseau et un parcours utilisateur
avec limites de prix/échéance restent à construire. Les essais de manipulation
de prix, de flash swaps, de jetons hostiles et de réorganisation réseau doivent
être complétés avant toute conclusion générale. Un ratio instantané d'AMM n'est
pas un oracle robuste pour des prêts ou un bridge.
