# Diagnostics de capacité et charge RPC — 2026-10-01

Les échecs originaux de la campagne 400 restent conservés. Aucun délai,
seuil de finalité, assertion ou règle de gas n'a été assoupli.
Builder ARM64 existant 61166694…, limites 2 CPU/3 Gio, réseau externe coupé,
volume nano-linux-sync-build ; sous-répertoires TMPDIR distincts sur disque.
Le tmpfs de la campagne originale : rw,nosuid,nodev,noexec,size=131072k.

## Exécutables originaux inchangés

nano bbe09460…, nano-p2p ae567df0…, fixture bft ddaaaa90….
Paris gros bloc : CTest exit 0, 22.68 s (délai 240 s, attente interne 100 s).
Shanghai gros bloc : réussite 22.50 s, mêmes délais/assertions. La majorité
certifie les cinq transactions dans un même bloc, page >8 Mio, certificat
vérifié ; le nœud isolé se reconnecte et reprend l'historique.
Installation : exécution directe originale exit 0, « Native relocated
installation checks passed ». Le chemin temporaire disque autorise exec ;
le noexec du tmpfs expliquait son précédent Permission denied.
Ces replays établissent le fonctionnement sur disque, pas une garantie
universelle de capacité ou une panne physique. Les timeouts gros bloc du
premier run restent échecs historiques : aucune trace d'erreur disque initiale
ne permet d'affirmer à elle seule que chaque timeout provenait d'ENOSPC.

## RPC multi-nœud : échec maintenu

Le rejeu original Paris sur disque échoue encore (job deux tests exit 8,
66.52 s au total). Traces activées par NANO_RPC_TIMING_DIR : aucune clé ni
paramètre enregistré. Préparation max 1.42 ms, verrou max 1.08 ms, estimation
max 4999.58 ms, worker max 5002.34 ms, file max observé 3913.03 ms.
La limite RPC d'exécution reste 5 s, celle de file 4 s. Les estimations répétées
sont un chantier à profiler/optimiser sans modifier la sémantique EVM.

## Expérience EVM retirée

Dans checkout isolé base 97f4a19d, remplacer la copie GMP de Frame::pop par
un déplacement compile. Cœur et RPC passent (0.33/1.30 s), mais le scénario
multi-nœud échoue encore en 40.52 s ; job exit 8, 42.16 s au total.
Aucun gain fiable ni correction du gate n'est revendiqué. Patch expérimental,
traces et hash b3a6df05… archivés ; source rétablie au commit 97f4a19d.
Les binaires du build isolé reflètent encore cette expérience retirée : ne pas
réutiliser leur qualification avant reconstruction depuis la source propre.
Le build original /build et le candidat en campagne complète sont inchangés.

Restent ouverts : expirations RPC, admission du scénario restart_flood,
régression complète, qualification des correctifs client sur CLI, AMD64/WAN,
profil EVM activé, sécurité indépendante et totalité des critères production.
