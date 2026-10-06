# Relecture historique avec le core cohérent

Core e33ad89e, outil de capture e4ce39eb. Les trois relectures originales sont terminales 0 : Mac natif et Docker Mac (pilote local 41468), Docker VPS (3355). Les bases V28 restent arrêtées et en lecture seule ; aucun démarrage de duty, migration ou nouvelle finalité.

Les captures vérifient les checkpoints principaux/témoins, tous les journaux indépendants (32/32/33), les bornes de séquence et le rejeu canonique. Les pilotes comparent exactement restart, validatorKeys et canonicalReplay aux captures après arrêt V28, puis les hashes de tous les fichiers avant/après (1728/1759/2716). Demandes, configurations et listes de hashes privées restent sur leur machine ; seuls capsules et reçus publics sont archivés.

Cette preuve retire l'incertitude sur la lecture historique par le core actuel. La restauration active, la migration froide de ces données et la finalité commune WAN restent ouvertes. Les anciens échecs V28 sont conservés. Le budget explicite utilisé reste celui des sources (1024 records,128 MiB).
