# Correctif étroit : logs vides du reçu Geth

Source uniquement ; aucun programme, test, compilateur, Docker, SSH ou Git
exécuté par l'auteur, aucun fichier R modifié et aucune donnée privée lue.
Le parent immuable reste inventory-v2-receipt-coverage-source-01 :
driver956f94400e42b58b4f49e1e5d6bf3c2ef341041e3ad4babcc9ea69130025e007.

Root rapporte le premier refus réel avant création du helper : Docker1,
0transaction et 0requête, 7/7FDs driver et 28/28FDs Root fermés. L'erreur
bounded log array provenait du premier reçu Geth, dont logs vaut null.
Cet échec est conservé ; ce nouveau lot de reçus n'a encore exécuté aucune
transaction Native. Les qualifications Native antérieures restent distinctes.

Root a relu le résultat Geth step01 : 2063B, SHA
3a4e465ef61746c961e1644dfa41a949cc956943074cb500cf099d1918724892,
logs:null, status0x1, gas0x3f3a5b, bloom du reçu et bloom agrégé tous deux nuls.
La réussite précédente du lot Geth65 reste dans sa portée initiale.

Delta unique : receiptFromReference exige un champ logs propre au reçu.
Seulement dans cette fonction côté Geth, null est représenté par le tableau
vide pour encoder le reçu et calculer bloom/root. Un champ absent, undefined,
objet ou valeur scalaire reste refusé. La fonction logs() globale est inchangée
et exige toujours Array ; les retours Native ne peuvent donc pas utiliser null.
Les comparaisons de contenu, ordre, bloom, encodage et trie restent strictes.

La copie entière du nouveau driver et le patch sont deux formes du même delta :
en appliquer une seule. CLI et cibles sont identiques au parent :
tests/hardhat/inventory-v2/native/receipt-coverage.mjs.
Le C++10b01e8e…/12018, CoreG2, son archivea1475733…/41077050, les helpers,
les65transactions signées et les65résultats Geth ne changent pas.

Root qualifiera le nouveau lot de reçus ; aucun rejeu Geth ni test séparé n'est
demandé ici. Aucun succès d'exécution Native, bloc miné, consensus ou production
n'est déduit de ce correctif source.
