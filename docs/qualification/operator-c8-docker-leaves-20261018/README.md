# C8 — outils opérateur Docker ARM64 et AMD64

État du 18 octobre 2026. Sources `ffae0567933bee0affdb553410335d2127bbea88`,
production C++ inchangée à `a8eee9d6d3a35b8e14e8a7f7a6cd1b9e5ee2acd7`.
Il s’agit de la compilation et des vérifications passives de neuf outils,
pas d’une restauration ni d’une admission entière des trois opérateurs.

| Preuve | Docker Mac ARM64 | Docker VPS AMD64 |
| --- | ---: | ---: |
| Unités C++ compilées et exécutables liés | 5 | 4 |
| Commandes terminales attendues | 28 | 23 |
| Headers/dépendances anciens tenus avant et après | 425 | 425 |
| Logiciels anciens inchangés | 9 | 10 |
| Produits C8 conservés inchangés (Core et nano) | 2 | 2 |
| Entrées totales tenues inchangées | 492 | 487 |
| Fichiers neufs conservés durablement | 20 | 16 |
| Octets de ces copies | 25 514 800 | 20 641 488 |
| Core reconstruit / image créée | 0 / 0 | 0 / 0 |

Les deux workers terminent avec code 0 et `status: PASS`, sans erreur secondaire.
Chaque conteneur possédé est supprimé ; aucune suppression globale ou nouvelle
image. Les images imposées e30ce85e… (ARM64) et fd9b0bb9… (AMD64) restent
identiques avant/après. Les archives C8 de 143 membres sont conservées.

## Recettes et fermeture effectivement vérifiées

Les cibles sont `nano-capella-isolated-bootstrap-import` (ARM64 seulement),
`nano-capella-isolated-bootstrap-audit`, `nano-capella-isolated-stopped-capture`,
`nano-capella-deposit-audit` et `nano-capella-lifecycle-submit`. Aucun exporter
n’est construit ou exécuté dans ce lot.

Les 16/15 fichiers CASE sont des copies EXCL des sources Git exactes. Leurs huit
champs de création et SHA/octets connus précèdent trois observations full9
identiques sur au moins deux secondes, sous trente secondes. Les lecteurs des
anciennes entrées restent stricts. Les inclusions locales sont résolues dans
CASE ; aucun recours aux anciens `/work/tests` ou `/work/src`.

Un `-M` précède chaque compilation `-MD` ; leurs ensembles de dépendances sont
normalisés et égaux. Les dépendances projet consommées sont tenues contre les
anciennes observations originales avant compilation, puis vérifiées après.
Les headers système proviennent de l’image immuable, sans installation.
Les recettes reprennent les flags et liens sélectionnés du CMake : include
BLST pour import/deposit, `NANO_TEST_SOURCE="/work"` pour deposit seulement.
Les neuf liens utilisent le Core C8 conservé ; ELF64 little-endian, machine
ARM64/AMD64 et résolution `ldd` sans bibliothèque manquante sont vérifiés.
Ni configure CMake global, ni CTest, ni reproduction complète ne sont exécutés.

Les 1 047 anciennes entrées ne sont pas revendiquées toutes inchangées : les 425
entrées sélectionnées sont tenues, et les dépendances effectivement consommées
sont vérifiées séparément. La différence du header
chain sélectionné reste limitée aux commentaires ; les deux copies ne sont pas
superposées. Les détails et SHA/octets de tous les nouveaux produits se trouvent
dans [arm64.json](arm64.json) et [amd64.json](amd64.json).

## Vérifications passives

- Import et bootstrap audit sans arguments : code 1 attendu, refus au stade
  arguments, aucun démarrage de runtime. Cela ne teste aucun vrai import/audit.
- Stopped capture `--self-test` : code 0, 25 cas, aucune base, lecture d’env ni
  signature. Cela ne capture aucune base de la campagne03.
- Deposit audit sans arguments : code 1, stdout vide et usage sur stderr. Son
  self-test signant n’est pas exécuté.
- Lifecycle submit `--self-test` : code 0, trois groupes, 26 refus attendus,
  aucune signature, base, réseau ou exécution de commande. Les fichiers opaques
  de test sont synthétiques ; aucune vérification cryptographique wire n’est
  déduite de ces tests de politique/IPC.

Les projections contiennent les sorties JSON réelles et SHA/octets stdout/stderr.
Les assertions du worker sélectionnent des champs ; il ne faut pas les qualifier
de validateur générique de schéma fermé. Aucune base, clé ou donnée d’opérateur
n’est montée ; aucun réseau de chaîne ni duty. Les logs complets, programmes,
objets, M/MD et reçus privés restent accessibles sur leurs machines. Les 36
copies durables sont relues avec SHA/octet et source full9 inchangés ; cette
conservation logicielle n’est pas une admission de runtime. Les originaux tmp ne
sont pas supprimés et aucun journal signé n’est déplacé.

## Refus et suites

[SOURCE01](source-review.json) reste NO GO, sans exécution. Ses quatre défauts
sont corrigés dans SOURCE02, revu avant les deux exécutions. Aucune ancienne
preuve ni autorité n’est réécrite pour transformer un échec en succès.

Les admissions entières C8 restent ouvertes : provenance P2P native, fermeture
précompilation des headers SDK des consommateurs natifs, jointure des 24 actifs
et nouvelles admissions exactes. Les 2 391 headers observés après la précédente
compilation native ne prouvent pas une tenue avant compilation et nécessitent
une nouvelle compilation ciblée des seuls consommateurs concernés. Aucun
rebuild du Core ou des images n’est déduit de ce manque.

Campagne03 FAILED et consentement consommé ; aucune04 autorisée. Runtime réseau
1925 inchangé, profil activé Paris/Shanghai, aucun fork/EIP promu. Pas de
restauration entière, reprise de duties ou finalité commune avançante qualifiées.
Supervision Source03 non intégrée/SIGABRT inexpliqué, autres critères de consensus,
pruning, CI/facturation, exploitation, reproductibilité et revue indépendante
non réalisée restent ouverts. La relation Base–Nano est une proposition non
adoptée ; bridge USDC, termes Doppler et visibilité T0 non qualifiés. Aucun
fonds, vente ni déploiement public. Objectif actif ; production non qualifiée.
