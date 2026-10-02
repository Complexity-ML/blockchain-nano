# Régression cohérente AMD64 0275b1af — contrôleur en attente

Contrôleur **Mac session77603 / VPS prepare PID531568** confirmé vivant. Attend la sélection AMD64 en cours : 16 noms/résultats exacts, zéro échec/skip, TIMEOUT originaux et sources/neuf programmes/sept entrées statiques avant/après inchangés. Aucun CTest complet démarré à cette observation. Si le gate échoue, retour non nul, sortie préservée, aucune relance ni allongement de délai.

Après ce gate uniquement : compilation de **toutes** les cibles, dans le nouveau répertoire `/build/full-coherent-0275b1af-amd64-20261002`, sans écrire les sorties déjà qualifiées, ni recompiler le P2P `/build/network/nano-p2p`. Source figée `/home/ubuntu/nano-checkpoint-row-dump-candidate-20261002`, CMake Release/configuration identique au candidat sélectionné, image AMD64 `sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c`, volume `nano-linux-sync-build`, deux CPU/3 GiB, réseau none, root readonly/capdropALL/no-new-privileges. Aucun build d'image. Ce build séparé protège les sorties sélectionnées tout en produisant les exécutables additionnels requis par la suite complète.

Les **426** cas enregistrés, commandes directes et helpers externes (`nano`, `nano-altair-probe`, `nano-finality-crash-fixture`, P2P) seront inventoriés ; TIMEOUT explicite requis, propriétés de skip interdites. Node est utilisé par le contrôleur **sur Mac** (VPS/builder sans Node), pour vérifier le registre complet avant le début de CTest, puis l'exactitude des résultats JUnit et le comptage EEST après terminaison. Les contrôles Node facultatifs non enregistrés sur Linux ne sont pas prétendus exécutés par cette campagne ; leurs contrôles natifs sont archivés séparément.

Sources, exécutables/auxiliaires et archives/librairies sont manifestés avant/après. Rapports VPS `/home/ubuntu/nano-full-coherent-0275b1af-amd-reports-20261002`, contrôleur Mac `/tmp/nano-full-coherent-0275b1af-amd-host-reports-20261002`, pilote `/tmp/nano-full-coherent-0275b1af-amd-driver-20261002.log`. Toutes les sorties terminales, y compris échecs, doivent être conservées. Code runtime déjà obtenu par Git ; SCP transmet uniquement ces contrôles/tests et rapports.

Cette préparation ne constitue pas une qualification, ne remplace pas le timeout AMD2d360,05 historique, et ne ferme ni la reprise/finalité WAN, les partitions, le cycle validateur, la charge/mémoire du cache, les profils EIP non activés, le bridge/vente ou la revue indépendante.

## Contrôleur terminal : gate refusé

Driver session77603 **exit1**, prepare PID531568 terminé. Le contrôle `ctest-exit==0` refuse le parent en sortie8 ; aucun répertoire de compilation/régression complète créé par ce contrôleur avant ce gate. Parent15/16 terminal archivés dans coherent-row-candidate-0275b1af-20261002. Aucun CTest complet AMD du candidat exécuté ; la préparation n’est pas requalifiée en réussite.
