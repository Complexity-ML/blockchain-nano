# Proposition publique — main HTTP et persistance C6

Paquet de données et plan, **pas une qualification d’exécution**. Il prépare le
lot suivant l’import de C6. Root rapporte maintenant main/origin/VPS à
bc99b0ec2cf8bc6bcb21a26ec980ef2c4e8573a8 ; les anciennes qualifications C6
restent attachées au main4388 observé pendant leur exécution. Le producteur
réseau1925 et les domaines signés anciens restent séparés.

## Données exactes, sans nouvelle signature

`source-fixture.json` est la copie exacte de la fixture officielle publique
Shanghai a6524e0d… /11527 B, licence copiée sans modification. Le script
`derive-fixture.py` n’importe que la bibliothèque standard. La préparation a
exécuté uniquement cet encodeur de données : pas de CLI Nano/RPC, SDK, Node,
Keccak, clé ou signature, Docker, SSH, test de projet, écriture R/Git.

`transaction-fields.json` conserve les champs signés existants v/r/s et sender.
`signed-raw-transactions.json` contient une unique transaction legacy de99 B.
L’encodage canonique des neuf champs RLP est exactement le span543:642 du bloc
RLP original ; les octets de la signature sont conservés. SHA-256 des **99octets
binaires** :197439dfb3cdece42bec497e78c69adcc34842bd8b1ea7e8ad34d71e1e663f2c.
Ce pin diffère volontairement du pin du fichier JSON209 B qui contient leur hex.

Le hash Ethereum attendu depuis le reçu source est
0x8d2e4fa292b46a6ae5cc2a0481c6b1447176b84613cb88f9e76c3b8a55ed4213.
Il **n’a pas été recalculé pendant la préparation**, et sender n’a pas été
récupéré cryptographiquement. Root devra vérifier Keccak-256(raw), la signature
et son sender lors de l’admission réelle ; `hashlib.sha3_256` n’est pas utilisé.
Le résultat d’eth_sendRawTransaction doit également égaler ce hash attendu.

`genesis.json` conserve intégralement les trois allocations pre, le gasLimit
120000000, la baseFee7, le timestamp0, nonce et extraData de source. Il indique
chainId1, profilShanghai, consensusPoW et difficulté1 positive, avec le codec
GenesisHeaderVersion2. Ce réseau de contrôle est privé : aucun RPC/P2P public.
Les formats et racines du bloc Ethereum source ne sont pas présentés comme
ceux de Nano. Les récompenses PoW de Nano empêchent d’exiger le post-state root
EEST complet ; les racines source de `derivation-public.json` sont des références
sans assertion dans le worker.

## Contrat du worker coordonné

`expectations.json` est un **modèle non exécutable** : ready:false et nano:null.
Root devra créer un autre contrôle neuf, compléter uniquement ready:true et
nano:{sha256,bytes} avec le vrai pin du binaire approprié, puis épingler le contrôle
avant lecture. Ne pas modifier ce paquet gelé. Les pins genesis et transactions
sont déjà présents ; les autres champs sont les constantes exactes du scénario.
Le worker Local utilise des assertions directes, sans mini-langage de requêtes.

Attentes : txhash ci-dessus ; reçu status0x1, type0x0, gasUsed0xd5ee ; sender
nonce0→1 et balance0x33b2e3c9fd0803ce8000000→0x33b2e3c9fd0803ce7f7a4b4 ;
contract cible nonce1, balance0 et code source inchangés ; slots0/1 passent de
[0,0] à[0,0x67]. Ces valeurs proviennent de la fixture, jamais des réponses
candidates. Les écritures et lectures sont authentifiées seulement contre les
racines fournies par les headers observés, pas par une finalité Beacon.

## Séquence bornée proposée, une fois par architecture

