# Reprise v11 : continuations des blocs — résultat terminal

Contrôleur original session 23882, checkout c193eb4a ; core 504c9df8, relais 76bb2468. Résultat FAILED, sortie 1 : aucune avancée commune depuis 1284. Critère initial de trois avancées distinctes en 900 s conservé, sans restart ni prolongation. Voir résultat et compteurs bruts ; aucune circulation de blocs ni duty locale ne remplace la finalité.

Trois arrêts originaux propres/sans OOM. Journaux/témoins conservés séparément, nouveaux engagements exacts post-v11 requis pour toute reprise ultérieure. Audits read-only de chaque propriétaire contre les reçus exacts : fichiers inchangés, historique canonique d'exécution et ancêtre finalisé vérifiés. Pas de rejeu complet des votes/fork-choice. Aucune base ni clé privée transférée entre hôtes.

Hypothèse complémentaire isolée hors réseau : cache de vote limité à une cible par époque, effacement possible de préfixes lorsque deux cibles valides alternent. Les refus réseau génériques ne prouvent pas cette cause. Prototype et contre-exemple archivés séparément ; pas dans les binaires v11. Compilations diagnostiques natives sur Mac pendant v11 : les mesures de ressources sont ponctuelles, pas un benchmark CPU isolé. Aucun compilateur ajouté sur VPS pendant la campagne.

Cause WAN, parents distincts, pression/coût des caches, coût des publications, pruning/saturation, portabilité physique et finalité après restauration restent ouverts. Production et revue indépendante non acquises.

Traces terminales : 155 blocs Mac→VPS et 80 VPS→Mac accepted ; aucune attestation send-completion accepted observée. VPS : deux cycles transport-suspended/resumed, reprises observées après environ 2,327 s et 5,487 s ; un core-local-failure sur réception de bloc, sans cause détaillée prouvée. Boucles propriétaires saines à leur arrêt, propositions locales 34/39/32, attestations 302/304/39. Ces compteurs ne remplacent pas la finalité. Dernier relevé VPS : 34,2 Go d'écritures cumulées, pas occupation disque ; aucune conclusion causale.
