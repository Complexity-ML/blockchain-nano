# Schémas stricts et reçus

`REQUEST` est du JSON compact canonique (`Json::dump()`), sans newline final, SHA256 externe lowercase64 sans 0x, maximum 65536 octets ; profondeur maximum32 et aucune clé dupliquée. Les objets listés sont exacts (pas de champs inconnus). Les artefacts sont bornés et leur taille est vérifiée exactement.

Request `nano-owned-bootstrap-archive-maintenance-v1` :

```
{format, genesis, profile, authority, stopReceipt, primary, witness, limits, budget}
genesis/profile/stopReceipt = {path: absoluteCanonicalString, sha256: lower64, bytes: positiveInteger}
authority = {format: "nano-capella-signed-bootstrap-proof-inputs-v1", genesisState: artifact, signedBlock: artifact}
primary/witness = {path, sourcePath, inventorySha256: lower64, sourceInventorySha256: lower64, token}
token = {sequence: positiveCanonicalHexQuantity, digest: "0x" + lower64}
limits = [envelopeBytes, blocks, messages, equivocations, stateBytes, totalStateBytes, replaySlots, syncTrials]
budget = {inventoryFiles, inventoryBytes, logicalRecords, logicalBytes, durationMs}
```

Les huit limits et cinq budgets sont des quantités hexadécimales positives canoniques. L’ordre des limites est celui de `CapellaStoreSnapshotLimits` ; durationMs≤INT64_MAX est comparé au temps écoulé monotone, sans addition susceptible de débordement. Le budget s’applique aux scans/lectures via checkpoints ; les appels système fsync/LOCK nécessitent aussi la limite extérieure finie du runner opérateur. Bounds artefacts : Genesis16MiB, profile1MiB, stop65536 ; l’autorité suit les limites profil/SSZ/block et l’API générale. Ces limites d’administration ne changent aucune règle de consensus.

StopReceipt `nano-stopped-checkpoint-copy-receipt-v1` :

```
{format, scope:"isolated-owned-copies", allWritersStopped:true, allReadersStopped:true,
 stoppedAtMs: positiveCanonicalHexQuantity,
 copies: [{role:"primary", path, inventorySha256, sourcePath, sourceInventorySha256},
          {role:"witness", path, inventorySha256, sourcePath, sourceInventorySha256}]}
```

Les deux valeurs stopped sont strictement booléennes et leur champ est authentifié par la SHA externe du reçu. stoppedAtMs n’autorise aucun reset/clockcatchup ; l’outil n’évalue pas le temps UTC d’une campagne.

Deux fichiers retenus `nano-bootstrap-archive-retained-next-v1` :

```
{format, role:"primary"|"witness", requestSha256,
 currentPrimary:token, currentWitness:token, next:token, hasWork:boolean,
 copies:[{path,device:decimalString,inode:decimalString}, {...}],
 invariants:[primaryUnrelatedSha256,witnessUnrelatedSha256],
 preparedInventories:[primaryClosedInventorySha256,witnessClosedInventorySha256]}
```

Les deux fichiers ont des chemins distincts et des SHA externes distinctement fournies. Ils sont identiques en dehors de role/next ; current pins sont exactement celles du request. Les invariants sont vérifiés sur les copies actuelles. Apply exige également leurs inventaires physiques fermés. Recover exige hasWork et utilise ces anciens invariants sans imposer l’ancien inventaire physique après interruption. Il n’accepte jamais une autorité/format/prediction absent comme indication d’une reprise générique.

`operation-intent.json` est privé et contient format=`nano-bootstrap-maintenance-operation-intent-v1`, operation, requestSha256, currentPrimary/currentWitness. `complete.json` est privé, écrit en dernier, format=`nano-bootstrap-archive-maintenance-completed-v1` et contient operation/requestSha256/hasWork/primary/witness/copies, allDatabaseHandlesClosedBeforeReceipt, originalSourcesUnchanged, rawSnapshotAndUnrelatedNamespacesPreserved, operatorHostOnly, newSignatures=0, runtimeStarted=false, productionQualified=false. Le reçu ne permet pas de sauter les pins/close/readback et ne démontre pas une sauvegarde de fichiers extérieurs.

Stdout succès : completed/operation/hasWork/stores=2/newSignatures=0/runtimeStarted=false/productionQualified=false. Stdout refus : completed=false/refusedStage/rawErrorsSuppressed=true/runtimeStarted=false/productionQualified=false. Pas de token, exception brute ni digest d’état dans stdout. Un stdout false après un succès durable PRIMARY peut nécessiter la reprise explicite avec les deux fichiers NEXT déjà synchronisés.
