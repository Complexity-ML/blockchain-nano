# Encodage hex à allocation bornée — candidat 669ee563

Parent 12ad9eb5, branche qualification/hex-allocation-20261002. hex() alloue sa sortie une seule fois puis écrit les deux caractères de chaque octet. Format canonique 0x/minuscules, entrée vide et tous les octets conservés. Dépassement de taille contrôlé avant calcul 2*n+2 ; ce cas théorique ne peut pas être exercé avec un vecteur matériellement alloué et n’est pas revendiqué couvert.

## Mesure directe dans les images Docker existantes

ARM64 61166694… et AMD64 0fa8a608… : source types.cpp réelle compilée O3 avec le nouveau test, contre GMP/OpenSSL de ces images fixes. Un CPU, 2 GiB, aucun réseau, root/source readonly, scratch exécutant borné. Aucun rebuild d’image. Référence legacy reproduisant le corps exact précédent, pas appel à un second client. Le compteur global new du seul programme de test instrumente les allocations ; il ne fait pas partie du nœud. Cinq fenêtres alternant l’ordre de mesure ; pas de garantie de performance hors de ce microbenchmark.

**1 031 comparaisons différentielles par architecture passent**, avec décodage inverse, 256 octets et frontières SSO, puis tailles 32 à 1 048 576. Pour les tailles mesurées : une allocation ; référence 3/10/12/14/18. À 1 MiB, ratio médian temps référence/candidat **6,37 ARM64**, **3,79 AMD64**. Ce n’est pas un gain de 6x ou 3,8x du nœud, de la finalité ou de la réindexation. Résultats bruts et autres tailles conservés, aucun pourcentage de profil imbriqué additionné.

Sources ARM64 avant/après identiques et programmes avant/après identiques des deux côtés. AMD64 : checkout par Git au commit exact, readonly, statut clean vérifié après la mesure ; pas de manifeste source complet pré-exécution AMD64, donc pas de revendication de cette preuve manquante.

## Intégration core ARM64

Le test core existant recompilé O3 avec le nouveau types.o et les bibliothèques 3030b799 readonly : **4 069 vérifications passent**, délai original 120 s. Couvre notamment vecteurs de hashes/signatures/transactions, EVM et persistance tels que définis dans tests/test_core.cpp ; pas tous les RPC ni tous les forks. Programme/sept entrées statiques identiques avant/après. C’est un lien direct partiel, pas une compilation CMake cohérente complète et pas une intégration AMD64 de ce candidat.

## Candidat cohérent suivant

**2d2764de**, branche qualification/production-hex-candidate-20261002, réunit 669ee563 et main 6af1db8d (contrôleurs WAN à inventaire complet). Code natif src/include/network/third_party et les deux tests natifs modifiés strictement identiques à 669ee563. **15 022 empreintes sources identiques Mac/VPS**, checkout distant par Git. Le cas d’inventaire réel passe sur cet ensemble ; aucun autre test Node inchangé rejoué. Aucun binaire de 2d2764de compilé ou qualifié à ce stade. Ne pas promouvoir sur la base de cette seule égalité des sources.

La compilation et le test réindexé 12ad9eb5, ainsi que la régression ARM64 3030b799, continuent sur leurs sources gelées. Le coût d’activation à 360 s reste ouvert. Régression pertinente finale, intégration des deux architectures, restauration WAN/finalité, autres critères de production, bridge/Doppler et revue indépendante restent ouverts ; main runtime inchangé.

### Compilation cohérente ARM64 démarrée ensuite

Source 2d2764de readonly, image/volume existants ; nouveau répertoire /build/coherent-2d2764de-arm64-20261002. Sept cibles : nano, transition Capella, helpers encodage/ancestry, hex différentiel, core, audit rotation. Ce n’est pas la compilation de toutes les cibles de la régression. Un CPU/2 GiB ; Docker Mac annonce 15 CPU/~7,75 GiB. La régression 3030b799 reste indépendante et ses artefacts inchangés. Conteneur nano-coherent-2d2764de-arm64-build confirmé actif ; journal /tmp/nano-coherent-2d2764de-arm64-build-reports-20261002/build.log. Aucun résultat de test du build cohérent ni promotion à ce stade.
