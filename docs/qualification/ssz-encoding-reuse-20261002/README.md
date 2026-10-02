# Réutilisation de l’encodage canonique SSZ — candidat 12ad9eb5

Parent e91d647b, branche qualification/production-candidate-20261002. Runtime main inchangé. Les sources gelées 3030b799 et la régression ARM64 active sont préservées.

Le cache compare directement tous les octets SSZ à leur encodage hexadécimal canonique avant de réutiliser celui-ci. Le dernier traitement d’époque est reconstruit depuis l’état courant. La comparaison complète de ligne reste obligatoire : en-tête, epochs fork-choice, bloc et état d’exécution, reçus, historique de dépôts, justification et statut d’ancre. Ni simple égalité de racines ni identité de pointeur utilisée. Limites, validation sur miss, profil, suppression des blocs absents et annulation conservés. Scan annulable tous les 4096 octets et en fin de scan. Le dump JSON et les autres conversions demeurent : aucun gain global annoncé.

## Qualification Docker ARM64 directe

Image existante 61166694…, bibliothèque core 3030b799 gelée montée readonly. Compilation directe O2 des deux modules modifiés (snapshot et audit cohérent) puis du test intégré contre les bibliothèques existantes ; ce n’est pas un rebuild CMake complet ni une qualification AMD64/WAN. Un CPU, 2 GiB, aucun réseau, root/source/volume readonly, cap-drop ALL.

- Helper natif : 773 refus de mutation/format, les 256 valeurs d’octet, entrée vide, grands buffers et annulation bornée passent.
- Intégration : trois variantes de transition Capella (sans migration, avec migration, migration avec slots omis) passent. Nouveaux contrôles différentiels : modification d’octet SSZ et changement du dernier epoch traité donnent le même résultat avec cache et sans cache, sans produire l’ancien snapshot. L’assertion existante de mutation de balance reste exécutée. Cache chaud et encodage sans cache restent identiques.
- Exit intégré et conteneur : 0. Empreintes programme, sept entrées statiques de lien et 14 906 sources avant/après inchangées (voir manifests). Bibliothèques dynamiques héritées de l’image fixe ; pas de qualification exhaustive de dépendances.
- Tentative A conservée : helper passe, compilation intégrée échoue faute de NANO_TEST_SOURCE. Tentative B : cette définition corrigée, édition de liens échoue faute du nouveau module audit. Aucun test intégré exécuté dans A/B. Tentative C ajoute ce module et passe ; helper inchangé non rejoué.

## Suite et limites

Checkout VPS mis à jour par Git au même candidat. Build CMake neuf des cibles nano, transition et deux helpers démarré dans /build/coherent-12ad9eb5-amd64-20261002, volume/image existants, un CPU/2 GiB. Aucun test AMD64 de ce candidat encore exécuté. Journal : /home/ubuntu/nano-coherent-12ad9eb5-amd64-build-reports-20261002/build.log ; conteneur nano-coherent-12ad9eb5-amd64-build. L’échec 3030b799 de réindexation à 360 s reste ouvert. Pas de temps allongé. Toutes les autres gates de production, régression complète, WAN/restauration, EVM activée, bridge/Doppler et revue indépendante restent ouvertes.
