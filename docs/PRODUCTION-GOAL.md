# Objectif : Nano C++ de niveau production

Décision utilisateur du 21 septembre 2026 : le portage doit corriger ce qui
n'était pas adapté à la production dans le Python. La référence Python reste
intacte. La parité avec ses défauts n'est plus un critère de réussite.
Le prototype ne doit plus imposer de dépendance d’exécution ni de pilote de
tests Python à Nano ; les tests du projet sont natifs C++.

## Cible retenue

Un réseau public Nano en C++, avec EVM native, cpp-libp2p, RocksDB et staking
par transactions signées vers un smart contract. Les nouvelles règles corrigées
priment sur la reproduction exacte des historiques Python divergents. Toute
rupture de format ou de règles doit être versionnée et documentée ; les données
existantes ne doivent jamais être réécrites silencieusement.

## Critères de clôture

1. Consensus et blocs authentifiés : signature du producteur, séparation des
   domaines de signature par chaîne et version, contrôle de sélection, rejet
   du rejeu et des altérations. Définir et vérifier les règles de concurrence
   de branches, de finalité et de sanctions applicables au réseau public.
2. Exécution déterministe : règles EVM et gaz définies, rejeu identique sur les
   pairs, invariants de soldes et de dépôts, annulation atomique des échecs.
3. Staking : inscriptions, ajouts et retraits signés, règles de verrouillage et
   de sortie explicites, protections adversariales et revue de sécurité.
4. Persistance : atomicité, récupération après arrêt brutal, détection des
   corruptions, sauvegarde/restauration et évolution de schéma vérifiées.
5. Réseau et RPC : contrôle des entrées, limites de ressources, résistance aux
   pairs malveillants, synchronisation vérifiée, séparation des clés et des
   interfaces publiques, comportement opérationnel documenté.
6. Livraison : compilation reproductible, dépendances et licences recensées,
   tests fonctionnels et adversariaux, essais multi-nœuds prolongés, procédures
   d'exploitation et limitations connues. Toute validation externe nécessaire
   mais non obtenue reste explicitement un point ouvert.

7. Entrée et sortie des actifs : bridge bidirectionnel qualifié vers au moins
   une chaîne choisie, avec identification des chaînes et contrats, protection
   contre le rejeu, règles de finalité et modèle de confiance explicites.
   Vérifier la conservation de la couverture des actifs représentés, les retraits,
   les doublons, les pannes de relais, les réorganisations admises par le modèle
   et la procédure de reprise. Le bridge et la pool de liquidité sont distincts :
   traverser le bridge ne fournit pas automatiquement de liquidité à une pool.
   La chaîne partenaire, les actifs et le mécanisme de vérification restent à choisir.
8. Pool de lancement : permettre l’échange de NANO (ou de sa représentation
   bridgée) contre un actif de contrepartie choisi. Définir le réseau hôte et
   le DEX, puis vérifier l’ajout/retrait de liquidité, les swaps, les décimales,
   les autorisations et les limites de slippage. Vérifier le parcours complet
   bridge vers pool puis retour, sans émission non couverte ni double retrait.
   La liquidité initiale et le prix initial nécessitent des montants décidés
   explicitement ; aucun apport de fonds réel n’est autorisé par le développement.

## Positionnement de lancement

Décision utilisateur : bridge bidirectionnel et pool de lancement. Le produit
ne cible pas un coffre-fort ou une fonction de gestion de trésorerie. Aucun
contrat de coffre supplémentaire n’est requis par ce positionnement.
Le réseau hôte de la pool et la chaîne partenaire du bridge restent à choisir.
Le bridge facilite le passage entre réseaux ; seuls les dépôts de liquidité
alimentent la pool. La présence du bridge ne garantit ni demande ni cotation
sur un service tiers.

Des suites vertes ne suffisent pas à déclarer le réseau prêt pour la production.
Aucun déploiement public ni engagement financier n'est autorisé implicitement
par cet objectif de développement.

## Priorité immédiate

Authentifier les blocs PoS et vérifier le rejet des blocs falsifiés sur le RPC,
l'import, le stockage et le réseau. Le format C++ versionné et les producteurs signés sont maintenant intégrés
(voir `BLOCK-SIGNATURES.md`). Les genesis livrés activent cette règle ; leurs versions historiques sont
conservées séparément (voir `GENESIS-VERSIONS.md`). La protection locale contre la double signature est
implémentée avec les limites décrites ci-dessous.

## Audit de stockage

La commande `nano audit` rejoue la chaîne canonique depuis le genesis dans une
base temporaire et compare états, registres de validateurs, reçus et index.
Les tests couvrent une preuve historique corrompue et un reçu altéré. Voir
`STORAGE-AUDIT.md` pour le périmètre exact ; cela ne remplace ni la finalité,
ni les essais de perte d’alimentation, ni une revue indépendante.
Le test du validateur distant couvre désormais un SIGKILL après réservation
et signature, avant réception du bloc : la reprise conserve le même bloc et
la même preuve. Les pannes disque et pertes d’alimentation restent à éprouver.

### Qualification OpenZeppelin demandée

Couvrir tout le catalogue OpenZeppelin, y compris ERC721/ERC1155 et leurs
extensions, contrôle d’accès, cryptographie, gouvernance, proxies et compositions
de marché. La première version figée est Contracts v5.0.2. Le suivi par composant
est décrit dans `OPENZEPPELIN-QUALIFICATION.md` ; compilation et quelques scénarios
réussis ne constituent pas une validation complète. Le paquet séparé
Contracts Upgradeable doit également être inventorié avant d’en revendiquer la
couverture. Les contrats propres aux marketplaces, au bridge et à l’AMM demandent
des tests de composition et de sécurité supplémentaires.
