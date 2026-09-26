# Finalité multi-hôte et validateurs actifs — 26 septembre 2026

Révision du cœur : `b1ccbac9ea805afed67ac33fd7b5f180669aed22`.
Mac natif, Docker ARM64 Mac et Docker AMD64 VPS : trois instances sur deux
machines physiques, 64 clés synthétiques réparties 22/21/21. Aucun fonds réel.

## Résultat

Trois avancées consécutives communes 2 → 3 → 4 → 5, rattachées aux blocs
d’exécution, en **145763 ms** dans la fenêtre observée. Le démarrage jusqu’au
checkpoint 2 prend 178920 ms. Le lanceur émet `passed` après son contrôle des
trois statuts terminaux, et termine avec code 0.

| Instance | Propositions admises | Attestations admises | Maximum proposition | Maximum poll |
| --- | ---: | ---: | ---: | ---: |
| Mac natif | 16 | 136 | 153 ms | 462 ms |
| Docker Mac | 21 | 133 | 172 ms | 490 ms |
| Docker VPS | 18 | 128 | 685 ms | 1257 ms |

Les trois validateurs indiquent healthy=true et failure=null. Aucun poll ne
dépasse le slot de 6000 ms. Racine finalisée commune à l’arrêt :
`0x5e96f7cc095b2a2d3b22f5aff74683580e6a6d9f66bc1e13c932452269f9ee83`.

Le correctif d’expiration de proposition conserve la réservation anti-slashing ;
les contrôles locaux dédiés figurent dans ../proposal-expiry/. Cette campagne
n’injecte pas de retard de fournisseur et ne prouve pas à elle seule que ce
chemin précis a été déclenché. Le témoin with-batch reste en échec historique.
Les maxima VPS sont inférieurs au témoin sain with-phases (2010/2550 ms),
avec les mêmes comptes de propositions/attestations ; une seule fenêtre ne
constitue pas une mesure statistique ni une preuve causale de performance.

## Traçabilité et limites

Les empreintes des binaires sont dans run.jsonl. Images inspectées après arrêt :
- Mac : sha256:fc8f249a5ee1adefa594e6bc9fbeea2afe6c4950b11a5b3d631e27094e1d996f
- VPS : sha256:cee1c0c7b79bfb5b7eae230cb9b171ece137fadfa72cdf8f533184f4b4249629

Les deux conteneurs sont exited, et le PID natif 32861 absent lors du contrôle.
Les données de campagne restent conservées localement ; aucune clé ni base
n’est copiée dans ces preuves. SHA256SUMS protège les fichiers archivés.

Qualification bornée : ni partition WAN, ni endurance, ni montée en charge,
ni grand registre, ni activation réseau des forks EVM plus récents. Les traces
de relais sont filtrées (>500 ms ou échecs), pas une distribution de latence.
La reprise après partition WAN reste une prochaine qualification à réaliser.
