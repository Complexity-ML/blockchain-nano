# Audit signé du binaire complet AMD64 — en attente, 2026-10-02

Contrôleur public préparé et transféré par SCP, pas de code runtime. Runtime figé `0f53b42d` toujours obtenu par Git. Processus du contrôleur confirmé vivant sur VPS : PID 587064 ; build complet observé vivant : PID Docker 582618, sans OOM. Son état réel est `waiting-verified-live-full-campaign`, aucun audit démarré.

Le contrôleur attend l’achèvement de la campagne existante `nano-full-coherent-0f53b42d-amd64` et ses manifestes terminaux, sans remplacer ni redémarrer de test. Échec de build, de régression ou de comparaison : refus. Handle absent/observation invalide sans résultat terminal : observation non résolue, aucune interprétation de succès. Toutes les observations sont bornées, espacées de 15 s. Délai d’observation total 24 h ; il n’étend aucun timeout de test.

Avant audit : vérificateur indépendant des 427 cas, zéro skip et résultats/exits cohérents ; 15 315 sources égales au manifeste gelé ; fixture publique identique octet par octet à celle qualifiée sur Mac ; hash exact du `nano` de la régression complète. Quatre requêtes signées dans un conteneur réutilisant l’image/volume existants, sans réseau, racine/volume sources/fixture en lecture seule, cap-drop ALL/no-new-privileges, 1 CPU/2 GiB et délai original de 180 s par cas. Aucun signer actif ni ouverture de réseau public.

Après audit : checkpoint valide et engagements attendus sur 99 blocs/97 transactions ; tête non finalisée, époque future et racine malformée refusées ; source/fixture/binaire inchangés. Résultat terminal seulement après ces assertions. Aucune reprise de duties ou qualification des votes historiques/Fork Choice déduite du rejeu seul.

Contrôleur `/home/ubuntu/nano-full-0f53b42d-amd-offline-control-20261002/queue.py`, journal `/home/ubuntu/nano-full-0f53b42d-amd-offline-queue-20261002.log`, futurs rapports `/home/ubuntu/nano-full-coherent-0f53b42d-amd64-offline-reports-20261002`. Contrôleurs syntaxiquement compilés ; voie d’audit terminale pas encore exécutée. Régression ARM64 active à 64/427 ; compilation AMD64 active à 85 %. Aucun résultat complet anticipé, gate WAN restant fermé.
