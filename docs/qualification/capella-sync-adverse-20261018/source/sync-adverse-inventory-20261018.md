# Inventaire causal sync/reconnexion — source seule, 18 octobre 2026

R lu propre à 4ae774f16220d8a9362004b573d83f3c8ed78a97 ; RPC C6 inchangé. Aucun test/import/compilation/SDK/privé/réseau ici. Présence d’un test ≠ exécution actuelle ni qualification WAN.

## Déjà couvert : ne pas relancer à l’identique

- tests/test_altair_runtime.cpp:24–65,70–87 : identité, configuration refusée avant DB, restart/token, lifetime et clock/cancel. CMakeLists.txt:1331–1337 : consensus_altair_runtime ; variante managed avec processus hors lot. Le runtime annonce executionBound=false et networkAltairActive=false.
- tests/test_altair_reactor.cpp:19–34 : entrées malformées, cancel avant publication, CAS concurrent, sync/reopen ; test_altair_fork_head.cpp:38–112 : corpus officiel, votes/têtes/finalité et modes durable/worker/restart.
- tests/test_altair_wire.cpp:88–149 : ticket/ACK perdu, retry/digest, retired, enveloppes/SSZ, saturation/arrêt. network/altair_rpc_service.hpp:59–97 expose receive/submit/block/inventory ; aucun .cpp correspondant.
- tests/test_altair_relay_progress.cpp:44–107 : ACK/perte, ordre parents, votes refusés, pages dupliquées/oversized et cache pruné, sur callbacks modélisés. test_altair_propagation.cpp:27–48 et test_altair_restart_p2p.cpp:45–69 : vraie HTTP/libp2p, ACK/restart/session ; hors scénario sans réseau.
- Altair : test_altair_archive.cpp:41–42 accepte un root connu à signature altérée sans remplacer l’archive. src/consensus_altair_reactor.cpp:228–238 republie ; storage.cpp:565–581 avance sa séquence. Idempotence état/archive ne signifie PAS token immobile pour cette API.
- Capella tests/test_capella_transition.cpp:2251–2298 : --incoming-parent-interference / --incoming-alternation, travail borné, oracle, nouvelles signatures invalides, duplicate/token stable et witness reopen ;2399–2432 : --prepared-block/worker réel. test_capella_duty_availability.cpp:125–135 : peer block accepted/deferred, progrès borné.
- Même TU:2945–3003, --wire : curseurs/checkpoints/budgets, saturation, collecte/order refusant mauvais root/duplicate/ancre manquante ; descendants demandés d’abord via CapellaBlockDelivery vers VRAI CapellaConsensusService, égalité tête/exécution/finalité. 3036–3070 : --reactor-sync/--owner-sync/--owner-witness couvrent WAL/perte/replay et P/W. 589–616 (--historical-export/--historical-beacon-http) : archive prunée, duplicate ACK, mauvaise signature, export/reopen.
- network/test_capella_dependency_route.cpp:12–48, CMake network:422–430 : churn/adversarial, 513/2049 blocs synthétiques, ACK tardif/cancel, caches bornés. Le receveur y est MODÉLISÉ, pas une transition Capella exécutée.

## API locale composable

CapellaDurableReactor::block_admission (src/consensus_capella_reactor.cpp:721–825) distingue accepted/rejected/deferred ; parent absent/branche hors finalized refusés:771–775 ; préparation bornée à une époque/appel:793. current(), signed_block(), inventaires et reprise avec témoin : include/nano/consensus_capella_reactor.hpp:227–246,284–296. Aucun socket requis.

CapellaConsensusService (network/capella_consensus_service.hpp:23–68) vérifie l’enveloppe avant queue, conserve deferred et propose submit_receipt/receipt/rotate_session/stop/join. Reçus locaux au processus : perte de session = résultat inconnu, pas rejet. Worker/Receipts donnent un endpoint asynchrone borné sans Nano/P2P.

Fixture réutilisable : tests/support/durable_owned_fixture.hpp:28–92, sans ancien main/groupe. Domaine déterministe disjoint des acteurs historiques ; signatures BLS de fixture autorisées mais comptées, pas « zéro signature ». Limites existantes 180 s/128 MiB/1024 signatures ; P/W distincts. Aucun journal/domaine privé03, duty ou campagne04.

## Sélection nouvelle complète proposée

Nouveau tests/test_capella_sync_adverse.cpp, target nano-capella-sync-adverse-tests, CLI --self-test, groupes isolables orphan-retry et deferred-session-reopen, CTest distinct. Core/anciennes suites inchangés sans défaut causal établi.

1. **orphan-retry** : petit parent/enfant signés par transition complète ; enfant d’abord au service réel. Rejected préserve snapshot brut P/W, tokens, tête/exécution, ancre/finalized et archive. Livrer le parent puis réessayer : comparer tête/racines/snapshot/bytes signés à un receveur canonique indépendant. Duplicate Capella accepté avec token stable ; pas cet oracle sur Altair.
2. **deferred-session-reopen** : écart imposant réellement deferred sans publication. Arrêter/joindre le service, fermer/reprendre P/W sur engagements exacts AVANT achèvement ; nouvelle session, ancien résultat inconnu, soumission identique. Sous appels finis, obtenir accepted et l’oracle canonique ; perdre ensuite l’ACK d’un commit, recréer le service et retry sans seconde publication. Observer deferred et session changée, pas seulement le résultat final.

Mesures : appels/bytes/blocs/signatures, attente et libération des réservations worker, deadlines ; finalized epoch/root avant/après. Vrais objets neufs après fermeture workers/DB. Aucun sleep de protocole, réseau ou mocked state transition. Wrong identity/queue/cancel déjà acquis ne sont pas rejoués en bloc. Ce lot ferme une composition locale absente, pas WAN, avance finalisée non-genesis, duties, cycle/sanctions/rotations, panne physique ou production. Une baseline peut PASS : aucun ancien échec causal inventé.
