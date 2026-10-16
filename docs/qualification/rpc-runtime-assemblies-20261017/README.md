# Candidats runtime au Core RPC — 17 octobre 2026

**Les trois assemblages ciblés Native/ARM/AMD passent ; les trois P2P retenus sont rejoints séparément.** L’[index actuel](software-candidate-index.json) couvre 24 actifs : neuf Native, huit ARM et sept AMD, Core inclus. Il remplace la sélection de candidat, sans modifier l’ancien index ni ses preuves. Root a émis un nouveau sourceMF commun et [trois admissions de code](evidence/admissions/result-public.json), validées au schéma exact de 13 champs. Cela ne valide pas un spec complet de campagne et n’autorise aucune campagne 03.

## Résultats et identité du candidat

| Qualification réelle | M consommateurs | CXX dans l’épisode final | Liens runtime | ldd | CLI précoces |
|---|---:|---:|---:|---:|---:|
| [Native 03](evidence/native03/result-public.json) | 7 | 0 | 7 | — | 8 |
| [ARM](evidence/arm/result-public.json) | 6 | 0 | 6 | 6 | 7 |
| [AMD](evidence/amd/result-public.json) | 5 | 0 | 5 | 5 | 6 |

Native 03 passe en 94 247 ms et ajoute un lien du test de contexte et son exécution : 476 checks, 126 refus attendus, 258 cas ; les événements canoniques valent `96ca4c5586e5f53c3417fc5642c95f908036e5e179ad81b0865c423c9d4806cb`. Il réutilise les compilations Core et test réussies dans les épisodes précédents : deux TUs réussies au total, sans nouvelle compilation en 03. L’overlay Native du test ajoute uniquement `#include <unistd.h>` ; son pin `3aafab49…` est distinct de celui du TU Linux. Les copies objet/MD sont admises prospectivement, sans récupération de leurs anciennes identités à neuf champs.

Linux réemploie tous les objets/MD producteurs et relie seulement les consommateurs au Core RPC déjà qualifié. Le M actuel est égal au MD retenu après normalisation et remappage autorisé. Pour le Tool ARM, les égalités littérale et simplement normalisée restent **fausses** ; seule l’égalité remappée passe, avec les préfixes retenus `/overlay`→`/work` et `/outputs/third_party`→`/old/third_party`. Aucun ancien MD n’est réécrit. AMD passe aussi les égalités littérales pour ses cinq consommateurs.

Les [clôtures ARM](evidence/arm/selected-host-final-fence-public.json) et [AMD](evidence/amd/originals-final-fence-public.json) préservent respectivement 1 223 et 43 entrées hôte sélectionnées. Leurs conteneurs possédés sont absents ; aucun ancien volume ni image n’est supprimé. ARM réadmet prospectivement le SDK de l’image actuelle et la vue invitée du nouveau Core ; les huit anciennes lignes invitées de fichiers de volumes restent strictes. Cela ne récupère ni les anciennes identités invitées des montages Mac ni l’ancien SDK global.

| Core RPC retenu | SHA256 | Octets |
|---|---|---:|
| Native | `7052b226dbc2e8363b1db9eafe267ebf67a03926d0161493c3b2c7557bbad62c` | 29 247 704 |
| ARM | `43b6b8ee60f7b614117dc46c79f80eb3cdf930202d2d32c35bc24983aa8f49f5` | 41 040 028 |
| AMD | `2678b628528295dd7d729e8917572d3076a1068399e58b12f97fa3849bb497bd` | 40 805 844 |

Ces Core gardent 143 membres, dont 142 inchangés : seul `rpc.cpp.o` a été remplacé depuis le Core runtime précédent ; le membre runtime reste inchangé. Linux réutilise cette continuité producteur, sans nouvelle mise à jour d’archive. Trois Nano, les trois Core et l’Exporter Native changent de SHA par rapport à l’ancien index ; les quatorze helpers et les trois P2P conservent leurs SHA/octets. Le [joint source exact de Root](source-join.json) distingue le C++ de production effectif `2095012b…`, les assemblages qualifiés en `d26e4744…` et le commit de portabilité du test `7bd24dea…`, qui ajoute seulement cet include au test. Les arbres runtime et CMake restent identiques aux builds qualifiés ; le TU Linux conserve sa qualification initiale, sans prétendre qu’il a été recompilé avec cet include.

[Le réemploi P2P](evidence/p2p/result-public.json) rapproche les trois programmes de leurs neuf champs et SHA originaux, avec les arbres include/network/third_party inchangés depuis bd11. Aucun programme, M, lien, réseau ou nouvel inspect des images vivantes n’est exécuté par cet audit. P2P ne dépend pas du Core ; les frontières historiques de loader et SDK restent conservées.

## Historique et suites restantes

[Native 01](evidence/native01/failure-public.json) reste FAILED sur `unexpected exit: rpc-context-compile` ; Root identifie le joint mkdtemp du test. [Native 02](evidence/native02/failure-public.json) reste FAILED sur `new products not quiescent` : refus d’admission de métadonnées, sans causalité ctime précise établie. Leurs clôtures préservent les anciennes entrées. Le nouvel ARM PASS ne transforme pas l’ancien échec hôte ARM du lot runtime précédent en réussite, ni ne récupère ses anciennes identités à neuf champs.

Les sources exécutées et leurs manifestes d’auteur sont archivés sans retouche ; leurs statuts antérieurs restent distincts des résultats Root. Les trois patches conservent aussi leurs espaces de contexte exacts, avec trois exceptions Git limitées à ces chemins archivés. Aucun SDK, binaire, admission privée ou table brute n’est copié. Le [sourceMF](source-manifest.public.json) `376c0766…` précède les admissions et pinne exactement cet index et le joint source ; chacune des [trois admissions](admissions/mac-native.json) le référence, sans circularité. Les champs pending de l’index préparatoire restent byte-exacts : le reçu d’émission séparé clôt ces prérequis, sans retoucher le snapshot que pinne le sourceMF. Le validateur pur passe pour trois propriétaires et 24 actifs. Root préserve les neuf champs et SHA des 17 logiciels locaux et des sept distants ; le [contrôle AMD sélectionné](evidence/amd/selected-products-fence-public.json) ne lance aucun programme et ne lit aucun SDK. d30e et les admissions historiques restent intacts. La suite est la liaison d’un spec complet aux trois admissions de code émises, dans une éventuelle campagne dédiée encore non autorisée.

L’émetteur initial `c1bd247d…` est conservé comme source refusée en revue avant exécution : sa réadmission aurait recalculé les pins connus. Le correctif `daca9cae…` garde les pins attendus déjà produits ; seul ce dernier a été exécuté. Ce refus source n’est pas un échec de test ou une opération antérieure.

Ces contrôles ne qualifient ni loader récursif/lazy symbols, SDK global, configure CMake complet, ni runtime. Les seules DB du nouveau test sont synthétiques ; aucune base privée, nouvelle clé ou signature de transaction/consensus n’est impliquée. Les preuves EEST, Doppler, Geth et CI inchangées restent acquises dans leur périmètre ; elles ne sont ni annulées ni rejouées. La comparaison Geth reste à 50 écarts et Anvil à 116, sans parité globale. Campagne 02 FAILED, campagne 03 non autorisée ; goal actif. Reprise/duties/finalité, lancement/bridge, CI GitHub bloquée par facturation et production restent ouverts. Les admissions présentes sont des métadonnées de code : aucun spec complet validé, nouvelle clé, signature, base privée ouverte ou runtime démarré par leur émission. Ce paquet n’émet aucune admission de campagne.
