# Objectif candidat production — état de départ audité

## WAN asymétrique : défaut testé, stabilité encore ouverte — 29 septembre 2026

La [nouvelle campagne réelle Mac/Docker/VPS](../capella-wan-asymmetric-20260929/README.md)
applique 500 ms de retard sortant au seul conteneur VPS. Les trois nœuds
progressent de la finalité 2 à 4 en 159,156 s sous défaut (2 092 paquets mesurés).
Après retrait, le moniteur échoue sur le saut 4 → 6 avant ses trois avancées
consécutives : **campagne globale en échec**, malgré les trois validateurs sains.
Les traces internes confirment les observations 4 aux slots 56–71 puis 6 aux
slots 72–73 ; cause à reconstruire depuis justification/participation, aucun
seuil relâché. 15 nouveaux cas de sécurité du banc, 41 cas au total, passent.
Rotation WAN, charge, exploitation, bridge et production restent ouverts ;
les preuves de rotation locale existantes sont référencées dans l'audit candidat.

**Mise à jour : la première campagne Capella WAN est réussie.**
[Preuves et limites](../capella-wan-partition-20260929/README.md) : reprise
en 136,691 s, progression 7→10 et trois validateurs sains. Les sections
suivantes conservent le journal de préparation ; le prochain manque porte
sur les perturbations asymétriques WAN et la charge bornée.

Base : `8d1cc1769e9a3e9299dbd73de4dfd7a57f75bb5e`. Objectif actif, production
non démontrée. Les sorties de validateurs ont leur audit de clôture distinct.

## Prochaine qualification : reprise de finalité WAN

- L'ancien contre-exemple local à 500 ms est résolu : voir
  `../capella-stream-reuse-20260927/README.md`. Ne pas rejouer ce cas comme
  s'il était encore l'échec actif.
- L'échec WAN historique reste conservé dans
  `../multihost-20260927/page-cache-recovery/README.md` : trois instances,
  deux hôtes, coupure de 96 s, reprise observée à 189,2 s pour un seuil de
  180 s. La progression ultérieure ne transforme pas cet échec en succès.
- Le run de sortie Capella entre Mac et VPS qualifie SIGKILL/reprise/paiement,
  pas la finalité sous partition avec tous les validateurs toujours actifs.
- Le banc `tests/multihost/prepare.mjs` était spécifique à Bellatrix. Il accepte
  maintenant les exports Capella et refuse les profils mixtes. La répartition
  des vrais exports de test a été vérifiée : 22/21/21 clés uniques, 64 au total,
  deux hôtes, trois instances, profils inchangés. Le fichier d'identité P2P natif
  est isolé dans le répertoire de cette instance.

Cette validation de configuration n'est pas un résultat réseau. L'orchestrateur
`run.mjs` sélectionne maintenant Capella avec `NANO_TEST_CONSENSUS_PROFILE=capella`,
et contrôle les statuts terminaux Capella (un profil Bellatrix ne suffit pas).
Les 26 tests du banc passent, dont le nouveau rejet de profil erroné ou de
validateur Capella arrêté. Les artefacts Docker réutilisables restent à raccorder ;
le run doit ensuite atteindre la finalité commune liée aux
blocs d'exécution dans les délais existants. Aucune baisse du seuil de quorum,
aucun relèvement du délai de reprise pour transformer l'ancien échec en succès.

## Travaux qui restent dans l'objectif

Après le WAN : perturbations asymétriques distribuées, rotation et sanctions,
charge bornée avec ressources mesurées, écarts du profil EVM/RPC activé,
qualification d'une version candidate et exploitation/restauration. Le bridge
USDC et le fair launch exigent leur propre spécification et qualification.
Aucun fonds réel, lancement public, achat ou audit indépendant réputé effectué.

## Build ARM64 réutilisable terminé

Le volume local `nano-linux-sync-build` contenait un cache CMake, mais aucun
exécutable `nano`/`nano-p2p`. Le conteneur `nano-production-build-20260929`
compile ces deux cibles dans ce volume avec `nano-build-dependencies:local`,
source montée en lecture seule et réseau désactivé. Les deux cibles ont compilé,
code de sortie 0 sans OOM (46 s). Journaux conservés ici. Aucune nouvelle image.

## Première campagne Capella WAN active

`/tmp/nano-production-wan-capella-20260929`, noyau `8d1cc17`, mode
`NANO_TEST_BUILD_VOLUME=1`. Le lanceur vérifie la révision enregistrée dans les
volumes, l'architecture de chaque image et les empreintes des binaires. Les
volumes sont montés en lecture seule pendant le run. Le P2P VPS est publié sur
127.0.0.1 uniquement et transporté par SSH ; les trois instances sont connectées
(1/1/2 pairs observés). La finalité de départ puis la partition restent à qualifier.
Les scripts exacts exécutés sont copiés dans `executed-harness` du run.

Aucune modification du cœur depuis les preuves de sorties. Le build ARM64
et la vérification incrémentale x86-64 sont terminés ; aucune nouvelle image.

## Audit des acquis de rotation — 29 septembre

La rotation n'est pas un chantier vierge : les preuves existantes couvrent
[216 blocs et trois frontières avec reprise](../capella-sync-rotation-20260927/README.md),
[deux vrais pairs locaux et huit agrégats finalisés authentifiés](../capella-distributed-sync-rotation-20260927/README.md),
puis [une interruption de transport de 96 s traversant le slot 128](../capella-sync-rotation-outage-20260927/README.md).
Cette dernière atteint la finalité 18 et vérifie cinq agrégats avec positions
signées par l'autre pair après récupération. Ces résultats restent historiques,
avec leurs sources et limites ; ils ne justifient ni un rejeu aveugle ni une
clôture WAN. La tranche encore manquante est la rotation authentifiée entre
hôtes distincts, avec défaut réseau traversant la frontière et contrôle des
agrégats depuis l'état finalisé rouvert. La campagne WAN courte actuelle ne
prétend pas couvrir cette frontière.
