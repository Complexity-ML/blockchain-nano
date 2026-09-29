# Partition WAN Capella — réussite du 29 septembre 2026

Trois instances : Mac natif, Docker ARM64 sur Mac et Docker x86-64 sur VPS.
64 clés synthétiques réparties 22/21/21. Binaires du code `8d1cc17`, profils
Capella/Shanghai minimaux, slots de 6 s, époques de 8 slots. Aucun fonds réel.
Les deux conteneurs montent leurs binaires en lecture seule depuis les volumes
existants, sans nouvelle image. Les empreintes sont dans `manifest.json`.

## Résultat

- Finalité commune de départ : époque 2, liée au même bloc d'exécution.
- Coupure configurée de 96 s ; intervalle observé entre événements : 97,239 s.
- Déconnexion du réseau Docker dédié du VPS, processus des trois nœuds actifs.
  Topologie en étoile : les deux Mac perdent aussi leur unique pair. Les trois
  groupes observent zéro pair et conservent la même finalité d'époque 2.
- Reconnexion à la même adresse réseau ; même identité et mêmes stores.
- Reprise commune époque 7 en **136,691 s**, avant le seuil inchangé de 180 s.
- Progression commune **7 → 8 → 9 → 10** en 152,575 s ; à chaque avancée,
  accord des checkpoints Beacon et liaison aux mêmes blocs d'exécution.
- Arrêt propre sans OOM. Trois boucles saines, 247/239/234 attestations admises.
- Processus orchestrateur terminé avec code 0. 26 tests du banc également passés.

Le transport est le vrai cpp-libp2p via un tunnel SSH privé. Le P2P VPS est
publié seulement sur 127.0.0.1 ; RPC lus localement ou via `docker exec`/SSH.
La mesure de délai inclut les appels de contrôle et leur échantillonnage ; elle
ne prétend pas mesurer l'instant interne exact de finalisation.

## Limites

Ce résultat qualifie cette partition WAN Capella, pas tous les défauts réseau.
Il ne prouve ni pertes/asymétrie WAN, ni majorité restante connectée pendant
la coupure, ni nombreux pairs, ni charge utilisateur, ni rotation de comité
spécifiquement contrôlée, ni production globale. Les anciennes campagnes
Bellatrix échouées restent inchangées : ce run n'est pas une comparaison
causale isolant un correctif unique entre les deux versions.

Les scripts exacts sont conservés dans `executed-harness`, les logs bruts sont
compressés sans modification, les fixtures sont publiques et synthétiques.
Stores et volumes restent à leurs chemins enregistrés dans les configurations.

```sh
NANO_TEST_CONSENSUS_PROFILE=capella NANO_TEST_BUILD_VOLUME=1 \
NANO_TEST_WAN_PARTITION=1 NANO_TEST_NETWORK_TRACE=1 \
node tests/multihost/run.mjs ubuntu@145.239.199.166 8d1cc1769e9a3e9299dbd73de4dfd7a57f75bb5e /tmp/NEW-CAPELLA-WAN
```

La commande exige les binaires et le fichier de provenance déjà préparés dans
`nano-linux-sync-build` sur les deux machines. Elle ne construit pas d'image.
