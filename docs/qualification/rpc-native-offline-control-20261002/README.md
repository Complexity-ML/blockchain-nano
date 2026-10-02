# Audit signé natif d159 de la sauvegarde complète

Conditionné au statut terminal zéro des 14 cas natifs et des 21 cas réseau, révision propre, référence des 15 315 sources et relecture des 598 entrées de programme/liaison/dépendances conservées. La demande positive est exactement la requête préparée à partir du reçu d’arrêt indépendant : epoch `0xc`, racine `af6a0f3a…`, séquence cœur `0x47b`, digest `89a62267…`.

Le binaire macOS `d1597d9a` reçoit quatre requêtes : positive, tête utilisée à tort comme checkpoint finalisé, epoch futur et racine malformée. Sandbox macOS refusant toute écriture de fichier et tout réseau ; maximum 180 secondes par cas inchangé. Les 313 fichiers de la fixture complète, sources et entrées du programme doivent rester identiques avant/après. Rejeu canonique attendu : 99 blocs, 97 transactions ; checkpoint et tête comparés aux engagements séparés.

Rapports `/tmp/nano-rpc-native-offline-reports-20261002`. Pas de démarrage de signataire, pas de mutation de RocksDB, pas de validation du contenu des journaux anti-double-signature et pas de finalité WAN. Ce contrôleur ne vaut pas audit complet du fork-choice ou des votes autonomes historiques ; ces limites restent explicites.
