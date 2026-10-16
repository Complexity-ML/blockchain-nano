# Correctif du chemin de copie entière lifecycle — avis et proposition source

Base annoncée par Root : main c1b66950. Lecture publique uniquement ; aucun Git,
Node, Perl, test, Docker, SDK, capsule, données privées ou programme projet
exécuté/lu par l’auteur. Aucun Rwrite. Les pins baselines ci-dessous lient les
fichiers lus ; ils ne prétendent pas vérifier le HEAD Git par une exécution.

## Défaut réellement atteint, distinct du reçu terminal corrigé

- tests/multihost/lifecycle-owner.mjs:11 injecte `+` entre deux chemins Perl dans
  mkdirprivate. Perl emploie `+` pour l’addition numérique, `.` pour la
  concaténation. Le répertoire namespace attendu n’est donc pas construit.
- run.mjs:1063 dérive ce programme avec le namespace/owner du plan ; :1065
  l’exécute en mode create au premier lifecycleRestore, avant la capture des
  copies (:1067) et la reprise du runtime (:1072).
- Ce défaut peut arrêter la première restauration avant les duties repris. La
  lecture seule ne prouve aucun échec opérationnel nouveau ni une mutation des
  sources. Les résultats et domaines historiques restent inchangés.
- Le pin du générateur dans lifecycle-driver-guard.test.mjs est actualisé ;
  aucune règle de garde ou autorisation n’est abaissée.

Le produit corrige ce `+` en `.` et ajoute une troisième ancre exacte : la liste
finale fsync reçoit /data/<namespace> et /witness/<namespace> AVANT data/witness/out.
Ces parents contiennent les entrées owner et doivent aussi être synchronisés.
Le template rotation original reste inchangé. C++/protocole/CMake/run.mjs restent
inchangés. Les mises à jour de configuration de reprise existent déjà dans
run.mjs:970–981 (pinnedRestoreConfiguration puis spawn) : l’avis ne crée pas une
nouvelle procédure de récupération ou de backup. Le CLI stopped-backup générique
n’est pas le chemin employé par lifecycleRestore ; ses copies ne suffisent pas à
prouver les deux restaurations de campagne.

## Autres ancres et bornes relues

La liste des deux cibles inclut exactement /data/<namespace>/<owner> et
/witness/<namespace>/<owner>. Le mkdir ajouté est leur parent namespace. Les trois
ancres remplacées une fois sont présentes une seule fois dans linuxRotationCopy.
Les substitutions de limites touchent les compteurs fichiers/répertoires et le
budget d’octets PAR racine ; le wrapper owner-copy borne les totaux à deux fois
les ressources. Le plafond historique de lecture individuelle et de preuve,
les délais, la stabilité >=3 observations/>=2s, les neuf attributs (atime exclu),
SHA, sources inchangées, mode privé/inodes indépendants et la preuve externe
restent intacts. Aucun vieux matériel n’est rafraîchi. La dérivation Perl n’est
ni remplacée par le copieur JS ni simulée par un stub.

## Régression réelle, enregistrée par le CTest existant

Un groupe Linux est ajouté à lifecycle-runtime.test.mjs, déjà enregistré par
consensus_multihost_fresh_lifecycle_runtime_protocol. Les 55 refus historiques
et leur after global restent BYTE-EXACT ; le nouveau cas emploie uniquement ses
assertions locales. Aucun fichier test ni entrée CTest nouvelle n’est créé.

Le test prépare DEUX racines opaques publiques entières : chain/chain-witness,
journaux 32/64 et leurs témoins, fichiers imbriqués, fichier vide, répertoires
vides et peer.key factice. Il appelle réellement /usr/bin/perl sur le programme
généré pour create puis verify, contrôle preuve SHA, arbres complets, modes,
propriétaire, inodes indépendants et neuf attributs/SHA des sources inchangés.
Un préambule de test enveloppe IO::Handle::sync : il relève la cible FD dans
/proc puis appelle le VRAI sync et renvoie son résultat inchangé. Sa trace démontre
que les deux parents namespace sont synchronisés, puis data/witness/output, dans
cet ordre, et que les owner roots sont aussi synchronisées. Aucun faux fsync
réussi n’est injecté ; ce n’est pas une preuve de coupure électrique.
Il altère ensuite la NOUVELLE copie peer.key : verify doit refuser sans mutation
des sources ni réécriture de la preuve. Ce sont des octets opaques, pas des DB,
clés/signatures, authentification de signataires ou reprise de duties.

Pour être exécutable dans CTest Linux sans montage/privilège global, le test
relocalise une seule égalité littérale `$out eq '/rotation-audit'` vers son propre
mkdtemp/output neuf. La garde d’égalité reste stricte, la substitution est exigée
exactement une fois ; tout le code filesystem/FD/copie reste inchangé. Le produit
conserve /rotation-audit. Sous macOS seul ce groupe Linux est skipped (/proc et
GNU stat requis). Root peut qualifier séparément le vrai mount /rotation-audit
neuf sans relocalisation ; cette intégration n’est pas revendiquée par le test.

## Plan causal Root — aucune exécution auteur

1. Dans une projection NEUVE du parent, ajouter le seul test proposé ; conserver
   le générateur parent. Exécuter le fichier complet en Linux ARM avec Node/Perl/
   GNU stat existants et TMPDIR Linux neuf : le nouveau groupe doit FAIL pendant
   create (avant verify), les anciens refus restant à 55.
2. Même nouvelle fixture, source candidate épinglée : fichier complet PASS, groupe
   réel non skipped ; create/verify réussis et mutation refusée. Exécuter ensuite
   le driver guard sur sa closure complète. Aucun replay C++/Core n’est requis.
3. Un succès de ces tests ne qualifie ni copie d’une DB active, ni witness consensus,
   restauration opérationnelle, progression finalisée, campagne complète ou
   nouvelle permission. Root seul choisit/exécute les opérations autorisées.

Le TIMEOUT CTest existant de 30s n’est pas changé. Le nouveau groupe a une borne
24s ; ses trois sous-processus sont bornés à 12s/5s/5s (22s cumulés au pire),
avec la stabilité réelle >=2s dans create. Le fichier conserve aussi son ancien
groupe FS (~2s hors surcoûts) ; un dépassement reste un FAIL, pas une extension
implicite du délai. Les durées réelles restent à mesurer par Root.

Commandes proposées à Root :

```
node --test tests/multihost/lifecycle-runtime.test.mjs
node --test tests/multihost/lifecycle-driver-guard.test.mjs
```
