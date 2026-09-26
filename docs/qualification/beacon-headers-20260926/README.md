# En-têtes Beacon signés et suivi des branches

Code local non commité basé sur `30b0de21a1225ba55603de437c1200f3437a1ecb`.
Compilation CMake du binaire Nano et du runner réussie sur macOS ARM64.

`GET /eth/v1/beacon/headers/{block_id}` est raccordé au snapshot publié du runtime.
Sélecteurs : head, finalized, racine 0x de 32 octets, slot décimal. Une recherche
par slot suit la branche canonique retenue. Un bloc absent ou sans signature
archivée renvoie 404, un identifiant invalide 400. Genesis renvoie 404 : le runtime
actuel ne conserve pas de bloc genesis signé. L'ancre importée n'est pas signée
artificiellement. Le corps SSZ et la signature BLS du proposant sont revérifiés
par le même validateur d'archives que la restauration durable.

Preuves :

- `consensus_bellatrix_beacon_reads` : HTTP réel, dont 404 sur l'ancre sans signature,
  passé en 1,01 s ; les protections Origin/bind/méthode restent testées.
- `consensus_bellatrix_beacon_headers` : passé en 0,59 s ; archive créée par admission
  d'un vrai bloc signé dans le réacteur natif. Égalité des réponses head/root/slot,
  signature exacte, rejet de signature et corps altérés, annulation, absence
  d'archive, bloc futur, racine inconnue et identifiants invalides.
- `consensus_bellatrix_beacon_headers_reorg` : passé en 5,85 s ; deux branches EVM,
  votes BLS valides, deux changements de tête. Le champ canonical et la recherche
  par slot suivent chaque changement ; le snapshot gelé conserve sa réponse.

Les cas positifs d'en-têtes sont testés au niveau du handler relié aux archives
réelles du réacteur ; le test HTTP de cette tranche porte sur l'ancre sans archive.
Ce n'est pas un lancement Assertoor ni une qualification d'un réseau distribué.
Pas de test de charge ni qualification exhaustive de tous les états de finalité.

```sh
cmake --build build-cmake --target nano nano-bellatrix-transition-tests -j 4
ctest --test-dir build-cmake -R '^consensus_bellatrix_beacon_(reads|headers|headers_reorg)$' --output-on-failure
```

Source de format consultée le 2026-09-26 :
https://github.com/ethereum/beacon-APIs/blob/master/apis/beacon/blocks/header.yaml
