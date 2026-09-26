# WAN après acquittement idempotent des votes — échec

Révision `b2e4ebefdd521c8bea8f5dd1b0a54454f0bf5206`, trois instances, deux hôtes,
64 clés synthétiques (22/21/21). Exécution :

```
NANO_TEST_NETWORK_TRACE=1 NANO_TEST_WAN_PARTITION=1 node tests/multihost/run.mjs ubuntu@145.239.199.166 b2e4ebefdd521c8bea8f5dd1b0a54454f0bf5206 /private/tmp/nano-wan-retry-20260927
```

Images Mac `sha256:19c9912e9ce20af226c6f4c8a68d23a8b9f40d1ac9fed4f6e93688a82422b5ac`,
VPS `sha256:07957434fbd8732d7785a71b8387e26ff7182e5bdd16f6c9652e25f81938d662`.
Les empreintes des exécutables et du banc figurent dans `run.jsonl`.

Démarrage avec finalité commune à l’epoch 2, coupure effective de 96 secondes,
puis reconnexion avec 1/1/2 pairs. Aucun nouveau checkpoint finalisé sous
180 secondes : sortie 1. La phase des trois avancées successives n’est pas
atteinte. Le gain local sur les doublons ne résout pas le retard distribué.

Trois boucles saines, 203/173/191 attestations et 30/29/22 propositions admises.
Maximum par cycle : 605/873/5970 ms ; aucun dépassement du slot de 6000 ms.
Les compteurs couvrent tout le run et ne sont pas un benchmark contrôlé.

Au slot 64, les échantillons montrent 22/21/64 derniers messages de l’epoch
précédente. Au slot 72 : 22/13/55. Les échantillons ne sont pas simultanés et ne
représentent pas le quorum pondéré. Ils orientent le diagnostic vers le relais
sortant du VPS (un seul envoi global en vol pour deux pairs) et l’ordre de
rattrapage des votes. Aucune causalité exclusive n’est encore démontrée.

Diagnostic reproductible : `tests/multihost/relay-summary.mjs` et
`docs/qualification/multihost-20260926/wan-partition/vote-visibility.mjs`, chacun
avec le dossier de campagne en argument. Les tentatives de relais journalisées
sont uniquement les lentes/échouées ; `ack` inclut transport et admission.

Après terminaison, les deux conteneurs sont `exited`, le PID natif 68064 est
absent. Bases et journaux de signature conservés ; aucune configuration privée
publiée. Les preuves précédentes restent inchangées. Production non qualifiée.
