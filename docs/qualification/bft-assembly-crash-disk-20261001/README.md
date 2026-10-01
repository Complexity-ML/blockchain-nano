# Reprise d’assemblage sur disque — 1 octobre 2026

Exécutables originaux inchangés : nano bbe09460…, fixture b40233da…,
nano-p2p ae567df0…. Aucun build/source modifié ; journal CTest disjoint
pour ne pas écraser celui de la campagne originale toujours active.
Builder existant, deux CPU/3 Gio, réseau externe coupé, source readonly.
TMPDIR sur volume disque, timeout 240 s inchangé pour chacun.

Paris passe en 30.71 s ; Shanghai en 27.34 s. Six frontières SIGKILL :
1/20/40 fragments par profil, audit pré-commit inchangé, aucune proposition
ou vote fantôme, ancienne session/replay refusés, redownload authentifié
puis quorum, dix reçus réussis et exactement dix transferts, seconde
réouverture avec certificat et audit. Sélection exacte, no skip, exit 0.

Les deux échecs originaux sont conservés : l’échec au quorum signé ne
contient pas à lui seul une trace ENOSPC, donc aucune cause unique n’est
affirmée. Ce contraste établit une réussite sur scratch disque, pas une
qualification générale du stockage ou une panne physique. Le premier
rejeu de diagnostic a échoué avant exécution car le montage /work était
absent ; preuves conservées dans missing-fixture-mount. Seul ce montage
readonly a été ajouté pour le second rejeu.

Fixtures legacy BFT locales, aucune intégration libp2p dans ces six frontières.
Beacon/WAN, crash d’E/S physique, ressource générale et huit exigences
originales demeurent ouverts. Production non qualifiée.
