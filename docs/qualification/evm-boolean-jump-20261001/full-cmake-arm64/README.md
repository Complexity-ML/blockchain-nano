# Reconstruction CMake ARM64 — 98868b93

Sources 98868b93e73888129234d2233df80b254d94844b figées, patch vide. Construction CMake Release dans /build/evm-boolean-jump-20261001, image builder existante. 20 tests exactement déclarés et exécutés : 20 réussis / zéro échec / zéro ignoré, sans doublon ; exit CTest 0. Les onze empreintes exécutables avant/après sont identiques. Durée 684,82 s. Borne Docker 2 CPU / 3 GiB, réseau externe désactivé, source/racine en lecture seule, capabilities supprimées, no-new-privileges, scratch sur disque.

Périmètre : cœur/RPC/HTTP/admission/BN128 à froid/EEST local ; huit charges RPC Paris/Shanghai ; deux imports volumineux locaux ; deux synchronisations libp2p de 40 fragments ; deux fautes avec fragment et ACK perdus, puis reprise et audits. Les sources/tests précisent que cette synchronisation ne qualifie pas la proposition distribuée d’un bloc volumineux, ni le consensus Beacon WAN. Aucun fork réseau Cancun/Prague ou conformité EEST universelle revendiqué.

La campagne AMD64 de la même révision reste active avec au moins un échec de charge. Cette preuve n’est pas celle de la pile réutilisée 082143a2. Les précédents échecs et gates originaux restent ouverts.
