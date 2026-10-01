# Candidats du diagnostic sync

Les targets Nano et P2P ont terminé exit 0 dans les volumes existants ARM64 et amd64 depuis `b76cc1d`. Le marqueur de révision est écrit après ces succès. Les images sont assemblées sur les bases locales immuables, sans téléchargement ni données de chaîne, avec vérification SHA-256, ldd et --help. Leurs identités, labels, architectures, utilisateur 10001 et empreintes sont relus après assemblage. Le Nano natif est également recompilé ; le P2P natif demeure inchangé.

Le cœur contient les diagnostics de `2e16522` qualifiés localement dans `../sync-signing-clock-20261001/`. La source P2P est inchangée. Aucun nouveau test réseau ou effet sur la cause WAN G n’est revendiqué. Ces candidats ne sont pas qualifiés production ; tous les critères et échecs historiques restent ouverts.
