# Refus et diagnostic des reculs d’horloge pendant la signature sync

Source `2e16522`. Trois nouveaux scénarios par architecture passent avec le vrai reactor, RocksDB et les journaux BLS : recul à l’entrée, avant le fournisseur, et après son retour. Aucun message publié ; token du reactor et engagement du journal inchangés. Le fournisseur est appelé seulement dans le troisième cas. Le diagnostic conserve le préfixe et indique nowMs, lastMs, durableMs, genesisMs, slot, phase, numéro de lecture et prédicats. Aucun seuil ou refus de signature modifié.

La suite locale `--local-sync` termine exit 0 dans Docker Mac ARM64 et Docker VPS amd64 : 40 blocs, 272 votes, finalité 3, deux stores rouverts et 64 journaux rouverts. Les mêmes cas sont suivis de signatures sync réussies dans le même fixture. Volumes et images de dépendances existants réutilisés, source VPS obtenue par Git.

Ce résultat qualifie les diagnostics et le refus local, sans transport réseau. La cause de WAN G reste inconnue : son runtime antérieur ne fournissait pas les valeurs. Aucun correctif causal, gain de reprise ou qualification production revendiqué. Les images candidates et le binaire natif restent antérieurs à cette modification ; leur actualisation est requise avant un nouvel essai WAN. Les échecs historiques et tous les autres critères restent ouverts.
