# Qualification native et audit signé de d1597d9a

Build natif neuf macOS : 14 cas cœur/RPC et 21 cas réseau réussis, délais explicites d’origine, aucun skip. Statuts CTest/audits/contrôleur tous zéro. Relecture indépendante : noms/commandes réseau et délais conformes au CMake de la source figée, JUnit exacts, 15 315 sources et 598 entrées de programmes/objets/archives/link.txt/dépendances disponibles inchangées et relues sur disque. Ancien helper du dépôt jamais utilisé.

Audit signé hors ligne : quatre cas [0, 1, 1, 1]. Le positif rejoue 99 blocs et 97 transactions, vérifie le cœur sequence `0x47b` / digest `89a62267…` et le checkpoint finalisé externe epoch `0xc` / racine `af6a0f3a…`. Les demandes tête utilisée comme finalité, epoch futur et racine malformée sont refusées avec les erreurs attendues. Les 313 fichiers de la fixture complète, sources et 598 entrées du binaire restent identiques avant/après. Sandbox refuse réseau et écritures ; aucune signature active.

Ces cas réutilisent des contrôles pertinents pour qualifier le nouveau binaire après le reset et la modification RPC ; ils ne constituent pas une nouvelle couverture complète de consensus. Sources, cache/configure, commandes, logs build/CTest/JUnit, manifests et refus sont dans l’archive ; SHA dans `SHA256SUMS`. Aucun fichier de données ni clé privée n’est inclus.

Limites maintenues : pas de régression native complète, dyld système non intégralement audité, ni rejeu historique des votes autonomes/fork-choice. Journaux des signataires conservés mais contenu pas encore audité par la reprise ; aucun duty ou WAN démarré. Les 427 cas Docker sur chaque architecture et leurs gates restent indépendants et en cours. Production non qualifiée.
