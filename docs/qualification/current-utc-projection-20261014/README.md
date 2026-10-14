# Diagnostic Capella à UTC réel, en lecture seule

Source proposée pour `tools/consensus/project-current-utc.cpp`, cible
`nano-capella-current-utc-projection`. La cible CMake est ajoutée seulement :
elle n’a pas encore été configurée/compilée par CMake, installée ni intégrée à
une image. Le même CPP d496 a été compilé et lié séparément sur les archives
qualifiées Docker Linux ARM64/AMD64 ; aucun cœur n’a été reconstruit pour ce
diagnostic. Les sources CPP et générateur sont copiées byte-exact.

Une compilation standard future depuis le dépôt utilise :

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DMCL_PORTABLE=ON
cmake --build build --target nano-capella-current-utc-projection
```

Usage du diagnostic :

```
build/nano-capella-current-utc-projection --request REQUEST_ABSOLU REQUEST_SHA256
```

Le request strict fournit snapshot/profile/Genesis/extraction et leurs pins,
les engagements déclarés et des budgets finis. Le diagnostic lit l’UTC réel
lui-même, authentifie le snapshot avec les APIs du cœur et projette en RAM.
Il ne vérifie pas indépendamment la provenance externe de l’extraction et
n’autorise aucune reprise. Il n’ouvre ni DB, journal, clé/env ou réseau ; il ne
signe pas, ne publie pas une horloge et n’active pas l’allowance startup2h/60.
Un résultat projeté n’est pas une preuve de duties, finalité ou production.

Pour préparer les huit cas PUBLICS SYNTHÉTIQUES horsE :

```
python3 -B tests/capella/current-utc-projection-public-cases.py \
  --fixture /chemin/canonique/depot/tests/fixtures/capella-bootstrap-anchor-cb532 \
  --output /chemin/canonique/neuf/public-cases \
  --runtime-root /chemin/canonique/neuf/public-cases
```

Le générateur ne lance pas le diagnostic. `cases.json` fournit le request et
la SHA à passer au binaire pour chaque cas. Sept refus doivent correspondre
exactement à raison/stage ; `real-utc-observation` reste une observation,
jamais un succès forcé. Watchdogs qualifiés :5s par refus et80s pour UTC,
budget de calcul60s/128MiB. Ces fixtures datées ne promettent pas les mêmes
résultats de projection pour une date future. ROOT a exécuté une seule fois
ces huit cas sur chaque architecture ; aucun CTest/ancien banc n’a été rejoué.

Les quatre préfixes finaux sont composés avec le corps historique intégral ;
les pins de suffixe et du CSV sont dans document-preservation.json. La cible
CMake reste non configurée/non installée. ARM : résultats et logs publics bruts
relus indépendamment. AMD : seulement le rapport filtré autorisé et
l’attestation ROOT in situ ; aucune prétention de lecture indépendante des
logs complets. Le refus automatique de capture massive et la correction
SDK V1 non exécutée sont conservés dans history.json. Les ressources ci-dessus
concernent ces fixtures publiques, jamais une preuve de reprise privée.

Les deux nouveaux terminaux build et les deux parents composites sont copiés
byte-exact ;1 TU et1 lien par nouveau build,0 cœur/CMake/ar. Les gros inventaires
publics et SDK restent locaux avec leurs hashes explicites, sans duplication.
Aucun objet, exécutable, archive, DB, .env, clé, demande/extraction privée ou
rapport de projection réelle VPS n’est inclus dans ce paquet.
