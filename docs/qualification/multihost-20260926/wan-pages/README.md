# WAN avec inventaire paginé — échec de reprise

Révision des binaires et images : `5da8484164ac9897047607eff9e839986aaf3aa7`.
Trois instances sur deux machines, 64 clés synthétiques (22/21/21), slots de
6 secondes et epochs de 8 slots. Commande :

```
NANO_TEST_NETWORK_TRACE=1 NANO_TEST_WAN_PARTITION=1 node tests/multihost/run.mjs ubuntu@145.239.199.166 5da8484164ac9897047607eff9e839986aaf3aa7 /private/tmp/nano-wan-pages-20260926
```

Images : Mac `sha256:41d2eb0addcec1efca1added0bf7ad62d09dc4d6f1f700920d9a88b9b3a489e5`,
VPS `sha256:46de62dec4548999f55c83f9a5d1c7a26fbe06ea0d4cbee44e5682bd21dba8d3`.
Les empreintes des exécutables et du banc sont dans `run.jsonl`.

Le démarrage atteint un checkpoint commun d’epoch 2 lié au bloc d’exécution.
La coupure de 96 s est effective (zéro pair) et la reconnexion rétablit 1/1/2
pairs. Le délai de reprise de 180 s expire : finalité toujours 2 partout,
justification 7/7/7 à l’arrêt. Le contrôle des trois avancées suivantes n’a
pas été atteint. Code de sortie du contrôleur : 1.

Les trois boucles sont saines, sans erreur fournisseur ; 195/179/183
attestations et 31/29/22 propositions admises. Le maximum de traitement d’un
cycle atteint 597/731/6360 ms ; le VPS dépasse une fois les 6000 ms du slot.
Ces valeurs sont des observations de cette campagne, pas un benchmark contrôlé.

Les échantillons de signature montrent au slot 56 respectivement 22/21/64
messages de l’epoch précédente connus, puis 22/17/60 au slot 64. La pagination
n’élimine donc pas l’asymétrie. Les échantillons ne sont ni simultanés ni une
mesure pondérée du quorum. La priorité reste de comprendre les votes refusés
ou reçus trop tard et le coût de leur admission avant de modifier le consensus.
Ne pas augmenter le délai pour transformer ce résultat en réussite.

Après terminaison : deux conteneurs `exited`, PID natif 55824 absent ; bases et
journaux de signature conservés, aucune configuration privée publiée. Les logs
complets, checkpoints, topologie et diagnostic reproductible sont joints.
Aucun nouveau statut de conformité EIP ni approbation de production.
