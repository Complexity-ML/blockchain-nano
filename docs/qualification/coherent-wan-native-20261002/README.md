# Propriétaire Mac natif — compilation et sélection terminales

Source isolée 554ebc3a (`/tmp/nano-coherent-wan-paths-20261002`), runtime `src/include/network/third_party` strictement identique à 2d2764de, diff vide conservé. Les deux différences du checkout portent sur le contrôleur WAN et l'enregistrement CTest. Compilation CMake Release de huit cibles nécessaires, deux tâches, répertoire distinct `build-release`. Session observée active 28891 ; rapports `/tmp/nano-coherent-wan-native-build-reports-20261002`.

C'est le propriétaire macOS natif explicitement demandé par le scénario à trois nœuds. Les builds Docker Linux ARM64/AMD64 existants sont réutilisés pour les deux autres propriétaires ; ils ne peuvent pas fournir un exécutable macOS. Aucun build d'image ajouté. P2P Mac préexistant réutilisé et hash avant/après exigé ; aucun processus ou volume des campagnes Docker touché.

Contrôle build : sources complètes avant/après, identité Git exacte, huit hashes de programmes après compilation ; provenance `production-source-revision.txt` écrite seulement après compilation réussie et comparaison des sources. Cette provenance identifie le checkout de compilation, **pas une qualification** ni une readiness production.

Contrôle test préparé mais **non lancé** : huit cas natifs, registre CTest déjà vérifié, limites originales : core, rotation empty aggregate, ascendance helper, checkpoint JSON, SSZ hex, hex différentiel, chemins WAN, manifeste du contrôleur. Il exige la fin du build et l'intégrité ; sources/programmes/objets/static libs et dépendances dynamiques absolues existantes via `otool` avant/après. Bibliothèques système intégrées au cache dyld et absentes sous forme de fichier ne peuvent pas être hashées par ce contrôle. Aucun résultat natif encore acquis. Ce périmètre ne démontre ni activation réseau, ni restauration, ni reprise des duties.

Les deux campagnes Docker 2d2764de restent actives. Sur AMD64, activation 346,72 s / 360 s puis activation après redémarrage 350,86 s / 360 s passent ; faible marge, autres cas encore pendants. Aucun runtime candidat promu dans main.

## Résultat terminal suivant

La compilation termine avec build/controller/source compare **0**, sources avant/après identiques, P2P inchangé et huit programmes identifiés. Nano Mac : SHA-256 `8dd333a646feab19a853ede3ff386668f93eb404181e19ae1af97f3ebdff845d`. Provenance checkout `554ebc3a` ; runtime égal à 2d2764de confirmé séparément, aucun numéro de révision Docker changé.

Sélection CTest native **8/8**, zéro skip, **2,71 s** : core, self-test aggregate vide, deux nouveaux contrôles de manifeste/chemins dans CTest et quatre helpers de sérialisation/ascendance/hex. Registre exact avec commandes et délais originaux, résultat XML et contrôle terminal concordent. **198 fichiers d'entrée hashés avant/après**, objets et bibliothèques statiques/dépendances dynamiques absolues existantes, programmes, Node et P2P ; sources avant/après identiques. Logs/CMake/provenance et manifestes archivés, grands fichiers gzip.

Ce n'est pas une régression native complète, ni un replay cryptographique de toute la chaîne, ni une rotation réelle, ni une campagne WAN. Les campagnes Docker ARM64 complète/AMD64 sélectionnée et audit arrêté AMD64 restent indépendantes et pendantes. Prochaine porte : résultats AMD64 puis restauration entière avec témoins de signature et reprise réelle des duties, finalité observée indépendamment. Aucun critère de production global fermé.
