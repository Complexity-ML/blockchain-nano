# eth_createAccessList — qualification publique ciblée du 15 octobre 2026

La méthode RPC native construit une liste d’accès par point fixe monotone sur copies isolées, avec le gaz net de la liste finale. Latest/pending/historique/EIP-1898, surcharges d’état et limites opérateur sont décrits dans [l’API](../../RPC-BEHAVIOR.md). Le layout des limites RPC change ; les consommateurs concernés sont reconstruits par le graphe CMake réel.

| Périmètre | ARM64 Docker | AMD64 Docker/VPS |
| --- | --- | --- |
| CTests sélectionnés | 2 PASS | 2 PASS |
| Nouveau consommateur | 7 groupes, 8 lignes, 4 signatures synthétiques | mêmes nombres |
| RSS mesurée du consommateur | 23108 KiB | 23680 KiB |
| Référence Geth 1.16.7 | 16/16 cas + 5/5 surcharges | non exécutée dans ce lot |

ARM est relu directement sur preuves publiques et par une contrelecture indépendante. La fermeture réelle comporte 143 objets/MD courants : 66 unités affectées toutes reconstruites, 67 compilations core dont un KZG supplémentaire de cause non établie par la revue, 70 CPP au total et 4 marqueurs de lien. Les six paires M/MD sélectionnées sont authentiques ; 76 objets restent identiques. AMD est décrit uniquement par le stdout ROOT filtré : préparation 44903 ms, configure+build 423262 ms avec 6 paires M/MD, puis contrôle des deux CTests 29636 ms. Aucun nombre total de compilations AMD ou temps du consommateur AMD n’est déduit. L’usage final du volume Linux possédé est 208 KiB, sans mesure de pic ni quota disque dur. Parents conservés et volumes/conteneurs possédés absents.

L’ancien binaire refuse séparément la nouvelle méthode avec -32601. Le nouvel objet C++ dépend de nouvelles API et n’est pas prétendu identique à un objet causal relié à l’ancien cœur. Un refus de préconfiguration ARM est aussi conservé : modification de métadonnées .DS_Store, avant création du dossier configure, compilateur ou test. L’admission distincte suivante exclut seulement deux métadonnées de bureau non sources ; les autres lignes anciennes restent exactes sans actualisation. Ce refus n’est pas une erreur C++. Aucun ancien groupe RPC/AMM complet n’est rejoué. `evm_small_stack` est une régression historique sur fixture Cancun et ne modifie pas le profil de chaîne Shanghai.

**L’oracle Anvil V1 reste FAILED 11/16.** La version 1.5.1 collecte une fois puis rejoue le gaz, convertit les REVERT en erreurs RPC et traite différemment les seeds exclus/doublons. Dans ce lancement sans genesis --init, le header annonce timestamp 100 mais l’EVM exécute timestamp 1. Une autre sonde mesure GASPRICE de 2 gwei pour eth_call contre 0 pour eth_createAccessList ; les contextes Anvil ne sont pas identiques. Le Geth primaire et Nano observent timestamp 100/GASPRICE de 2 gwei et le même point fixe. Anvil refuse aussi le troisième paramètre et diverge sur les frais nuls. Les 21 comparaisons strictes vérifient les listes canoniques, le gaz net et la présence/catégorie des erreurs VM, pas tous les messages/codes RPC. Deux diagnostics de frais nuls hors de ces 21 cas restent différents : Nano -32602 contre Geth -32000, bien que les deux clients refusent. Le différentiel strict Geth 16+5 passe avec le même binaire Nano ARM que les campagnes précédentes ; il ne requalifie pas Anvil ni une parité globale de l’EVM. Voir [les diagnostics conservés](evidence/anvil-v1-failure-and-v2-context-diagnostics.json).

Sélection standard reproductible après configuration dans un environnement de dépendances admis :

```sh
cmake --build build --parallel 2 --target nano nano-rpc-access-list-tests nano-evm-stack-tests
ctest --test-dir build --no-tests=error -R '^(rpc_create_access_list_shanghai|evm_small_stack)$' --output-on-failure
```

Le différentiel ciblé est `tests/hardhat/scripts/compare-rpc-access-list-geth.mjs`, avec Nano, Geth 1.16.7, Anvil1.5.1 et l’artefact public Uniswap V2 Pair 1.0.1 épinglés par l’opérateur. Il utilise uniquement des chaînes synthétiques sur loopback et ne diffuse aucune transaction. Le [contrat C++ figé](CONTRACT-cpp.json) conserve son ancienne définition de l’oracle Anvil comme histoire V1 ; la référence qualifiée actuelle est le [contrat Geth V2](CONTRACT-reference-v2.json). Les contrôleurs ROOT, cartes SDK/MD, ELFs et logs bruts VPS ne font pas partie de ce paquet.

Les quatre documents de mission gardent leurs corps historiques entiers sous un préfixe additionnel ; le CSV conserve son état antérieur en snapshot ; ROOT modifie uniquement les deux cellules Preuves/Reste_a_faire de la ligne EIP2930, sans changer Statut, Périmètre, Priorité ni les autres lignes. Les sources et preuves sont liées par [source-bindings.json](source-bindings.json), [scoped-results.json](scoped-results.json) et les reçus publics bornés. Aucun commit/push/pull n’est inféré de cette proposition. Les originaux privés/Mac/NAS, V37 FAILED, anciennes clés arrêtées, duties/readiness, finalité commune de trois nœuds, WAN, strict 4 MiB, perte électrique/restauration complète, CI et audit externe restent ouverts. Aucune qualification RC complète, EIP complet, profil supplémentaire ou production n’est annoncée.
