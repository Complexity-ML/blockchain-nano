# Passage original terminal — 1 octobre 2026

Exec 32017 terminal exit 8, container retiré après fin, 400 noms exacts
comparés au registre : 314 réussites, 86 échecs, aucun skip. Rapports complets,
source/patch/helper RocksDB, registre, empreintes et tous échecs conservés.
Runtime original nano bbe09460… / nano-p2p ae567df0… inchangé vérifié sur
volume readonly après fin. Aucune réussite globale ni conformité universelle.

46 échecs portent une trace directe No space left on device ; ils
sont listés séparément. Le scratch original /tmp était tmpfs 128 Mio/noexec.
Les autres échecs ne sont pas attribués automatiquement au disque : notamment
RPC paramètres/admission (correctifs ciblés), estimation sous charge toujours
ouverte, transport/consensus à réexaminer et toutes autres assertions.
Replays ciblés antérieurs conservés dans dossiers respectifs ; ils ne rendent
pas cette campagne verte. Les 22 absences par rapport au registre natif ont
été étudiées : 13 registrations de fault fixtures corrigées, outils/corpus
optionnels distincts. Nouveau candidat complet encore non qualifié.

Après cette fin, merge 23204db2 intègre le candidat aacc56da : runtime
ff53e4d1 qualifié sur 7 ARM64/11 AMD64 pertinents ; scripts/CLI/faults/dispatch
portent leurs preuves spécifiques. Les différences main/candidat sur les
sources C++/CMake/tests/scripts/contracts/réseau sont vides ; docs diffèrent.
Aucun déploiement public ni image neuve. Nouveau passage prévu avec scratch
volume disque et registre complet via lanceur Docker. Huit critères initiaux
restent ouverts, production non qualifiée.
