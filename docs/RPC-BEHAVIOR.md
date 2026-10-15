# Audit comportemental : enveloppe RPC et lots

Comparaison avec `NanoPyRPC.handle_request` et `handle_batch` du fichier source
`nanopy/network/http_rpc.py`, commit `77f2664390db135171608e7bbb613f4ea6468312`.
Les suites `rpc` et `http` vérifient les comportements natifs décrits ci-dessous.

| Cas | Python source | Nano C++ |
|---|---|---|
| Identifiant absent | Réponse avec `id: null` | Exécution sans réponse ; HTTP 204 |
| Identifiant explicitement nul | Réponse avec `id: null` | Même comportement |
| Identifiant chaîne ou entier | Valeur reprise dans la réponse | Valeur préservée, y compris entier 64 bits non signé |
| Identifiant booléen, fraction ou structure | Pas de validation explicite | Erreur -32600 avec identifiant nul |
| Paramètres absents | Liste vide implicite | Liste vide implicite |
| Paramètres non tableaux | Expansion Python ou interprétation des valeurs fausses | Refus explicite -32602 |
| Lot vide | Liste vide | Erreur -32600 |
| Notification invalide à l'exécution | Réponse d'erreur avec identifiant nul | Pas de réponse |
| Élément de lot qui n'est pas un objet | Peut lever une exception hors du gestionnaire | Erreur -32600 pour cet élément ; traitement du reste |
| Méthode inconnue | Erreur -32601 | Erreur -32601 ; message adapté |
| Exception interne | Traceback renvoyé dans le message | Erreur générique -32603, sans traceback |

Ces différences sont explicites et ne constituent pas une parité exacte avec
les défauts de l'enveloppe Python. Elles sont distinctes de la compatibilité des
blocs historiques et de la sémantique propre à chaque méthode.

## Preuves d'exécution des lots

Le test RPC en mode développement envoie dans un même lot :

1. Une notification programmant le prochain timestamp.
2. Une méthode inconnue.
3. Une demande de création de bloc.
4. Une lecture de hauteur.

Il vérifie trois réponses, l'erreur de méthode inconnue, le timestamp effectivement
appliqué, le hash créé et la hauteur lue après la création. L'état est ensuite
restauré par snapshot. Ce test prouve une exécution séquentielle dans Nano et
l'absence d'atomicité globale du lot : l'erreur d'un élément n'annule pas les autres.

Les tests HTTP vérifient aussi le corps vide et le statut 204 d'un lot constitué
uniquement de notifications, ainsi que l'erreur renvoyée pour un lot vide.
Les tests d'identifiants et de formes de paramètres complètent ces scénarios.

Cet audit porte sur l'enveloppe, les lots et leur exécution. Les 61 méthodes
nécessitent leurs propres preuves de paramètres, d'effets et de résultats ;
`rpc-port-map.json` ne remplace pas cet audit.

## Historique des frais

La méthode Python `NanoPyNode.fee_history` renvoie des listes constantes :
`config.gas_price`, une utilisation de 0,5 et `max_priority_fee`. Elle ignore
`newest_block`, ne borne pas la réponse au genesis et ne pondère pas les
récompenses par le gaz consommé. L'adaptateur HTTP Python exige trois arguments.

Nano accepte deux ou trois arguments, avec une liste de percentiles optionnelle.
Il lit les blocs et reçus historiques, borne la plage au genesis, renvoie les
base fees observées plus la projection suivante, calcule les ratios réels et
pondère les tips effectifs par le gaz consommé. Un bloc vide produit des
récompenses nulles ; une liste de percentiles vide omet le champ `reward`.
Les percentiles doivent être numériques, finis, croissants (répétitions admises)
et compris entre 0 et 100. Maximum : 100 percentiles et 1 024 blocs demandés.

