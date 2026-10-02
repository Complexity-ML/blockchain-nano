# Candidat80373152 : qualification du troisième propriétaire et gates de reprise

Aucun runtime promu sur main, aucune vente ouverte, aucun nouveau nœud WAN lancé.

## Travaux réellement démarrés

- Sélection ARM16 du candidat803 active : activation **79,11/360s** et activation avec redémarrage **80,34/360s** passent ; cinq premiers tests passent à cette observation. Les11 autres résultats ne sont pas acquis. Ni comparaison de performances causale ni nouveau résultat WAN.
- Build AMD Docker observé actif ; les16 tests originaux attendent sa fin vérifiée. Même source ARM/AMD,15313 fichiers manifestés, hash du manifeste d12eed4d3c2b50bb07161565a8d4e8324af122eed80fba5effb7eb7ca274c8e0.
- Build Mac natif réellement démarré (deux jobs, neuf cibles, répertoire build-release neuf du worktree803). Les9 contrôles natifs et4 audits de chaîne signée sont en attente. Provenance de build n'est pas attestation de qualification ; aucun résultat terminal ici. Audit positif99blocs/97transactions et trois négatifs historiques, deadlines180s, sandbox enfant deny network*/deny file-write*. Contrôle des archives/objets/programmes/dylibs matérialisées/Node/P2P après tests et après audit ; dyld cache non matérialisé hors inventaire.
- Driver complet AMD lancé et observé en attente. **Pas de build ni de test complet AMD démarré à ce stade**. Gate exige résultat16/16, zéro skip/échec, registre exact et délais d'origine, sources/neuf programmes/sept entrées inchangés. Puis build frais all-targets et426 tests sans filtre/skip, identités des exécutables/entrées avant-après. Le premier échec ferme le gate ; ne pas augmenter les délais ni relancer le même candidat automatiquement.

## Contrôleurs préparés, non exécutés

Audit AMD4 adapté mais non lancé ; attendra fin de sélection réussie et régression complète, sans contention avec celles-ci. Lanceur de restauration WAN préparé à la révision803, mais **non invoqué**. Il réexécutera les gates et ne se fie pas à une ancienne attestation ready.

## Observation du gate WAN effectivement exécutée

check.py **exit2**, readyForBinaryAttestation=false, newWanNodesStarted=false. Identité exacte des deux sources et régression ARM425 du parent2d vérifiées. Les six autres prérequis sont encore non acquis : sélection ARM16, AMD16, AMD426, natif9, audit natif4 et audit AMD4. Ce refus est attendu pendant les campagnes, pas un résultat de réseau ni un incident de consensus.

Le parent ARM2d complet ne prouve pas la régression complète du candidat803 (candidateFullArmProven=false). Diff runtime2d→803 limité aux deux fichiers snapshot ; périmètre réutilisé explicitement et deltas à qualifier. Tous les critères de finalité/duties/restauration, partitions, rotation/slashing/endurance, profils réseau EIP, bridge/Doppler et revue indépendante restent ouverts. Les échecs AMD antérieurs sont conservés.

## Résultats terminaux supplémentaires

Build AMD cohérent neuf terminal0, neuf cibles et registre426 vérifiés ; mêmes15313 sources que l'ARM. SélectionAMD16 réellement démarrée, trois premiers contrôles passent, activation en cours ; complèteAMD reste en attente de16/16.

Build natif cohérent terminal0, **9/9 tests**, zéro skip,2,89s, noms/délais originaux vérifiés. **4/4 audits réels** : checkpoint finalisé/ascendance/checkpointSequence/digest vérifiés sur99blocs/97transactions, tête/futur/malformation refusés. Sources/fixture/binaire et **200 entrées** matérialisées restent identiques avant/après tests/audit. NanoSHA `293ad6b40c6c020dfef633ad9dcdc1aa922c73b3e78f48530c39b2d888cc9fad`. Source native identique aux deux buildsDocker. Aucun nouveau duty, transport WAN ni consensus inter-nœuds qualifié par ces audits locaux.

## ARM16 terminal et enchaînement privé conditionnel

Candidat803 ARM **16/16**, zéro skip/échec, **457,46s**. Réindexation97,88/360, réindexation des sorties95,14/480 passent ; sources/neuf programmes/sept entrées avant/après identiques, registre exact et délais originaux vérifiés indépendamment. Même périmètre qu'avant, aucune causalité de performance déduite. Résultat intégral archivé. AMD encore actif (activation en cours), pas de régression complète ni de reprise WAN commencée.

Audit AMD futur corrigé pour viser **le binaire du build complet qui sera effectivement lancé en WAN** : hash attendu issu du manifeste full terminal, égalité source/archives et exit0 exigés, binaire hashé avant/après audit. Gate WAN vérifiera cette correspondance, au lieu de seulement accepter l'audit de la sélection. Aucune réussite future supposée.

Contrôleur queued-after-full effectivement lancé, en attente du driver completAMD spécifique. Échec/absence de preuves entraîne refus, jamais relance automatique. Si426/426 exacts, audits0, sources/programmes/entrées inchangés et révision803 sont acquis, il exécute l'auditAMD4 puis le lanceur qui réexécute huit gates/attestations avant toute restauration privée. Aucun nouveau nœud à cette observation. Tous les critères WAN, réseau, EIP, production/bridge/lancement restent ouverts.
