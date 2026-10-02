# Audit natif arrêté ARM64 → AMD64 — préparé, résultat pendant

Candidat gelé `2d2764ded1f1d7f065ef163d8dcc0fc216268bc1`. Contrôle `run.sh` vérifié par `bash -n`, installé sur le VPS et lancé en attente (PID observé 510599). Il attend la disparition du conteneur existant `nano-coherent-2d2764de-amd64-test` puis exige son rapport terminal et son intégrité, sans exiger que ses quinze cas soient tous verts. Une erreur de timing du parent n'est donc pas effacée par cet audit distinct.

Seul le dossier `chain` du témoin arrêté ARM64 a été transféré : archive SHA-256 `ff34f8568ed897cb58c0f754f77a1278b8267055d3802df61d1a16e93d9bc44b`, égal sur Mac et VPS. Aucun journal de signature ni clé privée transféré. Source `/tmp/nano-backup-restore-20260930/arm64/final/restored/chain`, destination `/home/ubuntu/nano-stopped-arm-chain-fixture-20261002/chain`.

Le binaire CMake AMD64 déjà compilé rejouera l'histoire canonique signée : positif 99 blocs/97 transactions et engagement exact, puis trois refus (tête non finalisée, époque future, racine malformée). Délai 180 secondes par invocation. Empreintes de toute la base avant/après, source avant/après, binaire identique à la campagne sélectionnée. Image et volume existants ; source/base/build readonly, réseau désactivé, deux CPU/3 GiB. Rapport attendu `/home/ubuntu/nano-coherent-2d2764de-amd64-offline-reports-20261002`.

**Limite :** checkpoint sélectionné dans un témoin antérieur. Ce n'est ni une nouvelle observation WAN indépendante, ni une restauration des journaux anti-double-signature, ni une reprise des duties. Historique complet des votes autonomes/fork-choice non vérifié. Aucun résultat de cet audit encore acquis.

Lors de la préparation : sélection AMD64 4/15 passes observées, activation 346,72 s / 360 s ; régression ARM64 44/425 passes observées, suivante active. Ces comptages intermédiaires ne remplacent pas les audits terminaux. Tous les critères de production et échecs historiques restent ouverts ; aucun runtime main promu.
