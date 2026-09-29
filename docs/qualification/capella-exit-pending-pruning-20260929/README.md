# Sortie jamais incluse et élagage — 29 septembre 2026

Le test `consensus_capella_exit_pending_pruning` passe sur macOS : 1/1,
32,56 secondes. Sources candidates et binaire identifiés dans `manifest.json`.

## Couverture nouvelle

Deux propriétaires d’état C++ indépendants suivent les mêmes blocs. Une sortie
signée du validateur 0 est admise seulement par le suiveur ; le producteur ne
la connaît pas, ce qui maintient l’opération en attente sans désactiver les
règles d’inclusion. Après progression de la finalité, le test supprime réellement
l’état d’admission par élagage. Il vérifie :

- le validateur n’était pas déjà sorti avant l’élagage ;
- la demande conserve exactement ses octets et son index après réouverture ;
- son état d’admission est remplacé par l’état finalisé conservé ;
- une resoumission identique ne réécrit pas le checkpoint ;
- le prochain bloc proposé par le suiveur inclut automatiquement la sortie ;
- le rejeu depuis les états élagué et non élagué produit le même état ;
- l’archive reste présente tant que cette inclusion n’est pas finalisée.

## Limites

Banc natif local, sans transport P2P ni panne physique. Ce scénario utilise
l’élagage explicite, pas une politique périodique automatique. Il ne qualifie
pas le paiement ni tous les points de coupure disque. Les autres campagnes
restent nécessaires à la clôture du jalon.

## Reproduction

```sh
cmake --build build-release --target nano-capella-transition-tests -j2
ctest --test-dir build-release -R '^consensus_capella_exit_pending_pruning$' --output-on-failure
```

Les lignes des logs textuels sont normalisées uniquement pour retirer les
espaces en fin de ligne ; le patch gzip conserve les octets exacts du diff.

Régression du banc partagé : `consensus_capella_pruning_bls` et
`consensus_capella_exit_retirement`, 2/2 réussis en 99,92 secondes
sur le même binaire candidat.
