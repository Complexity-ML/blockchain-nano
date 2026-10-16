# Copie entière lifecycle : backend statx Linux V2

Proposition SOURCE uniquement, hors R. Aucun Node, Perl, test, contrôleur,
compilateur, Docker, SSH, Git, SDK, fichier privé ou DB exécuté/lu par l’auteur.
Les anciens lots et refus restent conservés ; Root seul qualifie la proposition.

Le vrai essai VPS à 743 fichiers a refusé avant admission de la copie partielle.
Root a ensuite mesuré, en lecture seule, des sources/copies identiques neuf
champs+SHA : baseline 11.725s puis trois observations 12.052/12.958/13.159s,
soit 38.170s pour une fenêtre fixe de 30s. Les reçus publics désignés sont
lifecycle-owner-sampling-diagnostic-root-20261016/{stdout.public.json,
command-result.json} et lifecycle-whole-copy-real-amd-root-20261016/
first-refusal-projection-public.json. Cette mesure établit le coût des scans ;
elle n’admet pas la copie partielle et n’établit aucune restauration réussie.

## Delta borné

lifecycleWholeCopyProgram dérive toujours le même linuxRotationCopy public,
avec ses ancres comptées exactement une fois. Seuls son backend stat9 et son
adaptateur fdpath passent de GNU stat par fork à statx direct. Le template
rotation-owned-copy.mjs reste byte-exact. Les deux corrections V1 namespace
(concaténation `.` et fsync des parents namespace) restent intactes.

Les scans, accès FD et chemin avant/après lecture, namespace, SHA de tous les
octets, types/propriétaires/modes/nlink, inodes indépendants, sources inchangées,
publication de preuve et refus restent les mêmes. La fenêtre globale 175s et
la stabilité 30s avec >=3 observations identiques sur >=2s ne changent pas.
Aucun skip de fichier, cache de métadonnées, fallback GNU, rafraîchissement
historique ou extension de budget n’est ajouté. Les hash continuent à lire les
fichiers en entier. Le coût réel V2 reste à mesurer par Root.

## ABI explicite et neuf valeurs exactes

Le Perl appelant doit annoncer Config Linux, ivsize/ptrsize 8, byteorder
12345678 et archname x86_64-linux[-…] ou aarch64-linux[-…]. Numéros syscall
332 et 291 respectivement. Autre ISA/ABI, ENOSYS, EPERM, masque incomplet ou
nanosecondes hors [0,1e9) : refus, sans repli silencieux. Config décrit le
processus Perl, pas uname hôte. Seuls Config et Math::BigInt du Perl standard
sont ajoutés ; aucune installation ou image nouvelle n’est demandée.

Le buffer mutable fait 256 octets. Offsets Linux : mask0, nlink16, uid20,
gid24, mode28(u16), ino32/size40(u64), ctime96+104, mtime112+120 et dev_major136/
dev_minor140. Le masque retourné doit inclure 0x3df ; les bits supplémentaires
sont acceptés. Le chemin utilise AT_FDCWD et NOFOLLOW|NO_AUTOMOUNT (0x900), le
vrai FD utilise une chaîne vide mutable et AT_EMPTY_PATH (0x1000). La demande
reste synchronisée comme stat (aucun DONT_SYNC).

La row V2 est [dev,ino,mode,nlink,uid,gid,size,mtimeNs,ctimeNs] : neuf chaînes
décimales, mode décimal et timestamps entiers signés en ns. Math::BigInt fait
sec*1000000000+nsec et reconstruit dev_t depuis major/minor selon makedev
Linux64, sans passage flottant. Une nouvelle PV par champ évite les dualvars
JSON::PP après comparaisons numériques. Atime reste exclu.

Contrats primaires vérifiés par Root et relus par le pair ABI :
- https://man7.org/linux/man-pages/man2/statx.2.html
- https://github.com/torvalds/linux/blob/v6.8/include/uapi/linux/stat.h
- https://github.com/torvalds/linux/blob/v6.8/arch/x86/entry/syscalls/syscall_64.tbl
- https://github.com/torvalds/linux/blob/v6.8/include/uapi/asm-generic/unistd.h

La preuve PRIVÉE devient exclusivement
nano-fresh-lifecycle-owner-copy-private-proof-v2, avec
materialEncoding=linux-statx-nine-decimal-ns-v1. Verify exige les deux champs.
Une preuve V1 GNU (mode hex et dates texte) n’est ni lue sous cette nouvelle
sémantique ni renommée. Les anciennes preuves/admissions restent historiques.
Le résumé public conserve son format/allowlist actuel, car il ne publie aucune
row matérielle. Le driver guard ne change que le pin du module owner.

## Vérification ciblée proposée, non exécutée par l’auteur

Le fichier EXISTANT lifecycle-runtime.test.mjs, déjà enregistré CTest, conserve
tous ses anciens corps et le after global des 55 refus inchangés. Son test V1
réel create→verify→mutation-refus reste présent, avec les seuls imports/asserts
nécessaires au format V2. Il conserve les racines opaques entières, peer.key
PUBLIC factice, journaux imbriqués, fichiers/répertoires vides, neuf attributs
source et la trace qui appelle le vrai IO::Handle::sync. Une seule égalité de
sortie est relocalisée exactement vers son mkdtemp neuf, comme approuvé pour
V1 ; le produit conserve /rotation-audit. Aucun oracle C++/DB n’est simulé.

Un second groupe Linux exécute le fragment Perl statx réel sur de nouveaux
fichiers publics. Node lstat BigInt fournit la référence indépendante des neuf
valeurs chemin/FD ; GNU touch fixe mtime à .123456789 puis .123456790 pour
prouver 1ns exact. Un FD conservé lors d’un rename vise encore l’ancien inode,
contrairement au nouveau fichier au même chemin. Un symlink est observé sans
suivi et comparé à lstat. Un buffer ABI synthétique vérifie uint64 maximal,
dev_t maximal, seconde signée négative et conversion ns exacte. Douze refus
locaux couvrent taille, masque, deux ns invalides, ISA/OS/largeurs/endian,
chemin relatif, FD négatif et syscall sur chemin absent. Les neuf valeurs
JSON doivent toutes rester des chaînes. Aucun nouveau refus ne touche 55.

Les deux groupes dépendant de Linux sont skipped sur macOS ; Root doit les
exécuter sans skip en Linux ARM64 et AMD64. Le TIMEOUT CTest 30s est inchangé.
Le nouveau groupe est borné à 5s (touch 1s, Perl 3s) ; le groupe copie conserve
sa borne 24s et ses enfants 12/5/5s. Ces plafonds locaux ne garantissent pas le
cumul complet sous 30s : un dépassement est un FAIL à examiner, pas une hausse
implicite. Aucune assertion de vitesse nouvelle n’est codée dans le produit.

Root peut exécuter le fichier complet puis le driver guard et qualifier une
NOUVELLE vraie copie 743 fichiers, avec outils et sources déjà admis, sur les
deux ISA. Les scans doivent toujours respecter 30/175s. Succès synthétique ou
mesure de vitesse ne qualifient ni DB active, absence de writer, reprise de
duties, signatures, authentification witness consensus, finalité ou campagne
complète. Aucune nouvelle permission de campagne n’est créée par ce lot.
