# Changement de clés de retrait Capella

Opération `BLSToExecutionChange` et encodage SSZ signés ajoutés au cœur C++.
L'état Capella parent est authentifié ; la clé de retrait doit correspondre
aux credentials BLS existants. La signature utilise le domaine dédié, la
version genesis configurée et la racine genesis du registre. La clé active
de validation ne remplace pas cette autorisation. La transition privée
modifie uniquement les credentials ; une liste invalide, répétée ou annulée
ne publie aucune modification partielle.

14 vecteurs officiels épinglés : 7 acceptés et comparés octet par octet aux
états SSZ complets attendus, 7 rejetés. États actifs, non activés, en file
et sortis couverts par les cas amont. 63 rejets locaux supplémentaires,
7 rejeux et 14 annulations ; longueurs SSZ invalides et listes vides contrôlées.
Un contrôle négatif supprimant la vérification de signature échoue avec
`mutation accepted`. Source restaurée avant la régression finale : 3/3
(changements de clés, conteneurs Capella, couplage retraits/exécution).

Sources, empreintes et compteurs dans `result.json` ; corpus et empreintes
des presets dans `tests/fixtures/consensus-capella-bls-change-official`.
Référence : [consensus-specs Capella v1.4.0](https://github.com/ethereum/consensus-specs/blob/v1.4.0/specs/capella/beacon-chain.md#new-process_bls_to_execution_change).

Limite : opération isolée sur état authentifié. La composition complète du
bloc Capella doit traiter les retraits avant ces changements, vérifier les
autres opérations, gérer l'activation du fork et publier atomiquement l'état
consensus/exécution. Réacteur, P2P et migration réseau restent ouverts.
Le réseau Bellatrix n'est pas modifié et EIP-4895 reste partiel.
