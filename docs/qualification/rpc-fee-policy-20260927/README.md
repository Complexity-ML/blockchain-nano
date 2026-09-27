# Politique de frais RPC — 27 septembre 2026

Le pourboire fixe de 1 gwei est remplacé par une estimation historique configurable,
commune aux RPC et aux valeurs implicites du portefeuille. Le cache est indexé
par hash de tête ; les transactions du producteur sont exclues des échantillons.
La base fee EIP-1559 et les transactions explicitement signées ne sont pas modifiées.

## Résultat

Quatre tests passent en 8,65 secondes : `rpc` (5,14 s), `rpc_fee_policy` (0,49 s),
`wallet_cli` (1,77 s), `rpc_cli` (1,24 s). `rpc_fee_policy` est une nouvelle
couverture ; les trois autres sont des régressions des parcours affectés.

Le nouveau scénario vérifie les frais effectifs legacy/type 2, le plafond des
transactions, l’exclusion du producteur, les percentiles et budgets, le cache
après remplacement à hauteur identique, la fenêtre vide, les rechargements
invalides, les valeurs explicites du portefeuille et les limites uint256.

## Traçabilité et limites

`manifest.json` identifie le commit de base, les sources modifiées, les binaires
et les commandes exactes. Les logs de compilation, CTest et résultats JUnit
sont conservés. Le correctif était non commité lors de ce run ; les empreintes
sources identifient donc la version testée.

Qualification locale uniquement. Coût à froid sur de très gros blocs, saturation,
concurrence de réorganisation pendant le calcul et indisponibilité des ancêtres
ne sont pas qualifiés par ces quatre tests. Le rejet explicite de ces deux
situations dans le code n’est pas assimilé à une preuve de test. Aucun résultat
WAN, aucun déploiement VPS et aucune clôture globale de production.

Voir [politique et configuration](../../RPC-FEE-POLICY.md).
