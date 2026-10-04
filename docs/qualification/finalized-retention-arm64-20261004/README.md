# Pruning ARM64 4734f285 — 4 octobre 2026

Branche isolée feature/finalized-state-retention, base efbeb9bc, désactivée
par défaut. Compilation/link et CTest/contrôleur terminaux exit 0 : 7/7,
aucun échec/skip. Noms attendus et JUnit vérifiés indépendamment ; sources
réellement re-hachées après tests, quatre programmes re-hachés dans le volume,
manifestes stables. Binaires de livraison efbeb9bc laissés inchangés.

Deux propriétaires Capella : finalité epoch 7, six mouvements d'ancre,
réouverture des deux magasins et rejeu de 55 états élagués par propriétaire.
Ancre/frontière finale hauteur 56 ; genesis, ancre et head conservés.
Le scénario vérifie aussi le refus des ancres invalides et le RPC historique
State unavailable sans fallback vers latest. Le code exact reste sur sa branche.
Injection fsync : suffixe WAL conservé/perdu, checkpoint+marqueur+suppression
atomiques, reprise du commit suivant. Onze variantes de corruption refusées.
Régressions checkpoint, checkpoint sync, read-only et pruning Capella automatique.

`LastTest.log.gz` conserve les sorties complètes, dont certaines sont tronquées
par CTest dans JUnit. Octets originaux et hashes dans le manifeste.
Aucune coupure électrique physique, sauvegarde/restauration entière d'une base
élaguée, procédure de downgrade, qualification WAN du pruning ou mesure de
réduction physique des SST acquise. Archives signées toujours conservées.
Qualification AMD64 lancée séparément, encore en cours à cette observation.
Pruning non promu sur main ; ces sept tests ne remplacent pas les 427 tests
de la candidate ni ne qualifient la production complète.
