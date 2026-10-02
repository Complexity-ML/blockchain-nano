# Régression complète AMD64 c10426b2 — terminale, non réussie

417 résultats sur 417 enregistrés : 408 réussites, neuf échecs, zéro ignoré.
Durée réelle CTest : 15 101,53 s. Exit CTest 8 conservé, audit résultats exit 1.
Sources avant/après, registres avant tests et 197 programmes du manifeste
historique identiques ; génération du manifeste post-test exit 0.
L’audit EEST réussit : 4 882 exécutions + 505 validations = 5 387 résultats,
317 fichiers. Ce corpus ne démontre pas la conformité complète d’un fork.

Les deux tests RPC soutenus, l’admission Altair, la partition Bellatrix et cinq
scénarios Capella d’activation/réindexation/sortie restent en échec. Les progrès
intermédiaires ne qualifient pas leurs assertions terminales. Toutes les limites
originales sont conservées. ARM64 c104 est également terminal et non réussi
(410/417, sept échecs distincts), sans effacement des résultats historiques.
Le moteur reste isolé, sans promotion dans main ni nouvelle qualification WAN.

Le manifeste ancien omettait trois helpers présents dans les arguments CTest.
Leurs empreintes historiques avant/après ne peuvent être établies rétroactivement.
Le périmètre 197 programmes est explicite ; le nouvel outil renforcé est utilisé
pour les contrôles suivants. La comparaison source inclut 13 680 fichiers suivis.

Journaux compressés sans changer les octets décompressés. raw-sha256.json conserve
les empreintes originales ; SHA256SUMS contrôle tous les fichiers archivés.
Aucun nouveau test exécuté pour cet archivage. L’audit complet échoue exactement
sur les neuf résultats listés ; la production reste non qualifiée.
