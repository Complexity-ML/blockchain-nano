# Preuve de producteur PoS — version 1

Travail en cours : `config.posSignatureVersion: 1` active la vérification
obligatoire à l'import, avant exécution, ainsi que dans `validator_submitBlock`,
y compris pour un bloc déjà connu. La production native signée passe par
`Blockchain::validate_signed`. Les producteurs CLI, distants et automatiques signent les blocs activés ;
les anciens genesis sans activation restent non signés.

Le champ JSON optionnel `producerProof` contient exactement `version: 1` et
`signature`, hexadécimal de 65 octets : r (32), s (32), parité de récupération
(1, valeur 0 ou 1). Le parseur refuse les autres versions et tailles.
La vérification exige une signature secp256k1 low-s et un signataire correspondant
au champ `miner` de l'en-tête.

Le digest signé est Keccak-256 du RLP de :

1. La chaîne d'octets ASCII `Nano/PoS/Block`.
2. La version entière 1.
3. Le chain ID entier strictement positif sur 256 bits au maximum.
4. Le hash du genesis sur 32 octets.
5. L'encodage RLP complet de l'en-tête sous forme d'octets.
6. La liste des transactions signées brutes, dans leur ordre.

La version 1 ne signe pas le genesis et refuse les oncles et retraits de type
Ethereum. Le hash de l'en-tête reste l'identifiant du bloc ; la preuve est une
authentification externe à cet identifiant, à vérifier également lors d'une
soumission dupliquée. La vérification de signature ne remplace pas le contrôle
des racines, de l'exécution ni de la sélection du producteur.

La suite `block_auth` vérifie l'aller-retour JSON, le rejet d'un bloc non signé,
les changements d'en-tête et de corps, la mauvaise clé, les chaînes/genesis
différents, la variante high-s et les encodages mal formés.

## Activation et persistance

Un nouveau genesis PoS doit préciser `posSignatureVersion: 1` et
`bootstrapValidator`, adresse non nulle dont l'opérateur possède la clé.
La version, le chain ID, le contrat de staking, le bootstrap et l'ancien extraData
sont engagés dans un nouvel extraData de genesis par Keccak/RLP. L'identité de
la base change donc avec cette configuration ; elle ne peut pas être abaissée
silencieusement à la version non signée. Aucune base existante n'est modifiée.

Le bootstrap configuré n'est choisi que tant que le bootstrap du registre est
actif et qu'il n'existe aucun validateur. Le registre de contrat prend ensuite
le relais. La signature du bloc de tête est revérifiée à la réouverture.
Les preuves sont conservées par la sérialisation JSON du stockage.

La suite block_auth vérifie aussi : refus sans modification de l'état ni du
nombre de blocs, refus d'un signataire non sélectionné, production signée,
rejeu par un second moteur, réouverture RocksDB et refus d'un doublon RPC dont
la preuve a été supprimée ou falsifiée.

## Producteurs

`nano validate genesis.json db ADRESSE TIMESTAMP` lit `VALIDATOR_KEY` pour
une chaîne signée et refuse une clé différente de l'adresse demandée. Le chemin
non signé ne peut pas contourner la règle de consensus.

Le client `nano validator validator.json` exige les identités attendues :

```json
{
  "nodes": ["http://127.0.0.1:8545"],
  "chain_id": "0x539",
  "genesis_hash": "0xHASH_DU_GENESIS_VERIFIE",
  "signing_db": "/chemin/persistant/validator-signing"
}
```

Le hash doit être obtenu d'une configuration de réseau vérifiée. Le client
compare le chain ID RPC et le genesis du modèle aux valeurs configurées avant
signature. Avec un genesis épinglé, il refuse aussi un modèle non signé.
La clé reste locale (`VALIDATOR_KEY`) ; le serveur distant n'a pas à déverrouiller
un portefeuille. Un nœud RPC malveillant peut encore proposer des blocs dont
l'exécution ou la sélection est incorrecte : l'épinglage de l'identité ne
constitue pas une réexécution locale du modèle par le validateur distant.

