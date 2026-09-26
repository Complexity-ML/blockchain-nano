# Métadonnées Beacon natives — 2026-09-26

Source locale non commitée basée sur `30b0de21a1225ba55603de437c1200f3437a1ecb`.
Compilation du binaire Nano et du runner réussie ; test HTTP natif
`consensus_bellatrix_beacon_reads` passé en 0,81 s sur macOS ARM64.

Nouvelles routes :

- `/eth/v1/node/version` : version cohérente avec `web3_clientVersion`.
- `/eth/v1/beacon/genesis` : temps et racine des validateurs du BeaconState
  authentifié, version genesis du profil de signature des dépôts.
- `/eth/v1/config/spec` : projection partielle des paramètres Bellatrix réellement
  configurés (horloge, comités, bornes SSZ, staking, récompenses, slashing,
  opérations et payload). Quantités décimales sous forme de chaînes ; CONFIG_NAME
  inclut le SHA-256 du profil canonique. Aucun preset Ethereum déclaré par défaut.

Le test compare les réponses HTTP aux sources authentifiées, distingue limite
par comité et limite SSZ du registre, puis modifie un paramètre de profil et
vérifie que la valeur exposée et son identité changent. Les protections et les
lectures de tête précédentes restent exercées.

Limites : la réponse spec n'est PAS le catalogue complet de toutes les constantes
et de tous les forks exigé par la spécification Beacon. L'historique d'activation
non conservé dans le profil n'est pas inventé ; les epochs des futurs forks sont
absentes. Cela ne signifie pas que Nano doit reproduire l'histoire d'Ethereum :
il faut un calendrier explicite correspondant au réseau Nano effectivement lancé.
Aucune activation de fork, qualification Assertoor ou preuve production ajoutée.

## Vérification du consommateur réel

Source téléchargée par Go, module `github.com/ethpandaops/go-eth2-client v0.1.6`,
commit `745c3f52d19ef02121501270cc331aced5c59234`, somme
`h1:lG7Xz767YQQ+mN1ldzqJlj9Klg6zOElcemjXbCgJV5o=`.
C'est la version référencée par Assertoor au commit épinglé dans son document.

`http/service.go`, New / CheckConnectionState : l'initialisation appelle
**NodeSyncing avant les lectures de genesis/spec**. Un échec rend le service
inactif et New échoue sans allowDelayedStart. Ce contrôle ne sera pas désactivé
pour obtenir un démarrage artificiellement vert. La priorité de raccordement
suivante est donc la télémétrie réelle de synchronisation native, puis les corps
de blocs et événements. Source inspectée, SDK non exécuté contre Nano à ce stade.

Sources de format consultées le 2026-09-26 :
- https://github.com/ethereum/beacon-APIs/blob/master/apis/beacon/genesis.yaml
- https://github.com/ethereum/beacon-APIs/blob/master/apis/config/spec.yaml
- https://github.com/ethpandaops/go-eth2-client/blob/v0.1.6/http/service.go
