# Genesis livrés et activation des signatures

`genesis/mainnet.json` et `genesis/testnet.json` activent désormais
`posSignatureVersion: 1`. Le producteur d’amorçage est l’adresse déjà utilisée
par Nano : `0x311384E7e4803AdBDB1D8ab46b973eeE0f9cF55B`. Aucune clé privée
n’est ajoutée au dépôt. Les allocations et chain IDs restent inchangés.

| Fichier | Chain ID | Hash du genesis signé |
| --- | --- | --- |
| mainnet.json | 7770 | `0x94112c34ed57a1c9ab79011df4eb04b6f53f54ba1e51e118154a50391db25603` |
| testnet.json | 77777 | `0xab9b61774ac656e1afa2adb8aa9faec0ccf4a7f1b4aa949ae7021eec0db6a932` |

Le domaine de signature modifie le hash du genesis. Ces fichiers définissent
ainsi de nouvelles identités de chaîne, incompatibles avec les bases précédentes.
L’ouverture d’une ancienne base avec le nouveau genesis échoue ; aucune
migration automatique ni réécriture de son historique n’est effectuée.

Les anciennes configurations exactes sont conservées dans
`genesis/legacy/mainnet-unsigned.json` et
`genesis/legacy/testnet-unsigned.json`, pour inspection et migration historique.
Leur format non signé reste pris en charge explicitement. Elles ne constituent
pas une configuration adaptée à un réseau public.

L’exemple `examples/validator.json` contient les pins du nouveau testnet, un
fichier genesis et deux dossiers distincts pour la vérification et le journal
de signature. Ses chemins relatifs se résolvent depuis le répertoire de travail ;
les adapter à l’installation, de préférence en chemins absolus. Le producteur
d’amorçage doit disposer de la clé correspondant à l’adresse configurée.
Modifier cette adresse dans le genesis crée encore une autre identité de chaîne
et impose de recalculer le pin. Ne pas remplacer ni réutiliser un journal de
signature pour contourner un conflit.

## Limites de lancement

Les noms mainnet/testnet ne signifient pas qu’un réseau a été lancé ou qu’il
est prêt pour la production. Ces fichiers restent des configurations de travail.
Ils ne configurent pas encore de contrat de staking : la transition de
l’amorçage vers un ensemble ouvert de validateurs reste à définir dans les
paramètres de lancement. La finalité, le traitement des branches concurrentes,
les sanctions, les essais prolongés et la revue de sécurité restent ouverts.
Voir `PRODUCTION-GOAL.md` et `BLOCK-SIGNATURES.md`.

Le test `installed_node` vérifie les configurations effectivement installées,
les identités différentes et le refus de mélanger une base historique avec un
nouveau genesis, puis contrôle que cette base reste lisible avec l’ancien.

## Transition de staking testée localement

La suite `staking` démarre aussi deux chaînes signées depuis un genesis de test
sans code de contrat, mais avec une adresse de staking prédéterminée. La première
transaction signée du producteur déploie `NanoStaking` avec son dépôt initial.
Le pair rejoue ce bloc et retrouve le même état et le même registre. Une seconde
clé s’inscrit par transaction ; les producteurs sélectionnés signent ensuite
huit blocs supplémentaires. Les deux historiques sont audités depuis le
genesis, et la persistance du registre est contrôlée après réouverture.

Ce test utilise des clés synthétiques. Il ne prouve ni la possession de la clé
d’amorçage des genesis livrés ni l’exécution d’un déploiement public. Aucune
adresse de staking n’a été ajoutée aux genesis livrés à cette étape.

## Règles explicites du consensus

Le parseur prend désormais en charge un objet facultatif `config.consensusRules`
versionné pour les chaînes PoS signées :

```json
{
  "version": 1,
  "minimumBlockInterval": 12,
  "maximumFutureDrift": 15,
  "minimumGasLimit": 5000
}
```

Les quatre champs sont obligatoires, entiers et sans champ supplémentaire.
L’intervalle minimal vaut entre 1 et 3600 secondes ; la tolérance future entre
0 et 3600 secondes. La limite minimale de gaz vaut au moins 5000, tient dans
un uint64 et ne dépasse pas la limite de gaz initiale. Les versions inconnues
et les paramètres invalides sont refusés.

Ces règles modifient réellement la préparation et la vérification des blocs.
La préparation RPC utilise l’intervalle minimal ; le producteur automatique
attend qu’il soit écoulé. Les imports et le rejeu appliquent les mêmes bornes.
La tolérance future est mesurée par rapport à l’horloge du nœud qui vérifie.

Les paramètres sont engagés dans `extraData` du genesis par Keccak d’un RLP
contenant le domaine `Nano/Consensus/Rules`, version 1, intervalle minimal,
tolérance future, gaz minimal et l’engagement PoS précédent. Changer un
paramètre change donc le hash du genesis et l’identité de stockage/réseau.

Sans cet objet, les règles historiques restent 1 seconde minimale, 15 secondes
de tolérance et 5000 de gaz minimal. Le champ historique `config.blockTime`
n’est pas une règle de validité des blocs ; il ne doit pas être confondu avec
`block_time`, intervalle local du client/producteur, ni avec
`minimumBlockInterval`. L’objet n’a pas été ajouté aux genesis livrés à cette
étape : une activation doit mettre à jour ensemble leur identité et les pins.

