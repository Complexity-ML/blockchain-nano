# Allocation RocksDB — 2026-09-26

Le scénario précédent ouvrait 66 instances par nœud : base principale, témoin,
32 journaux de signature et leurs 32 témoins. Les mesures `du` représentaient
l'espace alloué, pas uniquement les états EVM ou les octets de WAL à rejouer.

Un reproducer Linux `tests/qualification/rocksdb-footprint.cpp`, exécuté dans
`nano-build-dependencies:local`, ouvre 66 bases avec les options de ChainStore,
écrit 100 octets par base avec `sync=true`, mesure les tailles tant que les bases
sont ouvertes, puis les rouvre et vérifie les valeurs. Trois configurations :

| Option | Octets logiques totaux | Octets alloués totaux | WAL alloué |
| --- | ---: | ---: | ---: |
| Défauts RocksDB | 1 726 906 | 5 151 522 816 | 4 872 265 728 |
| `allow_fallocate=false` | 1 726 972 | 2 973 696 | 270 336 |
| `write_buffer_size=4 MiB` | 1 726 642 | 583 925 760 | 304 668 672 |

Les 198 valeurs sont retrouvées après réouverture. Les résultats expliquent
un mécanisme suffisant pour reproduire l'ordre de grandeur précédent ; ils
ne constituent pas une ventilation rétrospective des anciens volumes supprimés.

Modification : `ChainStore` désactive la préallocation (`allow_fallocate=false`).
WAL, écriture synchrone, vérification des checksums et politique de récupération
restent inchangés. Pas de modification de `max_total_wal_size` ni de pruning.
L'allocation à la demande peut avoir un coût de fragmentation/latence, à mesurer
sur les supports de production.

Trois tests natifs ciblés passent (14,67 s) : journal de signature, récupération
avec témoin et reprise du runtime Bellatrix. Le test de récupération comprend
les erreurs de synchronisation injectées sur macOS. Ce résultat ne prouve pas
la résistance à toutes les corruptions matérielles.

Le rejeu Docker à 24 transactions est suivi séparément. Le redémarrage RPC du
premier scénario était d'environ 3 s (dont 2 s d'attente volontaire du test),
alors que les 236 s mesuraient le délai supplémentaire jusqu'à la réussite du
contrôle de finalité. Ne pas attribuer ce délai au rejeu du WAL sans mesure.

Références :
- https://github.com/facebook/rocksdb/blob/main/include/rocksdb/options.h
- https://github.com/facebook/rocksdb/wiki/Write-Ahead-Log-(WAL)

Commande du reproducer (image de dépendances locale, aucune donnée existante) :

```sh
docker run --rm --mount type=bind,source="$PWD/tests/qualification/rocksdb-footprint.cpp",target=/tmp/test.cpp,readonly nano-build-dependencies:local sh -c 'c++ -std=c++20 /tmp/test.cpp -lrocksdb -o /tmp/test && /tmp/test'
```

## Rejeu réseau terminé

Le même scénario à 24 transactions passe avec l'image corrigée : SIGKILL 137,
même conteneur/volume et préfixe finalisé conservés, nonce/compteur exacts,
bloc 39 finalisé commun. Mesures finales : 116 332 et 51 756 Kio alloués,
contre 5 041 244 et 5 038 800 Kio au précédent essai. Les instants/volumes
de travail ne sont pas strictement identiques ; les mesures varient entre flushs.

Kill → réouverture : 3,216 s (contre 3,217 s), dont 2 s de pause volontaire.
Réouverture → clôture du contrôle de finalité : 187,785 s (contre 236,290 s).
Un seul essai et des phases de slots différentes ne démontrent pas un gain
de finalité causé par l'option. Les rapports complets sont dans
`../kurtosis-allocation-20260926/`.
