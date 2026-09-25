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

## Candidat d'exécution (2026-09-25)

`prepare_bellatrix_execution` décode un payload sous budget et construit un bloc
Paris. Le demandeur doit fournir un parent et son état authentifiés, le timestamp
de slot, le RANDAO, le chain ID, une politique de rejeu et l'historique canonique
pour `BLOCKHASH`. La fonction vérifie le lien au parent, le numéro, le temps,
le RANDAO, le hash RLP reconstruit et les règles de frais/gaz avant le rejeu EVM.
La racine de l'état fourni doit correspondre au parent. Les enveloppes signées
canoniques de types 0/1/2 sont admises ; types ultérieurs et enveloppes vides sont
refusés. Le uint256 de base fee est converti depuis son encodage little-endian.

Le rejeu compare les engagements d'état, transactions, reçus, bloom et gaz. Il
n'attribue pas de subvention PoW et travaille sur une copie de l'état parent.
Un REVERT de contrat reste une transaction valide avec reçu d'échec ; il ne
doit pas invalider à lui seul le bloc. Les exceptions d'annulation sont conservées
et propagées même lorsqu'un chemin EVM les traduit en échec d'appel.

`consensus_bellatrix_execution_candidate` utilise quatre transactions signées
synthétiques couvrant les types 0/1/2, PREVRANDAO, BLOCKHASH, stockage et REVERT.
Il vérifie les pourboires sans subvention PoW, 16 payloads invalides (dont des
engagements faux avec hash cohérent), l'état parent incorrect, le budget et 41
points d'annulation. Le parent reste inchangé. Ce test partage le moteur EVM avec
son générateur : il ne constitue pas une comparaison avec un client indépendant.
Les huit vecteurs officiels SSZ restent vérifiés séparément.

Preuve : `build-cmake/qualification/bellatrix-execution-bridge-20260925/final-tests.log`.
Le premier `tests.log` conserve l'échec d'annulation, corrigé par propagation
persistante de l'exception dans l'adaptateur. Le binaire `nano` a été reconstruit.

## Étapes restantes

Intégrer la transition complète BeaconBlockBody/BeaconState Bellatrix et les
paramètres économiques correspondants ; appeler l'opération d'exécution depuis
cette transition signée, après avancement des slots et avant mise à jour RANDAO ;
publier puis restaurer ensemble les deux états avec preuve d'atomicité. Les
extensions Capella/Deneb et les forks ultérieurs exigent des schémas distincts.
Le profil Altair existant n'est pas modifié et le réseau n'accepte pas encore de
blocs Bellatrix du fait de ce seul codec.
L'adaptateur accepte uniquement une continuation Bellatrix après Merge ; il ne
valide pas le bloc terminal PoW, n'active aucun fork et n'écrit aucune base.

## Conteneurs et liaison à l'état Beacon (2026-09-25)

`BellatrixExecutionPayloadHeader` conserve la racine SSZ de la liste des
transactions, distincte de la racine MPT de l'en-tête d'exécution. Son codec
canonique vérifie offset, largeur et budget. Le calcul des racines d'état prend
désormais en charge les 25 champs Bellatrix ; celui du corps inclut le dixième
champ `execution_payload`. Les wrappers Phase0 et Altair gardent leur schéma.

`prepare_bellatrix_payload_state` réalise l'opération isolée d'exécution sur un
état Beacon dont la racine est authentifiée par le demandeur. Il lie l'en-tête
d'exécution parent à `latest_execution_payload_header`, dérive le timestamp de
`genesis_time + slot * seconds_per_slot` avec contrôle de dépassement, et prend
le mix RANDAO de l'epoch courante dans l'état. L'ordre attendu est celui de
[Bellatrix process_block](https://ethereum.github.io/consensus-specs/specs/bellatrix/beacon-chain/#block-processing) :
après les slots et le header, avant le nouveau reveal RANDAO.

Après vérification EVM, seule la fin de l'état contenant l'en-tête d'exécution
est remplacée. La fonction retourne deux candidats privés, Beacon et EVM, sans
écrire dans RocksDB. Une erreur ou annulation ne modifie aucun des parents.
L'historique `BLOCKHASH` doit être canonique et disponible : exception du
fournisseur ou hash de largeur incorrecte provoque un refus, même si un payload
présente des engagements correspondant à l'erreur hôte traduite en échec EVM.
Un hash de 32 octets n'est pas authentifié par sa seule largeur ; la provenance
canonique de l'historique reste à la charge de l'appelant.

Qualification dans `build-cmake/qualification/bellatrix-state-binding-20260925/final-tests.log` :

- 24 vecteurs SSZ officiels supplémentaires (8 en-têtes, 8 états, 8 corps),
  commit `bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908`, 218565 octets décompressés ;
  presets, fichiers Snappy, octets et racines vérifiés par manifeste.
- 20 conteneurs malformés ou hors budget refusés, deux modifications d'engagement
  imbriqué détectées, neuf points d'annulation des calculs de racines.
- Six refus de liaison à l'état (racine, parent, RANDAO, slot/temps, dépassement
  arithmétique, budget du nouvel état), quatre points d'annulation, deux erreurs
  d'historique qui ne peuvent pas devenir une exécution valide.
- Sept tests ciblés passent, dont les suites historiques de racines Phase0 et
  Altair et de transitions complètes Altair ; `nano` reconstruit.

Les essais de liaison utilisent une enveloppe d'état SSZ synthétique, dérivée
d'un vecteur de format : ils ne prouvent ni son cycle de vie validateur, ni une
transition Bellatrix signée complète. Les opérations Beacon, les slots/epochs
Bellatrix, les paramètres de pénalité, le choix de branche, le commit atomique
et la reprise des deux états restent à intégrer. Aucun fork réseau n'est activé.
