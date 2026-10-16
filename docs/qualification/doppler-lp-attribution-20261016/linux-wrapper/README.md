# Projections Linux neuves pour les unités LP V2

Source autonome uniquement ; aucun Node, test, Docker, RPC, SDK ou projet exécuté par l'auteur. R reste inchangé. Le wrapper commence par des imports standards et peut être fourni en UTF-8 exact à `node --input-type=module -e CODE`, avec le document public sur stdin. Root garde les contrôles du conteneur, du Node réel, de ses anciennes lignes et des dépendances déjà installées.

Les deux essais précédents restent refusés : la causalité baseline a été observée mais son collecteur a refusé ; le candidat a observé 29 tests réussis mais quatre nouvelles copies Mac ont changé de ctime. Aucune admission de ces essais n'est rafraîchie ou transformée en qualification complète.

## Entrée fermée

Un document JSON UTF-8 d'au plus 1 MiB, au format suivant, sans autre champ :

```json
{
  "format": "nano-doppler-lp-linux-unit-input-v1",
  "phases": {
    "baseline": {"files": [{"path": "CHEMIN_FIXE", "sha256": "64hex", "bytes": 1, "contentBase64": "BASE64_CANONIQUE"}]},
    "candidate": {"files": [{"path": "CHEMIN_FIXE", "sha256": "64hex", "bytes": 1, "contentBase64": "BASE64_CANONIQUE"}]}
  }
}
```

Chaque liste contient exactement ces quatre chemins, sans doublon :

1. `tools/doppler-launch/package.json` ;
2. `tools/doppler-launch/verify-migration.mjs` ;
3. `tools/doppler-launch/migration.test.mjs` ;
4. `docs/qualification/doppler-adversarial-lifecycle-20261004/attempt-b.json`.

Chaque feuille est positive et bornée à 256 KiB ; les huit feuilles décodées totalisent au plus 512 KiB. SHA256 et longueur des bytes décodés sont vérifiés avant toute projection. Les trois fichiers autres que le vérificateur doivent être byte-identiques entre phases ; les deux vérificateurs doivent différer. Root fournit les bytes du candidat gelé MF `3f9dbf2e5e076841eb011867dc57c01ef665255423ff80d7dc9829e9a8baf1ba` et du baseline R gardé :

- vérificateur baseline `e77c193c2f55c45f74644a6366ee9bd375f776e8131f26539a07ed54ffdf1e98` / 4946 octets ;
- vérificateur candidat `fe362e75e277ef4bb9d7162db866400624dfd4f6658f4ec9e6332e0bd2b0ab13` / 7533 octets ;
- test candidat commun `cf7fbbeb5e1b9d9fb0baaf19a92bd839b206ea19fd41e5f8ab4fc3787036f9ae` / 8401 octets ;
- package public commun `c64df2ff667febf5e3370544f6b47a5645cfe738a3f7a5d46d1a02a585c64ebd` / 341 octets ;
- fixture publique commune `864153d99ed2a254f18e9aa9986c5d47dae94b3cb9daae9b753d06d5d730dce4` / 18085 octets.

Le wrapper vérifie les engagements fournis ; leur liaison à cette proposition reste un contrôle Root. Il ne découvre ni ne réadmet une ancienne table SDK. Aucun appel npm ni installation n'est réalisé.

## Première admission et exécution

Dans un conteneur Linux arm64 ou x64, Root fournit `/deps/node_modules` en lecture seule et le Node déjà gardé. Le wrapper crée un namespace `/tmp/nano-doppler-lp-unit-*` neuf, puis deux arbres `baseline` et `candidate`. Les fichiers sont créés EXCL, chmod 0400, fsync puis fermés ; les répertoires 0700 sont synchronisés. Chaque phase a un lien `node_modules` exact vers `/deps/node_modules`, dont le lien lui-même reste gardé. Aucun montage Mac des nouveaux fichiers JS/JSON n'est requis.

Avant les tests, un lecteur FD O_NOFOLLOW compare fstat/lstat avant et après le SHA256 : `dev, ino, mode, nlink, uid, gid, size, mtimeNs, ctimeNs`. Ces neuf champs proviennent des Stats BigInt et sont publiés en chaînes décimales, jamais via Date ou Number. Feuilles physiques régulières, un seul lien, owner courant et mode 0400 sont obligatoires. Trois observations complètes consécutives identiques doivent couvrir au moins deux secondes, sous une borne de trente secondes et une horloge monotone. Une modification de metadata ne peut se stabiliser que **pendant la première admission de ces nouvelles copies**, avec toujours les mêmes bytes connus. Les premières lignes de la fenêtre stable retenue sont ensuite immuables ; aucune nouvelle première admission après les tests.

Les fences vérifient les huit feuilles, les liens et les répertoires avant/après chaque phase et à la fin. Les SHA et metadata initialement admis doivent rester exacts. La table initiale est gardée dans le JSON ; un écart de snapshot conserve aussi les lignes observées. Un refus pendant une lecture identifie la nouvelle feuille concernée. Les observations du sous-processus sont enregistrées avant le post-fence pour ne pas perdre son TAP si le fence refuse.

Le seul exécutable enfant est `process.execPath`, avec environnement fixe et `spawnSync`, sans shell :

- baseline : `--test --test-reporter=tap --test-name-pattern=^a pre-existing positive LP balance cannot replace a credit in this migration transaction$ tools/doppler-launch/migration.test.mjs` ; exit 1, un test, zéro pass, un fail, zéro skip/cancel/todo et **ERR_ASSERTION / Missing expected rejection.** exigés ; une erreur import/parser/timeout ne satisfait pas l'oracle ;
- candidat : le même fichier entier sans filtre ; exit 0, huit groupes de premier niveau, 29 tests réussis au total, zéro fail/skip/cancel/todo exigés.

Chaque enfant est borné à 15 secondes, réduites au budget global restant, et 128 KiB par flux ; tout timeout/signal/erreur ou divergence du TAP refuse. Le wrapper contrôle une échéance monotone globale de 90 secondes ; stdin a sa propre borne de cinq secondes. Un stdout JSON unique contient les deux TAP bornés, compteurs, premières admissions et fences, sans BigInt brut ; sa borne est 768 KiB. Le premier refus est conservé, et un fence final ne convertit jamais un échec en succès. Les namespaces ne sont ni réutilisés ni supprimés par le wrapper.

Root garde la durée du Docker, l'isolation réseau, 1 CPU / 768 MiB / tmpfs 64 MiB et sa suppression finale : le timeout de `spawnSync` n'est pas une preuve de terminaison de tous les descendants sur un hôte général. Ce wrapper n'est pas une sandbox OS autonome et ne qualifie pas de nouveau les dépendances. Sa seule portée est cette causalité et cette suite unitaire sur de nouvelles projections Linux. Aucun fork Anvil, appel réseau réel, signature, fonds, duties ou aptitude production n'est revendiqué.
