# Blocs Beacon JSON / SSZ et client de référence — 2026-09-26

Code local non commité basé sur `30b0de21a1225ba55603de437c1200f3437a1ecb`.
Binaire Nano et runner compilés sur macOS ARM64. Test Go externe épinglé dans
`tests/beacon-sdk-probe/go.mod` et `go.sum` : go-eth2-client v0.1.6, la version
utilisée par Assertoor, et dynamic-ssz v1.3.2. Le cœur reste C++.

## Implémentation

`GET /eth/v2/beacon/blocks/{block_id}` partage la sélection native des en-têtes
(head, finalized, racine, slot). La réponse vient d'une archive signée vérifiée
contre l'état, le corps SSZ et la signature BLS. Archives absentes : 404 ; aucune
signature synthétique dans les réponses du nœud.

- JSON : bloc signé, opérations de consensus, SyncAggregate, payload Bellatrix
  et transactions brutes ; nombres décimaux, base fee uint256 convertie depuis
  le little-endian SSZ sans troncature.
- SSZ : octets exacts archivés, application/octet-stream.
- Eth-Consensus-Version: bellatrix ; métadonnées execution optimistic/finalized.
- Accept avec pondérations et exclusions q=0, JSON par défaut. Représentation
  indisponible : 406. Même file bornée, contrôle loopback et Origin qu'auparavant.

## Résultats

1. `consensus_bellatrix_managed_runtime` avec NANO_TEST_BEACON_SDK_PROBE : **passé
   en 14,00 s**, deux processus Nano et deux démons P2P, redémarrage. Le SDK réel
   initialise sa connexion sans désactiver NodeSyncing, lit genesis/spec/en-tête,
   décode un bloc SSZ natif et réencode son JSON en SSZ : octets identiques. Voir
   `sdk-details.log`, qui contient le root et le nombre d'octets observés.
2. `consensus_bellatrix_beacon_headers` : **passé en 0,63 s**, archive native,
   vérification du bloc complet et du payload EVM en plus des contrôles précédents.
3. Huit corps officiels `consensus-spec-tests`, commit
   `bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908` : SHA-256 du manifeste et racine SSZ
   de référence vérifiés, projection JSON puis réencodage par le SDK : **8/8
   identiques**. Opérations couvertes : 50 proposer slashings, 8 attester slashings,
   40 attestations, 46 dépôts, 37 sorties. Enveloppes synthétiques pour ce test de
   sérialisation seulement, jamais soumises au consensus ni servies par le réseau.
4. Test HTTP natif supplémentaire : refus des Accept non supportés, q=0 explicite
   prioritaire sur wildcard, casse et pondération JSON. Voir `http-tests.log`.

Le premier export de fixtures échouait : une boucle parcourait une sous-valeur
JSON issue d'un temporaire détruit. Le manifeste est désormais conservé dans une
variable pendant l'itération. Le même motif a été corrigé dans la vérification
des presets du runner Bellatrix. Le compte attendu de huit cas est obligatoire.

## Reproduction

```sh
cd tests/beacon-sdk-probe
go build -o ../../.deps/beacon-sdk-probe .
cd ../..
cmake --build build-cmake --target nano nano-bellatrix-transition-tests -j 4
NANO_TEST_BEACON_SDK_PROBE="$PWD/.deps/beacon-sdk-probe" ctest --test-dir build-cmake -R '^consensus_bellatrix_managed_runtime$' --output-on-failure
NANO_TEST_BEACON_BLOCK_EXPORT=/tmp/nano-beacon-fresh-export build-cmake/nano-bellatrix-transition-tests --beacon-headers
.deps/beacon-sdk-probe --fixtures /tmp/nano-beacon-fresh-export
```

Le répertoire d'export doit être absent avant le test. Il ne contient que des
fixtures publiques. Aucun nœud de production ni fonds réels sollicités.

Limites : **SDK exécuté, suite Assertoor complète non exécutée**. Pas de flux
événementiel, ni qualification de charge, ni support REST des forks ultérieurs.
Ce test de formats ne remplace ni la transition consensus ni les EEST.

Sources :
https://github.com/ethereum/beacon-APIs/blob/master/apis/beacon/blocks/block.v2.yaml
https://github.com/ethpandaops/go-eth2-client/blob/v0.1.6/http/signedbeaconblock.go
