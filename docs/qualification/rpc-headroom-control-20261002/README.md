# Optimisation de la sonde proche de la borne haute — contrôleur préparé

Candidate `d1597d9a6987d0839ed2f0c7ded268aead35d071` ; baseline `911c7f415eb325a8b9005066e0a74b69d896155e` (mêmes nouveaux tests, algorithme précédent). La sonde proche reste réellement exécutée ; en cas d'échec son intervalle est déjà dans la tolérance existante 1,5 %. Aucun changement des plafonds, de la simulation initiale, du délai de file 4 s, des règles de succès ou de la cancellation.

Nouveaux cas Paris/Shanghai à 2/4/8 niveaux, budget déterministe renforcé, recherche indépendante du gaz minimal, exécution de la réponse RPC et de la transaction signée, plafonds serrés et plafond insuffisant. Contrôle négatif : les nouveaux tests doivent rejeter l'ancien algorithme par la seule assertion de performance attendue. Puis régression RPC complète et cas réseau WebSocket Paris original, 16 clients, timeout 480 s. Temps unitaires bornés à 180 s.

Réutilise image et volume AMD64 existants, avec sources séparées RO, compilation incrémentale, empreintes avant/après et traces opt-in. Le volume est intentionnellement avancé vers la nouvelle candidate ; résultats précédents conservés comme historiques. Aucun autre conteneur actif autorisé par ce contrôleur.

Contrôleur syntaxiquement vérifié, exécution non encore acquise. Aucun gain mesuré, fermeture de l'échec historique ou résultat production revendiqué. Candidate non promue sur main ; régression complète cohérente deux architectures et WAN restent requis.
