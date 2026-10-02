# Profil interne Bellatrix — campagne terminée, timeout conservé

Source cœur c10426b2, image AMD64 existante, volumes et bibliothèques en lecture seule. Réutilisation de l’objet du test de partition déjà instrumenté ; seuls deux objets cœur de diagnostic sont recompilés puis liés avant la bibliothèque historique.

Scénario original `consensus_bellatrix_partition_simulation`, 112 slots, timeout CTest 240 secondes, aucune assertion réduite. Mesures imbriquées : reactor vote/bloc/publication, copie du store, contexte du vote, vérification/fork-choice, sérialisation du snapshot. Ne pas additionner des mesures imbriquées. Les compteurs du diagnostic sont protégés par mutex ; traces cumulatives toutes les 64 occurrences.

État constaté le 2 octobre : compilation terminée, CTest et binaire réellement actifs dans `nano-bellatrix-inner-profile-amd64-test`. Rapports sur VPS : `/home/ubuntu/nano-bellatrix-inner-profile-20261002-reports`. Aucun résultat final ou gain de performance revendiqué avant inspection des rapports terminaux et des manifestes.

Deux tentatives de préparation conservées sur VPS n’ont exécuté aucun test : chemin de contrôle erroné (`...-path-failure-reports`), puis sortie non inscriptible avec capabilities Docker supprimées (`...-permission-failure-reports`). Seul le dossier dédié de rapports est devenu inscriptible ; sources et volume restent en lecture seule.

Ce code instrumenté est réservé au diagnostic, sans promotion en runtime. Les échecs historiques, exigences de production et revue indépendante demeurent ouverts.

## Résultat terminal

Le cas expire en 240,025 secondes, dernier slot de phase observé 88. Sorties CTest/conteneur 8 ; zéro skip. Sources, programmes et bibliothèques identiques avant/après ; post-manifest à zéro.

Compteurs cumulés aux derniers multiples de 64 (fenêtres différentes selon la phase, mesures imbriquées) : publication 137,117 s sur 1 792 appels, dont snapshot 70,948 s sur ces mêmes 1 792 appels (51,74 %). Contexte de vote 12,457 s / 2 432 ; vérification/fork-choice 4,957 s / 2 432 ; copie store 1,929 s / 4 160. Ces valeurs ne forment pas une somme du temps total. Les appels après le dernier multiple et le travail interrompu à expiration ne sont pas entièrement comptabilisés.

La copie seule n’est pas le coût dominant. La sérialisation durable encode le store, le reparcourt via `Json::parse`, puis réencode le document englobant : prochaine correction candidate, éviter ce reparcours tout en conservant exactement les octets canoniques et la limite d’enveloppe. Aucune écriture synchrone/WAL désactivée, aucun délai augmenté. La publication restante inclut plusieurs opérations : ces mesures ne prouvent pas que le reste est exclusivement du fsync.
