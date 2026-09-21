# Qualification OpenZeppelin sur Nano

Périmètre demandé : tout OpenZeppelin Contracts v5.0.2, y compris NFT et les
composants nécessaires à des applications de marché. Le paquet séparé
`contracts-upgradeable` et les autres versions ne sont pas couverts par cet
inventaire ; leur qualification exige un inventaire supplémentaire.

Les 143 sources Solidity de production officielles sont conservées sous
`tests/contracts/openzeppelin-v5.0.2/contracts`, avec la licence MIT. Elles
produisent 135 composants nommés (contrats, interfaces, bibliothèques). Le fichier
`tests/fixtures/openzeppelin-catalog.json` donne leur liste complète, les SHA-256
et les paramètres du compilateur. `node scripts/build-openzeppelin-catalog.mjs`
reproduit ce contrôle avec solc 0.8.28, cible Paris, optimiseur 200 passages.

**Compiler le catalogue ne prouve pas sa conformité d’exécution.** Les statuts
`pending` du manifeste désignent la qualification comportementale complète,
y compris pour les composants dont quelques scénarios sont déjà couverts.

## Scénarios natifs actuels

`ctest --test-dir build-cmake --output-on-failure -R '^erc20$'` exécute le
programme C++ `tests/test_erc20.cpp`. Son nom historique est conservé, mais il
couvre désormais plusieurs tokens. Les bytecodes précompilés se régénèrent avec
`node scripts/build-erc20-test.mjs`. Aucun compilateur Solidity, Node.js ou Python
n’est requis pour l’exécution normale des tests.

- ERC20 : émission initiale fixe, transferts, événements, allowance finie et
  illimitée, révocation, refus de dépassement et d’adresse nulle.
- SafeERC20 : appels imbriqués vers un ERC20 standard, retour vide, retour false
  avec mutation annulée, et refus d’une adresse sans code.
- ERC721 : émission initiale, propriété, approbation individuelle, opérateur
  non autorisé, transfert sûr à un compte, approbation effacée au transfert et
  restauration de la propriété et de l’approbation après refus du destinataire.
  Callbacks avec mauvais sélecteur ou revert après écriture, et retransfert du
  NFT pendant le callback avec contrôle du propriétaire final et des événements.
- ERC1155 : émission initiale, approbation globale, révocation, transfert simple,
  manque de solde et restauration des soldes après refus du destinataire.
  Lots contenant un identifiant répété, acceptation et refus du callback batch,
  restauration des écritures du destinataire, retransfert pendant un callback
  simple sans duplication du solde déjà détenu.

Chaque transaction est signée ; chaque bloc possède une signature de producteur
et est rejoué sur un second moteur, avec comparaison des racines et reçus. Les
historiques sont audités et la racine est contrôlée après réouverture RocksDB.
Les wrappers et tokens adversariaux sont réservés aux tests, sans déploiement
public ni attribution de valeur aux jetons.

## Couverture restant à établir

L’inventaire complet fait foi pour éviter de perdre un composant en route.
Les familles suivantes nécessitent encore des scénarios comportementaux :

- Extensions ERC20 : permit, votes, flash mint, burn, cap, pause, wrappers et ERC4626.
- Extensions ERC721/ERC1155 : autres cas batch (longueurs incohérentes, soldes
  insuffisants en fin de lot, callbacks batch réentrants),
  énumération, URI, royalties ERC2981, burn, pause et wrappers.
- Access control, Ownable, rôles, délais et gestionnaire d’accès.
- Governor, Timelock et comptage des votes.
- Proxies, beacon, transparent, UUPS, initialisation et protection des mises à jour.
- Utilitaires : signatures, cryptographie, Merkle, mathématiques, collections,
  encodages, stockage, réentrance et appels multiples.
- Compositions : ventes NFT, annulations, signatures d’ordres, paiements ERC20,
  royalties selon les règles du marché, réentrance et atomicité des règlements.

Une marketplace complète n’est pas livrée dans OpenZeppelin Contracts. Les
contrats de vente et leurs règles devront être définis et testés en composition.
Même exigence pour le bridge et l’AMM : les primitives OpenZeppelin ne remplacent
pas la vérification de leurs invariants ni la finalité distribuée de Nano.

## Rejeu malveillant entre identités de chaîne

Aucun fork permanent ni dépôt dérivé n’est créé. Le même test utilise des bases
temporaires supprimées à sa sortie, avec les identités 1337 et 7331, et des
transactions classiques protégées EIP-155 ou typées EIP-1559. Les quatre cas
CTest sont `erc20`, `openzeppelin_isolation`, `openzeppelin_eip1559` et
`openzeppelin_isolation_eip1559`.

Pour chaque transaction, une instance portant un autre chainId doit refuser le
rejeu sans toucher à la mempool, au tip ou à la racine d’état. Une seconde tentative
réécrit le domaine dans les octets de la transaction sans refaire r/s : la signature
ne doit pas autoriser le compte financé d’origine. Elle est refusée dans la fixture
(identité récupérée différente et non financée, ou signature invalide).

Cette protection suppose des chainId distincts. Elle ne prétend pas empêcher le
rejeu entre deux réseaux réutilisant le même chainId, ni qualifier un bridge ou les
partitions du consensus. Aucun réseau public n’est contacté par ces scénarios.

