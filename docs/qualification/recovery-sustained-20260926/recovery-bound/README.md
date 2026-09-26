# Reprise liée aux deux projections de finalité

La condition de reprise du scénario de partition exige désormais, en plus
des têtes communes et du bloc finalisé d’exécution avancé, que les deux API
Beacon exposent le même checkpoint réalisé et que son bloc Beacon contienne
le hash d’exécution finalisé observé sur chaque nœud. Accept JSON est explicite.

Le délai de reprise reste cinq époques (240 s sur ce profil). Cette condition
ajoute une exigence au passage en surveillance ; elle ne change ni quorum,
ni consensus, ni seuils de surveillance. La surveillance conserve trois
avancées consécutives, l’absence de conflit/recul, les bornes de retard et
la correspondance des blocs d’exécution. Chaque observation cohérente est
conservée avant les assertions pour rendre les prochains échecs analysables.

Les neuf tests du moniteur passent. Compilation du pilote réussie. Le réseau
passe en **517,99 s** : partition de 96 s, reprise liée Beacon/exécution
en 95 s, puis trois avancées communes **6 → 7 → 8 → 9** en 145,101 s.
Les deux processus s’arrêtent proprement avec le même checkpoint finalisé 9. Les échecs précédents restent
archivés et ne sont pas reclassés en succès.

Cette preuve porte sur deux cœurs natifs et deux démons P2P, 64 clés
synthétiques réparties 32/32, une horloge réelle et un seul Mac. Elle valide
ce scénario borné après les corrections, pas un réseau multi-hôte, une longue
endurance, la capacité de rétention indéfinie ni l’ensemble des critères de
production. Les empreintes correspondent aux fichiers/binaries exécutés.
