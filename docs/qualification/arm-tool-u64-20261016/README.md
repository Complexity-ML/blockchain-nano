# ARM Tool — header uint64/GMP qualifié dans un consommateur

Le 16 octobre 2026, la tentative02 a produit et exercé un nouveau Tool Linux ARM64 à partir des sources publiques `a6fdbaed…`, incluant le header `fresh_lifecycle_terminal.hpp` `6681926f…`. La portée est une TU, un lien et deux sélecteurs CLI ; le Core et l’image existants ont été conservés. Le [terminal réel](evidence/terminal-02.public.json) et la [jointure postqualification](evidence/postqualification-join-02.public.json) sont copiés sans modification.

| Résultat du Tool ARM | Valeur |
| --- | --- |
| Exécutable | nano-capella-deposit-audit |
| SHA-256 | `0058ecb3061234c6025ffff97f860692e2125f53011832f863621899d2e24fa9` |
| Taille | 6 212 328 octets |
| Compilation / lien observés | 6 592 ms / 202 ms ; ces durées ne sont pas un benchmark de nœud |

Un calcul M indépendant est égal au MD réellement produit par la compilation. Le lien conserve les cinq archives et deux objets assembleur existants, dont le Core `9ccadf8f…`. Le terminal rapporte 42 phases réussies. Les 523 dépendances observées et les 441 fichiers SDK admis dans le même conteneur sont des comptages de portées différentes ; ils ne constituent pas une partition à additionner. Les anciens enregistrements d’autorité ne sont pas rafraîchis. Les mêmes enregistrements SDK FD/path neuf champs et SHA ont été vérifiés après les sélecteurs ; aucun inode invité n’a été comparé à un inode hôte ou à un autre conteneur.

Les neuf sources overlay correspondent au Git courant selon la [jointure initiale](evidence/current-git-source-join.public.json) ; les 88 autres sources projet atteintes ont ensuite été rapprochées des blobs de `a6fdbaed…`, sans mismatch. La [comparaison publique neuf/quatorze](evidence/profile-9-vs-14.public.json) distingue les neuf feuilles Tool, les trois feuilles propres à Submit et les deux headers Tool partagés restés sous `/work`. Cette comparaison source seule n’était pas un inventaire de compilateur ; le lot réel ajoute les gardes M/MD et la jointure de ses dépendances projet.

Les sélecteurs `--fresh-lifecycle-self-test` et `--fresh-lifecycle-observe-self-test` ont passé six groupes et 21 refus attendus, créé sept signatures BLS de fixtures et zéro signature opérationnelle. Ils n’accèdent ni à l’environnement applicatif, ni à la base, ni au réseau ou au runtime. Ils qualifient ces fonctions CLI du Tool ; ils ne prouvent pas le chemin complet de paiement/retrait uint64, le cycle complet des validateurs ou un vrai transport HTTP/exec.

Objet, MD et exécutable ont reçu de nouvelles admissions stables ; le producteur a engagé le digest de leur descripteur privé et Root a vérifié les trois copies hôte après qualification. Les fichiers privés et leurs métadonnées brutes ne sont pas publiés. Conteneur et volume de travail possédés sont absents après nettoyage. Aucun configure, compilation du Core, construction/pull d’image ou accès VPS privé n’a été exécuté par ce lot.

Les échecs restent visibles : le premier [staging](evidence/staging-attempt-01.public.json) a refusé avant publication d’une admission ; la [préparation corrigée](evidence/overlay-root-preparation.public.json) admet seulement les nouvelles copies. La [tentative01](evidence/attempt-01.public.json) a échoué avant M/CXX/lien/CLI, lorsqu’un tuple de snapshot remplaçait le nom d’une preuve ; son nettoyage a réussi. Son code réellement exécuté et son manifeste restent sous `source/attempt-01/`. V2 ne change que ce nom et la validation des feuilles de sortie. Ni cet échec ni la lacune de digest brut historique de l’ancienne admission observe ne sont effacés ; `historicalObserveAdmissionRawDigestRecovered:false` et `historicalSdkAuthorityRecovered:false` restent explicites.

Le [reçu de publication précédent](evidence/prior-git-publication.public.json) et le [pull source VPS](evidence/prior-vps-source-pull.public.json) établissent la base `a6fdbaed…` ; le pull n’a ni rebuild ni redémarré le runtime. L’[observation CI de cette base](evidence/ci-observation.public.json) retourne zéro run : aucun succès CI n’est revendiqué. Le précédent échec de facturation reste conservé dans la [qualification Native Legacy](../native-legacy-20261016/README.md).

Le contrôleur V2, son README/schema et manifeste, le code/manifeste V1 et l’unique utilitaire dont les définitions AST sont utilisées figurent sous `source/`. Les README/schema V1 ne sont pas dupliqués. Les sources opérateur requièrent les autorités privées originales pour une invocation ; aucune n’est incluse.

Ce lot ne qualifie pas Submit ARM, le Tool AMD avec ce header, P2P, toutes les résolutions différées du chargeur, la configuration CMake complète ou les admissions de campagne des 24 rôles. Les preuves antérieures et leurs limites restent conservées. L’échec global historique des entrées Native (sept fichiers hors fermeture sélectionnée) et l’alias historique inconnu restent ouverts. Le header était déjà versionné : aucune source C++ n’est modifiée dans ce paquet, et CSV/RPC restent inchangés. Aucun EIP ou fork n’est promu. Le goal reste actif ; reprise des duties, finalité multi-propriétaire et production restent non qualifiées.
