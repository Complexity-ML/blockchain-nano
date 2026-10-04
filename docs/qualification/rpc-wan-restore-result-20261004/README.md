# Restauration complète privée et finalité reprise — 4 octobre 2026

Runtime `d1597d9a6987d0839ed2f0c7ded268aead35d071`, pilote initial `7434452d18d07d704b0f926d746444ccb9c416bb`, correction de portée `97a6b4de9dfa6bedfd776fbc0ee276cd3c81585e`. Même images/volumes/binaries qualifiés par 427 + réseau21 + signé4 sur ARM64/AMD64, gates et relectures indépendantes conservés. Transport privé SSH corrigé, aucun port public ni fonds réels.

## Preuves acquises

- Trois propriétaires distincts : Mac natif, Docker Mac et Docker VPS ; démarrage lié à la même finalité époque 2 et bloc d'exécution.
- VPS arrêté puis sauvegardé/restauré entièrement (chaîne, témoin cœur, 21 paires de journaux/témoins de signataires et identité P2P). Audit avant redémarrage lié aux engagements et à la finalité externes du reçu d'arrêt : 31 blocs, une transaction. Identité P2P conservée, configuration de reprise liée à 21 pins indépendants.
- Finalité commune de reprise à l'époque 3 en 35 624 ms dans la fenêtre originale de 180 000 ms. Stabilité partagée 3→6, 139 215 ms, sans désaccord de racine.
- Statuts terminaux sains des trois validateurs ; après restauration, le VPS admet 13 propositions et 82 attestations. Ses 21 signataires ont rouvert les journaux/témoins via la configuration de reprise imposée, sans failure ni erreur de trace.
- Audit terminal complété indépendamment en lecture seule : 61 blocs / une transaction, engagement cœur et checkpoint époque 6 concordants, ascendance externe vérifiée. Writer VPS arrêté exit 0, sans OOM. 443 fichiers restaurés inchangés avant/après cet audit ; volume source d'origine encore identique au manifest de sauvegarde.
- Le lanceur vérifie l'absence de dérive des sources/runtime/entrées natives après le scénario ; `runtimeArtifactHashesUnchanged: true`.

## Échec du pilote conservé et correction

Le pilote réseau initial finit exit 1 après la stabilité : `ReferenceError: prepared is not defined` à l'audit post-arrêt. Les variables de chemins ont été déplacées hors du bloc `try`, sans changement C++ ni assouplissement de vérification. La syntaxe du script corrigé passe ; la correction figure dans `harness-scope-fix.patch`.

L'audit manquant a été exécuté sur les mêmes données déjà arrêtées et vérifié par les mêmes helpers d'engagement, de santé et d'ascendance. `completion-result.json` distingue explicitement `originalHarnessExit: 1` et `terminalCompletionVerified: true` ; aucun rejeu réseau, aucun résultat initial réécrit. Le pilote corrigé n'a pas encore effectué une nouvelle campagne complète ; sa prochaine utilisation exercera cette correction.

## Limites et suite

Qualification bornée d'une restauration privée fraîche ; ce n'est pas la reprise de l'ancienne fixture historique du Mac. Une seule transaction synthétique, aucun test de charge, partition prolongée/asymétrique, panne physique ou population distribuée massive dans cette campagne. Votes autonomes historiques et fork-choice complet non rejoués. Cette preuve ne ferme pas les autres exigences de production, la revue de sécurité indépendante ou le bridge/fair launch.

L'archive conserve logs, observations, demandes et rapports, reçus/pins publics, manifests et contrôles du lanceur. Elle exclut bases de données et clés privées. Les dossiers complets et la sauvegarde originale restent sur Mac/VPS. Prochaine priorité : majorité connectée pendant l'isolement du VPS, puis asymétrie prolongée et cycle des validateurs selon les critères originaux.
