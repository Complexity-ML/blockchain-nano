# Lanceur complet Docker utilisable — 1 octobre 2026

Candidate outils aacc56da ; runtime ff53e4d1 inchangé. full-docker.sh emploie
le builder existant et volume existant, deux CPU/3 Gio, readonly source/root,
capabilities supprimées, aucun réseau externe ni installation/image build.
Compilation et CTest dans Docker ; inventaire Node/audit XML Python sur hôte.
Le constat précédent d'absence Node/Python dans les builders est résolu pour
ce parcours, sans prétendre qu'ils y ont été installés.

Préparation : checkout propre, source manifest, scratch disque exécutable,
provenance, tous les targets, registre complet et empreintes CMake de chaque
exécutable. Audit hôte refuse missing binaries/timeouts/skips avant tests.
Avant tests : registre et hashes identiques à la préparation ; après tests :
résultats exacts et source hashes/révision inchangés. Délais CTest originaux.

Contrôles possédés avec réel Docker/CMake/CTest : true réussi, false refusé
(CTest exit 8 et journal conservés), absence TIMEOUT refusée avant tests.
Le contrôle positif répété après ajout de la garde source confirme les trois
source manifests identiques et les deux manifests exécutables identiques.
Ces fixtures ne qualifient pas Nano. Sept tests Node des gardes passent.

246 contrôles opérations réussis sans skip, dont sept nouveaux ; essai sandbox
initial refusé sur listen loopback (EPERM) conservé, exécution autorisée réussie.
Aucun serveur public ni argent. Campagne originale de 400 toujours active ;
un passage complet Nano sur ce lanceur ne sera lancé qu'après sa fin réelle.
Le registre configuré reste distinct des corpus/outils optionnels absents et
ne prouve pas une conformité universelle Ethereum ou une production prête.

Exemple depuis checkout propre, après fin du banc précédent :
NANO_BUILDER_IMAGE=sha256:6116669430b02b7e8012af8c8bd515a5a7dcd1dd2034649f9b45ae5b9e70a3a4
NANO_BUILD_VOLUME=nano-linux-sync-build
NANO_BUILD_SUBDIR=rpc-positional-client-20261001
bash scripts/ci/full-docker.sh /tmp/nano-full-candidate-20261001
Les variables doivent être exportées ou préfixer la commande. Aucun lancement
Nano complet exécuté dans ce dossier de preuves ; huit critères restent ouverts.
