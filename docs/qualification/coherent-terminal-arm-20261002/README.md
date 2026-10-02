# Deux qualifications ARM64 terminales, échec AMD64 conservé

## Régression complète 3030b799 ARM64

**422/422 passent, zéro skip, 6 088,58 s**. Registry/résultats exacts et audit terminal passent. Source/programmes inchangés ; les trois helpers externes nano, nano-finality-crash-fixture et nano-altair-probe sont observés avant/après et inchangés. Observation auxiliaire conservée car le runner gelé ne couvrait pas encore tous ces helpers dans son manifeste. Environnement Docker existant, deux CPU/3 GiB, scratch Linux, délais déclarés du registre conservés. Pas de bibliothèque exhaustive avant/après revendiquée pour cette campagne : contrôle des programmes et sources, image fixe, compilation préalable.

Tous les cas de ce registre passent, dont les anciens échecs RPC/OOG/Altair P2P et pruning Bellatrix ainsi que les scénarios Capella. Le registre de cette révision est la limite de cette conclusion : ni toutes les futures EIP ni les tests Node absents/non enregistrés dans cet environnement. EEST consommé dans la régression : **5 387 appariements (4 882 exécution / 505 validation), 317 fichiers**, forkConformanceProven=false. Cela ne réactive pas Cancun/Prague dans le genesis. Pas de WAN dans cette campagne.

## Sélection cohérente 2d2764de ARM64

**15/15 passent, zéro skip, 487,22 s**, limites d’origine. Inclut dix cas Capella/canonical checkpoint et core/rotation/helper hex SSZ/ancestry/hex différentiel. Build CMake cohérent du cœur + sept cibles puis ajout de la cible checkpoint standalone ; huit programmes et sept entrées statiques de lien inchangés avant/après, source inchangée. Tests uniquement sur cette sélection, pas 425/425 du registre de cette configuration. Liveness Capella des scénarios synthétiques/deux états en processus ; pas de preuve WAN ni d’audit cryptographique complet apporté par le helper ancestry à graphe synthétique. Rotation self-test n’est pas la campagne de rotation multi-hôte.

Activation 85,77 s, restart 86,38 s, auto-registration 87,36 s ; activation réindexée **102,36 s / 360 s**. Les différences de temps avec un autre run ne constituent pas un benchmark contrôlé bout en bout.

## Échec AMD64 12ad9eb5

Sélection terminale **2/3**, zéro skip, **réindexation timeout 360,03 s**. Dernier marqueur slot 152 / finalité 17 / six nouveaux votes ; qualification finale de l’activation non atteinte. Helpers ancestry et hex SSZ passent. Sources/quatre programmes/sept entrées statiques inchangés avant/après. Cette correction de réutilisation SSZ n’a donc pas fermé le délai ; échec conservé.

## Suite

Après terme et archivage de 12ad9eb5, build AMD64 du candidat **2d2764de** démarré dans /build/coherent-2d2764de-amd64-20261002, image/volume existants, deux CPU/3 GiB, huit cibles, aucune modification des artefacts précédents. Le candidat inclut l’allocation hex bornée précédemment mesurée ; son intégration AMD64 et le respect du délai restent à démontrer. Journal : /home/ubuntu/nano-coherent-2d2764de-amd64-build-reports-20261002/build.log. Main runtime toujours inchangé. Régression complète sur le candidat final/des deux architectures, restauration et finalité WAN, partitions/rotation/cycle/sanctions, anomalie WAN A, profil EVM réellement activé, opérations, bridge/Doppler et revue indépendante restent ouverts.
