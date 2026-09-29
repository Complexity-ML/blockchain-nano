# WAN asymétrique avec fenêtre de récupération — 29 septembre 2026

**ÉCHEC conservé** : `Consistent finality sample unavailable` sur le VPS.
Pas de changement du noyau, des seuils de consensus, des délais ou du défaut
réseau depuis le run initial. Aucun retry/relaunch après cet échec.

Le [saut FFG initial](../capella-wan-ffg-analysis-20260929/README.md) est expliqué
par le checkpoint réel et le rejeu natif. Le moniteur corrigé recherche une
fenêtre consécutive après rattrapage, conserve son historique de sécurité et
son échéance absolue. 19 tests passent, dont six nouveaux. Le mode strict reste
inchangé pour les appelants qui n'activent pas `settleRecoveryGaps`.

Trois instances (Mac natif, Docker Mac, Docker VPS), deux hôtes physiques,
64 clés synthétiques 22/21/21 ; mêmes images et binaires du noyau `8d1cc17`.
Sous 500 ms sortants sur le seul conteneur VPS, finalité commune **2 → 4 en
159,741 s**, 2 107 paquets enregistrés dans netem, aucun drop dans ce relevé.
Le défaut est retiré et le moniteur démarre. Lors de sa première observation,
le head VPS alterne A→B, B→A, A→B pendant les trois tentatives, alors que les
checkpoint lus restent tous à l'époque 4 avec la même racine. Aucun échantillon
cohérent n'est accepté, donc aucune fenêtre stable n'est qualifiée. Les trois
validateurs terminent sains (154/147/144 attestations locales admises).

L'API actuelle ne sert les checkpoints que sous `/states/head/finality_checkpoints`.
Trois appels HTTP/SSH successifs ne forment pas une lecture atomique. Il reste à
ajouter une lecture par **state root** immuable et à l'utiliser depuis le header
capturé, avec tests de head concurrent, état inconnu/élagué, mauvais engagement et
conflit réel. Il ne s'agit pas d'augmenter le nombre de retries ou de transformer
la santé des validateurs en preuve de finalité. L'oscillation du head sous reprise
reste observée ; sa cause spécifique n'est pas attribuée dans ce rapport.

Les artefacts incluent les scripts effectivement exécutés (y compris le moniteur
importé), les configurations et clés de test publiques, les empreintes des
binaires, les lectures, l'erreur exacte, les logs comprimés et les statuts Docker.
Volumes et base native conservés, conteneurs arrêtés puis retirés séparément.
Les preuves du run précédent restent intactes. Production, rotation WAN, charge,
exploitation et bridge demeurent ouverts. Aucun fonds ou service public.

Reproduction : même commande que le run initial, avec le banc de ce commit,
`NANO_TEST_CONSENSUS_PROFILE=capella NANO_TEST_BUILD_VOLUME=1
NANO_TEST_WAN_ASYMMETRIC=1 NANO_TEST_NETWORK_TRACE=1`, révision binaire
`8d1cc1769e9a3e9299dbd73de4dfd7a57f75bb5e`, nouveau répertoire de sortie.
