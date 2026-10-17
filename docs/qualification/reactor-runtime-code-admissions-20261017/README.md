# Admissions de code courant : trois propriétaires, 24 artefacts

Root a terminé l’audit des sept fichiers logiciels AMD, puis l’émission Mac des trois admissions de code et de l’index couvrant 24 artefacts : 9 Native, 8 Linux ARM et 7 Linux AMD. Les deux clôtures finales sont PASS. Le validateur pur existant accepte les trois admissions de 13 champs et les 24 locateurs. Le présent rapport assemble uniquement les reçus publics sélectionnés ; son auteur n’a exécuté aucun émetteur, programme applicatif, test, commande distante ou accès privé.

Cette émission de métadonnées n’est pas une campagne ni une reprise des duties. Le reçu conserve `fullCampaignSpecValidated:false`, `newCampaignAuthorized:false`, `runtimeStarted:false`, `dutiesResumed:false` et `productionQualified:false`. Aucun programme applicatif qualifié n’a été exécuté ou copié par l’émetteur ; il n’a compilé, ouvert de DB, créé de clé de consensus ni demandé de signature de consensus. Python, Git et une invocation Node des API pures ont été utilisés par Root pour l’émission. La qualification des images et les futures opérations restent séparées.

## Résultats conservés

| Étape Root | Résultat public | Portée |
| --- | --- | --- |
| Audit sélectionné AMD | [résultat](amd-selected-audit/result-public.json), [clôture](amd-selected-audit/final-fence-public.json) | Sept fichiers, neuf champs originaux et SHA exacts sur le VPS ; deux tables de producteurs sélectionnées ; aucun SDK lu ni logiciel exécuté. |
| Émission Mac V2 | [résultat](code-admissions/result-public.json), [clôture](code-admissions/final-fence-public.json) | 17 fichiers locaux joints au résumé public AMD des sept fichiers distants ; anciennes identités non rafraîchies. |
| API pures existantes | [validation](code-admissions/pure-validation-public.json) | Trois admissions et 24 locateurs ; pas de validation du spec complet de campagne. |

Le résultat AMD est `7934f567a4bc03bab883cddd0672d97ce0e40fb3a437f7f32987a8c6bf614bb3` /3038 B. Le résultat Mac est `ab306e0e0534794e4f3491864d301e5ebaf8e383aacc6ba1fd787105968172a5` /2981 B. Les deux petits reçus de clôture sont byte-identiques (`a0887f33…` /91 B), avec `passed:true`, `firstFailurePreserved:false` et `oldAuthoritiesRefreshed:false`.

Le résumé distant est une observation Root réalisée sur le VPS, pas une observation distante répétée par l’émission Mac ou par ce rapport. Les tables privées, lignes d’identité détaillées, locateurs privés de lancement et contrôles opérateur réels ne sont pas publiés. Les chemins des logiciels présents dans les admissions et le résumé AMD sont des métadonnées logiciel explicitement sélectionnées.

## Lignée des sources et ordre sans circularité

Le nouveau paquet porte `sourceBaseRevision=85f6922fb5dd2516905f080230ba518d2d39f9d8`, descendant documentaire du code qualifié. Les producteurs de reliage restent au commit `1925eeeabc9f7d0aef41f2c3fe43c11c31f1f352`. Les exécutables P2P sont joints séparément au commit `00154f57d4b6e1515d60edeae4e0ab38c42da28a`. Le [joint de sources](code-admissions/source-join.json) enregistre l’égalité explicite des arbres sélectionnés/CMake au qualifié 1925 et du réseau/JSON au P2P qualifié. Il ne prétend pas que les six arbres ou le CMake courant égalent encore 68d, ni que le SDK ou l’arbre multihost entier est qualifié.

L’ordre produit est [index](code-admissions/software-candidate-index.json) et joint de sources, puis [manifest source](code-admissions/source-manifest.public.json), puis les trois admissions, puis le reçu d’émission séparé. Le manifest n’inclut pas les admissions qui le nomment. Les pins structurants sont :

