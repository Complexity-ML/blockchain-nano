# Contre-exemple : préfixe de catalogue répété

Base 6076684d, relais qualifié 0e2d943a, compilation/lien 0, test attendu en échec 1. Huit tentatives d’inventaire racines 1..4 : les racines 1 et 2 sont définitivement refusées ; l’export de 3 n’est pas confirmé. Chaque tentative reprend à 1, donc la racine indépendante valide 4 n’est jamais envoyée/admisе. Requêtes, réponses et classifications exactes dans run.log. Aucun refus ni résultat inconnu n’est utilisé comme ACK.

Défaut de progression reproduit dans le vrai adaptateur C++, avec transport/core modélisés. Ce test ne qualifie ni admission EVM, ni finalité, ni une cause WAN globale. Une correction doit conserver une position de sélection distincte des admissions, rester bornée, traiter les nouveaux snapshots/racines supprimées, permettre le retour sur les inconnus/refusés et conserver les protections génération/session/annulation.

Exécuté après la sortie terminale v16 et l’arrêt de ses propriétaires ; aucune compilation concurrente avec le réseau. Pas de rebuild d’image ni de transfert de base/clé.
