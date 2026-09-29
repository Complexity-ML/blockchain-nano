# Raccordement réseau des sorties Capella

## Changement

Le service Capella accepte `voluntary_exit` dans les enveloppes hex et compactes,
puis appelle la file durable via le worker ordonné existant. Le RPC privé de
soumission/récupération des reçus emprunte ce même service. Identité complète,
112 octets SSZ et budgets d'entrée sont toujours contrôlés.

Le reactor expose inventaire, catalogue immuable et export borné des sorties.
Le catalogue n'annonce que les demandes applicables à la tête courante ; une
réorganisation les rend à nouveau visibles. L'export conserve les octets signés.
Le relais valide SHA-256 avant livraison. Le daemon P2P tourne désormais sur
six catégories de messages, avec un cache d'acquittements de sorties réinitialisé
au changement de session. Les budgets et le pacing restent inchangés.

## Preuves

macOS ARM64 Release : trois tests cœur passent en **14,92 s** :
service sorties 4,65 s, file durable 4,59 s, régression service BLS 5,66 s.
Le nouveau scénario vérifie mauvaises identités, tailles 111/113 rejetées,
signature invalide sans mutation, admission valide, reçu compact idempotent,
reprise du catalogue et budgets 31 octets de racines / 111 octets d'export refusés.
La suite du scénario conserve deux bascules de branche et réouvertures.

Le vrai daemon `nano-p2p` compile. Les **35 scénarios** du relais de catalogue
passent en 0,45 s, dont sorties à empreinte correcte/incorrecte et libération
du catalogue. Ces tests du relais simulent les échanges et acquittements.

```sh
cmake --build build-release --target nano-capella-transition-tests -j 2
ctest --test-dir build-release -R '^consensus_capella_(exit_wire|queued_exits|queued_bls_wire)$' -V
cmake --build build-network --target nano-capella-catalog-relay-tests nano-p2p -j 2
ctest --test-dir build-network -R '^capella_catalog_relay$' -V
```

## Limites et suite du jalon

Aucune campagne de propagation entre processus libp2p ni Mac/VPS n'est incluse.
L'appel au service et au gestionnaire des reçus n'est pas une requête HTTP réelle.
Le compteur historique `wireMessages` reste nul : il compte une autre variante
de livraison du banc, pas ces appels spécifiques au service des sorties.
L'impact de la rotation à six catégories sur les délais de finalité reste à
mesurer. Finalisation/retrait des fonds, pannes WAL, témoins et élagage restent
à qualifier. Aucun VPS mis à jour, aucun fonds, aucun jalon production clôturé.
