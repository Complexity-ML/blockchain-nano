État courant : préparations/corruptions/reference restore ARM64 et AMD64 terminaux0 ; runtime32 blocs en cours78239/7934 ; code writer toujours isolé.

# Vérification des liens froids par en-têtes — 6 octobre 2026

Statut : prototype isolé, non appliqué au worktree ; source initiale ARM64 terminal0, variante combinée ARM64 en cours58003 ; AMD64 non commencé, lancement soumis à qualification du parent duplicate. Aucune production ou capacité de grand graphe qualifiée.

Le writer valide et authentifie toujours tous les records dans sa première passe (SSZ, exécution, métadonnées, receipts et dépôts). Sa seconde passe ne lit que les en-têtes déjà vérifiés pour vérifier parent.slot, parent.execution.number et parent.execution.hash : plus de leases simultanées parent/enfant qui rechargeaient les états évincés. Aucune modification de layout public, format ou confiance des références ; core modifié : uniquement consensus_capella_store_snapshot.cpp.

La fixture ajoute une restauration canonique avec cache d'un seul record. Le refus précédent de ce cache provenait des deux leases de la seconde passe ; le budget insuffisant est désormais exercé par maximum_row-1 octets. Les sept refus de manifeste/corruption, authentification avant fetch, annulation, canonical oracle, choix de tête et COW restent contrôlés. Sortie initiale ARM64 : controller0, hashes parent/programmes avant/après OK ; référence runtime froid. Pas de mesure de gain ou RSS.

Source-v2 combine les sources du correctif de doublons (déjà intégré localement, AMD64 en qualification) et ce seul changement du writer. Parent archive duplicate-fastpath ; fixture avec contrôles de corruption primaire/witness/tip en plus de reference restore et de cache1. Ce second run est requis pour qualifier leur composition ; il ne remplace pas les preuves de la source initiale.

Restent ouverts : sérialisation et authentification complètes des rows lors des publications nouvelles, restore preflight complet, GC/RSS, crashs froids, graphe1024, restauration des journaux historiques, finalité WAN et tous les critères production.

ARM64 combiné :58003 terminal0 pour le parcours cold-reactor/guards uniquement. Audit des logs : ce mode sort avant reference-writer/restore, donc le premier pilote ne couvrait pas ces deux gates malgré leurs variables présentes. Lacune conservée comme limite de portée ; réparation de pilote, même binaire sans compilation, exécute uniquement reference-links et termine0. Cache1 canonique, sept refus, annulation avant fetch, oracle/fork-choice et hashes parent/programmes inchangés. Pilote AMD64 corrigé avant tout lancement pour effectuer les deux modes séparés et exiger leurs événements de sortie. Sources combinées ARM64 qualifiées uniquement dans ces périmètres ; AMD64 encore non commencé.

AMD64 contrôleur40272 terminal0 : deux parcours séparés, réacteur/corruptions3 puis writer/reference restore/cache1/refus7. Hashes parent/programmes avant/après OK. Runtime32 blocs avec cache8 requis pour exercer les évictions et pruning sur cette variante ; ARM6478239 et AMD647934 actifs, mêmes programmes sans recompilation. Aucune qualification runtime de cette variante tant que ces deux sorties sont absentes.
