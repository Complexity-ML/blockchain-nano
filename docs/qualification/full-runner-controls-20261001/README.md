# Lanceur complet — contrôles, pas qualification Nano

scripts/ci/full.sh construit tous les targets et sélectionne tout le registre
CTest configuré. Inventaire require-complete exige chaque exécutable et
TIMEOUT explicite, aucun mécanisme de skip. TMPDIR sur disque de build avec
sonde d'exécution, provenance/hashes avant tests, journal complet conservé.
verify-results.sh contrôle noms exacts, doublons, statut, skips et compteurs.
Quatre tests Node passent : succès, manque/duplication/étranger, skip/disabled,
vrai échec et XML incohérent. Contrôle bout en bout avec vrai CMake/CTest :
fixture true exit 0 et fixture false refusée (CTest exit 8 conservé,
lanceur exit 1). Ce sont des contrôles du lanceur, pas des tests de Nano.

Prérequis explicites : Git, CMake/CTest, Node et Python 3.11+. Les builders
Docker actuels n'ont ni Node ni Python ; ils doivent être équipés ou le
lanceur adapté avant son usage Docker. Vérification préalable échoue avant
compilation si outil absent. Aucun nouveau full run lancé tant que l'ancien
est actif. La sélection couvre le registre configuré, pas un corpus EEST
universel ni les outils optionnels absents. Candidat isolé, pas main.
