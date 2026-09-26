# Relais sortant indépendant par pair

Le relais précédent attendait une seule tentative globale, même lorsqu’un
second pair pouvait recevoir des votes. Le réseau conserve désormais une
permission par identité de pair, avec quatre tentatives simultanées au maximum
par défaut. `NANO_CONSENSUS_RELAY_CONCURRENCY` configure ce budget local
(1..1024). Il ne s’agit ni d’un plafond de validateurs ni d’une règle de consensus.
La valeur 1 permet le contrôle sériel. Les budgets d’entrée restent inchangés.

Le parcours round-robin saute les pairs occupés. Chaque pair conserve son
ordre blocs/votes/preuves/sorties, son curseur et son cache d’acquittements.
Une déconnexion ne libère pas prématurément sa permission : le résultat ou le
timeout la libère. L’identité du pair est ajoutée aux traces de tentative.

Test `nano-peer-relay-concurrency-tests` : vraies instances AltairArchiveRelay
avec transport injecté et réponse suspendue. Le contrôle à capacité 1 bloque
le second pair ; à capacité 2, ce dernier termine avant toute réponse du premier.
Sont aussi vérifiés la saturation, la reprise d’un troisième pair, l’isolation
des acquittements, le timeout et une réponse tardive après réutilisation de
l’identité. Pas de seuil de performance machine. Test réussi et compilation
`cmake --build build-network --target nano-p2p -j 2` réussie.

Ce test ne mesure pas les RPC réels, la contention du cœur ni la reprise WAN.
Les trois échecs WAN précédents sont conservés. Une nouvelle campagne aux
mêmes critères est nécessaire avant de qualifier cette correction en réseau.