Le validateur automatique du nœud signe avec le portefeuille coinbase déverrouillé.
Le minage RPC de développement sur PoS signé exige aussi la clé du producteur
sélectionné. Les restrictions des portefeuilles RPC à l'interface locale restent
appliquées. La suite `signed_validator_cli` couvre les trois producteurs, le
rejeu d'un bloc CLI, les identités erronées et la soumission sans preuve.

## Réservations persistantes avant signature

Chaque producteur utilise une réservation RocksDB synchrone indexée par version,
chain ID, genesis, adresse et hauteur. Elle enregistre le digest et le bloc complet dans une même WriteBatch
synchrone avant toute signature. Une nouvelle signature du même digest est permise ; un digest
différent à la même hauteur est refusé, même après redémarrage ou restauration
d'un snapshot de développement. La restauration de chaîne n'efface pas ces clés.

Les producteurs locaux utilisent leur base de chaîne. Le validateur distant
exige `signing_db`, un dossier persistant exclusivement verrouillé par RocksDB
pendant l'opération. L'échec d'écriture bloque la signature. Après une coupure, le producteur relit la proposition réservée et vérifie son
digest, son adresse et sa hauteur avant de la signer à nouveau. Il reprend
exactement le même bloc, même si le modèle RPC a changé entre-temps. Une ancienne
réservation sans corps de bloc reste conservatrice : elle ne permet pas de
reconstituer une proposition différente.

Cette protection est locale au journal. Ne jamais utiliser la même clé avec
des journaux indépendants, supprimer un journal ni restaurer une ancienne copie
sans procédure de récupération. Les copies de clés sur plusieurs machines ne
sont pas rendues sûres par un verrou de fichier. Le protocole de finalité reste à développer ; une proposition réservée devenue
invalide sur une autre branche ne peut pas être remplacée à la même hauteur.

La suite block_auth vérifie la répétition identique, le conflit après réouverture,
le verrou du journal et la conservation de la réservation après un rewind.
Elle simule aussi une fermeture entre réservation et signature. Le test client
coupe la connexion RPC avant la première soumission, puis propose un timestamp
différent au redémarrage : le bloc initial est néanmoins renvoyé et accepté.
Le test client couvre aussi un arrêt réel par SIGKILL : un proxy retient la
soumission signée avant livraison au nœud, pendant que le journal et la base de
vérification sont ouverts. Le processus est tué sans fermeture propre. Après
redémarrage, le bloc soumis conserve exactement son hash et sa preuve malgré
un nouveau modèle RPC. La base de vérification est ensuite auditée. Cela ne
simule pas une perte d’alimentation du système de fichiers ni une panne disque.

Les genesis livrés activent maintenant les signatures (voir
`GENESIS-VERSIONS.md`). Le protocole de finalité reste à intégrer. L’audit explicite de toute la chaîne
est décrit dans `STORAGE-AUDIT.md`.

## Transport cpp-libp2p vérifié

La suite `signed_network` lance deux moteurs C++ et deux daemons cpp-libp2p.
Un validateur distant avec identité épinglée et journal persistant produit
les blocs. Le second nœud synchronise trois blocs historiques puis reçoit une
annonce de nouveau bloc incluant une transaction propagée par le réseau.
Les quatre preuves sont conservées à l'identique ; hash, racine d'état et reçu
concordent entre les nœuds. Le redémarrage du daemon conserve son identité P2P.
Les pairs avec un chain ID ou un genesis différent sont refusés.

Le test soumet aussi au RPC du second nœud une copie sans preuve et une copie
avec preuve altérée ; les deux sont refusées sans changement du bloc de tête.
Cela ne constitue pas encore une injection de paquet forgé depuis un pair
malveillant, ni un essai prolongé de partitions ou de finalité.

## Identité du RPC avant inscription

Le client `nano validator` exige `chain_id` et `genesis_hash` lorsque
`register: true`. Si l’un des deux paramètres est fourni, l’autre est requis.
Les quantités et la longueur du hash sont validées avant toute requête. Le
client vérifie ensuite `eth_chainId` et le hash du bloc zéro sur le serveur
courant avant chaque requête, y compris les tentatives répétées et les
basculements vers un autre serveur. Une identité divergente interdit l’envoi
de la requête métier à ce serveur. Les validations du domaine du modèle signé
restent également appliquées.

