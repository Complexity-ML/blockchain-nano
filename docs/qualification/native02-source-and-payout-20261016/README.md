# Native02 et total des payouts — consolidation publique bornée

Ce lot livre le correctif JS du total et son nouveau test, avec leurs preuves ciblées. Le build source SDK global reste actif à cette observation ; aucun succès terminal n’est acquis. La liaison du consommateur reste une étape séparée : ses sources réseau proviennent de `5fc36d9a`, et tout descendant utilisé doit conserver exactement ces sources réseau, tiers et script SDK. Aucun changement C++ ne découle du correctif JS.

## BoringSSL : valeurs et reçus, sans causalité des archives

392 fichiers Release distincts, dont 113 sous `gen/`, sont égaux au paquet source canonique ; cela ne reproduit pas leur génération locale. La jointure couvre par valeur les 784 membres de quatre archives Release/Debug avec 786 reçus de compilation validés M==MD. 143 membres gardent plusieurs candidats de la même famille de configuration ; 641 en ont un seul. Ces familles sont des filtres, pas une preuve de l’appel `ar` producteur. Les 786 chemins `.o` sont absents ; aucun objet physique courant n’est vérifié. Les recettes `ar`, les autres appels compilateur, le SDK global et le DAG transitif restent ouverts.

Le premier inspecteur refuse `nlink==1` sur des archives installées Hunter à `nlink==2`. Cet échec est conservé ; aucune archive n’a été modifiée et le build n’a pas été interrompu. Les trois publics utilisés sont le contrôle source canonique `047976f7…` (537 octets), la jointure `017a7ef1…` (1 023 octets) et ce refus `97663d28…` (389 octets).

## JS : le total Gwei dépasse légitimement uint64

Le terminal C++ émet une somme GMP exacte de retraits individuels positifs uint64. L’unique garde JS du total imposait à tort cette borne individuelle à la somme. Le correctif garde l’hexadécimal canonique minuscule, utilise BigInt et exige `N <= totalGwei <= N * UINT64_MAX`, après les gardes existants sur N. Les montants individuels uint64, crédits Wei uint256, C++, header66819 et autres gardes restent inchangés.

Root a réellement exécuté seulement le nouveau test pur : deux groupes, dix refus attendus, zéro fail/skip, 189 ms, Node v25.9.0. L’ancienne suite runtime n’est pas rejouée. Le premier essai échoue à l’admission des métadonnées avant version Node ou test ; le chemin divergent n’a pas été retenu. Aucune cause physique n’est attribuée. Les sources restent celles du manifeste `e82352627592fa783f780c9ac4f45f2b495636cc7253a6fad36b4ff7050a6aa7` ; son statut auteur « PENDING » est conservé, et le résultat Root séparé `7013a317bef88dd587dfac9b386fed56a6a4c1697a629db5b48980b303584ea4` (583 octets) apporte le PASS borné. Le refus initial `5e6300f7…` (414 octets) reste conservé.

Ce lot ne qualifie ni cycle complet de payouts, sorties ou paiements réellement exécutés, HTTP/exec, reprise, duties, finalité commune, admission de campagne ou production. Ce paquet ne contient aucune donnée SDK, table privée, archive, binaire ou journal brut. Les inspections SDK de Root sont représentées par leurs seuls reçus publics et engagements ; le helper pur utilisé pour indexer les reçus est inclus sous `source/compiler-receipt-index/`. Le manifeste auteur de la proposition reste un snapshot PENDING ; le reçu Root séparé démontre le résultat du test ciblé.