Les tests RPC utilisent des transactions legacy et type 2 avec consommations de
gaz différentes. Ils vérifient les percentiles entiers, fractionnaires et répétés,
la borne au genesis, un bloc historique demandé explicitement, la base fee
suivante et les erreurs de paramètres. La formule native est un écart explicite
avec les valeurs constantes Python ; elle ne doit pas être présentée comme
une réponse identique à celle du nœud source.


## `eth_createAccessList` : API et limites opérateur

Cette section décrit le contrat de l’intégration et sa qualification publique
ARM64 et AMD64 ciblée : sept groupes, huit lignes de scénario et quatre signatures
synthétiques par architecture ; différentiel Geth 1.16.7 ARM64 de 16/16 cas et 5/5
surcharges d’état. Les exemples de réponse ci-dessous restent illustratifs. Ces
preuves ne qualifient ni un déploiement, ni la reprise des duties, ni la production.

### Paramètres et résultat

La méthode accepte de un à trois paramètres positionnels :

```text
eth_createAccessList(transaction, blockNumberOrHash?, stateOverrides?)
```

`transaction` est un objet d’appel : `from`, `to` (omis pour CREATE), `gas`,
`value`, `input`/`data`, `nonce`, `chainId`, les frais legacy ou dynamiques, et un
`accessList` initial facultatif. Les adresses ont 20 octets ; les clés de stockage
ont 32 octets. Les quantités sont des chaînes hexadécimales canoniques. `input`,
s’il est fourni même vide, prend le pas sur `data`. Les champs nuls sont omis.
Le compte source doit disposer des fonds nécessaires au gaz et à la valeur dans
l’état simulé, éventuellement après une surcharge d’état.

Un paramètre : contexte `latest` implicite. Remplacer les adresses par celles du
contrat et d’un compte disposant des fonds dans la chaîne interrogée.

```json
{"jsonrpc":"2.0","id":1,"method":"eth_createAccessList","params":[{"from":"0x0000000000000000000000000000000000000101","to":"0x00000000000000000000000000000000000000a1","gas":"0x186a0","gasPrice":"0x77359400","input":"0x"}]}
```

Deux paramètres : contexte `pending` explicite.

```json
{"jsonrpc":"2.0","id":2,"method":"eth_createAccessList","params":[{"from":"0x0000000000000000000000000000000000000101","to":"0x00000000000000000000000000000000000000a1","gas":"0x186a0","gasPrice":"0x77359400"},"pending"]}
```

Trois paramètres : sélection EIP-1898 et surcharges isolées. Le bloc `0x10` doit
exister et son état historique doit être disponible. Les champs `stateDiff` et
`state` sont placés sur des comptes distincts dans cet exemple.

```json
{
  "jsonrpc": "2.0",
  "id": 3,
  "method": "eth_createAccessList",
  "params": [
    {
      "from": "0x0000000000000000000000000000000000000101",
      "to": "0x00000000000000000000000000000000000000a1",
      "gas": "0x186a0",
      "maxFeePerGas": "0xb2d05e00",
      "maxPriorityFeePerGas": "0x7"
    },
    {"blockNumber": "0x10"},
    {
      "0x0000000000000000000000000000000000000101": {
        "nonce": "0x1",
        "balance": "0x100000000000000000000"
      },
      "0x00000000000000000000000000000000000000a1": {
        "code": "0x6000545000",
        "stateDiff": {
          "0x0000000000000000000000000000000000000000000000000000000000000000": "0x0000000000000000000000000000000000000000000000000000000000000001"
        }
      },
      "0x00000000000000000000000000000000000000a2": {"state": {}}
    }
  ]
}
```

Forme d’un succès, avec un gaz illustratif qui dépend du contrat exécuté :

```json
{"jsonrpc":"2.0","id":1,"result":{"accessList":[{"address":"0x00000000000000000000000000000000000000a1","storageKeys":["0x0000000000000000000000000000000000000000000000000000000000000000"]}],"gasUsed":"0x7000"}}
```

Un REVERT ou une autre faute EVM renvoie aussi la liste et le gaz, avec un champ
`error` dans **le résultat**. Ce n’est pas une erreur de l’enveloppe JSON-RPC :

