# Store de branches Capella en mémoire

Le Store Capella conserve les branches et leurs états consensus/EVM couplés,
reçoit les blocs signés, applique horloge, finalité et proposer boost, traite
votes/slashings et sélectionne la tête. L'ancre doit être déjà authentifiée
extérieurement. Son payload est décodé jusqu'au début des changements BLS,
pas jusqu'à la fin du corps. Le header d'état est décodé jusqu'aux résumés
historiques. Les engagements SSZ et MPT des retraits sont vérifiés séparément.
Les helpers de dépôts natifs ont également une interface Capella.

Qualification bornée : trois chaînes synthétiques de neuf blocs avec retraits,
une ancre Capella déjà validée par scénario, huit admissions suivantes et
une branche concurrente supplémentaire. Vérification des têtes couplées,
doublons, mauvaises signatures, corps de retrait altéré et annulations.
Régression Bellatrix signé / Capella signé et Store / migration officielle :
3/3 suites. Le Store est initialisé au premier bloc Capella authentifié ;
ce test ne transporte pas les branches ni votes Bellatrix antérieurs.

Limites : les API de votes, slashing et dépôts natifs sont implémentées mais
leur qualification spécifique Capella reste requise. Le test n'établit pas
une finalité multi-époque via votes réseau. Snapshot/restauration, profil
persisté, Store traversant les forks, commit atomique et réacteur restent
à intégrer. Aucun lancement ou profil actif modifié, aucune conformité
production déduite de cette qualification.
