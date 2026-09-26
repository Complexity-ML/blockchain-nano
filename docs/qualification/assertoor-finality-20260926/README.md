# Finalité native sous observation Assertoor — échec ouvert

Date : 2026-09-26. Cible : `consensus_bellatrix_assertoor_finality`.
Assertoor officiel inchangé : `89366be6cb211522249a6d9424adb47631864432`.

## Périmètre

Deux processus Nano, deux démons P2P, 64 clés synthétiques réparties 32/32,
profil Bellatrix/Paris, slots de six secondes, epochs de huit slots, un seul Mac.
Le nouveau profil `tests/assertoor/finality.json` exige une époque finalisée
au moins égale à 2 et au plus quatre epochs non finalisées. La sonde prévoit
ensuite une comparaison indépendante des checkpoints REST des deux nœuds et
de leurs blocs EVM finalisés. Aucun seuil n'a été abaissé après échec.

## Résultat constaté

**ÉCHEC après 265,84 s**. Trois tâches réussissent (santé EL/CL, synchronisation
avec progression, deux propositions) ; `check_consensus_finality` expire au
bout de 240 s. Sa valeur finalisée reste 0 jusque dans l'époque 5.

Les deux reçus d'arrêt natifs confirment : époque justifiée 2, finalisée 0,
clock et boucles de validation déclarées saines. Les validateurs ont admis
respectivement 24/23 propositions et 142/138 attestations locales. Ces compteurs
ne prouvent pas l'inclusion ni la propagation de toutes les attestations.
La comparaison finale REST/EVM et le redémarrage ultérieur ne sont **pas atteints**.

Deux erreurs Beacon 503 apparaissent dans les logs Assertoor à 17:34:03 et
17:34:42 CEST lors du traitement d'événements de blocs. Des lectures directes
pendant le test observent des têtes aux slots 42 et 44, avec la même époque
justifiée 2 et finalisée 0 ; elles sont successives, pas atomiquement simultanées.
Le topic avancé `execution_payload_available` est refusé explicitement (400),
comme prévu pour le profil Bellatrix ; son flux est distinct du flux de base.

## Diagnostic et priorité

La cause n'est pas établie. Les 503 tardifs ne suffisent pas à prouver qu'ils
causent la perte de finalité. Le faible volume des sorties natives avant l'arrêt
ne confirme pas l'hypothèse d'un blocage par pipe plein.

Le code actuel partage deux workers/huit réservations entre lectures Beacon et
RPC internes de consensus. Les lectures attendent le verrou du réacteur et
revalident des engagements/signatures ; leur deadline de calcul inclut cette
attente. Examiner la contention et la propagation/inclusion des votes, puis
comparer au témoin natif sans Assertoor dans les mêmes conditions.

Cet échec devient prioritaire avant d'étendre la campagne aux partitions ou de
revendiquer une disponibilité de production sous supervision. Il ne supprime
pas les preuves antérieures de finalité native, mais interdit d'en déduire que
la présente combinaison réseau + observation conserve la finalité.

## Preuves et reproduction

- `failed-test.log` : CTest et reçus d'arrêt natifs.
- `config.json`, `runs.json`, `run-1.json` : profil et résultats Assertoor.
- `assertoor.log` : événements, finalité attendue, refus et timeouts.

```sh
cmake --build build-cmake --target nano nano-bellatrix-transition-tests -j4
ctest --test-dir build-cmake -R '^consensus_bellatrix_assertoor_finality$' --output-on-failure
```

La configuration CMake doit fournir `NANO_NETWORK_EXECUTABLE` et
`NANO_ASSERTOOR_EXECUTABLE`. La campagne est terminée ; ses processus de test
ont été arrêtés. Aucune modification du VPS, aucun fonds réel, aucun flood.
