# Diagnostic FFG de la reprise WAN

Analyse hors ligne du checkpoint RocksDB natif arrêté de la campagne
`sustained-recovery`, cœur `c41e7b4`. Le dump a été obtenu avec
`rocksdb::DB::OpenForReadOnly` et la clé `fork-checkpoint/v1/payload`.
Archive synthétique sans clés privées. Aucun nouveau réseau lancé.

Reproduction :
```sh
python3 docs/qualification/multihost-20260927/ffg-recovery-analysis/analyze.py docs/qualification/multihost-20260927/ffg-recovery-analysis/native-checkpoint.json.gz
```

Le script lit les offsets Bellatrix utilisés par
`src/consensus_bellatrix_fork_state.cpp`, les limites enregistrées dans le
checkpoint et les soldes effectifs non slashés actifs. Le profil de cette
campagne utilise 8 slots par epoch. Les lignes retenues pour le diagnostic
sont les ancêtres par liens parent du checkpoint finalisé 11.

| Slot | Poids cible précédente | Poids cible courante | Justifié | Finalisé |
|---|---:|---:|---:|---:|
| 63 | 34,4 % | 57,8 % | 3 | 2 |
| 64 | 60,9 % | 0 % | 3 | 2 |
| 66 | 76,6 % | 10,9 % | 3 | 2 |
| 70 | 85,9 % | 75 % | 3 | 2 |
| 72 | 100 % | 0 % | 8 | 2 |
| 80 | 100 % | 0 % | 9 | 8 |

Au slot 64, la cible précédente reste sous les deux tiers. Elle dépasse ce
seuil au slot 66 ; la cible courante le dépasse au slot 70. Les transitions
d’époque produisent la justification 8 au slot 72 puis la finalisation 8
au slot 80. Cela explique la marche temporelle visible dans cette branche :
réduire seulement un timeout d’observation ne corrige pas le rattrapage.

Prochaine investigation : distinguer votes arrivés tard et votes disponibles
mais inclus tard avant la frontière 64, puis examiner la sélection des
attestations par le proposant. Les compteurs de latest messages seuls ne
prouvent pas leur inclusion dans l’état ni leur poids de cible valide.

Limites : décodage diagnostic, pas revalidation des signatures, racines SSZ
ou transitions ; liens parent non revérifiés cryptographiquement. Un seul
stockage natif terminal, pas une chronologie d’arrivée des votes ni une preuve
causale sur tous les nœuds. Le délai initial de 180 secondes reste échoué.

## Délais d’inclusion et instrumentation suivante

`inclusion.py native-checkpoint.json.gz` reproduit `inclusion.json` : les
blocs de la branche finalisée entre slots 56 et 72 contiennent 2 à 21
attestations (capacité du profil : 128). Au slot 66, cinq attestations du
slot 61 sont incluses ; au slot 67, six du slot 63. La capacité des blocs
n’est donc pas saturée dans cet échantillon. Cela n’exclut pas un budget de
sélection épuisé par des candidats invalides : les candidats rejetés et les
instants de réception ne sont pas présents dans ce checkpoint terminal.

Le code de sélection parcourt l’archive par identifiant, ignore les votes
identiques déjà inclus sur les ancêtres récents, puis vérifie les candidats
par transition complète (`src/consensus_bellatrix_local_body.cpp`). Les
preuves actuelles ne permettent pas d’attribuer le retard à cette politique.

Les événements réseau de relais, auparavant sans horodatage absolu, reçoivent
`timeMs` à leur émission, y compris les réussites rapides. Ce temps système
sert à corréler les traces avec les slots ; il ne mesure pas la latence entre
machines sans qualification des horloges. `elapsedMs` reste la durée mesurée
par le relais. Aucun nouveau trafic, changement de validation, de délai ou
de budget. Les anciennes traces restent non horodatées ; cette modification
ne les enrichit pas rétroactivement et n’est pas encore qualifiée en WAN.

Validation de l’instrumentation : `cmake --build build-network --target nano-p2p -j 2` réussit (`relay-time-build.log`). Pas de nouvelle campagne réseau ni de changement du résultat WAN existant.

## Corrélation préparée pour la prochaine campagne

`node tests/multihost/inclusion-timing.mjs checkpoint.json.gz campagne/`
relie l’empreinte SHA-256 des attestations incluses aux événements
`test-altair-vote-relayed` horodatés. Il exclut les lectures `known` du cache.
Le résultat conserve toutes les correspondances, leur pair destinataire et
l’écart signé par rapport à l’ouverture du slot d’inclusion. Ce sont des
fins de relais enregistrées, pas une preuve de disponibilité chez le proposant.

Contrôles exécutés : archive précédente = 519 inclusions, 2606 événements
sans date, aucune correspondance datée inventée ; événement synthétique sur
une empreinte incluse = une correspondance à -123 ms ; événement `known`
identique exclu. L’outil n’altère aucun état et ne valide pas les signatures.