Cela ajoute deux lectures RPC par tentative. Il s’agit d’une protection contre
les erreurs de configuration, pas d’une preuve cryptographique du serveur :
un endpoint malveillant ou une infrastructure changeant de backend entre les
requêtes peut mentir. Le rejeu local décrit ci-dessous vérifie l’exécution et la sélection ; il
ne prouve pas la finalité ni que le serveur présente la meilleure branche. Un validateur configuré avec ces pins refuse toujours les
modèles de blocs non signés, même après une inscription réussie.

## Rejeu avant réservation locale

`Blockchain::verify_proposal` vérifie une proposition PoS sans exiger une
preuve déjà signée et sans publier d’état : extension du parent, timestamp,
gaz, sélection du producteur, base fee, exécution et engagements du bloc, puis
registre de consensus résultant. La même routine interne vérifie le contenu
des blocs importés, dont l’authentification reste obligatoire séparément.

`Blockchain::reserve_signature` effectue aussi ce rejeu sous le verrou de la
chaîne avant d’écrire la réservation. Une proposition altérée ou devenue
incompatible avec la tête locale ne peut donc pas réserver une nouvelle
signature par cette API. Les producteurs locaux et les reprises de leurs
propositions passent par ce contrôle. Le journal externe `ChainStore` reste
un stockage de réservations, sans moteur de validation : le validateur distant
utilise désormais une chaîne locale de vérification avant de réserver dans
ce journal.

## Chaîne locale du validateur distant

La signature distante exige maintenant `genesis_file` (fichier genesis local)
et `verification_db` (dossier RocksDB distinct du nœud et de `signing_db`). Le
genesis local doit activer les signatures et correspondre exactement aux pins
`chain_id` et `genesis_hash`. Exemple de paramètres supplémentaires :

```json
{
  "genesis_file": "/chemin/genesis.json",
  "verification_db": "/chemin/validator-verified-chain"
}
```

À la première proposition du processus, la base locale est auditée avant
utilisation, puis reste ouverte et verrouillée pendant les cycles suivants. Les parents
manquants sont téléchargés avec leurs transactions complètes, authentifiés et
rejoués dans l’ordre. Chaque cycle importe au maximum 128 blocs ; s’il en manque
encore, il retourne un résultat `skipped` de synchronisation sans signature. Le
mode continu reprend au cycle suivant ; `--once` doit être relancé. Après le
rattrapage, `verify_proposal` contrôle la proposition, y compris la sélection
locale du producteur et les engagements issus de l’exécution. Une proposition
réservée antérieurement est également revérifiée avant sa nouvelle signature.

Le validateur ne rembobine pas sa chaîne pour suivre un serveur divergent et
refuse une proposition qui ne prolonge pas son historique vérifié. L’audit
complet initial coûte du temps et de l’espace temporaire proportionnels
à l’historique. Le mode continu ne le répète pas à chaque cycle : il conserve
l’état vérifié en mémoire et importe les nouveaux parents incrémentalement.
Un redémarrage ou une nouvelle invocation `--once` impose un nouvel audit.
Les limites RPC bornent les réponses individuelles, pas le volume total
d’historique. La finalité, les partitions et la sélection entre branches
restent des travaux distincts. Aucun état distant n’est accepté comme état de
départ sans rejeu depuis le genesis.

Le test RPC falsifie d’abord la signature d’un parent, puis la racine d’état du
modèle : aucune soumission ne doit être émise. Une fois ces falsifications
retirées, le test vérifie la reprise d’une soumission interrompue en conservant
la proposition réservée. Le test réseau signé exerce ce chemin avec une
transaction et deux nœuds cpp-libp2p. Le test CLI produit aussi plusieurs blocs
en mode continu, vérifie le verrou exclusif pendant son fonctionnement puis
réaudite sa base après un arrêt SIGTERM.
