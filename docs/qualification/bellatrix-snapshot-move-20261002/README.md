# Snapshot : transfert des tableaux temporaires — échec terminal conservé

Core c10426b2, même scénario original 112 slots et timeout 240 secondes. La variante précédente d’assemblage canonique est conservée ; six tableaux JSON locaux sont transférés vers l’objet final au lieu d’être copiés. Le store authentifié reste const, contrôles de validation/cancel/limites inchangés.

ARM64 : 512 comparaisons byte à byte passent sur l’expression finale extraite, valeurs synthétiques et helpers de contexte simulés. Portée : construction JSON uniquement, pas authentification SSZ, reprise complète ou production.

AMD64 : timeout 240,025 s, dernier slot observé 96, zéro skip, sorties CTest/conteneur 8. Au compteur de 1 792 publications : encodage 42,678 s, publication inclusive 107,889 s ; variante précédente 44,416 / 109,663 s. Petit écart descriptif non répété, aucun nouveau slot terminal atteint. Ne pas qualifier ce changement comme résolution du délai.

Sources/programmes/bibliothèques originales identiques avant/après, post-manifest à zéro. Aucun runtime principal modifié ou promu. Les échecs historiques et critères de production restent ouverts.

Diagnostic suivant, nécessaire pour identifier le coût restant : séparer témoin durable, commit couplé, authentification du checkpoint stocké, et appel Write synchrone de RocksDB. Aucune protection retirée ni timeout augmenté.
