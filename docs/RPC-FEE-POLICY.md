# Recommandation des frais RPC

Nano estime désormais le pourboire à partir des transactions des blocs récents.
Il n’impose plus une recommandation constante de 1 gwei. Cette politique locale
ne modifie ni le calcul de base fee EIP-1559, ni les frais des transactions
signées explicitement par les utilisateurs.

## Configuration du nœud

Le fichier de configuration chargé par `nano run` ou le serveur RPC accepte :

```json
{
  "fee_policy": {
    "history_blocks": 20,
    "samples_per_block": 3,
    "percentile": 60,
    "fallback_tip": "0x0"
  }
}
```

| Champ | Défaut | Domaine |
| --- | --- | --- |
| history_blocks | 20 | Entier de 1 à 1024 ; fenêtre en blocs, y compris les blocs vides |
| samples_per_block | 3 | Entier de 1 à 100 ; nombre maximal de transactions retenues par bloc |
| percentile | 60 | Entier de 0 à 100 |
| fallback_tip | 0 | Quantité uint256 en wei, utilisée si aucun échantillon n’est disponible |
| maximum_tip | Aucun | Quantité uint256 facultative limitant la recommandation |

Les bornes de fenêtre et d’échantillonnage sont des limites de travail RPC,
pas des limites du registre de validateurs. Une clé inconnue, un type invalide,
une valeur hors domaine ou un fallback supérieur au maximum font échouer la
configuration. Un rechargement invalide conserve la politique précédente.
Un rechargement valide invalide le cache de recommandation et les échantillons
de blocs.

## Calcul

1. Capturer l’en-tête canonique courant et son hash.
2. Parcourir ses ancêtres par hash dans la fenêtre configurée.
3. Calculer le pourboire effectif de chaque transaction : prix effectif moins
   la base fee du bloc, borné à zéro. Pour une transaction de type 2, le plafond
   `maxFeePerGas` limite donc bien le pourboire réellement payé.
4. Écarter les transactions émises par le bénéficiaire du bloc, puis retenir
   les moins chères jusqu’au budget d’échantillons de ce bloc.
5. Trier les échantillons réunis et prendre l’indice entier
   `(nombre - 1) * percentile / 100`. Les pourboires nuls sont admissibles.
6. Appliquer le maximum facultatif ; utiliser le fallback uniquement si la
   fenêtre ne contient aucun échantillon admissible.

Les blocs vides n’ajoutent pas de prix synthétique. Le résultat est mémorisé
pour le hash de tête, ce qui invalide aussi une réorganisation à hauteur égale.
Une consultation du cache ne copie pas les transactions du bloc. Les échantillons
sont également conservés par hash de bloc : un nouveau bloc réutilise ses ancêtres
communs sans relire leurs transactions. Seule la fenêtre de la dernière estimation
réussie est conservée (au plus `history_blocks` entrées de `samples_per_block`
pourboires). Une fenêtre candidate de même taille est temporairement construite.
Les anciennes branches et les blocs sortis de la fenêtre sont évincés.

Si la tête change pendant le calcul, ou si un bloc non présent dans ce cache
n’est plus disponible en base, la requête échoue et peut être réessayée. Un
échantillon déjà calculé reste réutilisable par son hash même si son bloc a depuis
été élagué. Ni la fenêtre ni la recommandation ne sont publiées partiellement.

## RPC et portefeuille

- `eth_maxPriorityFeePerGas` retourne ce pourboire estimé.
- `eth_gasPrice` ajoute la base fee de l’en-tête capturé ; la somme est bornée
  au maximum uint256.
- Le portefeuille utilise la même estimation pour un pourboire omis. Si un
  plafond `maxFeePerGas` est fourni, le pourboire implicite ne le dépasse pas.
- Sans plafond fourni, le portefeuille calcule `2 * baseFee + pourboire`, borné
  à uint256. Les prix et pourboires explicitement fournis restent inchangés.
- Les simulations `eth_call` et `eth_estimateGas` conservent leurs règles
  existantes : elles ne reçoivent pas de prix inventé par cet estimateur.

## Périmètre et limites

Une recommandation historique ne garantit pas l’inclusion future. Des adresses
indépendantes du bénéficiaire peuvent encore influencer les échantillons. Les
valeurs peuvent différer de celles d’autres clients, qui emploient leurs propres
politiques. Une mesure locale sur 20 blocs de 1 428 transactions observe environ 652 ms
sans cache de recommandation (caches disque non purgés), sous verrou RPC. Le
cache évite de répéter ce coût sur une tête inchangée. Avec réutilisation des
ancêtres après un nouveau bloc de 1 428 transactions : 33,518 ms, contre
671,225 ms pour un estimateur neuf sur la même tête dans le même run. Le premier
calcul, les branches nouvelles, la fenêtre maximale et la saturation HTTP/P2P
restent à qualifier au-delà de ces mesures locales.
[Cache incrémental et limites](qualification/rpc-fee-incremental-20260927/README.md).
[Mesures et défaut d’adresse corrigé](qualification/rpc-fee-load-20260927/README.md).

Référence de conception pour l’échantillonnage des frais effectifs et le cache
par tête : [oracle de frais de Geth](https://github.com/ethereum/go-ethereum/blob/master/eth/gasprice/gasprice.go).
Nano utilise sa propre politique de fenêtre vide et ses paramètres ci-dessus.
Le comportement EIP-1559 du protocole reste défini par
[l’EIP-1559](https://eips.ethereum.org/EIPS/eip-1559).

## Vérification

`rpc_fee_policy` couvre historique vide, mélange legacy/type 2, exclusion des
transferts du producteur, budgets et percentiles configurés, invalidité et
atomicité de configuration, remplacement à hauteur égale, expiration de la
fenêtre, défauts du portefeuille, champs explicites et bornes uint256.
Les parcours existants `rpc`, `rpc_cli` et `wallet_cli` complètent cette
qualification locale. Elle ne vaut pas qualification complète de production.
