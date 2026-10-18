# C8 — outils natifs Mac et jointure de provenance P2P

État du 18 octobre 2026, sources main `7ae6e7cf51f3fc291e6fac2640d3fe552f860127`,
C++ de production inchangé `a8eee9d6d3a35b8e14e8a7f7a6cd1b9e5ee2acd7`.
Ce lot clôt la compilation ciblée des six outils Mac et de leur shim, puis les
contrôles passifs indiqués. Il ne délivre aucune admission entière d’opérateur.

## Résultats

| Preuve | Résultat borné |
| --- | --- |
| Unités compilées / produits liés, parent07 + continuation08 | 7 / 7 |
| Continuation08 seule | 5 nouvelles compilations, 6 liens, 24 commandes sortie0 |
| Anciennes observations / chemins distincts tenus avant M | 9 713 / 9 712 |
| Dépendances précédentes tenues avant M | 2 556, dont 2 391 anciennement post-only |
| Headers supplémentaires du shim tenus avant M | 3 |
| Recettes prospectives / fournisseurs de lien prospectifs | 14 / 6 |
| Entrées tenues inchangées pendant08 | 12 329 |
| Fichiers conservés durablement / octets | 28 / 55 340 064 |
| Core reconstruit / image créée / exporter exécuté | 0 / 0 / 0 |

[native.json](native.json) projette les résultats, recettes et hashes/octet des
28 produits ; les logs complets et observations privées restent conservés.
Le Core SHA `ec2d12064b6f8606823b7d6ac53857dbd9953cca9046b800853c335a46acfc2a`
reste inchangé : 144 entrées BSD, soit un index et 143 objets distincts. Les six
anciens exécutables restent inchangés avant/après chaque invocation Root.
Les sept TU ont M avant compilation, MD égal après normalisation. L’exporter et
le shim compilés en07 sont réutilisés, sans rejouer ces deux compilations.

La première observation indépendante des sept fichiers conservés de07 a lieu
après son terminal FAILED. Leur tenue prospective est démontrée pour08 ; une
identité mesurée à l’instant original de leur compilation n’est pas revendiquée.
Les six fournisseurs supplémentaires sont tenus avant les liens, ce qui ne
qualifie pas leur SDK historique de compilation. Les recettes directes retenues
du CMake ajustent les destinations, le nouveau shim et le premier rpath exporter
`@loader_path`. Ce n’est pas une invocation du build CMake généré inchangé.
Le shim co-localisé possède l’identifiant `@rpath/libnano-test-sync-fault.dylib` ;
chargement Mach-O et ordre des rpaths de l’exporter sont inspectés sans l’exécuter.
Le SDK global Core/fournisseurs, la fermeture dyld globale, la reproductibilité
et CMake/CTest global ne sont pas qualifiés par ce lot.

[retention.json](retention.json) lie les copies EXCL aux SHA/octets connus,
huit champs créateur puis trois captures full9 identiques sur au moins deux
secondes sous trente secondes. Les sept Mach-O sont créés0500, les objets/M/MD
0400. Les fichiers tmp originaux ne sont pas supprimés. Ce reçu conserve du
logiciel ; il ne qualifie pas son admission dans une campagne.

## Vérifications passives des nouveaux produits

[passives.json](passives.json) : cinq invocations bornées terminent avec leurs
codes attendus, groupes fermés et28 fichiers conservés inchangés. Import et audit
sans arguments refusent au stade arguments (code1), deposit sans arguments
refuse avec usage (code1). Capture self-test passe25 cas ; lifecycle-submit
self-test passe3 groupes/26 refus attendus. Aucune base opérateur, lecture d’env,
signature, réseau de chaîne ou duty. Les fixtures lifecycle sont synthétiques ;
aucune vérification cryptographique wire n’en est déduite. Le self-test signant
deposit et l’exporter ne sont jamais exécutés. Ces vérifications lient les
nouveaux hashes et leur résolution native ; aucune régression générale répétée.

## P2P existant, jointure sans recompilation ni exécution

[p2p.json](p2p.json) : 902 entrées originales fournisseur, 3 401 entrées de
compilation/lien,81 pins sources courants et3 produits inchangés, total4 292.
L’original V2 termine0 avec `originalFenceAfterPassed:true` et sans fenceError ;
aucun champ `passed` historique n’est inventé. Le P2P conservé correspond aux
sources `00154f57d4b6e1515d60edeae4e0ab38c42da28a`, SHA
`0db7d0ec5c83bb8239e84c3a54b13401f560d9919fc3ad8a5f10b7bc55d738fa`.
Jointure de provenance sélectionnée uniquement : aucune exécution P2P,
compilation ou interopérabilité réseau nouvellement qualifiée. La revue SOURCE01
refuse une double lecture JSON non jointe ; SOURCE02 lit/parses les mêmes bytes
du FD tenu et passe. Les anciennes autorités ne sont jamais rafraîchies.

## Échecs et critères toujours ouverts

[history.json](history.json) conserve deux refus SOURCE et cinq exécutions
FAILED. SOURCE08 est une continuation distincte, pas une réécriture de07FAILED.
Les problèmes concernent le contrôleur, les recettes et leur provenance ; ils
ne démontrent pas la cause de campagne03 ou des anciens échecs de chaîne.

La nouvelle tenue prospective des headers consommateurs et la jointure P2P
native complètent le manque décrit dans le précédent lot Docker. Restent la
jointure entière des24 actifs, les admissions exactes des trois opérateurs et
les portées SDK/chargement non démontrées. Les copies durables Docker du lot
précédent sont0400 : elles ne sont pas directement exécutables ; il faut de
nouvelles copies exécutables engagées, sans chmod des anciens reçus.

Campagne03 FAILED, accord consommé ; aucune04 autorisée. Runtime1925 inchangé,
Paris/Shanghai activé, aucun EIP/fork promu. Restauration entière, reprise des
duties et finalité commune avançante restent ouvertes. Tous les autres critères
normatifs de consensus, exploitation, pruning, supervision/SIGABRT, CI/facturation,
reproductibilité et revue indépendante non réalisée restent ouverts selon leur
périmètre. Base–Nano reste une proposition non adoptée ; bridge USDC, Doppler et
T0 non qualifiés. Aucun fonds, vente, nouvelle identité signante ou déploiement
public. Objectif ACTIF ; production non qualifiée.
