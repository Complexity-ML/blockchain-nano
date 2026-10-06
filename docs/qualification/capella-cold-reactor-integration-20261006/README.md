# Capella : réacteur, publication froide et récupération — 2026-10-06

## Changement

Le réacteur possède désormais les providers gardés du stockage primaire et du témoin. Sa destruction ferme leurs lecteurs avant que le propriétaire du stockage soit détruit. Les snapshots publiés survivants peuvent conserver une lease déjà décodée ; une nouvelle lecture non mise en cache est refusée après fermeture.

Migration explicite `migrate_reference_checkpoint({cache_records, cache_bytes})` vers `nano-capella-cold-checkpoint-v1`, références validées avant I/O, blobs et checkpoint dans les batches synchrones existants. Le format reste froid pour les publications suivantes. Les enveloppes de votes, slashings, changements BLS et sorties conservent leurs validateurs ; leur contenu non vide sous ce nouveau format n'est pas encore qualifié. Les options locales sont engagées dans le checkpoint.

Configuration ajoutée : `coldCheckpoint: {cacheRecords: "0x2", cacheEncodedBytes: "0x..."}`. Les quantités doivent être canoniques, positives et respecter les budgets locaux. Le bootstrap avec cette configuration activée n'est pas encore qualifié ; les tests configured-runtime/automatic-recovery de ce lot utilisent leurs configurations legacy.

## Qualification et portée

Docker ARM64 et AMD64, images et volumes existants, aucun build d'image, aucun nœud public activé. Changement de layout du réacteur : fermeture des 16 unités core dépendantes détectées dans les fichiers .d, plus fixture recompilée. Parent : archive libnano_core-v2.a du lot a0153612 (provider-lifetime). Ce sont des builds incrémentaux, pas une release propre.

Scénario nouveau : annulation avant écriture avec token/journal inchangés ; migration explicite et idempotence ; refus de downgrade fragment ; récupération d'un PRIMARY indépendant précédent depuis le WITNESS engagé ; admission d'un vrai bloc signé ; graphe publié entièrement froid ; conservation du format ; lecture différée refusée après destruction ; reprise PRIMARY-next/WITNESS et primaire seul ; archive signée et graphe comparés au témoin legacy.

Journal de fixture créé vide : sa conservation ne prouve pas le cycle complet d'un journal historique anti-double-signature. `migrationAtomic` couvre ici l'annulation pré-I/O et le raccordement aux batches existants ; aucun nouveau SIGKILL ou défaut WAL injecté dans le réacteur froid.

Régressions concernées : owner-witness, configured-runtime, automatic-recovery. Hashes parent et programmes vérifiés après exécution.

## Échecs conservés

1. Source initiale : comparaison string/JSON invalide dans cold_options, compilation du réacteur refusée sur les deux architectures ; 15 autres unités réussies conservées.
2. source-v2 : réacteur compile, fixture refusée (return 0 dans fonction void). Le pilote avait laissé errexit désactivé : ses mentions passed, codes de tests 0 et controller 0 sont INVALIDES ; aucun exécutable ni test exécuté. Logs et INVALID-QUALIFICATION.txt conservés.
3. source-v3 : assertions du scénario imprimées, puis attente futex après destruction prématurée du propriétaire avant le réacteur emprunteur. SIGTERM ciblé après diagnostic, sorties terminales 143. Aucun succès global revendiqué.
4. source-v4 : destruction explicite du réacteur avant le propriétaire. Seule fixture recompilée ; objets core identiques à source-v2. Sources figées et différences archivées.

## Ouvert

Limite graph.blocks=1024, V28 FAILED et finalité WAN commune ; démarrage runtime coldCheckpoint activé ; preuves non vides et limites/corruption de l'enveloppe froide ; crashs en cours de batch ; pruning froid (comparaison legacy encore plate), garbage collection blobs, budget RSS global et pression multi-snapshots. La récupération PRIMARY-previous matérialise encore les rows avant staging : pic mémoire non clos. Staging limité au budget de batch ; ce n'est pas une migration arbitrairement grande.

Aucun gain RSS, grande capacité, fork EVM supplémentaire, bridge, Doppler, production ou audit indépendant qualifié. Les autres critères des quatre feuilles de route restent intégralement ouverts selon leurs preuves antérieures.

## Résultats terminaux

ARM64 contrôleur 5323 / sortie 0 ; AMD64 contrôleur 56924 / sortie 0. Quatre modes par architecture, tous terminaux 0. Initials core ARM78923/AMD34969 sortent 1 ; v2 ARM99762/AMD35935 sortent 0 trompeur, explicitement invalidé ; v3 ARM35492/AMD26065 sortent 143, teardown corrigé. Parent a01536120a50137a54a48052124982c946cfc24f. Manifeste source-v4 : 1615 fichiers comparés au worktree, zéro écart ; seules modifications de source-v1 à v2 : réacteur ; de v2 à v3/v4 : fixture.
