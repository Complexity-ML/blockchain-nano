# Cycle Doppler et trois refus adversariaux — 4 octobre 2026

Tentative B : **exit 0**, 17 étapes. Fork privé Anvil Base bloc 52173454,
chain ID 31337, loopback 19546, module SDK et numéraire USDC du cycle précédent,
ETH et USDC fictifs. Anvil arrêté après le résultat. Aucun RPC public en écriture,
aucun lancement réel, aucune modification du runtime C++ en qualification.

Les transactions suivantes sont réellement minées puis annulées :

- Migration prématurée : `CannotMigrate()`, sélecteur `0xa3454aa0`.
- Transfert de propriété par le compte non propriétaire :
  `OwnableUnauthorizedAccount(address)`, sélecteur `0x118cdaa7`, lié à l'attaquant.
- Seconde migration après graduation : même erreur de propriété, liée à Airlock.

Chaque essai conserve aussi le revert d'`eth_call`, le reçu reverted et un gas
consommé inférieur à la limite ; un manque de gas ne compte pas comme refus
sémantique. Les snapshots avant/après sont égaux : configuration Airlock,
propriétaire, supply, état du hook, soldes token/USDC du créateur, hook, Airlock
et pair, réserves, supply LP et LP du bénéficiaire. Achat, revente, migration V2
et leur preuve événement/réserves/LP passent dans ce même cycle.
La relecture de la preuve au bloc de migration ne remplace pas le contrôle des
réserves courantes avant/après le rejet : les deux sont effectués.

Tentative A : **exit 1** après les trois rejets, à cause d'une faute de champ
`governnance` dans la relecture finale du lanceur. Son rapport brut est conservé
avec `complete:false`. Elle ne devient pas une réussite. B corrige ce champ et
ajoute les réserves et LP aux snapshots ; le fork a été explicitement réinitialisé
au même bloc pour cette nouvelle couverture.

`independent-check.json` contrôle les trois reçus et égalités des snapshots dans
le résultat B. `executed-*.mjs` sont les sources réellement exécutées pour B.
Lancer le scénario avec la commande Anvil documentée dans tools/doppler-launch,
puis `node fork-lifecycle.mjs NEW-OUTPUT-DIRECTORY`.

Limites : trois attaques déterministes seulement, aucune preuve de fuzzing,
réentrance générale, reorg du bridge, backing Base/Nano, sécurité du protocole
Nano ou paramètres économiques réels. Le nom de lancement est Nanobees ; les
fixtures TEST restent synthétiques. Tous les autres critères restent ouverts.
