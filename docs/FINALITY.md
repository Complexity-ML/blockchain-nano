# Finalité BFT : intégration et qualification

## État actuel au 23 septembre 2026

Le mode `bftVersion: 1` est intégré au cœur Nano. La configuration de nœud
`bft` instancie `BftReactor` avec une clé et un journal persistants.
L’import vérifie le certificat contre l’ensemble issu du parent, exécute le
bloc, puis stocke atomiquement le bloc, l’état, les reçus et le certificat.
La progression du journal suit le commit durable ; une panne entre ces étapes
exige la réconciliation du journal avant de nouveaux votes.
Un import certifié reçu du réseau peut donc laisser temporairement le journal
en retard, notamment si le nœud s'arrête avant de construire son prochain tour.
À la création du tour, `recover_voting_progress` vérifie l'historique certifié et
rejoue sa progression dans le journal avant tout nouveau vote. Il conserve les
réservations existantes et refuse un journal en avance sur la chaîne ; il ne
réinitialise pas arbitrairement sa hauteur. Une vérification après arrêt doit
contrôler cette récupération, et pas seulement comparer les hauteurs brutes.

Les scénarios locaux couvrent notamment les quorums pondérés, les changements
de stake, les partitions et les reprises propres ou par SIGKILL. Leurs preuves
et limites sont conservées dans `QUALIFICATION-FAILURES.md` et le manifeste de
qualification. Ils ne valent pas qualification complète multi-hôtes ni
approbation d’un lancement public. Les tags RPC `safe` et `finalized` restent
indisponibles ; une pénalité automatique de slashing n’est pas implémentée.

Ce document conserve ci-dessous les étapes historiques du développement.
Les formulations « reste à intégrer » de ces étapes décrivent leur état à
l’époque, et doivent être lues avec les sections d’intégration ultérieures.

## Étape historique : primitives avant intégration

`finality.hpp` et `finality.cpp` ont d’abord fourni les votes signés et le
vérificateur de certificat pondéré, avant leur intégration au réacteur actif.

