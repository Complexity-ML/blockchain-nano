# Lecture cohérente du moniteur de finalité

Un nouveau test reproduit un faux retard : checkpoint de l’ancienne tête
puis slot de la nouvelle tête, lors d’une frontière d’époque. Avant correction,
six tests passent et ce nouveau cas échoue avec le même message de retard.

Le moniteur encadre désormais la lecture du checkpoint par deux lectures
de racine de tête. Si elles diffèrent, il conserve la trace de cette course
et recommence, au plus trois fois. Les conflits et reculs de finalité sont
testés immédiatement, y compris pendant une course ; les seuils de retard,
de blocage, de succession d’époques et de liaison avec l’exécution sont inchangés.
Cette méthode détecte les changements de tête observables entre ces lectures ;
elle n’est pas une API transactionnelle ni une preuve contre un aller-retour
de tête indétectable entre deux requêtes.

Neuf tests passent, dont la nouvelle course, le churn qui épuise le budget et
un conflit de finalité pendant changement de tête. La qualification réseau
avec le même binaire corrigé du relais échoue en 373,31 s, toujours
sur le retard de finalité. La course de lecture ne suffit donc pas à expliquer
cet échec.

L’analyse hors ligne du checkpoint authentifié démontre une différence de
projection : les états des blocs 62 et 63 portent la finalité réalisée 2 et
la finalité non réalisée 6. Le Store a pu promouvoir cette dernière ; l’état
du bloc 64 expose bien la finalité réalisée 6. L’ancien seuil de démarrage
de la fenêtre utilisait le Store via le RPC d’exécution, alors que le moniteur
utilise la finalité réalisée de l’état Beacon. Il démarrait avant que ces
deux projections aient rattrapé. Voir `source-beacon.json`.
