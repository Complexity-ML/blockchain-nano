# Admission brute malformée — correction ciblée

La candidate complète figée aacc56da échoue sur `http` : « new connection bypasses transaction budget ». Aucun redémarrage de la régression n'a été nécessaire pour observer cet échec.

Cause : `eth_sendRawTransaction` rejetait l'hex/RLP inconnu avant `charge_transactions`, contrairement au gossip. Les enveloppes réellement étrangères restent rejetées avant la charge ; les enveloppes malformées consomment désormais le budget local partagé avant de propager l'erreur. Les limites de taille et de signature, la capacité/refill, les délais, les règles de consensus et les réponses étrangères ne changent pas.

Nouvelle couverture déterministe : 128 tentatives réparties entre hex invalide, type inconnu, longueur RLP tronquée et liste vide ; épuisement bloque ensuite transaction saine et hex invalide ; enveloppe étrangère reste identifiable à épuisement ; état/events/pool intacts ; admission saine après refill. Le test HTTP existant couvre nouvelles connexions, batch JSON-RPC et passage à WebSocket après exhaustion.

Docker ARM64 : RPC (993 checks), isolation d'admission et HTTP passent, trois tests, zéro skipped, 2,59 s environ. Sélection exacte et hashes dans `fixed`. Cette correction est distincte de l'expérience d'estimation/GMP, qui n'est pas promue et n'a pas fermé le gate de charge. Aucune conformité universelle ou résistance à un flood continu n'est déduite de ces contrôles.

La régression complète active reste attachée à son ancien SHA et conserve son échec HTTP. Une CI nouvelle et une qualification AMD64 de la correction restent à observer après intégration ; aucun service VPS n'est mis à jour par cette archive.
