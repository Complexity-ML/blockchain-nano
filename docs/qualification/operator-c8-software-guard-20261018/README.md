# Garde des logiciels opérateur C8 — 18 octobre 2026

Ce lot relie une garde source versionnée à des vérifications ciblées de logiciels.
Les résultats ci-dessous sont ceux communiqués par Root ; ils ne constituent
ni une nouvelle campagne de consensus ni une admission opérateur complète.

## Résultats ciblés

| Vérification Root | Résultat |
| --- | --- |
| Tests `native-loader-guard` et `lifecycle-driver-guard` | 13/13 PASS, exit0, stderr vide |
| Identités logicielles Mac natif | 9 assets + 1 shim auxiliaire, PASS |
| Identités logicielles Docker ARM64 | 8 assets, PASS |
| Identités logicielles Docker AMD64 | 7 assets, PASS |
| Jointure P2P Docker ARM64 sélectionnée | 175 entrées tenues, PASS |
| Jointure P2P Docker AMD64 sélectionnée | 175 entrées tenues, PASS |

Les24 assets des trois propriétaires restent distincts du shim auxiliaire.
Ces comptes ne prouvent pas, à eux seuls, la fermeture de l’index complet,
les admissions des trois propriétaires ou toute la provenance de compilation.
Les deux jointures P2P sont sélectionnées ; elles ne qualifient pas un nouveau
build P2P, son exécution ou l’interopérabilité réseau.

## Exporteur natif et shim

Le format de campagne C8 sélectionne explicitement une garde nouvelle.
Le format legacy garde son lecteur existant ; les24 rôles/assets et les13
champs d’admission propriétaire ne sont pas élargis.
Le manifeste externe lie l’exporteur au shim co-localisé par SHA et taille,
avec un reçu structurel Root séparé et les neuf champs stat originaux.
Ces champs sont des chaînes décimales, sans perte de précision des nanosecondes.

La garde exige l’identité attendue `@rpath/libnano-test-sync-fault.dylib`
et le premier rpath `@loader_path`, selon le reçu structurel épinglé.
Elle tient les deux fichiers dans les fences globales et les vérifie avant
et après l’appel exporteur prévu par le launcher, y compris en cas d’erreur.
Cette capacité du code n’annonce aucune exécution d’exporteur dans ce lot.
Le launcher ne lance pas d’inspecteur externe ; le reçu Root reste son autorité
structurelle. La fermeture globale dyld et le SDK global restent hors portée.

## Refus source conservés

Le worker P2P SOURCE01 n’a pas été exécuté : sa revue a refusé les problèmes
de croissance des métadonnées et de cleanup ; SOURCE02 apporte le correctif.
Le worker de sélection SOURCE02 n’a pas été exécuté : sa revue a refusé les
mauvais namespace des feuilles ; SOURCE03 apporte le correctif.
Ces refus de source ne sont pas transformés en échecs runtime ni effacés.
Les corrections ne remplacent aucune autorité originale par une capture courante.

## Limites maintenues

Les13 tests ciblés ne sont ni toute la suite de régression ni une CI globale.
Ce lot ne reconstruit ni Core, ni les exécutables, ni les images ; il ne démarre
pas de runtime de chaîne et n’autorise aucun nouveau duty, signature ou fonds.
La campagne03 reste FAILED et consommée. La campagne04 n’est pas autorisée.
La garde logicielle ne répare pas un journal perdu et ne prouve pas une
restauration opérateur suivie de duties ou une progression de finalité commune.
Les profils de consensus et les conditions économiques restent inchangés.
La préparation complète des autorités, les admissions opérateur et la
qualification de production restent ouvertes au-delà de ces résultats ciblés.

Le contrôle pur des locators confirme 24 actifs et 9 montages fichier supplémentaires
(5 ARM64 et 4 AMD64), sans copie ni commande Docker. Cet index d’identités est
distinct des trois admissions et de la fermeture composée Core/main encore ouvertes.
