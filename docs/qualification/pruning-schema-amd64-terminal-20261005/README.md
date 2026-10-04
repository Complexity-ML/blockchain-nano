# Schéma pruné — AMD64 terminal, 5 octobre 2026

Révision 59fa653033aefb23651c32f1d986ac644e4866d2, source figée obtenue via Git. Même lot que ARM64 : ancien lecteur lié au vrai cœur efbeb9bc, nouveau schéma atomique, copie physique/bornes, onze corruptions, checkpoint, pertes simulées de suffixe WAL pruné et non pruné, restauration Capella. Deux contrôleurs exit 0 ; revue indépendante exit 0. 16 497 fichiers source et binaires re-hachés ; anciens programmes/bibliothèques qualifiés inchangés. Image et volume AMD64 existants réutilisés, sans rebuild d'image.

Ancien lecteur : contrôle positif sur nano/1 non pruné ; refus explicite de nano/2-state-pruned sans modification. Nouveau lecteur accepté ; deux désaccords de schéma/marqueur refusés. Capella local : 64/64 journaux avancés, témoins indépendants conservés, conflit refusé avant signature, finalité 2 → 7, source/sauvegarde inchangées.

La modification n'est pas promue au runtime main. Aucune migration automatique des anciennes bases expérimentales nano/1 prunées, panne électrique, nouveau WAN, gain SST ou production complète acquis. Les preuves WAN 092a153c restent attribuées à leur révision et schéma antérieurs. Sources, commandes, logs, hashes et revue archivés ; aucune clé ou base ajoutée à Git. Tous les échecs historiques conservés.
