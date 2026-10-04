# Mise à jour Docker d'une version candidate

Cette procédure concerne une candidate déjà qualifiée sur son architecture et
son réseau. Elle ne lance pas un réseau public et ne certifie pas la production.
Le Compose par défaut reste un exemple testnet sans validateurs : il ne fournit
pas la configuration Beacon/Capella complète d'un réseau de production.

## Préparer sans interrompre le nœud

1. Récupérer la révision revue par Git, en conservant le checkout de la version
   en service. Compiler une seule fois par architecture et réutiliser les caches.
   Ne pas remplacer une image en service sous son tag pendant cette préparation.
2. Assembler et qualifier une image d'exécution comprenant les **deux** binaires
   actuels `nano` et `nano-p2p`. Un volume de compilation à jour monté dans une
   image de dépendances n'est pas encore une image de déploiement candidate.
3. Conserver un manifeste revu indépendamment : identité locale immuable
   `sha256:…`, révision complète des sources, architecture, empreintes des deux
   exécutables, des fichiers genesis/node/profil et des preuves de qualification.
   Les clés restent hors de l'image, du manifeste et de Git. La provenance de la
   construction, la qualification du profil et la revue indépendante restent des
   vérifications séparées ; une étiquette Docker ne prouve pas leur réalisation.
4. Exécuter le précontrôle en lecture seule :

   ```sh
   node scripts/operations/check-candidate.mjs candidate.json
   ```

   Le format est `nano-candidate-v1`, avec `image`, `sourceRevision` (40 hex),
   `architecture` (`arm64` ou `amd64`), `maximumArtifactBytes` (budget total),
   `executables` (`nano` et `nano-p2p`, SHA-256 sans préfixe), et `files` : liste
   de `{kind, file, sha256}`. Les rôles `genesis`, `node`, `profile` et au moins
   un `evidence` sont obligatoires ; les chemins sont relatifs au manifeste ou
   absolus. Les fichiers réguliers sont hachés, pas leurs données de chaîne.
   Les preuves sont vérifiées en intégrité seulement : le script ne déduit pas
   un verdict de conformité de leur présence et ne valide pas leur contenu.

   Le script vérifie le daemon Linux et son architecture, l'identité et
   l'étiquette de source de l'image, puis les binaires et leurs bibliothèques.
   Ses probes sont des conteneurs éphémères sans réseau, sans montage de chaîne,
   racine en lecture seule et ressources limitées. Ils n'exécutent ni `nano run`
   ni des fonctions de signature. Un succès renvoie `productionQualified:false`.

## Arrêt, conservation et promotion

5. Vérifier l'espace disponible et la capacité de reprise avant l'arrêt. Préparer
   la configuration exacte du réseau retenu, y compris les montages du profil,
   de l'ancre et des secrets. Capella exige un RPC local au conteneur ; utiliser
   Docker exec/SSH pour l'exploitation. Ne pas transposer aveuglément les ports
   et le `rpc_host` du Compose testnet. Aucun changement de genesis sur une base
   existante, ni migration implicite de format.
6. Arrêter proprement **l'unique instance qui signe avec ces clés**. Vérifier
   l'état terminal réel, le code de sortie, l'absence d'OOM et le reçu d'arrêt
   sain. Conserver indépendamment l'engagement de chaîne et celui de chaque
   journal anti-double-signature. Une restauration depuis un témoin seulement
   n'authentifie pas la fraîcheur si ce témoin a lui aussi été restauré ancien.
7. Sauvegarder l'ensemble arrêté : chaîne RocksDB, témoin de chaîne, tous les
   journaux/témoins des signataires, identité P2P, genesis et configuration.
   Vérifier les empreintes puis l'audit canonique non mutateur avec l'ancre et
   l'engagement indépendants. [Reprise Capella](CAPELLA-RECOVERY.md) et
   [restauration qualifiée](qualification/capella-restored-validators-20260930/attempt-b/manifest.json).
   Respecter le budget d'horloge local de reprise ; ne pas remettre l'horloge ou
   les compteurs à zéro pour contourner un refus.
8. Seulement après ces vérifications, sélectionner l'identité approuvée :

   ```sh
   export NANO_IMAGE='sha256:IDENTITE_APPROUVEE'
   docker compose up -d --no-build --pull never
   ```

   Cette commande suppose un Compose/configuration **adapté au réseau retenu**.
   Elle ne doit pas servir à lancer le Compose testnet par défaut en production.
   `--no-build --pull never` empêche une reconstruction ou un remplacement par
   téléchargement pendant la promotion. `NANO_IMAGE` doit correspondre au
   manifeste approuvé. Ne jamais utiliser `down -v` ni lancer deux instances
   avec les mêmes données ou les mêmes clés.

## Vérifier et revenir en arrière

