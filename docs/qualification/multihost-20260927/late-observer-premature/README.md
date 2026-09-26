# Fenêtre de stabilité démarrée avant reprise — échec conservé

Cœur c41e7b4, banc 88fc78f, logs du 27 septembre. La coupure de 96 s est
restaurée, mais le délai de reprise de 180 s échoue. L’observateur de stabilité
commence alors que la finalité est encore à 2 (tête au slot 79) et échoue sur
`Finality lag exceeds four epochs`. Aucune progression de trois epochs n’est
qualifiée. Les trois boucles de validation sont saines à l’arrêt (195/188/193
attestations). Sortie 1 ; aucune réussite globale.

Le contrôle de retard a fonctionné. Le banc doit attendre une reprise tardive
commune, avec une borne distincte et les contrôles de conflits conservés,
avant d’ouvrir une nouvelle fenêtre de stabilité. Cette observation ultérieure
ne doit ni relever le seuil initial ni effacer l’échec à 180 secondes.
