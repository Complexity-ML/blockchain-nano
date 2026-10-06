# Préparation locale VPS : options administratives

Helper Python et contrôleur raccordés pour transmettre retentionExperiment/coldCheckpoint, après vérification des capsules et pins. Support de la politique historique sans captureBudget et des nouveaux chemins AMD64 cohérents. Aucun changement de core, de profil, de clock ou de clés. Docker inspect se limite désormais à State.

25 tests comparent le helper au planner JavaScript indépendant : configuration complète identique en mode legacy et 4096 records/cache8, pins/clock préservés, refus des budgets/overflow/options inconnues, sticky cold, chemins étrangers, signataires retirés et pins manquants. La nouvelle option CLI est un JSON administratif sans clés ; les secrets VPS restent locaux.

Le test CLI complet vérifie également un refus avant écriture pour hash incorrect, puis la création locale conforme, les permissions0600 et l’absence du contenu de l’environnement dans la réponse. Suite intégrée :74/74 (25 nouveaux et49 planner).

Cette qualification est une dérivation de configuration. Aucun graphe de 4096 records n'a encore tourné, aucun lancement WAN ni nouvelle finalité n'est déduit. Le helper prépare des fichiers seulement après égalité du hash canonique attendu par le contrôleur.
