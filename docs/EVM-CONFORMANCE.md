# Qualification complète de l'exécution EVM

## Bilan courant — tests d'état seulement

| Groupe officiel épinglé | Fichiers | Cas concordants | Rejets vérifiés | Divergences / erreurs / non pris en charge |
| --- | ---: | ---: | ---: | --- |
| for_berlin | 127 | 1 963 / 1 963 | 150 | 0 / 0 / 0 |
| for_paris | 129 | 2 250 / 2 250 | 280 | 0 / 0 / 0 |

Ces groupes sont exécutés intégralement, sans filtres de cas, avec le pilote C++.
Les rejets comparent le motif exact mappé, l'état intégral inchangé, sa racine
et les logs vides. Trois témoins négatifs (motif erroné, rejet inattendu, racine
erronée) échouent tous comme attendu. Un message d'erreur inconnu n'est jamais
compté comme rejet conforme.

Le pilote demande explicitement la politique de signature Ethereum historique.
La politique est indépendante de la version EVM ; celle du réseau Nano exige
par défaut une signature liée à son identifiant, y compris avec Paris. Une
signature protégée pour une autre chaîne reste refusée dans les deux politiques.

Ce résultat ne couvre pas tous les tests d'état de l'archive, les tests de blocs,
les formats Engine, les versions plus récentes ni les propriétés du réseau BFT.
`forkConformanceProven` reste faux. Les sections ci-dessous conservent les étapes
du diagnostic ; leurs compteurs antérieurs ne sont pas le bilan courant.


L'objectif est une conformité vérifiable aux règles Ethereum versionnées, pas
une simple compatibilité avec quelques contrats. Nano conserve son consensus
BFT propre ; la conformité EVM ne signifie pas qu'il devient un client du réseau
Ethereum ni qu'il implémente son consensus.

## Corpus indépendant

La release officielle `ethereum/execution-specs`, `tests@v20.0.0`, est épinglée
avec URL, taille et SHA-256 dans `tests/fixtures/ethereum-official.json`.
L'archive a été téléchargée et son empreinte vérifiée. Un premier diagnostic natif du groupe Berlin est
désormais exécuté, avec le bilan incomplet détaillé plus bas. Les tests ne doivent pas être comptés comme passés simplement
parce qu'ils ont été téléchargés, parsés ou compilés.

Le pilote C++ devra recenser chaque cas/fork et comparer l'état attendu, les logs,
les erreurs d'exécution et les erreurs de validation. Un format ou fork non pris
en charge doit être comptabilisé explicitement comme couverture incomplète.
Aucun filtre ne doit rendre invisible un échec connu dans une qualification.
Les règles Paris servent de premier jalon, mais les forks plus récents font
partie du travail restant ; les définir et les activer explicitement est requis.

## Preuves acquises et limites

La suite locale de 37 tests passe après correction de la double facturation du
stockage et de REVERT. La comparaison Anvil Paris de quatorze transactions signées identiques
(ERC20 et stockage Solidity) concorde sur statuts, logs, état observé et gas. Ces résultats sont
décrits dans OPENZEPPELIN-QUALIFICATION.md ; ils ne prouvent pas la conformité
complète. Les historiques expérimentaux antérieurs ne sont pas migrés.

Les écarts restant à traiter incluent notamment la comptabilité
originale/courante de SSTORE et ses remboursements, ainsi que le traitement
économique des frais EIP-1559. La sémantique des forks,
précompiles, créations, auto-destructions et types de transactions doit également
être confrontée au corpus. La correction d'un cas ne dispense pas des autres.

## Conditions pour une déclaration de conformité

- Règles et forks annoncés explicitement, liés à l'identité/configuration du réseau.
- Corpus officiel épinglé entièrement inventorié et exécuté pour ces règles.
- Aucun échec, cas ignoré ou format non supporté dans le périmètre annoncé.
- Résultats reproductibles et comparaisons indépendantes des divergences corrigées.
- Déploiements Solidity/Hardhat et comportements des contrats OpenZeppelin testés.
- Qualification séparée du consensus, du transport, du stockage et des RPC.

La préparation d'un réseau public demeure non acquise tant que ces conditions
et les autres étapes du projet restent ouvertes.

## Premier diagnostic natif du groupe Berlin

`nano-ethereum-state-runner <fichier-ou-dossier>` exécute les transactions brutes
signées des fixtures d'état et compare la racine d'état, le hash des logs et le
reçu RLP lorsqu'il est présent. Il utilise les règles actuelles de Nano, encore
non versionnées par fork : un cas concordant ne prouve donc pas la conformité
complète au fork indiqué. Les cas à exception attendue sont pour l'instant
signalés non pris en charge plutôt que classés arbitrairement comme réussis.

