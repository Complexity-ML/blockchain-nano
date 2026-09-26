# Cohérence de finalité entre phases

`node tests/multihost/audit-history.mjs <dossier-campagne>` conserve un seul
historique de racines et d’epochs par nœud, depuis le checkpoint initial,
pendant la partition et jusqu’aux observations ultérieures. Les lectures ayant
croisé un changement de tête sont incluses : elles ne doivent pas effacer un
conflit de finalité. Des noms manquants/inconnus ou une observation partielle
font échouer l’audit. Les encodages équivalents d’epoch sont normalisés.

Six tests passent : parcours cohérent, régression entre phases, conflit à la
frontière, conflit pendant une lecture instable, données incomplètes/inconnues,
et encodages d’epoch équivalents. La campagne historique `with-expiry` contient
64 checkpoints relus, sans conflit observé, et se termine à l’epoch 5 partout.
Elle ne comporte pas de partition ; les contrôles négatifs couvrent cette
frontière artificiellement. Aucun résultat historique n’est reclassé.

Cet audit de traces est complémentaire : il ne prouve ni l’ascendance entre
checkpoints d’epochs différentes, ni la liaison à l’exécution, ni la vivacité,
ni l’exhaustivité des événements réseau. Il ne modifie pas la campagne active.
