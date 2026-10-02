# Profil interne Bellatrix — campagne démarrée, résultat non acquis

Source cœur c10426b2, image AMD64 existante, volumes et bibliothèques en lecture seule. Réutilisation de l’objet du test de partition déjà instrumenté ; seuls deux objets cœur de diagnostic sont recompilés puis liés avant la bibliothèque historique.

Scénario original `consensus_bellatrix_partition_simulation`, 112 slots, timeout CTest 240 secondes, aucune assertion réduite. Mesures imbriquées : reactor vote/bloc/publication, copie du store, contexte du vote, vérification/fork-choice, sérialisation du snapshot. Ne pas additionner des mesures imbriquées. Les compteurs du diagnostic sont protégés par mutex ; traces cumulatives toutes les 64 occurrences.

État constaté le 2 octobre : compilation terminée, CTest et binaire réellement actifs dans `nano-bellatrix-inner-profile-amd64-test`. Rapports sur VPS : `/home/ubuntu/nano-bellatrix-inner-profile-20261002-reports`. Aucun résultat final ou gain de performance revendiqué avant inspection des rapports terminaux et des manifestes.

Deux tentatives de préparation conservées sur VPS n’ont exécuté aucun test : chemin de contrôle erroné (`...-path-failure-reports`), puis sortie non inscriptible avec capabilities Docker supprimées (`...-permission-failure-reports`). Seul le dossier dédié de rapports est devenu inscriptible ; sources et volume restent en lecture seule.

Ce code instrumenté est réservé au diagnostic, sans promotion en runtime. Les échecs historiques, exigences de production et revue indépendante demeurent ouverts.