1. Créer une base neuve isolée ; lancer le vrai binaire avec
   `serve GENESIS DB 0 127.0.0.1 CONFIG`. CONFIG contient littéralement{}.
   Le port choisi vient du banner du propre processus. Vérifier l’état initial,
   code/nonce/balances/storage, HTTP, comptes déverrouillés vides et refus
   d’evm_mine, puis l’admission des octets signés existants. Aucun wallet,
   dev_mode, validateur, Capella, BFT ou service P2P n’est configuré.
2. Envoyer SIGTERM au seul enfant possédé, joindre sa sortie et prouver son
   absence **avant** un autre accès RocksDB. Exécuter
   `mine GENESIS DB COINBASE 12 RAW_JSON` : l’admission HTTP et l’inclusion CLI
   sont deux faits séparés ; aucune persistance de la mempool n’est supposée.
   Rouvrir serve, vérifier bloc1, reçu/nonce/code/storage, historique Genesis et
   preuves par hash/numéro.
3. Arrêter/rejoindre de nouveau, puis `mine GENESIS DB COINBASE 24` sans RAW.
   Troisième ouverture HTTP : bloc2 vide, bloc1/transaction/reçu persistés,
   états et preuves historiques encore cohérents. Arrêt/jonction final,
   clôtures des sources/binaires/inputs et absence des processus/conteneur.

Les timestamps12 puis24 excèdent strictement leurs parents. L’écart12 garde la
difficultéPoW1, donc ce test n’exige aucune charge de minage importante ; le
mine interne est déjà borné10s. Ne pas ouvrir simultanément CLI et serveur sur
la même base. Un arrêt forcé/refus de jonction doit faire échouer le lot ; ne pas
présenter ensuite un simple reopen comme arrêt propre qualifié.

Cinq captures de preuves HTTP sont prévues : Genesis ; bloc1 et Genesis
historique ; bloc2 et bloc1 historique. Format du module déjà qualifié :
{scenario:'proof-vector',root:header.stateRoot,address:target,keys,
 expectedPresent:true,expectedValues,response:résultat_eth_getProof}.
Le futur mini-pilote Root appelle le module indépendant déjà qualifié ; le driver
94b, figé pour l’ancien flux de30records, ne correspond pas à ce nouveau flux.
Il n’est ni modifié ni rejoué, et les huit unités JS restent acquises séparément.

Plafonds proposés :600s par architecture, moins de60requêtesHTTP,5s parrequête,
30s par ouverture/arrêt et10s par mine. Logs/receipts et ressources restent bornés.
AMD réutilise l’image fd9 existante : networknone, client HTTP dans son loopback,
logiciel/providersRO, seul nouveau DB/outputRW ; aucune image construite/pullée.
Les anciennes rows produit/logiciel restent strictes ; nouvelles données admises
prospectivement aux pins connus. Cleanup et fences finales sont indépendants,
avec première erreur conservée, jamais un ancien journal/domaine03.

## Exécutables et limites d’autorité

Native : E/native-rpc-state-proof-root-20261017-04/nano,
ac7f9b611a89b7c74523c655c499434dd3debb3bb754bed7fa8167d4fd39ce29 /15160760 B,
retenu par l’autorité Native04 puis la continuation. AMD :
/home/ubuntu/rpc-candidate-amd-root-20261018-01/outputs/nano ; son pin physique
et ses octets viennent exclusivement de son producteur original
final-observations.private.json épinglé866e3f3d… /1512552 B. Root seul lit cette
autorité restée sur le VPS ; ce paquet ne contient ni ne fabrique sa row.

Ce lot vise main/serve, transportHTTP, inclusion transactionnelle et reopenRocksDB.
Il ne qualifie pas Beacon, la finalité multi-nœud, pruning, crash physique,
bridge, Doppler, fermeture SDK globale ou production. Source/fixtures nouvelles
ne valent pas autorisation de campagne04, nouvelle signature, fonds réels,
vente, exposition publique ou dépense. Aucune opération du plan n’a été lancée.
