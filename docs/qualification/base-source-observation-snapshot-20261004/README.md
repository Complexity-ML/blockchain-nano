# Stabilité des observations RPC — 4 octobre 2026

Trois nouveaux scénarios échouent sur l’implémentation initiale (24 succès,
3 échecs, exit 1) : configuration de l’événement modifiée pendant un await,
reçu mutable partagé par un cache et checkpoint finalisé partagé réécrit.
Ces cas étaient acceptés alors que la définition revue ou l’observation avait
changé. Logs avant correction conservés : échecs intentionnels de contrôle,
pas dépôts réels, incident public ou échec de la campagne C++.

Correction : copie des champs vérifiés des headers et reçus, topics détachés,
et définition exacte de l’événement copiée avant le premier await. Les champs
non vérifiés d’un reçu ne sont plus propagés dans l’observation retournée.
Même lot ciblé : 27/27, zéro échec/skip, exit 0. Hashes des modules et logs
avant/après conservés ; aucune campagne C++ ou image Docker relancée.

Le transport HTTP normal retourne des objets JSON distincts ; ces scénarios
visent aussi les transports injectés/caches/configurations mutables. Ils ne
qualifient pas l’intégration à un bridge ni un dépôt Base réel. Un RPC peut
toujours fabriquer des réponses cohérentes. Aucune preuve L1 ou d’inclusion
du reçu ajoutée : cryptographicFinalityVerified et eligibleForMint restent false.