Le groupe officiel `state_tests/for_berlin` contient 127 fichiers, soit 1 963 cas :
1 584 concordances, 170 divergences, 150 validations non prises en charge et 59
erreurs liées à l'identifiant de chaîne. Le processus retourne un code non nul,
ce qui est attendu tant que le corpus n'est pas entièrement compatible.
Le rapport détaillé local est conservé dans le cache des fixtures.

Les premiers correctifs couvrent la double facturation des accès aux comptes,
les frais de CALL/STATICCALL/DELEGATECALL, le retour du stipend de 2 300 gas lors
d'un appel refusé avant exécution, et le préchauffage des adresses et clés d'une
liste d'accès signée. Les précompiles 1 à 9 sont initialement chauds ; le coinbase
n'est plus préchauffé implicitement dans ces règles antérieures à Shanghai.
Onze fichiers officiels complets, soit 123 cas, sont conservés avec leur
licence CC0 et leurs empreintes. `ethereum_official_regressions` les exécute dans
CTest. Ce sous-ensemble ne remplace jamais le bilan du corpus complet.

## Corrections de création de contrats

CREATE et CREATE2 ne remboursent pas le stipend réservé aux appels avec valeur.
CREATE2 facture le hachage de chaque mot de code initial. La création refuse
le dépassement du nonce 64 bits du créateur ; une collision d'adresse consomme
le budget transférable au constructeur, conserve la réserve du parent et
avance le nonce. L'adresse de création reste chaude après une collision ou
un constructeur échoué. Les fixtures officielles de dépôt de code épuisant le
gas et de lecture EXTCODEHASH dans les constructeurs sont ajoutées entières.

Ces modifications changent potentiellement les racines d'état et nécessitent
une activation versionnée avant tout usage sur un historique existant.

## MODEXP et copies mémoire

Le tarif de MODEXP est corrigé selon EIP-2565 : carré du nombre de mots de huit
octets, itérations calculées avec la longueur et le premier mot de l'exposant,
division par trois et minimum de 200 gas. Le tarif est vérifié avant de lire
l'exposant complet et d'exécuter l'exponentiation. La limite locale de taille
reste à qualifier ; ceci ne prouve pas une prise en charge de tous les forks.

EXTCODECOPY et RETURNDATACOPY facturent maintenant trois gas par mot de 32
octets copié, en plus de l'accès éventuel et de l'extension mémoire. Des tests
couvrent 0, 1, 31, 32, 33, 64 et 65 octets, les adresses chaudes/froides et
les données renvoyées par le précompile identité. Les anciens vecteurs de
précompiles gardent leurs sorties mais utilisent les tarifs Berlin pour MODEXP.

Le diagnostic complet passe de 1 218 à 1 584 concordances sans transition d'un
cas concordant vers un échec. Les 150 validations non prises en charge et les
59 erreurs de chaîne demeurent ouvertes. Aucun contournement du contrôle de
chaîne n'a été ajouté au moteur ni au pilote.

Références : https://eips.ethereum.org/EIPS/eip-2565 et
https://eips.ethereum.org/EIPS/eip-211 ; instructions environment.py du fork
Berlin dans ethereum/execution-specs pour EXTCODECOPY.

## Comptabilité SSTORE transactionnelle

Le moteur conserve la valeur originale de chaque slot modifié pendant toute
une exécution, y compris au travers de CALL/DELEGATECALL et de leurs annulations.
Il applique les transitions original/courant/nouveau de EIP-2200, les coûts
chaud/froid de EIP-2929 et le remboursement de suppression de 4 800 gas de
EIP-3529. Les compteurs par appel peuvent être négatifs ; ils sont fusionnés
seulement si l'appel réussit. La protection gasleft <= 2 300 est vérifiée avant
le supplément d'accès froid. Le cache original est réinitialisé à chaque transaction.

La table de transitions est testée sur des slots froids et préchauffés, ainsi
que sur des appels imbriqués réussis et annulés. StorageMeter.sol est déployé
sur Nano et Anvil Paris : écritures successives, remises à zéro, restauration
et appels externes annulés concordent sur gas, statuts, logs et valeur stockée.
Les quatorze transactions incluent les trois cas ERC20 antérieurs.

