# Lecture Capella attachée à une racine d'état — 29 septembre 2026

Implémentée et vérifiée sur Mac ; nouvelle qualification Linux/WAN à réaliser.
Les deux échecs WAN précédents sont conservés.

L'API Capella accepte `head` ou une racine d'état de 32 octets dans
`/eth/v1/beacon/states/{state_id}/{root|fork|finality_checkpoints}`.
La recherche reste bornée aux blocs retenus du store publié sous verrou.
L'engagement SSZ est recalculé avant réponse. Une racine malformée renvoie 400,
un état inconnu/élagué 404 ; aucun repli sur le head. Le champ `finalized` suit
l'ascendance retenue du checkpoint finalisé. Les autres sélecteurs standard
(slot, justified, etc.) ne sont pas revendiqués par cette tranche.

Le moniteur WAN Capella lit le header puis sa `state_root` explicite. La paire
slot/finalité appartient ainsi au même état, même si le head change pendant les
appels. Les 404 restent fatals, de même que les racines invalides, conflits et
régressions. Le mode historique avec double lecture demeure pour Bellatrix.
Aucun délai, quorum ou règle de transition du consensus n'a changé.

Validation : `consensus_capella_beacon_reads` passe en 5,42 s. Les nouvelles
assertions couvrent root/fork/finalité, état ancien après évolution du head,
état élagué, malformation, absence, confusion block root/state root, corruption
SSZ et absence de mutation du checkpoint. Le scénario existant exerce aussi
les reorgs par votes. 24 tests JS du moniteur passent, dont cinq nouveaux pour
lecture immuable pendant churn, racine invalide, conflit, régression et 404.
L'HTTP relaie génériquement les chemins `/eth/` au handler ; l'intégration réseau
réelle n'est pas déduite des tests directs.

La première compilation a refusé une déclaration `auto` mixte ; elle est corrigée
et son journal conservé. Build natif final réussi. Sources/binaires identifiés
dans `manifest.json`. Ni conformité Beacon API complète ni production prête.

Référence : [Beacon API, finality checkpoints par state_id](https://github.com/ethereum/beacon-APIs/blob/master/apis/beacon/states/finality_checkpoints.yaml).
