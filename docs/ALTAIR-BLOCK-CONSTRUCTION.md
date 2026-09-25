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

## Signature liée au coordinateur durable

`AltairDurableReactor::sign_proposal` choisit la tête via le fork choice et exige
un créneau courant postérieur à cette tête. Il authentifie le parent, construit
le bloc et résout clé publique, domaine et genesis depuis l'état dérivé. Le
proposant fourni par l'appelant est vérifié par la sélection RANDAO du moteur ;
un index arbitraire ne suffit pas à obtenir une signature.

L'engagement attendu doit correspondre au coordinateur et au checkpoint disque.
Le journal doit correspondre à la chaîne, aux genesis, à la clé et aux paramètres
d'époque, et utiliser une base dédiée. Le verrou partagé du Store reste tenu
pendant la construction et la signature. Les callbacks doivent être bornés et
ne pas réentrer dans les coordinateurs.

Le journal réserve avant l'appel du fournisseur BLS. L'heure indépendante est
vérifiée avant/après construction, autour du fournisseur et après vérification
cryptographique ; un recul pendant la tentative, une expiration ou une annulation
finale retient la signature sans supprimer une réservation déjà écrite.
La politique globale des grands sauts d'horloge/redémarrages reste ouverte.

La méthode retourne un bloc signé, sans publier son état ni son archive. Il doit
passer l'admission normale. Un arrêt entre signature et admission garde la
protection anti-slashing mais n'offre pas encore de reprise automatique de la
diffusion ; la file persistante de sortie reste à implémenter.

Campagne `build-cmake/qualification/altair-proposer-signing-20260925/final-tests.log` :
le test `consensus_altair_attester_duty` comprend maintenant une proposition
construite avec le corps officiel du premier bloc du cas
`shorter_chain_but_heavier_weight`. La clé de test du proposant est dérivée et
comparée au registre ; les octets signés sont identiques au bloc officiel.
Un second coordinateur l'admet, l'archive et restaure les mêmes octets depuis le
checkpoint. Onze refus avant fournisseur couvrent engagement, créneau, heure,
index, RANDAO invalide, annulation, identité du journal, recul d'horloge, conflit
après réouverture, tête au créneau courant et coordinateur périmé. Le conflit
utilise un graffiti différent dont la construction est indépendamment valide,
puis exige le refus précis du journal. Une expiration au contrôle final retient
le bloc signé et conserve sa réservation.

## Intégration encore nécessaire

L'appelant fournit toujours le corps, dont la révélation peut maintenant être
obtenue via `sign_randao` ci-dessous. La sélection des opérations en attente,
le planificateur, la file de diffusion et la liaison atomique avec l'exécution
EVM restent à intégrer. La méthode de signature n'est pas exposée en RPC public.
Ces tests ne créent pas un réseau autonome ni un profil Altair-EVM activable.

## Révélation RANDAO locale

`AltairDurableReactor::sign_randao` dérive le proposant depuis la tête authentifiée
avancée au slot demandé. La sélection utilise le registre et les mixes RANDAO
de cet état avancé ; un proposant sanctionné est refusé sans nouveau tirage.
L'époque, le domaine RANDAO `0x02000000`, la version du fork et le genesis sont
déduits localement. L'appelant ne fournit pas de racine arbitraire à signer.

Le journal dédié vérifie son checkpoint et son statut de retrait sous son verrou,
puis vérifie la signature retournée par le fournisseur BLS. Ce message d'époque
n'est ni une proposition de bloc ni une attestation slashable : il ne modifie
pas leurs réservations et ne crée aucun faux créneau de proposition. La méthode
n'expose pas un nouveau mode de signature générique du journal.

Les contrôles de Store, clé, temps, recul d'horloge, expiration et annulation
restent applicables. Un backend qui divulgue lui-même une signature ou une clé
volée utilisée hors du logiciel ne peut pas être neutralisé par ces contrôles.
La révélation n'est ni publiée ni ajoutée à une file persistante automatiquement.

Campagne `build-cmake/qualification/altair-local-randao-20260925/final-tests.log` :
révélation de 96 octets identique au corpus, réinjectée dans le corps du bloc,
signature de proposition puis admission indépendante ; journal et Store inchangés
par la seule révélation. Sept refus avant fournisseur couvrent index, engagement,
créneau, heure, annulation, journal retiré et mauvaise identité de genesis.
Un fournisseur renvoyant une signature zéro est refusé ; une expiration finale
retient le résultat. Un scénario synthétique depuis l'ancre officielle avance
jusqu'à l'époque 1 : la signature vérifie pour cette époque et échoue pour
l'époque 0. Ces tâches locales ne constituent pas encore un validateur autonome.
