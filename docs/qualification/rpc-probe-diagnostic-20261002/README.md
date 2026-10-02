# Diagnostic des sondes de gaz — 2026-10-02

Source candidate instrumentée `3fa8b805eebaa9e934ba4eacb771a98f7f9d5290`, branche `qualification/rpc-probe-timing-20261002`. Seul `src/rpc.cpp` diffère de 9e9 : wrapper chronométrant les simulations sans changer les arguments, l'ordre, les budgets, la recherche ou les vérifications. Ce code n'est pas promu sur main.

Image AMD64 `sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14` et volume `nano-clean-rpc-build-20261002` réutilisés. Compilation incrémentale réussie ; les binaires actuels du volume appartiennent désormais à cette candidate, pas à la qualification 9e9 précédente. Contrôleur terminé et conteneurs temporaires retirés.

Cas Paris original, 16 clients, assertions et timeout 480 s conservés : **PASS 82.05 s**. Chaque estimation des 192 exécutées a effectué **cinq simulations** : initiale, au gaz consommé mesuré, avec marge de profondeur, près de la borne haute, puis une binaire. La poursuite après la simulation au gaz mesuré démontre son échec dans ce scénario précis ; aucune généralisation aux autres contrats.

Médianes des simulations : initiale 415.903 ms, mesurée 393.849 ms, marge 413.144 ms, proche 398.607 ms, binaire 403.324 ms. Estimation complète médiane 2103.730 ms, maximum 3696.004 ms. Préparation médiane 3.713 ms, maximum 66.325 ms ; attente de verrou maximum 361.259 ms. Attente WebSocket maximale 3197.998 ms sur limite 4000 ms. Ces distributions agrègent quatre processus ; durées parallèles non assimilables au temps mural ou CPU. La préparation inclut la capture de contexte, pas toutes les opérations d'exécution du candidat pending.

Cela identifie le coût des répétitions EVM dans ce passage réussi. Cela ne prouve ni la cause de l'expiration historique ni une correction ; instrumentation et ordonnancement peuvent modifier les mesures. L'optimisation devra préserver une borne haute effectivement exécutée avec succès, les plafonds caller/solde/bloc, EIP-150, cancellation et les contrats dépendant du gaz, avant régression pertinente puis qualification cohérente complète.

Audit indépendant de l'archive : 15 315 empreintes sources et trois programmes identiques avant/après ; compilation et CTest codes zéro. Archive SHA-256 `2cf606da840118bb4d5b458e16f60072cd66508376b01ea9814689ca28ec0e9b`, contenant logs, manifests, JUnit, quatre traces et analyse (p95 empirique sans interpolation). Aucun résultat nouveau de régression complète, restauration des duties ou WAN. Échec historique et tous les critères production restent ouverts selon leurs statuts.
