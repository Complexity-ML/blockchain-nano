# Rétention RPC et simulation Doppler par hash — 16 octobre 2026

La source du test est dans le commit `583c58952efde5ccbc033750a06d5a6f7ae056bc`.
Le cœur C++ reste la sélection `bd11bd9f9dedf3f003b707d1c38fb1ecbabae91d`.
Les preuves ci-dessous ont des périmètres distincts. Aucune ne clôt la production.

## Rétention réelle et RPC : PASS Docker ARM

Le mode CTest `rpc_pruning_retention_boundaries` du binaire existant
`nano-rpc-simulation-block-id-tests --pruning-boundary-self-test` ajoute trois
scénarios : **112 vérifications, 20 refus attendus**, six transactions de fixture
avec une clé de test publique. Il utilise la vraie publication atomique
`commit_execution_checkpoint`, sans suppression brute de clés RocksDB :

- frontière `before=3`, ancre de rollback `6` : numéro/hash 2 refusés, 3 conservé ;
- une simulation capturée à 3 termine avec son état et ses ancêtres pendant que
  la rétention avance à `before=5` ; les nouvelles lectures de 3 sont refusées ;
- blocs, transactions, reçus et archives restent présents ; réouverture en lecture
  seule, engagement externe exact, ancien token refusé, rollback sous 6 refusé,
  checkpoint à 6 accepté et réouvert.

Un seul M indépendant, CXX, lien, contrôle loader et passage du nouveau mode ont
été exécutés. M et MD concordent sur 463 dépendances. Compilation 4001 ms,
exécution 55 ms ; archive Core `e2127db4…` inchangée. L'image ARM existante est
réutilisée, sans construction d'image ni rejeu des cinq anciens groupes. Les
21 fichiers sélectionnés passent leur contrôle final ; conteneurs propres absents,
volume de sortie conservé. Les 143 membres Core ne sont pas requalifiés ici.
L'ancien mode est restitué octet pour octet en retirant les deux insertions ;
CMake ajoute seulement deux lignes CTest. La revue a corrigé avant compilation
un PUSH1 de la nouvelle fixture : BLOCKHASH(2), et non BLOCKHASH(1).

Cela qualifie cette intégration locale. Le mode n'a pas été exécuté sur Mac natif
ou AMD dans ce lot. Il ne qualifie pas Beacon/WAN, une coupure physique, la reprise
des duties ou la récupération d'espace SST. `historicalStatePruning` reste opt-in ;
le défaut archive et les limites du CLI de compaction/GC de blobs sont conservés.

## Base : surface des sélecteurs observée

[Dix lectures publiques](evidence/base-selector-result.json) sur l'endpoint officiel
le 16 octobre à 13:26 UTC acceptent le même `{blockHash,requireCanonical:true}`
pour getCode, call decimals USDC (6) et estimateGas (30958). Les trois hash inconnus
et la hauteur future sont refusés avec -32001 ; le contrôle final retrouve le même
hash. Cela ne démontre pas une exécution historique arbitraire ni le refus d'un
orphelin connu pendant une vraie reorg. Le tag finalized est une assertion RPC.

## Doppler : création synthétique simulée au même hash

Le wrapper courant et le SDK installé 1.0.43/viem 2.33.3 ont effectué une seule
simulation de la fixture publique. Seul referenceTimestamp est actualisé dans une
nouvelle copie ; aucun ancien artefact ou terme réel n'est remplacé. Douze appels
strictement en lecture : identité, deux headers, six codes, deux calls et une
estimation. Les neuf sélecteurs par hash sont identiques ; call/estimate utilisent
le même calldata. Token/hook retournés concordent avec les prédictions du SDK.
Gas estimé **10 912 762**, contrôle final du header concordant.

[Résultat](evidence/doppler-strict-result.json),
[revue unsigned synthétique](evidence/doppler-unsigned-review.synthetic.json).
**Le wrapper Root a ensuite quitté avec 1** : son dernier affichage console tente
JSON.stringify sur un BigInt. Les deux rapports JSON avaient déjà été écrits avec
leur convertisseur BigInt. Leur contenu, les douze appels, les neuf hash, le calldata
et les adresses ont été contrôlés séparément, sans nouvelle requête. L'échec console
reste dans [son reçu](evidence/doppler-wrapper-console-failure.json) et la source
exécutée est conservée. Aucun exit0 du wrapper n'est revendiqué.

Aucun wallet, signature, transaction envoyée, déploiement ou fonds réel. Cette
réussite synthétique ne qualifie ni les termes économiques Nanobees, ni le bridge,
les achats/ventes/graduation actuels, l'indexation T0 ou la finalité cryptographique.
Les simulations historiques par hauteur et leurs échecs restent inchangés.
L'estimation par hash est une extension du fournisseur, hors EIP-1898.

## Exigences encore ouvertes

La campagne privée à trois propriétaires n'a pas été lancée : l'accord pour un
ancien transfert d'audit ne vaut pas autorisation des clés/duties de cette campagne.
Reprise entière, finalité commune progressante, cycle complet des validateurs,
partitions/WAN/charge et ressources, pruning opérationnel et espace récupéré,
CI complète, bridge/termes réels du lancement et revue de sécurité externe restent
ouverts selon les quatre documents de production. Aucun ancien journal ni engagement
n'est reconstruit ou réadmis. La CI précédente n'a pas démarré pour facturation ;
aucun résultat local ne la remplace.

Sources primaires : [Base endpoint](https://docs.base.org/get-started/connect-to-base),
[EIP-1898](https://eips.ethereum.org/EIPS/eip-1898),
[eth_estimateGas Base](https://docs.base.org/base-chain/api-reference/ethereum-json-rpc-api/eth_estimateGas),
[USDC Circle](https://developers.circle.com/stablecoins/usdc-contract-addresses).
