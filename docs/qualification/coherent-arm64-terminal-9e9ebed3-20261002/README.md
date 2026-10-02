# Qualification terminale Docker ARM64 — 9e9ebed3

La campagne cohérente a terminé : 427/427 cas, aucun échec ni skip, 5709,10 secondes. Les 21 tests réseau supplémentaires ont terminé : 21/21, 1,09 seconde. Aucun nouveau scénario dans cette campagne : régression complète après recompilation cohérente du réseau et des helpers, nécessaire après la cause historique du probe périmé.

Le contrôleur indépendant `coherent-terminal-review-controls-20261002/run-review.sh` a retourné 0. Commandes et délais originaux conservés ; 15 315 fichiers source inchangés avant/après compilation/tests et comparés au checkout actuel ; 208 programmes inchangés et leurs SHA relus dans le volume conservé, monté en lecture seule. Les traces EEST correspondent exactement à 317 fichiers / 5 387 variantes (4 882 exécution, 505 validation). Les contrôles individuels décrivent leurs limites dans leurs JSON : le résultat global de revue ajoute les étapes distinctes, il ne transforme pas les champs `false` du premier contrôle en qualification implicite.

Sources : `9e9ebed3833a24f75ce7b29b9a80dcce341dfc3a`. Image gelée : `sha256:6116669430b02b7e8012af8c8bd515a5a7dcd1dd2034649f9b45ae5b9e70a3a4`. Volume existant : `nano-linux-sync-build`, build `/build/full-network-9e9ebed3-arm64-20261002`.

Les fichiers gzip préservent les octets des preuves ; `artifact-manifest.json` donne SHA-256 et taille décompressés. Les résultats proviennent des campagnes existantes : aucun test ni build relancé pour cette archive.

Limites ouvertes : campagne AMD64 et audit signé cohérent encore en cours ; aucune restauration WAN ni reprise des duties démontrée. Pas d'audit des entrées de liaison statiques dans cette revue, ni conformité intégrale d'un fork, ni revalidation indépendante des booléens de hash de transaction/reçu. Aucun runtime C++ promu dans main. Aucun déploiement public. La revue automatisée des résultats n'est pas une revue de sécurité indépendante. Tous les critères de production globaux restent ouverts.
