# V29 : refus de capacité catalogue, avant disponibilité réseau

Contrôleur original82373/1, coree33ad89e et checkout77566654. Préflight réussi, puis VPS arrêté1 sans OOM après94s : `Capella catalog budget below configured inventory capacity`. Mac natif et Docker Mac non démarrés ; aucune finalité commune/duty qualifiée. Sources/snapshots de préparation et sauvegarde physique V28 conservés.

Cause : le graphe est configuré4096 records mais catalogs.rootBytes=65536 ne couvre que2048 racines de32 octets. La garde runtime était placée après recovery/migration, donc le checkpoint a pu être publié avant ce refus. Le profil/quorum n'est pas changé ; c'est une incohérence entre deux budgets administratifs.

Relecture VPS après arrêt, avec capture existante : sortie0, témoin indépendant vérifié, pins de33 signataires exactement inchangés, tous champs du rejeu canonique identiques sauf token de checkpoint. Publication principale/témoin avance exactement d'une séquence0x3695→0x3696 ; digest nouveau vérifié séparément. Les2716 fichiers sont inchangés durant la relecture. Pas de nouvelle signature ni reprise de duties. Le test ne qualifie pas une reprise active ou la finalité WAN.

Échecs des pilotes conservés : v1 requête encore liée aux anciens budgets ; v2 référence profile avec maximumBytes inadmissible ; v3 capture terminale0 mais comparaison finale exigeait à tort l'ancien token. Aucun rejeu de la capture v3 : vérification séparée1610/0 compare le token migré et exactement tous les autres champs, puis rehash readonly. Requêtes/configurations/listes de hashes privées restent sur VPS ; seulement capsules/logs publics archivés.

Correction en cours : garde catalogue avant toute ouverture de témoin/publication ; planner/helper exigent budget catalogue explicite quand nécessaire. Nouveau scénario bootstrap/migration sans écriture qualifié ARM64/natif ; AMD64 également terminé0 ensuite ; voir [lot qualifié](../runtime-catalog-preflight-20261006/README.md).78 tests planner/helper passent. Aucun nouveau réseau lancé ; aucune qualification production ou EIP déduite.
