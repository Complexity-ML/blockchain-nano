# Frontières de blocs v12 : oracle complet hors ligne

Core f077205f, source du dépôt acef6b7d ; session originale 55137, sortie 0. Sur deux stockages Mac arrêtés et audités, les trois frontières de blocs manquants (slots 10751, 10753 et 10752, parent 10574) sont admises par prepare_capella_received_block avec une borne diagnostique de 256 slots. Les en-têtes et hashes d'exécution résultants correspondent aux blocs de la source. Toutes les empreintes des stockages restent inchangées.

Ce diagnostic rejoue les transitions sur copies en mémoire. Il ne signe pas, ne publie pas, ne modifie pas les bases, ne réinitialise pas les journaux et n'ouvre aucun réseau. Les limites de production n'ont pas changé. Le graphe reste sous 1024 blocs (326/419), mais 324/231 blocs de la source manquent dans le graphe opposé.

Les blocs frontaliers sont valides dans ce périmètre. La cause complète de leur non-admission en direct n'est pas encore prouvée : continuité des préparations lorsque les parents/cibles alternent, budgets, scheduling, délais et livraisons restent à éprouver avec un contre-exemple réel. Ce résultat ne qualifie pas la finalité réseau, le VPS ou la production.
