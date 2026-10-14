# Build ROOT du seul harness corrigé

Réutilise le vrai build maintenance cb9 (trois objets/liens déjà réussis) et son manifeste externe de 22 métadonnées. Le nouveau source-view 1320 ne change que tests/test_bootstrap_archive_maintenance.cpp e5f664, manifest dae95. Le CPP du CLI, le shim, le cœur et CMake restent inchangés. Aucun configure, core compile, commande ar ou test ; un CPP harness et un lien, deux objets/programmes copiés byte pour byte.

L’archive142 est byte-copiée puis protégée avant/après les liens ; sa preuve ar-p parent est réutilisée uniquement sous même SHA d’archive. Aucun des142 objets cœur n’est relu comme objet physique dans ce lot (compteur0 honnête). Les véritables -M/MD du harness doivent conserver la fermeture exacte du parent ; les SDK, libraries, KZG, inputs et parents sont contrôlés avant/après. Image existante, réseau désactivé,2CPU/4GiB/PID256,900s global ; CPP180s/lien120s.

Arguments externes : --controller-sha, --guard/--guard-sha cd1b, --parent-plan/--parent-plan-sha, --parent-terminal/--parent-terminal-sha (cb9 réel), --parent-metadata/--parent-metadata-sha (root-build-public-bindings.json réel22), --harness/--harness-sha dae95, --output (dossier neuf distinct).

Terminal format nano-bootstrap-maintenance-one-consumer-build-terminal-v1, statut BUILD_PASS_MAINTENANCE_TEST_NOT_EXECUTED. Compteurs 0core/0ar/1CPP/1lien/2objets+2programmes copiés ; source-before.json complet1320, SDK/probes/flags/liens/CTF exacts disponibles pour le nouveau manifeste de22files. Aucun PASS CTest hérité, les premiers échecs ARM/AMD restent conservés. L’auteur n’a compilé, lancé Docker ni testé.
