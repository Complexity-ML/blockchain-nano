# Cache des octets publiés — candidat non qualifié

Commit fa390c3b, branche `qualification/checkpoint-serialization-20261002`, poussé puis récupéré par Git sur le VPS. Une chaîne privée garde les octets canoniques du dernier checkpoint seulement après une publication durable réussie ; son transfert après commit est statiquement garanti sans exception. Les comparaisons exactes des octets remplacent le hash redondant de détection des doublons. À la réouverture, cache vide et repli cryptographique original conservé.

Même en cas de doublon, les vérifications du checkpoint sur disque, de son token et du témoin éventuel restent exécutées ; un échec rend le réacteur indisponible. Aucun checkpoint, token, format, règle de consensus, WAL ou écriture synchrone modifié. Un seul snapshot supplémentaire est conservé en mémoire, borné par la limite d’enveloppe : empreinte mémoire et comportement à qualifier.

Nouveau scénario `consensus_bellatrix_publication_cache` : retransmissions idempotentes puis modification du checkpoint couplé par un second writer ; la retransmission doit refuser le token devenu stale et fermer le réacteur. Résultat non acquis.

Un champ privé modifie la taille du réacteur : recompilation complète du cœur et des cibles nano/Bellatrix/Capella/tests stockage et témoin, sans réutiliser les anciens objets compilés avec le header précédent. Build `/build/published-bytes-fa390c3b-20261002`, image AMD64 existante ; pas de nouvelle image. Contrôleur sélectionne douze tests pertinents avec les délais CMake d’origine. P2P existant séparément identifié, pas de qualification globale d’une nouvelle image réseau.

Compilation réellement active constatée dans `nano-published-bytes-candidate-control-amd64-test`. Rapports `/home/ubuntu/nano-published-bytes-candidate-control-20261002-reports`. Attendre les résultats et les manifestes avant toute revendication de gain ou promotion. Le parent conserve 7/8 réussites et un timeout de partition au slot observé 96 ; tous les échecs historiques et critères de production restent ouverts.
