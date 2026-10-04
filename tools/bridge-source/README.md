# Observation de finalité source Base — composant préliminaire

`observeBaseFinalizedReceipt(rpc, {transactionHash, event})` est une fonction
JavaScript en lecture seule. `rpc.request({method, params})` doit être fourni
par un transport borné (taille de réponse/délai), sans retry implicite.
Il n’existe aucun CLI de mint, clé, relayer, transaction ou déploiement ici.

`event` exige `emitter`, `topics`, `data`, `logIndex` exacts en hex canonique
minuscule, et `codeSha256` (SHA-256 des bytes du code attendu, sans préfixe).
Ces paramètres doivent venir d’une définition de contrat revue indépendamment,
pas être copiés sans vérification depuis le reçu qu’on veut accepter.
Ce hash de code n’est pas un hash Keccak d’état ; un proxy exige encore une
vérification séparée de son implémentation/gouvernance. Un événement choisi
ne prouve pas à lui seul que l’USDC est couvert par une réserve.

Le module exige Base mainnet (`8453`), un reçu réussi dont le bloc ne dépasse
pas la tête `finalized`, des liens exacts entre reçu/log/bloc, et l’événement
attendu. La lecture du code est liée au hash avec `requireCanonical:true`.
Relectures du bloc, du reçu, de la chaîne et du checkpoint finalisé ; refus des
changements/régressions et vérification d’une éventuelle nouvelle tête canonique.
Aucun fallback vers `safe`, `latest` ou un nombre fixe de confirmations.

Le résultat conserve identités source et checkpoint observé, une clé source
`chainId:transactionHash:logIndex` et les bytes exactes de l’événement.
**`eligibleForMint:false` et `cryptographicFinalityVerified:false` demeurent
obligatoires même quand toutes les observations RPC concordent.** Un RPC malveillant
peut fabriquer l’ensemble ; l’inclusion du reçu et la dérivation L1/consensus
ne sont pas authentifiées. Cette fonction n’est ni un light client ni un bridge.
La clé source seule n’est pas un journal idempotent persistant.

Référence protocole : [dérivation Base et signal de finalité L1](https://docs.base.org/base-chain/specs/protocol/consensus/derivation).
Vingt-sept tests avec RPC injecté passent : finalité/reorgs/logs/bindings,
code/identité, refus d’EIP-1898 absent, budgets et évolution de tête finalisée.
Pas encore de preuve issue d’un dépôt réel ou d’un réseau privé interchaîne.
La CI d’outillage inclut ces tests ; pas de nouvelle qualification C++ revendiquée.

Restent : contrats lock/mint/burn/release et standard USDC, preuve/confiance
source authentifiée, journal atomique et reprise/rejeu, couverture, retraits,
compromission, tests interchaînes et définition du lien Nanobees Base–Nano.
Aucun ratio, allocation, offre ou déploiement public n’est approuvé par ce module.

## Transport HTTP borné

`createSourceRpc(endpoint, {timeoutMs, maxResponseBytes})` dans `rpc.mjs`
fournit le transport de lecture : HTTPS (HTTP uniquement sur loopback),
aucune redirection ni retry, délai couvrant aussi le corps de réponse, budget
sur les bytes effectivement reçues, enveloppe JSON-RPC et identifiant exacts.
Defaults : 10 secondes par requête et 1 MiB par réponse ; plafonds configurables
60 secondes et 4 MiB. Les requêtes sont limitées à 64 KiB et aux quatre méthodes
de lecture utilisées par l’observateur. Les erreurs ne reproduisent pas
l’endpoint ni le message du fournisseur. Aucun timeout global de toute
l’observation ou quota de concurrence n’est ajouté ici.

```js
import {createSourceRpc} from './rpc.mjs';
import {observeBaseFinalizedReceipt} from './finality.mjs';
const rpc = createSourceRpc(process.env.BASE_RPC_URL);
// `event` est la définition exacte revue indépendamment, jamais dérivée
// automatiquement du reçu à accepter.
const observation = await observeBaseFinalizedReceipt(rpc, {transactionHash, event});
// observation.eligibleForMint reste false.
```

Qualification initiale : 15 tests avec véritables serveurs HTTP loopback et
24 tests de l’observateur avec RPC injecté, 39/39 sans skip.
Le lot de l’observateur compte désormais 27 tests après la correction des
références mutables ; les 15 tests du transport ne sont pas rejoués pour cette
modification sans changement du transport. Cette combinaison
ne teste pas encore un dépôt réel Base ni un bridge interchaînes.
[Rapport](../../docs/qualification/base-source-rpc-transport-20261004/README.md).

Les champs vérifiés des reçus/checkpoints et la définition revue de l’événement
sont copiés avant les attentes asynchrones ; un cache/configuration mutable
ne doit pas réécrire l’observation initiale. Les champs additionnels non vérifiés
du reçu ne sont pas propagés.
[Échecs reproduits et correction](../../docs/qualification/base-source-observation-snapshot-20261004/README.md).
