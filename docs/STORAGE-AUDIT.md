# Audit explicite de la chaîne stockée

Après arrêt du nœud qui utilise la base :

```sh
nano audit genesis.json /chemin/vers/base-existante
```

La commande exige une base RocksDB existante et le genesis correspondant.
Elle ouvre la base avec son verrou exclusif habituel. Ce n’est pas une ouverture
forensique en lecture seule : RocksDB peut modifier ses fichiers de métadonnées.
Aucun bloc, état canonique ou journal de signature n’est réparé par l’audit.
Pour préserver les fichiers originaux, travailler sur une copie cohérente
réalisée à l’arrêt ; ne pas utiliser cette copie comme journal d’un producteur.

Une base temporaire distincte est créée dans le répertoire temporaire système.
L’audit rejoue chaque bloc depuis le genesis avec les règles configurées, y
compris les preuves PoS lorsque `posSignatureVersion` vaut 1. Il compare les
racines des états historiques, les registres de consensus, les index de blocs
et de transactions et tous les reçus. Il compare aussi le nombre de blocs et la
difficulté totale. Les timestamps sont validés avec l’heure système capturée au
début de l’audit ; l’horloge de la machine doit être correcte.

En cas de succès, le JSON contient `verified: true`, `blocks` (genesis inclus),
`transactions` (quantité hexadécimale), `blockHash`, `stateRoot` et
`posSignatureVersion`. Une incohérence provoque une sortie non nulle et indique
la hauteur lorsque l’erreur survient pendant le rejeu. Une erreur d’ouverture
ou du bloc de tête peut être détectée avant le début de cette boucle.

## Limites

- Le rejeu coûte le temps d’exécution complet de la chaîne et nécessite de
  l’espace temporaire pour une autre base complète. Son dossier est supprimé
  à la sortie normale ou après une exception ; un arrêt forcé peut le laisser.
- L’audit est explicite, pas exécuté automatiquement à chaque démarrage.
- Il suit les références de la chaîne canonique ; il ne recherche pas toutes
  les clés orphelines et ne certifie pas l’intégrité du journal de signature.
- Il réutilise le moteur local : il ne constitue pas une validation indépendante
  de l’EVM, du consensus ou des règles économiques.
- Un succès sur une chaîne historique non signée ne lui ajoute aucune preuve.
  La finalité, les sauvegardes et la résistance aux pertes d’alimentation
  restent des validations distinctes.

Les suites `block_auth` et `signed_validator_cli` vérifient un rejeu sain,
l’absence de création d’une base manquante, une preuve altérée dans un ancien
bloc malgré une tête valide et un reçu divergent de la réexécution.

## Cohérence des index lors des lectures ordinaires

Les lectures par hauteur refusent un bloc stocké sous une autre hauteur. Les
lectures par hash vérifient à la fois le hash demandé et la hauteur ciblée par
l’index. La lecture de la tête vérifie également que sa hauteur correspond à
la clé `meta/tip`. Ces contrôles s’appliquent sans lancer l’audit complet.

La suite `block_auth` altère les index pour qu’ils désignent un autre bloc
correctement formé et signé. Les lectures concernées échouent explicitement,
au lieu de renvoyer ce bloc comme s’il était celui demandé. Ces contrôles ne
prouvent pas la finalité et ne remplacent pas le rejeu de l’historique.
