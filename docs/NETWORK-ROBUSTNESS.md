# Vérifications du daemon P2P

Le daemon utilise cpp-libp2p avec TCP et Noise. Les protections ci-dessous
appartiennent à l’adaptateur Nano ; elles ne constituent pas une certification
de sécurité de la bibliothèque ou du réseau public.

- Le statut d’un pair doit fournir le même chain ID et le même hash de genesis
  que le cœur local, avec types et largeurs valides.
- Un message JSON est limité à 16 Mio et 64 niveaux. Le repérage de sa fin est
  progressif ; l’analyse JSON complète n’est effectuée qu’une fois.
- Lecture et écriture de message expirent après 10 secondes. Les échanges
  requête/réponse expirent après 30 secondes, ouverture de flux comprise. Un
  flux établi après cette expiration est immédiatement réinitialisé.
- Un seul sondage de statut est en cours par pair. Un callback tardif ne peut
  pas exécuter une seconde fois la fin de l’échange.
- Les flux entrants sont limités à 4 par pair et 16 au total. Les budgets de
  requêtes sont de 16 en rafale/8 par seconde par pair et de 128 en rafale/64
  par seconde globalement. Le quota est conservé jusqu’à la fin du traitement
  asynchrone ; les demandes excédentaires sont refusées sans file d’attente.
- La table de quotas contient au plus 256 identités. Les entrées sans opération
  active depuis 60 secondes peuvent être évincées quand la table est pleine.

Les callbacks sont explicitement retirés de leur emplacement avant invocation.
Déplacer une `std::function` ne suffit pas à établir l’invariant « callback
consommé » de manière portable. Ce retrait concerne les opérations JSON, les
échanges chronométrés, le RPC local et la résolution des bootnodes.

## Tests disponibles

Le build `build-network` contient `bootstrap`, `json_frame`, `peer_status`,
`inbound_budget`, `timed_reply` et `json_stream`. Le dernier utilise les classes
réelles de lecture et d’écriture Nano avec un flux de test : un timeout entraîne
un reset, puis le flux livre l’erreur d’annulation de manière différée. Chaque
callback utilisateur doit être invoqué une seule fois.

Les suites `network`, `signed_network` et `supervisor` du build principal
vérifient les échanges entre daemons réels et la gestion de leurs processus.
Elles ne constituent pas un essai de charge ni un test distribué de partitions.

## Limites restantes

Les quotas s’appliquent aux flux applicatifs entrants après identification du
pair. Les connexions TCP, les négociations cryptographiques, les octets par
seconde et les annonces sortantes ne sont pas couverts par ces quotas. Le délai
d’échange termine l’attente applicative, sans démontrer que toute opération
interne de connexion de cpp-libp2p est immédiatement libérée. Les attaques avec
multiples identités, l’équité entre pairs, la saturation et les partitions
prolongées restent à éprouver. Le protocole de finalité distribué n’est pas
encore activé.
