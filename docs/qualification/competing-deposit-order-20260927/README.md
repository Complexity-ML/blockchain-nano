# Dépôts concurrents dans des ordres différents

## Nouvelle couverture

Deux branches d'exécution Paris partent du même état et exécutent chacune deux
transactions signées vers le contrat de dépôt : A puis B et B puis A. Les
réserves exécutées augmentent de 64 ETH de test par branche. Les signatures BLS
des deux dépôts et leurs preuves de Merkle passent l'admission du registre.

La même clé A reçoit l'index 0 sur AB et 1 sur BA. Les deux historiques ont la
même cardinalité mais des racines et ancres différentes. Quatre preuves de la
branche sœur sont refusées sans modifier le curseur ni le registre ; quatre
rejeux de positions déjà consommées le sont également. Chaque historique
restauré reconstruit exactement son registre, tandis qu'une restauration sous
l'ancre de la branche sœur échoue. Les préfixes partagés restent inchangés.

Le test place ensuite les deux ascendances dans un même `CapellaForkStore`
synthétique, sans admission de blocs Beacon. Le sélecteur natif rend les quatre
opérations obligatoires avec les bonnes clés et tous les siblings Merkle.
Un vote de la branche sœur est refusé malgré une cardinalité identique. Une
racine sélectionnée étrangère dans l'état fourni est également refusée lors
de la recherche des opérations obligatoires.

## Résultat

`consensus_execution_deposit_history` passe en **0,39 s**, avec les deux nouveaux
scénarios et les régressions existantes de top-up, PoP invalide, snapshots,
préfixes authentifiés et pruning. Aucun changement du code de production.
Empreintes du code/binaire, sortie détaillée et JUnit joints.

## Périmètre restant

Les transactions EVM, BLS, preuves, historiques et admissions de registre sont
réels. Les enveloppes Beacon du test de sélection sont construites par le banc :
pas de fork choice, admission complète de blocs Beacon, propagation P2P ni
réorganisation de reactor avec les deux dépôts. La prochaine intégration doit
faire basculer un reactor entre ces ordres et vérifier la résolution des index
et les devoirs sans mélange des journaux. Ce test ne clôture pas ce scénario,
EIP-6110, le cycle validateur ou la production. Aucun fonds réel utilisé.
