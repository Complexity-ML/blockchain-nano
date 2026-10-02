# Coût du commit Bellatrix — scénario borné, timeout conservé

Core c10426b2 et variante de sérialisation précédente ; instrumentation de coût seulement. Cas original 112 slots, timeout 240,030 s, dernier slot observé 96, zéro skip, sorties CTest/conteneur 8. Sources/programmes/bibliothèques originales identiques avant/après ; post-manifest à zéro.

Derniers compteurs toutes les 64 occurrences : 1 920 encodages 47,855 s ; publications inclusives 121,002 s ; digest du nouveau snapshot 11,405 s ; commits couplés 42,966 s dont appels Write synchrones RocksDB 13,194 s. Lectures authentifiées du checkpoint 20,617 s sur 6 848 appels : certaines appartiennent aux commits et d’autres aux demandes de validation ; ne pas les additionner au commit ni aux publications.

Aucun témoin externe actif dans cette simulation. Le compteur témoin (4 microsecondes) mesure essentiellement le retour anticipé ; il ne qualifie pas son coût ni sa reprise. Les appels tronqués après le dernier multiple et le travail interrompu ne sont pas entièrement comptabilisés. Ces données ne prouvent pas une cause unique de tout l’écart ni un benchmark répété.

Le Write synchrone n’explique donc pas à lui seul le délai. WAL et synchronisation restent conservés. La variante de sérialisation nécessite des tests de reprise/annulation/témoin sur un candidat non instrumenté, pas seulement un test d’assemblage JSON. Tous les échecs historiques et critères de production restent ouverts.
