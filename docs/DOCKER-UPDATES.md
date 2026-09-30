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
