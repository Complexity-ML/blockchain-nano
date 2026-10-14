# Test du CLI de maintenance sur copies synthétiques

Contrôleur ROOT uniquement : un CTest nouvellement compilé, `storage_capella_bootstrap_archive_owned_copies`, et exactement treize lignes sémantiques du harness Linux. Aucun ancien test startup/pending n’est relancé. Le CLI réel et le shim de rétention sont utilisés ; aucune nouvelle signature ni donnée privée de campagne.

Les SHA externes authentifient le contrôleur, les guards cd1b, le plan parent, le terminal de build cb9 et le manifeste de 22 métadonnées publiques. Les 1 320 sources, trois objets, trois programmes, archive de 142 membres, sept bibliothèques, KZG généré, graphes CTest et SDK sont vérifiés avant/après. Le reçu de preuve ar-p est lié au terminal et au manifeste ; ce contrôleur ne reconstruit ni ne requalifie l’archive.

Le test tourne dans l’image existante, réseau désactivé, sources/build en lecture seule, UID/GID du plan, 2 CPU, 1 GiB, 256 PID et tmpfs Linux de 512 MiB mode 700. Budget global 400 s ; CTest conserve TIMEOUT 130 s, harness 120 s/128 MiB, enfants 20 s. Les trois CTestTestfile réellement présents sont copiés byte pour byte. Les dossiers ckzg/mcl, présents dans le build mais sans CTestTestfile, restent des dossiers vides dans la vue de découverte. Aucun fichier CTest absent n’est inventé.

Exécution après revue, avec tous les chemins absolus et SHA observés :

    python3 run-maintenance-one-test.py --controller-sha <SHA> --guard <cd1b.py> --guard-sha cd1b0ce8abd1a69ec6c6dcc0b3c3ac79415ff00a0676187c48309bd5943a9ae2 --parent-plan <plan.json> --parent-plan-sha <SHA> --build-root <run-01> --build-terminal <run-01/receipts/terminal-build.json> --build-terminal-sha <SHA> --expected-build-controller-sha cb9acac75470ff0ab8627060e8c7de9db7f2312f30d252ed9f01ec82c06731c2 --metadata-manifest <root-build-public-bindings.json> --metadata-manifest-sha <SHA> --output <dossier-neuf-distinct>

Le résultat est un seul JUnit réel sans skip et les familles owner/exclusivité/provenance, rétention et fsync, application/relecture hot/cold/fragmented, stale et noWork. Les compteurs d’exécution d’un échec restent observés ou inconnus, jamais assimilés à zéro. Les ctime Darwin sont diagnostiques ; SHA et autres métadonnées matérielles demeurent stricts, comme sur le build qualifié.

La reprise du WAL interrompue du wrapper, perte physique d’alimentation, sauvegarde complète des journaux, horloge UTC actuelle, duties, finalité réseau et production restent ouvertes. Un build PASS ne constitue pas une réussite de ce test. Aucun compiler, Docker ou test lancé par l’auteur de ce contrôleur.
