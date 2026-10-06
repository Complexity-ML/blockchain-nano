# Prototype isolé : replays Capella idempotents — 2026-10-06

Statut : ARM64 seulement, sources isolées non appliquées au core du worktree/main. Parent core : cold-runtime ARM64, hash dans parent-before.sha256 ; en qualification AMD64 séparée. Aucun déploiement/image reconstruite ni critère production clos.

Chemin rapide pour un root déjà authentifié : contrôles execution tip, token primaire et token witness conservés ; aucune nouvelle publication ni remplacement d'archive. Ce chemin accepte comme avant une signature altérée portant sur un root déjà admis, sans écraser les bytes signés enregistrés. Les nouveaux roots suivent leurs validations originales.

Fixture : annulation sans mutation ; sept séries de32 replays alternant wire original et signature altérée, publication/shared-state/archive/journal inchangés. Trois mutations via writers locaux de stockage : token primaire, token witness et suffixe d'exécution ; refus puis réacteur indisponible. Première fixture échouée car elle utilisait le writer standalone interdit pour un checkpoint couplé ; ce contrôle est conservé, injection corrigée avec le writer atomique autorisé. Core identique pendant réparation de fixture.

Contrôleur70905 terminal1 (fixture),22613 terminal0 (réparée),89552 terminal0 (trois paires). Même objet de fixture lié successivement aux archives reference/candidate, hashes parents/candidate/reference vérifiés avant/après. Pour32 replays sur deux records : médianes reference97913/96401/97591µs ; candidate3589/3705/3607µs. Callbacks checkpoints1158304→5440 par série. Mesure locale bornée, pas un gain WAN, RSS ou graphe1024 ; aucune conclusion sur les publications de nouveaux votes/blocks. Source-v2 ne change que la fixture.

À faire : qualification AMD64 du prototype après campagne en cours, comparaison au commit livré du parent, intégration sur main, nœud/restauration réelle. Coût de la revalidation complète des publications nouvelles, capacité du graphe et finalité V28 restent ouverts.