```json
{"jsonrpc":"2.0","id":1,"result":{"accessList":[],"gasUsed":"0x7000","error":"Execution reverted"}}
```

`gasUsed` est le gaz net de l’exécution avec la liste stabilisée, incluant les
frais intrinsèques de cette liste et le remboursement plafonné par le fork
(au plus un cinquième du gaz consommé dans le profil Shanghai). Il ne s’agit ni
de `eth_estimateGas`, ni d’une promesse d’inclusion ou de coût d’une transaction
future. Chaque tour peut suivre une branche différente lorsque le coût de la
liste modifie le gaz disponible ; les touches déjà observées sont conservées
jusqu’au point fixe. Les listes finales sont triées et dédupliquées.

### Contexte et isolation

- Le contexte omis ou nul est `latest`. `pending` doit être explicite : il capture
  le candidat et les transactions admissibles du pool puis les exécute sur copie.
- Un numéro historique, `earliest` ou `finalized` utilise l’état associé au header
  sélectionné, lorsque ce contexte est disponible. `safe` reste indisponible.
- EIP-1898 accepte un objet avec exactement `blockNumber` **ou** `blockHash`.
  `requireCanonical` est un booléen facultatif uniquement avec `blockHash`.
  Par exemple, remplacer le hash illustratif suivant par un hash connu :
  `{"blockHash":"0x1111111111111111111111111111111111111111111111111111111111111111","requireCanonical":true}`.
- Le magasin historique expose les états par hauteur canonique. Une branche
  non canonique sans état réellement associé est refusée, même avec
  `requireCanonical:false` ; aucun état canonique de même hauteur n’est substitué.
- Header, état et fenêtre BLOCKHASH sont capturés ensemble. Le contrôle de
  cohérence de racine, les surcharges et les tours EVM s’exécutent après libération
  du verrou RPC. Tous les tours repartent de la même copie initiale.

Les écritures, nonces et créations simulés ne sont publiés ni dans l’état
canonique, ni dans le pool. Les touches d’un sous-appel REVERT restent dans la
liste, bien que ses écritures soient annulées. Sender, destinataire et
précompilés actifs sont exclus des entrées compte sans stockage ; leurs clés
explicites ou découvertes restent présentes. La coinbase Shanghai n’est pas une
exclusion de cette collecte.

### Surcharges d’état et frais

Les surcharges s’appliquent avant les défauts de nonce et uniquement à la copie :

| Champ par compte | Effet et validation |
| --- | --- |
| `nonce` | Quantité uint64 remplaçant le nonce de l’état simulé |
| `balance` | Quantité uint256 remplaçant le solde simulé |
| `code` | Données hexadécimales avec préfixe ; taille cumulée bornée avant décodage |
| `state` | Remplace le stockage entier ; clés et valeurs de 32 octets |
| `stateDiff` | Modifie seulement les clés fournies ; clés et valeurs de 32 octets |

`state` et `stateDiff` ne peuvent pas être fournis ensemble pour un compte.
Les champs inconnus, dont `movePrecompileTo`, sont refusés. Pour CREATE, le nonce
d’état après surcharge détermine l’adresse exécutée ; le nonce des arguments,
s’il est fourni, détermine l’adresse d’exclusion du tracer. Ce sont deux usages
distincts dans le contrat Geth v1.16.7.

Ne pas mélanger `gasPrice` avec les champs dynamiques. Dans le profil Shanghai,
un `gasPrice` explicite nul est refusé ; un couple dynamique complet dont
`maxFeePerGas` est nul est également refusé. Un pourboire omis utilise la
[politique locale de frais](RPC-FEE-POLICY.md) et un plafond dynamique omis est
calculé avec ce pourboire et deux fois la base fee du bloc sélectionné. Les
plafonds explicites restent soumis aux validations et à la disponibilité des
fonds. Ces règles concernent le nouveau chemin ; les défauts de `eth_call` et
`eth_estimateGas` restent inchangés.

