# Audit de clôture du jalon sorties — 29 septembre 2026

**Qualification technique du jalon acquise dans le périmètre ci-dessous.**
La publication Git est vérifiée séparément à la clôture. Les six critères
restent ceux de `docs/PRODUCTION-GOAL.md` ; aucune validation mainnet implicite.

| Critère | Preuves inspectées | Conclusion et portée |
| --- | --- | --- |
| 1. Admission authentifiée et bornée | [Contexte BLS](../capella-exit-authentication-20260929/README.md), [régression finale](../capella-exit-reinclusion-20260929/README.md), [RPC/P2P](../capella-exit-network-20260929/README.md) | Domaine, fork, genesis, clé/index, époque et tenure ; identité de chaîne du service, taille, file saturée, annulation, doublons. Rejets sans mutation. RPC positif réel ; rejets détaillés locaux. |
| 2. Diffusion, inclusion, reprise | [Opération autonome](../capella-exit-offline-relay-20260929/README.md), [WAN SIGKILL](../capella-exit-multihost-20260929/README.md) | Demande admise hors connexion, octets conservés après fermeture/réouverture, redémarrage sans resoumission, export et livraison cpp-libp2p, inclusion et paiement. Sur WAN, déconnexion par SIGKILL puis reprise et finalité. Ces deux scénarios ne représentent pas un test exhaustif de tous les accusés de réception perdus en vol. |
| 3. Conservation/reorg/index | [Archive](../capella-exit-retirement-20260929/README.md), [réindexation](../capella-exit-reindex-20260929/README.md), [réinclusion](../capella-exit-reinclusion-20260929/README.md) | Archive conservée avant finalité et retirée après ; sortie redevenue applicable après reorg/réouverture puis sélectionnée dans un nouveau bloc signé accepté. Ancien index refusé ; bon index inclus, autre validateur intact. |
| 4. Finalité/retrait/signatures | [WAN et propriétaires rouverts](../capella-exit-multihost-20260929/README.md), [réseau local](../capella-exit-offline-relay-20260929/README.md) | Bloc finalisé 55 commun sur Mac/VPS, sommes exactes et capital unique au bon destinataire. Journal du signataire vérifié sur Mac, où sa clé réside ; pas de proposition/attestation après l'époque de sortie. |
| 5. Panne/WAL/pruning | [Admission WAL](../capella-exit-wal-20260929/README.md), [seize transitions WAL](../capella-exit-lifecycle-sync-20260929/README.md), [sortie non incluse élaguée](../capella-exit-pending-pruning-20260929/README.md), WAN SIGKILL | Quatre transitions × quatre variantes sur Mac et Linux. Reprise exacte par témoins, accès incertains refusés, registre/file/EVM cohérents, aucun double capital, rejeu idempotent. Pruning réancre sans perdre la sortie valide. |
| 6. Sources/binaires/régressions | [Continuité des sources](source-continuity.json), manifestes et patches exacts dans chaque preuve | Sources `src/include/network/third_party` identiques depuis `77cb84b`. Binaires du nœud et du P2P identifiés ; bancs versionnés séparément. Dernière sélection CTest 3/3 en 16,45 s, réseau hors connexion 1/1 en 427,44 s, Linux 3/3 en 476,02 s, régressions précédentes 6/6 sur le même code de production. |

## Limites qui ne sont pas masquées

- Profils synthétiques : 64 validateurs, tenure/délai de retrait raccourcis.
  Ni réseau de 10 000 nœuds ni paramètres de production qualifiés ici.
- Les injections WAL et le SIGKILL ne prouvent pas une coupure électrique.
  Les témoins sont des bases séparées, sans indépendance physique du disque.
- Le WAN transporte effectivement cpp-libp2p via un tunnel SSH privé. La
  livraison autonome de l'opération est prouvée localement ; les traces WAN
  démontrent sa propagation dans des blocs, pas une livraison autonome WAN.
- Le garde 404 du script WAN maintenu a été durci après le run conservé ; ce
  durcissement n'est pas présenté comme exécuté dans ce run. Le résultat
  historique est lié au banc exact `executed-harness.mjs` et aux propriétaires
  rouverts qui confirment ensuite la même finalité et la même comptabilité.
- Les tests de réorganisation et de réindexation sont distincts du paiement
  WAN finalisé. Le tableau expose cette composition au lieu d'inventer un
  unique scénario réunissant toutes les pannes.

Ce jalon ne clôture pas la qualification générale du consensus sous partitions
asymétriques/charge, les écarts EVM/RPC, l'exploitation mainnet, le bridge USDC,
le fair launch ni une revue indépendante. **Production globale non démontrée.**
