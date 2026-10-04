# Candidate cohérente ARM64 : campagne complète terminée

Révision `efbeb9bca3d42e41edb9cb81afba98c0b953a90f`, image et volume
existants épinglés par le contrôleur. Build entièrement neuf sous
`/build/coherent-efbeb9bc-arm64`, pas de relance des tests.

**427/427 réussis, zéro échec/skip, 5 771,83 s.** CTest, audit, manifeste et
contrôleur terminent tous exit 0. Revue indépendante terminale : mêmes 427 noms
uniques dans registre et JUnit, relecture des 16 494 sources et rehash des 208
programmes du volume en lecture seule, identiques aux manifestes avant/après.
Le checkout reste propre à cette révision. Journaux et documents bruts conservés,
les grands fichiers sont compressés sans changement de contenu (`gzip -dc`).

EEST sélectionné : 5 387 variantes, 4 882 exécutions et 505 validations issues
de 317 fichiers. Ceci ne certifie pas un fork entier ni un profil public actif.
L'audit EEST et celui du registre sont réexécutés indépendamment sur les traces
terminales sans nouvelle exécution de tests.

AMD64 encore actif ; restauration de duties/finalité et reproductibilité des
programmes attendent les deux architectures. Aucun supplément réseau autonome
Linux, full natif, lancement économique ou production globale acquis par cette
seule campagne. Tous les critères/échecs historiques restent conservés.
