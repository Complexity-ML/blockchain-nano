# Pagination des votes acquittés

Le relais demande jusqu'à 32 entrées de vote par lecture locale, dans la limite
du budget de parcours restant. Il ignore uniquement les racines déjà acquittées
par ce pair dans cette session. Au premier vote inconnu, il exporte et envoie
un seul message ; la réponse perdue conserve le curseur sur son prédécesseur.
Les inventaires de blocs, preuves et sorties restent lus un par un.

Le test nano-altair-relay-progress-tests passe : 70 entrées acquittées puis
une entrée nouvelle utilisent 3 lectures au lieu de 71, toujours en trois
passes bornées. Sont aussi vérifiés : rejet puis successeur, nouvelle tentative,
changement de catalogue, nouvelle session, éviction du cache, réponse perdue,
ordre des blocs, pages dupliquées ou trop grandes. Aucun acquittement n'est
fabriqué à partir de la présence dans l'inventaire.

Ce résultat mesure des appels au cœur dans le banc, pas la latence WAN. Le
relais conserve son délai global et n'envoie pas de nouveaux votes en parallèle.
La reprise WAN précédente reste en échec ; une nouvelle image et une campagne
aux mêmes critères sont nécessaires avant toute conclusion réseau.
