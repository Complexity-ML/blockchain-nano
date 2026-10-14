# Diagnostic public : RO simultané à l’owner RocksDB RW

Constat statique précis, pas une preuve de corruption observée. CLI1b034/manifeste4611, harnessV3e5f664/manifeste dae95. SDK local public `/opt/homebrew/include/rocksdb/db.h`, SHA d31da9d3b4db690e43842f78f6a2d2cc4a560cf5924b1d907155106fa56b4702, lignes195–199 : le reader OpenForReadOnly simultané à un owner Open a un comportement undefined. Le writer est documenté comme unaffected. Une pauseSIGSTOP ne ferme pas cet owner. Ce contrat local n’est pas une qualification de SDK Docker ; ROOT doit aussi épingler le header réellement utilisé dans ses images, sans supposer un numéro de version.

## Chemin concret concerné

CLI `invariant()`125–127 ouvre RO sur le chemin des copies. Premier appel154 précède PairRW161. Le test `locked_and_stopped`113–116 possède un autre child ownerRW, prouve sa vraie acquisition de LOCK, puis appelle `recover`. Le CLI exécute donc son invariantRO contre une DB déjà détenue RW, avant que son futur Pair tente/refuse le lock. Ce cas ne démontre pas une lecture sûre sous contention, même si le refus final est celui attendu. Aucune erreur logique/donnée modifiée n’est attribuée à ce point sans mesure.

Les autres ROlive du CLI après Pairclosed (`invariant`166/175 et ChainStoreRO167) sont sûrs seulement sous la quiescence effectivement maintenue ; un autre owner peut ouvrir entre ces phases. Une validation robuste ne doit pas dépendre de cette fenêtre non verrouillée. Les scans physiques des originaux ne constituent pas des OpenForReadOnly RocksDB ; ils ne sont pas le problème SDK, mais demandent toujours des bindings/stabilité/provenance réels.

## Proposition CLI V3 à relire avant implémentation

Remplacer les lecteurs RocksDBRO des userpaths par des lecteurs sur **replicas privées physiques indépendantes** sous l’output owned0700. Aucun hardlink/symlink ni copie paresseuse partageant un inode. Outputexclusif/new, chemins hors des quatre arbres sources/copies, leafmode/owner conservés sous protection700. Copies bornées par budgets fichiers/octets/durée existants et buffers≤64KiB. Les noms viennent de l’inventaire réel ; pas de guessWALfilename ni offsetsrecords.

Ouvrir les arbres d’entrée avec DirectoryFDs tenus et bindings stricts ; pour chaque leaf, lstat/fstat/O_NOFOLLOW, owner/type/nlink1, taille et SHA, recheck inode/path/mode/mtime/ctime après lecture. Capturer inventaires complets avant/après copie et exiger égalité ; l’ensemble replica doit avoir les mêmes bytes/modes/relativepaths et des inodes distincts. Une instabilité, remplacement, WALrotation/append concurrent non lié, budget ou replica incohérente refuse avant protocole ; aucune « réparation »/baisse de garde. Ne pas prétendre qu’une observation stable prouve seule l’absence d’un owner : stopreceipt/provenance externes et l’exclusivePair restent requis.

Les replicas sont de nouveaux arbres privés non ouverts RW ; `OpenForReadOnly`/ChainStoreRO ne les ouvre qu’une fois scellés et sans ownerRW. Elles servent **uniquement au hash logique/audit/closed-readback**, jamais de source de restauration à copier en retour et jamais de nouvelle autorité. Garder exactement le request, public signedproof et deux originalNEXT. Rechecks des vraies copies/inputs autour de la reconstruction et la CAS restent nécessaires ; une invariant de replica ne remplace aucun pin live du Pair.

La paire opérationnelle continue d’ouvrir **les deux seules copies** en ReadWriteExisting, après la rétention durable des deuxNEXT, pour apply/recover. Les originaux et les replicas d’audit ne sont jamais ouverts DBRW. Sur contention, le Pair refuse le lock ; l’audit préalable n’aura lu que sa replica, sans prétendre à une opération sous owner externe. Après Pairfermé, reconstruire une replica fraîche pour tous ROreadbacks et comparer pins/rawsnapshot/closure/blobs et invariants complets ; l’éventuel typedreopenRW existant ne devient pas un nouvel import/repair, et doit conserver ses pins/CAS actuels.

Privé operatorhost uniquement : ces nouvelles copies DB et leurs inventaires ne vont jamais en stdout/Git/transfert. Écrire les preuves internes600 dans output700, retour CLIstatus-only existant. Aucun backup whole-node ni journals/floors privés qualifiés ; aucun reset. Les handles/scratch sont fermés explicitement ; ne jamais nettoyer des données préexistantes ou une branche de reprise encore requise.

## Conséquences de qualification

Le correctif serait un vrai changement du CLI production, pas un simple shim : nouvelle proposition immutableCLI V3 + revue, nouvelle compilation consumer/link avec closureSDK/source liée, puis **treize cas wrapper** à rejouer sur ce CLI (plus les nouveaux crashs lorsqu’ils existeront). Les trois retentions seuls suffisent uniquement pour une modification de shim commune ; ils ne suffisent pas pour ce changement de lecteurCLI. Conserver tous anciens résultats/sourceV2 et préciser le scope de contention non qualifié par eux. Ne pas supprimer le test LOCK ni desserrer OpenReadOnly/expectedtokens.

TESTdeadline est corrigé séparément en V2 (9b5b…), avec ROrows avant openRW/après destruction dans trois scopes ; prod8e/helpera585/CM011a inchangés, aucune compilation/exécution. Le designWAL V2a55c utilise aussi des replicas scellées pendant SIGSTOP, pas des ROlive.

Aucun code CLI/WAL implémenté, aucun DB/privé lu, aucun compiler/test/Docker/SSH, aucun changement du dépôt ou grant. Causalité performance, repriseUTC/duties/finalité et production restent ouvertes.
