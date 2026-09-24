# Régressions des précompilés Nano

`precompiles.json` contient 111 vecteurs initialement capturés depuis
`nanopy/vm/precompiles.py` du prototype. Le chemin source et son SHA-256 restent
conservés comme provenance ; ces vecteurs ne sont pas des fixtures officielles
Ethereum. Le dépôt du prototype reste inchangé.

Les attentes corrigées suivent les règles Ethereum : une signature ECRECOVER
invalide produit une sortie vide, et MODEXP utilise le prix Berlin EIP-2565.
Les anciennes valeurs sont conservées dans `prototype_output` et `prototype_gas`
avec la raison de la correction. Les quatre coordonnées BN128 invalides
acceptées par le prototype restent explicitement signalées et doivent être
rejetées par Nano.

Les 111 cas couvrent 27 opérations SHA256/RIPEMD160/identité, 16 ECRECOVER,
38 MODEXP, 26 opérations BN128 valides et quatre invalides. Pour les 107 appels
valides, `core` vérifie les octets du résultat, le coût exact et l'échec avec une
unité de gas en moins. Une signature ECRECOVER invalide est un appel réussi à
sortie vide, pas une erreur de précompile.

Le pairing et BLAKE2F sont couverts séparément par leurs vecteurs existants.
Le précompile KZG de Cancun a sa suite native `kzg` et des fixtures Ethereum
complètes dans `ethereum-official/`. La qualification officielle et ses limites
sont documentées dans `../../docs/EVM-CONFORMANCE.md`.
