# Garde après fermeture du relais

Complément de la qualification timeout-preparation-integrated-20261006 : un seul header change, capella_head_catalog_relay.hpp. La fonction fail retourne immédiatement si le relais est déjà fermé, avant toute modification du curseur. Un callback de délai déjà prêt ne peut donc réintroduire une préparation après completion. Aucun changement des limites, quorum, wire, cœur ou règles d'ACK.

Natif, Docker ARM64 et Docker AMD64 : douze cas affectés (neuf variantes de préparation, head catalog, termination, history fairness) et runtime nano-p2p compilés. Sources figées 349 fichiers et hashes avant/après vérifiés. Sorties distinctes, anciennes qualifications conservées. Le scénario wire qualifié dans le lot précédent est réutilisé : scan réel des dépendances du compilateur prouve que le header modifié n'est pas une entrée de cette fixture, et tous ses autres fichiers et le cœur restent identiques. Il n'est pas rejoué.

Ces preuves locales ne qualifient ni la finalité WAN ni la régression de branche V24. Les échecs et critères de production non démontrés, bridge/fair launch et revue indépendante restent ouverts. Images/volumes réutilisés, aucun fonds ni déploiement public.
