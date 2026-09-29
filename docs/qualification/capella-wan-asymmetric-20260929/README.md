# Capella WAN : retard asymétrique sortant de 500 ms — 29 septembre 2026

**ÉCHEC de la campagne complète**, phase sous retard réussie. Aucun changement
du seuil ni nouvelle exécution pour masquer ce résultat. Statut détaillé :
`manifest.json`, `run.jsonl` et `observations/failure.txt`.

Les trois instances atteignent l'époque 4 depuis 2 en **159,156 s** sous retard
(2 092 paquets, délai noyau 0,5 s, zéro drop dans ce relevé). Le défaut est retiré.
La fenêtre suivante observe 4 puis 6 ; le moniteur refuse ce saut avant d'avoir
validé ses trois avancées consécutives. Les traces internes de devoirs corroborent
cette observation : époque 4 aux slots 56–71, puis 6 aux slots 72–73, aucune
observation de 5 dans les trois traces. Cela ne suffit pas à attribuer la cause
à la perte/retard d'un vote particulier. Ce n'est pas simplement un succès
réseau ni une preuve d'un conflit de finalité.

Les trois statuts terminaux sont sains (183/176/169 attestations locales admises),
les deux conteneurs ont terminé sans OOM. Les journaux et volumes sont conservés.
Le cœur FFG possède quatre règles de finalisation (`src/consensus_altair_epoch.cpp`
et `src/consensus_ffg.cpp`) ; il n'impose pas un incrément systématique de +1.
La prochaine analyse doit reconstruire justification/participation aux frontières
pertinentes depuis les états/blocs, avant d'attribuer ce saut à un défaut du cœur
ou de réviser le cadrage temporel du contrôle de stabilité. Ce run reste rouge.

## Périmètre

Trois instances réelles : macOS natif, Docker Linux ARM64 sur ce Mac, Docker
Linux x86-64 sur le VPS autorisé. 64 clés publiques synthétiques, réparties
22/21/21. Noyau `8d1cc1769e9a3e9299dbd73de4dfd7a57f75bb5e` ; aucune
modification de `src`, `include`, `network` ou `third_party`. Images et volumes
de compilation réutilisés. Source du banc et empreintes des binaires conservées.

Le réseau est une étoile passant par le VPS avec transport libp2p via SSH privé.
Les RPC restent sur loopback. `tc netem delay 500ms` ne touche que `eth0` dans
l'espace réseau du conteneur VPS frais, après validation du label, de la révision,
de l'identité/PID, du réseau exclusif et de la différence avec l'espace réseau hôte.
Aucune modification du réseau hôte. Les ACK TCP sortants sont retardés aussi :
ce scénario n'est pas une mesure de latence applicative unidirectionnelle pure.

## Critères fixés avant le run

- Checkpoint finalisé commun initial >= 2, lié au même bloc d'exécution.
- Retard maintenu au moins 96 s ; dans les 144 s supplémentaires, progression
  d'au moins deux époques communes, pairs connectés, aucune régression ni racine
  finalisée contradictoire observée. Aucun allongement du délai après observation.
- Statistiques `netem` : paquets effectivement traités ; retrait du défaut même
  si RPC, finalité ou réponse SSH échoue. L'échec de nettoyage reste un échec.
- Après restauration : trois avancées consécutives communes ; contrôle de chaque
  checkpoint Beacon et de son bloc d'exécution canonique sur les trois RPC.
- À l'arrêt, chacun des trois groupes de validateurs doit avoir un statut sain,
  sans failure, avec attestations locales admises. Un simple follower ne suffit pas.

Le contrôle sous défaut compare les checkpoints Beacon ; la liaison détaillée
aux blocs d'exécution est vérifiée au démarrage et pendant la fenêtre de stabilité.
Les quinze nouveaux tests du banc couvrent notamment perte de réponse SSH après
mutation, RPC défaillant, finalité stagnante/contradictoire/régressive, absence de
paquets, nettoyage refusé, mauvaise provenance et changement de PID. Avec les
26 cas existants, 41 cas passent, également via la nouvelle entrée CTest.

## Reproduction

```sh
NANO_TEST_CONSENSUS_PROFILE=capella NANO_TEST_BUILD_VOLUME=1 \
NANO_TEST_WAN_ASYMMETRIC=1 NANO_TEST_NETWORK_TRACE=1 \
node tests/multihost/run.mjs ubuntu@145.239.199.166 \
8d1cc1769e9a3e9299dbd73de4dfd7a57f75bb5e /tmp/NEW-OUTPUT
```

L'export produit un nouveau genesis de test. Les volumes de build doivent porter
la révision ci-dessus ; la révision du dépôt peut avoir avancé pour les seuls
bancs/documentation, mais un écart du cœur bloque le préflight. Les conteneurs
restent arrêtés pour inspection après le run ; leur suppression est séparée des
volumes de données conservés. Aucun fonds réel, vente, exposition RPC ou flood.

## Limites

Deux machines physiques et 64 identités de test ne représentent ni 64 opérateurs
indépendants ni une qualification de grande population. Pas de charge applicative,
perte aléatoire de paquets, corruption Byzantine ou panne physique dans ce run.
La rotation des comités n'est pas authentifiée par ce scénario court ; ses preuves
locales restent distinctes et son extension WAN reste à couvrir. Une fenêtre
bornée sans conflit n'est pas une démonstration universelle de sécurité BFT.
Ni activation d'un nouveau fork EVM, ni clôture EIP générale, ni production prête.
