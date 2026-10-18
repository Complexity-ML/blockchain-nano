# Hôte portable du nouveau test Capella sync adverse

Proposition source seulement. Root seul admet le contrôle, les entrées et exécute
le contrôleur. L'auteur n'a exécuté/importé ni ce host, ni le worker, ni un test,
et n'a lu aucune autorité privée, bibliothèque SDK ou donnée de campagne.
Cette proposition ne remplace aucun fichier/contrôleur antérieur et n'écrit pas R.

## Contrat

`host.py CONTROL_JSON CONTROL_SHA256 CONTROL_BYTES`. Format neuf
`nano-capella-sync-adverse-retained-control-v1`, exactement15 champs dans le
template. Le template `ready:false` n'est pas exécutable : Root crée un nouveau
contrôle complet, avec chemins, véritables anciennes rows et pins fournis par
ses audits, puis fournit son SHA/octets séparément. Aucun ancien champ n'est
actualisé à partir d'un stat courant.

Le contrôle conserve989 pins Core `src/include/third_party` inchangés et leur
jointure au snapshot public complet. Tous les fichiers de ce snapshot figurent
dans `projectPins` et dans les rows Root ; seuls CMake, le nouveau TU et les
éventuels inputs runtime réellement lus doivent figurer dans `sourcePins`.
L'inventaire refuse tout autre fichier ou alias. Les markers vides connus sont
limités aux `.emptydir` sous third_party ; les fichiers du snapshot sont nlink1.
Pour les anciens matériaux, le nombre de liens reste celui de la row Root
authentifiée, comme tous ses autres champs ; il n'est ni deviné ni rafraîchi.

Le Core ARM fourni par Root est `11c1494f…/41225178` et doit être exact ; AMD
reçoit son propre pin Root dans un nouveau contrôle. Le lecteur d'archive vérifie
143 membres objets uniques, sans `ar`, mise à jour ou compilation du Core.
Six bibliothèques et les headers générés explicitement sélectionnés restent
montés RO. Ce contrôle de contenu n'est pas une nouvelle qualification du SDK,
du DAG ou de tous les producteurs historiques.

## Un seul nouveau TU, aucune ancienne suite

Le worker public immuable e9bfc40c…/37768 s'exécute **dans Python sur l'hôte**,
via `run_qualification` inchangé. Aucun Python n'est requis dans l'image. Son
contrôle effectif a les20 champs exacts du mode retained : huit champs de
`workerConfig` plus les12 paramètres hôte/montages/identités ajoutés par le host.

Une seule unité candidate : `/work/tests/test_capella_sync_adverse.cpp`, binaire
`nano-capella-sync-adverse-tests`, objet compilé neuf. Un seul cas `--self-test`
et trois records JSON typés exacts : deux groupes puis summary. Root doit
compléter toutes les clés de ces oracles depuis le contrat source revu, sans
prendre le résultat candidat comme oracle. Les groupes diagnostiques ne sont
pas exécutés en plus. La summary doit conserver2 groupes,3 blocs signés et8
signatures de fixtures de test seulement, sans entrées privées/duties/réseau.

Attendus : 1 M indépendant,1 CXX,1 lien,1 ldd,1 invocation test, M==MD ; zéro
scan/CXX/ar Core, reuse d'objet, ancien test, CLI runtime ou configure CMake.
Les quatre nouveaux produits M/objet/MD/exe sont tenus sur les rows originelles
du worker jusqu'après cleanup. Les reçus worker/autorité ne deviennent pas des
autorités historiques inventées : nouvelles admissions explicites aux octets
connus. Si le worker échoue avant retour, les logs/résultat partiel restent sur
le disque et `newCppCompilations` reste null, jamais0 inventé.

## Isolation et clôture

Les namespaces Root neufs s'appellent `capella-sync-adverse-arm-root-20261018-NN`
ou `capella-sync-adverse-amd-root-20261018-NN`, en dehors de R et du snapshot.
Docker local, image épinglée sans pull/build, networknone, aucun port, rootfs RO,
capdropALL/no-new-privileges,2CPU/3G/128pids, tmpfs128–1024MiB. Seul le nouveau
répertoire outputs est RW. Core/providers/source sont RO. Le contrôle accepte le
préfixe Docker direct Mac ou sudo-n Linux, outils présents dans les rows Root.
Le temps worker est borné60–1500s, setup180s et cleanup60s distinct.

CID/name/image/label sont contrôlés avant arrêt/suppression. Un cidfile vide ou
partiel ne bloque pas le fallback possédé par nom. Le cleanup, l'inspection finale
de l'image et toutes les fences full9/SHA/inventaire sont tentés séparément,
première erreur conservée. Les processus CLI sont joints et leurs terminaux
publiés avant interprétation. Fence et résultat ont des publications indépendantes.
SIGINT, SIGTERM et SIGHUP entrent dans cette même clôture ; leurs handlers sont
restaurés avant de construire et publier le résultat final, de sorte qu'un refus
de restauration ne puisse laisser un ancien JSON PASS.

Les observations GNU/stat/SHA du worker ne revendiquent pas l'identité FD de la
VM. L'identité FD9 est réellement tenue côté hôte, selon les rows Root. Un résultat
vert porte seulement sur ce nouveau test et cette enveloppe : pas de campagne04,
restauration de duties, finalité WAN/multiowner, cycle complet, production, fonds
ou ouverture de vente. Les anciens FAILED restent intacts.
