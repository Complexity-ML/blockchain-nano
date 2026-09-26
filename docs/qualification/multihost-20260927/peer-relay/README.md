# Relais par pair : reprise tardive, critère WAN échoué

Révision `c41e7b4997c045e4ebc6d791a0fab8e45d8eaad9`. Trois instances, deux
hôtes, 64 clés synthétiques, slots de 6 secondes, epochs de 8 slots.
Commande :

```
NANO_TEST_NETWORK_TRACE=1 NANO_TEST_WAN_PARTITION=1 node tests/multihost/run.mjs ubuntu@145.239.199.166 c41e7b4997c045e4ebc6d791a0fab8e45d8eaad9 /private/tmp/nano-wan-peers-20260927
```

Images Mac `sha256:43ce6e8614f5bd3e4cf62074d6ae03993e64cd7a4142ad77096413bcd38f1319`,
VPS `sha256:7c9115d52d81ae2ae0a2f797c9ca1770ab46c8d4c217d15a9296a1310db95bea`.
Empreintes des exécutables et du banc dans `run.jsonl`.

## Verdict du banc : échec

Démarrage finalisé commun à l’epoch 2, coupure effective de 96 secondes,
reconnexion avec 1/1/2 pairs. Pas de nouvelle finalité commune observée dans
le délai de 180 secondes. Sortie 1 ; aucune fenêtre des trois avancées
successives n’a été exécutée. Le délai n’est pas relevé pour reclasser le run.

## Progrès observé distinct du verdict

Les trois statuts terminaux portent la même finalité d’epoch 8, racine
`0xfca4a6e7e6128158b972a4dc6e0ddccd5c4e48b4c7fb66e1d82df67f8cfeefff`,
et une justification 9. Les premiers échantillons conservés montrant cette
finalité sont au slot 80, environ 184 secondes après reconnexion (voir
`late-recovery-samples.json`). Ils ne datent pas exactement sa première
acquisition et ne remplacent pas le contrôle RPC du banc.

Au slot 64, les votes de l’epoch précédente connus sont 48/57/64 ; au slot 72,
64/64/64, contre 22/13/55 dans le run précédent. Les têtes et cibles signées
des échantillons deviennent communes au slot 64. Ce résultat est cohérent avec
un rattrapage amélioré ; il ne prouve pas un délai garanti ni une causalité
exclusive. Les échantillons ne sont pas simultanés ni un calcul de quorum.

Trois boucles saines, 201/191/188 attestations et 28/22/27 propositions admises.
Cycles maximum 551/581/3366 ms, aucun dépassement des 6000 ms du slot.
Les statistiques de relais sont ventilées par destination avec le script
`tests/multihost/relay-summary.mjs` à la révision `277b154` (diagnostic seul).
Elles portent seulement sur les tentatives lentes/échouées, pas sur le débit.

Après arrêt : deux conteneurs `exited`, PID natif 80496 absent. Bases conservées,
aucune configuration privée publiée. Il reste à expliquer la marge temporelle
et à qualifier la progression durable après reprise. Production non qualifiée.
