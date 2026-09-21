# Comparaison des précompilés Python / Nano

`python-precompiles.json` contient 111 appels réels aux fonctions du fichier
`nanopy/vm/precompiles.py` du dépôt Python original. Son SHA256 est enregistré
avec les vecteurs. Le générateur utilise une graine fixe (1337) et désactive
l'écriture de bytecode Python dans le dépôt de référence.

```sh
PYTHONDONTWRITEBYTECODE=1 /chemin/vers/python scripts/generate-precompile-fixtures.py /chemin/vers/blockchain-python
```

L'environnement doit disposer des dépendances du Python de référence (`eth_keys`,
`eth_utils`, `py_ecc`). Les 111 cas se répartissent ainsi :

- 27 SHA256, RIPEMD160 et identité, aux frontières de mots de 32 octets ;
- 16 ECRECOVER : signature valide, high-s, valeurs invalides et padding ;
- 38 MODEXP : exposants nuls/longs, entrées tronquées et tailles autour des
  branches de coût à 64 et 1 024 octets ;
- 26 additions/multiplications BN128 valides, infini, grands scalaires et padding ;
- 4 coordonnées BN128 invalides acceptées par le Python et rejetées par Nano.

Pour les 107 cas communs, la suite core compare les octets du résultat et le coût
exact, puis vérifie l'échec avec une unité de gaz en moins. Les quatre écarts
sont identifiés explicitement : la réduction implicite de coordonnées hors du
corps et l'absence de validation de courbe du Python ne sont pas reproduites.

Cette comparaison conserve la formule MODEXP de la source ; son commentaire
EIP-2565 ne suffit pas à prouver la conformité Ethereum. Le pairing est couvert
séparément par les vecteurs go-ethereum existants. BLAKE2F utilise les vecteurs
EIP-152 existants, puisque la source Python renvoie un placeholder. L'adresse 10
reste absente du registre de fonctions Python et non implémentée dans Nano.
