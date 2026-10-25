# InventoryV2 : 65 transactions identiques comparées à Geth t8n

**Les 65 transactions signées retenues de l'exécution Native concordent avec Geth 1.16.7 sur chacune des 65 racines d'état après transaction et sur les autres champs stricts comparés.** Cette preuve compare le moteur Native in-process et l'outil standalone Geth en environnement privé Linux AMD64, sous Shanghai. Elle complète la qualification des contrats InventoryV2 ; la production du réseau reste ouverte.

Le pilote standalone `evm t8n` reçoit les enveloppes signées exactes sous liste RLP, sans resigner ni rejouer Native. Son préétat contient trois comptes publics de fixture financés explicitement. Chaque étape reprend les **bytes du postétat propre à Geth**, sans normalisation d'allocation ni rafraîchissement depuis Nano/Anvil. Contexte : chainId 31337, transactions legacy, gasPrice 1, baseFee 0, coinbase 0x9999, gasLimit 60M/transaction 30M, timestamp 1700000000+12×étape, Random 0, retraits vides et récompenses de bloc désactivées.

| Contrôle réellement comparé | Résultat |
|---|---:|
| Hash de l'enveloppe signée, statut et gasUsed/cumulativeGasUsed | 65/65 exacts |
| Nonce émetteur, solde du payeur et frais cumulés du coinbase | 65/65 exacts |
| Racine d'état après **chaque** transaction | 65/65 exactes |
| Commandes détenues : version +65 transitions | 66 exit0/close0, groupes joints |
| Descripteurs explicitement détenus et fermés | 657 pilote +25 contrôleur Root |

La [trace](trace-public.json) totalise **207 551 638 gas** ; sa dernière racine est `0xe8e7f446e644abb4e2aa38a00394ada349edbad7e9d5787634c0053459edcf87`. Le [résultat externe Root](root-result-public.json) constate Docker exit0, sans échec primaire ou secondaire. Le [contrôleur intermédiaire](controller-public.json) reste volontairement `passed:false` et exige ce terminal externe ; il compte 656 FD fermés avant sa dernière publication, pour 657 au terminal observé par Root. Les 25 FD Root sont comptés séparément ; ces compteurs ne couvrent pas tous les descripteurs du système.

La [provenance de référence](reference-provenance-public.json) authentifie l'outil Linux AMD64 officiel 1.16.7 par la signature PGP Linux Builder `FDE5A1A044FA13D2F7ADA019A61A13569BA28146`. Le [lien des entrées](input-binding-public.json) épingle l'accounting Native et le binaire `evm` (`7bad7839da2684bbc6eeab1c5b045e2f2bedd2e9132d8971ccd4f25adc058861`). Source du pilote : `79392467104fcddb69ff2d30181641daffc64764229b8624bc04134e737f6da4`. Aucune nouvelle compilation C++/Solidity, aucun build d'image et aucun rejeu Native. Revue opérateur avant exécution ; revue Source complémentaire après observation, sans nouveau rejeu. Ces revues ne ferment pas l'audit de sécurité indépendant. Les fichiers publics sont épinglés dans le [manifeste](manifest-public.json).

La racine du préétat initial n'est pas comparée indépendamment par ce pilote. Les logs, blooms et autres engagements complets de reçus/en-têtes ne sont pas des comparaisons supplémentaires de ce lot. Les reçus de commande retenus couvrent les 66 réussites observées ; l'outil ne sauvegarde pas séparément stdout/stderr d'une commande échouée. Il ne qualifie pas RPC/P2P, BFT, reprise de signataire, disponibilité, activations d'autres forks, réserve/bridge réel, paramètres économiques ou lancement Doppler. Il utilise le résultat Native préexistant ; il ne réexécute pas ses 24 assertions contractuelles et 22 points appariés.

La divergence Anvil 1.5.1 sur la feuille de stockage zéro reste un **FAILED historique**, avec sa preuve et son explication distinctes. Les 324 gas d'écart observés dans une autre trace à expéditeurs différents restent ouverts ; ce nouveau 65/65 ne réécrit pas cette ancienne comparaison. Les preuves EEST, les échecs de campagnes de consensus et tous les critères des quatre documents normatifs conservent leur portée. Objectif de production actif ; audit indépendant et autres exigences non closes restent ouverts.

