# Prélecture RPC bornée — 1 octobre 2026

Source candidate : `bf173958b7795b8ac07624399a0e14156d1ad555`, branche
`qualification/rpc-admission-20261001`. Aucun changement runtime dans main.

La prélecture ne descend plus dans les listes RLP et ne copie pas leurs
charges utiles. Elle inspecte au plus quatorze en-têtes de champs, extrait
le chainId/v (32 octets maximum), vérifie les bornes et refuse la mauvaise
chaîne avant le débit du budget. La conversion hex reste linéaire, limitée
à 1 Mio ; cette correction ne constitue pas une protection DoS générale.
Le décodeur canonique complet et ECDSA restent obligatoires pour la chaîne
locale après débit du budget. Aucun quota, délai, quorum ou gas modifié.

Le nouveau test échoue sur le précédent runtime à cause du parcours récursif
avant budget (`baseline/results.xml`, exit 8). Après correction : quatre
noms exacts, aucun skip/échec, 46.98 s (`fixed/results.xml`).
512 enveloppes étrangères rejetées, dont 128 imbriquées ; 63 signatures
locales high-s et 64 enveloppes locales imbriquées rejetées dans le budget.
Admission saine et refill vérifiés, état canonique inchangé.

Les fixtures legacy BFT Paris/Shanghai passent en 23.57/22.10 s : quatre
validateurs, partition 2/2 sans finalisation, votes réservés identiques
après restart, transaction saine finalisée sur chacun sous charge bornée.
Ce ne sont pas des tests du consensus Beacon Capella ou du WAN.

ARM64 : builder et volume existants, deux CPU, 3 Gio, racine/source readonly,
capabilities supprimées, scratch sur volume disque ; aucun nouvel image build.
Les empreintes exécutables et les journaux complets sont conservés.
AMD64 exécute encore la version précédente 9861d94c : aucun résultat
portable de cette nouvelle prélecture n'est revendiqué.

La campagne originale de 400 tests continue, ses exécutables et échecs
restent préservés. Estimation RPC sous charge, admission gossip/même chaîne,
matrice complète, cycle Beacon/WAN et les huit exigences originales restent
ouverts. Production non qualifiée ; aucun déploiement public ni fonds.
