# Tentative B : diagnostic instrumenté de partitions répétées

Le second délai initial de 180 s manque à nouveau (191,767 s de reprise).
Le premier cycle passe en 135,381 s. Les binaires, profils et budgets sont
identiques à A ; seules des observations supplémentaires ont été activées.

Les premières observations de head commun, de justification plus récente et de
finalité plus récente sont enregistrées séparément dans `timing-analysis.json`.
Cycle 1 : 87,027 / 87,027 / 135,381 s. Cycle 2 : 105,612 / 142,005 / 191,767 s.
Une époque dure 48 s. Ce sont des instants échantillonnés, pas des mesures
continues ni une preuve causale d’un défaut du relais ou de la transition FFG.
Le partage du relais entre six types de messages est une piste à examiner ;
aucune correction de consensus ou de débit n’a été appliquée pendant ce test.

La campagne termine à exit 1. La fenêtre tardive échoue ensuite sur HTTP 503
(Beacon head, Docker Mac). Le log indique `Capella reactor unavailable; reopen
required; initial failure: Capella local clock discontinuity`. Docker Mac sort
à 1 sans OOM, et ne fournit aucun reçu terminal de validateur ; le VPS sort à 0
sans OOM. Les journaux et états terminaux sont conservés. Le défaut d’horloge
est explicite ; le type exact (retour avant temps durable/observation précédente,
ou saut au-delà de 120 s) et son amplitude ne sont pas journalisés. Il reste à
les distinguer avant une correction. Le fail-closed ne doit pas être contourné.
L’ancien incident WAN A n’est pas expliqué rétroactivement par ce seul résultat.
