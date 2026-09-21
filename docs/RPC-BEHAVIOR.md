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
