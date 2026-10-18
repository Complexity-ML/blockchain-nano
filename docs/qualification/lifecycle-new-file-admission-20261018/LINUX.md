# Complément Linux du lot lifecycle du 18 octobre

Après le push de `d75dd3d7e64ad662815cc65f64eb8912d001de37`, le VPS autorisé
effectue un `git pull --ff-only` depuis `8374fc62` ; main reste propre. Le même
code est projeté dans un dossier privé neuf, avec sources et logiciels tenus
sous leurs identités FD/path et SHA avant/après les tests. Aucun programme de
chaîne, ancien volume, journal privé ou clé de campagne n'est ouvert.

| Phase Node exécutée sur le VPS | PASS | FAIL | SKIP |
| --- | ---: | ---: | ---: |
| Admission NEW-file, dont 16 scénarios Python mockés | 8 | 0 | 0 |
| Driver guard `generation20261018` | 6 | 0 | 0 |
| Protocole runtime, avec les deux cas Linux | 18 | 0 | 0 |

Les trois commandes terminent à zéro, leurs groupes de processus sont fermés,
stderr vide. Les 55 refus attendus du protocole restent vérifiés. Les cas Linux
couvrent les copies synthétiques Perl et les champs statx/ABI, pas une
restauration de données de campagne. Les 106 entrées de sources, logiciels et
projection retenues restent inchangées. La projection contient 51 fichiers ;
aucune compilation C++, Core ou image, aucun redémarrage runtime.

L'environnement observé est Node18.19.1 et Python3.12.3 déjà présents ; aucun
runtime n'est installé ni recommandé pour la production par ce résultat. Les
empreintes sont dans [le reçu Linux](linux-result.json). Ce reçu est relu sur le
VPS et sa persistance vérifiée : 2 262 octets, SHA-256
`de8bd6f0db59ec2a3ab14026c2efb3b49066ccf83f418f6e742b905e8573c833`.

Ce lot ferme la qualification Linux **de ces helpers et scénarios**. Il complète
[le bilan Mac](README.md), sans effacer son test initial 7 PASS/1 FAIL, ses deux
préparations refusées ou l'agrégat Root FAILED corrigé par une lecture séparée,
sans rejeu. Le manifeste Mac et ses fichiers restent byte-exacts ; le nouveau
manifeste Linux est séparé. Le succès global de huit groupes est ici celui du
VPS, pas une nouvelle exécution globale sur Mac.

L'enregistrement CTest est source seulement : aucune configuration CMake
globale ni invocation via CTest. Fermeture complète des runtimes, reproductibilité,
tenue temporelle du vrai cycle et cohérence complète des logiciels opérateur
restent ouvertes. Aucun nouvel accord de campagne : 03 FAILED consommée,
aucune04 autorisée. Aucun owner entrypoint, exporteur, fonds, signature, réseau
de chaîne ou notification externe. La supervision Source03 non intégrée garde
son SIGABRT inexpliqué. La reprise des duties et la progression d'une finalité
commune ne sont pas démontrées.

Le C++ de production, Core C8, images et runtime réseau ne sont pas remplacés ;
Paris/Shanghai reste le profil activé, aucun EIP/fork promu. Les autres critères
intégraux de production, la CI/facturation et la revue indépendante restent
ouverts ; économie Base–Nano, bridge USDC, Doppler et T0 non qualifiés. Aucun
fonds, vente ou déploiement public. Objectif actif, production non qualifiée.
