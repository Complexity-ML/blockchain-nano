# Reliaison AMD des programmes au Core RPC qualifié — proposition source

`host.py` dérive du contrôleur public `current-amd-runtime-relink-source-20261016/host.py` (1368ec3354b64f5651e969fffb554d36e0687913994219f1c339c26fcb7933d5, 25721 octets). Le delta complet est `source.patch`. Il importe seulement les définitions du module public f28ae7a55fadc9fc9bfe04a70c18f991e7440d4c4ab1c4d860fd7aca872fc350/52822, depuis ses octets authentifiés, sans exécuter son ancien main ni produire de cache Python. L'auteur n'a lu aucune autorité privée et n'a exécuté ni Python de projet, ni worker, compilateur, Git, SSH, Docker ou SDK. Seuls les traitements de texte, SHA et parsing AST de la proposition ont été effectués.

## Commande prévue pour Root

```
python3 host.py --worker /chemin/neuf/worker.pl --output-root /home/ubuntu/current-amd-rpc-runtime-relink-20261017-01
```

Le worker doit être exactement f60a63c70d5d2fab65647917458d9829f6966df840275c2d778291aaef8b3154/27010, gelé sous `../current-linux-rpc-runtime-source-20261017/worker.pl` ; son manifeste source est 9e944a6a3e3bb71771f752f81a76b6f7544c2f149f8e18f4b95e3cb7aed65446/2183. Les formats et le sélecteur `--current-core-runtime-relink` sont conservés. Aucun worker n'est recopié dans ce dossier. Root garde le transport de ces sources publiques et vérifie leurs SHA/octets avant exécution.

Le namespace de sortie doit être absent, avec parent physique possédé UID1000. Il ne peut chevaucher le dépôt, les anciennes autorités sélectionnées, les producteurs Main/Tool ou le namespace Core RPC courant. La création du namespace précède tout rapport ; aucun dossier ancien n'est modifié. Le contrôle Linux UID/GID1000 reste obligatoire. Les entrées originales sont strictement RO et refencées à la fin, y compris après refus.

## Changement d'autorité Core, sans récupération historique

L'archive courante est `/home/ubuntu/rpc-context-amd-root-20261016-01/outputs/libnano_core.a`, SHA2678b628528295dd7d729e8917572d3076a1068399e58b12f97fa3849bb497bd, 40805844 octets. Le reçu public hôte `/home/ubuntu/rpc-context-amd-root-20261016-01/result-public.json` doit être ad0fd6a977b3450932532fd962c42718e8ed52cb2a8720bb9f64b88baec08a00/2188. Il engage le binding `outputs/software-admission.private.json` f1202685a2880c62ad81b3d007938a64df1960ec6ddb7cbdd87e89604cb40ed0/805044, format `nano-rpc-simulation-context-software-v1`.

Root lit seulement ces documents sélectionnés et exige que `binding.guestMaterials['/outputs/libnano_core.a']` égale `publicResult.newCore`. La ligne GNU neuf champs est convertie exactement en Python9, sans flottant, puis `hold(CORE, originalRow)` compare FD, chemin, SHA et octets au fichier original. Le reçu public rapporte déjà l'égalité hôte/guest établie par le producteur RPC01 ; le contrôleur ne remplace pas cette ligne par une observation courante. Une copie Core neuve, fsync et mode0400, est ensuite admise prospectivement après trois observations identiques couvrant au moins deux secondes.

L'ancien Core1687, son binding pruning et son checkpoint hôte ne sont pas ouverts. Le reçu RPC conserve 143 membres et 142 membres inchangés lors de son remplacement de `rpc.cpp.o` ; aucune analyse d'archive, compilation Core ou suite RPC n'est rejouée ici.

## Objets, recettes et projection sélectionnée

Cinq rôles : Nano, bootstrapAudit, stoppedCapture, lifecycleTool et lifecycleSubmit. Chaque rôle exige son véritable objet et MD retenus. Nano rejoint le producteur Main38016 ; Audit/Capture/Submit rejoignent leurs lignes originales des quatre reçus fixes du module f28 ; Tool utilise exclusivement le producteur cfe9 actuel, dont le header668 a été compilé. Aucun ancien objet Tool lié au header079 n'est promu. Les objets/MD originaux restent inchangés ; aucun fallback CXX n'existe.

