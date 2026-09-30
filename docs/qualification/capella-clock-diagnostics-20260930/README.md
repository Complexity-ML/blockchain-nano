# Diagnostic exact des discontinuités d’horloge Capella

Source C++ `8d06ff5` : la même garde distingue maintenant les valeurs `nowMs`,
`durableMs`, `observedMs`, `maximumForwardStepMs` et les trois prédicats
`beforeDurable`, `beforeObserved`, `forwardStepExceeded`. Aucun seuil n’est
augmenté ; aucun saut n’est accepté par cette modification. Le préfixe existant
et la propagation de l’erreur initiale sont conservés.

Quatre nouveaux scénarios utilisent le vrai `CapellaLocalClock`, un reactor
lié à un propriétaire d’exécution et un stockage RocksDB/témoin : retour avant
le temps durable, retour avant une observation coalescée, dépassement du saut
maximal, limite exacte autorisée. Ils passent dans Docker ARM64 et Docker
x86-64 du VPS (processus exit 0). Les trois défauts ferment bien les accès
au consensus et préservent exactement le checkpoint ; l’invalidation répétée
conserve le premier diagnostic. La limite exacte reste acceptée sans publication
inutile dans la phase inchangée.

Le premier build ARM64 a échoué sur une déclaration `auto` de deux types
incompatibles dans le nouveau test ; elle est corrigée, son log conservé.
Volumes et images de dépendances réutilisés. Pas de test réseau ajouté ici,
pas de nouvelle image d’exécution déployée, pas de reproductibilité déduite.
Les images candidates historiques restent compilées à `ca780d5`. Les valeurs
manquantes de l’incident WAN B ne peuvent pas être retrouvées rétroactivement.
Sa cause physique, la reprise >180 s et l’incident WAN A restent ouverts.
