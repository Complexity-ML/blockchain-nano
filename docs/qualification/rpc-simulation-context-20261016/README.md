# Contexte des simulations RPC — ARM et AMD

Rapport du **17 octobre 2026** ; les chemins de qualification gardent leur suffixe du 16 octobre, date de démarrage du lot. **Les tests Nano ciblés passent sur Linux ARM64 et AMD64. La comparaison Anvil02 est complète et reste FAILED : 116 divergences.** Les [75 fichiers publics Root](evidence/manifest.json), totalisant 616 839 octets, conservent les résultats et les échecs précédents. Aucun état privé, table SDK complète, clé opérationnelle ou binaire n’est publié ici.

## Correctif et résultats ciblés

Chaque simulation prépare une copie de l’état : achat initial du gaz au prix effectif, solvabilité au fee cap, nonce de l’état avancé avant exécution, valeur transférée et BASEFEE nul lorsque le prix effectif est nul. La validation borne les quantités avant la capture de l’état. Les valeurs par défaut des listes d’accès et leur remboursement restent distincts de call/estimate.

[ARM09](evidence/arm/result-public.json) et [AMD01](evidence/amd/host/result-public.json) passent chacun quatre groupes, **476 checks, 126 refus attendus et 258 enregistrements publics** sous Paris/Shanghai. Le nouveau test seul dure 78 ms sur ARM et 128 ms sur AMD. Les quatre baselines réelles échouent comme attendu sur chaque ancien Core, aux checks 3/2/2/1 : frais, soldes, nonce et bornes. Les [119 527 octets de résultats candidats](evidence/cross-architecture.json) sont identiques entre les deux architectures, SHA256 `96ca4c5586e5f53c3417fc5642c95f908036e5e179ad81b0865c423c9d4806cb`.

Trois régressions passent sur chaque architecture : RPC **1 053 checks**, liste d’accès **sept groupes**, EIP-1898 **cinq groupes/120 checks/58 refus**. Les sorties RPC et EIP-1898 sont identiques entre architectures. Les pics RSS de la liste d’accès sont 108 080/108 112 KiB ; ses quatre signatures ECDSA par exécution sont des signatures synthétiques de fixture, sans rôle de consensus.

Les [continuités ARM](evidence/arm/core-member-continuity.json) et [AMD](evidence/amd/worker/core-member-continuity.json) établissent **143 membres, 142 inchangés, seul rpc.cpp.o remplacé** :

| Core candidat | SHA256 | Octets |
|---|---|---:|
| ARM64 | `43b6b8ee60f7b614117dc46c79f80eb3cdf930202d2d32c35bc24983aa8f49f5` | 41 040 028 |
| AMD64 | `2678b628528295dd7d729e8917572d3076a1068399e58b12f97fa3849bb497bd` | 40 805 844 |

Les clôtures finales conservent 937 entrées sélectionnées sur ARM et 624 sur AMD, sans rafraîchir les autorités anciennes ; les contrôleurs terminent avec succès et leurs conteneurs sont absents. Ce lot ne reconstruit aucune image ni tout le Core. Le runtime Mac natif n’a pas été relié au nouveau Core.

## Comparateur Anvil02 : échec conservé

Le [rapport Anvil](evidence/anvil/attempt02-result-public.json) et la [trace](evidence/anvil/attempt02-comparisons.jsonl) couvrent les 258 cas, 129 sous Paris et 129 sous Shanghai, sur Anvil/v1.5.1. **passed=false, exit1** : 116 comparaisons divergent, dont 14 valeurs de retour et 102 classes de résultat/refus ; aucune n’est excusée par la politique zéro-frais des listes d’accès. Quatre différences supplémentaires de quotes estimateGas sont non bloquantes selon cet oracle. Les 142 autres comparaisons passent selon ses critères ; cela n’établit aucune parité globale.

Un cas zéro-frais renvoie BASEFEE0 côté Nano contre 1 Gwei côté Anvil. Les sources primaires Geth v1.16.0 donnent une piste : [CallDefaults/ToMessage](https://raw.githubusercontent.com/ethereum/go-ethereum/v1.16.0/internal/ethapi/transaction_args.go) gardent le prix nul lorsque les caps sont nuls ; [applyMessage](https://raw.githubusercontent.com/ethereum/go-ethereum/v1.16.0/internal/ethapi/api.go) met alors le BASEFEE simulé à zéro. Cette lecture de source n’est ni une mesure sur Geth exécuté, ni une explication acquise des 116 divergences. Anvil01, refusé en readiness avant toute comparaison, reste conservé séparément.

## Harness CI et historique

Les dix groupes du harness sont qualifiés dans deux contrôles : [neuf gardes V2 dans CI07](evidence/ci/attempt07-stdout.log), puis [daily-shell V2 dans CI05](evidence/ci/attempt05-daily-guard.log). Les deux terminaux passent avec changed=[] sur les mêmes entrées conservées. Il ne s’agit pas d’une invocation unique à 10/10, ni d’une CI GitHub verte : ces contrôles locaux Docker n’exécutent pas les tests C++ du Core. Python 3.12.3 a été installé pour CI05 dans un conteneur éphémère supprimé après déconnexion réseau et test, sans construire d’image.

Les terminaux CI01–04 restent FAILED avec leurs écarts de métadonnées sur cinq nouvelles copies ; leurs assertions vertes ne qualifient pas ces contrôleurs. CI06 reste FAILED : la sélection négative retenait aussi le parent du fichier et lançait le garde quotidien sans Python ; neuf assertions passaient, mais l’invocation échouait. La sélection positive CI07 ferme ce défaut sans effacer les essais précédents.

L’[historique C++](evidence/history-summary.json) conserve **sept refus d’outils et un échec d’oracle** : métadonnées01–03, header manquant04 après quatre baselines réelles, copie Core non modifiable05, ctime du candidat06–07 ; puis08, où CREATE enfant zéro ne faisait légitimement pas échouer son parent. La fixture V2 exige désormais ce refus parent. Aucun de ces huit essais n’est requalifié rétrospectivement.

## Portée restante

Les sources et scénarios testés sont qualifiés dans les limites ci-dessus. Seul rpc.cpp est modifié dans le Core de ce lot ; les sources EVM restent inchangées. Les preuves historiques EEST et Doppler restent acquises dans leur périmètre : elles ne sont ni annulées ni élargies, et aucun rejouage automatique n’est exigé. Ce lot ne qualifie pas davantage HTTP, EEST, le SDK global ou le configure CMake complet. Les exigences réellement restantes sont la classification des divergences Anvil et une comparaison Geth exécuté, l’assemblage Native et les admissions runtime après ce Core, le cycle de campagne avec reprise/duties/finalité, les opérations, le bridge/lancement et la revue indépendante. La CI GitHub reste bloquée par la facturation. Aucune promotion EIP ou production n’est déduite de ce lot.
