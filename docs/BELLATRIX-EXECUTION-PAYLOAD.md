# Payload d'exécution Bellatrix : format SSZ

Le module `consensus_execution_payload` implémente le décodage canonique, l'encodage
et `hash_tree_root` du conteneur Bellatrix `ExecutionPayload`. Il prépare la liaison
entre le consensus Beacon et l'exécution EVM, sans encore activer cette liaison.

Le conteneur suit la [spécification Bellatrix](https://ethereum.github.io/consensus-specs/specs/bellatrix/beacon-chain/#executionpayload).
Les bornes d'extra-data, de transactions et d'octets par transaction sont fournies
explicitement par le profil. Le budget de transport est distinct et vérifié
avant allocation au décodage. Les arbres SSZ sont calculés avec remplissage nul
sparse ; aucune allocation à la taille maximale du preset n'est effectuée.

Le décodeur exige le premier offset canonique, des offsets de transactions
monotones et dans le message, ainsi que les limites de listes. La validation des
offsets précède les copies des transactions. Le uint256 `base_fee_per_gas` reste
un tableau de 32 octets little-endian ; il n'est pas tronqué en uint64. Des
callbacks peuvent annuler validation, copie ou calcul des racines.

Les transactions sont des enveloppes opaques. Une transaction vide peut être un
ByteList SSZ valide ; cela n'autorise pas son exécution. Ce codec ne vérifie pas le
hash RLP du bloc, les signatures des transactions, le gaz, le timestamp de slot,
le parent d'exécution ou les racines produites par l'EVM. Il n'effectue aucune
écriture disque ni validation optimiste d'un payload reçu.

## Preuves

Huit vecteurs officiels `ExecutionPayload/ssz_random` du preset minimal :
[consensus-spec-tests](https://github.com/ethereum/consensus-spec-tests/tree/bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908/tests/minimal/bellatrix/ssz_static/ExecutionPayload/ssz_random),
commit `bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908`.
Le manifeste conserve taille et SHA-256 des racines, fichiers Snappy originaux et
SSZ décompressés. Le preset Bellatrix v1.6.0-beta.0 est conservé et vérifié aussi.
Le test C++ utilise directement les SSZ, sans dépendance Python ou Snappy runtime.

`ethereum_official_bellatrix_execution_payload` vérifie les racines officielles et
le réencodage exact de 21053 octets contenant 35 transactions opaques. Il couvre
les messages tronqués, les offsets incorrects, les bornes de listes, le budget,
les valeurs uint64/uint256 hautes, l'engagement de la longueur de liste et trois
points d'annulation. Les artefacts se trouvent dans
`build-cmake/qualification/bellatrix-execution-payload-20260925/`.
Ces huit cas ne représentent pas l'ensemble du corpus SSZ Bellatrix.

## Étapes restantes

Intégrer les conteneurs BeaconBlockBody/BeaconState Bellatrix, leur transition et
les paramètres économiques correspondants ; convertir et vérifier le payload
contre le moteur EVM ; lier parent, timestamp et RANDAO à l'état de consensus ;
publier puis restaurer ensemble les deux états avec preuve d'atomicité. Les
extensions Capella/Deneb et les forks ultérieurs exigent des schémas distincts.
Le profil Altair existant n'est pas modifié et le réseau n'accepte pas encore de
blocs Bellatrix du fait de ce seul codec.
