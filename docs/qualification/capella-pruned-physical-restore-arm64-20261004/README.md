# Restauration physique Capella élagué ARM64 — 94535766

Nouveau scénario sur la branche feature/finalized-retention-recovery,
runtime identique à 4734f285. Seul son programme de test compilé/linké,
contre le cœur qualifié existant ; aucune image reconstruite.
Compilation/link exit 0. Premier test exit 1 : manque de place dans le tmpfs
256 Mio au slot 8, en écrivant le WAL d’un témoin indépendant. Échec conservé.
Le même binaire est rejoué uniquement pour ce nouveau scénario avec TMPDIR
sur le volume disque existant : test exit 0. Aucune règle/budget de consensus
ou assertion de réussite modifiée. Script et logs des deux tentatives conservés.

Au slot 32, arrêt d’un propriétaire et des journaux : copie physique hors ligne
du stockage élagué et des 64 journaux. Témoins de consensus et de signature
restent indépendants hors sauvegarde. Hashes exacts sauvegarde/copie,
audit en lecture seule depuis le genesis avec rejeu des états élagués,
réouverture avec les engagements conservés et les témoins. Proposition
conflictuelle refusée avant callback signataire. Duties poursuivis, 64/64
journaux avancés sans régression ; finalité commune des deux propriétaires
2 → 7. Sauvegardes/source demeurent inchangées. Programme re-haché après rejeu.

Deux propriétaires locaux, clés synthétiques et injection directe du transport.
Aucun réseau WAN, coupure électrique, validation de fork-choice historique
exhaustive ou qualification de production complète. Le lot AMD64 4734f285
ne qualifie pas ce nouveau scénario 94535766. Pruning reste hors main/désactivé
par défaut ; reprise multi-hôte avec pruning, downgrade et ressources ouverts.
