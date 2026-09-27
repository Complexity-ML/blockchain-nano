# Frais RPC : exclusion du producteur et charge bornée

## Défaut reproduit et corrigé

Au commit d808db6, `sender()` retourne une adresse avec checksum tandis que
`address(header.coinbase)` est normalisée en minuscules. La comparaison de
chaînes laissait donc entrer des transactions du producteur dans les échantillons.
Le scénario à vingt blocs retourne 1 wei au lieu de 101 : voir l’échec initial
et le diagnostic conservés. Le correctif normalise les deux opérandes.

Le test précédent du producteur utilisait un pourboire élevé, donc hors des trois
échantillons les moins chers : il ne prouvait pas cette exclusion. Son entrée
est corrigée à 1 wei, inférieur aux pourboires externes. Le nouveau scénario
inclut 64 transactions du producteur à faible pourboire par bloc.

## Preuves après correction

Les cinq tests RPC ciblés passent en 9,80 s (journal `corrected-regressions.log`).
Le scénario de charge est ensuite paramétré pour un second volume ; sa version
finale est rejouée seule avec succès en 1,11 s. Les autres sources et binaires
RPC testés n’ont pas changé entre ces deux passages.

| Mesure locale | 20 × 128 transactions | 20 × 1 428 transactions |
| --- | --- | --- |
| Transactions exécutées et stockées | 2 560 | 28 560 |
| Gas par bloc, tous remplis | 2 688 000 | 29 988 000 |
| Trois calculs sans cache de recommandation | 86,802 / 81,868 / 80,741 ms | 652,874 / 651,291 / 652,085 ms |
| 1 000 consultations en cache | 5,314 ms au total | 5,385 ms au total |
| 8 threads, 2 048 requêtes incluant un calcul sans cache | 118,982 ms au total | 950,740 ms au total |

Chaque consultation vérifie le pourboire attendu de 101 wei ; les requêtes
concurrentes alternent estimation et lecture de hauteur. La tête et la mempool
restent inchangées. Les blocs sont réellement admis, exécutés et persistés dans
une base RocksDB temporaire ; la fixture utilise le producteur local de tests,
pas le consensus Beacon de production.

## Commandes

```sh
ctest --test-dir build-release -R '^rpc_fee_load$' -V
build-release/nano-rpc-fee-load-tests 1428
```

## Limites ouvertes

Le terme sans cache concerne le cache de recommandation : les caches RocksDB
et du système d’exploitation ne sont pas purgés. Aucun HTTP, P2P, VPS, flux
continu de nouveaux blocs ni consommateur malveillant n’est exercé. Ces temps
sont des observations locales, pas des seuils garantis ou un débit réseau.

En particulier, le calcul d’environ 652 ms détient le mutex RPC et peut retarder
les autres appels. Une nouvelle tête invalide le cache global ; le coût répété
à chaque bloc et l’équité des appels restent à traiter/qualifier. La fenêtre
maximale configurée de 1 024 blocs n’est pas mesurée ici. Aucun changement de
consensus et aucune clôture globale de production.
