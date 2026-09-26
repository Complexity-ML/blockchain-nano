# Dimensionnement de la graduation V3

Outil Python hors ligne, sans dépendance Python externe, sans fonds ni RPC.
TickMath traduit la version Uniswap v3-core 1.0.1 sous GPL-2.0-or-later.
La liquidité suit l'arrondi de LiquidityAmounts (periphery 1.4.4), et les
montants de mint sont arrondis vers le haut. Les entiers sont conservés sans float.

```sh
python3 scripts/simulation/graduation_v3.py --usdc 10000 --price 0.01 \
  --quote-index 1 --tick-lower=-330000 --tick-upper=-315000 --tick-spacing 60
python3 -m unittest discover -s scripts/simulation -p 'test_*.py'
```

Exemple synthétique : allocation C = 973546.151243217295796975 NANO.
Ces paramètres ne sont pas une décision de lancement. L'ordre token0/token1
vient des adresses ; USDC a six décimales, NANO dix-huit par défaut. Le spacing
vient de la factory/pool. Les bornes sont des ticks natifs, pas des prix humains.
Pour une pool existante, fournir --sqrt-price-x96 issu de slot0 à la place de
--price. Ne pas remplacer son prix par le prix souhaité de la vente.

Le prix doit être strictement à l'intérieur des bornes : les positions à un seul
actif sont refusées pour cette simulation de graduation. Ajouter de la liquidité
ne change pas le tick ; aucun swap, franchissement de ticks, frais gagnés, MEV ou
mouvement de marché n'est simulé ici. Le résultat couvre une position nouvelle,
pas les limites par tick ou les positions préexistantes de la pool. Une transaction
réelle requiert encore des bornes de slippage, un délai et un état RPC actualisé.

Validation : vecteurs extrêmes TickMath, calcul rationnel indépendant pour six
ticks, budgets et arrondis dans les deux ordres d'actifs, refus des paramètres
invalides. La comparaison à un mint effectif sur Nano Paris passe dans les deux ordres
des actifs (quote token0 et token1), montants et liquidité exactement égaux.
Preuves : docs/qualification/graduation-v3-20260926. Cela ne qualifie ni le
contrat de graduation ni le bridge.
