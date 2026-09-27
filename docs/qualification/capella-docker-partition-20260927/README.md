# Capella : partition Docker avec daemons actifs — 27 septembre 2026

**PASS, une campagne complète** sur deux conteneurs Linux locaux, avec 32 validateurs synthétiques distincts chacun. Le réseau Docker dédié est interne ; aucun port n’est publié sur le Mac, aucun VPS n’est modifié et aucun fonds réel n’est utilisé.

## Défaut de démarrage corrigé

L’image initiale échouait avec `Nano P2P: Invalid supervisor PID` : le cœur était légitimement PID 1 dans son conteneur. Le parseur `managed_supervisor` autorise désormais ce cas sous Linux, vérifie toujours le parent réel et rejette les valeurs invalides, débordements et changements de parent. La politique hors Linux conserve l’exclusion de PID 1. La surveillance du parent en cours d’exécution est inchangée.

Le noyau termine les processus restants lorsque l’init d’un namespace PID disparaît : [documentation Linux pid_namespaces(7)](https://man7.org/linux/man-pages/man7/pid_namespaces.7.html). Le test utilise le démarrage direct de l’image, sans contourner le défaut avec `--init`.

Les tests du parseur et le rattrapage natif avec accusé perdu passent. L’image reconstruite démarre les deux cœurs et daemons et réalise la campagne ci-dessous.

## Scénario et observations

1. Exiger une finalité commune d’époque au moins 2, confirmée par les en-têtes finalisés pendant trois observations.
2. Déconnecter le seul réseau du second conteneur pendant au moins 96 secondes. Contrôler à chaque observation que le conteneur reste actif, sans réseau attaché, et que les deux processus P2P ne sont ni stoppés ni zombies. Les clés restent séparées ; aucun validateur n’est suspendu.
3. Conserver l’historique des checkpoints : refuser les régressions par nœud et toute racine contradictoire observée pour une même époque, même à des instants différents.
4. Exiger des branches distinctes ayant progressé et une observation sans pairs. Restaurer la même adresse IP.
5. Exiger une nouvelle finalité commune au-delà de la période isolée, des pairs connectés et trois confirmations d’en-têtes finalisés en 240 secondes maximum.

Résultats :

- Fenêtre mesurée entre les événements de coupure et de restauration : **97,801 secondes**, pour 96 secondes demandées.
- **43 observations** pendant l’isolement ; finalité maximale observée : époque 2. Zéro pair observé des deux côtés, branches distinctes.
- Seuil de récupération : époque 6. Résultat : **époque 7 commune**, root `0x2c1d051d76ca3a551a865f4967cd86f73ada19ed37466f0ae1ea684e27f15af5`.
- Récupération avec trois confirmations : **142,223 secondes après restauration**.
- Sortie du harnais : 0. Les deux conteneurs, leurs volumes et le réseau dédiés ont été supprimés ; une inspection indépendante confirme l’absence de ressources portant le label de cette campagne.

## Traçabilité

`qualified/` conserve les événements, le résultat et les journaux des deux nœuds. `fixture/` contient le genesis, le profil, le snapshot initial et les configurations exactes. **Les clés exportées sont des clés privées synthétiques connues publiquement (entiers 1 à 64) ; ne jamais financer ces comptes.** Les bases finales ne sont pas archivées.

`startup-pid1-failed/` conserve l’échec réel du démarrage avant correction. `startup-header-failed/` conserve un second échec du harnais : l’en-tête du bloc initial non signé n’était pas encore disponible. Le harnais attend désormais un premier en-tête signé pendant une durée bornée, exclusivement au démarrage ; les erreurs HTTP pendant la partition et la reprise restent fatales. Ces deux essais n’ont pas atteint la partition.

L’image testée est `sha256:07b7c6a20ff2496543eacf31ada597a1c1a2f279a3c425be96df54869f0e0283`, construite à partir du commit de base indiqué et du correctif alors non committé, avec label explicite `org.nanobees.source-dirty=true`. Le manifest identifie cette image, les sources et l’exporteur hôte ; `source.patch` s’applique avec `git apply --unidiff-zero`. Les hashes hôte ont été capturés pendant la campagne puis revérifiés, pas utilisés comme affirmation que l’image est celle d’un checkout propre.

La fixture augmente uniquement les budgets opérationnels de rétention pour cette campagne plus longue. Aucun seuil de quorum ni règle de consensus n’est modifié.

## Limites et suite

Une partition symétrique entre deux nœuds sur un seul hôte Docker, sans transactions applicatives injectées. Les observations RPC ne sont pas une preuve formelle de sûreté byzantine. Restent le WAN, les partitions asymétriques, les pertes et latences variables, les comportements malveillants, la charge applicative et la grande échelle. La qualification complète de production reste ouverte.
