# Assertoor : base de résultats multi-connexion — 27 septembre 2026

## Échec observé

`consensus_bellatrix_assertoor_finality` échoue après 296,02 s dans la campagne
sur `af7f99e`. Les deux nœuds terminent avec la même finalité d’époque 4 et
justification 5. Le test Assertoor dépasse son délai de 280 s. Son journal
signale `SQL logic error: no such table: task_logs (1)` avant le timeout.
Les traces originales sont conservées, sans requalifier ce test en succès.

## Reproduction déterministe

Le checkout Assertoor est `89366be6cb211522249a6d9424adb47631864432`.
`pkg/assertoor/coordinator.go` utilise par défaut `:memory:?cache=shared` ;
`pkg/db/common.go` lui ajoute `?_pragma=journal_mode(WAL)` et ouvre un pool
pouvant atteindre 50 connexions. Le programme conservé emploie son pilote
exact `github.com/glebarez/go-sqlite v1.22.0` et deux connexions détenues
simultanément : création de `task_logs` sur la première, insertion sur la seconde.

- DSN mémoire par défaut : même erreur `no such table: task_logs` reproduite.
- Fichier SQLite temporaire propre au run : insertion réussie.

Commande depuis la racine du dépôt :

```sh
go -C .deps/assertoor run /chemin/absolu/connection-pool-reproducer.go
```

## Correction et limites

Le probe configure désormais `database.engine=sqlite` avec un fichier
`assertoor.sqlite` dans son répertoire temporaire unique. Cette base stocke
les résultats Assertoor ; le stockage RocksDB de Nano ne change pas.
Aucun seuil, timeout ni contrôle de finalité n’est abaissé. `node --check`
passe. Le rejeu Assertoor réel reste à effectuer après la campagne active.
Le reproducer explique l’erreur SQL ; il ne suffit pas à clôturer le test
réseau ni à garantir l’absence d’autres causes de timeout.

La commande `go version -m .deps/assertoor-bin` confirme que le binaire
utilisé a été compilé depuis ce même commit, sans modifications, et embarque
`github.com/glebarez/go-sqlite v1.22.0`. Son résultat complet est conservé dans
`assertoor-build-info.txt`.

## Rejeu réel corrigé : réussi

`consensus_bellatrix_assertoor_finality` passe en **198,32 s**, code 0, avec
le délai CTest d’origine de 420 s et les délais internes inchangés. Les quatre
tâches Assertoor sont démarrées, terminées et réussies : santé des clients,
progression de synchronisation, propositions de blocs et finalité.

La vérification indépendante des deux nœuds observe la même finalité d’époque
2, racine `0x89599b052fa1ab5f2faff8bea4ec3c4be49993a6067b7999e0e08bb776f67e0d`,
et le même bloc d’exécution finalisé
`0x6200bba050a5f22b670db380b7a9d708b962b982d5b725d76b927446d52227e6`.
La correspondance Beacon/exécution est identique sur les deux endpoints.
Le journal corrigé ne contient pas d’erreur `no such table`.

`corrected/` conserve configuration, détails des quatre tâches, observations,
journaux et manifeste de lancement. Nano et nano-p2p sont inchangés. Il s’agit
d’un test local à deux nœuds ; aucune qualification WAN ou production globale.
Les abonnements à des topics Beacon non pris en charge ne sont pas qualifiés
par ces quatre contrôles. L’échec initial est conservé séparément.