### Configuration et erreurs

Le fichier JSON de `nano serve` ou `nano run` accepte :

```json
{
  "rpcSimulation": {
    "timeout_ms": 5000,
    "maximum_access_list_iterations": 64,
    "maximum_access_list_addresses": 4096,
    "maximum_access_list_storage_keys": 65536,
    "maximum_access_list_override_code_bytes": 1048576
  }
}
```

| Champ | Défaut | Domaine opérateur |
| --- | ---: | ---: |
| `timeout_ms` | 5000 | entier 1–60000 |
| `maximum_access_list_iterations` | 64 | entier 1–1024 |
| `maximum_access_list_addresses` | 4096 | entier 1–65536 |
| `maximum_access_list_storage_keys` | 65536 | entier 1–1048576 |
| `maximum_access_list_override_code_bytes` | 1048576 | entier 1–16777216 |

Une clé inconnue, un booléen, une fraction ou une valeur hors domaine font échouer
la configuration avant le démarrage du runtime. Les bornes d’adresses et de clés
couvrent les entrées brutes des seeds/surcharges et les entrées uniques collectées.
Le gaz de l’appel est plafonné par celui du bloc sélectionné et par le `gas`
explicite s’il est inférieur. Ce sont des limites de travail de l’hôte RPC,
sans modification des limites de consensus.

Le délai est coopératif et partagé par préparation, surcharges, tours et
sérialisation. L’attente en file et l’acquisition initiale du verrou RPC restent
en dehors. Une copie d’état, un accès disque, un calcul de racine ou une routine
native déjà engagée n’est interrompu qu’au prochain contrôle : ce délai n’est
pas une borne stricte de temps réel. `timeout_ms` configure aussi les chemins
`eth_call` et `eth_estimateGas` qui emploient les mêmes limites de simulation.
Les autres bornes ci-dessus sont propres à la création de liste d’accès.

| Situation | Réponse |
| --- | --- |
| Paramètres, sélecteur, frais ou surcharge invalides | erreur JSON-RPC `-32602` |
| Bloc/état indisponible, fonds insuffisants ou gaz intrinsèque non couvert | erreur JSON-RPC `-32000` |
| Quota dépassé ou point fixe non atteint dans les tours autorisés | erreur JSON-RPC `-32005`, aucune liste partielle |
| Annulation ou délai de calcul expiré | erreur JSON-RPC `-32002`, aucune liste partielle |
| Faute EVM après préparation valide | `result.accessList`, `result.gasUsed` et `result.error` |

HTTP et WebSocket utilisent la file bornée commune aux simulations, notifications
comprises. L’admission de file peut refuser un lot entier par HTTP 503 ou par
l’erreur de capacité du transport avant le calcul. La télémétrie optionnelle
`prepare`, `access_list_execute` et `access_list_round` mesure les phases sans
enregistrer les paramètres ou les données de transaction.

### Références et périmètre

