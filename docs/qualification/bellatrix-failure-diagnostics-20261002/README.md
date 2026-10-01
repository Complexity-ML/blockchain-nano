# Bellatrix : conservation du premier échec — qualification en attente

Le test ARM64 c10426b2 `consensus_bellatrix_voluntary_exit_pruning` échoue
avec un message de fermeture de l'exécution. La campagne initiale et son
résultat ne sont pas modifiés. Le saut d'une synchronisation intermédiaire
est une hypothèse à confirmer ; aucun assouplissement d'horloge n'est proposé.

Cette branche, issue du correctif de durée RPC, conserve l'exception initiale
lors de l'invalidation de Bellatrix et des échecs de publication protégés,
comme le fait déjà Capella. Le diagnostic d'horloge détaille temps courant,
durable, observé et limite. Une seconde invalidation préserve la première
cause. Les bornes, transitions, formats persistés et refus d'accès à l'état
restent inchangés. Le test d'horloge existant vérifie la cause conservée et
l'accès à l'exécution toujours fermé.

La compilation et l'exécution de ces contrôles restent à qualifier. Ni cette
préparation ni une vérification syntaxique ne résolvent l'échec d'élagage.
Rejouer après les campagnes figées, avec ces diagnostics, avant correction.
