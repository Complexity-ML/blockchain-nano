# Portabilité du test de comparaison des comptes — 2026-10-02

Le build natif figé `0f53b42d3e78da85aa9a2c2f2993472747aabe8a` échoue (exit 2) dans `test_capella_snapshot_accounts.cpp:13` : surcharge GMP ambiguë pour l’addition d’un `unsigned long long` retourné par `mt19937_64`. Sources avant/après identiques ; échec conservé dans les fichiers joints. Les tests natifs et le rejeu signés dépendants ne se sont pas exécutés.

Candidat descendant `016ab243` publié sur `qualification/snapshot-account-portability-20261002` : conversion exacte du tirage 64 bits par chaîne décimale en `Int`, comme pour le nonce. Une seule ligne de test change ; diff runtime `src/include/network/third_party` vide. Aucune mutation des arbres figés des campagnes Docker ARM64/AMD64.

Une construction native fraîche des onze programmes est lancée avec les contrôleurs joints, puis treize cas sélectionnés et quatre audits signés en lecture seule si leurs prérequis réussissent. Session foreground 40863 ; rapports `/tmp/nano-coherent-016ab243-native-{build,test,offline}-reports-20261002`. Aucun résultat terminal de ce candidat n’est encore acquis. Le contrôleur de comparaison des inputs après audit inclut désormais les onze exécutables, comme le manifeste avant audit ; l’ancien contrôleur incohérent n’avait pas atteint cette étape.

Les régressions complètes 427 cas sur les sources `0f53b42d` restent indépendantes et actives. Leur résultat ne qualifiera pas automatiquement le descendant `016ab243` ; le diff limité au test et les contrôles de raccordement devront être vérifiés. Aucun gain runtime, qualification WAN, conformité complète Ethereum, statut EIP clôturé ou disponibilité production déduit.
