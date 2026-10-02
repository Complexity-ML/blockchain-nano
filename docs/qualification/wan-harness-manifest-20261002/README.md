# Inventaire de sources du runner WAN corrigé

La liste précédente de 23 fichiers omettait restore-offline-audit.mjs, le contrôleur de stabilité Assertoor et des modules de supervision/livraison d’alertes. Le runner produit désormais l’inventaire de 31 fichiers : racines des commandes externes explicites et imports ES statiques relatifs récursifs. Il inclut le producteur de manifeste lui-même. Fichiers manquants et liens hors dépôt refusés ; cycles visités une fois. Pas de parsing générique de JavaScript ni preuve des exécutables, de Node ou du comportement WAN : ces limites et les autres manifestes demeurent nécessaires.

Six nouveaux cas Node passent : inventaire réel comprenant les modules omis, mutation d’une dépendance seule, imports multilignes/latéraux/cycles, dépendance manquante, import externe et symlink externe. La première version passante est complétée par la racine externe monitor-and-deliver ; seul le cas d’inventaire réel concerné est rejoué et passe. Zéro cas ignoré dans les deux exécutions. node --check du runner passe. CMake enregistre ce contrôle, sans reconfiguration ni rebuild des sources gelées actuellement testées.

Runtime main inchangé ; sources du candidat 12ad9eb5 et de 3030b799 inchangées. Cette modification du contrôleur doit être incluse et identifiée dans la prochaine campagne WAN ; aucun résultat WAN obtenu ici. Les quatre feuilles de route gardent tous les critères ouverts.

## Requalification réindexation AMD64 mise en attente

Le contrôle joint attend la fin réelle du conteneur de build cohérent 12ad9eb5 et exige ses statuts/manifestes avant de tester. Il sélectionne exactement trois cas : activation réindexée TIMEOUT 360, helper encodage TIMEOUT 60 et helper ancestry TIMEOUT 60. Aucun délai augmenté. Source/programmes/sept entrées statiques sont comparés avant/après ; registry et résultats exacts, zéro skip exigés. Le build est réellement actif au moment du lancement du contrôleur (PID 497885). Aucun résultat de ces cas encore acquis. Il ne lance pas de flood, de déploiement public, de vente ni de promotion runtime main.
