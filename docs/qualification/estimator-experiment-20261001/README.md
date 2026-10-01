# Expérience estimation / pile GMP — non promue en production

Source de départ : 12ba92575647a4fa9ab611895514963062593534, même moteur que la candidate complète aacc56da. Chaque dossier conserve son patch, sa sélection, ses journaux, son résultat et les hashes des exécutables. Les phases sont distinctes ; aucune ne remplace un échec précédent.

- `baseline` : le nouveau contrôle profond échoue sur le moteur initial (Paris : 415 lectures de l'horloge pour eth_call, 5689 pour estimation). Shanghai n'est pas atteint dans cette phase.
- `fixed` : première allowance liée à la profondeur ; cœur passe, contrôle RPC échoue encore (2408 lectures).
- `tightened` : borne haute vérifiée plus proche ; cœur et RPC passent, 2013 lectures sur chaque profil. Le devis est réellement exécuté, signé et comparé à une frontière retrouvée séparément, avec tolérance 1,5 % inchangée.
- `load` : cœur et RPC passent (3483 et 1007 contrôles), mais les DEUX charges de 16 clients sur quatre nœuds échouent. Paris : 6 expirations de file + 2 d'exécution ; Shanghai : 7 de file + 2 d'exécution. Le marqueur est finalisé avec 16 clients encore actifs sur les deux profils. Ce témoin ne rend pas la campagne réussie. `bounded-summary.json` résume les requêtes sans remplacer le journal complet.
- `owned-stack` : nouvelle expérience avec normalisation GMP en place et transferts de propriété de pile. 3741 contrôles cœur, 1007 contrôles RPC, et fixtures enregistrées passent. 5387 cas d'exécution + 505 validations, 317 fichiers ; pas de preuve de conformité complète du fork (`forkConformanceProven=false`). Les nouveaux contrôles ciblent valeurs négatives, produits >256 bits, DUP indépendant, gas exact et frontière 1024/1025 de pile. La charge réseau n'a PAS encore été qualifiée sur cette dernière phase.

Les lectures d'horloge incluent préparation/checkpoints/retries ; ce compteur n'est ni du gas ni une mesure universelle de débit. Aucun changement des délais 5 s / file 4 s, de capacité, du gas ou du consensus. Profondeur observée = hint RPC, jamais engagement sérialisé. Le nombre de simulations ne garantit pas une durée sous contention CPU. L'exécution pleine candidate était aussi active sur le Mac pendant la phase `load` ; l'impact du partage hôte n'est pas isolé. Les échecs demeurent opposables.

Le job CI core 36908337763 est terminal et réussi : neuf tests, zéro échec/skipped sur bef03980. C'est le core CI, pas la régression complète 416, ni cette expérience. Le job réseau est encore à observer.

Aucun binaire public ou service VPS n'a été modifié. L'expérience reste dans sa branche séparée. Prochaine décision : qualifier sa dernière phase sous charge bornée dans un environnement identifié, et corriger séparément le bypass d'admission brut trouvé par `http` dans la régression complète.
