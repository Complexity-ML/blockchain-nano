# Éviter le retour d’un vote à son émetteur

Le chemin de réception utilisait l’admission durable mais ne renseignait pas
le cache de possession du pair émetteur. Le parcours sortant pouvait donc
lui réexporter le même vote. `remember_vote_sender` calcule l’identifiant
SHA-256 des octets SSZ et, uniquement après décision `accepted`, mémorise
cette possession dans le cache existant borné de sa session.

Aucun curseur sortant avancé par la réception. Aucune validation BLS/consensus
contournée. Les autres pairs ont des caches indépendants et continuent à
recevoir le vote. La fonction capture l’objet de session existant ; une
complétion tardive ne renseigne pas celui d’une reconnexion. Le cache reste
facultatif : son échec d’allocation ne supprime pas la notification de
complétion durable. Le traitement s’effectue sur l’exécuteur I/O existant.

## Nouvelle couverture

- Avant admission, après rejet ou erreur locale : aucune connaissance ajoutée.
- Refus de mise en file : aucune connaissance ni complétion artificielle.
- Un bloc accepté ne renseigne pas le cache des votes.
- Vote accepté : seule la session émettrice connaît l’empreinte ; curseur intact.
- Relais sortant réel injecté : zéro envoi de retour vers cet émetteur,
  puis un envoi avec accusé pour un autre pair.

Les deux tests CTest de progression du relais et d’isolation des pairs passent.
Compilation `nano-p2p` réussie. Les sources et résultats sont conservés ici.

Cette qualification démontre la suppression de l’écho dans le scénario
injecté, pas une réduction mesurée de latence WAN. Le délai de reprise
180 s demeure non qualifié ; aucune nouvelle campagne distribuée n’a encore
évalué cette correction. Aucun changement de consensus ni activation EIP.
