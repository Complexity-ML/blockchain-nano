# Mint full range sur Nano

Binaire natif b1ccbac, empreinte jointe ; Paris, Uniswap V3 core 1.0.1 / periphery
1.4.4. Test local éphémère terminé code 0, aucun actif réel.

100 unités quote synthétiques (token0, six décimales), prix initial de la fixture
10 quote/NANO, ticks -887220/+887220 pour spacing 60. Allocation calculée et
consommée : 10.000000000000065358 WNANO ; liquidité 31622776601684.
Égalité exacte simulateur/mint, variations des soldes contrôlées et prix/tick
inchangés. Ce scénario ne mesure pas les swaps importants, la perte de valeur
ou une résistance à la viralité. Une position full range conserve des limites
finies. Quatre tests Python couvrent aussi le calcul de bornes et les refus.

## Achat puis revente du montant reçu

Extension exécutée sur le même profil : achat de 100 unités quote, réception de
4.992488688210107688 NANO, revente de ce montant, retour de 99.549998 quote.
La position reste active. Le prix passe d'environ 10 à 39,88009 puis revient
près de 10,03002 quote/NANO. Les valeurs précises sont dans roundtrip-prices.json.
La perte de quote sur cet aller-retour est vérifiée, sans gain artificiel de NANO.
Le LP et le trader sont ici le même compte ; les frais accumulés dans la position
ne sont pas collectés, donc ce résultat n'est pas son bilan patrimonial global.
Ce scénario confirme une forte variation malgré full range ; ce n'est pas un
benchmark de débit ni un test de bridge. Le mint et les swaps sont distingués.
