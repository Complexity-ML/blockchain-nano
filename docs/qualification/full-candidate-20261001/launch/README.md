# Nouveau registre candidat ARM64 : préparé, tests actifs

Source immuable aacc56da, code équivalent à main intégré. Compilation complète
réussie ; 416 tests sélectionnés sur 416, aucun missing executable, TIMEOUT
explicite partout, aucun skip property. 195 exécutables distincts mesurés.
Manifests source après build et exécutables juste avant tests égaux aux
captures de préparation. Ce sont des preuves de préparation, pas de réussite.

Docker nano-full-rpc-positional-client-20261001-test confirmé vivant,
exec 62528. Logs /tmp/nano-full-candidate-arm64-20261001/tests.log.
Scratch sur volume disque exécutable, builder/volume existants, pas d’image
neuve ni réseau public. Les 400 résultats originaux (314/86) restent dans
full-regression-original-20261001. Outils/corpus facultatifs absents ne sont
pas couverts par ce registre ; aucune conformité Ethereum universelle déduite.
