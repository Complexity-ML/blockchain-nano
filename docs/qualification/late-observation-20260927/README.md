# Observation après délai manqué, verdict inchangé

La campagne c41e7b4 a montré une finalité commune tardive dans ses traces
terminales, sans laisser observer sa progression durable. Le banc poursuit
maintenant son observation bornée (trois avancées consécutives liées à
l’exécution) après la seule erreur typée PartitionRecoveryDeadline.

Le délai de 180 secondes reste un échec : `partition-recovery-deadline-missed`
est enregistré avant l’observation, puis `late-finality-passed` ou
`late-finality-failed`. Le processus conserve une sortie non nulle ; aucun
événement global `passed` n’est émis après ce retard. La santé des validateurs
est contrôlée également après un scénario échoué, dans un événement séparé.

Les erreurs de restauration, de RPC et de sécurité ne prolongent pas le test.
Les 9 tests de partition et 4 nouveaux tests d’orchestration passent (13/13).
Cette vérification porte sur le banc, pas sur le consensus ou le réseau réel.
Les seuils et les échecs historiques restent inchangés. Les images existantes
c41e7b4 peuvent être réutilisées : aucun changement de code exécuté par les nœuds.
