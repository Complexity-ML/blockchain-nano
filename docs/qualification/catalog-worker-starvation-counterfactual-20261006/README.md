# Contention de deux travailleurs — comparaison contrôlée

Parent main 5f0faadf et cœur natif cohérent qualifié de ce parent. Quatre jobs avec la file SimulationQueue réelle (deux travailleurs, limites inchangées), catalogue réel et réacteur réel. Capture retenue volontairement et second acquéreur en attente ; contrôles de génération/release derrière eux.

Baseline, original 14001 sortie 0 : contrôle direct du registre réactif, mais deux contrôles de la file expirent. Draft, original 48776 sortie 0 : capture concurrente refusée immédiatement, les deux contrôles de la file terminent avant expiration. Dans les deux : quatre réservations drainées, jeton primaire inchangé, entrées et hashes conservés. Ce sont des attentes volontairement provoquées ; aucun délai réel WAN ni cause WAN ne sont prouvés. Pas de transport HTTP réel dans cette comparaison.

Le draft modifie uniquement le comportement du verrou de capture dans un arbre isolé et utilise le même cœur. Il n’est pas une qualification du runtime intégré. Prochain lot : exception occupé distincte avec code RPC explicite, test HTTP réel et qualification cohérente natif/ARM64/AMD64. Les acquéreurs concurrents doivent gérer le refus transitoire ; les tests de quota conservent leurs bornes, les doublons et la reprise. Pas de thread ni quota supplémentaire.
