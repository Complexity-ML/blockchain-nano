# Attendre la reprise avant la fenêtre de stabilité

Le délai initial de 180 s reste inchangé et son dépassement reste un échec.
Après ce dépassement seulement, le banc peut observer pendant au maximum
180 s supplémentaires le retour d’un checkpoint commun supérieur au baseline,
avec pairs reconnectés. Cette lecture réutilise les mêmes cartes de checkpoints
que la coupure : ni régression ni conflit ne sont effacés par une nouvelle phase.

Une fois la reprise tardive observée, et seulement alors, la fenêtre existante
de trois avancées consécutives démarre avec tous ses critères inchangés, dont
la limite de retard de finalité. Aucun contrôle de stabilité n’est désactivé.
Les événements `late-partition-recovery-bound` et `late-finality-passed` restent
distincts ; même s’ils arrivent, le processus termine en échec pour le délai
initial manqué. La santé des validateurs est rapportée séparément après arrêt.

19 tests réussis : 13 pour partition/restauration et observation tardive réelle
avec horloge injectée, 6 pour l’orchestration. Ils couvrent l’absence de seconde
coupure, la borne supplémentaire, les conflits/régressions après expiration,
la phase de reprise obligatoire et la conservation de l’échec original.
Ce sont des preuves du banc, pas des preuves réseau.