Le contrat suit les sources primaires [API Geth v1.16.7](https://github.com/ethereum/go-ethereum/blob/v1.16.7/internal/ethapi/api.go),
[arguments Geth v1.16.7](https://github.com/ethereum/go-ethereum/blob/v1.16.7/internal/ethapi/transaction_args.go)
et [traceur Geth v1.16.7](https://github.com/ethereum/go-ethereum/blob/v1.16.7/eth/tracers/logger/access_list_tracer.go).
### Résultats publics ARM64 et écarts Anvil

Les deux CTests ciblés ont réellement réussi sur ARM64, dont le nouveau
consommateur à sept groupes/huit lignes/quatre signatures synthétiques. Le
second CTest est la régression historique `evm_small_stack` ; sa fixture Cancun
n’active pas ce profil dans la chaîne Shanghai. Le différentiel primaire Geth
1.16.7 réussit 16/16 cas et 5/5 cas de surcharges d’état, avec listes canoniques et gaz net comparés exactement,
ainsi que la présence/catégorie des erreurs VM, sans imposer leur texte.
Le binaire Nano reste identique entre ces mesures.
Ce résultat porte sur les cas et contextes publics couverts, sans équivalence
générale avec Geth.

La première comparaison Anvil 1.5.1 reste **FAILED, 11/16**. Elle n’est ni effacée
ni reclassée comme réussite après le passage à l’oracle Geth. Les sources de la
version exécutée et les observations publiques distinguent les écarts suivants :

- Anvil collecte les touches d’une seule passe puis rejoue avec cette liste pour
  mesurer le gaz. Il ne recalcule pas les touches de cette seconde exécution.
  Geth cherche un point fixe ; Nano conserve aussi les touches des tours
  précédents, notamment quand une branche dépend du gaz disponible.
- Les REVERT observés deviennent des erreurs JSON-RPC chez cet Anvil. Le contrat
  Geth et Nano conserve au contraire `accessList`, `gasUsed` et `result.error`.
- Pour les seeds, la version Anvil exécutée conserve les entrées de comptes
  exclus et le dernier doublon d’adresse remplace le précédent. Le traceur Geth
  fusionne les clés des doublons et omet les comptes exclus dépourvus de clés.
- Dans ce lancement Anvil avec `--timestamp 100` sans genesis `--init`, le header
  annonce 100 mais une sonde TIMESTAMP exécutée dans l’EVM retourne **1**. Les
  sondes Nano et Geth retournent 100. Cet écart de contexte est propre au
  lancement observé ; un header de bloc seul ne prouve donc pas le contexte EVM
  employé par un oracle, notamment pour les calculs temporels de l’AMM.
- Pour un `gasPrice` explicite de 2 gwei, les sondes de ce même Anvil retournent
  GASPRICE = 2 gwei avec `eth_call`, mais GASPRICE = 0 pendant
  `eth_createAccessList`. Nano et Geth conservent 2 gwei dans les deux méthodes.
  Les deux contextes Anvil ne doivent donc pas être résumés comme identiques.

Anvil 1.5.1 confirme par ailleurs CREATE avec le nonce d’état, mais refuse le
troisième paramètre `stateOverrides` et accepte des frais nuls que les défauts
Geth v1.16.7 précités refusent. Le nouvel oracle emploie donc Geth comme référence
primaire et conserve Anvil comme diagnostic distinct ; il ne masque pas ces
écarts pour obtenir une parité artificielle. Les cas de surcharges sont désormais
comparés à un Geth réellement exécuté, en complément des oracles C++.

La parité des erreurs RPC n’est pas complète : pour `gasPrice:0x0` et le couple
explicite `maxFeePerGas:0x0`/`maxPriorityFeePerGas:0x0`, Nano et Geth refusent tous
deux, mais Nano renvoie `-32602` contre `-32000` chez ce Geth. Ces deux diagnostics
de frais restent hors des 21 comparaisons strictes et ne sont pas présentés
comme une égalité des codes ou des messages RPC.

Le périmètre décrit est Shanghai. Les types supérieurs à 2 et les extensions
blob/authorization ne sont pas activés par cette méthode. Aucune compatibilité
Cancun ou activation d’EIP supplémentaire n’est déduite de cet ajout. La
préparation VPS et le build CMake AMD64 ont également réussi : six paires M/MD
sélectionnées, deux CTests réussis, huit lignes/sept groupes/quatre signatures
synthétiques, RSS mesurée 23680 KiB (23108 KiB sur ARM64). Les résultats AMD64 sont
les compteurs ROOT filtrés ; les logs, tables SDK et bindings bruts restent sur
le VPS. L’usage final observé du nouveau volume est 208 KiB, sans mesure de pic
ou quota dur. Parents conservés, volume et conteneurs possédés absents. Aucun
différentiel Geth AMD64 n’est attribué à ce lot. Aucune qualification globale de
l’EVM, de la production ou d’une chaîne privée n’est annoncée ici.
