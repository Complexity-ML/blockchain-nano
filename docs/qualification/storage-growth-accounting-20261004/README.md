# Comptabilité RocksDB hors ligne — 4 octobre 2026

**Exit 0**, outil C++ autonome compilé pour Mac ARM64, RocksDB 11.8.1 selon
le rapport, sans relinker les programmes Nano qualifiés. Lecture des 46 bases
Nano dans les fixtures arrêtées originale et migrée. Sandbox macOS : réseau et
toute écriture de fichier interdits. Les 331 fichiers originaux et 461 fichiers
migrés sont identiques avant/après ; les 603 entrées natives précédemment
qualifiées sont relues et inchangées. Aucun pruning ni compactage effectué.

| Mesure | Original | Copie migrée |
|---|---:|---:|
| Octets logiques clés + valeurs | 2 686 457 | 2 686 457 |
| Taille apparente des fichiers de base | 51 403 853 | 5 290 934 |
| États EVM : 54 entrées, clés + valeurs | 60 985 | 60 985 |
| Checkpoint vivant, clés + valeurs | 2 231 548 | 2 231 548 |
| Archive signée, clés + valeurs | 290 441 | 290 441 |

Toutes les catégories/tailles/nombres sont identiques dans les deux fixtures,
pas seulement leur total. Les octets logiques ne sont pas la taille comprimée,
ni l'espace alloué physique des fichiers, ni un gain causal contrôlé. Cette
petite fixture ne mesure pas la croissance d'un réseau chargé.

Le code conserve automatiquement le fork store Capella autour de la finalité
(`prune_if_ready_locked`), mais conserve `state/<height>` pour chaque bloc
canonique et les archives signées. `state_at` alimente RPC et audit ; l'audit
Capella vérifie également ces états. Les supprimer isolément casserait ces
contrats. Le chantier restant est une politique de rétention couplée au
checkpoint finalisé, avec ancre de rejeu, états non finalisés conservés, annonce
RPC de disponibilité historique et validation de reprise/rollback.

Contrôles négatifs : répertoire vide et racine symlink refusés exit 1. Une
première vérification utilisait `/tmp`, qui est lui-même un symlink macOS :
le nouveau refus était correct mais l'assertion attendait un autre message.
Ce lanceur a échoué exit 1 ; son résultat périmé n'est pas une réussite et le
refus est conservé dans `negative-control-failure/`. Le lanceur terminal utilise
un répertoire vide réel dans une nouvelle destination.

Limites : comptabilité en lecture seule et intégrité des fixtures ; aucun
allègement nouvellement implémenté, aucune qualification Docker/Linux du helper,
ni borne de croissance, temps de reprise ou production démontrés.