La base de travail est la [spécification de consensus CometBFT v0.38.17](https://github.com/cometbft/cometbft/blob/v0.38.17/spec/consensus/consensus.md).
Elle utilise des tours avec proposition, prévote et préengagement, des verrous
et des changements de tour. Un certificat comporte plus de deux tiers du poids
de préengagements. La sûreté du protocole dépend aussi des règles de verrou et
des hypothèses sur le poids byzantin ; vérifier un quorum ne remplace pas ces
règles. Nano n’est pas compatible avec le format réseau CometBFT.

## Format Nano expérimental

Le hash d’un ensemble est Keccak-256 d’une liste RLP : domaine ASCII
`Nano/BFT/Validators`, version 1, puis liste des couples adresse/pouvoir triés
par adresse normalisée. L’ensemble comporte 1 à 1000 membres uniques, adresses
non nulles et poids entiers strictement positifs inférieurs à 2^256.

Le digest de vote est Keccak-256 de la liste RLP :

1. domaine ASCII `Nano/BFT/Vote` ;
2. version 1 ;
3. chain ID positif inférieur à 2^256 ;
4. hash du genesis, 32 octets ;
5. hauteur positive, entier uint64 ;
6. tour, entier uint64 ;
7. étape : 1 pour prévote, 2 pour préengagement ;
8. hash de l’ensemble, 32 octets ;
9. hash de bloc de 32 octets, ou octets vides pour un vote nil ;
10. adresse du signataire, 20 octets.

La preuve contient r(32), s(32), parité(1). Les signatures à s haut sont refusées.
La routine de signature est une primitive : `sign_vote_durable` ajoute une réservation persistante ; son appelant doit
encore garantir les règles et la persistance des verrous de consensus. Aucune clé réelle ne doit être
branchée sur une boucle de votes tant que ce journal n’est pas intégré.

Le certificat ne contient que des préengagements pour un même bloc non nil,
une hauteur, un tour et un ensemble attendus par l’appelant. Chaque signataire
est unique et appartient à cet ensemble. Le quorum est calculé avec des entiers
exacts : `3 * poids_signé > 2 * poids_total`. Un poids exactement égal à deux
tiers échoue. Les pouvoirs annoncés par les votes ne sont jamais utilisés :
ils proviennent uniquement de l’ensemble fourni au vérificateur.

## Liste historique d’intégration avant activation

- Fixer l’ensemble de votes à partir de l’état parent vérifié, avec une règle
  versionnée d’entrée/sortie et de conservation des dépôts.
- Persister les votes par hauteur/tour/étape et les verrous avant toute signature ;
  éprouver les reprises et empêcher des votes incompatibles après redémarrage.
- Implémenter les transitions de tours, le choix du proposant, les délais et les
  justifications nécessaires aux changements de verrou.
- Vérifier les blocs avant vote, transporter les votes avec limites de ressources
  et conserver les certificats avec les commits atomiques de blocs.
- Définir la synchronisation, le refus des branches incompatibles avec une
  hauteur finalisée, les preuves d’équivoque et les sanctions du staking.
- Tester les partitions, pertes/réordonnancements, producteurs indisponibles et
  fautes byzantines, puis obtenir une revue indépendante avant activation.

La suite `finality` couvre la séparation des domaines, les altérations des
champs, les votes nil, les doublons, les signataires inconnus, le seuil strict,
les poids très grands et les changements d’ensemble. Elle vérifie ces primitives,
pas la sûreté ni la vivacité d’un protocole complet.

## Journal anti-double-vote

`sign_vote_durable` vérifie d’abord la correspondance clé/adresse, puis réserve
le digest avant de produire la signature. `ChainStore::reserve_vote` utilise
un espace distinct `voting/v1`, séparé par chaîne, genesis et signataire. La clé
contient hauteur, tour et étape ; elle ne contient pas le hash de l’ensemble
de validateurs, afin qu’un changement d’ensemble ne permette pas de voter deux
fois au même emplacement. Ce hash reste lié au digest signé.

Une WriteBatch synchrone écrit ensemble la réservation et la dernière position
atteinte. Un vote différent au même emplacement est refusé. Un nouvel
emplacement antérieur à la dernière position est refusé. Une répétition exacte
d’une réservation passée reste autorisée, sans faire reculer cette position.
Le journal vérifie que sa dernière position référence bien un digest enregistré.
Les clés de votes sont distinctes des clés de chaîne supprimées par `rewind`.

La suite teste fermeture après réservation avant signature, reprise identique,
conflits de bloc/nil/ensemble, recul de tour ou d’étape, changement de hauteur
et séparation des chaînes. Ce journal ne valide pas une proposition, ne prouve
pas un quorum et n’empêche pas à lui seul des votes incompatibles entre tours.
Les verrous et justifications sont traités par la réservation verrouillée décrite ci-dessous. Comme pour
les signatures de blocs, copier une clé vers des journaux indépendants ou
restaurer une ancienne copie du journal annule cette protection locale.

## Transitions de verrou vérifiées

`advance_vote_lock` est une fonction pure : elle retourne le prochain verrou
ou refuse la transition. L’état porte une hauteur, un hash d’ensemble et,
lorsqu’il est verrouillé, un tour et un hash de bloc. Les ensembles et hauteurs
incompatibles, les signataires hors ensemble et les verrous futurs sont refusés.

- Avant un prévote, un quorum pour une autre cible (bloc ou nil), à un tour
  strictement postérieur au verrou et strictement antérieur au nouveau tour,
  libère le verrou. Le validateur peut alors choisir la proposition courante
  vérifiée, même si elle diffère de la cible du quorum. Sans déverrouillage,
  il doit conserver le bloc verrouillé comme cible du prévote.
- Un préengagement de bloc exige un quorum de prévotes du tour courant et
  enregistre le verrou sur ce bloc à ce tour.
- Un préengagement nil sans quorum conserve le verrou. Un quorum nil du tour
  courant le libère.

Toutes les justifications fournies sont contrôlées avec les signatures et le
quorum pondéré strict, pour la même chaîne, hauteur et ensemble. Les tests
couvrent changements de cible, preuves anciennes ou insuffisantes, relock,
quorums nil et conservation du verrou sans preuve.

Cette fonction est reliée atomiquement au journal par `sign_consensus_vote`,
décrit ci-dessous. Son appelant futur devra encore vérifier l’exécution des
blocs et appliquer les règles de progression de hauteur. Les délais, les propositions et le transport restent à intégrer.
Aucune activation de consensus n’est effectuée par ces tests.

## Verrous durables et signature

`sign_consensus_vote` vérifie la clé, appelle `reserve_consensus_vote`, puis
signe. Sous le même mutex, le journal charge le verrou de la hauteur, vérifie
la transition et prépare une seule WriteBatch synchrone contenant le digest,
la dernière position et le nouveau verrou. Si la transition ou l’écriture
échoue, aucune nouvelle signature n’est produite. Une reprise exacte renvoie
la même signature sans appliquer de nouveau une transition déjà persistée.

Le mode `locked` est fixé pour le domaine chaîne/genesis/signataire. Les appels
à la réservation simple `raw` sont alors refusés. Un domaine ayant déjà des
réservations simples ne peut pas passer silencieusement au mode verrouillé,
car ses décisions passées ne permettent pas de reconstituer des verrous sûrs.
Ce refus ne doit pas être contourné en supprimant le journal.

Les tests ferment le journal après réservation avant signature, le rouvrent,
vérifient la reprise exacte, refusent un prévote incompatible, empêchent le
contournement par l’API simple, conservent le verrou après un préengagement nil
sans quorum, puis autorisent un déverrouillage avec une justification récente.
Le consensus actif n’appelle toujours pas ces primitives. La sélection du
proposant, les délais, la validation des blocs avant vote, les échanges réseau et le commit canonique certifié restent à intégrer.

## Progression certifiée des hauteurs de vote

Un journal verrouillé neuf commence à la hauteur 1. Les réservations nouvelles
sont limitées à sa hauteur active et au hash d’ensemble fixé pour cette hauteur.
`advance_voting_height` exige un certificat valide de préengagements pour le
bloc de la hauteur courante. Il écrit atomiquement le certificat, les poids
et signatures qui le justifient, et l’autorisation de voter à la hauteur
suivante avec le hash du prochain ensemble. Un observateur peut démarrer ce
journal en enregistrant le certificat de hauteur 1 sans avoir voté lui-même.

Les certificats doivent se suivre ; un quorum insuffisant, un saut de hauteur
ou un ensemble courant différent sont refusés. Répéter un certificat déjà
enregistré pour le même bloc et les mêmes ensembles est idempotent. Un autre
bloc ou un autre prochain ensemble à cette hauteur est refusé. Les répétitions
exactes de votes anciennement réservés restent possibles sans reculer l’état.

L’appelant doit fournir l’ensemble initial depuis le genesis vérifié et calculer
le prochain ensemble depuis l’exécution du bloc certifié. Le journal ne reçoit
que le hash du bloc : il ne peut pas prouver lui-même cette relation avec l’état.
Cette responsabilité doit être intégrée au moteur avant activation. La
progression du journal n’est donc pas une publication de finalité de la chaîne
canonique, ni une modification des réponses RPC de finalité.

Les tests couvrent vote prématuré à la hauteur suivante, quorum insuffisant,
réouverture avec nouveaux poids, répétition du certificat et refus de modifier
après coup l’ensemble suivant. Les anciens journaux expérimentaux verrouillés
sans métadonnée de hauteur active ne sont pas convertis automatiquement.

## Lien avec l’exécution locale

`Blockchain::certify_voting_progress` relie désormais les primitives au moteur
de vérification, sans les activer sur le réseau. Sous le verrou de chaîne, il
construit l’ensemble courant depuis le registre de l’état parent, authentifie
le producteur et vérifie le certificat pour cet ensemble. Il vérifie ensuite
le contenu et rejoue le bloc sur une copie d’état. Le registre issu de cette
exécution détermine le prochain ensemble enregistré dans le journal.

Pendant l’amorçage sans registre, l’ensemble expérimental contient uniquement
`bootstrapValidator`, avec un poids unitaire. Après déploiement du staking,
les membres actifs prennent le poids de leur dépôt dans le registre. Ce poids
unitaire ne représente pas un dépôt et ne crée pas de solde. Il s’agit d’un
amorçage centralisé, pas d’un réseau BFT déjà distribué.

Le test signé de staking fournit des certificats du parent pour le déploiement,
l’inscription d’une seconde clé et huit blocs suivants. Il refuse un bloc dont
la racine d’état est fausse malgré des signatures et un quorum valides, ainsi
que l’emploi anticipé des poids résultant du déploiement. Le journal ne progresse
qu’après le rejeu réussi.

Cette méthode écrit dans le journal de votes mais ne publie pas le bloc dans
la chaîne canonique. Le commit canonique dispose maintenant de sa propre transaction atomique,
décrite ci-dessous ; son couplage au journal de votes séparé reste à intégrer. Les appels bas niveau du journal restent des
primitives ; le consensus intégré devra passer par le moteur vérifié, appliquer
les tours/délais et transporter les preuves. Les RPC de finalité restent inactifs.

## Validation avant signature de vote

`Blockchain::vote_for_block` fournit une entrée pour les votes non nil. Sous
le verrou de chaîne, il déduit l’ensemble depuis l’état parent, vérifie que la
clé appartient à cet ensemble, authentifie le producteur et rejoue le bloc.
Il vérifie aussi que l’état résultant donne un prochain ensemble valide. Ce
n’est qu’ensuite qu’il construit le vote et appelle la réservation verrouillée
et durable, avant signature. L’appelant ne choisit ni la hauteur, ni le hash du
bloc, ni les poids du vote. Un préengagement exige toujours sa justification
par quorum de prévotes ; un bloc valide ne dispense pas des règles de verrou.

Les tests refusent une racine d’état fausse avec signature de producteur valide,
un producteur sans signature, une clé non encore inscrite et un préengagement
sans quorum. Une proposition valide reste votable après ces refus. La fermeture
et réouverture du journal conserve la signature exacte du prévote. Les dix
blocs du scénario signé de staking passent par cette entrée pour les votes du
premier validateur, avec certificats synthétiques pour les autres signatures.

Cette API ne signe pas de votes nil et n’est pas appelée par une boucle réseau.
La progression des tours, les délais et la coordination entre journal et chaîne
restent nécessaires avant une activation. Le journal et la chaîne restent deux bases
séparées dans ces tests.

## Commit canonique certifié expérimental

`Blockchain::import_certified_block` authentifie le producteur, vérifie le
quorum à partir des validateurs de l’état parent, rejoue le bloc et calcule le
prochain ensemble. `ChainStore::commit_certified` écrit dans une seule
WriteBatch synchrone le bloc, les comptes, les reçus, les index, le registre,
le certificat et la hauteur certifiée. L’état en mémoire n’avance qu’après
succès de cette écriture. Les préengagements et les deux ensembles sont
conservés pour pouvoir revérifier la preuve.

Ce chemin expérimental commence à la hauteur 1 et exige ensuite un certificat
à chaque hauteur. Le prochain ensemble du certificat parent doit correspondre
à l’ensemble courant. Après le premier commit certifié, un import ordinaire
sans certificat est refusé. `rewind` refuse une cible inférieure à la hauteur
certifiée. Aucune conversion implicite d’une chaîne existante n’est proposée.

La lecture d’un certificat vérifie son lien au bloc canonique et ses signatures
dans le domaine de la chaîne. La réouverture vérifie le certificat du sommet.
L’audit complet rejoue les certificats depuis le genesis et compare les deux
ensembles avec les registres issus de l’exécution ; la vérification du seul
sommet ne remplace pas cet audit historique.

Le scénario de staking importe dix blocs certifiés, refuse une exécution
incorrecte et un quorum absent sans écrire de bloc ni certificat, refuse
l’import ordinaire et le retour arrière, rouvre et audite l’historique, puis
vérifie le refus d’une signature de certificat corrompue au sommet. Cela teste
les écritures et la réouverture, pas une coupure d’alimentation réelle.

Le journal de votes externe reste distinct de la base canonique. Sa progression
et ce commit sont deux opérations : leur reprise coordonnée, les tours et le
transport des certificats restent à intégrer. Les genesis livrés et les RPC
`safe`/`finalized` ne sont pas activés par cette API.

## Rattrapage du journal après commit canonique

`recover_voting_progress` travaille sous le verrou de chaîne. Il refuse un
journal dont la hauteur active dépasse le prochain bloc canonique. Avant toute
progression, il audite intégralement l’exécution et les certificats depuis le
genesis et exige un certificat pour chaque bloc non genesis. Il réapplique
ensuite les certificats au journal par l’opération idempotente existante, puis
vérifie la hauteur active et le hash d’ensemble contre l’état canonique courant.
Un certificat contradictoire déjà présent dans le journal est refusé.

Le chemin de reprise prévu est de conserver le journal existant, enregistrer le
bloc certifié dans la base canonique, puis rattraper le journal si nécessaire.
Une interruption pendant le rattrapage laisse un préfixe durable qui peut être
repris. Cette méthode réalise actuellement un audit complet ; elle convient à
une reprise, pas à une boucle par bloc à grande échelle. Elle ne remplace pas
une clé ou un journal perdu, et ne répare pas un journal en avance sur la chaîne.

Les essais rouvrent une chaîne de dix blocs certifiés avec un journal resté à
la première hauteur : le vote suivant est refusé avant rattrapage, puis autorisé
après audit. La répétition et une seconde réouverture restent idempotentes. Un
nouveau validateur peut aussi suivre l’historique avant son premier vote. Les
journaux en avance, les certificats contradictoires et les historiques non
certifiés sont refusés. Ces essais modélisent la frontière entre deux écritures
et ferment les bases ; ils ne simulent pas une coupure électrique.

L’intégration automatique de cette reprise au futur moteur de tours reste à
faire. Aucun vote réseau ni tag RPC de finalité n’est activé.

## Arrêts brutaux du processus

La suite CTest `finality_crash` lance un programme C++ isolé avec une clé de
fixture et deux bases temporaires. Le superviseur C++ attend l’accusé de
chaque phase durable, vérifie que le processus est encore vivant, puis lui
adresse SIGKILL et exige un code de sortie correspondant à ce signal. Les
instances RocksDB sont encore ouvertes à chacune des trois interruptions :

1. après réservation et signature du préengagement, avant commit canonique ;
2. après commit canonique du premier bloc, avant progression du journal ;
3. après rattrapage audité du journal, avant le vote suivant.

Chaque phase suivante est un nouveau processus. Le test compare exactement
la signature et le hash du premier bloc entre les deux premières phases,
vérifie les hauteurs distinctes de la chaîne et du journal, puis certifie un
second bloc et audite les deux transferts, les reçus et les certificats. Les
arrêts ne déclenchent donc pas les destructeurs C++ ni une fermeture propre de
RocksDB. Le superviseur nettoie ses propres processus et répertoires temporaires.

Cette preuve couvre ces frontières applicatives précises, avec un validateur
d’amorçage. Elle ne couvre ni une coupure électrique, ni une interruption au
milieu d’une écriture interne de RocksDB, ni un réseau BFT distribué.

## Enveloppe de vote bornée

`encode_vote_message` et `decode_vote_message` définissent une enveloppe JSON
plate `nano/bft/vote`, version 1. Elle contient chain ID, hash de genesis,
hauteur, tour, étape (`prevote` ou `precommit`), hash d’ensemble, hash de bloc
(ou `null` pour nil), adresse et signature. Les quantités sont des chaînes
hexadécimales canoniques, les octets ont une largeur fixe et un encodage
hexadécimal minuscule. Les hauteurs et tours sont limités à uint64.

La limite de 2048 octets est vérifiée avant l’analyse JSON. Le parseur refuse
les structures imbriquées, les tableaux, les clés dupliquées, les champs
manquants ou supplémentaires et les versions inconnues. L’appelant fournit
la chaîne, le genesis, la hauteur, le tour et l’ensemble du parent attendus.
Les contrôles de contexte et d’appartenance précèdent la vérification de la
signature. Aucun poids fourni par le message n’est accepté.

La suite `finality` couvre l’aller-retour du digest signé, les messages nil,
la borne uint64, les champs manquants, doublons, types incorrects, quantités
non canoniques, domaines étrangers, membres inconnus, signatures invalides,
structures imbriquées et dépassement de taille. Le codec ne constitue pas
encore un transport : il reste à le brancher au moteur de tours et à cpp-libp2p,
avec déduplication et limitation du débit par pair. Il n’accepte actuellement
que le tour attendu par l’appelant ; les règles de rattrapage de tours restent
à définir dans le moteur.

## Collecte des votes d’un tour

`RoundVoteCollector` est lié à une hauteur, un tour, une chaîne et un ensemble
vérifié. Son entrée passe par le décodeur borné. Chaque validateur possède au
plus une entrée comptée par étape. Relayer plusieurs fois le même vote n’ajoute
aucun poids. Un vote signé contradictoire conserve la première entrée comptée
et une paire de signatures comme preuve d’équivoque ; d’autres contradictions
n’allouent pas de nouvelles entrées pour ce signataire et cette étape.

La mémoire conservée est bornée à quatre votes par validateur : original et
contradiction pour chacune des deux étapes. Les quorums utilisent les poids
entiers exacts et le seuil strict de plus de deux tiers. La participation
compte les signataires indépendamment de leur cible, pour de futurs délais de
tour ; elle ne certifie pas un bloc. Un certificat exige des votes pour la même
cible. Un quorum de prévotes nil peut justifier un changement de verrou ; aucun
certificat de bloc n’est produit à partir de préengagements nil.

Les certificats sont triés par signataire, indépendamment de l’ordre de réception.
Les tests couvrent répétitions, contradictions multiples, preuves vérifiables,
cibles réparties, poids très grands et refus d’un autre tour sans mutation.
Le test d’arrêts brutaux passe maintenant ses votes par encodage, décodage et
collecte avant de signer le préengagement et d’importer le certificat.

Cette classe est destinée à une boucle d’événements sérialisée et n’est pas
thread-safe. Les preuves sont uniquement conservées en mémoire pour ce tour ;
leur publication, persistance et traitement par le staking restent à intégrer.
La limitation du débit et la durée de conservation des tours relèveront du
moteur réseau, qui n’est pas encore activé.

## Métadonnées de certification incomplètes

Les lectures du sommet et des certificats, les commits et le retour arrière
contrôlent la cohérence du marqueur de hauteur certifiée avec le sommet et la
présence de sa preuve. Si le marqueur manque mais que l’index contient encore
un certificat canonique, le stockage refuse l’opération. Il ne rétrograde pas
silencieusement la base vers les imports ordinaires. La recherche utilise un
positionnement dans l’index de certificats, sans parcourir les blocs.

Le test supprime séparément le marqueur certifié, le sommet canonique et le
certificat du sommet. Pour chaque défaut, les lectures et le retour arrière
échouent, et la chaîne refuse de rouvrir. Après restauration des valeurs exactes
de fixture, un audit vérifie que les opérations refusées n’ont pas effacé de
blocs. Ce contrôle détecte ces incohérences ; il ne protège pas contre une
réécriture cohérente de toute la base par un acteur ayant accès au disque.

## Votes nil émis par le moteur

`Blockchain::vote_nil` construit un vote sans hash de bloc pour la hauteur
suivant le sommet canonique. Il dérive l’ensemble depuis l’état parent et
vérifie l’appartenance de la clé avant la réservation durable et la signature.
L’appelant fournit le tour, l’étape et une éventuelle justification ; il ne
fournit pas de hauteur ou d’ensemble arbitraire. Les contrôles de progression
et de verrou du journal s’appliquent aussi à ces votes.

Un préengagement nil sans quorum conserve le verrou existant. Un prévote nil
qui contredirait un verrou reste refusé sans la justification exigée par les
règles de transition. Un vote nil ne certifie aucun bloc et ne fait pas avancer
la hauteur active. Les tests vérifient la signature identique après réouverture,
le refus d’un non-membre, la conservation du verrou, la possibilité de voter
encore pour le bloc verrouillé et le refus de sauter une hauteur du journal.

La décision de voter nil et les échéances qui la déclenchent doivent encore
être pilotées par le moteur de tours. Cette API seule ne constitue pas cette
machine d’états et n’émet aucun message réseau.

La transition de prévote suit la distinction de la spécification CometBFT entre
preuve qui libère le verrou et proposition courante. Les tests couvrent une
preuve pour B libérant un verrou sur A avant un prévote pour C, une preuve nil,
le refus d’une preuve qui confirme seulement A, et le déverrouillage même si
la proposition courante est à nouveau A. Un scénario de journal durable couvre
également la preuve pour B suivie du vote pour C. Les préengagements continuent
d’exiger un quorum du tour courant visant exactement leur cible.

## Délais conditionnés par les quorums

`RoundWait` suit un seul couple hauteur/tour à l’aide d’une horloge monotone
injectée par l’appelant. Les messages passent par la collecte vérifiée. Le délai
de proposition part de l’entrée du tour. Les délais de prévote et de
préengagement ne commencent qu’à la première réception de plus de deux tiers
du poids de participation à l’étape concernée. Les doublons ne les réarment pas.

Un quorum de prévotes pour une cible précise (bloc ou nil) termine immédiatement
l’attente correspondante. Avec des cibles réparties, il faut attendre le délai
après quorum de participation. Un quorum de préengagements nil autorise le tour
suivant immédiatement ; un quorum pour un bloc donne une preuve de commit et
empêche de demander un tour suivant, même après expiration d’un délai. Le seul
silence du réseau ne suffit jamais à déclencher les délais de vote.

Les tests utilisent quatre validateurs et une horloge contrôlée, sans sommeil :
frontières exactes, arrivée tardive du quorum, répétitions, votes répartis,
quorums nil, priorité du commit et refus d’une horloge qui recule. Les durées
doivent être strictement positives et sont fournies par l’appelant.

Ce composant donne des conditions de progression ; il ne signe pas, ne change
pas lui-même le tour et ne programme pas de timer réseau. La machine d’états
qui applique ces conditions, l’accroissement des délais de proposition selon
le tour, la reprise de son état et le choix du proposant restent à intégrer.

## Commit canonique puis progression du journal

`Blockchain::commit_certified_round` coordonne les deux écritures sous le verrou
du moteur. Il vérifie d’abord que la hauteur et l’ensemble du journal correspondent
au parent canonique ; un journal neuf n’est accepté que pour le bloc 1. Il vérifie
ensuite l’authentification, le quorum et l’exécution du bloc, puis enregistre
atomiquement le bloc, son état, ses reçus et son certificat dans la base canonique.
Enfin, il avance le journal avec l’ensemble de validateurs dérivé de cette exécution.

Ces deux opérations RocksDB sont synchrones mais ne constituent pas une transaction
atomique entre bases. Si la seconde échoue, le bloc reste enregistré et l’appel
peut lever une exception malgré ce commit. L’appelant doit alors lancer
`recover_voting_progress` avant de reprendre les votes ; répéter simplement le
commit du même bloc n’est pas une stratégie de reprise. La notification de bloc
peut également précéder l’avancement du journal.

Les tests vérifient le refus d’un journal déjà en avance avant toute mutation
canonique, puis le commit du déploiement de staking et l’activation des poids
issus de son exécution. Le scénario SIGKILL conserve volontairement un arrêt
entre les deux écritures pour tester la reprise auditée, et utilise cette API
coordonnée pour le bloc suivant. Cela ne teste pas une coupure électrique pendant
une écriture interne de RocksDB. L’API n’active pas le consensus BFT sur le réseau.

## Retrouver un vote réservé après arrêt

Pour les nouvelles réservations en mode consensus, le même WriteBatch synchrone
contient désormais les champs non signés du vote, son digest, le watermark et
le verrou. `ChainStore::latest_consensus_vote` lit le dernier vote réservé dans
le domaine chaîne/genesis/validateur demandé. Il vérifie le slot, les champs,
l’identité du signataire et le digest avant de retourner un vote sans signature.
Un vote nil reste explicitement nil. La lecture ne signe rien et n’autorise
aucune nouvelle transition ; la signature doit toujours passer par le journal.

Le test SIGKILL récupère ces champs après l’arrêt précédant le commit et reproduit
la signature exacte réservée auparavant. Des tests de réouverture couvrent aussi
les domaines distincts, nil, un contenu modifié, une étape invalide et la suppression
du contenu réservé. La récupération échoue si les champs manquent, notamment
pour un ancien journal qui ne stockait que les digests ; elle ne les invente pas.
L’ancien chemin de répétition d’un vote intégralement fourni reste disponible.

Cette lecture peut retourner un vote d’une hauteur déjà certifiée. Elle ne remplace
donc ni la reprise auditée de la progression canonique ni la récupération du bloc
proposé et des preuves nécessaires au moteur de tours. Elle ne constitue pas une
protection contre la réécriture cohérente du journal entier ou sa restauration
depuis une sauvegarde ancienne.

Le watermark est également comparé au dernier slot réellement présent dans
l’index des réservations. Le supprimer ou le remplacer par une ancienne valeur
ne permet plus de présenter le journal comme neuf ou de masquer des votes plus
récents. Les lectures de reprise, les nouvelles réservations et l’avancement
certifié refusent cette incohérence. La recherche se fait par positionnement
dans l’index RocksDB, sans parcourir tout l’historique.

Les tests altèrent séparément le watermark dans un journal brut et dans un
journal avec verrous, après deux réservations de tours distincts. Ils vérifient
le refus d’un nouveau slot et, pour le mode consensus, des lectures de reprise
et de l’avancement certifié. Un observateur ayant suivi des certificats sans
jamais voter peut légitimement ne pas avoir de watermark. La restauration
cohérente de l’ensemble du journal à une ancienne date reste hors de cette
détection locale.

`ChainStore::voting_lock` permet de retrouver le verrou de la hauteur active,
avec son ensemble de validateurs, son tour et son hash de bloc éventuels.
L’absence de verrou après une progression certifiée est distincte d’un verrou
supprimé après un vote : ce dernier cas est refusé. La lecture vérifie le
watermark, les champs canoniques et la cohérence du tour du verrou avec le
dernier tour réservé. `voting_position` utilise également ces contrôles.

Les tests vérifient la conservation du verrou après un préengagement nil,
sa levée par une justification plus récente, et le démarrage sans ancien verrou
à la hauteur certifiée suivante. Ils refusent un ensemble incohérent, un tour
futur, un hash absent ou mal formé, et un enregistrement supprimé.
Cette API retrouve l’identité du bloc verrouillé. Le contenu peut être relu via
`voting_proposal` pour les votes émis par le moteur avec la persistance décrite
ci-dessous. Les preuves de prévotes et la machine de tours restent à raccorder
avant une reprise automatique du consensus réseau.


## Contenu des blocs votés

`Blockchain::vote_for_block` transmet désormais le bloc déjà authentifié et
rejoué à la réservation durable. Pour une nouvelle réservation, le même batch
RocksDB enregistre son contenu complet, le vote et le verrou avant la signature.
Le journal refuse un bloc dont la hauteur, le hash ou la signature du producteur
ne correspondent pas au domaine du vote. Le contenu JSON est limité à 16 MiB.

`ChainStore::voting_proposal` vérifie à la lecture le hash, la hauteur et la
signature dans le domaine chaîne/genesis. L’appelant doit toujours rejouer le
bloc contre le parent canonique avant de l’utiliser. Le test SIGKILL retrouve
le bloc verrouillé dans le journal et le revérifie après redémarrage ; le test
staking vérifie également sa conservation avec ses transactions et le refus
d’une signature de producteur altérée avant toute réservation de vote.

Les primitives de vote employées sans bloc n’enregistrent pas ce contenu. Une
ancienne réservation exacte peut recevoir le bloc validé par le moteur avant
la répétition de sa signature ; un contenu existant différent est refusé.
Les propositions restent conservées dans le journal : une politique sûre de
rétention et de nettoyage après finalité reste à définir.

## Coordinateur d’un tour (non activé sur le réseau)

`ConsensusRound` relie `RoundWait` aux méthodes de signature vérifiée de
`Blockchain` et au journal RocksDB. Une proposition doit être authentifiée et
rejouée avant d’être retenue. `advance` produit les votes à diffuser après leur
réservation persistante ; un quorum de préengagements pour la proposition connue
permet un `commit_certified_round`. Les appels suivants après commit ne signent
pas à nouveau. Un changement externe du parent fait échouer le coordinateur.

L’expiration sans proposition émet un prévote nil. Le préengagement dépend du
quorum ou du délai de participation ; un quorum nil ne publie aucun bloc et
signale le besoin d’un autre tour. Un quorum pour un bloc dont les données ne sont
pas présentes ne suffit pas à signer un préengagement ni à publier le bloc.

Le test `consensus_round` utilise la vraie chaîne, les signatures et RocksDB :
proposition avec faux état refusée avant réservation, deux hauteurs certifiées,
réouverture et expiration nil. Il utilise le validateur bootstrap unique ; les
tests de quorum multivalidateur restent dans `finality` et ne constituent pas une
preuve d’orchestration réseau distribuée.

Limites explicites : le coordinateur retient une seule proposition par tour,
n’effectue pas la rotation du proposant et ne transporte aucun message. Il refuse
un démarrage lorsqu’un vote local existe déjà à la hauteur courante : il manque
encore le contrôleur de reprise et les preuves permettant de changer de tour avec
un verrou existant. L’appelant doit arrêter et récupérer après une erreur de
persistance ; il ne doit pas ignorer cette erreur et reprendre des signatures.
Aucun démarrage automatique, changement de protocole live, ni disponibilité des
tags RPC `safe`/`finalized` n’est annoncé par cette intégration.

### Scénarios à quatre validateurs

`consensus_round` prépare également quatre validateurs de même poids par
déploiement et inscriptions signées dans NanoStaking. Leur état exécuté initialise
quatre instances locales isolées (fixtures de test, sans modification du genesis
livré). Le test distribue les messages en mémoire de façon déterministe : doublon,
équivocation, signature falsifiée, préengagements insuffisants, quatrième
validateur silencieux puis retardataire. Deux votes sur quatre ne suffisent pas ;
trois permettent un certificat. Un certificat reçu avant la proposition attend
les données vérifiées. Les quatre racines convergent, les historiques sont audités
et les certificats sont relus après réouverture.

Il s’agit d’une orchestration multivalidateur en processus, pas d’un essai du
transport P2P ou d’une preuve face à toutes les partitions et changements de tour.

### Reprise du même tour

`Blockchain::resume_consensus_vote` relit la dernière réservation, vérifie son
digest, la hauteur, le comité courant et le verrou. Pour un vote de bloc, il
recharge la proposition persistée et vérifie son authentification, son exécution
et le comité suivant avant de reproduire exactement sa signature. Aucun nouveau
vote ni changement de verrou n’est créé par cette opération.

Le coordinateur accepte maintenant cette reprise dans le même tour : le vote
retrouvé est rediffusable une fois, et les votes des pairs doivent être reçus à
nouveau pour former un quorum. Les anciens paragraphes indiquant le refus de
toute reprise sont remplacés sur ce point. Un changement de tour avec un vote
existant reste refusé. Les tests rouvrent chaîne et journal après prévote et
préengagement avec quatre validateurs, puis vérifient l’identité des signatures
et la convergence ; la fixture SIGKILL passe aussi par cette API de reprise.

### Transition explicite vers le tour suivant

`begin_next_round` exige un quorum nil de préengagements ou l’expiration du délai
après participation de plus de deux tiers. Sans participation suffisante, il
refuse. Il conserve la proposition du verrou persistant, retient la justification
de prévotes disponible et crée un collecteur vide dans le nouveau domaine de
tour. Aucun ancien vote n’est compté dans ce nouveau quorum. Le test vérifie le
refus prématuré et un commit au tour 1 après un tour nil. Le redémarrage direct
sur un autre tour reste refusé ; les votes des pairs doivent être rejoués dans le
tour durable avant de reconstruire cette transition. Rotation du proposant,
ordonnanceur et transport restent à intégrer.

### Compromission d’une clé de validateur

Deux machines possédant la même clé représentent le même poids de vote, pas deux
validateurs indépendants. Un journal local empêche les contradictions produites
par ce journal, mais pas les signatures émises ailleurs avec une copie de la clé.
La détection d’équivocation ne constitue ni une révocation automatique ni un
slashing implémenté. Le vol d’une clé exige une procédure de remplacement/retrait
validée par le protocole ; son adresse IP ne permet pas de rétablir la propriété
cryptographique. Nano ne doit pas être annoncé comme protégé contre le phishing
de clés par le seul mécanisme BFT.

### Politique retenue pour une clé détenue par un tiers

Le protocole n’arbitre pas la propriété humaine de la clé. Son détenteur peut
continuer la validation en respectant les règles ; aucun gel fondé sur un simple
changement d’IP ou une déclaration de vol n’est introduit. Le contrôle porte sur
les signatures, le poids autorisé, l’exécution des blocs et les règles de vote.
Une copie de clé ne doit jamais augmenter le poids du validateur.

Ce choix ne signifie pas qu’un détenteur malveillant est forcé de coopérer : il
peut rester silencieux ou retenir des messages. La disponibilité dépend du quorum
restant ; les garanties BFT restent conditionnées au poids byzantin et aux
hypothèses réseau. Aucun comportement arbitrairement malveillant n’est rendu
impossible par le seul fait de détenir une clé valide.

### Verrou traversant un changement de tour

Le test intégré utilise aussi des préengagements répartis entre bloc et nil : la
participation autorise l’expiration, mais pas un certificat. Le passage au tour 1
conserve le bloc verrouillé, refuse une proposition incompatible et rejette les
votes du tour 0. Une reprise du prévote au tour 1 reproduit sa signature, conserve
l’ancien verrou puis certifie le même bloc uniquement après un nouveau quorum.
Le test `consensus_round` passe avec ce scénario ; le transport réseau et la
rotation du proposant restent hors de cette preuve.

### Mode BFT explicite dans le genesis

`config.bftVersion: 1` exige `consensus: pos` et `posSignatureVersion: 1`.
L’absence du champ conserve les règles antérieures ; les autres valeurs, y
compris zéro explicite, flottants et chaînes de caractères, sont refusées. Le
marqueur BFT est lié au hash du genesis par le domaine `Nano/BFT/Rules`.

Dans ce mode, `import_block` sans certificat et `validate_signed` sont refusés
dès la première hauteur. Les blocs passent par l’import certifié ; la réouverture
refuse un tip non-genesis sans certificat. Les fixtures à quatre validateurs
utilisent ce mode et vérifient ces refus. Aucun genesis livré n’a été modifié.

Cette option est une règle expérimentale de certification obligatoire, pas une
annonce d’activation opérationnelle : la rotation du proposant et le transport
du consensus ne sont pas encore intégrés. Ne pas lancer un réseau public sur
la seule base de cette option ou contourner ses refus avec l’ancien producteur.

### Rotation native du proposant BFT

Le mode expérimental `bftVersion: 1` encode maintenant le tour de création du
bloc dans `extraData` : trois octets ASCII `NBF`, l’octet version 1 et huit octets
big-endian pour le tour. Le champ est couvert par le hash et la signature du
bloc. Les blocs sans cet encodage canonique sont refusés dans ce mode. Cela
modifie les fixtures BFT expérimentales ; aucune migration d’un historique BFT
antérieur à cet encodage n’est fournie. Les genesis livrés ne sont pas modifiés.

Le tour 0 conserve le proposant sélectionné selon le stake et le parent. Les
tours suivants parcourent cycliquement les membres actifs triés par adresse,
à partir de ce proposant. Le calcul reste borné même pour un grand numéro de
tour. Les quorums restent pondérés par le stake. Cette rotation est une règle
native Nano, **pas** l’algorithme de priorité pondérée des proposants CometBFT ;
Nano ne revendique pas la compatibilité avec son format ou son implémentation.

Une nouvelle proposition doit correspondre au tour du coordinateur. Un bloc
verrouillé peut conserver son ancien tour de création lors d’un tour ultérieur.
Un vote ou certificat antérieur au tour de création est refusé. Le test retire
complètement le proposant initial : les trois autres validateurs échangent des
votes nil, avancent au tour 1, puis certifient le bloc du proposant suivant. Il
vérifie aussi l’échec d’un producteur non sélectionné et d’un réétiquetage du tour.
Le transport et l’ordonnancement réseau restent à intégrer.

### Transport des blocs certifiés

Les réponses `nano_networkBlocks` et les annonces `nano_networkEvents` incluent
maintenant `bftCertificate` (version, tour canonique hexadécimal et votes signés).
Le relais cpp-libp2p transporte cette enveloppe via ses chemins de synchronisation
et d'annonce existants. `validator_submitBlock` exige le certificat en mode BFT,
vérifie les signatures et le quorum avec l'ensemble local du parent, puis importe
le bloc et sa preuve atomiquement. Aucun poids de vote envoyé par le pair n'est
accepté. Les votes sont limités à 2 048 octets chacun et le certificat à 1 000 votes,
sans dépasser le nombre de validateurs locaux. Les limites globales des messages
et des lots réseau restent applicables. Les annonces répétées sont idempotentes.

Les tests `consensus_round` couvrent les certificats absents, insuffisants,
dupliqués, trop grands et les enveloppes non canoniques. Le scénario `bft_network`
exerce deux processus de cœur et deux relais cpp-libp2p, synchronise trois blocs
certifiés puis un quatrième contenant une transaction propagée. Son certificat
est produit par un coordinateur local à un seul validateur de test : ce n'est pas
encore un réseau distribué de validateurs BFT. La diffusion des propositions et
des votes avant engagement, leur ordonnanceur et les changements de tour en
réseau restent à raccorder et qualifier.

### Réacteur et premiers essais distribués

Le réacteur expérimental `BftReactor` est désormais raccordé au timer du cœur et
au protocole cpp-libp2p `/nano/bft/1.0.0`. Une configuration locale explicite
`bft: {key_env: "VARIABLE", journal: "/chemin/journal"}` l'active uniquement avec
un genesis BFT version 1. Le superviseur retire cette variable de l'environnement
du relais. Le genesis livré n'active pas cette option.

Les échanges `nano_bftExchange` véhiculent la proposition et au maximum deux votes
vérifiés par validateur (prevote et precommit), plus le dernier tour pour permettre un rattrapage immédiat. Ils ne
transportent aucune clé ni poids de validateur. Le journal réserve les signatures
avant diffusion. Le cœur vérifie les propositions et signatures ; le relais ne
peut pas signer. Les échanges sont limités par les cadres et quotas P2P existants.

Tests vérifiés : quatre processus validateurs avec quatre relais parviennent à
un bloc identique et un certificat d'au moins trois signatures ; en omettant le
proposant initial, les trois autres changent de tour et certifient un bloc.
Ces essais courts locaux ne constituent pas une qualification de production.
Les votes vérifiés sont retransmis entre pairs avec déduplication par validateur
et étape ; les preuves d’équivoque restent dans le collecteur sans grossir la
diffusion. Les topologies en étoile et en chaîne de quatre processus passent les
tests `bft_star_network` et `bft_line_network`. Les doublons répétés, lots au-delà
de deux votes par membre et signatures falsifiées sont testés dans le réacteur.
Les pertes prolongées, le rattrapage
sur plusieurs tours, le remplacement d'une proposition verrouillée avec preuve,
les partitions et la reprise distribuée sous charge restent incomplets ou non
qualifiés. Le scheduler est natif Nano, pas le binaire officiel CometBFT.

### Partition et arrêt brutal distribués

`bft_partition_network` isole quatre validateurs de même poids en deux groupes de
deux pendant douze secondes, vérifie qu'aucun ne finalise, puis rétablit les liens
et exige le même bloc certifié sur les quatre cœurs. `bft_restart_network` ajoute
un SIGKILL des quatre cœurs avant reconnexion. Chaque processus reprend sa base
et son journal existants ; le test exige la réémission exacte du dernier vote
local réservé, signature incluse, puis la convergence après reconnexion.
Les deux scénarios passent. Ils ne couvrent pas encore une partition après
verrouillage divergent, des votes byzantins coordonnés, ni une coupure de plusieurs
tours dans un réseau où une majorité continue à progresser.

## Partition, arrêt brutal et flood RPC combinés

`bft_restart_flood_network` démarre quatre cores et quatre relais cpp-libp2p
locaux avec quatre validateurs de même poids. Pendant une partition 2+2 de
12 secondes, quatre workers envoient continuellement des transactions signées
pour une autre chaîne et des enveloppes BFT malformées, alternées avec des
lectures `eth_chainId`. Ils utilisent des connexions HTTP persistantes.

Le test vérifie l'absence de finalisation des deux moitiés, tue les quatre cores
par SIGKILL, les redémarre sur leurs bases et journaux existants et vérifie la
reproduction exacte de chaque vote réservé. Les workers reprennent les nouveaux
ports, le réseau est reconnecté, et tous les validateurs doivent finaliser le
même premier bloc avec certificat de quorum. Une transaction valide est ensuite
soumise sous flood continu : chaque nœud doit retrouver son reçu réussi dans le
même bloc et le destinataire doit recevoir exactement une unité.

Exécution observée : 29,22 secondes, 80 943 rejets de transactions étrangères,
80 946 rejets d'enveloppes BFT, 161 889 lectures correctes et aucune réponse
inattendue. Quatre erreurs de transport ont été observées pendant l'arrêt brutal.
Les compteurs dépendent de la machine ; le test impose des minima et l'absence
de réponses inattendues, pas ces valeurs précises.

Le premier générateur ouvrait une connexion par requête et a épuisé les ports
éphémères clients de macOS. Ce défaut du banc a été corrigé par la réutilisation
des connexions, sans supprimer le flood ni les vérifications de consensus.

Ce résultat est limité à quatre workers RPC locaux et une partition courte.
Il ne mesure pas un débit maximal, ne couvre pas les votes byzantins valablement
signés, le flood des connexions P2P, la saturation longue durée ou les transactions
EVM coûteuses. Il ne suffit donc pas à déclarer le réseau prêt pour la production.

## Votes byzantins signés et reprise d'un réacteur verrouillé

Le test natif `consensus_round` comprend aussi trois réacteurs honnêtes et un
quatrième membre malveillant, tous de même poids dérivé du contrat de staking.
Le proposant initial est honnête. L'attaquant signe avec sa vraie clé des prevotes
et precommits nil ou portant sur des hashes différents selon le destinataire,
puis répète chaque enveloppe. Aucune signature forgée n'est nécessaire.

Le scénario vérifie que ces votes ne fabriquent pas de certificat, que la
diffusion reste bornée à deux votes par membre et que les trois prevotes honnêtes
permettent de verrouiller le bloc. Un réacteur est alors détruit et rouvert avec
sa base et son journal : il doit réémettre exactement son precommit réservé.
Les trois réacteurs doivent ensuite finaliser le même hash et la même racine
d'état, avec un certificat de trois votes honnêtes et un audit réussi, tandis
que l'injection de votes contradictoires continue avant la finalisation.

La suite `consensus_round` passe avec ce scénario (7,13 secondes pour la suite
lors de cette exécution). Il s'agit d'une simulation déterministe en processus
avec stockage réel, pas d'un essai réseau ni d'un SIGKILL. Elle ne couvre pas un
proposant byzantin, les partitions sur plusieurs tours, les poids inégaux ou les
changements d'effectif sous attaque.

## Proposant équivoque : préserver les votes de participation

Un second scénario de `consensus_round` utilise la clé du proposant initial
pour signer deux blocs exécutables différents au même tour et à la même hauteur.
Deux réacteurs honnêtes reçoivent le premier, le troisième reçoit le second ;
le proposant cesse ensuite de participer. Les trois réacteurs échangent leurs
enveloppes complètes, comme le fait le relais.

Ce test a reproduit un blocage : le rejet de la proposition contradictoire
interrompait le traitement de l'enveloppe avant la collecte de ses votes.
Sans ces votes de participation, le délai des prevotes ne permettait pas de
sortir du tour divisé. Le réacteur vérifie désormais l'authenticité et l'exécution
de l'autre proposition, conserve sa proposition locale, puis traite les votes
indépendamment du désaccord entre propositions. Il ne remplace aucun verrou et
ne permet pas de signer une seconde fois au même tour.

Le test exige que les trois réacteurs finalisent un même bloc dans un tour
ultérieur, avec la même racine d'état et un audit réussi. Il vérifie aussi qu'une
proposition alternative signée mais portant une racine d'état incorrecte est
toujours rejetée sans modifier l'enveloppe locale. Ce scénario ne qualifie pas
encore les partitions avec des verrous divergents ni les changements de poids.

Après reconstruction de tous les binaires, `ctest --test-dir build-cmake
--output-on-failure -j2` passe les 39 tests natifs (116,50 secondes), y compris
ce scénario, les essais BFT réseau et les régressions EVM embarquées. Cette suite
ne représente ni l'intégralité du corpus Ethereum ni un essai de longue durée.

## Rattrapage d'un validateur après quatre hauteurs manquées

`bft_lagging_network` lance quatre cœurs et quatre relais cpp-libp2p. Un
validateur est isolé dès le départ ; les trois autres doivent finaliser au moins
quatre blocs et une transaction signée, pendant que le nœud isolé reste vivant
à la hauteur zéro. Son cœur est ensuite tué par SIGKILL et rouvert sur sa base
et son journal existants. Avant reconnexion, il doit réémettre exactement son
ancien vote réservé pour la hauteur un.

Après reconnexion, le nœud doit importer les blocs certifiés manqués, retrouver
les mêmes hashes et racines d'état aux quatre hauteurs, le même reçu et le
solde attendu. Un autre validateur est alors déconnecté. Une seconde transaction
est soumise au nœud revenu : les trois participants restants doivent la finaliser
dans le même bloc, sans perte ni double application. Le certificat de ce bloc
doit explicitement contenir le vote du validateur revenu.

Ce test qualifie un rattrapage de plusieurs hauteurs avec journal obsolète et
reprise effective du vote. Il ne couvre pas une longue partition sur une même
hauteur avec verrous divergents, les changements de validateurs, ni une reprise
après restauration d'une ancienne sauvegarde du journal de signatures.

Le scénario complet passe en 42,73 secondes lors de l'exécution observée ;
ce temps dépend des proposants et de la machine, et n'est pas un seuil de débit.

## Changement de poids par transaction signée

Le scénario de `consensus_round` démarre avec quatre dépôts égaux réellement
exécutés dans NanoStaking. Une transaction `addStake()` signée porte le dépôt
du premier validateur à quatre unités, contre une pour chacun des trois autres.
La transaction en attente ne change aucun poids. Un certificat calculé avec les
poids futurs est rejeté pour son propre bloc d'activation ; les anciens poids
doivent certifier ce bloc. Deux chaînes rejouent ensuite la transaction et
obtiennent le même nouvel ensemble de poids, enregistré dans le certificat.

Pour le bloc suivant, les certificats valablement signés mais liés à l'ancien
ensemble sont rejetés. Trois petits validateurs ne représentent plus que 3/7
du poids ; le plus gros seul représente 4/7. Aucun de ces groupes ne peut
certifier le bloc, et répéter la signature du plus gros ne suffit pas non plus.
Chaque rejet conserve la hauteur, la racine d'état et l'absence de certificat
pour cette hauteur. Le plus gros accompagné d'un petit représente 5/7 : ce
certificat est accepté, les deux états concordent et l'audit reste valide après
réouverture de la base.

Ce test exerce le contrat et la vérification/persistance des certificats. Il ne
qualifie pas encore les entrées/sorties de validateurs dans les réacteurs réseau
en cours de fonctionnement, ni leur comportement pendant une partition.

## Activation d'un réacteur après inscription

Une clé BFT configurée mais absente de l'ensemble certifié reste désormais en
mode observateur : aucun tour de signature n'est créé et `nano_bftExchange`
expose un snapshot nul. La synchronisation des blocs certifiés reste disponible.
Auparavant, chaque tick tentait de créer un tour et levait `round signer not in
parent set`. L'appartenance est relue lors du changement de parent ; une
inscription exécutée et certifiée peut donc activer le réacteur sans redémarrage.

Le test `consensus_round` démarre quatre validateurs inscrits et un cinquième
réacteur configuré avec une clé financée mais non inscrite. Ni un tick ni une
proposition contenant son inscription ne l'autorisent à voter. Le bloc portant
la transaction `registerValidator()` est importé avec trois signatures de
l'ancien ensemble. Pour le bloc suivant, un ancien validateur reste silencieux :
les quatre autres réacteurs doivent converger, et chaque certificat doit contenir
la signature du nouveau membre. Le test passe avec le même réacteur conservé
avant et après l'inscription.

Cette preuve porte sur les réacteurs natifs et les bases réelles, avec import
explicite du bloc d'inscription. Elle ne remplace pas un test de cette transition
via cpp-libp2p, ni la qualification du retrait et de la réinscription sous partition.

Après reconstruction avec ce correctif, les 40 tests CTest passent en
131,04 secondes (`--output-on-failure -j2`), dont le rattrapage réseau ajouté.

## Inscription sur le réseau cpp-libp2p

`bft_joining_network` démarre quatre cœurs et quatre relais, avec seulement trois
validateurs inscrits. Le quatrième suit le premier bloc certifié sans voter,
puis soumet sa transaction signée `registerValidator()` par son propre RPC.
Tous les nœuds doivent recevoir le même reçu réussi. Le certificat du bloc
d'inscription contient exactement les trois anciens validateurs et exclut
explicitement le nouvel inscrit.

Le relais d'un ancien validateur est ensuite arrêté. Une nouvelle transaction
est envoyée au nœud nouvellement inscrit, sans redémarrage. Les trois nœuds
connectés doivent la finaliser dans le même bloc, appliquer le transfert une
seule fois et stocker un certificat contenant la signature du nouveau membre.
Ce test passe en 16,78 secondes lors de l'exécution observée. Il s'ajoute aux
40 tests précédents ; la suite complète de 41 tests n'a pas été relancée à cette
étape. Le retrait, la réinscription et les changements d'effectif sous partition
restent à qualifier.

## Retrait et réinscription avec le même journal

Le scénario natif de `consensus_round` prolonge l'inscription jusqu'à la hauteur
100 avec des blocs certifiés et exécutés, sans modifier les règles de verrouillage
du contrat. Le retrait signé est exécuté à la hauteur 101, après les 100 blocs
requis depuis l'inscription. Le même réacteur devient observateur et cesse de
diffuser des votes. Une signature du membre retiré est rejetée au bloc suivant,
qu'elle annonce l'ancien hash d'ensemble ou celui des membres actuels.

Une réinscription signée est ensuite certifiée à la hauteur 102. Sans recréer le
réacteur, son journal reprend l'historique certifié et autorise un prevote pour
la proposition courante à la hauteur 103. La base et le journal sont enfin
fermés puis rouverts : la signature réservée doit être réémise à l'identique et
l'audit de la chaîne doit réussir. Les blocs intermédiaires sont importés avec
des certificats construits par le banc ; ce test ne constitue pas encore un
essai réseau du retrait ou d'une réinscription pendant une partition.

## Reprise après SIGKILL avec Paris et Shanghai

Les suites finality_crash_paris et finality_crash_shanghai reprennent les trois
frontières d'arrêt brutal avec evmFork explicite et bftVersion=1. Avec la suite
historique, neuf SIGKILL sont vérifiés. Chaque scénario récupère la signature
exacte, progresse jusqu'à deux blocs certifiés et réussit l'audit canonique.

Après réouverture, le test construit aussi une proposition alternative valide
et tente un préengagement au même tour via sign_consensus_vote. Il exige
le refus exact « conflicting vote refused at height/round/step », une signature
encore vide et le vote durable initial inchangé. Le premier test ajouté utilisait
à tort sign_vote_durable en mode brut : son refus de mode ne constituait pas
une preuve de double-vote. Les résultats intermédiaires et l'échec du contrôle
plus strict sont conservés sous localBftCrashRecovery.

Périmètre : un validateur d'amorçage local, frontières applicatives durables.
Cela ne qualifie pas encore un réseau distribué, une coupure électrique ou
une interruption interne d'écriture RocksDB.

## Reprise distribuée locale sur Paris et Shanghai

La configuration Release actuelle raccorde explicitement nano-p2p via
NANO_NETWORK_EXECUTABLE ; son absence excluait les suites réseau de cette
configuration. Les quatre nouvelles suites qualification_bft_network_local
couvrent Paris et Shanghai, chacune en fonctionnement normal puis reprise.
Les quatre validateurs utilisent un registre obtenu par exécution du contrat
de staking ; les échanges passent par quatre relais cpp-libp2p locaux.

Pour chaque profil, la coupure en deux groupes de deux dure douze secondes
sans finalité. Les quatre cœurs sont ensuite tués par SIGKILL, puis relancés
sur les mêmes bases et journaux. Le test exige la reproduction exacte des
votes réservés, reconnecte les relais et compare le hash/racine d'état du
premier bloc certifié sur les quatre nœuds, avec au moins trois votes.
Résultat actuel : 4/4 suites réussies, sans flood. Les résumés structurés,
commandes sélectionnées et empreintes figurent sous localDistributedBftRecovery.

Ce périmètre reste local, à quatre validateurs et ensemble initial fixe. Il
ne qualifie pas les longues durées, le réseau WAN, les pannes internes de
stockage, les grandes variations d'effectif ou toute la préparation production.

## Rattrapage et nouveau membre sur Paris/Shanghai

Les quatre suites qualification_bft_membership_local passent : rattrapage et
adhésion sur chacun des deux profils. Un validateur isolé reste à hauteur zéro
pendant que les trois autres certifient quatre blocs et un transfert. Après
SIGKILL, il reproduit son ancien vote, importe les quatre mêmes hashes/racines
et le reçu, puis fournit une signature nécessaire à une nouvelle finalité après
déconnexion d'un autre signataire. Le solde destinataire vérifie l'application
unique des deux transferts.

L'adhésion démarre avec trois validateurs et un observateur. L'inscription
signée via NanoStaking est certifiée uniquement par les trois anciens membres ;
le nouveau membre ne certifie pas sa propre inscription. Une fois un ancien
membre déconnecté, le nouveau signe le certificat du transfert suivant, sans
redémarrage des processus. Les pairs actifs concordent sur reçu et solde.

Preuves : localBftMembershipRecovery (4/4, 93,11 secondes). Périmètre local à
quatre nœuds et stakes égaux ; départs, poids inégaux, WAN et longue durée ne
sont pas qualifiés par ces scénarios. Aucun flood exécuté.

## Quorum réseau avec stakes inégaux

Les scénarios bft_weighted_paris_network et bft_weighted_shanghai_network
créent quatre validateurs de poids relatifs 4:1:1:1 par exécution du staking,
puis vérifient ces poids dans l'ensemble de vote réel. Le validateur à 4/7
isolé et les trois autres réunis à 3/7 restent sans finalité douze secondes.
Après changement des connexions, le groupe de deux à 5/7 finalise, tandis
que le groupe de deux à 2/7 reste à hauteur zéro.

Le certificat contient exactement deux signatures, vérifiées contre l'ensemble
pondéré complet par verify_commit_certificate. Aucun signataire du groupe
isolé n'y apparaît. La reconnexion des quatre nœuds converge vers le même
hash et la même racine d'état. Résultat : 2/2 en 60,11 secondes, sans flood.
Preuves : localWeightedBftQuorum. La frontière exacte de deux tiers, les
changements de poids en direct et les conditions WAN restent à qualifier.

## Frontière stricte de deux tiers sur le réseau local

Les tests bft_two_thirds_paris_network et bft_two_thirds_shanghai_network
utilisent les poids 3:1:1:1. Les groupes initiaux restent sans finalité, puis
les deux signataires connectés à 4/6 restent à hauteur zéro pendant douze
secondes. Le test vérifie leur handshake cpp-libp2p et leur participation au
vote. Le groupe élargi à 5/6 finalise avec trois signatures vérifiées contre
l’ensemble pondéré. Après reconnexion, les quatre nœuds concordent sur le
hash et la racine d’état du bloc 1. Résultat : 2/2, 79,54 secondes.

Référence : localBftStrictTwoThirds. Cela complète la frontière laissée ouverte
par localWeightedBftQuorum, sans qualifier les changements de poids, les
départs, les conditions WAN ou la tenue prolongée sous charge.

## Ajout de stake en direct

Les scénarios bft_stake_change_paris_network et
bft_stake_change_shanghai_network passent (2/2, 56,04 secondes). Une
transaction signée addStake transforme les poids 1:1:1:1 en 4:1:1:1. Chaque
pair vérifie le certificat du bloc d’activation avec les anciens poids.
Le signataire à 4/7 isolé ne finalise pas un nouveau transfert pendant douze
secondes. Après connexion à un second validateur, le groupe à 5/7 finalise
ce transfert avec exactement deux signatures vérifiées sous les nouveaux
poids, et le destinataire est crédité une seule fois sur les deux pairs.
Aucun cœur ni journal n’est redémarré pendant cette transition.

Référence : localBftLiveStakeChange. Le test ne prouve pas à lui seul
l’absence de progression du groupe à 3/7, ni les départs, les changements
concurrents ou successifs de poids, ou la qualification WAN/charge.

## Départ d’un validateur : résultat intermédiaire Paris

Le scénario bft_departing_paris_network passe en 260,11 secondes : retrait
anticipé exécuté en échec, maintien du rôle de vote, attente du véritable
délai de 100 blocs (déverrouillage 104), retrait réussi certifié par l’ancien
ensemble, puis arrêt des votes sans redémarrage. Les trois membres restants
certifient une transaction ordinaire signée par l’ancien validateur ; les
certificats sont vérifiés contre le nouvel ensemble et le transfert appliqué
une seule fois. Le scénario Shanghai est encore en cours à ce point de
contrôle ; ce résultat partiel ne qualifie pas les deux profils.
Référence : localBftDepartureCheckpoint.

## Départ : campagne Paris et Shanghai terminée

La campagne localBftLiveDeparture complète le point de contrôle précédent :
2/2 passent en 524,49 secondes. Les deux profils attendent le déverrouillage
104, certifient le retrait au bloc 106 sous l’ancien ensemble et le transfert
ordinaire de l’ancien validateur au bloc 108 sous les trois membres restants.
Le processus sortant cesse de voter sans redémarrage. Le résultat intermédiaire
Paris reste conservé ; réinscription, messages malveillants d’anciennes clés et
conditions WAN/charge restent à qualifier en réseau.

## Écriture RocksDB refusée par le système

localStorageWriteFailure couvre Paris et Shanghai (2/2, quatre frontières de
panne, 2,64 secondes). RLIMIT_FSIZE=0 dans un processus de test provoque une
erreur EFBIG réelle lors de l’ajout au WAL : journal de vote ou base canonique.
La réservation échouée ne produit aucune signature ; l’écriture canonique
échouée n’avance ni état en mémoire ni indices après réouverture. Le processus
sort sans destructeurs, limite toujours active. Une nouvelle exécution reprend
la même signature précommit durable, réessaie le bloc et vérifie par audit
l’application unique du transfert et l’avancement correct du journal.

Les premiers résultats sont conservés, puis les assertions renforcées vérifient
la base exacte concernée et l’égalité de signature entre processus. Cela ne
couvre pas ENOSPC, l’écriture partielle, les erreurs fsync, une coupure physique
ni la disponibilité d’un réseau pendant la panne. Aucun hook de panne ajouté
au moteur de production.

## Écritures partielles du WAL

localStoragePartialWrite étend les erreurs d’écriture précédentes : 2/2 profils
passent en 11,67 secondes, soit 28 interruptions réelles d’ajout au WAL.
Pour chaque frontière (réservation et bloc canonique), une exécution témoin
mesure la taille complète ; les interruptions surviennent après 0, 1, 6, 7,
32 octets, la moitié ou juste avant le dernier octet. La taille physique du
fichier confirme exactement le préfixe écrit avant EFBIG.

Après sortie sans destructeurs, limite toujours installée, un autre processus
vérifie l’absence de bloc/reçu/certificat partiel et réexécute avec audit.
Une réservation incomplète ne libère pas de signature ; le précommit déjà
durable survit à un échec canonique avec la même signature. Le premier
essai limité à 32 octets est conservé avec la campagne élargie.
Cela qualifie ces troncatures de fin de journal, pas ENOSPC, fsync, une
corruption arbitraire de secteurs ou une coupure physique d’alimentation.

## Erreur de synchronisation après écriture complète

localStorageSyncFailure passe sur Paris et Shanghai (2/2, quatre frontières,
2,12 secondes). Une bibliothèque liée uniquement au test macOS injecte EIO
sur fcntl(F_FULLFSYNC) pour le WAL choisi ; le compteur et l’erreur RocksDB
confirment que le chemin a été exercé après ajout des données.

La réservation ne libère aucune signature après l’erreur. Si elle subsiste
à la réouverture, un vote contradictoire est refusé. L’échec de synchronisation
canonique laisse l’état en mémoire à zéro, mais le bloc entièrement écrit
subsiste ici à la réouverture : Nano reprend le journal et vérifie le bloc,
le reçu et l’application unique du transfert, avec le même précommit.
Une erreur de synchronisation ne signifie donc pas forcément absence du bloc
après redémarrage. Le test couvre le chemin où le WAL subsiste, pas la perte
physique des octets non synchronisés. Linux, ENOSPC et disponibilité réseau
pendant la panne restent hors de ce périmètre.

## Perte simulée de la fin non synchronisée et rattrapage du journal

localStorageSyncLossRecovery passe sur Paris et Shanghai : 2/2 profils,
12 cas de panne, 5,15 secondes. Chaque échec F_FULLFSYNC concerne soit
la réservation, soit le bloc canonique, soit l’avancement du journal après
écriture canonique durable. Le suffixe ajouté reste présent ou est supprimé
par le superviseur, après sortie du processus, dans le seul WAL temporaire
ciblé. La taille précédente est contrôlée ; les autres données sont préservées.

À la réouverture, bloc, signature, audit et transfert unique concordent.
Pour l’avancement du journal, le bloc canonique demeure présent et le journal
repart de hauteur 1 si le suffixe est perdu, ou 2 s’il est conservé ; le
rattrapage aboutit à 2 dans les deux cas. Les campagnes intermédiaires et
les assertions renforcées sont conservées. C’est une simulation logicielle
de perte du suffixe non synchronisé, pas une qualification de coupure
électrique physique, de panne du périphérique ou de disponibilité réseau.

## Charge RPC contrôlée par profil

localRpcProfileLoad passe sur Paris et Shanghai (2/2, 8,02 secondes). Chaque
profil reçoit 400 transferts type 2 via huit clients HTTP concurrents, refuse
400 transactions de chaîne étrangère et répond à 400 lectures d’identité.
Le flood est désactivé. Après admission, les transactions sont exécutées
sur au moins quatre blocs de 2,1 millions de gas et importées directement
dans un second moteur. Reçus, racines d’état, solde destinataire exact,
audits des deux historiques et réouverture de la base productrice sont vérifiés.

Les débits/latences consignés mesurent l’admission locale de cette campagne.
La production des blocs suit l’admission ; ce scénario ne qualifie pas
la finalité BFT distribuée, le P2P, la concurrence admission/consensus,
une charge prolongée ou une capacité de production garantie.

## Charge pendant la finalisation BFT via cpp-libp2p

localBftConcurrentLoad termine les scénarios Paris et Shanghai à 240
transactions chacun : 2/2 en 215,12 secondes. Quatre clients attendent
250 ms après chaque séquence envoi/lectures ; des confirmations sont
observées pendant les envois. Les reçus des 240 transactions concordent
sur les quatre pairs, les certificats des blocs concernés sont vérifiés
et le solde destinataire exclut pertes/doublons. Le même bloc/racine final
est retrouvé après réouverture et audit des quatre bases. Aucun flood.

Limite importante : les premiers scénarios, limités à 80 transactions,
échouaient faute de confirmation observée pendant la fenêtre d’envoi.
Allonger la campagne au même rythme permet de vérifier la correction
fonctionnelle mais ne résout pas ce problème de latence. Les durées de
109,23 s (Paris) et 97,60 s (Shanghai) incluent vérifications et réouvertures,
et ne mesurent pas uniquement le temps de finalisation.

Un échantillon CPU d’une seconde sur Shanghai montre la chaîne d’appels
BftReactor::ensure_round → ConsensusRound → recover_voting_progress →
audit_locked. Le code réaudite l’historique entier à chaque récupération
de progression. Ce coût est une piste de correction prioritaire, sans
supprimer les garanties de vérification au redémarrage.

## Réutilisation en mémoire du préfixe certifié vérifié

La reprise ne réexécute plus tout l’historique à chaque ronde si le préfixe
a déjà été intégralement vérifié dans ce processus puis uniquement étendu
par des blocs certifiés, exécutés et écrits durablement. Le repère est inconnu
après réouverture, invalidé avant écriture incertaine/restauration/import non
certifié, et renouvelé par audit explicite. La séquence complète des certificats
du journal est toujours vérifiée : ce coût reste linéaire par reprise.

Preuves bftCertifiedAuditReuse : 15 tests de sécurité/reprise et 19 tests EVM
passent. Les nouvelles régressions refusent un reçu historique corrompu après
réouverture, y compris après extension valide d’un préfixe encore non vérifié,
sans avancer le journal neuf. Les reprises SIGKILL et erreurs d’écriture ou
de synchronisation passent également.

Les deux charges prolongées passent encore, mais les deux essais courts
échouent toujours faute de confirmation observée pendant les envois.
Sur cette exécution Paris passe de 109,23 à 67,42 secondes (vérifications
incluses) ; cela reste une observation ponctuelle, pas un débit garanti.
Le problème de latence courte n’est pas résolu par cette optimisation.

## Réponse BFT immédiate et effet de l’instrumentation

Après acceptation complète d’un échange, le RPC fait désormais avancer
le consensus avant de retourner le snapshot. Les votes localement permis
peuvent ainsi accompagner la réponse sans attendre le prochain tick.
La validation du message précède cet avancement ; les règles de quorum
et de signature durable sont conservées. Cinq régressions natives passent.

Les traces montrent la sensibilité du test court au calendrier : Paris
finalise d’abord un bloc vide, tandis que Shanghai peut inclure un transfert
plus tôt. Quatre répétitions instrumentées passent après la modification,
mais les DEUX essais sans lectures diagnostiques échouent encore. Les
lectures supplémentaires modifient la fenêtre/ordonnancement ; ces passages
ne résolvent donc pas le critère original. Le diagnostic est désormais
optionnel via NANO_TEST_BFT_TRACE et désactivé par défaut.
Référence : bftImmediateReplyDiagnostics. La cadence périodique du relais
reste à examiner ; aucune latence de production n’est qualifiée ici.

## Cadence BFT indépendante du cycle général du relais

Le relais échange désormais les snapshots BFT via un cycle de 500 ms
distinct du cycle d’une seconde de découverte/synchronisation/gossip.
Le cycle suivant attend la réponse locale ; chaque pair conserve au plus
un échange BFT en cours, jusqu’au traitement local de sa réponse. Les
limites entrantes sont inchangées.

Preuves bftRelayIndependentCadence : les quatre répétitions courtes, sans
NANO_TEST_BFT_TRACE et sans lectures de diagnostic, passent (deux par
profil Paris/Shanghai, 79,65 secondes au total). Les 80 transactions par
exécution restent espacées de 250 ms par client ; confirmations pendant
les envois, reçus/certificats/état et réouverture des quatre pairs sont vérifiés.
Les six tests du relais et les quatre scénarios quorum strict/reprise passent
également ; exactement deux tiers ne finalisent toujours pas.

Le critère auparavant en échec est satisfait dans ces quatre exécutions.
Les échecs historiques restent conservés. Cela ne garantit pas une borne
de latence générale : réseaux plus grands, budgets partagés sous charge
prolongée, panne pendant les envois et conditions WAN restent à qualifier.

### Réserve du budget entrant pour consensus et synchronisation

Le relais applique aux transactions, annonces et handshakes un sous-budget :
4 requêtes/s par pair (rafale 8, concurrence 2) et 32 requêtes/s globales
(rafale 64, concurrence 8). Ces requêtes consomment également le budget
commun original : 8/s par pair, rafale 16, concurrence 4 ; 64/s globales,
rafale 128, concurrence 16. Les échanges BFT, chainsync et blocksync restent
soumis à ce budget commun. La limite de 256 identités reste inchangée.

Ainsi, le trafic de transactions seul ne peut plus épuiser la capacité
réservée aux échanges de contrôle. Cela ne garantit pas la disponibilité
face à un pair qui sature lui-même les protocoles de contrôle, ni la
livraison des transactions : la diffusion sans reprise reste à qualifier.
Les vérifications cryptographiques et les seuils de quorum sont inchangés.

### Diffusion avec accusé de traitement (version 2)

Le relais émet les transactions via `/nano/txgossip/2.0.0`. Il conserve un
curseur du journal d'événements par pair et ne l'avance qu'après un accusé
contenant le curseur et le nombre d'éléments attendus. Une réponse perdue,
une fermeture ou un délai expiré laisse ce curseur inchangé. Il réessaie
au cycle suivant ; au plus quatre opérations sont actives globalement,
une par pair, avec sélection circulaire entre les pairs. Chaque lot comporte
au plus 50 transactions et 8 Mio de données hexadécimales. Les annonces de
blocs et leur synchronisation conservent leurs chemins existants.

Le RPC `nano_receiveTransactions` vérifie les limites avant toute admission,
reconnaît les transactions déjà connues (pool ou historique) et renvoie un
résultat par élément. Une répétition connue n'émet pas de nouvel événement.
Un accusé signifie « traité », pas nécessairement « accepté » : les rejets
d'admission sont indiqués dans `accepted`. Le protocole v1 reste reçu, mais
l'émission v2 nécessite des pairs compatibles ; un parc mixte n'est pas qualifié.

Cette reprise couvre les pertes de transport et d'accusé. Elle ne garantit
pas encore la récupération après expiration des 10 000 événements en mémoire,
redémarrage du cœur, ni après un rejet temporaire d'admission chez le pair.
La régulation du coût de vérification par transaction au sein des lots et
les gros ensembles de pairs restent à qualifier. Le mécanisme de perte
`NANO_TEST_DROP_GOSSIP_REPLY_ONCE` est un outil de test désactivé par défaut.

### Budget des transactions transmises au cœur

Les protocoles de diffusion v1 et v2 partagent un budget supplémentaire,
consommé après lecture de la requête et avant appel au cœur : 8 transactions/s
par pair (rafale 64) et 64/s globalement (rafale 128). Chaque élément compte,
y compris un doublon ou une donnée invalide. Le débit de requêtes, les octets
et la concurrence restent soumis à leurs limites distinctes. Un refus de
budget ferme le flux sans accusé ; le curseur v2 reste donc à réessayer.

Le budget utilise l'horloge monotone et la table d'identités existante,
limitée à 256 entrées. Une nouvelle connexion ne réinitialise pas les jetons
consommés. Cette protection borne les éléments transmis par le relais,
pas le temps CPU absolu ni les appels HTTP directs au cœur. Les requêtes
directes et les données de grande taille nécessitent leur propre qualification.

### Budget commun d'admission RPC

Le cœur applique un budget commun de 64 transactions/s (rafale 128) à
`eth_sendRawTransaction`, `nano_receiveTransactions` et l'envoi via compte
local `eth_sendTransaction`. Chaque élément d'un lot consomme une unité.
Le budget est partagé par les connexions HTTP et WebSocket utilisant le
même objet RPC du serveur, sous son verrou existant. Ouvrir une connexion
ou utiliser un tableau JSON-RPC ne le réinitialise pas.

Un dépassement renvoie le code JSON-RPC `-32005` avant décodage/admission
(envoi brut) ou avant signature (compte local). Pour un lot de diffusion,
le budget est vérifié avant toute admission du lot. Les clients peuvent
réessayer après attente sur ce refus explicite ; une erreur de transport
ne prouve pas l'absence d'admission. Les lectures et les règles de consensus
ne sont pas soumises à ce budget. Il ne remplace pas les protections de
parsing HTTP, de calcul EVM ou de répartition équitable entre clients.

### Récupération par inventaire après perte du journal

`nano_gossipPage` remplace la lecture du seul journal dans la diffusion v2.
Une première connexion, un curseur expiré ou une nouvelle session du cœur
commence par un inventaire du pool, ordonné par hash et paginé (50 éléments,
8 Mio maximum). Le curseur d'événements du début de l'inventaire est conservé
jusqu'à sa dernière page. La diffusion reprend ensuite les événements depuis
ce curseur : les insertions derrière le hash déjà parcouru ne sont pas perdues.
La position de pagination n'avance qu'après l'accusé de traitement.

Un identifiant aléatoire de session est recréé à l'ouverture du cœur ; il
n'entre ni dans le genesis ni dans les signatures ou les règles de consensus.
Il invalide les anciens curseurs locaux, y compris un hash de pagination
provenant d'une session précédente. Le pool reste en mémoire : cette reprise
n'assure pas sa persistance, ni la notification d'un redémarrage distant à
un relais qui garde la même identité P2P. Ces scénarios restent à qualifier.
Un journal qui expire de nouveau pendant un long inventaire provoque un
nouvel inventaire à la reprise du flux d'événements ; sa progression sous
saturation continue reste à mesurer.

### Historique de déduplication du pool

Les hashes actifs sont ceux des entrées du pool. Les hashes de transactions
minées récemment sont conservés dans un cache FIFO borné à la capacité du
pool (10 000 par défaut). Une répétition d'un hash déjà miné ne renouvelle
pas sa position dans ce cache. L'historique ne croît donc plus avec la durée
de vie du nœud. Une éviction ou un retrait d'une transaction non minée ne
la marque plus comme définitivement connue : elle peut être admise à nouveau
si les règles courantes le permettent.

Un pool plein refuse une transaction moins rémunératrice que sa moins bonne
entrée ; à frais égaux, l'ordre d'arrivée continue de départager les entrées.
Le contrôle de remplacement de même nonce reste inchangé. Le cache des
transactions minées est une optimisation : les nonces et transactions
canoniques persistants empêchent leur replay même après oubli du cache.
Cette borne porte sur l'historique de hashes. La borne des transactions
en attente est décrite ci-dessous.

### Budget des transactions en attente

Le pool impose aussi 64 Mio maximum de transactions sérialisées par défaut,
configurables par le quatrième argument de son constructeur. Les limites
en nombre et par compte restent applicables. Une admission peut nécessiter
plusieurs évictions, par frais croissants puis ancienneté à frais égaux.
Toutes sont préparées avant mutation : si un candidat ne paie pas assez
pour une éviction nécessaire, aucune entrée existante n'est supprimée.
Les remplacements, retraits, inclusions et remises à zéro ajustent le compteur.

Le test natif vérifie la limite avec 65 transactions proches de 1 Mio, puis
recalcule les octets depuis l'inventaire. Les cas de remplacement et refus
vérifient aussi la conservation des entrées. Cette borne ne couvre pas
l'ensemble du tas, les copies temporaires, le RSS ni le coût CPU sous charge.
Preuves : `boundedPoolBytes` dans le manifeste de qualification.

### Redémarrage du cœur distant

Le statut réseau transmet aussi `gossip_epoch`. Un changement observé par
handshake ou chainsync remplace la progression de diffusion du pair et
redémarre son inventaire, même si son identité P2P est conservée. Les réponses
en vol ne modifient que l'ancienne progression. La détection dépend du
rafraîchissement du statut ; les anciens pairs sans ce champ ne la permettent
pas. Les tests natifs passent, mais le scénario complet entre processus
reste à qualifier (`gossipRemoteSessionDetection`).

Le scénario local `gossip_remote_restart` vérifie désormais une transaction
en attente retransmise après SIGKILL du cœur destinataire, avec les mêmes
processus relais et identité P2P. Les relais sont suspendus uniquement
pendant le redémarrage pour observer le pool vide avant retransmission.
Cette preuve ne couvre pas encore les courses sans pause ni plusieurs pages
d'inventaire (`gossipRemoteRestartIntegration`).

Le scénario `gossip_remote_restart_live_pages` complète cette preuve avec
75 transactions et aucune suspension des relais. Chaque hash revient en
attente après le crash, avec la même identité P2P et le nouvel epoch distant.
Ce passage couvre plusieurs lots de 50 maximum, mais pas toutes les positions
possibles du crash parmi les requêtes en vol (`gossipRemoteRestartLivePages`).

### Présentation des nonces à trous

Les réponses du pool classent comme pending la suite de nonces contigus
depuis le nonce canonique de chaque compte. Les transactions après un trou
sont queued et sont reclassées lorsque le trou est comblé. Ce classement
par nonce n'est pas une garantie d'inclusion : la sélection de bloc vérifie
aussi le gas, les fonds et les autres règles d'exécution. Le total interne
des transactions conservées inclut les deux catégories.

Après l'enregistrement d'un bloc accepté, le pool retire aussi toute
alternative dont le nonce est désormais inférieur au nonce canonique.
Le retrait conserve les successeurs et libère leur budget d'octets sans
confondre ces alternatives avec les transactions effectivement minées.
Un bloc refusé ne déclenche pas ce nettoyage (`txpoolStaleNonceCleanup`).

### Réconciliation périodique du pool

Chaque pair reçoit un nouvel inventaire au plus tôt 60 secondes après
la fin du précédent. Les limites de pagination et d'admission restent
applicables ; une page en cours ou en échec conserve sa position. Ce passage
permet de reproposer les transactions toujours présentes à la source,
y compris après un accusé de traitement avec refus. La validation complète
d'un refus temporaire suivi d'une admission reste ouverte
(`gossipPeriodicInventory`).

Le test local `gossip_temporary_rejection` démontre une reprise après le délai
réel de 60 secondes : une transaction refusée selon la règle de remplacement
est reproposée et admise après retrait contrôlé de sa concurrente. Les cœurs
conservent leur session et aucun nouvel envoi source n'est effectué. Ce cas
ne couvre pas toutes les causes de refus temporaire.

Le test `gossip_capacity_recovery` démontre aussi une résolution naturelle :
un bloc signé libère le quota de 100 transactions d'un compte, puis le
gossip périodique repropose le nonce suivant initialement refusé. Un second
bloc l'exécute et les deux historiques concordent. Cette preuve utilise
un validateur bootstrap, pas un quorum BFT distribué.

L'admission tient compte du coût maximal cumulé de la suite de transactions
pending contiguës d'un compte (valeur et gas au plafond de frais). Les
transactions derrière un trou de nonce ne réservent pas ce budget. Un remplacement exclut
le coût de l'ancienne entrée au même nonce. Cette réservation est une règle
locale du pool ; le solde canonique n'est modifié que par l'exécution.

La fermeture d'un trou peut promouvoir plusieurs transactions déjà admises
dont le coût cumulé dépasse le solde. Ce comportement concorde avec Geth
sur le scénario testé (`txpoolGapPromotionGeth`) : pending ne garantit pas
l'exécution de toute la suite, et les contrôles d'exécution restent décisifs.

Après un bloc accepté, le nettoyage retire également les entrées devenues
individuellement non finançables selon le nouveau solde canonique. Les
transactions futures encore finançables restent disponibles ; leur classement
dépend des trous de nonce laissés par ces retraits. Le budget d'octets est
libéré sans modifier les soldes ni créer de reçu pour une transaction retirée.


## Réessai après erreur de synchronisation du stockage

Les tests `storage_sync_retry_paris` et `storage_sync_retry_shanghai` désactivent
l'injection EIO après l'échec initial puis réessaient dans le même processus.
Dans ces cas, RocksDB conserve l'erreur et refuse les nouvelles écritures ; aucun
vote contradictoire n'est signé. Il faut corriger la cause de l'erreur puis
rouvrir le nœud avec les mêmes bases et le même journal de vote. Ne supprimez pas
le journal pour contourner l'erreur : un bloc peut déjà être durable alors que
l'avancement du journal a échoué. La reprise réconcilie ces deux positions.
Cette preuve concerne les trois frontières testées sous macOS, pas toutes les
erreurs matérielles ou tous les systèmes de fichiers.

### Transport BFT par tour (2026-09-24)

Le relais utilise désormais `nano_bftExchangePage` pour transmettre séparément
le tour courant et le tour précédent. Les deux pages conservent leurs votes
signés et leur proposition complète ; elles ne modifient ni les seuils de
quorum ni les verrous persistants. L'alternance est suivie pour chaque pair.
Le diagnostic historique `nano_bftExchange` reste agrégé et peut dépasser la
limite de transport. Voir [BLOCK-TRANSPORT.md](BLOCK-TRANSPORT.md) pour le
format, les preuves, et la limite encore ouverte d'une proposition seule
dépassant l'enveloppe de 16 Mio.


### Attente des propositions après changement de tour

Le réacteur utilise désormais une attente de proposition de 5, 10, 20, 40,
80 puis 160 secondes selon le numéro de tour, plafonnée à 300 secondes.
Un redémarrage dérive la même attente du tour enregistré. Les attentes de
prevote/precommit restent à cinq secondes ; les règles de quorum, de
signature et de verrouillage ne changent pas. Une durée invalide est rejetée
avant le remplacement de l'état du coordinateur.

Cette adaptation répond à un échec observé de transfert libp2p : les tours
fixes trop courts interrompaient l'arrivée d'une proposition de 18 Mo.
Les deux profils locaux Paris/Shanghai finalisent ensuite au tour 2 avec
reconstruction complète chez au moins deux validateurs distincts.
La preuve couvre quatre cœurs dans un processus et quatre relais séparés,
pas encore quatre processus de cœur indépendants ou plusieurs machines.
Voir `bftFragmentNetworkQualification` et `docs/BLOCK-TRANSPORT.md` pour les
échecs conservés et les limites, notamment les cycles prolongés de proposeurs.


### Régression reproduite : retour du proposant après quatre tours

Les nouveaux tests `bft_proposer_cycle_paris` et `bft_proposer_cycle_shanghai` reproduisent un blocage au tour 4 après quatre quorums nil signés. La reprise de proposition utilise une réservation par hauteur et recharge celle du tour 0 ; le coordinateur la refuse correctement. Aucun bloc contradictoire n'est validé dans ce scénario. La vivacité après rotation complète des proposants n'est donc pas qualifiée. Cette reproduction historique est corrigée par les réservations par tour décrites ci-dessous ; elle reste conservée avec ses résultats initiaux. Voir `docs/QUALIFICATION-FAILURES.md` et la campagne `bft-proposer-cycle-20260924`.


### Réservations des propositions BFT par tour

En BFT, le journal de production distingue maintenant `(chainId, genesis, producteur, hauteur, tour)`. Le tour est lu dans la proposition et contrôlé avant réservation. La réservation d'un autre bloc dans le même emplacement est refusée ; la reprise recharge les octets exacts. Les votes, leurs domaines de signature et les verrous de consensus sont inchangés. Hors BFT, le domaine reste limité à la hauteur.

Un ancien enregistrement par hauteur est vérifié par son empreinte et son bloc conservé. Il ne sert de proposition de reprise que pour son tour d'origine et continue d'interdire une proposition contradictoire dans ce tour. Une ancienne empreinte sans bloc conservé bloque la migration, car son tour ne peut pas être établi. Les nouveaux tours utilisent le sous-espace `/round/v1/`. Pour une hauteur sans réservation ancienne, un marqueur `nano/bft-round-signing/v1` est écrit dans l'emplacement historique, dans le même WriteBatch synchrone que la nouvelle réservation. Le chemin historique refuse ainsi de signer après une réservation par tour. Il ne faut pas supprimer ce marqueur pour effectuer un downgrade. La compatibilité d'un déploiement avec versions mélangées n'est pas qualifiée par ces tests.

La lecture et la réutilisation refusent une réservation dont le bloc, l'empreinte ou le marqueur de hauteur manque, ainsi qu'un bloc ne correspondant pas à son empreinte. Le correctif concerne le blocage reproduit au retour du proposant ; il ne prouve pas la vivacité sous toutes les partitions ou tous les scénarios byzantins.

Les neuf suites finales passent en 218,49 secondes. Chaque profil Paris/Shanghai vérifie le retour au tour 4, la reprise exacte du journal après réouverture, un commit sous quorum signé et un audit après réouverture complète. Dix-sept refus sont contrôlés par profil, dont la lecture et la réutilisation de cinq variantes corrompues. Les essais de gros blocs avec quatre processus cœur indépendants passent également. Les résultats initiaux en échec restent archivés séparément. La campagne ne qualifie pas encore les pannes d'alimentation pendant l'écriture de ce nouveau format, les versions mélangées ni toutes les conditions de vivacité.


### Erreur de synchronisation du journal de propositions

La campagne `bft-signing-sync-20260924` ajoute huit scénarios Paris/Shanghai avec erreur EIO réelle de `fsync`/`F_FULLFSYNC` sur le WAL temporaire. Les réservations nouvelles et les journaux hérités sont testés avec conservation puis perte contrôlée des seuls octets non synchronisés. Aucune signature n'est produite après l'erreur ; les tentatives suivantes dans le processus fautif restent refusées. Après réouverture, le bloc et son empreinte sont présents ensemble ou absents ensemble, avec le marqueur requis pour le nouveau format. Les anciennes réservations restent intactes. La nouvelle réservation durable peut ensuite être signée et relue après une seconde réouverture. Six suites passent en 14,63 secondes avec les régressions existantes. Ces preuves portent sur macOS et une perte de suffixe WAL simulée ; elles ne remplacent pas un essai de coupure électrique physique.

### Estimation RPC par défaut sous charge bornée (2026-09-24)

Les deux nouveaux scénarios `bft_rpc_default_overload_*_network` passent sur Paris et Shanghai, successivement, avec quatre validateurs locaux et 48 clients sur douze tours chacun. Ils testent explicitement les estimations sans bloc demandé, désormais exécutées sur le candidat pending. Les seules erreurs admises sont les refus précis de capacité et d’expiration d’attente ; les erreurs d’exécution, de transport ou de contrôle font échouer le test.

Sur 2533 requêtes, les 1152 lectures d’identité réussissent. Les blocs continuent à être finalisés pendant la charge ; les certificats des transferts témoins sont vérifiés cryptographiquement, les effets appliqués une seule fois et les huit bases rouvertes auditées. Les 24 paires estimation/appel de récupération passent, ainsi que les devis dont la vérification avait dû attendre la fin de la charge. Preuves : `bft-default-estimate-load-20260924`. Ce résultat local et fini ne vaut pas endurance multi-hôte ni validation générale de production.

### Partitions, arrêts forcés et poids de vote : reprise actuelle (2026-09-24)

`bft-recovery-refresh-20260924` vérifie huit scénarios Paris/Shanghai avec les binaires actuels. Les votes réservés sont reproduits exactement après SIGKILL, les partitions se rejoignent sur un bloc commun, et exactement deux tiers du poids ne finalisent pas pendant la phase observée. Les deux essais de panne sous charge retrouvent leurs 720 transferts au total.

Après changement de poids via le contrat de staking, arrêt forcé des quatre cœurs et redémarrage, les nouveaux poids restent actifs. Huit journaux de vote sont remis à la hauteur certifiée suivante sans mutation de la chaîne ; seize historiques de pairs sont rouverts et audités dans les scénarios charge/changement de poids. La trace de l’interruption initiale reste conservée et le test alors incomplet n’est pas compté comme réussi. Cette qualification locale bornée n’établit ni coupure électrique réelle, ni endurance multi-hôte, ni couverture exhaustive des entrelacements BFT.

### SIGKILL pendant assemblage local d'une grande proposition (2026-09-24)

Les tests `bft_assembly_crash_paris` et `bft_assembly_crash_shanghai` couvrent
chacun trois arrêts forcés après accusé de réception de 1, 20 et 40 fragments
sur 40, avant `nano_finishBftImport`. Un processus C++ séparé utilise le vrai
RPC et RocksDB, signale le point atteint, s'arrête par SIGSTOP, puis le parent
vérifie sa terminaison SIGKILL. Aucun hook de panne n'est ajouté au runtime.

Après réouverture, le genesis et son état passent l'audit, aucun vote/proposition
fantôme n'est visible et les anciens identifiants de session sont refusés.
Un nouvel assemblage complet ne suffit pas à commettre : la proposition est
validée, puis les quorums signés de prevote/precommit permettent le commit.
Une seconde réouverture vérifie le bloc certifié, les dix reçus réussis et le
solde exact de dix unités du destinataire. Six cas et deux régressions d'import
passent ; manifeste `bftAssemblyCrashQualification` et preuves immuables dans
`build-cmake/qualification/bft-assembly-crash-20260924`.

Ce test appelle le dispatch RPC dans un processus fixture ; il ne qualifie pas
le transport libp2p interrompu pendant assemblage, le superviseur `nano run`,
une coupure électrique ni les courses multi-hôtes. Les sessions en mémoire
sont perdues à l'arrêt : la reprise exige un nouvel assemblage complet.

### Téléchargement BFT libp2p interrompu : reprise locale (2026-09-24)

`bft_fragment_download_crash_paris` et `bft_fragment_download_crash_shanghai`
réussissent respectivement en 82,12 et 89,72 secondes. Chaque réseau comporte
quatre cœurs fixture dans des processus distincts et quatre vrais relais
cpp-libp2p. Le relais destinataire signale l'accusé de réception du premier
fragment de proposition ; le test le suspend, vérifie que son cœur n'a pas
validé de proposition et reste au genesis, puis observe les terminaisons
SIGKILL du cœur et du relais. La base rouverte reste au genesis et passe l'audit.

Le redémarrage réutilise la base, le journal de vote et l'identité libp2p.
L'ancien identifiant d'assemblage est refusé avec l'erreur précise de session
inconnue/expirée. Après reconnexion, les quatre pairs s'accordent sur le bloc
contenant les dix transactions, tous leurs reçus et le solde de dix unités.
Au moins deux destinataires distincts téléchargent les 40 fragments de la
proposition finalement retenue. Les huit historiques finaux passent leur
audit après réouverture. Un bloc finalement accepté peut différer de la
proposition interrompue, selon le round : le test n'impose pas une finalisation
du premier proposant au détriment de la vivacité.

La seule addition au relais est une trace de progression aux fragments 1 et 20
sous `NANO_TEST_NETWORK_TRACE`. Aucun mécanisme de consensus n'est changé.
Manifeste `bftDownloadCrashQualification`, preuves dans
`build-cmake/qualification/bft-download-crash-20260924`. Cette campagne couvre
l'interruption après le premier fragment sur un seul hôte. Elle ne qualifie
pas tous les points d'interruption, le superviseur `nano run`, les coupures
électriques, l'endurance ni les réseaux multi-hôtes.

### Arrêt du relais après mort du superviseur (2026-09-24)

Un nouveau test `supervisor_parent_crash` reproduit un défaut de cycle de vie :
après SIGKILL du cœur `nano run`, son relais restait actif au-delà de six
secondes. Le destructeur du superviseur ne peut pas nettoyer un enfant quand
le processus parent reçoit SIGKILL.

Le superviseur transmet maintenant son PID direct via `NANO_SUPERVISOR_PID`,
en remplaçant toute valeur héritée. Le relais vérifie ce parent avant d'ouvrir
ses écouteurs, puis toutes les 250 ms sur sa boucle d'événements. Quand il est
réattribué à un autre parent après la mort du superviseur, il arrête l'hôte
libp2p et la boucle. La détection ne dépend pas de la disponibilité du RPC.
La cadence n'est pas une garantie de latence en cas de boucle bloquée. Les
relais lancés indépendamment sans ce paramètre gardent leur cycle de vie.

Quatre suites réussissent : contrôle des secrets et du PID transmis, arrêt
normal et propagation d'une panne du relais, mort forcée du parent avec
réouverture de la même base et identité, puis transferts de blocs certifiés
fragmentés avec perte d'un fragment/accusé de réception sur Paris et Shanghai.
Le test de parent périmé échoue avant ouverture des écouteurs. Preuves dans
`supervisorParentCrashQualification` et
`build-cmake/qualification/supervisor-parent-crash-20260924`.

Cette correction POSIX a été vérifiée sur cet hôte macOS. Elle ne qualifie pas
encore le crash d'une proposition BFT distribuée sous `nano run`, ni les pertes
d'alimentation, les réseaux multi-hôtes ou l'endurance.

### Grande proposition et panne sous nano run (2026-09-24)

`bft_managed_large_crash_paris` et `bft_managed_large_crash_shanghai` passent en
111,74 et 111,06 secondes. Chaque réseau lance quatre vrais `nano run` et leurs
relais cpp-libp2p. Les dix transactions signées de 900 000 octets de calldata
sont admises par HTTP RPC ; les nonces futurs sont envoyés avant le premier
nonce pour rendre l'ensemble éligible au même bloc. Les 40 soumissions par
réseau sont vérifiées (une duplication éventuelle par gossip doit correspondre
au hash exact déjà présent).

Après réception du premier fragment de proposition, un cœur reçoit SIGKILL.
Son relais orphelin s'arrête automatiquement. Sa base rouverte correspond au
dernier bloc observé, passe l'audit et ne contient aucun reçu des transactions
cibles. Le redémarrage réutilise base, journal de vote et identité réseau, sans
réinjecter manuellement la mempool perdue. Les quatre pairs retrouvent ensuite
le même bloc certifié de 18 003 850 octets, les dix reçus réussis et le solde
exact de dix unités. Au moins deux destinataires distincts ont reçu les 40
fragments de la proposition finale. Les huit bases finales rouvertes passent
l'audit. Les échecs de téléchargements intermédiaires restent dans les logs.

Manifeste `bftManagedLargeCrashQualification`, preuves dans
`build-cmake/qualification/bft-managed-large-crash-20260924`. Aucun changement
runtime dans cette campagne, qui utilise la correction du relais orphelin.
Le scénario sous superviseur est désormais couvert à ce point d'interruption
sur cet hôte ; les autres points, réseaux multi-hôtes, endurance et coupures
électriques restent ouverts.

### Coût du collecteur et ensemble votant immuable (2026-09-24)

`RoundVoteCollector` conserve maintenant l'empreinte de son ensemble votant et
réutilise sa table normalisée existante. Auparavant, chaque réception
reconstruisait cette table et recalculait l'empreinte de tous les membres.
Le parseur commun reste strict (taille, champs canoniques, domaine, hauteur,
round, appartenance) et chaque signature est toujours vérifiée, y compris
pour les doublons et conflits. Un nouveau collecteur reçoit un nouvel ensemble
lors du changement de contexte ; il n'y a pas de cache global partagé entre
hauteurs ou ensembles.

La campagne `vote-scaling-20260924` mesure 4, 16, 64, 256 et 1 000 validateurs.
À 1 000, recevoir les votes et contrôler la participation après chacun passe
localement de 1 691,85 à 270,79 ms ; recevoir à nouveau les mêmes votes passe
de 1 660,56 à 255,56 ms. La construction devient légèrement plus coûteuse
(0,59 à 1,59 ms). Ce sont des observations uniques avant/après sur cet hôte,
pas des garanties de débit. Le rapport conserve toutes les tailles, dont les
petites où le gain n'est pas établi. Les 1 000 messages pèsent 558 000 octets
avant comme après : le trafic n'est pas optimisé par ce changement.

Les tests vérifient le seuil strict à chaque préfixe de votes, les doublons,
les signatures invalides, membres étrangers, ensembles modifiés, mauvais
rounds, preuves d'équivoque bornées et acceptation du nouvel ensemble par un
nouveau collecteur. Les suites finality/consensus_round et les réseaux locaux
Paris/Shanghai avec changement de poids et SIGKILL passent également.

Manifeste `voteCollectorScalingQualification`. Le plafond 1 000 reste inchangé.
Les parcours de participation/certificat, le décodage externe du réacteur et
la rediffusion de listes de votes restent à optimiser/mesurer : aucune
complexité linéaire globale ni capacité réseau à 1 000 votants n'est démontrée.

### Contrôle de participation sans rescanner les votes (2026-09-24)

Le collecteur maintient deux poids de participation, prévote et précommit.
Seul le premier vote validé d'un signataire à cette étape augmente le poids ;
les doublons, signatures invalides et équivoques n'ajoutent rien. La somme est
préparée avant insertion dans la table de votes, puis publiée par échange GMP
sans allocation après insertion réussie. Les poids restent des entiers exacts.
`has_participation_quorum` compare ce poids au seuil strict, sans parcourir les
votes. Il ne certifie pas un bloc : les votes nil participent aux délais de
round, mais les contrôles de quorum de bloc restent distincts et inchangés.

Le test `vote_participation_model` compare ce résultat à un recalcul indépendant
à chaque étape. Avant et après modification, 776 comparaisons passent, avec
96 signatures invalides rejetées, 96 conflits, ordres direct/inverse, votes nil
et poids jusqu'à 2^200. Le benchmark conserve 4/16/64/256/1 000 membres. À 1 000,
5 000 paires de contrôles (10 000 appels) passent localement de 183,82 à 0,33 ms.
C'est une mesure unique d'une méthode ciblée, pas un gain de débit global.

Six suites finales passent : modèle, benchmark, finality, consensus_round et
les réseaux Paris/Shanghai qui empêchent la finalisation à exactement deux
tiers puis permettent le quorum pondéré suffisant. Le manifeste
`voteParticipationQualification` conserve les preuves. Le plafond, le trafic,
les recherches de certificats et les contrôles externes du réacteur restent
inchangés ; une capacité réseau à 1 000 votants n'est toujours pas démontrée.

### Diffusion des votes connus par pair (2026-09-24)

Le relais géré BFT v2 peut maintenant omettre les votes dont un pair connaît
les octets exacts : vote déjà reçu de ce pair, ou vote envoyé dans une requête
ayant obtenu une réponse réussie de même hauteur/round. Un envoi seul, une
réponse perdue ou un contexte différent ne valent pas accusé de réception.
Cette connaissance de transport ne donne aucun poids de vote et ne dispense
pas le cœur de ses contrôles. Une déclaration mensongère d'un pair peut modifier
ce qui lui est renvoyé, pas les votes que le cœur compte.

Le suivi stocke des empreintes SHA256, au maximum deux contextes de 2 000
empreintes par pair. Il remplace l'objet de suivi quand l'époque distante
change ; un callback d'une ancienne session ne peut pas alimenter le nouvel
objet. Une page complète est retransmise après cinq secondes même si tous ses
votes paraissent connus, pour réparer une perte de mémoire distante ou une
détection tardive de redémarrage. Les propositions et autres champs restent
transmis ; seul le tableau des votes est filtré. Le format v2 ne change pas.

Trois suites de transport et les deux réseaux gérés de grandes propositions
avec SIGKILL Paris/Shanghai passent. Les traces échantillonnées constatent
128 et 148 votes omis sur 80 messages différentiels par profil. Ce ne sont pas
les économies totales du réseau. Un cas unitaire synthétique de 2 000 entrées
connues passe de 1 112 999 à 110 octets de JSON entre deux réémissions complètes ;
il ne représente pas un réseau de 1 000 validateurs. Manifest
`bftVoteDeltaQualification`, preuves dans `bft-vote-delta-20260924`.

Le décodage/vérification du cœur, la construction des snapshots et les autres
parcours de votes restent distincts. Cette campagne ne prouve pas une complexité
globale O(N), la capacité à 1 000 validateurs, un déploiement mixte d'anciennes
versions ou la vivacité dans tous les scénarios byzantins. Le plafond reste
inchangé.
