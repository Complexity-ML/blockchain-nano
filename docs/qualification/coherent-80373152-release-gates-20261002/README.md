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
