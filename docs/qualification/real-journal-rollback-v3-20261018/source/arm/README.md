# ARM rollback successor02 — SOURCE ONLY

Proposition neuve ; aucun programme exécuté par l’auteur, aucun contrôle effectif, SDK, DB ou log réel lu. Les fichiers du parent01 gelé restent intacts. Root seul prépare les autorités/contrôles et décide d’une invocation02 unique après revue.

## Historique communiqué par Root

ARM01 a réellement terminé `71557/1` **avant tout M/CXX/RUN** (tous zéro). Root a observé entre les deux premières observations d’admission des providers montés RO : octets, alias, dev/ino/mode/nlink/taille/mtime/ctime identiques ; seuls UID/GID des sept providers (Core et six bibliothèques) sont passés de 0:0 à 501:20. La cause et l’acteur ne sont pas prouvés. Les 17 fichiers hôte tenus, dont les neuf logiciels originaux, ont gardé leurs anciennes identités ; cleanup/image/volume PASS selon Root. Aucun défaut C++ ou nouveau résultat de journal n’en est déduit.

## Delta exact

`worker.py` dérive du parent `1caa9da9… /16640`. Avant le premier `admit_inputs(initial_paths,'initial')`, il fait **une seule** `observe(initial_paths)` explicitement non admise. Il publie lignes GNU9, alias et comparaisons des octets connus, y compris un booléen de résultat, puis refuse toute incohérence. Cette observation ne remplit ni `guest` ni `aliases` et ne devient jamais une autorité.

L’admission suivante est inchangée : huit champs fixes, octets/alias connus, puis trois échantillons FD9 identiques pendant au moins deux secondes sous trente secondes. Elle conserve ses refus, ses logs et ses premières identités. Aucun retry d’une admission échouée après tenue, aucune UID/GID imposée à la place des valeurs observées, aucune réinitialisation d’ancienne row hôte. Si l’instabilité persiste, cette invocation refuse.

`host.py` ne change que son `WORKER_PIN`. Les CppV3 `2e5ef451… /28801`, helper `d3e2ddd7… /20319`, Core65/143 membres, neuf autorités logicielles d’origine, inventaire source1047 et anciens engagements portables69/8 restent identiques. Les champs de contrôle restent exactement13 ; le modèle reste `ready:false`. Aucun nouvel assouplissement de journal, Seal, productionAdmissions, borne historique du parseur ou résultat C++.

`prepare-root.py` ne change que D et les trois nouveaux namespaces case/control/output `-02`, ainsi que les pins host/worker/template dérivés. La recette ancienne est conservée exactement, sans append/remap ; les deux copies EXCL0400/fsync restent **créées et non admises** avec leurs huit identités et SHA. Anciennes entrées tenues toujours FD9+SHA strictes. Il ne crée aucune configuration effective chez l’auteur.

## Exécution prospective réservée à Root

Même transport Python Mac → Docker exec, aucun Python invité. Un TU/RUN seulement, zéro Core/image build. Même volume Linux persistant, sibling neuf, audit passif des anciennes69 entrées avant/après et après cleanup ; aucun ancien test/cohort rejoué. L’exit C++ brut n’est pas remplacé par le statut du keepalive. L’historique FAILED01 n’est jamais promu en PASS.

Ce paquet n’a pas de résultat runtime. Protection externe contre rollback simultané des engagements, restauration opérateur, duties, finalité commune, réseau et production restent ouverts. Les sources sont livrées0644 pour revue ; Root décidera du gel avant tout premier usage.
