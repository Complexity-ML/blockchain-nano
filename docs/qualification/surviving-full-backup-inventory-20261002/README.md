# Sauvegarde complète conservée après reset Docker — inventaire courant

Fixture locale arrêtée du 30 septembre : `/tmp/nano-backup-restore-20260930/arm64/final/restored`. Inventaire indépendant courant : **313 fichiers, 121 445 250 octets**, chaque SHA-256 identique aux manifests source/restore avant/après de la qualification historique. Archive complète `data.tar.gz` vérifiée : `f26068c5cf3a4220750857362385bcbfc483b110e8e8832ac702348f37279ddb`.

La fixture contient `chain`, `core-witness`, 21 répertoires `key-N` avec leurs 21 répertoires `key-N-witness` (CURRENT présents), et `peer.key` de 32 octets. Aucun contenu de clé privée ni archive de données n'est publié dans Git. Ce matériel est plus complet que la copie historique chaîne seule utilisée pour certains audits offline du VPS.

Les chemins d'origine des inputs sont des chemins de conteneur supprimé (`/work`, `/out`, `/build`). Le Genesis et la requête externe séparée sont résolus sur l'hôte et correspondent exactement aux empreintes originales. L'ancien binaire Docker est indisponible après reset ; il faut qualifier/re-auditer le nouveau binaire plutôt que présumer la compatibilité. `inventory.json` distingue l'absence des anciens chemins de conteneur de la présence des deux inputs sur l'hôte.

Ce contrôle établit la conservation byte-identique de données/journaux/témoins. Il ne prouve pas une restauration avec `d1597d9a`, la concordance de chaque engagement anti-double-signature séparé, une reprise des duties, ni une finalité commune WAN. Ces critères et tous les autres gates de production restent ouverts. Les deux régressions complètes en cours restent nécessaires avant réutilisation opérationnelle.
