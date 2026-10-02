# Finalité externe exigée avant reprise des duties

Candidate de pilote `7434452d18d07d704b0f926d746444ccb9c416bb`, branche `qualification/rpc-wan-harness-20261002`, basée sur d1597d9a. Aucun runtime C++ modifié. Cinq fichiers JS diffèrent de d159 : trois sélecteurs/pilote précédents, plus configuration de restauration et ses tests.

Avant cette correction, le contrôle hors ligne précédant le redémarrage exigeait l’engagement du cœur mais pas explicitement le checkpoint finalisé du reçu d’arrêt. La nouvelle requête contient `requiredFinalized` issu du reçu externe, valide sa forme canonique et vérifie la réponse complète d’audit (ascendance externe, cœur, checkpoint exigé et finalité terminale) avant suppression/remplacement du conteneur et reprise des signatures. Audit final après trois epochs communs inchangé. Budgets et délais inchangés.

44 contrôles passent ; six nouveaux contrôles de pin non aliasé et de finalité absente/malformée échouent avec la configuration précédente (contrôle négatif). La requête reconstruite depuis le vrai reçu d’arrêt et la configuration conservée, dont les empreintes sont vérifiées, correspond aux engagements cœur/finalité/profil/ancre/limites des quatre audits signés déjà qualifiés. Aucun audit runtime supplémentaire ni WAN lancé ici ; la réussite future demeure à démontrer.
