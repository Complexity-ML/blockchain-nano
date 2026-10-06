# Sélection du catalogue entre tentatives — qualification ciblée

Base `19b722bd`, 345 sources figées dans `sources/`. Le contre-exemple du préfixe répété demeure dans `../catalog-prefix-starvation-baseline-20261006/` ; la campagne v16 reste un échec de finalité WAN.

La sélection par pair et catégorie reprend après le dernier élément tenté. Elle ne constitue ni admission ni ACK ; refus et réponses incertaines sont réessayés après bouclage. Une nouvelle session ou génération remet la sélection à zéro. Le seek recherche dans le snapshot stable sans assouplir la validation d'appartenance du curseur de page ; authentification privée, propriété, budgets et annulation restent contrôlés. Fin d'un suffixe ne signifie pas parcours complet.

Qualification native et Docker ARM64/AMD64 : 136 unités de cœur rescannées, 18 dépendantes recompilées depuis l'archive précédente ; scénarios runtime configuré, reprise automatique, HTTP, wire. Huit vérifications de bornes seek et trois propriété/limites, neuf engagements indépendants de reprise, deux réouvertures SIGKILL dans wire. SDK : sélection 9 cas nouveaux, catalogue 46, tête 19, diagnostic 7, historique 10. Tests SDK modélisés ; ils ne prouvent pas la finalité WAN. Binaire nano-p2p compilé sur les trois environnements.

Échecs des lanceurs natifs conservés : mauvais nom de fichier, définition CMake NANO_HEAD_HISTORY_FIRST omise, répertoire de lien absent. La définition CMake rétablie fait passer les dix cas historiques ; aucune source corrigée pour cacher ces échecs. Les images et volumes sont réutilisés, sans build d'image ni transfert de base/configuration/clés entre hôtes.

Pas de qualification release sans instrumentation, de finalité commune à trois nœuds, de charge générale, ni de production par ce lot. La reprise v17 devra conserver les engagements post-v16 et le critère original de trois finalités nouvelles en 900 secondes. Revue indépendante et tous autres critères du goal restent ouverts.
