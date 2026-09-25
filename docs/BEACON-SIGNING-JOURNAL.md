# Journal de signature Beacon

État au 25 septembre 2026 : composant local implémenté et testé, sans activation
des tâches de validateur dans `nano run`. Il ne remplace pas le journal de votes
hauteur/tour du consensus BFT historique.

## Réservation avant signature

`BeaconSigningJournal` protège une clé publique BLS dans une base dédiée.
Le détenteur de la clé secrète fournit un callback de signature ; la classe ne
conserve ni n'écrit cette clé. Le caller doit vérifier les tâches du validateur,
l'état, le choix de branche, la version de fork et l'exécution avant d'appeler
le journal. Le callback ne doit pas réentrer dans un journal partageant la même base et doit borner
ses propres délais. Aucune méthode de signature publique RPC n'est ajoutée.

Pour une proposition, le journal construit la racine de signature du header
avec le domaine proposer. Pour une attestation, il utilise le domaine attester
et vérifie la cohérence slot/époque cible et source/cible. La racine des
validateurs au genesis fait partie des deux domaines.

Avant le callback, le journal réserve la racine via un checkpoint RocksDB
synchronisé avec comparaison de la séquence/digest attendus. Un échec de stockage
rend l'instance indisponible jusqu'à réouverture. Une signature produite par le
callback n'est retournée qu'après vérification BLS avec la clé publique liée.
Un callback qui échoue ou renvoie une signature invalide laisse sa réservation
intacte ; une tentative contradictoire ultérieure reste refusée.

## Politique minimale

- Proposition : slot strictement croissant ; seule la répétition exacte de la
  dernière racine au même slot est autorisée.
- Attestation : cible strictement croissante et source non décroissante ; seule
  la répétition exacte du dernier vote est autorisée à cible identique.

Ces gardes excluent les doubles votes et les votes qui s'entourent. Elles sont
conservatrices : un ancien message potentiellement sûr peut être refusé. Elles
ne constituent pas une pénalité/slashing sur les soldes et ne détectent pas des
signatures effectuées hors du journal.

La version de fork ne crée pas un nouvel espace de réservation : changer de
version ne permet donc pas de signer une autre racine au même slot/à la même
cible. La base est liée au chain ID, au genesis d'exécution, au genesis des
validateurs, à la clé publique et aux slots par époque. Elle contient uniquement
les deux dernières réservations et leurs identités ; le plafond interne de
4 Kio borne ce format fixe, pas le nombre de validateurs du réseau.

## Reprise et limites

Un nouveau journal requiert une base vide. Une base existante exige un engagement
séquence/digest conservé indépendamment et correspondant exactement au checkpoint.
Un second objet partageant cette base ne peut continuer depuis un état périmé.
L'appelant doit conserver les nouveaux engagements ; après un arrêt dans une
fenêtre où cet engagement externe n'a pas été mis à jour, la résolution reste
une procédure explicite, pas un redémarrage qui ignore le contrôle.

Cela ne protège pas contre la suppression du journal et un redémarrage volontaire
en mode initialisation, ni contre le rollback conjoint du disque et de son
engagement externe. Deux copies de la clé utilisées avec deux journaux distincts
ne sont pas coordonnées. Une clé volée peut être utilisée hors de Nano : ce
composant ne résout donc pas une compromission de quorum de type Ronin.
Le déploiement multi-clés, la migration et la gestion matérielle des clés restent
à intégrer et qualifier avant activation des validateurs.

## Qualification

`build-cmake/qualification/beacon-signing-journal-20260925/` contient :

- `journal-tests.log` : 18 refus avant callback, doublons exacts, mauvais domaine
  de fork au même slot, votes contradictoires, réouverture, mauvais engagement,
  mauvaise identité, instance périmée, concurrence, signature invalide du backend,
  échec de synchronisation WAL injecté et `SIGKILL` après réservation avant signature.
- `crypto-tests.log` : régressions des fixtures officielles BLS et racines de signature.

Une panne de processus n'est pas une coupure électrique du stockage. Ces tests
ne démontrent ni une finalité distribuée ni l'autorisation d'une tâche de validation.

## Interopérabilité EIP-3076

L'[EIP-3076](https://eips.ethereum.org/EIPS/eip-3076) définit l'échange d'historique
anti-slashing entre clients (format version 5). Les API C++
`retire_and_export_interchange()` et `import_interchange()` utilisent la version 5.
Le parcours opérationnel multi-clés, la CLI et l'intégration aux tâches réelles
de validateur restent à réaliser. Le format local du journal n'est pas un fichier EIP-3076.
Les règles de slashing sont définies par la
[spécification Beacon](https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/).

## Export avec arrêt durable du journal

L'API C++ exporte une seule clé, les deux dernières réservations et leurs racines,
avec des slots/époques sous forme de chaînes décimales. Une réservation dont le
backend n'a pas retourné de signature reste incluse par précaution. Un historique
vide produit les deux tableaux vides. Aucun secret n'est exporté.

Avant de retourner le JSON, la méthode écrit et synchronise un checkpoint local
v2 marqué `retired` (v3 si une racine est inconnue). Cette base ne peut plus signer, même après réouverture avec
son nouvel engagement externe. Les exports suivants restent possibles et ne
créent pas de nouveau checkpoint. Les journaux actifs v1 restent lisibles ; un
ancien binaire ne comprend pas v2 et refuse de le charger. Aucune reprise de
signature depuis une base retirée n'est proposée.

