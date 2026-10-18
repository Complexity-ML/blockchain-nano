# P2P natif privé : configuration et garde du pilote

La candidate Git `4186501012ff0185147d11a31214b1967fdf282e` conserve le
correctif de `6a70fc0e` : le plan frais configure le P2P natif sur `127.0.0.1`,
les conteneurs sur `0.0.0.0` et le RPC sur sa boucle locale existante.
Les modes historiques et le routage des bootnodes ne sont pas modifiés.

## Vérifications ciblées

Le nouveau test de topologie passe dans Docker AMD03. Son contrôleur global
reste FAILED : le garde de sources manque alors de deux helpers dans le montage.
Les compteurs complets restent 1 passé, 1 échoué, 70 ignorés ; seule la réussite
nommée est réutilisée. Le garde passe dans AMD06 : exit0, stderr vide, 1 test
passé, 5 ignorés. Il vérifie les sources figées puis trois refus attendus
(pilote non activé, transfert VPS non autorisé, transfert des preuves non
autorisé) avant seed, sortie ou réseau. Le diagnostic du stderr synthétique
ajouté après AMD04 ne change ni l’oracle ni les contrôles.

L’image AMD existante est réutilisée sans build ni pull, avec réseau Docker
désactivé, fichiers en lecture seule et limites de ressources. Le Node système
VPS et ses dépendances sont montés en lecture seule. Ses 19 fichiers binaires/
bibliothèques et les deux helpers sont tenus jusqu’au contrôle final. L’arbre des
ressources JavaScript Node est monté en lecture seule, sans qualification globale
de cette fermeture runtime. La candidate passe par bundle Git ; main VPS reste
à f218ee4e pendant les tests. Les imports relatifs sont contrôlés avant AMD06.

## Échecs conservés

Le premier essai Mac s’arrête avant test, Node absent (127). AMD01 refuse une
entrée de répertoire de l’archive avant Docker. AMD02 avorte à l’initialisation
Node faute de ressources externes (139). AMD03 manque les helpers docs ; AMD04
et AMD05 rencontrent un module de supervision absent du montage. Les reçus et
hashes des sorties restent conservés. AMD06 complète le montage depuis le même
snapshot Git et atteint les refus attendus. Aucun échec global n’est remplacé
par un statut réussi.

## Périmètre encore ouvert

Ce lot valide le générateur et les gardes négatives. Aucun nœud, signature,
seed ou nouvelle campagne n’a été lancé. Il ne qualifie ni l’écoute WAN réelle,
ni une restauration, ni la production. La restauration entière Native/Docker
Mac et l’échéance globale avec clôture restent à intégrer et qualifier. La
campagne03 a signé et échoué ; son autorisation est consommée et aucune04
n’est autorisée par ce lot. Paris/Shanghai reste activé, sans promotion EIP.
Base–Nano1:1 reste proposé/non adopté ; aucun jeton réel, vente, bridge ou
lancement Doppler n’est qualifié. La revue indépendante et les autres critères
normatifs restent ouverts.
