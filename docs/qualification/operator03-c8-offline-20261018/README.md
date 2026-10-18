# C8 : audit hors ligne des données Native de la campagne 03

Le 18 octobre 2026, sur les sources main `1533c979923b034f2be71973cccdc47d3de3db9d`,
le `nano` C8 installé audite positivement une copie neuve des données conservées de
la campagne privée 03. La base du C++ de production reste
`a8eee9d6d3a35b8e14e8a7f7a6cd1b9e5ee2acd7`. Aucun code de production, Core,
image ou runtime réseau n'est remplacé par cette publication.

| Vérification | Résultat acquis | Limite |
| --- | --- | --- |
| Inventaires historiques | 34 rôles DB concordent ; 372 fichiers, 156 490 800 octets copiés | `fileproviderd` garde 34 `LOCK` ouverts en écriture sur les sources ; quiescence source non qualifiée |
| `nano audit-capella` installé | Sortie 0, groupe fermé, 694 blocs avec ancre, 2 transactions, historique canonique et ascendance finalisée externe vérifiés | Ni choix de chaîne historique ni rejeu des votes autonomes |
| Six consommateurs Native | Compilation/lien réussis, 6 TU, Core conservé, 9 713 entrées antérieures inchangées | Ni six suites runtime réussies ni build reproductible |
| Capture C8 de la copie | 34 DB RO, 16 paires primaire/témoin, tokens extérieurs et contenus conservés concordants | Pas de reprise de duties ni de restauration entière de l'opérateur |
| Commandes passives | Deux self-tests et trois refus d'arguments attendus ; sorties et fermetures exactes | Exporteur compilé seulement ; aucune invocation signante |

## Engagements indépendants et portée de la copie

L'arrêt original est épinglé par SHA-256
`b38dddb3fad083958399af3c3b5b95724a2f78c7c5476ee5427b26a400ac403f` ;
la capture historique par
`a1102df2b0f74289653c2152a516e1e4b8a9b16fa44f5ab38c61fca9c551dbe0`.
Le checkpoint attendu est conservé séparément : séquence 7 858, digest
`0xad745448f4d102cce2a738ec84687c8ea54ae79d7be9cf1bc8ca32e530c8f576`.
La finalité requise est l'époque 87, racine
`0x47a52f089fc2d0516a54810b2b05311fffa6bffc847df0efcb9058a08de16986`.
Ces attentes proviennent des reçus d'arrêt, pas du résultat à valider.

L'audit installé vérifie ce checkpoint, son ascendance finalisée et 2 033 766
octets d'archive. Les états historiques sont matérialisés ; aucun état élagué
n'est rejoué. La finalité antérieure de l'époque 29 n'est pas qualifiée
séparément. Les deux transactions EVM ne sont pas présentées comme une preuve
d'inclusion de dépôts, de sortie volontaire SSZ ou du cycle des validateurs.

La copie est admise avec de nouveaux inodes et des observations FD/SHA stables ;
les autorités des sources ne sont pas réinitialisées. Les contenus, métadonnées
observées et octets des originaux/copies restent inchangés après les commandes.
La clé privée `peer.key`, extérieure aux 34 rôles capturés, est exclue. Aucune clé
privée n'est lue. La copie n'est donc ni un snapshot atomique des sources ni une
restauration de l'identité entière du nœud. Les descripteurs système observés ne
prouvent pas la cause des anciens échecs de métadonnées.

Le captureur relit les checkpoints, snapshots, intentions et floors en RO et
compare les tokens externes primaire/témoin. Son résultat ne vérifie pas les
signatures des archives witness, l'ascendance historique des digests, ni le
rejeu canonique : ce dernier est le résultat distinct du `nano` installé. Il ne
constitue pas une invocation de l'API Core d'authentification de journaux qui
autoriserait une reprise signante.

## Logiciels et provenance

