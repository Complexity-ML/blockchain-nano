# Construction locale des blocs Altair avant signature

État au 25 septembre 2026 : `prepare_altair_unsigned_block` construit un
`BeaconBlock` SSZ non signé depuis un état parent authentifié et un corps candidat.
Le créneau, le proposant et le parent sont vérifiés par la transition ; la racine
d'état est calculée, jamais acceptée d'un appelant. L'état d'entrée reste intact.

La transition interne est partagée avec l'admission signée pour conserver les
mêmes contrôles : SSZ canonique, avancée slots/epochs, parent, proposant sélectionné
et non sanctionné, RANDAO signé, vote Eth1, ordre et validité des opérations,
SyncAggregate puis calcul de la racine finale. Le corps est borné avant copie.
Les budgets et callbacks d'annulation restent applicables.

Le chemin public `prepare_altair_state_transition` exige toujours la signature
BLS du bloc et la racine annoncée. Aucun paramètre public ne permet de désactiver
ces contrôles. La construction locale retourne seulement un candidat privé,
sans admission, publication, signature, persistance ou diffusion automatique.
Le mode interne de construction ignore uniquement la signature de bloc absente
et remplit sa racine d'état ; les signatures des opérations restent vérifiées.

Ce découpage suit la construction de la racine d'état puis la signature décrites
par la [spécification du validateur Ethereum](https://ethereum.github.io/consensus-specs/specs/phase0/validator/#packaging-into-a-signedbeaconblock).
La qualification ci-dessous reste épinglée au corpus Altair existant du dépôt.

## Qualification

Campagne `build-cmake/qualification/altair-unsigned-block-20260925/` :

- Corpus officiel de 52 cas, manifeste et condensés vérifiés : 37 cas valides,
  15 invalides, 105 transitions de blocs acceptées.
- Pour chaque bloc accepté, construction du message identique octet par octet
  au message officiel et état candidat identique à la transition signée.
- Pour les 12 cas invalides hors seule signature/racine d'enveloppe, le constructeur
  refuse aussi le candidat : proposant, parent, créneau et opérations incorrects.
- Les trois cas ayant seulement une mauvaise signature de bloc ou racine annoncée
  sont reconstructibles à partir de leur corps ; l'admission signée originale
  continue de les refuser. Construire n'est donc pas admettre une enveloppe reçue.
- Cinq mutations locales de proposant/parent/RANDAO/corps sont refusées ; un
  message construit avec signature zéro est refusé à l'admission réseau.
- Annulations au début, au milieu et à la fin : aucun changement de l'état parent.
- Régressions du choix de branche, réorganisations, snapshots, publication durable
  et tâche d'attestation dans `final-tests.log`.

## Intégration encore nécessaire

L'appelant fournit encore le corps et l'état parent authentifié. Le coordinateur
doit choisir sa tête, dériver la tâche du proposant, vérifier l'heure et le
journal dédié, réserver avant signature, puis admettre et diffuser le bloc signé.
La production des révélations RANDAO, la sélection des opérations en attente,
le planificateur et la liaison atomique avec l'exécution EVM restent à intégrer.
Ce composant ne crée pas un réseau autonome ni un profil Altair-EVM activable.
