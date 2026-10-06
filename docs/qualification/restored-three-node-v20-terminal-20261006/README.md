# Reprise v20 : finalité commune non avancée

Contrôleur original 34949, checkout e8008698, cœur d8d5b46c et relais diagnostic a373efe7. Sortie terminale 1 à 08:41:14 UTC. Trois propriétaires restaurés et actifs ; disponibilité IPC réussie cette fois, sans explication rétroactive du démarrage v19. Zéro nouvelle finalité commune sur les trois demandées en 900 secondes. Epoch finalisée 1284. Des blocs et des têtes avancent mais cela ne qualifie pas la finalité.

Trois arrêts normaux, zéro OOM indiqué. Audits read-only : engagements de reprise et témoins de clés identiques aux reçus indépendants, historique canonique et ancêtre finalisé vérifiés. Les preuves d'immutabilité donnent le nombre exact de fichiers ; les configurations et requêtes VPS restent dérivées et utilisées sur le VPS. Aucune base ni clé privée transférée.

Les deux captures Mac montrent des participations timely-target limitées au groupe local. Le reader étendu réutilisé mesure la pression du graphe ; tous les autres champs correspondent exactement au reader qualifié et les bases restent inchangées. Ce diagnostic à l'arrêt ne démontre pas l'absence de saturation transitoire ni la cause WAN.

Les logs complets enregistrent des admissions de blocs, des envois non confirmés et des erreurs RPC. Un catalog close admission-rejected n'établit pas un refus de vote distant : le wrapper normalise également les dépendances de blocs non satisfaites. Les erreurs RPC génériques ne prouvent pas une cause unique. Aucun quota, délai ou fencing n'a été relâché.

Brouillon isolé, non intégré : 48 cas d'attestation (observateur absent, présent ou lançant une exception) et 10 cas de classification RPC/redaction réussissent. Il distingue les dépendances non satisfaites, les ACK de vote et les deux messages HTTP fixes de saturation/expiration. Qualification native de diagnostic uniquement : aucune preuve de finalité WAN ni de transport réel ajoutée par ces fixtures.

Aucune image reconstruite, aucun redémarrage de la campagne, fonds, vente ou déploiement public. Le critère de finalité et toutes les exigences de production non démontrées, bridge/lancement et revue indépendante restent ouverts.
