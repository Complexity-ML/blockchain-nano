# Reprise : preuves terminales et garde de stockage — 12 octobre 2026

Ce lot conserve les échecs antérieurs et ne qualifie pas la production.
Le cœur C++ demeure celui de `85eb492f` ; la nouvelle modification opérationnelle
empêche le contrôleur de démarrer une campagne dans un répertoire temporaire.
Le VPS et les anciens volumes ont été audités en lecture seule. Aucun signataire
V37, duty ou réseau n’a été redémarré ; aucune image Docker n’a été reconstruite.

| Travail | Résultat actuel | Portée |
| --- | --- | --- |
| Build Mac durable | Release, sortie 0, trois exécutables et bibliothèque identifiés | Source `0842078e`, Apple clang 21 ; 3 CTest ciblés passent, pas une release complète |
| Localisation des données | 11 cas utiles passent, refus du contrôleur avant création de campagne | Chemins réels et liens vers `/tmp`, `/var/tmp`, `os.tmpdir()` ; ne remplace pas les sauvegardes |
| Anciens volumes Mac | 14 domaines genesis différents, aucun parmi les 154 couples existants ne correspond à V37 | 294 autres couples demandés absents ; fichiers de journaux inchangés, aucune acceptation assouplie |
| Domaine VPS | Seed et tête authentifiés par le C++ qualifié | Racine des validateurs, profil et checkpoint liés ; ne reconstruit pas les journaux Mac |
| Projection après arrêt | **Échec réel**, `empty Altair sync active set` | Sept chunks terminés ; rejeu complet jusqu’au temps réel, duties et finalité non qualifiés |
| CI GitHub | Aucune étape exécutée sur run 38285487283 | Refus actuel pour paiements échoués ou limite de dépense ; aucune action de facturation |
| EVM/RPC | Revue statique du profil activé | Capella exige Shanghai/beacon ; les genesis et Compose génériques restent legacy, profils modernes non activés |

La projection a mesuré **5 j 15 h 58 min** d’interruption et tenté 81 861 slots
au-delà de la tête conservée. Le dernier chunk terminé atteint le slot 34520,
époque 4315 : **58 validateurs actuellement actifs**, sept sortis et 26 nouvelles
sorties programmées cumulées (ce compteur inclut des sorties déjà exécutées).
Le refus du chunk suivant concerne la sélection du comité à l’époque courante
du calcul + 1. Son époque exacte reste inconnue. Il serait faux de déclarer
qu’il n’y avait déjà plus de validateurs actifs au dernier état terminé.

Durée 64,946 s, pic RSS 89 128 KiB ; aucune panne de ressources. Les 3241 fichiers
de la source de projection sont inchangés : 3220 données récupérées, 16 fichiers
d’audit privés et cinq configurations. Les 3216 fichiers de la source originale
ont leurs preuves séparées de préservation et ne sont pas inclus dans ce total.
Les refus précédents du banc, de compilation et de liaison restent archivés.
Ne pas augmenter simplement le délai administratif, réinitialiser les pins,
fabriquer des journaux ou modifier l’horloge pour prétendre à une reprise.

Les dossiers Mac V37 restent vides. La découverte limitée de métadonnées locales
indique aucune destination Time Machine configurée et aucun disque de sauvegarde
monté ; cela n’exclut pas une copie ailleurs. Aucune sauvegarde/restauration
Time Machine n’a été déclenchée. La question sur les autres sauvegardes demeure
ouverte. Ces métadonnées locales ne sont pas exportées.

Prochaine voie de qualification : une campagne **privée distincte**, avec clés
BLS et domaine de genèse isolés, stockage persistant et restauration entière
authentifiée. Elle devra démontrer ses propres duties/finalité ; elle ne pourra
pas être présentée comme une restauration de V37. Un simple changement du hash
de genesis d’exécution n’isole pas nécessairement le domaine de signature BLS.
La conception de cette campagne reste à finaliser avant son lancement.

Les qualifications acquises sont conservées dans leur périmètre daté. Les
exigences restantes des quatre matrices demeurent ouvertes : cycle des validateurs, partitions/asymétries, WAN A, charge bornée,
ressources, EVM/RPC, release/CI, exploitation, revue indépendante et relation
Base–Nano/bridge USDC/Doppler. Aucun fonds réel, vente ou déploiement public.

Sources et limites : [build natif](native-candidate/README.md),
[anciens volumes](mac-volume-journal-audit/metadata-refinement/README.md),
[projection réelle](long-gap-state-projection/README.md),
[CI actuelle](ci-522-current-review/summary.json),
[activation EVM/RPC](evm-activation-review/README.md),
[garde des chemins](persistent-runtime-path-guard/summary.json).
`export-manifest.json` lie les copies publiques exactes ; environnements de
signature, capsules privées et bases n’ont pas été copiés dans le dépôt.