Les flags et tails restent ceux des recettes canoniques sélectionnées : cinq archives dont une unique archive Core et deux objets ASM. Les six inputs nonCore proviennent des lignes hôtes originales avec neuf champs et SHA, montés RO ; aucune ancienne table SDK entière ni `sourceAdmission` n'est lue. Le Core seul est substitué dans les tails. La macro Tool unique reste `-DNANO_TEST_SOURCE="/work"` ; les autres cibles n'en reçoivent pas et NANO_SYNC_FAULT reste interdit.

Le dépôt doit être propre, descendant de d26e4744416d033dad5a2519cc2ab8899b838faf. Les six arbres C++ sélectionnés reprennent leurs OID connus sauf `src=7c82858ec741c7076a4f89ca336dee574e11244d`. Le seul fichier multihost sélectionné est `tests/multihost/lifecycle_submit.cpp` : son blob Git courant et ses SHA/octets sont vérifiés, puis comparés aux pins projet du reçu original de l'objet Submit. Son M courant doit aussi rejoindre son MD retenu. Aucune égalité de l'arbre MJS multihost complet n'est exigée ou déclarée.

Le diff `src/include` depuis bd11 doit contenir exactement `src/consensus_capella_runtime.cpp` et `src/rpc.cpp`. CMake doit être 20b4b7c666777c389d401dd31b79755673a4aa4fac684800f6ee9a586b28f69f/176250 et chaque déclaration de cible/lien/macro sélectionnée est vérifiée. Le tar Git est extrait dans un namespace neuf avec bornes, refus des liens et contrôle de chaque blob. Les trois `.emptydir` vendor connues restent physiques mais exclues de la table de pins positifs ; toute dépendance M qui les atteindrait serait refusée.

## Résultat prévu et admissions nouvelles

Le lot prévu est exactement 5M indépendantes, 0CXX, 5liens, 5ldd et 6CLI (Nano help puis refus d'arité ; quatre helpers sans arguments). Chaque M normalisé doit égaler le MD physique normalisé/remappé ; l'égalité littérale et celle sans remappage restent des observations distinctes. Les seuls remappages sont ceux déjà permis par le worker. Les smokes restent avant DB/réseau/seed.

L'image AMD existante fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14 est inspectée, sans pull/build. Le conteneur neuf est readonly, UID1000, réseau none, cap-drop ALL, no-new-privileges, 2CPU/3GiB/128pids, tmpfs `/tmp`256MiB. Les sorties sont un nouveau bind privé UID1000, pas un volume vide susceptible de copy-up.

L'inventaire des sorties est conservé avant assertion du code worker. Pour les deux JSON nouveaux `outputs/result-public.json` et `outputs/software-admission.private.json`, les SHA/octets attendus viennent respectivement du stdout exact du worker et de son engagement du binding. Trois observations FD/path9+SHA identiques couvrant au moins deux secondes sont exigées avant leur premier `hold`, dans la fenêtre30s existante. Aucune ligne ancienne ni ligne déjà HELD n'est réadmise. Les cinq nouveaux exécutables sont aussi stabilisés, contrôlés mode0500 et liés aux pins du résultat. Le stdout, le résultat enregistré et le binding doivent se rejoindre.

Le délai global hôte historique reste1600s, la commande worker1250s et la propre limite worker900s ; cleanup séparé60s. Le superviseur distant Root doit couvrir l'ensemble, au-delà de1660s et du transport. Les ressources ne constituent pas une qualification de durée future. Cleanup ne retire que le conteneur exact ID/name/image/label ; les fichiers possédés sont retenus. Toute première erreur reste prioritaire même si l'écriture d'un diagnostic secondaire échoue.

## Limites

Proposition source seulement, qualification réelle Root en attente. P2P est entièrement hors worker : aucune M, reliaison, ldd ou exécution P2P ; Root doit conserver sa provenance distincte et ses anciennes neuf valeurs/SHA avec l'image et les arbres pertinents inchangés. Aucun membre Core, ancien test RPC/consensus, configure global, image, SDK global, symbole paresseux, domaine, clé, base historique, transaction, duty, reprise, finalité ou campagne n'est qualifié par cette proposition. Les trois admissions propriétaires et l'index24 restent à émettre séparément par Root après ses preuves réelles. Aucun choix de bridge ni claim de production.
