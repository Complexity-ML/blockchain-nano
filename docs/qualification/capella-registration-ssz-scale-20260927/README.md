# Résolution des clés locales dans un registre SSZ à grande échelle

## Changement

La résolution des clés automatiques Capella ne projette plus tous les membres
via `capella_fork_block_context`. Elle authentifie le SSZ complet contre la
racine de la tête déjà admise, vérifie la cardinalité des listes, puis parcourt
les enregistrements de 121 octets en ne conservant que les clés demandées.
Le parcours complet conserve le refus des doublons, même après tous les matches.
Les contrôles de devoir, de signature et le journal restent inchangés.

## Qualification

Deux tests passent en **29,37 s** :

- `consensus_capella_registration_scale`, nouveau test : états synthétiques
  de 100 000 et 1 000 000 entrées, quatre clés demandées dont une absente,
  trois index exacts trouvés ; racine incorrecte, clé de largeur incorrecte,
  doublon demandé et annulation refusés. Un état à racine valide mais avec une
  balance manquante est refusé à 100 000 entrées. La source reste inchangée.
- `consensus_capella_automatic_registration`, régression ciblée du runtime :
  64 clés enregistrées et une absente, attente sans devoir, propositions,
  reçus et reprise des journaux/témoins. Aucun HTTP/P2P dans ce test.

| Entrées | SSZ (octets) | Authentification + recherche | Pic RSS du processus |
| --- | ---: | ---: | ---: |
| 100 000 | 13 910 821 | 145 599 µs | 54 542 336 octets |
| 1 000 000 | 139 010 821 | 1 432 485 µs | 346 947 584 octets |

Une mesure locale par taille ; aucun seuil de débit ou gain relatif revendiqué.
Le pic RSS inclut le générateur et les contrôles négatifs, et cumule le maximum
du processus ; ce n'est pas l'allocation propre de la recherche. La racine seule
mesurée séparément coûte respectivement 160 693 et 1 608 758 µs ; les variations
entre mesures ne démontrent pas une accélération de cette primitive.

## Limites et prochaine étape

L'authentification SSZ reste linéaire et exécutée sous verrou du reactor à chaque
poll automatique. 1,43 s à un million constitue encore un coût opérationnel
significatif. Qualifier une réutilisation liée à la racine authentifiée, sans
cache périmé lors des changements de branche, avant de promettre ce débit.

Les clés de la mesure sont des octets synthétiques, sans vérification BLS/PoP.
Le helper ne remplace ni l'admission d'état, ni les dépôts, ni l'activation.
Aucun million de nœuds, réseau distribué, crash pendant commit ou signature
concurrente à une réorganisation n'est testé ici. Aucun EIP/profil ni objectif
production n'est clôturé. Pas de déploiement VPS.

`manifest.json` contient la révision de base, les empreintes des sources et
binaires testés. Les logs et le JUnit conservent les observations exactes.

Les espaces de fin de ligne du log CMake ont été normalisés pour Git.
