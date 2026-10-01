# Diagnostic monotone des reprises WAN

Le banc conserve désormais la durée monotone totale à l'échec, le dépassement exact du budget et la dernière lecture complète (incluant validation et journalisation existante), son checkpoint et l'observation ou non d'une finalité commune avancée. Ces valeurs sont figées avant journalisation du diagnostic et conservées dans `partition-recovery-deadline-missed`, même si la fenêtre tardive réussit. Aucun nouvel appel RPC n'est ajouté.

Quatre nouveaux cas déterministes : lecture réussie à 10001 ms rejetée pour un budget 10000 ms et preuve intacte après observation tardive ; journalisation de reconnexion épuisant le budget sans inventer une lecture ; finalité bloquée au seuil avec dernière lecture non avancée ; transmission des valeurs exactes par l'observateur tardif. Les trois fichiers de tests pertinents passent 37/37. Ces nombres synthétiques ne sont pas des mesures WAN.

Seuil WAN 180000 ms, coupures 96000/192000 ms, fréquence des observations et règle de rejet après lecture inchangés. Les campagnes M/N restent en échec et leur durée monotone manquante ne peut pas être reconstituée par cet ajout. Pas de nouvelle campagne ni gain de finalité revendiqué.
