# Futur lot ARM — préparation explicite des owners avant première admission

**SOURCE ONLY / DESIGN-PATCH, non exécuté.** Ce dossier est neuf. Les paquets gelés ARM01/02 restent intacts. Il ne fournit ni configuration effective ni préparateur à HEAD supposé. Root résoudra le prochain HEAD explicite et les nouvelles namespaces après publication ; modèle13champs `ready:false`, `actualHead:null`. Aucune invocation03 n’est proposée dans le lot courant.

## Motif communiqué par Root, sans lecture d’actuals

ARM01 `71557/1` puis ARM02 `76604/1` ont refusé le préflight, **0 M /0 CXX /0 RUN**. Dans ARM02, l’observation de priming non admise voit les sept providers RO (Core et six bibliothèques) UID/GID0:0 ; le premier échantillon d’admission voit encore0:0, le suivant environ une seconde plus tard501:20. Les autres sept champs FD9 et les octets/alias étaient identiques selon le diagnostic Root. Les 17 entrées hôte tenues, les neuf logiciels originaux, cleanup/image/volume sont restés valides. La cause et l’acteur de cette présentation changeante ne sont pas prouvés. Une observation unique de priming ne ferme pas ce prérequis.

## Contrat futur et delta

L’interface externe hôte reste13champs. L’interface fermée hôte→worker ajoute **`typedOwners`**, map exacte de chaque clé `projectPins|knownGuestPins` vers `{uid:int,gid:int}` (entiersnonbool0..65535). L’hôte dérive ces valeurs des **anciennes lignes FD9 originales** des1047 fichiers `/work`, des sept Core/bibliothèques montés RO, et des lignes des deux nouvelles copies `/case` admises par Root. Soit1056 valeurs, sans inventer501:20 comme propriété des fichiers. Les neuf logiciels hôte sont un ensemble différent : Core/sixlibs+Git/Docker ; Git/Docker ne sont pas des providers invités montés et gardent leur FD9 d’origine strict.

Avant `admit_inputs`, seulement s’il existe des chemins connus encore non tenus, `prepare_known_owners` observe explicitement sans créer d’autorité. Chaque observation complète et ses vérifications sont publiées avant les décisions et le délai. Les octets/alias/dev/ino/mode/nlink/taille/mtime/**ctime** sont fixes pendant cette préparation ; seule la paire UID/GID peut encore changer. Tous les octets connus et les chemins canoniques restent exigés à chaque lecture. Les observations qui traversent un changement intra-stat/hash/stat restent refusées : aucun retry d’observation incohérente.

La préparation est bornée à30secondes/30observations, inclusions des fsync des observations. Elle attend les valeurs UID/GID dérivées des vraies lignes hôte ; elle n’altère jamais les valeurs observées. À la première observation conforme, elle quitte cette phase **sans tenir ses lignes**. L’admission existante commence ensuite normalement : huit champs/alias/octets fixes et trois échantillons FD9 identiques pendant au moins deux secondes, sous trente secondes. Un contrôle supplémentaire exige encore l’owner attendu à chaque échantillon, avant le premier ancrage8. La tolérance ctime prospective de cette admission antérieure reste inchangée et bornée ; après tenue, FD9+SHA complets stricts.

Les entrées ou alias déjà tenus sont comparés immédiatement aux anciennes valeurs à chaque observation. Tout écart refuse ; aucune attente de reconvergence, aucun reset, aucune nouvelle première tenue ou remplacement d’autorité. Un refus après tenue reste un refus. Les résumés de préparation sont seulement observationnels et ne revendiquent pas l’identité historique d’un inode invité.

Le patch remplace le priming unique ARM02, ajoute l’owner attendu au même `admit_inputs`, et fait dériver/passer le nouveau champ fermé par l’hôte. Le worker conserve tous ses M/MD, produits, linkPrefix/linkTail/ldd, C++42champs/productionAdmissions/sommes, limites et fences. Aucun autre oracle n’est assoupli.

## Portée conservée et gates

Même CppV3 `2e5ef451… /28801`, helper `d3e2ddd7… /20319`, Linux Core65/143membres et imageARM existante. Futur objectif borné :1TU /1RUN, zéroCore/imagebuild. L’inventaire1047 demeure noms/taille/SHA strict, et chaque dépendance M/MD consommée doit mapper une ancienne ligne hôte exacte puis rester inchangée. `historicalAllSourceFD9Qualified:false` ; aucune adoption des dérives descriptives horsclosure.

Même volume Linux persistant, sibling neuf, anciens69/portable8 passivement vérifiés, fences après nettoyage ; aucun ancien test DB rejoué. Le véritable exit C++ via Dockerexec reste distinct de celui du keepalive. Aucun owner virtuel ne remplace une ancienne row hôte.

Le lot courant reste AMD PASS / NativeCLI local selon preuves Root, ARM01/02 FAILED ; aucune de ces qualifications ne se transfère à ce patch futur. Aucune production, duty, finalité, restauration opérateur, protection indépendante contre rollback simultané des engagements ou provenance globale SDK n’est revendiquée. Root décidera d’une éventuelle qualification séparée après revue et autorités explicites.
