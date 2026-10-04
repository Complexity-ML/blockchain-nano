# Compaction d'une copie de qualification arrêtée

`compact-owned-copy.cpp` est un helper de mesure hors ligne, pas une commande de maintenance du nœud public. Il ouvre exclusivement un répertoire nommé `chain` dont le parent contient le témoin `.nano-owned-compaction-copy` avec la ligne `owned-offline-qualification-copy-v1`. Ce témoin est un garde-fou d'usage, pas une autorisation cryptographique. Ne jamais le placer dans les données d'un nœud réel.

Le contrôleur doit copier une fixture arrêtée dans un répertoire neuf, monter la source en lecture seule, conserver ses hachages avant/après et n'autoriser les écritures que sur la copie. Le helper refuse les liens symboliques et les schémas inconnus. RocksDB garde son verrou exclusif ; WAL actif, checksums vérifiés, SST format 6. Un flush puis une compaction complète doivent conserver exactement le SHA-256 des clés/valeurs ordonnées, avec longueurs encadrées. Aucune clé ou valeur n'est affichée.

Les tailles apparentes, allocations de blocs disque, SST et WAL sont séparées. Un gain sur cette petite fixture n'est ni une estimation de croissance de la chaîne ni une qualification de compaction en ligne. Il faut réauditer les blocs, engagements et journaux de la copie après la mesure ; une identité logique seule ne démontre pas la finalité. Sources de preuve et volumes d'origine doivent rester inchangés.
