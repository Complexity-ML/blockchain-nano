# Optimisation RPC mesurée — qualification AMD64 bornée

Candidate `d1597d9a6987d0839ed2f0c7ded268aead35d071`, branche `qualification/rpc-headroom-margin-20261002`, image AMD64 `sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14`. Compilation incrémentale dans le volume existant ; contrôleur terminé et conteneurs temporaires retirés. Aucun build d'image supplémentaire.

La sonde proche de la borne haute utilise au maximum la tolérance existante 1,5 % de high. Si elle échoue, l'intervalle suffit déjà au critère d'arrêt ; si elle réussit, la recherche poursuit avec cette borne réellement vérifiée. Plafonds caller/solde/bloc, simulation initiale, règles EIP-150, contexte, état isolé et deadline conservés. Candidate non promue sur main ; régression complète et autre architecture encore requises.

## Preuve fonctionnelle et contrôle négatif

Baseline `911c7f415eb325a8b9005066e0a74b69d896155e` contient les mêmes nouveaux tests et l'ancien algorithme. CTest RPC rejette exactement le nouveau critère de performance : Paris profondeur 4, 2013 checkpoints d'estimation pour 415 d'appel. Ce rejet attendu n'est pas un nouveau défaut de production. Avec la correction : 1609 checkpoints pour les mêmes 415, critère renforcé réussi.

RPC optimisé **PASS 2.90 s, 1053 vérifications**. Nouvelles profondeurs 2/4/8 pour Paris et Shanghai : réponse exécutée, transaction signée réussie, écritures RPC non commises, gaz minimal recherché indépendamment par eth_call, écart <=1,5 %, plafond serré respecté et plafond inférieur au minimum rejeté. Autres vérifications RPC existantes (frais, pending, cancellation et contrats) également passées dans ce binaire ; cela ne certifie pas les profils publics non activés.

## Mesure réseau

Cas WebSocket Paris original, 16 clients, assertions et timeout 480 s inchangés : **PASS 71.03 s**. Les 192 estimations comportent toutes **quatre sondes**, contre cinq dans le diagnostic précédent `3fa8b805`. Coût exact en simulations réduit de 20 % dans ce scénario ; médiane d'estimation observée 1620.740 ms contre 2103.730 ms, attente de file maximale 2595.770 ms contre 3197.998 ms. La limite de file reste 4000 ms. Exécutions séparées et instrumentées : ordonnancement/charge peuvent affecter les durées ; aucun pourcentage garanti de débit ou de réduction de latence globale.

## Intégrité et limites

Audit indépendant de l'archive : deux manifests de 15 315 sources stables (baseline/candidate), quatre programmes stables dans chacun des deux passages, échec négatif attendu, deux succès, recomputation des statistiques depuis quatre JSONL, six couples fork/profondeur et leurs bornes indépendantes vérifiés. Archive SHA-256 `a0cffab50ada8e2cef68a5d72a7e91afa2ce3c90e75c41e3c8ee7c0007fae174`.

L'inefficacité de recherche est corrigée et qualifiée sur cette tranche. L'origine complète de l'expiration historique n'est pas démontrée ; elle reste archivée et ouverte, notamment avant une régression sans instrumentation. Aucune régression complète neuve, restauration des duties anti-double-signature, finalité WAN, qualification deux architectures ou production démontrée. Tous les autres critères du goal et des matrices restent ouverts selon leurs statuts.
