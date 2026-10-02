# Deux builds complets terminés, registres comparés — 2026-10-02

Candidat `0f53b42d`, réutilisation des images/volumes figés. Le build AMD64 termine exit 0 et entre en régression 427 cas ; ARM64 était déjà en régression. Vérificateur joint exécuté exit 0 sur les rapports actuels : deux build-exit 0, révisions exactes, 15 315 sources avant égales au manifeste gelé, 427 noms distincts et mêmes commandes/délais sur les deux architectures. Seule normalisation permise : le nom du sous-répertoire de build ARM64/AMD64. Aucune propriété DISABLED/SKIP présente.

Deux builds réussis ne constituent pas deux régressions réussies. Résultats terminaux, sources/programmes/link-inputs après tests, EEST exacts et audit signé du binaire VPS restent à vérifier. Observations : ARM64 80 cas terminés/427, AMD64 26 cas terminés/427, mêmes processus Docker 80870/582618 actifs sans OOM. Aucun nouveau build/test/volume/runtime modifié par ce contrôle.

Le contrôleur d’audit signé VPS PID 587064 reste vivant en attente ; il n’utilisera que le binaire exact de la campagne complète réussie. Gate WAN encore fermé. Toutes les erreurs historiques et tous les critères de production restent conservés.