Les scénarios d’entrée adversariaux incluent désormais un lot ERC1155 avec
débit initial possible puis dépassement cumulé, des longueurs de tableaux
incohérentes, une calldata tronquée, un offset ABI maximal et un destinataire nul.
Les refus doivent conserver tous les soldes et les écritures du receiver, sans
événements résiduels. Un transfert de quantité zéro sert de cas normal limite.

### Marketplace volontairement vulnérable : confinement d'une attaque

`TestMarket.sol` est exclusivement une fixture locale, jamais un contrat de
lancement. Elle utilise IERC721 d'OpenZeppelin mais omet volontairement le contrôle
du vendeur lors de l'annulation. Le test exige que l'attaquant réussisse à retirer
le NFT d'un autre compte : le moteur n'ajoute aucune interdiction spéciale.
Il vérifie l'absence de duplication, la conservation des autres tokens et la
reproduction exacte de l'état et des reçus par un second moteur indépendant.

Une boucle infinie consomme ensuite les 3 000 000 unités de gas de sa transaction.
Les écritures sont annulées, le gas débité et le nonce consommé ; une transaction
normale réussit après l'attaque. L'historique complet est audité et la base rouverte.
Ce scénario couvre les enveloppes legacy et EIP-1559 sur deux identités locales.
Il ne prouve ni la résistance à un flood distribué ni la finalité BFT en réseau ;
les blocs de ce test utilisent le mode PoS signé existant. Il ne constitue pas un
audit de marketplace ni une garantie de confinement de toute vulnérabilité EVM.

### Publication des contrats concrets et parcours Hardhat

L'AST du compilateur distingue les contrats concrets des interfaces, bibliothèques
et contrats abstraits. Les neuf contrats concrets du catalogue 5.0.2 sont exportés
dans `openzeppelin-deployments.json` et déployés par transactions signées :
AccessManager, VestingWallet, TimelockController, ERC2771Forwarder, ERC1967Proxy,
BeaconProxy, UpgradeableBeacon, ProxyAdmin et TransparentUpgradeableProxy.
Les tests vérifient des paramètres de construction, getters et appels délégués
avec séparation du stockage. Ce sont des scénarios initiaux, pas leur suite
comportementale exhaustive. Ils passent dans les quatre variantes legacy/type 2.

`tests/hardhat` contient un client Hardhat 3.17.0, ethers 6.16.0, solc 0.8.28 et
OpenZeppelin 5.0.2 épinglés. Depuis ce dossier : `npm ci`, puis
`NANO_BINARY=/chemin/absolu/vers/nano node run-local.mjs`.
Le pilote lance un vrai processus Nano temporaire, compile DeveloperToken via
Hardhat, le déploie avec signature locale, transfère 125 unités, vérifie les logs
et soldes et exige le refus d'un transferFrom non autorisé à l'estimation.
Le nœud utilise un validateur BFT local de test. Les blocs sont certifiés ; aucun
`hardhat node`, fork, compte réel ou réseau public n'est utilisé.
Ce parcours ne couvre pas encore MetaMask dans un navigateur, les outils Foundry,
ni tous les composants abstraits et bibliothèques OpenZeppelin.

### Comparaison indépendante Anvil

Depuis `tests/hardhat`, lancer
`ANVIL_BINARY=/chemin/anvil NANO_BINARY=/chemin/nano node run-local.mjs scripts/compare-anvil.ts`.
Le script utilise Hardhat et déploie les mêmes transactions legacy signées sur
Nano et Anvil configuré Paris, chain ID 1337. La référence observée est Anvil
1.5.1, commit b0a9dd9ceda36f63e2326ce530c10e6916f4b8a2. Trois transactions sont
comparées : déploiement ERC20, transfert et transferFrom sans autorisation envoyé
avec un budget explicite, donc réellement inclus et exécuté malgré son revert.
Les statuts, logs, soldes, offre totale et consommations de gas concordent.

| Cas | Nano avant correction | Anvil / Nano après correction |
|---|---:|---:|
| Déploiement | 572191 | 552591 |
| Transfert | 61360 | 51560 |
| Revert sans autorisation | 3000000 | 24903 |

Le test a révélé une double facturation du stockage et le traitement incorrect
 de REVERT comme une exception consommant tout le gas. Ils sont corrigés.
La conformité Paris complète reste non démontrée : notamment la comptabilité SSTORE des réécritures dans une même
transaction demande encore correction et qualification indépendante. Les accès
aux comptes, appels et listes d'accès ont depuis reçu de premiers correctifs
vérifiés par les tests officiels ; le bilan reste incomplet dans EVM-CONFORMANCE.md.

### Extension du différentiel au stockage Solidity

`StorageMeter.sol` complète le différentiel Anvil Paris avec onze transactions
supplémentaires (déploiement et dix exécutions), soit quatorze transactions
signées identiques au total. Les scénarios couvrent les cycles de stockage,
les remboursements et les appels imbriqués réussis/annulés. Le script compare
les statuts, tous les logs, le gas de chaque reçu et la valeur du slot après
chaque appel au contrat. Ce contrat est une fixture propre au projet, pas un
composant OpenZeppelin ; ces cas ne complètent pas le catalogue OpenZeppelin.
