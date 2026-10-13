# Compaction d'une copie de qualification arrêtée

`compact-owned-copy.cpp` est un helper de mesure hors ligne, pas une commande de maintenance du nœud public. Il ouvre exclusivement un répertoire nommé `chain` dont le parent contient le témoin `.nano-owned-compaction-copy` avec la ligne `owned-offline-qualification-copy-v1`. Ce témoin est un garde-fou d'usage, pas une autorisation cryptographique. Ne jamais le placer dans les données d'un nœud réel.

Le contrôleur doit copier une fixture arrêtée dans un répertoire neuf, monter la source en lecture seule, conserver ses hachages avant/après et n'autoriser les écritures que sur la copie. Le helper refuse les liens symboliques et les schémas inconnus. RocksDB garde son verrou exclusif ; WAL actif, checksums vérifiés, SST format 6. Un flush puis une compaction complète doivent conserver exactement le SHA-256 des clés/valeurs ordonnées, avec longueurs encadrées. Aucune clé ou valeur n'est affichée.

Les tailles apparentes, allocations de blocs disque, SST et WAL sont séparées. Un gain sur cette petite fixture n'est ni une estimation de croissance de la chaîne ni une qualification de compaction en ligne. Il faut réauditer les blocs, engagements et journaux de la copie après la mesure ; une identité logique seule ne démontre pas la finalité. Sources de preuve et volumes d'origine doivent rester inchangés.

## Migration expérimentale sur copie

`migrate-pruned-owned-copy.cpp` ne migre que `nano/1` déjà pruné vers `nano/2-state-pruned`. Il exige le témoin distinct `.nano-owned-schema-migration-copy` contenant `owned-offline-schema-migration-v1`, et l'engagement exact `--audit-request` conservé indépendamment. **L'audit canonique avec l'ancien lecteur doit passer avant l'appel, celui du nouveau lecteur doit passer après.** Le helper ne remplace pas ces audits de consensus. Il contrôle le token/payload, le marqueur et la liaison d'exécution, revalide après verrou exclusif et ne change que `meta/schema` dans une écriture synchrone. Le digest de toutes les autres clés/valeurs reste identique ; le rejeu sur schéma déjà migré est sans écriture.

Les arrêts `NANO_TEST_MIGRATION_STOP=before-write|after-write` servent uniquement au contrôleur de fixtures : ils émettent un témoin puis SIGSTOP pour permettre SIGKILL au point connu. Ils ne constituent pas une panne électrique. Ne jamais employer ces modes sur un nœud réel. Une source immuable, des témoins/journaux séparés et des audits avant/après sont indispensables ; migration automatique ou en place d'un réseau public non qualifiée.

## Portabilité et témoins

Les ouvertures détectent l’API RocksDB effectivement installée (`DB**` ou `unique_ptr`) pour les environnements Mac et Linux. Les témoins doivent être des fichiers réguliers, non symboliques, au plus 96 octets ; leur contenu doit correspondre exactement au texte prescrit, avec un saut de ligne final facultatif. Utiliser un chemin canonique : sur macOS `/tmp` est un alias symbolique vers `/private/tmp`, refusé par le garde-fou. Ces protections restent des garde-fous d’usage sur copie, pas une preuve cryptographique de propriété ni une autorisation de maintenance en place.


## Candidate GC Capella sur copies arrêtées — non qualifiée

Le mode proposé `nano-storage-maintenance --checkpoint-blob-gc-owned-copies REQUEST.json REQUEST-SHA STOP-RECEIPT.json STOP-SHA PROFILE.json PROFILE-SHA RECEIPT.json` conserve le mode de compaction précédent. Il exige des SHA-256 externes exacts, des sources arrêtées et des copies physiques privées distinctes, deux engagements principaux/témoins séparés, et le next du témoin. Les budgets sont des quantités hex canoniques positives dans la requête ; ils ne limitent pas le nombre de validateurs.

La closure est issue du décodeur durable Capella complet, sans Reactor ni signer. Seuls des `checkpoint-blob/v1/` non référencés par les racines validées peuvent être supprimés. Un état pending protège l’union previous/next et ne déclenche aucune réparation. Chaque Delete est synchrone, WAL conservé ; une coupure entre bases ne prétend jamais à une transaction cross-DB. Tout doute de format, paire, hash, longueur, rôle, provenance, limite ou mutation refuse l’opération. Bellatrix et les futurs formats restent refusés dans cette candidate Capella.

Après fermeture/réouverture froide, les refs vivantes, tokens, snapshots et namespaces non-GC doivent être exacts. Les journaux de signataires et leurs témoins sont toujours exclus. Les deux sources originales sont hachées avant/après sans ouvrir leurs DB ; la collecte ne doit toucher que les copies. Un plan n’est pas une capability JSON libre : il est construit et recontrôlé dans la session qui possède les deux handles exclusifs.

Le reçu final est publié hors des quatre arbres via fichier exclusif0600, fsync du contenu, publication par lien atomique sans remplacement, suppression du lien temporaire et fsync du parent. Il requiert aussi un terminal à0 ; un reçu laissé lors d’un refus de fsync ne qualifie pas la copie. L’autorité des reçus d’arrêt/copie épinglés relève de l’opérateur : elle ne prouve pas qu’un acteur privilégié ne peut mentir ou accéder au filesystem. Le marker seul n’est jamais suffisant.

Cette copie de sources ajoute un contrôle staged séparé et un scénario naturel de publications cold puis pruning du graphe. Le scénario naturel n’injecte aucun digest et doit refuser si aucun vrai orphelin n’est produit. Aucun de ces cas n’est encore exécuté ; les 11 cas du design initial restent design-only. Ce lot n’élague ni les blobs EIP-4844, ni les archives signées, ni l’ensemble de l’état EVM et n’active aucun fork ou background GC.

La seconde proposition ouvre chaque DB avec `ReadWriteExisting` : création de répertoire/base et initialisation de schéma restent interdites, sans transition RO puis RW créatrice. Le CLI transmet une deadline totale par temps écoulé aux inventaires, plans et collecte ; ses statistiques physiques bornées sont calculées après la validation des copies. Une limite de durée ne rend pas une E/S noyau bloquée interruptible ; le contrôleur doit garder un timeout externe.

Les tests adverses écrits ciblent les closures absentes/corrompues, pins/pending, formats/rôles, liens, plan remplacé, limites/annulation, namespace hors-GC, refus de création, erreurs de fsync et un SIGKILL entre les deux bases. Le processus de crash est exécuté à neuf avant toute ouverture RocksDB ; le parent ne poursuit qu'après preuve de terminaison. Aucune de ces nouvelles frontières n'a été exécutée dans cette proposition. Le scénario naturel reste distinct et n'injecte aucun orphelin.
