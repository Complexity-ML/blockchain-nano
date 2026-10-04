# Schéma de base prunée — ARM64, 5 octobre 2026

Branche feature/pruning-schema-guard, runtime 59fa653033aefb23651c32f1d986ac644e4866d2. Contrôles Docker terminaux exit 0, images et volumes existants, aucun rebuild d'image. Sources inchangées ; bibliothèques et anciens tests qualifiés conservés. Seul objet storage.cpp.o remplacé dans une copie du cœur 4734f285 ; nouveaux exécutables et ancien lecteur compilés/linkés séparément.

Le premier élagage écrit nano/2-state-pruned dans le même WriteBatch synchrone que le checkpoint, le marqueur et les suppressions. Un vrai lecteur lié au cœur historique efbeb9bc ouvre d'abord la base non prunée nano/1 (contrôle positif), puis refuse explicitement la base prunée avec unsupported Nano database schema. Lectures et refus ne modifient aucun fichier. Le nouveau lecteur rouvre correctement checkpoint/ancre/état récent ; deux désaccords schéma/marqueur sont refusés.

Sept scénarios terminaux : ancien/nouveau lecteur, copie physique/bornes, onze corruptions, checkpoint, suffixe WAL pruné conservé/perdu, suffixe WAL de checkpoint conservé/perdu, restauration Capella locale. Cette dernière reprend 64 journaux avec témoins indépendants, refuse un conflit avant signature, avance la finalité 2 → 7 et laisse source/sauvegarde inchangées. Pas de panne électrique ou nouveau WAN déduit.

Les anciennes bases expérimentales prunées nano/1 sont désormais refusées ; aucune migration automatique n'est implémentée. Retour à un ancien binaire : restaurer une sauvegarde non prunée compatible, jamais rouvrir en force ni changer manuellement le marqueur. Une procédure de migration validée et la qualification AMD64 restent ouvertes. Cette branche n'est pas intégrée au runtime main ; la production globale reste non qualifiée.

Contrôleurs, commandes, lecteurs, journaux, hachages indépendants et manifests de sources compressés conservés. Le test de refus ancien utilise le vrai cœur historique ; il n'est pas remplacé par une simulation du comportement ancien.