L'appelant doit arrêter les autres utilisateurs de la clé avant la migration.
Un mutex associé au `ChainStore` sérialise signatures, import et export entre
tous les objets partageant cette base. L'export attend donc aussi les callbacks
déjà en cours sur ces autres objets. Les engagements refusent ensuite les objets
périmés. Cette protection ne coordonne pas un backend utilisé hors de Nano ni
une copie indépendante de la clé/base ; l'arrêt opérationnel reste nécessaire.
Le JSON retourné et le nouvel engagement doivent être conservés par l'appelant.
Une erreur d'écriture ne retourne aucun export et rend l'instance indisponible.
Une erreur de sauvegarde du JSON après retour permet de réexporter la base retirée.

Campagne `build-cmake/qualification/beacon-signing-export-20260925/` :
`journal-tests.log` vérifie le contenu des réservations, l'historique vide,
l'idempotence, les refus après export/réouverture, une instance périmée et un
échec de synchronisation WAL à l'export, en plus des régressions du journal.
Cette campagne historique valide l'export uniquement. La campagne d'import
ci-dessous complète la qualification locale ; le parcours opérationnel reste ouvert.

## Import borné et fusion conservatrice

`import_interchange(json, max_bytes)` valide entièrement le document avant toute
écriture et applique uniquement les entrées correspondant à la clé du journal.
La valeur retournée compte ces entrées, y compris les doublons ; zéro indique
qu'aucune clé correspondante n'a été trouvée. Les autres entrées sont validées,
mais ne sont pas importées dans ce journal. L'opération est atomique par journal,
pas entre plusieurs clés. Elle ne réactive jamais une base retirée.

La version doit être 5 et la racine des validateurs au genesis doit correspondre.
Les quantités sont des chaînes décimales uint64. Les racines manquantes deviennent
inconnues : elles ne sont pas assimilées à une racine nulle et sont omises lors
du prochain export. Une racine explicitement JSON `null` dans l'interchange est
refusée. Champs obligatoires, tailles hexadécimales, source <= cible, débordements,
clés JSON dupliquées et imbrication supérieure à 16 sont contrôlés. La limite
d'octets est fournie explicitement par l'appelant, avant parsing ; le DOM utilise
une mémoire proportionnelle au document. Ce n'est pas une API RPC publique.

La fusion ne diminue jamais les maxima slot/source/cible. Des entrées contradictoires
à une même hauteur effacent la connaissance de la racine. Si les maxima source
et cible proviennent de messages différents, leur combinaison est une protection
synthétique sans racine, jamais un vote que le journal prétend avoir signé.
Une racine connue réimportée ne remplace pas une racine devenue inconnue au même
maximum. Le format local v3 conserve cet état après réouverture ; les anciens
formats v1/v2 restent lisibles, et un ancien binaire refuse les snapshots v3.

L'appelant doit arrêter les autres utilisateurs de la clé avant import/export,
conserver les engagements externes et ne pas réutiliser une ancienne copie de
la base. Aucune fusion ne détecte des signatures faites hors du journal.

## Corpus officiel et reproduction

Source : [tests inter-clients EIP-3076](https://github.com/eth-clients/slashing-protection-interchange-tests),
révision `bb7900688eecd287d8723154faac6addb5e63336`.
`tests/fixtures/slashing-interchange-manifest.json` épingle les SHA-256 des
38 fichiers. Les fichiers upstream sont lus sans modification depuis un checkout
externe ; aucun corpus n'est téléchargé implicitement par CMake.

```sh
git clone https://github.com/eth-clients/slashing-protection-interchange-tests.git /tmp/nano-slashing-interchange-tests
git -C /tmp/nano-slashing-interchange-tests checkout bb7900688eecd287d8723154faac6addb5e63336
cmake -S . -B build-release -DNANO_SLASHING_INTERCHANGE_TESTS_DIR=/tmp/nano-slashing-interchange-tests
cmake --build build-release --target nano-beacon-signer-tests nano-beacon-interchange-tests
ctest --test-dir build-release -R '^consensus_beacon_(signing_journal|interchange_official)$' --output-on-failure
```

Sans le chemin configuré, le test externe n'est pas enregistré dans CTest ; cela
ne doit pas être interprété comme un succès du corpus. Un fichier absent ou dont
le SHA-256 diffère fait échouer le runner.

Campagne `build-cmake/qualification/beacon-signing-import-20260925/final-tests.log` :

- Journal natif : 51 refus avant callback, régression des signatures BLS,
  import d'un export réel, racines absentes, rechargement, fusion des doublons,
  absence de mutations partielles, entrées malformées, export/import des limites
  synthétiques et échec de synchronisation WAL injecté à l'import. Un callback volontairement
  bloqué prouve qu'un export demandé par un second objet attend sa fin, puis
  empêche toute nouvelle signature depuis les deux objets.
- Corpus : 38 cas, 65 imports par clé, 146 réservations (38 acceptées et 108
  refusées), 211 rechargements du journal. Quatre cas avec source supérieure à
  cible sont refusés à l'import ; leurs étapes suivantes ne sont pas exécutées,
  conformément à l'option de rejet des données slashables prévue par le README.
- Le runner utilise les fonctions privées de réservation du vrai journal, sans
  produire de signatures : les fixtures fournissent des racines, pas des clés
  privées. La signature/domaines sont testés dans la suite native séparée.
- Nano emploie la stratégie minimale avec répétition exacte du dernier message.
  Le runner exige toutes les acceptations du champ `should_succeed` et tous les
  refus du champ `should_succeed_complete`. Les deux acceptations supplémentaires
  sont des répétitions exactes sans mutation du checkpoint, autorisées par les
  conditions de l'EIP. Ce n'est pas une revendication de base historique complète.

Ces preuves ne constituent pas une migration entre deux logiciels en exploitation,
une validation de réseau distribué ou une autorisation de fonds réels.
