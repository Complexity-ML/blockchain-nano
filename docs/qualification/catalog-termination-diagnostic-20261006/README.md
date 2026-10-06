# Diagnostics de terminaison du catalogue — qualification ciblée sur trois environnements

Le premier refus reste dans reason/failureStage ; terminationReason/terminationStage enregistrent la cause d’arrêt. catalogExhausted distingue un catalogue parcouru jusqu’au bout de closed, qui indique seulement une libération confirmée. Si le wrapper tête/historique expire ou est annulé, catalogProgress conserve ses compteurs, leur caractère provisoire et le premier refus. Aucun ACK, admission, quota, signature ni règle de consensus n’est changé.

Contre-exemple sur l’en-tête 7c6c1fd3 : sortie 1 attendue, export indisponible masqué par dependency-depth-limit. Correction finale : 7 nouveaux scénarios (5 causes d’arrêt puis délai et annulation du wrapper avec refus préalable et réponse tardive). Sur Mac natif, Docker ARM64 et Docker AMD64/VPS : ces 7 scénarios, les 46 scénarios de catalogue, les 19 de priorité de tête et les 10 d’alternance historique passent. Les CMake SDK/CTest ARM64 et AMD64 passent chacun 4/4. Relais nano-p2p recompilés sur les trois environnements ; empreintes conservées et lancement sans arguments Usage/sortie 1 attendu vérifié. Ce lancement ne qualifie pas le réseau.

Images et volumes existants réutilisés, sans build d’image ni mutation/transfert de base ou stockage de signatures. Core inchangé 5787915e. Réseau v15 déjà terminalement arrêté avant compilation. Les commandes, sources figées, manifests, sorties et journaux sont conservés dans integrated.

Échecs conservés : 60169/sortie 1, inclusion initiale entre guillemets sélectionnant le mauvais en-tête pour le baseline ; 86620/sortie 2, nouveau test CMake sans répertoire d’inclusion explicite. Sources/proofs v1 séparées. Inclusion et chemin corrigés avant les qualifications finales, sessions 74537/89918/82021 et collecte 66305, toutes sorties 0.

La convergence WAN et la cause complète de l’échec v15 restent ouvertes. Le nouveau relais n’a pas encore participé à une campagne réseau. Cette qualification de diagnostic ne clôt aucun critère de consensus, de production ou de revue indépendante.
