# Erreur de synchronisation WAL pendant une réorganisation de dépôts

## Injection

Le scénario AB/BA produit les deux branches complètes jusqu'à la réindexation.
Le test identifie le vote qui change effectivement la tête de l'observateur
vers BA. Il prépare une base isolée avec l'état précédant ce vote, puis un
sous-processus rouvre le propriétaire et injecte EIO sur fsync/F_FULLFSYNC du
WAL RocksDB ciblé. La bibliothèque d'interposition est uniquement liée au
binaire de test macOS. Le sous-processus termine par `_exit`, sans fermeture
ordonnée des objets.

Après l'erreur, cinq accès doivent échouer : état du reactor, résolution des
validateurs, tête du propriétaire, état EVM et dernière entrée du stockage.
Le suffixe WAL est conservé dans un cas ; dans l'autre, il est tronqué à la
longueur relevée avant l'écriture, uniquement dans le répertoire temporaire.

## Critères et résultat

Le test passe en **28,61 s** (28,62 s CTest) :

- WAL conservé : enveloppe durable complète du nouvel état, index 65 et vote
  signé archivé. Le rejeu est idempotent et conserve l'état attendu.
- Suffixe perdu : enveloppe complète de l'ancien état, index 64 et absence du
  vote signé. Le rejeu publie l'état attendu, avec index 65.
- Les racines EVM et têtes d'exécution correspondent au Store restauré dans
  les deux cas, sans mélange des branches.

Les enveloppes attendues sont produites avant injection dans une base de
référence par publication normale du même vote. Leur état consensus est
également comparé à la transition pure. Le token retrouvé est accepté dans
ce banc seulement après comparaison exacte avec l'issue attendue ; cette
procédure ne définit pas une politique de confiance de production.

## Échec initial conservé

Le premier run échouait en 22,64 s : il comparait un snapshot consensus seul
à l'enveloppe durable contenant aussi le vote signé. Le banc utilise désormais
l'enveloppe de référence complète et vérifie séparément l'archive du vote.
Aucun contrôle de consensus ni persistance n'a été assoupli, aucun code de
production changé. Logs initiaux, JUnit et empreintes finales sont joints ;
seuls les espaces finaux des logs sont normalisés.

## Limites

macOS local, pas de panne électrique physique, pas de Linux/VPS, HTTP/P2P/WAN
ou partition. Les enveloppes initiales sont des ancrages de test connus ; pas
de récupération avec témoins indépendants dans cette nouvelle campagne.
Le test cible le commit du vote de réorganisation, pas l'ensemble des points
possible d'interruption, ni la corruption matérielle arbitraire. L'activation
après reorg reste couverte par sa campagne distincte, non rejouée ici.
Aucun EIP, profil ni objectif production clôturé ; aucun fonds réel.
