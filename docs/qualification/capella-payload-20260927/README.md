# Payload Capella et retraits SSZ

Le codec C++ Capella traite les trois listes variables extra-data,
transactions et retraits. Les offsets canoniques, limites de listes et
budgets de transport sont contrôlés avant décodage. Les transactions
restent opaques : ce contrôle structurel ne vaut pas admission EVM.
La racine SSZ des retraits est distincte de leur racine MPT d'exécution.
Le header construit engage la même racine SSZ que le payload complet.

Preuves nouvelles : 30 vecteurs officiels de payloads, roundtrip exact et
racines attendues concordantes ; 271 rejets locaux, 30 mutations de retrait
modifiant la racine et 60 annulations. Cas explicites de listes vides et de
transactions de longueur nulle, séparées des retraits. Presets et fichiers
amont vérifiés par empreintes ; révision dans result.json et les manifests.

Le décodeur est aussi relié au candidat de retraits SSZ déjà implémenté :
les 54 vecteurs process_withdrawals donnent 35 états complets postérieurs
identiques octet par octet et 19 refus. Cette qualification authentifie
le débit des balances et les curseurs, pas encore un bloc signé complet.

Régression finale : cinq suites (payload Bellatrix, payload Capella,
conteneurs Capella, retraits/exécution, changements BLS) réussies.

Référence : [Capella v1.4.0](https://github.com/ethereum/consensus-specs/blob/v1.4.0/specs/capella/beacon-chain.md).
Prochaine dépendance : brancher le payload reçu sur une transition unique
qui vérifie parent, timestamp, RANDAO, exécution Shanghai, retraits attendus
et header consensus. Puis composer slots/epochs et autres opérations avant
la publication atomique du bloc. Activation réseau et reprise durable
Capella restent ouvertes ; EIP-4895 reste partiel.
