# Qualification Mac natif + Docker Mac + Docker VPS

Cette campagne utilise trois processus de nœud sur deux machines physiques.
Elle n’est pas une qualification à trois domaines de panne indépendants.
Les images ARM64 et x86-64 sont construites depuis le commit Git `b7e2748` ;
le moniteur et la préparation des fixtures sont versionnés séparément.

`prepare.mjs` transforme une sortie fraîche de `--export-kurtosis` en une
partition disjointe des 64 clés **publiques synthétiques**, réparties entre les
instances déclarées dans le tableau JSON de topologie. Chaque entrée déclare
`name`, `host` (domaine de panne), `mode` (`native` ou `docker`), `rpcPort` et
`p2pPort`. Une entrée native fournit aussi `p2pBinary` et `runtimeDirectory`
absolus. L’outil refuse un dossier de sortie existant et vérifie chaîne 1337,
identité des clés synthétiques, unicité des indices et partition non vide.

```sh
node tests/multihost/prepare.mjs exported-fixtures fresh-output topology.json
```

La configuration de consensus, le genesis et le snapshot sont copiés sans
modification. Seuls les chemins opérationnels et la répartition des clés changent.
Les bootnodes doivent être renseignés avec le véritable peer ID observé au
lancement. Les RPC restent sur loopback ; les observations Docker passent par
`docker exec` ou un transport privé. Ne jamais financer ces comptes.

Le moniteur `tests/assertoor/stability.mjs` exige maintenant l’accord de toutes
les instances fournies, sans limiter la comparaison aux deux premières. Les
13 tests passent, dont troisième instance en retard et divergence d’exécution.
Ce sont des tests du moniteur, pas un résultat du réseau réel.

Vérification préparatoire réalisée avec le véritable exporteur natif :
64 indices uniques distribués 22/21/21, deux domaines de panne, fichiers
préparés sans lancement réseau. Ces fixtures de vérification doivent être
régénérées juste avant la campagne : le genesis dépend de l’horloge.

## Critère de campagne

Vérifier le commit et l’architecture des binaires, les connexions réelles,
la progression et trois avancées consécutives du même checkpoint finalisé
sur **les trois instances**, liées aux mêmes blocs d’exécution. Conserver
observations, latences, empreintes et logs, y compris en cas d’échec.
Les changements de connexion et crashs viennent ensuite, sur ces seules
instances isolées. Aucun résultat multi-hôte n’est encore revendiqué.
