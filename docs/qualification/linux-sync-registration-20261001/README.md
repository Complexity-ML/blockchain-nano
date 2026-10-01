# Raccordement des injections disque Linux — qualification isolée

Checkout /tmp/nano-rpc-positional-worktree-20261001, base 886eac8320df32ddca059127e4972a8b37cefb7f, patch exact archivé.
Builder ARM64 6116669430b02b7e8012af8c8bd515a5a7dcd1dd2034649f9b45ae5b9e70a3a4,
volume existant nano-linux-sync-build, build distinct rpc-positional-client-20261001.
La campagne complète 400 reste séparée et ses binaires ne sont pas modifiés.

CMake enregistre 413 scénarios : treize ajouts exacts, aucun retrait. Les
nouveaux tests couvrent les WAL de réservation/commit/progression et leurs
suffixes conservés/perdus, refus de signature et de retry après erreur,
checkpoints, propriétaire d'exécution Capella, témoin et élagage. Trois noms
existants exercent en plus les branches EIO Altair/Beacon/Bellatrix.
Les assertions exigent une erreur réellement observée par l'injecteur ; les
hooks sont liés seulement aux fixtures, jamais à nano.

Première exécution : 15/16 passent, 16.08 s, CTest exit 8. L'élagage échoue
avant son injection avec ENOSPC sur le tmpfs /tmp limité à 128 Mio.
Cet échec n'est pas effacé. Rejeu de ce seul nom lancé, mêmes binaires et
délai 240 s, TMPDIR sur un sous-répertoire du volume disque existant ;
résultat encore en cours lors de cette première archive.

Les sorties de deux anciennes fixtures portent encore les labels macOS ;
le builder, registre et recipe établissent que cette exécution est Linux.
Ces injections et les pertes de suffixe sont synthétiques : aucune panne
physique ni finalité WAN ni release globale n'est qualifiée par cette tranche.

## Résultat terminal du rejeu

CTest exit 0, un scénario réussi en 63.11 s, sans compilation supplémentaire.
Les seize noms sélectionnés passent à travers deux exécutions (15 puis 1),
ce qui inclut les treize ajouts et trois branches EIO supplémentaires.
Le guard vérifie l'union exacte, les délais positifs et l'absence de propriétés
skip. Les quatre combinaisons faultWitness/lostSuffix sont présentes ; votes
signés préservés, retry idempotent et reprise propriétaire/RPC vérifiés.
JUnit tronque stdout à 1024 octets : LastTest.log intégral est aussi archivé.
Le premier guard, limité à JUnit, ne pouvait pas retrouver le quatrième cas ;
la vérification depuis LastTest.log réussit, sans rejouer les tests.
Répertoire TMPDIR sur volume existant : 828 Gio disponibles au démarrage,
nettoyage par les fixtures ; mêmes limites CPU/RAM et délai 240 s.
Correctif enregistré dans le checkout isolé au commit 97f4a19d, non fusionné
à main pendant la régression complète, non déployé sur le VPS.
