# Finalité : primitives et intégration restante

## État actuel

`finality.hpp` et `finality.cpp` fournissent des votes signés et un vérificateur
de certificat pondéré. Ils ne sont pas appelés par le consensus actif et ne
modifient ni le format des blocs ni les genesis. Le stockage de blocs certifiés est disponible pour les essais locaux décrits
ci-dessous, sans activation du protocole réseau ; les tags RPC `safe` et `finalized` restent indisponibles.

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

## Intégration requise avant activation

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
