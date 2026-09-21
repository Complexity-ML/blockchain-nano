# NanoStaking

Les dépôts et retraits sont des transactions signées, exécutées dans un bloc.
Le registre PoS est ensuite recalculé depuis le contrat. Aucun appel de staking
RPC ne modifie immédiatement les soldes ou le registre local.

## Règles

- Dépôt minimal : 10 000 Nano ; maximum 1 000 validateurs.
- Le constructeur payable inscrit le déployeur comme premier validateur.
- `registerValidator()` inscrit uniquement l'émetteur, avec le montant envoyé.
- `addStake()` augmente son dépôt et renouvelle le verrouillage.
- Verrouillage : 100 blocs après inscription ou ajout.
- `withdrawStake()` retire la totalité après verrouillage. Le dernier validateur
  ne peut pas sortir avant l'inscription d'un remplaçant, afin de conserver un
  producteur. Le remboursement et la suppression sont atomiques.
- Pas de propriétaire, proxy ou retrait administrateur. Un verrou protège les
  appels réentrants. Le slashing du module natif n'est pas raccordé au contrat.

## Construire les artefacts

Depuis la racine du dépôt, avec Node.js et npm :

```sh
npm install --prefix .deps/solc --ignore-scripts solc@0.8.28
node scripts/build-staking.mjs
node scripts/build-staking-test.mjs
```

`NanoStaking.json` contient l'ABI, les bytecodes, la version du compilateur,
les paramètres et le SHA256 du source. `NanoStaking.hex` est le bytecode de
construction utilisé par la CLI. La cible EVM est Paris, optimisation 200 passes.

## Démarrer une nouvelle chaîne PoS

Déchiffrement des portefeuilles : définir `NANO_WALLET_PASSWORD` dans
l'environnement local sans l'inscrire dans la configuration publique.

1. Calculer l'adresse future avec `nano wallet deployment-address deployer.json 0`.
2. Définir cette adresse dans `config.stakingContract` du nouveau genesis PoS.
   Allouer au déployeur au moins 10 000 Nano plus les frais. Utiliser une nouvelle
   base RocksDB : modifier cette identité sur une base existante est refusé.
3. Démarrer le nœud et la production bootstrap. Déployer avec le nonce prévu,
   sans autre transaction du déployeur entre-temps :

```sh
nano rpc http://127.0.0.1:8545 deploy deployer.json contracts/NanoStaking.hex 120 10000
```

Le client attend un reçu réussi (120 secondes ici). La production doit tourner
pendant cette attente. Vérifier que l'adresse créée correspond au genesis.
Le constructeur inscrit le premier validateur ; celui-ci prend ensuite le relais
de la production bootstrap. Le contrat configuré devient la source du registre.
Les genesis distribués ne sont pas modifiés automatiquement.

## Opérations depuis un client

Remplacer `CONTRACT` par l'adresse vérifiée du contrat :

```sh
nano rpc http://127.0.0.1:8545 stake-register validator.json CONTRACT 10000
nano rpc http://127.0.0.1:8545 stake-add validator.json CONTRACT 1
nano rpc http://127.0.0.1:8545 stake-withdraw validator.json CONTRACT
```

Ces commandes vérifient que la destination possède du code et correspond au
contrat de staking configuré de la chaîne, puis signent localement et envoient
`eth_sendRawTransaction`. Une adresse sans code ou un autre contrat est refusé
avant soumission. Ces vérifications reposent sur les réponses du serveur RPC ;
elles ne remplacent pas la vérification indépendante de son identité. Le serveur
peut garder `eth_accounts` vide. Les montants CLI sont en Nano (18 décimales).
Elles renvoient le hash soumis ; consulter le reçu pour confirmer le succès.
L'estimation ajoute 25 000 unités de gaz, dans la limite du bloc, pour absorber
notamment le changement du verrou entre simulation et inclusion. Une estimation
ne garantit pas le succès si l'état change avant l'exécution.

Les RPC `nno_registerValidator(address, amountWei)`, `nno_addStake(address,
amountWei)` et `nno_withdrawStake(address)` sont également disponibles pour un
compte explicitement déverrouillé sur un nœud local. Les montants RPC sont des
quantités hexadécimales en wei. Elles renvoient un hash, contrairement aux
anciennes mutations immédiates du Python. Un serveur sans portefeuille ne peut
pas signer pour l'adresse fournie.

## Validation et limites

Les suites `staking` et `staking_cli` exécutent le bytecode compilé dans l'EVM
Nano : déploiement payable, inscription, ajout, retrait, refus avant échéance,
refus du dernier retrait, rejeu sur un pair et réouverture RocksDB. Le test CLI
utilise un serveur sans portefeuille et des signatures côté client.
La suite native comprend 454 contrôles, dont un destinataire contractuel qui
refuse le remboursement : dépôt, registre et fonds restent alors intacts et
les logs sont annulés. Un nouvel essai accepte le remboursement tout en tentant
un retrait réentrant ; cette tentative échoue et un seul dépôt est remboursé.
Le contrat adversarial est dans `tests/contracts/StakingReceiver.sol` ; son
artefact est reproductible avec le second script ci-dessus.

Le contrat et le consensus n'ont pas fait l'objet d'un audit externe. Les blocs
PoS ne sont pas encore authentifiés par une signature de producteur. Ce travail
ne constitue donc pas une validation pour un lancement public. La cible production privilégie désormais des règles corrigées ; la transition
des historiques divergents doit être versionnée et documentée.

## Inscription optionnelle du client validateur

`nano validator validator.json [--once]` accepte cette configuration :

```json
{
  "nodes": ["http://127.0.0.1:8545"],
  "register": true,
  "staking_contract": "0xADRESSE_DU_CONTRAT",
  "stake_nano": "10000",
  "registration_timeout": 120
}
```

Définir `VALIDATOR_KEY` localement comme pour le client validateur existant.
L'inscription est désactivée par défaut. Si un stake existe déjà, aucun nouveau
dépôt n'est envoyé. Sinon, le client signe localement, affiche l'événement
`registration_submitted` avec le hash puis attend le reçu avant de produire.
`stake_nano` est une chaîne décimale exacte, sans conversion flottante.
`registration_timeout` accepte 1 à 3 600 secondes. Un échec ou délai expiré
arrête le démarrage ; la transaction déjà soumise reste dans la mempool.
Au redémarrage, un nonce pending différent du nonce confirmé bloque une
nouvelle inscription et demande de vérifier les reçus. Cela évite une deuxième
soumission lorsque le nœud connaît déjà une transaction du compte en attente.
Cette vérification ne détecte pas une transaction absente de la mempool du nœud
interrogé ; ne pas lancer plusieurs inscriptions concurrentes avec la même clé.
Après inclusion, le client constate le stake et reprend sans nouveau dépôt.
Un producteur existant doit
inclure la transaction pendant cette attente.
