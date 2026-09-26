# Réception idempotente des attestations déjà authentifiées

Le réacteur reconnaît les octets SSZ exacts d’un vote présent dans son archive
validée et durable. Un doublon acquitte la livraison sans reconstruire le contexte
BLS, rejouer le choix de branche ni resérialiser l’état. L’archive est revalidée
à la réouverture et élaguée avec sa fenêtre d’admission. Le traitement normal des
nouveaux votes reste inchangé ; un vote différé reste appliqué au tick suivant.

Le test de chaîne signée ajoute 18 retransmissions, six signatures altérées,
l’annulation, l’identité du token durable et de l’archive, avant puis après
réouverture RocksDB. Le même test a été exécuté sur l’ancien code puis sur le
correctif. Aucune assertion de vitesse dépendante de la machine.

| Mesure du lot, signatures négatives incluses | Avant | Après |
| --- | ---: | ---: |
| Avant réouverture | 115109 µs | 5628 µs |
| Après réouverture | 139480 µs | 5728 µs |
| Callbacks de contrôle des 18 doublons, avant réouverture | 176274 | 126 |
| Callbacks de contrôle des 18 doublons, après réouverture | 336006 | 126 |

Ces mesures sur Mac sont indicatives, pas une qualification de débit réseau.
Le test existant conserve aussi les contrôles de réorganisation d’exécution,
blocs invalides, slashing, erreurs de synchronisation disque et réouverture.
Commande : construction de `nano-bellatrix-transition-tests` dans `build-release`,
puis exécution du binaire sans argument. Logs complets joints.

La campagne WAN `5da8484` reste échouée. Ce correctif doit être recompilé pour
les deux architectures puis qualifié aux mêmes seuils avant toute conclusion
sur la reprise distribuée. Aucun statut EIP ou production n’est changé.

Régressions ciblées : expiration d’attestation (5,01 s) et partition en
simulation à deux cœurs (107,76 s), toutes deux réussies. Voir `regression.log`
et `ctest-details.log`. La simulation utilise une horloge synthétique et une
livraison directe ; elle ne remplace pas la qualification P2P/WAN.
