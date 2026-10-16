# Sélection du bloc pour les simulations RPC

Proposition source uniquement, préparée hors dépôt. Aucun programme, compilateur, test, conteneur ou nœud n’a été exécuté par l’auteur. La qualification causale de ROOT reste attendue.

`eth_call` et `eth_estimateGas` rejetaient tout objet comme second argument, alors que `eth_createAccessList` acceptait déjà les sélecteurs `{blockHash, requireCanonical}` et `{blockNumber}`. Le changement extrait le parseur et la validation de canonicalité déjà présents, puis les réutilise pour les deux simulations. `rpc.hpp` et les autres méthodes de lecture restent inchangés.

L’[EIP-1898 officiel](https://eips.ethereum.org/EIPS/eip-1898) définit les objets de sélection et cite `eth_call`. Il recommande de distinguer un bloc introuvable d’un bloc connu non canonique, avec priorité à la recherche du bloc. `eth_estimateGas` n’est pas dans sa liste : sa prise en charge ici est une extension Nano explicite, sans revendication de conformité générale à cet EIP.

Le parseur exige exactement un numéro QUANTITY canonique de 64 bits ou un hash DATA de 32 octets, refuse les champs supplémentaires et réserve le booléen `requireCanonical` aux hashes. Un objet inconnu pour call/estimate renvoie `-32001` ; les anciens sélecteurs chaînes et les codes de createAccessList restent inchangés. La valeur par défaut d’estimateGas demeure pending ; eth_call omis demeure latest. Les contrôles de frais, l’algorithme d’estimation et le budget partagé d’annulation/temps ne changent pas.

Le contexte objet fige l’état, l’en-tête et les ancêtres suivis par leurs hashes avant de libérer le verrou RPC. L’état doit correspondre à la racine de l’en-tête. Le stockage conserve des états par hauteur canonique : un en-tête sans état disponible est refusé, sans lui associer l’état d’une autre branche. Une branche non canonique conservée avec son état propre n’est pas fournie par le Store actuel ; cette limite reste ouverte. Le test d’un hash devenu obsolète après rewind est un cas de ressource introuvable, pas une preuve d’exécution d’une branche retenue.

## Test nouveau et qualification attendue

La cible nouvelle `nano-rpc-simulation-block-id-tests` accepte seulement `--eip1898-self-test`. Le CTest `rpc_simulation_eip1898` possède une limite de 45 secondes. Elle n’appelle aucune ancienne suite RPC, aucun serveur HTTP et aucun réseau. Ses bases temporaires et ses soldes sont exclusivement synthétiques ; aucune clé ou signature de transaction n’est nécessaire.

Les cinq groupes prévus sont : équivalence hash/numéro historique et maintien de pending ; douze objets malformés et priorité des erreurs de ressources absentes ; contexte détaché conservé pendant un rewind avec disponibilité d’une lecture d’identité concurrente ; annulation et échéance sur hash ; en-tête connu privé de son seul état historique. Chaque famille inclut createAccessList quand elle exerce le parseur partagé. Le rendez-vous concurrent est borné et libère le travailleur même si une assertion échoue.

Le résumé final attendu est un JSON de format `nano-rpc-simulation-eip1898-targeted-v1`, avec `passed:true`, `groups:5`, `expectedRefusals:58`, `oldSuiteExecuted:false`, `networkStarted:false` et `productionQualified:false`. Les 58 refus sont 36 objets malformés, 9 ressources inconnues, 6 hashes supprimés après rewind, 4 annulations/échéances et 3 états historiques absents. Ce sont des attentes source, pas des résultats déjà acquis.

ROOT doit d’abord lier le nouveau test à l’ancien Core immuable : la première requête objet échoue après une baseline chaîne réussie. Le résultat causal attendu est un seul JSON final `passed:false`, `stage:canonical-selectors`, `checks:4`, `expectedRefusals:0`, avec stderr `selected simulation should succeed` suivi d’un saut de ligne. Ensuite, ROOT peut compiler `src/rpc.cpp` et le nouveau test (deux TUs), copier le Core, remplacer seulement son membre rpc, vérifier les 142 autres membres inchangés sur 143, lier et exécuter uniquement cette nouvelle cible. Aucune reconstruction du Core original, d’image ou ancienne suite identique n’est requise par cette proposition.

La proposition ne qualifie pas les autres méthodes EIP-1898, les branches hors chaîne canonique retenues, les forks non activés, le transport HTTP, la charge, les duties, la finalité, la production ou la CI. Les preuves et échecs historiques du projet restent valables dans leurs portées antérieures.
