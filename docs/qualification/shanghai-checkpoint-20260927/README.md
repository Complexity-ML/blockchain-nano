# Checkpoint atomique avec retraits Shanghai

Le commit_execution_checkpoint liait déjà transactions, reçus, racines d'état,
hashes et métadonnées de checkpoint, mais ne contrôlait pas les retraits.
Il refuse maintenant une racine de retraits différente du corps, un corps
non-tableau, ou des retraits présents sans engagement dans le header.
Ce contrôle complète l'intégrité du candidat ; il ne remplace pas la
validation consensus des bénéficiaires/montants ni l'exécution EVM préalable.

Deux nouvelles cibles réutilisent le banc de stockage avec blocs Shanghai,
parent genesis Paris et retraits réels crédités par le moteur. Les tests
contrôlent le corps restauré, sa racine et le montant exact du solde, en plus
des index canoniques et du lien checkpoint/exécution.

Couverture Shanghai : réorganisation vers une branche plus courte, écritures
concurrentes CAS, checkpoint sans changement de tête, archives conservées,
quatre SIGKILL suivis de réouverture. La faute de synchronisation WAL vérifie
les deux issues : suffixe conservé ou perdu, avec invalidation de l'instance
incertaine. Cela simule une erreur disque, pas une panne électrique physique.
Un corps de retraits altéré est refusé spécifiquement par le nouveau contrôle,
sans interférence de l'archive. Le même contrôle refuse des retraits sur Paris.

Régression stockage Paris + Shanghai, normale et faute sync : 4/4 suites.
Résultats détaillés dans result.json et ctest-detail.log ; aucun rejeu réseau.

Le snapshot du banc reste synthétique. Le Store Capella, le profil de fork
persisté et le raccordement du réacteur ne sont pas encore implémentés par
cette modification. Il ne s'agit donc pas d'une preuve de reprise complète
d'un nœud Capella ni d'une activation réseau.