L'ouverture RPC ne suffit pas : vérifier identité de chaîne/P2P, engagements,
reprise des duties, finalité et cohérence avec les autres nœuds. Utiliser le
[moniteur](FINALITY-MONITOR.md), avec l'historique conservé. Pour une candidate,
exiger également la fenêtre stricte de trois avancées communes liée à l'exécution.
Conserver les logs, la révision, les empreintes, le checkpoint et les résultats.
Un seul conteneur de validation actif ; conserver les bits de la version
précédente récupérables jusqu'à acceptation, sans accumuler des images inutiles.

Un retour logiciel ne doit **jamais** restaurer d'anciens journaux de signature.
Si la candidate a signé ou réservé des duties, ses journaux/témoins les plus
récents restent la référence. Arrêter la candidate, conserver ses engagements
et vérifier que l'ancien logiciel lit le format actuel avant de le redémarrer.
Si cette compatibilité n'est pas démontrée, maintenir le signataire arrêté et
préparer une correction vers l'avant. Ne pas restaurer silencieusement l'ancienne
sauvegarde complète pour obtenir un démarrage vert : les autres nœuds peuvent
avoir reçu les signatures émises après cette sauvegarde.

## État de qualification

Onze tests du précontrôle passent ; un véritable appel Docker refuse l'image
locale de dépendances non candidate avant tout probe de binaire. Compose conserve
l'identité immuable sélectionnée dans sa configuration rendue. Aucun cycle de
promotion/retour arrière d'une image candidate actuelle n'est encore qualifié.
Packaging courant, régression complète, livraison des alertes, compatibilité
future des formats et revue indépendante restent ouverts.

## Assembler depuis les binaires déjà compilés

`scripts/operations/package-candidate.sh` reçoit six arguments obligatoires : ID immuable de
la base runtime locale, ID de l’image locale de compilation, volume compilé,
révision compilée complète et SHA-256 de chacun des deux exécutables. Un septième
argument facultatif sélectionne le sous-répertoire qualifié du volume (par exemple
`production-evm-c10426b2-20261001`) ; sans cet argument, le chemin historique
`/build` est conservé. Le binaire `nano` et `production-source-revision.txt` doivent
se trouver dans ce répertoire. Sans huitième argument, `nano-p2p` reste dans
`/build/network/` pour les volumes historiques. Le huitième argument sélectionne
explicitement `/build/<argument>/network/` ; pour la candidate cohérente, fournir
`coherent-efbeb9bc-arm64` (ou `amd64`) en septième **et** huitième arguments.
Le dossier réseau, son parent direct et les trois fichiers doivent être réguliers,
sans symlink ; aucun fallback vers le P2P racine si le P2P sélectionné manque. Ne créer
le marqueur qu’après qualification et liaison vérifiée des sources/binaires. Les
empreintes doivent provenir d’une preuve conservée et revue séparément. Le
script exporte uniquement les deux binaires et leur marqueur depuis un volume
en lecture seule, contrôle le marqueur et les architectures, puis vérifie les
empreintes dans l’image assemblée. Le marqueur seul ne prouve pas la provenance.

BuildKit exige un nom dans FROM : une référence locale dérivée de l’ID est
contrôlée avant/après, et le log doit confirmer sa résolution vers cet ID.
Le script ne lance ni apt ni compilation. Il imprime l’ID final, sans remplacer
de service ni d’image en service. Son image démarre sur `--help`. Les fichiers
genesis/node historiques hérités de la base ne sont pas des entrées approuvées :
monter explicitement la configuration réseau qualifiée avant une campagne.
[Assemblage des deux architectures et limites](qualification/candidate-images-20260930/README.md).

## Limite vérifiée du retour arrière

L’image VPS historique `c2ec4d7` refuse la fixture Beacon Shanghai actuelle
(exige Paris). Ne pas la sélectionner comme cible de retour. Le remplacement
à image candidate identique est qualifié avec engagements de chaîne et de
21 journaux, reprise de duties et trois avancées communes. L’ancienne
configuration de reprise échoue sur une copie du nouvel état après signature ;
conserver les derniers engagements indépendants et les derniers journaux.
Un vrai retour entre versions demande encore une cible compatible qualifiée.
[Preuves et limites](qualification/pinned-replacement-20260930/manifest.json).

## Correction de sélection du P2P isolé — 4 octobre 2026

Le packaging peut désormais sélectionner séparément le dossier qualifié du cœur
et celui contenant `network/nano-p2p`. Les appels historiques à six/sept arguments
conservent leur comportement. Dix tests de précontrôle et trois tests exécutant
le vrai shell d’export passent. Une fixture P2P racine ancienne ne remplace pas
le fichier sélectionné ; fichier absent et symlinks sont refusés.
Cette preuve ne remplace pas l’assemblage réel, la vérification des bibliothèques,
la qualification du manifeste ni le cycle de mise à jour/restauration actuel.
[Preuves et périmètre](qualification/coherent-package-paths-20261004/README.md).