Le binaire installé `nano` conserve SHA-256
`1b4d502c7a2401aa36f56496461a7e3b7c1c4bd99eef7a7d5fe4bf90b68415bd` ;
le Core conserve
`ec2d12064b6f8606823b7d6ac53857dbd9953cca9046b800853c335a46acfc2a`.
Les six nouveaux programmes sont détaillés dans
[le reçu de compilation](consumer-build-result.json). Leurs sources projet
consommées concordent avec main 1533 ; leurs M/MD et recettes de lien sont
engagés dans [la fermeture observée](consumer-closure-summary.json).

Cette fermeture comporte 2 556 dépendances distinctes. Parmi elles, 2 391 headers
externes ont seulement une observation après compilation : leur identité au
moment de compiler n'est pas prouvée. La fermeture SDK/dyld complète, la
reproductibilité et la cohérence de tout le lot opérateur, notamment le P2P
Native, restent ouvertes. Ces six programmes ne sont pas une nouvelle image
Docker. La compilation d'un shim de test macOS puis de six consommateurs ne
recompile aucun objet Core.

Les deux self-tests concernent le captureur et le soumetteur synthétique ; ce
dernier utilise des callbacks/fichiers locaux, sans clé, réseau ou exécution de
commande externe. Les trois refus attendus concernent import, audit bootstrap
et outil lifecycle sans arguments. L'exporteur n'est jamais exécuté. Les stderr
non vides ne sont pas masqués : 777 octets pour l'audit installé, 6 165 pour la
compilation et 96 pour un refus d'arguments.

## Échecs et exigences ouvertes

[Les refus conservés](historical-refusals.json) comprennent la première copie
refusée avant toute lecture DB à cause du fichier privé hors périmètre et le
dry-run récursif `make -n` sans shim matérialisé. La construction réelle du shim
résout ce dernier cas sans changement CMake. Trois refus de projection publique
des reçus sont conservés ; aucun test de qualification n'est rejoué pour eux.

La supervision proposée reste **non intégrée** : 16 cas ciblés passent, mais le
rejeu parallèle Mac Source03 termine en échec avec trois processus Node en
SIGABRT. Sa cause et les refus Mac01/02 restent inexpliqués. Voir
[le résumé d'échec ouvert](monitoring-open-failure.json).

Le défaut de Genesis de l'ancienne fixture de portabilité C8 reste conservé ;
ce succès CLI porte sur le vrai Genesis de la campagne 03, sans modifier le
codec. Les journaux Mac du 7 octobre perdus/NAS refusé constituent un domaine
distinct des données du 17 octobre relues ici.

La campagne 03 reste FAILED, son autorisation consommée ; aucune 04 n'est
autorisée. Aucun processus de nœud, signature, nouvelle identité, transfert de
capsule, fonds réel, vente ou déploiement public. Le runtime réseau 1925 est
inchangé. Paris/Shanghai reste le profil activé, sans promotion d'EIP/fork.
Restauration complète, reprise effective, progression d'une finalité commune,
cycle des validateurs, autres écarts consensus/WAN/charge, pruning prolongé,
exploitation, CI/facturation, builds reproductibles et revue indépendante restent
ouverts dans leur périmètre. Production **non qualifiée**, objectif actif.

La proposition de token Nanobees sur Base reste non adoptée ; offre, ratio,
réserve, conversion, bridge USDC, termes Doppler et visibilité T0 ne sont pas
qualifiés. Aucun lancement économique n'est déduit de ces résultats de stockage.

## Fichiers conservés

[Le manifeste](manifest.json) engage les reçus publics et les hashes d'entrées
privées. [L'audit](installed-audit-result.json), [sa sortie](installed-audit-stdout.json),
[la capture résumée](stopped-capture-result.json) et
[le préflight](storage-preflight.json) sont publiés sans chemins locaux, clés,
configuration privée, DB ou capsule complète. Les reçus détaillés et la nouvelle
capsule restent conservés localement hors Git ; seul son hash est publié.
