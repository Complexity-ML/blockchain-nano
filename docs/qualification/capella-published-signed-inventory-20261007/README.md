# Inventaires signés publiés

Les inventaires durables d’attestations et de preuves de slashing sont publiés atomiquement avec leur état/checkpoint. Les captures ne prennent plus le verrou d’admission ; sync, sorties et changements BLS conservent leur verrou. Les snapshots ne retiennent que les racines. Construction avant commit, bornes et annulation conservées ; aucune règle de signature ni format disque modifié.

Sur chaque architecture : contre-exemple ancien moteur attendu1, puis six parcours ciblés0. Les trois états vérifiés sont vote admis, slashing non vide et réouverture durable. Les dépendances compilateur de139 fichiers core identifient16 consommateurs ABI recompilés ; nano/test/capture sont recompilés et reliés avec la nouvelle archive, parents inchangés. HTTP/RPC passe séparément sur les trois environnements. Images Docker existantes réutilisées ; pas de build image ni de release propre qualifiée.

Refus AMD64 initial conservé : audit en lecture seule encore présent avant toute compilation. Le garde reste inchangé ; le parcours a commencé après sa fin. Les erreurs historiques V35 restent FAILED. Ces tests ne prouvent ni la cause globale du blocage réseau ni l’avancement d’une finalité commune. Production, Base–Nano/Doppler et revue indépendante restent ouverts.