Le bilan Berlin reste 1 584 concordances, 170 divergences, 150 validations non
supportées et 59 erreurs de chaîne, sans changement de statut des cas. Ce n'est
pas une conformité Berlin : ses remboursements (15 000 et plafond de moitié)
diffèrent de ceux appliqués ici. La sélection de règles par fork reste requise.
Références : https://eips.ethereum.org/EIPS/eip-2200,
https://eips.ethereum.org/EIPS/eip-2929 et https://eips.ethereum.org/EIPS/eip-3529.

## Versions de règles EVM explicites

Berlin et Paris désignent des versions de règles d'exécution, pas des réseaux
ni un fork de l'historique Ethereum. Nano reste une chaîne indépendante.
Le pilote officiel sélectionne explicitement Berlin ou Paris, et signale les
autres versions comme non prises en charge. Le diagnostic Berlin courant est
1 728 concordances, 26 divergences, 150 validations non prises en charge et
59 erreurs de chaîne sur 1 963 cas ; les bilans précédents ci-dessus concernent
le moteur expérimental non versionné. La conformité complète n'est pas acquise.

`config.evmFork: "Paris"` est une option pour un nouveau genesis Nano. Elle est
liée à son hash par un domaine distinct ; aucun genesis livré n'est modifié.
Sans cette option, le comportement expérimental antérieur reste sélectionné.
Berlin est réservé au moteur de tests, pas accepté comme configuration réseau.
Les versions sont propagées aux transactions, aux simulations RPC et aux
lectures du contrat de staking. Le profil Paris active notamment le brûlage
des frais de base, le refus des expéditeurs ayant du code et du préfixe de code
0xEF. PUSH0 reste indisponible dans Berlin et Paris ; BASEFEE seulement dans Paris.
Les remboursements Berlin sont séparés de ceux de Paris. Les règles encore
incomplètes, notamment SELFDESTRUCT et certaines validations, restent ouvertes.
Il n'y a pas encore de calendrier de transitions de versions sur une chaîne existante.

Le banc Hardhat peut sélectionner Paris uniquement pour son genesis temporaire
avec `NANO_TEST_EVM_FORK=Paris`. Cette option ne touche jamais le genesis livré.

## Destruction différée et collisions de création

Dans les versions Berlin et Paris explicites, SELFDESTRUCT garde le code et le
stockage accessibles jusqu'à la fin de la transaction, transfère le solde,
charge le bénéficiaire froid et sa création éventuelle, et annule la suppression
si l'appel est annulé. Berlin accorde son remboursement une seule fois par
compte ; Paris ne l'accorde plus. Le dépôt de code d'un constructeur ne doit
pas ressusciter un compte ainsi supprimé. Les règles expérimentales non
versionnées conservent encore leur ancien comportement.

La création est également refusée si l'adresse possède déjà du stockage,
selon la règle rétroactive EIP-7610 utilisée dans ce corpus épinglé. Ce contrôle
est appliqué aux transactions de déploiement, CREATE/CREATE2 et simulations RPC.

Le diagnostic Berlin actuel est 1 754 concordances, aucune divergence parmi
les cas exécutés, 150 validations non prises en charge et 59 erreurs de chaîne.
Le pilote retourne toujours un échec global : ces 209 cas ne sont ni validés ni
exclus du bilan. Les formats/forks du reste du corpus sont encore à qualifier.
Trois fixtures officielles complètes supplémentaires couvrent les destructions
dans les constructeurs, les sous-appels et les collisions avec stockage.
Référence : https://eips.ethereum.org/EIPS/eip-7610 ; instruction SELFDESTRUCT
Berlin dans ethereum/execution-specs.

### Transactions historiques et environnement des fixtures

Le profil Berlin réservé au diagnostic accepte les signatures historiques v=27/28
sans identifiant de chaîne ; il vérifie toujours la signature et refuse les
transactions protégées signées pour une autre chaîne. Les profils réseau Nano
(expérimental et Paris) conservent leur exigence de signature liée à la chaîne.
Cette différence de politique reste une limite explicite à une affirmation de
conformité Ethereum exhaustive du réseau Nano.

Le pilote fournit pour BLOCKHASH la convention des state tests, Keccak-256 du
numéro de bloc écrit en décimal, comme vmTestBlockHash dans les tests Geth.
Aucun historique synthétique n'est ajouté à l'exécution du réseau Nano.
Le bilan courant est désormais 1 813 concordances, zéro divergence, zéro erreur
et 150 validations non prises en charge. Le groupe contient toujours 1 963 cas
et le code de sortie reste non nul en raison de ces validations incomplètes.
Référence : https://github.com/ethereum/go-ethereum/blob/master/tests/state_test_util.go.
