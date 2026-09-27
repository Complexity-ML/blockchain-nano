# Votes et réorganisations Capella

Dans chacun des trois scénarios de chaîne signée, la branche concurrente
contient maintenant une transaction différente (transfert de 1 wei), donc
un état EVM distinct. Après avance de l'horloge et expiration du proposer
boost, six validateurs sélectionnés dans les vrais comités signent : un vote
pour la branche initiale, deux pour l'alternative, puis trois pour l'initiale.
La tête et l'état EVM doivent basculer ensemble dans les deux directions.

Résultats : 18 votes acceptés, 18 signatures altérées rejetées, six bascules
de tête/EVM. Les cibles sont dérivées de l'ascendance de chaque branche, y
compris lorsque la bifurcation est au premier slot d'époque. Le snapshot du
Store avec votes doit se restaurer exactement.

Une preuve de double vote du premier signataire est construite et signée
par scénario. Le Store doit inscrire ce validateur dans equivocating_indices,
puis conserver cet état au round-trip du snapshot. Cela qualifie l'exclusion
du poids dans le fork-choice, pas une pénalité économique appliquée dans un
bloc. Trois preuves positives sont couvertes ici.

La suite étendue passe avec les contrôles précédents (blocs, contextes,
snapshots et six SIGKILL). Aucun code de consensus n'a été changé pour faire
passer cette nouvelle couverture.

Limites : les votes sont fournis directement aux API du Store local. Aucune
preuve de finalité FFG multi-époque n'est ajoutée par ces bascules LMD-GHOST.
Réseau, propagation P2P, réacteur Capella, dépôts natifs et Store traversant
Bellatrix/Capella restent ouverts. Pas de lancement public.
