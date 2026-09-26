# Événements Beacon natifs — 2026-09-26

## Implémentation

`GET /eth/v1/events` accepte les topics `block` et `finalized_checkpoint`,
par virgules ou paramètres `topics` répétés. Le format est SSE HTTP chunked
`text/event-stream`, avec commentaires de maintien de connexion.
Les autres topics sont refusés (400), sans notification synthétique.

Les événements sont préparés avant l'écriture du checkpoint, puis publiés sous
le même verrou que l'état, après le succès du commit durable. Les ancres initiales,
imports refusés et doublons ne produisent pas de notification de bloc. Le topic
`block` décrit une importation validée, pas nécessairement un bloc canonique.
La notification de finalité contient le checkpoint publié et la racine d'état
de son bloc. Aucun événement ne change les règles du consensus.

## Bornes opérationnelles

- Journal éphémère de 256 événements, sans états complets retenus.
- Quatre flux simultanés par serveur HTTP local ; capacité excédentaire : 503.
- Polls via les deux workers et huit réservations partagés avec les lectures RPC.
- Au plus un poll/une écriture en cours par flux ; intervalle de poll d'une seconde.
- Sortie par lot plafonnée à 256 Kio ; délai d'écriture de cinq secondes.
- Curseur perdu : fermeture du flux, jamais de saut silencieux dans le journal.
- Reconnexion au présent, sans reprise historique. `Last-Event-ID` est refusé :
  le consommateur doit relire head/finalité via REST après une interruption.
- Bind loopback et refus de l'en-tête Origin conservés.

Ces bornes limitent les ressources d'observation, pas le nombre de validateurs.
L'acquisition du verrou du réacteur reste dépendante de la durée du commit en
cours ; ce raccordement ne prouve pas une latence maximale sous charge.

## Résultats

**5/5 tests passés**, 95,64 s au total (logs : `tests.log`).

| Test | Résultat nouveau | Durée |
| --- | --- | --- |
| managed_runtime | SDK : événement natif slot 4 et en-tête durable identiques ; redémarrage des processus | 18,93 s |
| validator_loop | 31 événements de blocs, dernière finalité identique au checkpoint durable ; journal vide après réouverture | 69,52 s |
| beacon_reads | Topics/encodages, plafond de 4 flux, journal de 256 entrées, fermeture sur perte de curseur | 1,38 s |
| beacon_headers | Signature invalide/annulation sans événement ; import validé unique ; doublon silencieux | 0,64 s |
| beacon_headers_reorg | Régression : deux changements de branche, archive et vue canonique cohérentes | 5,17 s |

La réception SSE de finalité par le SDK n'est pas observée dans le scénario
court ; la finalité est vérifiée au niveau du journal du réacteur. Le test de
perte de curseur utilise un producteur de notifications synthétiques ; le test
SDK utilise bien les blocs produits par Nano. Aucune panne disque physique
supplémentaire n'a été simulée pour cette tranche.

## Vérification

Commande locale :

```sh
cmake --build build-cmake --target nano nano-bellatrix-transition-tests -j4
NANO_TEST_BEACON_SDK_PROBE="$PWD/.deps/beacon-sdk-probe" \
  ctest --test-dir build-cmake \
  -R '^consensus_bellatrix_(beacon_reads|beacon_headers|beacon_headers_reorg|managed_runtime|validator_loop)$' \
  --output-on-failure
```

La sonde utilise `go-eth2-client v0.1.6`, la dépendance Beacon d'Assertoor,
avec son contrôle NodeSyncing actif. Elle souscrit aux événements, reçoit un bloc
natif et vérifie son slot via l'en-tête durable obtenu par sa racine.

La suite Assertoor complète n'est pas exécutée ici. La réception d'un événement
n'est pas une qualification de partitions ou de finalité distribuée. Les autres
topics, l'accès depuis son environnement d'orchestration et les besoins de
métadonnées des scénarios Assertoor restent à qualifier.

Source de protocole : [Beacon API eventstream](https://github.com/ethereum/beacon-APIs/blob/master/apis/eventstream/index.yaml).
Les topics de base utilisés par Assertoor ont été vérifiés dans
`pkg/clients/consensus/clientlogic.go` et `rpc/beaconstream.go`, checkout
`89366be6cb211522249a6d9424adb47631864432` ; son flux avancé
`execution_payload_available` est séparé du flux de base.
