# Qualification système Nano — 2026-09-26

## État observé

- EEST : 21 780 cas réussis dans le périmètre documenté dans `EEST.md`.
- Kurtosis natif : deux nœuds, 64 validateurs synthétiques, même bloc finalisé 15.
  Preuves : `qualification/kurtosis-20260926/summary.json`.
- Assertoor officiel : première campagne native de trois tâches passée ; preuves
  dans `qualification/assertoor-native-20260926/README.md`.
- Ces résultats ne valident pas automatiquement les scénarios ci-dessous.

## Prochaines campagnes, par priorité

| Priorité | Campagne | Critère de réussite | État |
| --- | --- | --- | --- |
| P0 | Crash de processus sous écritures EVM bornées | Après SIGKILL, reprise atomique des états EVM/Beacon et journaux de signature, aucune double signature, mêmes engagements finalisés après synchronisation | SIGKILL sous 24 transactions réussi sous Kurtosis ; bloc 45 finalisé commun, effets exacts. Frontières précises d’écriture et panne physique non qualifiées |
| P0 | Partition puis reconnexion | Aucun checkpoint finalisé contradictoire ; quorum perdu implique arrêt de finalité à court terme ; convergence après rétablissement | Test natif existant ; transposition conteneurs à faire |
| P1 | Charge bornée et frais dynamiques | Débit, latence, mémoire, croissance disque, rejet contrôlé de surcharge, finalité et règles EIP-1559 mesurés | À qualifier sous Kurtosis |
| P1 | Invariants applicatifs sur Nano | Séquences reproductibles exécutées sur Nano, comparaison des reçus/états et conservation comptable adaptée au contrat | Générateur/rejeu à raccorder ; Forge seul ne qualifie pas Nano |
| P1 | Supervision Beacon | Exposer progression des slots, justification/finalité, santé de l'horloge et validateurs avec contrôle d'accès adapté | Ancien champ réseau `validating` insuffisant |
| P2 | Assertoor | Endpoints EL et Beacon réellement compatibles avec les tâches retenues, puis résultats propres au réseau Nano | Trois tâches natives passées ; campagne finalité 32/32 échouée (époque 0, timeout). Diagnostic prioritaire ; partitions et cycle des validateurs ouverts |

## Contraintes et corrections de méthode

Assertoor attend des endpoints d'exécution et de consensus dans sa configuration
standard (`executionUrl`, `consensusUrl`). Le seul JSON-RPC Ethereum de Nano ne
suffit donc pas à rendre ses tâches Beacon utilisables. Préparer les interfaces
nécessaires ou des tâches adaptées ; ne pas annoncer les scénarios intégrés comme
qualifiés après un simple démarrage de l'outil. Cela n'impose pas Lighthouse ni
une séparation du binaire Nano.

`blst` est la bibliothèque de signatures, pas l'algorithme de finalité. Les
scénarios de quorum portent sur le **poids** de vote, pas simplement le nombre de
processus. Une perte de 40 % du poids empêche à court terme un quorum strict de
plus de deux tiers. Sur une longue partition, traiter séparément les règles
économiques d'inactivité et l'évolution des poids.

Les BLS-to-execution changes concernent les credentials de retrait ; ce n'est pas
une rotation arbitraire des clés actives de signature.

Pour le fuzzing, Forge utilise son propre moteur. Générer puis rejouer les
séquences signées sur Nano, ou employer un injecteur pilotant réellement Nano.
Un invariant de lending doit tenir compte des créances, dettes, intérêts et
arrondis : comparer les dépôts à la seule trésorerie disponible est incorrect
lorsque des fonds sont prêtés. Un AMM avec frais ne garde pas nécessairement un
produit constant exact au fil des opérations.

SIGKILL n'est pas une coupure électrique ni un test de corruption matérielle.
Vérifier l'atomicité, la durabilité et les journaux de signature avec les états
attendus ; ne pas supposer que RocksDB peut réparer arbitrairement une corruption.

Limiter les campagnes, conserver les seeds et contre-exemples, puis arrêter les
conteneurs. Pas de flood continu ni de fonds réels.

## Sources primaires

- Assertoor : https://github.com/ethpandaops/assertoor
- Configuration : https://github.com/ethpandaops/assertoor/wiki#configuring-assertoor
- Kurtosis : https://docs.kurtosis.com/api-reference/starlark-reference/plan/

## Mesures de reprise du 26 septembre

Voir `qualification/kurtosis-crash-20260926/result.json` et `disk-usage.json`.
Le nœud tué revient avec le même conteneur, volume et préfixe finalisé ; les 24
transactions finissent au bloc 32 et le bloc 45 est finalisé par les deux nœuds.
Le compteur et le nonce passent chacun de 1 à 25. Les transactions identiques
sont rediffusées après reprise, sans hypothèse de persistance de la mempool.

Le délai observé entre réouverture RPC et réussite de la finalité est d'environ
236 secondes. Une mesure avant arrêt a trouvé environ 4,9 Go sur le premier
nœud. Ces observations ne localisent pas encore la cause : profil synthétique,
rétention des snapshots, amplification d'écriture, témoins et coût du stockage
sont à instrumenter. Ne pas déclarer une latence de finalité nominale ni un
débit soutenable à partir de ce résultat.

Chaos Mesh PodChaos/IOChaos nécessite un déploiement Kubernetes adapté. Aucune
campagne Chaos Mesh n'a été exécutée ici. L'injection utilisée est Docker
SIGKILL, et les données synthétiques ont été supprimées après archivage.

### Correction d'allocation et comparaison qualifiée (2026-09-26)

`allow_fallocate=false` dans ChainStore supprime la réservation anticipée,
sans désactiver WAL ni sync. Les trois tests ciblés de durabilité passent ;
le rejeu Kurtosis SIGKILL/24 transactions passe aussi (bloc 39 commun).
Empreinte finale : ~114/51 Mio au lieu de ~4,9 Go par nœud. Reprise RPC
~3,2 s dans les deux essais ; finalité après reprise 188 s versus 236 s,
sans attribution causale à partir d'un seul essai. Preuves :
`qualification/rocksdb-allocation-20260926/README.md`. Prochaine priorité :
interfaces Beacon pour Assertoor, décrites dans `ASSERTOOR-INTEGRATION.md`.
Le pruning et la maîtrise de l'amplification d'écriture restent ouverts.