| Artefact | SHA-256 | Octets |
| --- | --- | ---: |
| Index 24 | `d91eef9c040ac604878e359db542081971844b85a9dc1da1a7a24b2960d75627` | 7750 |
| Joint de sources | `d74338348273d5e53afa914c64f79ee6d120c9bcbe69cbfd514d04d1b65a0959` | 1187 |
| Manifest source | `72538c5db0ecacafc5caf47f0ff345eba282e113daf1c3daafbf256ca8cc17ce` | 1515 |
| [Native](code-admissions/mac-native.json) | `6000163952882b7068245c1fa808c260da28232eea05b642f101963fe1f4b1d8` | 3517 |
| [ARM](code-admissions/mac-docker.json) | `4d93549890ff063e7097cd2ee7211088537791e5cb05f941aacb1fbc66927b95` | 1991 |
| [AMD](code-admissions/vps-docker.json) | `63648d8b08cb7fefe9247901a7d767d03fb729d4edf2446ee9307e2939cdb43e` | 1844 |

Chaque admission a exactement les 13 champs existants et nomme ce même manifest. Les 24 couples propriétaire/rôle de l’index concordent avec les SHA/octets des admissions. L’index indique 16 pins modifiés par rapport à l’index historique ; 24 artefacts admis ne signifie donc pas 24 nouveaux binaires. Les anciens index et admissions restent conservés, sans réécriture ni requalification rétroactive par ce lot.

## Refus et correction de contrat préservés

1. **Clôture Native P2P : défaut de contrat détecté avant exécution.** L’[émetteur V1](source/v1/emit.py) exigeait un champ `passed:true`. Le [vrai reçu Native](history/native-p2p-original-producer-fence.json) n’a pas ce champ ; il contient `exitCode:0`, `originalFenceAfterPassed:true`, `fenceError:null`, 902 fichiers retenus et 20339 ms. La [V2](source/v2/emit.py), dont le [delta](source/v2/source.patch) est conservé, accepte cette alternative exactement typée pour la seule clôture P2P Native. Les autres clôtures exigent toujours `passed:true` ; échec explicite, première erreur ou rafraîchissement d’autorité restent refusés. Aucun faux reçu n’a été fabriqué et aucune qualification P2P n’a été rejouée pour ce contrat.
2. **Préparation du contrôle Git refusée.** Le [reçu Root](history/control-preparation-git-refusal.json) conserve le refus du stub Git système à 78 liens physiques par le garde exigeant un lien. Root a choisi le Git CLT existant à un lien. Le reçu compte zéro invocation du contrôleur et zéro fichier logiciel de production modifié ; cela n’est pas un échec C++ ou applicatif.
3. **Lecture de retour du résultat AMD refusée.** Le [reçu Root](history/public-readback-refusal.json) conserve une comparaison des objets stat complets, incluant `atime`. Aucun champ différent n’avait été enregistré : la cause précise de la différence reste inconnue. Root a corrigé la comparaison vers les neuf champs requis. L’audit avait déjà terminé exit0 ; il n’a pas été rejoué (`auditReruns:0`) et aucun fichier privé n’a été transféré.

Les gels source [V1](source/v1/MANIFEST.json) et [V2](source/v2/MANIFEST.json) sont conservés séparément, avec leurs contrôles EXEMPLES incomplets (`ready:false`), jamais les contrôles Root réels. Les reçus publics PASS attestent les opérations Root terminées, et non une exécution par l’auteur ou les pairs.

## Limites qui restent ouvertes

Ce lot ne démarre aucun réseau ou validateur, ne réserve aucun nouveau domaine, ne crée aucun seed/ledger, ne transfère aucune clé et ne qualifie aucune progression de finalité, restauration ou action de campagne. Les garde-fous de lancement, les permissions de l’essai distinct et les vérifications courantes des fournisseurs/alias Native relèvent du préparateur opérateur séparé. Les preuves de reliage, de remplacement d’un membre Core et des trois P2P sont réutilisées dans leur portée déjà documentée ; leur fermeture SDK/loader complète n’est pas étendue ici. Aucune production n’est déclarée prête par ces admissions.
