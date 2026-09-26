# Reprise WAN tardive puis finalité soutenue

Cœur et images `c41e7b4997c045e4ebc6d791a0fab8e45d8eaad9`, banc `14f1f15`,
audit hors ligne `61e84eb`. Trois instances sur deux hôtes, 64 clés synthétiques,
slots de 6 secondes et epochs de 8 slots. Aucun fonds ni transaction utilisateur.

```
NANO_TEST_NETWORK_TRACE=1 NANO_TEST_WAN_PARTITION=1 node tests/multihost/run.mjs ubuntu@145.239.199.166 c41e7b4997c045e4ebc6d791a0fab8e45d8eaad9 /private/tmp/nano-wan-late-stage-20260927
```

## Critère initial : échec

Coupure de 96 s après finalité initiale commune à 2, retour des pairs 1/1/2.
Aucune nouvelle finalité commune observée sous 180 s. La reprise commune à
l’epoch 8 est relevée 186922 ms après la reconnexion. Cette durée dépend de
l’échantillonnage ; elle ne date pas exactement la finalisation interne.
Sortie globale 1 et erreur initiale conservée. Aucun reclassement en réussite.

## Fenêtre après reprise : réussite bornée

Finalité commune **8→9→10→11**, trois avancées consécutives en **143118 ms**.
Les checkpoints sont liés aux blocs d’exécution correspondants par le banc.
L’audit séparé des 207 checkpoints enregistrés entre phases ne détecte ni
conflit de racine pour une même epoch ni régression par nœud. Cet audit de
traces ne prouve pas l’ascendance cryptographique entre epochs.

Trois validateurs sains : 266/256/251 attestations et 38/29/35 propositions
admises. Maximum par cycle 581/619/3545 ms ; aucun dépassement du slot de
6000 ms. Statuts terminaux communs : epoch 11, racine
`0xb49429cbfeff8d76db176f65b0d027db31d99a45307e84682a9f794e212948e8`.

Les traces de relais concernent uniquement les tentatives lentes/échouées,
pas un débit exhaustif. Après arrêt, les conteneurs sont `exited`, PID natif
1887 absent, bases et journaux de signature conservés.

Cette campagne démontre une reprise suivie d’une progression dans ce périmètre.
Elle ne démontre pas le respect du délai initial, une endurance prolongée,
une charge utilisateur, trois domaines de panne ni une qualification production.
La marge temporelle de reprise et les autres critères des feuilles de route
restent ouverts. Tous les échecs antérieurs sont conservés.
