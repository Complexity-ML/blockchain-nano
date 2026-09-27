# Corps de bloc Capella

La racine SSZ du corps Capella engage ses onze champs, dont le payload
avec retraits et la liste des changements BLS signés. Les offsets SSZ,
limites des listes, largeur des changements (172 octets) et budget total
sont vérifiés. Les fonctions Phase0/Altair/Bellatrix gardent leurs schémas.

30 corps officiels à la révision épinglée concordent avec leurs racines.
330 rejets locaux : offsets, troncature de changement, liste trop longue,
budget et confusion avec Bellatrix. 177 mutations de champs modifient la
racine (index, clé, destination, signature, hash du payload, retrait selon
présence dans les cas). 60 annulations propagées. Régression ciblée 5/5.
Les presets et les fixtures sont vérifiés par empreintes au lancement.

C'est une validation structurelle et cryptographique du commitment SSZ,
pas la vérification BLS des opérations ni l'exécution du bloc. Une signature
invalide peut être encodée canoniquement ; son rejet appartient à la
transition déjà dédiée à cette opération. Les étapes slots/epochs Capella,
composition du bloc signé, fork-choice, activation et stockage durable
restent à intégrer. Le profil réseau Bellatrix n'est pas changé.

Référence : [Capella BeaconBlockBody](https://github.com/ethereum/consensus-specs/blob/v1.4.0/specs/capella/beacon-chain.md#beaconblockbody).
