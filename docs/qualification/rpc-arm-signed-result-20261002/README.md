# Audit signé Docker ARM64 de d1597d9a

Quatre contrôles réussis après les 427 tests et le supplément réseau21 terminaux. Revue préalable refaite : sources, programmes, corpus EEST sélectionné et résultats réseau. Binaire du volume conservé, aucune recompilation, aucune image supplémentaire.

Le positif rejoue 99 blocs / 97 transactions et vérifie les engagements externes du cœur et du checkpoint finalisé. Tête présentée comme finalité, epoch futur et racine malformée refusés : sorties [0,1,1,1], mêmes délais de 180 secondes et mêmes requêtes que les audits natifs. Revue indépendante réussie. Sources et neuf fichiers de chaîne identiques avant/après ; seul le répertoire chaîne est monté, en lecture seule. Aucun journal de signataire ni clé privée monté. Réseau désactivé, racine du conteneur en lecture seule, image/volume existants.

Il s’agit du rejeu des contrôles signés existants avec le binaire Linux ARM64 désormais qualifié, pas de nouveaux scénarios de consensus. Pas de vérification historique exhaustive des votes/fork-choice, de reprise des duties ni de WAN. AMD64 reste en cours. Aucun runtime promu sur main, aucun lancement public, production non qualifiée.

Archive : rapports, contrôleur et requêtes publiques, revue terminale ; aucune base ou clé privée incluse. Les scripts conservés sont adaptés aux chemins de cette campagne, pas un lanceur générique.
