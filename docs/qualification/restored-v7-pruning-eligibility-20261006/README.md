# Ancre et éligibilité d’élagage du VPS v7 arrêté — 6 octobre 2026

Lecture Docker AMD64 hors réseau, bases montées en lecture seule. Engagement exact v7 7285/7f4d9e3f et témoin du cœur vérifiés ; 1393 fichiers de données identiques avant/après. Aucun signer rouvert ou réexaminé par cet outil, aucune suppression, aucune reprise de duties. Audits canoniques et de signers précédents conservés séparément.

Le Store retient 256 blocs sur la borne locale de 256. Son ancre égale la racine finalisée epoch 1284, slot 10271. Les 256 blocs descendent tous de cette ancre : zéro bloc supprimable par l’algorithme existant d’élagage sur frontière finalisée. L’âge de la finalité satisfait la garde min_seed_lookahead=1 (horloge epoch 1401), mais la frontière ne peut plus avancer faute de nouvelle finalité. Le graphe a dix points de branchement et douze feuilles ; il ne faut pas supprimer arbitrairement ces branches encore post-finalité.

SSZ des états : 7 956 128 octets sur 67 108 864. Snapshot sérialisé : 21 914 821 octets sur 134 217 728. Cela identifie la borne en nombre de blocs comme la première atteinte dans ce snapshot, sans estimer la RAM réelle ni garantir l’espace pour une transition future. Aucun vote signé conservé dans cet instantané. L’absence actuelle de votes ne qualifie ni le réseau ni son rattrapage.

Diagnostic structural fondé sur le snapshot local engagé : parse les en-têtes, vérifie les parents/slots et compte les descendants ainsi que les contextes de votes hors candidat. Il ne réexécute pas la transition de chaque bloc, ne prouve pas le fork choice historique et ne qualifie pas une opération d’élagage. La validation cryptographique/canonique précédente reste dans son périmètre propre.

Deux échecs de contrôleur/build conservés : ancien nom de source puis include manquant du décodeur SSZ. Version fixed2 compilée/liée sortie 0, pas d’OOM ; source distincte et logs des trois versions conservés. Lien à l’archive actuelle atomic-proposal AMD64 ; runtime b5ee00ac précédent intact, images réutilisées. Le diagnostic ne déploie pas le nouveau runtime.

Conséquence : une relance du prune existant ne libérera rien sur ce VPS. Il reste à fournir une rétention capable de récupérer le progrès sous pression, puis démontrer une finalité commune avec les nouveaux relais et cœur. Une expérience de profil de ressources plus large devrait rester distincte de la qualification à 256 blocs et mesurer les ressources ; augmenter la borne seule ne ferme pas ce défaut. v7 reste FAILED, portabilité physique et production ouvertes.

run/capture.json contient les seuls compteurs et racines exposés ; les bases restent sur le VPS. manifest.json couvre les pièces conservées, hors lui-même.
