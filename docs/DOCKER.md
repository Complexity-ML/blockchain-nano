# Exécuter Nano avec Docker

L'image Ubuntu 24.04 compile et installe `nano` et `nano-p2p`. `nano run`
supervise les deux processus. Docker Compose ajoute un init pour transmettre
les signaux et récupérer les processus enfants. Le service utilise UID/GID
10001, une racine en lecture seule et un volume pour RocksDB et l'identité P2P.

## Démarrage local

Depuis la racine du dépôt, avec Docker Engine et le plugin Compose :

```sh
docker compose build
docker compose up -d
docker compose logs -f nano
```

La première compilation télécharge les dépendances cpp-libp2p et peut être longue.
Deux tâches de compilation sont utilisées par défaut ; `NANO_BUILD_JOBS=1 docker
compose build` réduit la pression mémoire. L'image est construite pour
l'architecture locale : construire directement sur le VPS pour Linux amd64,
ou qualifier séparément une construction croisée avec Buildx.

Le Compose fourni monte le genesis testnet du dépôt et démarre sans clé de
validation ni bootnode. Il ne crée donc pas un réseau actif à lui seul. Ajouter
les multiadresses des pairs à `deploy/docker/node.json` dans `bootnodes`, puis
recréer le service. Pour un autre réseau, monter son genesis dans
`/etc/nano/genesis.json` et utiliser un volume distinct. Ne jamais changer de
genesis sur une base existante.

```sh
curl -s -H 'Content-Type: application/json' \
  --data '{"jsonrpc":"2.0","id":1,"method":"eth_chainId","params":[]}' \
  http://127.0.0.1:8545
docker compose ps
```

Le RPC est publié sur la boucle locale de l'hôte. Le port P2P TCP 30303 est
publié pour les pairs ; configurer le pare-feu du serveur en conséquence.
Le contrôle de santé vérifie une réponse RPC, pas la synchronisation ni la finalité.
Une publication publique du RPC demande une configuration réseau adaptée.

## Données, arrêt et mises à jour

`nano-data` conserve `/var/lib/nano/chain` et `/var/lib/nano/peer.key`.
Un volume neuf reçoit les permissions du répertoire de l'image. Avec un bind
mount, préparer le répertoire pour UID/GID 10001. Ne pas mettre de clés privées
dans l'image, le dépôt ou le fichier Compose. La configuration fournie n'active
pas la signature BFT ; elle n'est pas une configuration de validateur prête à l'emploi.

```sh
docker compose stop                 # arrêt avec délai de 60 secondes
# Sauvegarder le volume arrêté avant une mise à jour.
docker compose build
docker compose up -d
```

`docker compose down` conserve le volume ; `docker compose down -v` le supprime.
Sauvegarder aussi l'identité P2P. Ne pas faire fonctionner deux instances avec
la même base de données. La rotation des journaux est limitée à trois fichiers
Docker de 10 Mo chacun.

## Portée de cette configuration

Le contexte de build est limité aux sources nécessaires et exclut les caches,
les fichiers de clés et les dépendances locales. La révision cpp-libp2p est
vérifiée par le script de build. Le tag Ubuntu et les paquets apt ne sont pas
figés par digest/version : ce packaging n'est pas une preuve de reproductibilité
bit à bit. Un conteneur ne modifie pas les règles de consensus et ne constitue
pas une qualification de production du réseau.

Après construction, `sh scripts/test-docker.sh` vérifie le RPC, l'utilisateur
non privilégié, le processus relais, l'arrêt et la persistance de l'identité
après redémarrage. Ce test crée puis supprime son propre volume temporaire,
sans publier de port ni contacter de pairs. Il ne qualifie pas la synchronisation
entre plusieurs conteneurs.

## Vérification du 24 septembre 2026

Construction Ubuntu 24.04 / Clang réussie sur Linux ARM64. Image locale vérifiée :
`sha256:9c75c9b2b083c0cf601d7b37efe1e768e654262724147e2a6ccc919d3ea92ae1`.
Le test `scripts/test-docker.sh` passe : RPC, utilisateur 10001, relais P2P,
arrêt avec code 0 et identité identique après redémarrage. Le test natif `chain`
passe également après l'adaptation des deux interfaces RocksDB (8.9 Ubuntu,
11.8.1 macOS). La construction Linux AMD64 reste à qualifier séparément.

La construction utilise Clang pour éviter l'échec de BoringSSL avec les
avertissements GCC 13 traités en erreurs. La détection Boost tient compte du
suffixe d'architecture des archives Hunter. Le répertoire `blst/build` contient
des sources assembleur indispensables et est conservé dans le contexte Docker.
