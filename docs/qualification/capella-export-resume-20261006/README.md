## Reprise de préparation des exports Capella — 6 octobre 2026

Le relais conserve par pair/session les enveloppes exportées localement et validées avant expiration, avec bornes indépendantes de racines et de mémoire chargée. Une reprise ne redemande plus ces mêmes exports au cœur. Le cache ne vaut jamais admission distante : il faut toujours un ACK accepté, parent avant enfant. Les deux chemins de renouvellement de session effacent ce cache.

Régression avant/après : trois tentatives ne transmettaient aucun bloc avec les anciens headers (sortie 1 conservée). Avec le correctif, elles transmettent et font admettre trois blocs, en cinq demandes d’export au total (sortie 0). Les gardes couvrent ACK perdu/tardif, refus, nouvelle session, éviction et entrée surdimensionnée. Les 106 cas existants concernés passent en Docker ARM64 et AMD64 ; transport simulé, aucune validité de consensus déduite des mocks.

Les relais complets compilent et se lient sur Mac natif, Docker ARM64 et Docker AMD64. Leur lancement sans arguments rend l’usage attendu et sort 1 : c’est un contrôle CLI négatif, pas une exécution réseau. Les exécutables précédents et le cœur b5ee00ac sont inchangés. Images et volumes de build existants réutilisés, aucune reconstruction d’image.

Les commandes reprennent les flags et liens CMake existants. La nouvelle cible est enregistrée dans network/CMakeLists.txt, mais une configuration CMake fraîche n’a pas été qualifiée. L’ARM fixed-current porte les derniers octets du header ; le premier résultat ARM est gardé avec son périmètre exact.

La campagne v7 reste FAILED. Ce correctif n’est pas déployé ni qualifié WAN ; la saturation du Store à 256/256, la finalité commune qui avance, les duties restants, la portabilité physique et les critères de production demeurent ouverts. Aucun journal ni témoin n’a été réinitialisé. Aucun fonds, vente ou déploiement public.

summary.json relie les hashes des sources aux reçus Docker ; native/program.json et les reçus program.sha256 identifient les nouveaux exécutables. Les états Docker ne contiennent pas les variables d’environnement. manifest.json couvre les pièces conservées, hors lui-même.
