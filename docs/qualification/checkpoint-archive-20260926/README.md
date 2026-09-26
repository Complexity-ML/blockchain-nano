# Archive atomique de checkpoint — 2026-09-26

Primitive `ChainStore` ajoutée : les enregistrements opaques validés par l’appelant sont persistés dans le même WriteBatch synchronisé que le checkpoint et le suffixe d’exécution. Identifiant immuable, index par slot/racine, checksum de contenu, budgets de lecture/écriture et refus de collision. Une réorganisation ne supprime pas ces archives. Le checksum est une détection de corruption, pas une preuve de validité consensus.

Deux tests passent en 1,49 s : `storage_execution_checkpoint` et `storage_execution_checkpoint_sync`. Couverture nouvelle : conservation de l’historique après réorganisation, doublon identique, conflit avec l’existant et dans un batch, limites, quatre SIGKILL/réouvertures et cohérence archive/checkpoint après conservation ou perte du suffixe WAL non synchronisé. Treize refus contrôlés ; CAS concurrent conservé. Les charges archivées de ce test sont synthétiques, pas des blocs Beacon signés.

La première compilation a échoué sur une comparaison string/JSON ; accès explicitement typé corrigé avant le build et les tests archivés. `git diff --check` passe.

Non raccordé au runtime Bellatrix ni au P2P à ce stade. Aucun état n’est élagué. Restent l’écriture des vrais blocs signés depuis le coordinateur, le service de l’historique, la migration durable et la politique de rétention. Pas de preuve de panne électrique ou de production globale.

## Raccordement au coordinateur

Les nouveaux blocs signés admis par Bellatrix sont maintenant écrits via la même primitive, dans les chemins ChainStore direct et Blockchain. Le scénario signé passe en 3,24 s : neuf archives retrouvées exactement avant et après réouverture. Les anciens snapshots ne sont pas migrés, le P2P lit encore les archives du snapshot et aucun état n’est élagué. Cinq contrôles supplémentaires avant publication passent en 4,29 s (conteneurs Capella, horloge et lectures/en-têtes Beacon).
