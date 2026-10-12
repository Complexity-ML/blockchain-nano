# Audit physique des journaux d’amorçage — trois propriétaires vérifiés

Les 65 vraies paires primaire/témoin de la campagne isolée native `cb532…` ont été copiées par le contrôleur puis vérifiées sur leurs destinations : 16 sur Mac natif, 16 dans Docker ARM64 du Mac et 33 dans Docker AMD64 du VPS. Les trois contrôles ont conclu PASS, avec 130 réouvertures à froid au total, les namespaces/clés/floors exacts et tous les fichiers, octets et inodes inchangés pendant lecture. Le reçu public `three-owner-scope-receipt.json` (SHA `fbfb180c584da00c67886041e9c68379a0b6cc074726a28849ed5be3101d1dbc`) lie ces résultats aux preuves conservées séparément. La préparation initiale est conservée verbatim dans `README.pre-three-owner-results.md` ; ses statements « pas encore exécuté » décrivent uniquement cet ancien stade.

| Propriétaire | Paires | Réouvertures à froid | Preuve terminale | SHA binaire |
| --- | ---: | ---: | --- | --- |
| Mac natif | 16 | 32 | `../isolated-three-node-cb532-20261012/mac-native-physical-audit.json` | `f81c0eaf76f6dc0d7f0d75a9a71d435c8e8058ed8e9e8aebe149ff832be62002` |
| Docker Mac ARM64 | 16 | 32 | `arm64-audit/audit.log`, `arm64-audit/run-receipt.json` | `82d86bb632a2fb8ec883c5c23f94d8f044a5e411d2553fe71e0cf4d1e56e108e` |
| Docker VPS AMD64 | 33 | 66 | `amd64-audit/audit.log`, `amd64-audit/run-receipt.json` | `51952ffc9ab388cdb86ec6420e8585d2441c08a752793b0e5b210edaac79b862` |

Le helper de test `tests/capella/isolated_bootstrap_audit.cpp`, source SHA `a3e77dd3b57c2bb8e5b72939fcbe3b623043869230e92daa5fd482acd45ad1cd`, a été compilé contre les trois archives core85 existantes. Les sources, commandes, logs, résultats et binaires ciblés sont conservés sous `sources/`, `native-build/`, `arm64-build/` et `amd64-build/`. Aucun rebuild de core ni d’image ; les binaires parents épinglés restent identiques. Les sources du helper sont restées gelées durant les audits.

## Interface et ancrages

`isolated-bootstrap-audit --inventory <source-bootstrap-journals-root>` produit l’inventaire complet sans ouvrir RocksDB : fichiers et directories, dev/inode, UID/GID, mode, taille, mtime/ctime et SHA-256 des fichiers. Les symlinks, fichiers spéciaux et capsules seed/env sont refusés avant lecture. Atime est exclu car une lecture peut le modifier. Les fichiers sont bornés à 512 MiB, la liste à 10 000 entrées.

Source canonique : manifest `ec150c28c8c187e6728cea7ef717acc6383e0b75242302b0a481fc2a9c425fc6`, campagne `cb532955c358607d94e4e1074e26be4f4a235dd61b1a00ac2117129973a385ed`, 65 paires sous `isolated-fixture-exporter/private-binding-audit/bootstrap-journals`. Les inventaires complets privés avant/après les transferts sont identiques verbatim, SHA `bc2dd8b8ea94de6dd80373f4ac0498918c001d830c7fceaf04baf85c1ea08fc5` : 1 432 entrées, 1 302 fichiers, 130 directories, 11 560 465 octets réguliers. Ils restent mode0600, exclus de la publication. `inventory-receipt.json` lie la première capture au helper ; le reçu des trois propriétaires conserve les digests avant/après.

La vérification physique utilise :

```text
isolated-bootstrap-audit <manifest.json> <plan.json> <anchors.json> <source-inventory.json> <owner> <dataRoot> <witnessRoot>
```

`anchors.json` contient exactement `format: nano-isolated-bootstrap-physical-audit-anchors-v1`, `fixtureManifestSha256`, `planDocumentSha256`, `planSha256`, `sourceInventorySha256`. Le caller épingle son SHA séparément : `766d2bbe7eb48fbfa88f38b503dd8404eb3bc2dc23637bf8c5c15d3dad832af7`. Le document de plan SHA `0d234e822f27f9152527ea627a7f6782db438e96376063a3474e03f79d8a2d51` porte le modèle SHA `4e1e6f5763fd0258ee05857abc1ef233b4ed8ccff1e6923c1b21b8040dbb104f`. Aucun default déduit des disques ne remplace ces ancrages externes.

Owner : `mac-native` (0–15), `mac-docker` (16–31), `vps-docker` (32–64). Les racines primaire/témoin sont distinctes et owner0700, terminées par `<campaignNamespace>/<owner>`, avec `journal-i`/`witness-i`. La correspondance UID501/1000 est contrôlée sans modifier le contenu. Le dépôt64 reste configuré avec l’index runtime null ; le proposant63 conserve sa proposition présignée du slot1 et son exact signing root.

## Portée acquise et limites

Tous les noms, modes, tailles et SHA copiés correspondent aux projections de l’inventaire source. Les snapshots primaires et intents témoins sont recalculés depuis la racine bootstrap liée au manifest, sans import ou reconstruction des bases. Les deux engagements restent séparés. Raw RocksDB est ouvert uniquement en lecture seule pour `meta/identity` ; l’API core `fork_checkpoint(expected)` ouvre ensuite les copies à froid deux fois et vérifie les payloads et tokens exacts. Toute mutation de LOG/metadata aurait fait échouer l’inventaire final : aucune exclusion n’a été ajoutée pour obtenir PASS.

Ces succès qualifient la lecture physique de ces 65 nouveaux journaux/témoins, notamment le passage des fichiers natifs RocksDB11.8 aux lecteurs Linux8.9. Ils ne qualifient pas toute la base de chaîne. L’absence de writer est contrôlée par le parent et corroborée par l’immutabilité observée ; elle ne constitue pas une prévention universelle d’un writer privilégié concurrent. Le helper ne lit aucune clé/env, ne signe pas, ne bind/import/commit/retire/repair pas et ne lance pas de runtime.

La revue indépendante statique du helper séparé de capture arrêtée est conservée sous `stopped-capture-independent-review/` : source SHA `718761a654576f93bb0bbbf258f52015e1d10ba0544672556a189568ab3e1f55`, reçu SHA `6601d964f25c8eb146f93f1b36413d5653c85b6cb8f95a3cf4f0a34127f886e9`, aucun blocage statique dans cette portée. Elle ne lance aucune capture. Les sources doivent être arrêtées et la provenance des reçus de stop conservée extérieurement.

L’échec de démarrage `81954` (HTTP404 avant duties, rapporté par le parent) demeure un échec distinct. Aucun de ces audits ne prouve une reprise de duties, une finalité commune qui avance, une restauration V37 ou la production prête.

## Publication bornée

Publier uniquement les sources/helpers/scripts sans secrets, ce README et sa version historique, les commandes/logs/receipts/metadata de build et d’audit, le reçu des trois propriétaires et le dossier de revue statique. Ne pas publier les inventaires `.private.json`, bases RocksDB, journaux physiques, env, seeds, capsules ou clés. Les résultats de build indiquant « helper pas encore exécuté » restent leurs reçus historiques immuables ; les reçus d’exécution distincts constatent les audits ultérieurs.
