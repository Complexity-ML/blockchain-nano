# Compaction d'une copie de qualification arrêtée

`compact-owned-copy.cpp` est un helper de mesure hors ligne, pas une commande de maintenance du nœud public. Il ouvre exclusivement un répertoire nommé `chain` dont le parent contient le témoin `.nano-owned-compaction-copy` avec la ligne `owned-offline-qualification-copy-v1`. Ce témoin est un garde-fou d'usage, pas une autorisation cryptographique. Ne jamais le placer dans les données d'un nœud réel.

Le contrôleur doit copier une fixture arrêtée dans un répertoire neuf, monter la source en lecture seule, conserver ses hachages avant/après et n'autoriser les écritures que sur la copie. Le helper refuse les liens symboliques et les schémas inconnus. RocksDB garde son verrou exclusif ; WAL actif, checksums vérifiés, SST format 6. Un flush puis une compaction complète doivent conserver exactement le SHA-256 des clés/valeurs ordonnées, avec longueurs encadrées. Aucune clé ou valeur n'est affichée.

Les tailles apparentes, allocations de blocs disque, SST et WAL sont séparées. Un gain sur cette petite fixture n'est ni une estimation de croissance de la chaîne ni une qualification de compaction en ligne. Il faut réauditer les blocs, engagements et journaux de la copie après la mesure ; une identité logique seule ne démontre pas la finalité. Sources de preuve et volumes d'origine doivent rester inchangés.

## Migration expérimentale sur copie

`migrate-pruned-owned-copy.cpp` ne migre que `nano/1` déjà pruné vers `nano/2-state-pruned`. Il exige le témoin distinct `.nano-owned-schema-migration-copy` contenant `owned-offline-schema-migration-v1`, et l'engagement exact `--audit-request` conservé indépendamment. **L'audit canonique avec l'ancien lecteur doit passer avant l'appel, celui du nouveau lecteur doit passer après.** Le helper ne remplace pas ces audits de consensus. Il contrôle le token/payload, le marqueur et la liaison d'exécution, revalide après verrou exclusif et ne change que `meta/schema` dans une écriture synchrone. Le digest de toutes les autres clés/valeurs reste identique ; le rejeu sur schéma déjà migré est sans écriture.

Les arrêts `NANO_TEST_MIGRATION_STOP=before-write|after-write` servent uniquement au contrôleur de fixtures : ils émettent un témoin puis SIGSTOP pour permettre SIGKILL au point connu. Ils ne constituent pas une panne électrique. Ne jamais employer ces modes sur un nœud réel. Une source immuable, des témoins/journaux séparés et des audits avant/après sont indispensables ; migration automatique ou en place d'un réseau public non qualifiée.
