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
Vingt-quatre tests avec RPC injecté passent : finalité/reorgs/logs/bindings,
code/identité, refus d’EIP-1898 absent, budgets et évolution de tête finalisée.
Pas encore de preuve issue d’un dépôt réel ou d’un réseau privé interchaîne.
La CI d’outillage inclut ces tests ; pas de nouvelle qualification C++ revendiquée.

Restent : contrats lock/mint/burn/release et standard USDC, preuve/confiance
source authentifiée, journal atomique et reprise/rejeu, couverture, retraits,
compromission, tests interchaînes et définition du lien Nanobees Base–Nano.
Aucun ratio, allocation, offre ou déploiement public n’est approuvé par ce module.