Cela ne définit pas encore tout le lancement BFT. L’ensemble initial et les
poids de vote, l’évolution des dépôts, les paramètres économiques et l’activation
du protocole de finalité doivent encore être liés à une version de règles
complète. Les certificats expérimentaux ne sont pas actifs sur ces genesis.

## Paramètres initiaux EIP-1559

Le genesis accepte maintenant `evm_rules` à la racine :

```json
{
  "gasLimit": "0x1c9c380",
  "evm_rules": {
    "version": 1,
    "initial_base_fee": 25000000000,
    "target_gas_per_block": 15000000
  }
}
```

`version` est facultatif et vaut 1 ; les deux autres champs sont obligatoires.
Les champs supplémentaires, versions inconnues, frais négatifs ou hors uint256,
cibles nulles et désaccords entre cible et capacité sont refusés. La capacité
initiale doit être exactement deux fois la cible. `baseFeePerGas` à la racine
est aussi accepté ; si les deux formes sont présentes, leurs valeurs doivent
coïncider. Sans aucun de ces champs, le défaut historique reste 1 Gwei.

`initial_base_fee` fixe le frais de base du bloc zéro, et non un plancher
permanent. Ce montant figure dans l’en-tête et change directement le hash du
genesis. Le bloc zéro est vide : avec 25 Gwei, le bloc 1 attend 21,875 Gwei.
La cible initiale ne devient pas une constante indépendante : les blocs suivants
utilisent la moitié de la limite de gaz de leur parent.

Le calcul natif existant suit la [règle EIP-1559](https://eips.ethereum.org/EIPS/eip-1559) :
avec B le frais parent, U son gaz utilisé et T sa cible, il conserve B lorsque
U=T ; sinon il applique une variation entière proportionnelle à l’écart, divisée
par T puis par 8. La hausse est d’au moins 1 wei ; la baisse n’a pas ce minimum.
À cible 15 millions et B=25 Gwei, un parent plein à 30 millions donne 28,125 Gwei.

Les tests vérifient ces valeurs, la hausse minimale, le lien avec le hash du
genesis et le rejet par un pair d’un bloc 1 réutilisant à tort le prix initial.
Les genesis livrés ne sont pas modifiés automatiquement par cette prise en charge.

La variation de la limite de gaz respecte également la borne stricte EIP-1559 :
`abs(limite_enfant - limite_parent) < floor(limite_parent / 1024)`.
La vérification PoS et PoW refuse donc les deux valeurs exactement sur la borne,
ainsi que celles au-delà. Les tests acceptent les valeurs situées une unité à
l’intérieur et vérifient qu’un bloc PoS signé sur la borne ne modifie pas la chaîne.

Ce contrôle corrige l’ancienne comparaison inclusive du port. Un historique
contenant un bloc accepté uniquement par cette ancienne comparaison échouera au
rejeu avec cette version. Aucune migration automatique de tels blocs ni activation
par hauteur n’est fournie ; les nœuds d’un même réseau doivent appliquer la même règle.

L’EVM arrondit désormais la mémoire active aux mots de 32 octets : après une
écriture à l’octet 33, `MSIZE` retourne 64, contre 34 dans le prototype et
l’ancien port. Le gaz d’expansion reste calculé par mots. Cette correction peut
modifier les résultats et racines des blocs dont les contrats utilisent `MSIZE`.
Elle ne comporte pas d’activation par hauteur : un tel historique doit être
identifié avant une mise à niveau réseau, et ne doit pas être réécrit implicitement.
Référence : https://ethereum.github.io/yellowpaper/paper.pdf.

### Correction de la facturation du stockage et de REVERT

La table d'opcodes ne facture plus une seconde fois la surcharge de SLOAD ni le
coût dynamique de SSTORE. Un REVERT restitue le gas non consommé ; les exceptions
exceptionnelles (par exemple manque de gas) consomment toujours le budget.
La comparaison Hardhat/Anvil des transactions ERC20 est décrite dans
OPENZEPPELIN-QUALIFICATION.md et le test conserve les divergences corrigées du
prototype sous forme d'attendus explicites.
Ces changements modifient les reçus, soldes de frais, racines et hashes des blocs
concernés. Les historiques expérimentaux précédents ne sont pas migrés. Aucun
mécanisme d'activation pour un réseau public déjà en fonctionnement n'est fourni
par cette correction ; ce chantier reste nécessaire avant production.

Les correctifs ultérieurs d'accès aux comptes, de coût des appels, de restitution
du stipend sur refus et de préchauffage des listes d'accès ont le même impact sur
les reçus et engagements historiques. Le pilote officiel reste diagnostique tant
que la sélection des règles par fork et leur activation ne sont pas établies.

## Sélection explicite de Shanghai

`config.evmFork: "Shanghai"` active les règles EVM Shanghai au bloc genesis.
Le domaine `Nano/EVM/Shanghai/v1` est engagé dans le hash du genesis. Une base
Shanghai ne peut pas être rouverte avec un genesis Paris, et réciproquement.
Les fichiers mainnet/testnet livrés restent inchangés ; modifier cette option
ne constitue pas une migration d'une chaîne déjà existante. Aucun calendrier
de changement de règles en cours de chaîne n'est encore implémenté.

La limite d'initcode de 49 152 octets et son coût par mot sont appliqués dans
le moteur, l'admission des transactions et les RPC de simulation. Les règles
de signature liées au chain ID restent obligatoires sur le réseau Nano.
