# Peer source de l’hôte RPC header-selection

Verdict : GO SOURCE ciblé sur `host.py`
`0133e133099e845f3a40686f3c3746ab3fe68c2b3974839cb1c7a06fc89eb8c4` /29981 B,
Root auteur. Aucune exécution/import projet, opération, lecture SDK/autorité
privée, contrôle privé ou mutation R effectuée par ce peer.

Deux défauts de préparation relevés et corrigés prospectivement :

1. Le draft `0a08a220…` exigeait `--self-test` pour les trois unités, alors que
   les CLI sont `--state-read-cost-self-test`, `--state-readers-self-test` et
   `--get-proof-self-test`. Le mapping par unité est explicite dans `a658a928…`
   /29965 B ; les noms/exits/oracles restent contrôlés.
2. Le futur test coût utilise une vraie fixture Beacon nécessitant
   `NANO_TEST_SOURCE`. CMake doit enregistrer cette propriété, pas seulement la
   recette Root. `0133e133…` ajoute `CMakeLists.txt` au delta explicite des deux
   projections (dix fichiers), ainsi que le diagnostic correspondant. Inverser
   ces deux changements textuels restitue exactement
   `a658a928e7738744019efa7db4c57fc4b448d38ca9d162c27da47f34e303137e` /29965 B.
   Le chemin `third_party` nécessaire au header BLST est déjà hérité de
   `nano_core` PUBLIC ; aucun nouvel include général n’est requis pour ce joint.

Raccords vérifiés statiquement contre worker V2 `e9bfc40c…` : configuration
22 champs en mode incremental, 143 scans Core et trois scans tests, K Core
sélectionnés par union des dépendances/MD, K+3 compilations, une mise à jour
archive, trois liens/ldd/tests ; ensemble exact 156+2K produits. MMD utilise
`normalizedMEqualsMd`. Six bibliothèques/headers générés ont leurs pins et
montages RO distincts. ARM exige le pin Core `11c1494f…` ; AMD dépend de la
véritable ancienne ligne externe Root, sans transformer guestStatSha en FD9.

Nettoyage : CID complet ou fallback par nom avec contrôles name/image/label
avant stop/rm, aucun PID28722 ; première erreur conservée. Les anciens inputs,
les nouvelles autorités et les produits worker sont tenus jusqu’à la clôture
full9/SHA après cleanup et image-final. Les publications fence/résultat sont
séparées. La source Root actuelle et les doubles projections restent sous les
pins du contrôle ; aucune hypothèse que le candidat C++ soit déjà dans main.

Ce GO ne qualifie ni les tests Beacon/ABA encore en préparation, ni une
exécution ARM/AMD, ni CMake réellement configuré, ni l’ensemble SDK ou la
production. Root fournit les autorités, recettes exactes, oracles prospectifs
et les résultats réels. Aucun test antérieur n’a été rejoué pour cette revue.
