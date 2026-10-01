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

Contrôle syntaxique ARM64 terminal sur l'image existante
`sha256:6116669430b02b7e8012af8c8bd515a5a7dcd1dd2034649f9b45ae5b9e70a3a4`,
source 5fe1a43e, clang++ Release/NDEBUG : horloge, réacteur et test passent.
La première invocation du test omettait le chemin blst/bindings ; son erreur
est conservée. Seul ce fichier a été revérifié avec le chemin corrigé, exit 0.
Pas d'édition de liens ni d'exécution du scénario ; qualification toujours
ouverte. Source montée en lecture seule, aucune connexion réseau, 0,25 CPU,
1 GiB, aucun volume de build partagé ni image reconstruite.

Le banc de synchronisation imprime désormais un diagnostic borné à l'échec :
état de l'horloge, hash attendu et première erreur du réacteur. Il relance
la même exception ; aucun retry, succès forcé ou timeout modifié. Ce dernier
ajout doit encore être compilé et exécuté avant qualification.
