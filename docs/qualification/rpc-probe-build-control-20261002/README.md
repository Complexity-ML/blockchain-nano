# Mesures individuelles des sondes RPC — contrôleur préparé

Branche candidate `qualification/rpc-probe-timing-20261002`, source `3fa8b805eebaa9e934ba4eacb771a98f7f9d5290` : seul `src/rpc.cpp` diffère de 9e9. Les simulations initiale, au gaz mesuré, avec marge, proche de la borne haute et binaires sont chronométrées séparément, sans modifier recherche/budgets/résultats/cancellation.

Le contrôleur réutilise l'image AMD64 identifiée et le volume neuf du diagnostic précédent. Les mtimes des sources byte-identiques sont conservés pour éviter une recompilation complète. CMake recompile/re-lie les cibles nécessaires. Les binaires en direct du volume appartiendront alors à cette candidate ; les preuves 9e9 restent historiques. Aucun autre conteneur ne doit tourner. Cas original Paris, 16 clients, assertions et timeout 480 s inchangés. Sources et programmes vérifiés avant/après ; traces opt-in non confidentielles.

Contrôleur syntaxiquement vérifié, exécution et résultat pas encore acquis. Ce diagnostic ne résout pas l'échec historique et n'est pas une régression complète ou une qualification production. Aucun changement runtime promu sur main.
