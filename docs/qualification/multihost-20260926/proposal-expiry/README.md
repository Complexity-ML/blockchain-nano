# Expiration d'une proposition locale

Correctif non encore qualifié sur WAN : l'expiration monotone au-delà du slot,
avant/pendant/après le fournisseur de signature d'une proposition, est classée
ExpiredBellatrixDuty et rend la main à l'ordonnanceur. Les autres exceptions
restent fatales ; le journal de signatures n'est ni effacé ni réinitialisé.
Les chemins attestation/RANDAO n'ont pas été modifiés par ce correctif.

Nouveau scénario --proposal-expiry-loop : passage au slot suivant pendant la
signature de proposition, absence de publication, réservation du slot conservée,
puis proposition admise au slot suivant avec boucle saine. Les contrôles complémentaires forcent aussi les fenêtres avant fournisseur
et après vérification, et refusent de classifier un recul d’horloge comme une
expiration normale. Le journal relu refuse une proposition contradictoire sans
appeler le signataire ; une erreur réelle du fournisseur reste fatale.

Trois tests passent en 21,91 s : ce scénario, la boucle complète et la chaîne
signée. Logs joints. La régression WAN observée dans with-batch reste ouverte
jusqu'à répétition avec ce correctif et contrôle des trois validateurs.
