# Trace bornée des choix locaux

Diagnostic uniquement, désactivé par défaut. `NANO_TEST_NETWORK_TRACE=1`
active 128 observations (au plus une par slot avec vote local signé).
Chaque observation associe la tête/source/cible effectivement signées au
checkpoint et aux derniers messages connus immédiatement après signature,
avant admission locale. Le snapshot peut déjà avoir avancé : il ne constitue
pas une capture atomique de la décision de signature. Sa séquence et son temps
sont conservés pour rendre cette limite explicite.

La trace ne contient ni clés privées ni jetons d'authentification. Elle reste
en mémoire et n'est exportée qu'à l'arrêt du runtime ; les réponses de statut
courantes ne recopient pas cet historique. Les erreurs de diagnostic sont
comptées sans faire échouer la validation. La limite borne des observations,
pas des validateurs. La taille de chacune dépend du registre connu.

Le test dédié vérifie l'éviction, la désactivation, la correspondance avec
des attestations admises et l'accord des deux cœurs. Test réussi en 8,56 s. Le scénario réseau instrumenté échoue en 522,42 s :
saut d’époque finalisée dans la fenêtre surveillée.
Cette instrumentation ne prétend pas résoudre le retard de finalité.

## Résultat exploité

Aucune erreur de trace. Entre les slots 61 et 67, les derniers messages
distants restent presque figés, alors que les attestations locales continuent.
Au slot 64 les nœuds choisissent des cibles différentes ; le source rejoint
la cible de l’observateur au slot 69. Les deux cibles sont identiques à partir
du slot 72 dans les observations conservées. Ce relevé ne démontre pas à lui
seul une faute de consensus ; il oriente vers la fraîcheur des votes reçus.

Le relais sautait déjà les votes acquittés sans les retransmettre, mais chaque
saut consommait un intervalle du planificateur. Un catalogue réinitialisé après
élagage pouvait donc retarder les votes récents derrière un long préfixe connu.
La correction en cours poursuit ce parcours dans une tentative asynchrone
bornée, avec échéance et budget de travail ; les votes inconnus conservent
leur exigence d’acquittement. Il reste à qualifier son effet réseau.

L’outil C++ `--duty-report` fonctionne sur des copies de bases arrêtées dont
le checkpoint est authentifié par le reçu externe. Il compte les messages
par âge et relation d’ascendance avec la tête signée. Il ne reconstitue pas
une décision atomique pondérée ; les observations sont postérieures à la
signature. Les anciens reçus sans trace sont refusés.
