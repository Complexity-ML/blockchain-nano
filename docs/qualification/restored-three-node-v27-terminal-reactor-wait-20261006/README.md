# V27 FAILED et contre-exemple de verrou

Main d5b2d26f, cœur EE5 et relais 2f93dda0. Le contrôleur original 42291 termine avec sortie 1 : zéro nouvelle finalité commune en 900 secondes (trois requises). Les trois reçus d’arrêt sont conservés. Aucun redémarrage de campagne ni nouvelle image Docker.

Les audits originaux 34309 et 8101 terminent avec sortie 0. Ils vérifient les engagements d’arrêt et les fichiers inchangés : natif 1667, Docker Mac 1693, VPS 2648. Les configurations/requêtes du VPS ont été dérivées sur le VPS ; elles ne sont pas exportées ici. Les captures publiques et reçus restent des preuves de restauration/immutabilité, pas de finalité qui avance.

Le diagnostic original 85421 termine avec sortie 0 et reproduit catalog-cancellation-waits-for-reactor. Pendant que le vrai réacteur détient son mutex pour une admission annulée avant publication, l’acquisition attend, puis continue d’attendre après son annulation. Aucun checkpoint d’annulation n’est appelé pendant le verrou. Un second appel reçoit occupé ; après libération la capacité revient et le primaire reste inchangé. Ce contre-exemple utilise le cœur EE5 non modifié ; il ne prouve pas que cette attente explique à elle seule V27 ni l’anomalie WAN A.

Une correction distincte est en cours : acquisition non bloquante sous verrou d’admission, retour occupé via le type/RPC existant, mêmes quotas et barrières de requête. Aucun résultat de cette correction n’est inclus dans cette preuve du défaut historique. Finalité commune, autres critères de production, bridge/lancement et revue indépendante restent ouverts.
