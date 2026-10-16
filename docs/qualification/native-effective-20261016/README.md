# Native : fermeture sélectionnée et deux consommateurs — 16 octobre 2026

La fermeture sélectionnée 04 réussit sur la base Git `def5cda61919bbb40e91744a18661a2ad7a76eba`. Deux consommateurs Native manquants sont ensuite construits et testés avec une unique correction de portabilité du support de test. Leurs identités sont connues ; **aucun des 24 rôles n’est admis pour une nouvelle campagne**. La production reste non qualifiée.

## Preuves ciblées et limites

| Contrôle | Résultat et portée |
| --- | --- |
| Matériels API Native | PASS : cinq matériels logiciels/Core correspondent à leur autorité originale. Ce contrôle matériel seul ne qualifie pas leur source effective. |
| Ancienne autorité globale | **FAIL conservé** : 14 828/14 835 fichiers réguliers vérifiés, sept fichiers en écart et 14 constats. Les 3 761 cibles d’alias sont vérifiées ; leurs neuf champs historiques ne sont pas récupérés. Aucune autorité rafraîchie. |
| Exporteur et support runtime | PASS limité : 13 matériels réguliers, 16 alias originaux et 26 arêtes de chargement conservées. Le chargement de tous les programmes courants reste non qualifié. |
| Branches CMake sélectionnées | PASS de source : 51 lignes de fondation Core et les commandes de quatre cibles identiques. L’ancien texte `3a523…/170210` est comparé au texte effectif `275b30…/174261`. Le décalage historique de chemin demeure ; aucune configuration ou réévaluation CMake entière. |
| Reproduction de `storage` | PASS : une compilation C++ produit les mêmes 625 856 octets que le membre conservé ; M=MD sur 1 183 dépendances, trois observations stables sur 2 648 ms. Un locateur d’alias prospectif et une garde parent sont admis ; **sa liaison historique reste inconnue**. Aucun Core modifié, lien ou test de projet. |
| Fermeture sélectionnée 04 | PASS : 363 sources atteintes égales aux blobs Git ; 3 190 fichiers réguliers originaux dont 2 535 SDK, et 3 761 résolutions vérifiés. Core ferme 142 membres/MD hérités et le membre API de son producteur retenu ; `storage` s’appuie sur sa reproduction distincte. Cinq nouveaux M, zéro C++ ou test de projet. Ces nouveaux M ne deviennent pas les anciens MD. |

Les sept écarts globaux sont prouvés hors de cette fermeture : `.DS_Store`, `CMakeLists.txt` et cinq sources multihôtes. Cela ne transforme pas l’audit global en réussite. L’équivalence complète de recette CMake et les chargeurs courants restent ouverts.

L’historique du collecteur conserve 01 avant M avec une cause non qualifiée par son terminal ; 02 avant M sur un locateur SDK absent de la table d’alias, dont la cible régulière était déjà épinglée ; 03 après les cinq M sur un ancien objet de template exigé à tort. Les versions source V3/V4 ont été refusées en revue et non exécutées. La collecte 04 corrige ce contrôle sans rejouer de test C++.

## Deux consommateurs Native : premier échec puis réussite 02

L’essai 01 échoue au premier CXX après deux M et deux lectures Git réussis : une invocation CXX, zéro compilation terminée, zéro lien/test CLI. La cause publique est l’ambiguïté Apple GMP de deux `Int += uint64_t` dans `fresh_lifecycle_terminal.hpp:64–65`. Aucun journal brut n’est publié.

L’overlay unique remplace ces deux opérandes par `Int(std::to_string(withdrawal.amount_gwei))` : `07993…/24334` vers `66819…/24376`. Elle conserve les valeurs non signées et les unités gwei. Elle modifie un support de test, sans Core/EVM changé, et reste distincte du blob Git de base. L’import public et le contrôleur gardent la fermeture Core04 inchangée.

L’essai 02 réussit : deux M égaux aux vrais MD, deux compilations C++, deux liens contre Core `7264…`, cinq archives et deux objets assembleur dans leur ordre canonique. Trois CLI ciblées réussissent : **neuf groupes, 47 refus attendus, sept signatures BLS de fixtures et zéro signature opérationnelle**. Elles vérifient la politique d’actions, l’observation de HEAD scellé et une soumission via callbacks. Elles ne prouvent ni un cycle complet de paiements/retraits, ni l’exécution de retraits uint64 de bout en bout, ni un transport HTTP/exec réel. Le header de comptabilité des paiements compile ; ce chemin terminal complet reste à qualifier.

| Nouveau programme Mach-O arm64 | SHA256 | Octets |
| --- | --- | ---: |
| `nano-capella-deposit-audit` | `d46b0891a122ff90baa15156ef8430030ff6869510b26db3c508dacb16b55ac5` | 6 518 584 |
| `nano-capella-lifecycle-submit` | `78269d7fe300c7d6bc986035ff9ab5bcfd4c6c5db7cd2f927229afb1eab0f8cd` | 469 904 |

Le producteur publie le SHA et la taille de sa nouvelle admission privée et trois observations stables sur au moins deux secondes. Le paquet contient le terminal public, pas cette admission. Les anciennes qualifications ARM/VPS restent liées à l’ancien header ; elles ne sont pas requalifiées avec l’overlay.

## État pour la prochaine campagne

La carte publique référencée dans `STATUS.json` est un état antérieur à 24 rôles/22 identités connues. Les deux identités Native publiées par 02 portent maintenant le total connu à **24**, avec **zéro admission de campagne**. Un nom, un digest ou ces tests ciblés ne remplacent pas la fermeture opérationnelle complète.

Les identités/matériels connus de P2P et des anciens exporter/importer ne ferment pas leurs dépendances C++/SDK. Le chargement complet, la recette CMake effective entière, les admissions composites, la reprise des duties, la finalité commune et le cycle complet des validateurs restent à fermer. Aucune qualification EIP/fork, CI, réseau ou production n’est promue ; l’objectif reste actif.

Les opérateurs publics exécutés sont copiés exactement sous `source/`. Le helper `c586…` reste [versionné dans le lot Native antérieur](../validator-read-native-20261015/source/v2/build-native-validator-read-root.py), sans duplication. Ces opérateurs nécessitent les autorités originales de leur campagne et ne constituent pas un lanceur autonome. Aucun input privé, SDK, binaire ou journal opérationnel n’est inclus.
