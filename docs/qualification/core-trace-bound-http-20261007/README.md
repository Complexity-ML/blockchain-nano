# Diagnostic RPC : borne partagée et catégories privées

Le compteur de l’observateur précédent était copié pour chaque connexion HTTP : le contre-exemple reproduit 1000 diagnostics pour une borne prévue de 64 (sortie1 attendue). La correction partage un compteur atomique saturant entre les copies de callback. Elle conserve la borne64 existante et ne modifie ni règles de consensus, ni budgets de relais, ni retries, ni délais.

Les refus indiquent maintenant une catégorie d’inventaire strictement autorisée. Tokens, racines, handles, paramètres et corps d’erreur arbitraires restent exclus. Les observateurs qui lèvent une exception n’altèrent pas la complétion RPC.

Qualification ciblée sur Mac natif, Docker ARM64 et Docker AMD64 : compilation/liaison du relais, nouveau test réel HTTP (100 connexions,100 callbacks,4 diagnostics exactement, indices block/attestation/null, busy hint intact et paramètres privés absents),18 variantes keepalive/erreur/observateur,9 scénarios catalogue busy/retrait/nettoyage de handle tardif. Primitives copie/concurrence/rédaction qualifiées séparément sur les trois architectures ; contre-exemple parent exécuté dans les deux Docker. Les hashes des relais parents restent inchangés. Images et volumes existants réutilisés ; aucun nœud démarré.

La nouvelle cible CMake est enregistrée dans les sources. Les exécutions présentes sont des commandes incrémentales explicites, pas une nouvelle configuration CTest ni une release propre complète.

Cette correction rend le diagnostic borné et attribuable. Elle ne démontre pas que la contention explique l’échec global : V34 reste FAILED, zéro nouvelle finalité commune. Le test HTTP utilise des serveurs de boucle locale et ne qualifie pas le réseau WAN ou le consensus. Production, revue indépendante et Base–Nano/Doppler restent ouverts.
