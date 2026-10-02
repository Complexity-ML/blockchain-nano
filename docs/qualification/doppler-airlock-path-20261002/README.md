# Raccordement Doppler : inspection des contrats, non qualifié

Révision officielle figée **5754c7ee01f1bdbd6f07c62be721e1223b725ecd**, empreintes/liens dans sources.json. Airlock exige des modules autorisés, appelle lockPool/unlockPool et transfère la propriété à un timelock. Autorisation contrôlée par son propriétaire. [Airlock](https://github.com/whetstoneresearch/doppler/blob/5754c7ee01f1bdbd6f07c62be721e1223b725ecd/src/Airlock.sol).

Le `WrappedNano.sol` local ne fournit pas ces contrôles. Son dépôt enveloppe l'actif natif de sa chaîne, sans transfert interchaînes. Il reste inchangé : aucun pouvoir administratif ajouté.

Critères proposés : vérifier bytecode/adresses/modules du déploiement retenu ; concevoir séparément le token Base et le remboursement NANO ; couvrir toute l'offre créée (vente, allocations et liquidité) par une réserve canonique, avec conversion d'unités explicite. Qualifier mint/burn, finalité des deux sources, anti-rejeu et reprise ; tester les effets du timelock/gouvernance sur les réserves. Ce sont des exigences à implémenter, pas des fonctionnalités acquises.

L'exemple de suivi SDK mélange Base Sepolia et l'adresse USDC Base. Choisir les adresses Circle par réseau et vérifier decimals par RPC avant de calculer les montants : [Circle](https://developers.circle.com/stablecoins/usdc-contract-addresses), [exemple](https://docs.doppler.lol/reference/examples/quoting-monitoring-and-metrics).

Inspection statique uniquement : aucune ABI/permission de déploiement vérifiée, aucun bridge ni vente exécuté, aucun référencement garanti.
