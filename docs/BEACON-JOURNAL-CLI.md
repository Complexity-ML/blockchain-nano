# Migration hors ligne du journal Beacon

État au 25 septembre 2026 : commandes locales disponibles, sans activation des
validateurs Beacon dans `nano run`. Elles ne chargent aucune clé privée, ne
signent aucun message et ne déplacent aucun fonds. Le journal BFT historique
hauteur/tour reste distinct.

## Identité et engagement attendus

Le fichier d'identité contient exactement les cinq champs suivants :

| Champ | Valeur |
| --- | --- |
| `chainId` | Quantité hexadécimale canonique positive |
| `executionGenesis` | Hash du genesis d'exécution, 32 octets |
| `genesisValidatorsRoot` | Racine des validateurs au genesis, 32 octets |
| `publicKey` | Clé publique BLS du validateur, 48 octets, vérifiée sur la courbe |
| `slotsPerEpoch` | Quantité hexadécimale canonique positive |

Les encodages sont préfixés par `0x` et en minuscules. Ces valeurs doivent provenir
de la configuration effective du réseau et de l'identité du validateur. Une
identité ne contient ni clé privée, ni mot de passe, ni chemin de keystore.

Pour une base existante, chaque commande exige un fichier d'engagement conservé
indépendamment, avec exactement `sequence` (quantité hexadécimale) et `digest`
(hash de 32 octets). Un ancien engagement ou une identité différente sont refusés.
La commande ne remplace jamais cet engagement par une valeur découverte sur disque.

## Commandes

```sh
# Initialisation explicite d'une nouvelle base dédiée, uniquement hors ligne.
nano beacon-journal init identity.json new-journal init-receipt

# Contrôle d'une base existante sans nouvelle réservation.
nano beacon-journal status identity.json journal expected-commitment.json

# Retrait permanent du journal source, puis export EIP-3076 v5.
nano beacon-journal export identity.json source-journal expected-commitment.json export-receipt

# Nouvelle base destination, encore inactive ; l'initialisation seule n'est
# pas une autorisation de signer avec une clé déjà utilisée ailleurs.
nano beacon-journal init identity.json destination-journal destination-init

# Import dans la destination et conservation de son nouvel engagement.
# 1048576 est ici un budget explicite d'octets, à adapter au fichier réel.
nano beacon-journal import identity.json destination-journal destination-init/commitment.json export-receipt/interchange.json 1048576 destination-import

nano beacon-journal status identity.json destination-journal destination-import/commitment.json
```

Les dossiers de résultats doivent être nouveaux, hors de la base et avec un
parent existant. Les chemins relatifs sont résolus depuis le répertoire courant.
La commande refuse un écrasement et ne supprime aucun résultat précédent. `init`
refuse une base ou un lien déjà présent ; `status/import/export` exigent une base
RocksDB existante. Une base détenue par un autre processus reste verrouillée par
RocksDB : il faut arrêter ses utilisateurs avant la migration.

L'import applique seulement la clé du fichier d'identité. Son résultat indique
le nombre d'entrées correspondantes fusionnées, qui peut dépasser un si le fichier
répète cette clé. Un fichier sans entrée correspondante est refusé par la CLI.
Les autres clés ne sont pas importées. Les contrôles et limites de fusion figurent
dans [le journal de signature](BEACON-SIGNING-JOURNAL.md).

## Résultats et durabilité

Chaque commande réussie imprime un objet JSON contenant l'opération, la clé
publique, l'engagement et l'état `retired`. Les opérations qui écrivent créent
également un dossier privé (0700), contenant des fichiers privés (0600) :

1. `intent.json`, synchronisé avant modification de la base : opération,
   identité et ancien engagement attendu ; ce fichier ne prouve pas la réussite.
2. `commitment.json`, nouvel engagement écrit après le commit du journal.
3. `interchange.json` pour l'export uniquement, au format EIP-3076 version 5.
4. `complete.json`, reçu de fin écrit en dernier. Le SHA-256 annoncé pour un
   export porte sur les octets du fichier `interchange.json`, fin de ligne incluse.

Les écritures synchronisent les fichiers et leur répertoire. Vérifier le code de
sortie et le reçu de fin ; ne pas déduire une réussite d'un dossier partiellement
rempli. Conserver le nouvel engagement hors des sauvegardes susceptibles d'être
restaurées avec la base. Le dossier séparé créé par la commande ne garantit pas,
à lui seul, cette indépendance opérationnelle.

Un journal exporté reste retiré après redémarrage et ne peut plus importer ou
signer. Une nouvelle exportation est autorisée avec son engagement courant et un
nouveau dossier de résultats, sans modifier le checkpoint.

## Échecs et récupération

Le commit RocksDB et les fichiers externes ne constituent **pas** une transaction
atomique unique. Si le processus meurt ou si le stockage échoue entre les deux,
le journal peut avoir avancé sans que son nouvel engagement soit durablement
conservé. La CLI échoue alors sans réinitialiser le journal ni contourner son
contrôle d'engagement. Ne pas supprimer la base pour débloquer la clé.

Si `commitment.json` a été synchronisé mais l'écriture d'`interchange.json` échoue,
l'export peut être relancé avec ce nouvel engagement dans un dossier neuf.
Si le nouvel engagement n'a pas été conservé de façon fiable, une procédure de
récupération explicite et vérifiée reste nécessaire ; aucune commande de confiance
automatique dans l'état local n'est fournie. Les copies indépendantes de la clé,
les doubles bases et la restauration conjointe base/engagement restent hors des
protections locales.

Ces commandes sont mono-validateur et hors ligne. La coordination multi-clés,
la récupération de la fenêtre entre deux stockages, le gestionnaire de clés et
l'intégration aux tâches réelles restent des critères ouverts de production.

## Qualification

Campagne `build-cmake/qualification/beacon-journal-cli-20260925/` :

- `tests.log` valide trois suites. Le parcours CLI exécute 29 commandes
  (16 refus attendus) avec le binaire `nano` pour l'initialisation, l'import, le retrait,
  l'export répété et le transfert d'historique entre deux bases distinctes.
- Refus vérifiés : base absente/existante au mauvais endroit, ancien engagement,
  identité différente, verrou RocksDB détenu, entrées invalides ou trop grandes,
  liens sur les fichiers d'entrée/dossiers de sortie, sortie dans la base,
  écrasement de résultats et import dans un journal retiré.
- Le test C++ rouvre les bases et vérifie que le journal source ne peut plus
  atteindre un fournisseur de signature, et que la destination conserve les
  refus associés aux racines inconnues.
- Sur macOS, la bibliothèque de test injecte deux échecs de `fsync` dans le code
  exact de la commande : fichier d'engagement puis fichier d'interchange. Aucun
  reçu de fin n'est créé, le journal reste retiré et l'ancien engagement est
  refusé. Une réexportation fonctionne lorsque le nouvel engagement a déjà été
  synchronisé. Cette bibliothèque n'est pas liée au binaire de production.
- Les suites du journal et du corpus EIP-3076 épinglé sont rejouées séparément.

Ce sont des tests locaux de migration et de pannes d'E/S, pas une preuve de
résistance à une coupure électrique ni une activation de consensus en production.
