# GC AMD64 — compilation réussie, qualification partielle

Source `f9d078d0`. Build réel : 41 unités cœur fraîches, 99 réutilisées, archive de 140 membres et 8 consommateurs/liens. La compilation ne qualifie pas la production.

Deux runs CTest distincts sont conservés avec leur sortie 8 : run01 comporte 2 PASS et 5 refus avant scénario (fixture manquante) ; run02 exécute seulement ces5 cas et obtient 4 PASS et 1 timeout naturel de 180,027 s. Cela fait 6 tests distincts réussis sur 7, sans run global vert ni réutilisation du naturel ARM. Le dernier jalon naturel imprimé est le slot 16/finalizedEpoch 0 ; le dernier slot réellement traité et la cause du timeout ne sont pas établis. GC au slot 40, fin au slot 56 et réouverture froide naturelle AMD restent ouverts.

Les logs/JUnit sont les fichiers réels : aucun JUnit n’est fabriqué, aucun résultat de l’auditeur composite7PASS n’est revendiqué. `partial-result.json` provient uniquement des lectures/hash/stat de ces preuves publiques et des comparaisons avant/après. Les originaux volumineux avant/après sont référencés par SHA et conservés dans le workspace. Les flags/link après omettent les textes présents avant ; leurs 16 SHA restent exacts.

La CLI a réellement supprimé 1 blob primaire et 1 témoin dans une fixture synthétique fermée, vérifié 5 refus et un reçu 0600/nlink1. Le test adverse fournit 30 cas, 2 fsync et l’observation de suppression primaire avant témoin. Ce ne sont ni des bases de campagne ni une panne matérielle.

`foundation/` est une copie byte-exact du paquet préliminaire de 21 fichiers, dont le manifeste original et les refus du collecteur restent séparés. Ses anciens statuts d’attente demeurent historiques ; le présent manifeste décrit le résultat actuel. Les refus V2 aliases, V4 header et run01 fixtures restent conservés. La CI GitHub f9 n’a exécuté aucune étape (refus de facturation), le code a été livré par Git mais aucun runtime n’a été mis à jour.

Aucune image runtime/installation, clé/env, capsule privée, DB ou preuve privée VPS en attente n’est incluse. La relation bridge/token, la reprise des duties, la finalité commune et les autres critères de production restent ouverts. Aucun compiler/test/Docker/SSH/runtime n’a été exécuté par ce collecteur.
