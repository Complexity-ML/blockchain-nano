# Bridge Base–Nano — composants communs et frontières ouvertes

État du 16 octobre 2026 : le socle Solidity est implémenté et ses [153 cas synthétiques par moteur sont qualifiés](qualification/bridge-common-20261016/README.md), sur Anvil et sur le véritable `ExecutionEngine` C++ Nano. **Aucune route réelle n’est activée.** Inclusion/finalité source, vérificateur, actifs, conversion et gouvernance ne sont pas choisis ni qualifiés.

## Composants livrés

| Source | Fonction |
| --- | --- |
| `contracts/bridge/API.sol` | Message versionné à 14 champs, domaine du vérificateur, interfaces de preuve et mint/burn. |
| `BaseBridgeEndpoint.sol` | Route immuable, contrôle du domaine complet, nonce source, outbox, consommation atomique et plafond configurable. L’activation est une fonction abstraite obligatoire sur les quatre opérations. |
| `CanonicalEscrow.sol` | Dépôt/restitution ERC-20, crédit/débit exact du sender, du destinataire et de la réserve. Ni actif natif ni conversion implicite. |
| `MintBurnGateway.sol` | Émission/destruction par un token externe ; contrôle exact de l’offre et des soldes. Ne fournit pas de token officiel ni de vérificateur. |

Les trois endpoints sont abstraits. Seuls les mocks sous `tests/hardhat/contracts/BridgeHarness.sol` implémentent une porte de test, un token synthétique et une approbation de digest. Aucun preset, relayer signant ou script de diffusion en production n’est fourni.

## Message et comptabilité

L’engagement `NanoBridgeMessageV1` lie type LOCK/BURN, chain IDs, identités des domaines source/destination, endpoints, tokens, nonce, sender, destinataire, montant et domaine du vérificateur. `keccak256(abi.encode(...))` inclut tous les champs. Le contrat recalcule cet engagement ; le vérificateur doit en authentifier l’événement exact et la finalité source.

L’identité consommée est `keccak256(abi.encode(kind, sourceChainId, sourceDomain, sourceEndpoint, nonce))`. Elle exclut volontairement le payload : même si une autorité approuve deux montants ou destinataires contradictoires du même événement source, cet endpoint ne peut consommer ce nonce deux fois. Cela ne rend pas fiable une autorité qui invente de nouveaux événements. Un fork copiant le domaine complet reste à départager par l’authentificateur et ses checkpoints, pas par un UUID.

En mêmes unités synthétiques et sans frais :

```
réserve comptabilisée = offre représentée + locks non mintés + burns non libérés
solde réel de l’escrow >= réserve comptabilisée
```

Le don à l’escrow ne crée pas de droit d’émission. Les transferts avec taxe, no-op ou débit incorrect sont refusés. Les messages indépendants peuvent arriver dans l’ordre inverse. La consommation est inscrite avant les effets token ; une exception annule aussi nonce/outbox/consommation et logs. Les callbacks sont protégés contre réentrance. L’outbox, les événements et `consumed` sont l’autorité de reprise ; l’index du relayer est reconstructible. Aucun remboursement automatique sur timeout n’est fourni.

L’offre du token représenté doit être initialement nulle et rester égale à l’encours local du gateway. Un autre minter, un rebase ou une destruction externe rompt cette précondition et bloque mint/burn. Ce blocage protège la comptabilité de cette route ; il n’est ni une réparation, ni une garantie de disponibilité ou de résistance à la compromission de l’émetteur. Les fonds déjà engagés peuvent rester immobilisés : la récupération doit être définie avant activation.

## Actifs et authentification encore ouverts

`WrappedNano` reste un wrapper **local** : son émission est couverte par les NANO natifs déposés. Il n’est pas transformé en token à mint distant. Le token Doppler Base ne donne pas automatiquement un droit sur le NANO natif. Actif et chaîne canoniques, réserves, unités/ratio, frais, remboursements et allocations Base–Nano restent à fixer explicitement ; le banc synthétique ne choisit pas de ratio économique.

`tools/bridge-source` reste un observateur RPC avec `eligibleForMint:false` et `cryptographicFinalityVerified:false`. Ses observations ne sont pas injectées comme autorisation dans les endpoints. Le vérificateur réel doit traiter inclusion, finalité/canonicalité, reorg avant finalité, finalité contradictoire, confiance/compromission et domaines des deux sens. Une simple signature de relayer ou un RPC cohérent ne résout pas ces exigences. Le choix entre preuve cryptographique et quorum explicite reste ouvert.

Pour USDC, utiliser les contrats/proxy FiatToken officiels et la procédure du [Bridged USDC Standard de Circle](https://github.com/circlefin/stablecoin-evm/blob/master/doc/bridged_USDC_standard.md), avec initialisations, rôles, intégration mint/burn et gouvernance examinés. Le gateway commun n’implémente ni ce proxy ni sa procédure de mise à niveau. Une éventuelle reprise par Circle reste soumise à son accord, jamais promise.

## Portée de la qualification

Même bytecode Solidity cible Paris, 153 cas et 77 refus par moteur, rollback, rejouements, désordre, plafond, don, tokens adverses et garde réentrant : voir le rapport et les reçus. Le natif utilise deux états indépendants, des transactions réellement signées et des headers synthétiques ; aucun serveur RPC Nano ni réseau de validateurs n’est lancé. Les reloads sont des roundtrips JSON en mémoire, pas une reprise RocksDB. Pas de preuve de finalité interchaînes, de vraie route Base–Nano, de conformité USDC ou de production acquise.

Les critères de [sécurité du bridge](BRIDGE-SECURITY-REQUIREMENTS.md) et de [production](PRODUCTION-GOAL.md) restent applicables. Authentification, termes et gouvernance doivent être finalisés, implémentés et qualifiés avant toute entrée de fonds ; revue indépendante encore ouverte.
