# Candidate efbeb9bc : stockage portable et restauration en attente

## Lectures physiques croisées ARM64/AMD64 — réussies

`physical-cross/result.json` rapporte un exit 0. Les vrais helpers compilés avec
la candidate complète créent chacun une base physique avec SST et OPTIONS
format 6. Chaque architecture lit la copie des fichiers écrits par l’autre,
avec payload, séquence 1 et engagement connus indépendamment. Les transferts,
lectures et programmes restent identiques par SHA-256. Les writers utilisent
les images et volumes existants ; aucune image reconstruite. Bases synthétiques
exclusivement, API d’écriture refusée lors de la lecture, mounts en lecture seule.

Les campagnes complètes Docker tournent pendant cette preuve : aucune mesure
de performance isolée, aucun résultat complet de régression hérité. Les bases
restent sous /tmp et /home/ubuntu ; aucun fichier de base n’est publié dans Git.

## Journaux de signature de la copie migrée — audit natif réussi

`journals/result.json` : 22 journaux relus avec la candidate et les engagements
d’un reçu d’arrêt conservé indépendamment ; état original et copie migrée
strictement identiques. Le témoin cœur correspond à l’engagement et au snapshot
commis. 66 bindings erronés sont rejetés dans chaque fixture : token, genesis,
témoin d’une autre clé. 331 fichiers originaux, 461 fichiers migrés et 603
inputs natifs inchangés. Aucune signature demandée ni duty reprise ; aucun
audit Linux ni fork choice historique déduit.

La première sandbox imbriquée refusée (exit 71) est conservée. Le lancement
autorisé avec sandbox interdisant réseau et écritures a ensuite terminé exit 0.
Aucune clé privée ni base de données n’est copiée ici.

## Restauration effective — pas encore lancée

`restore-queue/run.py` attend les contrôleurs Docker déjà actifs. Il refuse
un échec et ne redémarre aucun job. Avant de lancer la fixture privée, il exige
427 cas chacun, sorties terminales 0, source/programmes inchangés et hashes
réels des daemons ; il rehash les 603 inputs natifs et exige le cross-reader SST.
Il utilise exclusivement les binaires efbeb9bc et le harness figé de cette
révision, avec chemins natifs, P2P, images et volumes explicitement sélectionnés.

Session de la queue : 8231. Snapshot initial : deux conteneurs complets actifs,
phase waiting, wanStarted false. Aucun résultat de restauration n’est encore
revendiqué. À terme le harness créera une fixture neuve, sauvera/restaurera le
nœud VPS avec pins indépendants, reprendra les duties et exigera la finalité
commune qui avance avant le contrôle de stabilité et l’audit terminal.

La production reste non qualifiée ; tous les critères et échecs historiques
des quatre feuilles de route restent conservés.
