# SIGKILL du vrai Store Capella

Le test de chaînes signées lance désormais un processus enfant avec les
vrais snapshots Capella issus de l'admission des blocs. Il alterne une chaîne
de un bloc et une chaîne de neuf blocs via commit_execution_checkpoint :
retrait déjà payé sur la chaîne longue, non payé sur la courte. Le processus
est tué une fois dès son signal ready, puis une fois après observation d'un
commit acquitté. Chaque writer est borné à 128 commits ; aucun réseau utilisé.

Trois scénarios (continuation, migration exacte, migration avec slots sautés),
soit six SIGKILL/réouvertures. Après chacun : snapshot exactement court ou
long, token au moins aussi récent que tous les acquittements observés,
restauration authentifiée du Store, hash de tête et racine EVM concordants,
corps des retraits conservé et index blocs/transactions/reçus cohérents avec
la hauteur retenue. Les états mixtes et suffixes canoniques périmés échouent.

La suite enrichie conserve les contrôles de blocs, contextes, branches et
snapshots précédents. Elle passe en 3,95 secondes dans cette exécution locale.
Aucune modification de consensus ou du stockage n'a été nécessaire pour
faire passer cette nouvelle couverture.

Limites : SIGKILL signifie crash de processus, pas panne électrique réelle.
Le moment exact dans le WriteBatch/fsync n'est pas piloté ; une écriture non
acquittée peut être conservée ou perdue. Les tests de défaut sync restent une
campagne distincte. Ce résultat ne qualifie ni votes/finalité réseau Capella,
ni passage d'un Store Bellatrix vivant vers Capella, ni réacteur public.
