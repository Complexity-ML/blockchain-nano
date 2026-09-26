# Réutilisation bornée des pages de votes inconnus

À la suite de la campagne WAN `80493b7`, le relais conserve le reste d’une
page validée par pair/session (au plus 32 entrées au total), après un vote
exporté. Le tour suivant peut réutiliser ce reste seulement si le curseur
a avancé jusqu’au vote précédent. Il exporte toujours chaque nouveau vote
depuis le cœur durable et exige son propre accusé de réception.

Le cache ne contient pas les payloads. Une perte d’accusé conserve le curseur
antérieur et force le rafraîchissement au tour suivant ; un export devenu
indisponible ne peut pas être envoyé ou acquitté. Le prochain inventaire
détecte les changements de catalogue. Une ancienne page encore disponible
peut être exportée avant cette détection, mais chaque objet doit toujours
passer l’admission distante normale. Le cache ne vaut pas preuve de validité.

## Qualification

Nouveau scénario : 32 votes inconnus, **1 requête d’inventaire**, 32 exports
et 32 accusés distincts (32 requêtes d’inventaire auparavant). L’ancien
header, compilé avec le nouveau test, échoue précisément sur ce contrôle ;
le header corrigé est ensuite restauré et recompilé.

Nouveaux cas : perte d’accusé au milieu de la page ; payload indisponible
après inventaire, puis rotation de catalogue et reprise sans réacquitter
l’entrée déjà connue. Les contrôles existants couvrent aussi rejet différé,
pages malformées, cache borné, pairs indépendants, timeout et accusé tardif.

`ctest --test-dir build-release -R 'consensus_relay_(rejected_vote_progress|peer_concurrency)$' --output-on-failure` : 2/2 passent.
`cmake --build build-network --target nano-p2p -j 4` : réussi.

Politique de transport locale uniquement. Aucun seuil de finalité modifié,
aucun profil de consensus activé. Le gain de requêtes est démontré par test
injecté ; le gain de temps WAN et la reprise sous 180 s restent à qualifier.
