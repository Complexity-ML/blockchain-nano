# Cache de résolution des clés locales Capella

## Changement et frontière de confiance

Cache privé à une seule entrée dans `CapellaDurableReactor`, lié à la racine
SSZ de la tête admise et à l'ensemble exact des clés demandées. Il ne peut
être rempli depuis une entrée réseau non admise. La lecture de tête, le lookup
et la publication du résultat sont sérialisés sous le mutex existant ; les
règles de décodage du reactor sont immuables. Un cache froid authentifie encore
le SSZ complet avec le helper du commit c2f1568.

Le cache ne conserve que les clés demandées et leurs indices, aucun snapshot.
Un nouveau root ou un changement des clés impose un recalcul. Les absences sont
mises en cache uniquement pour cette racine. Un résultat complet est construit
avant publication ; l'annulation reste vérifiée sur une lecture à chaud.
Le journal et le contrôle d'identité du devoir avant signature sont inchangés.

## Nouvelle couverture

Le scénario `consensus_capella_execution_owner_reorg` ajoute des assertions de
cache au reactor réellement lié à la blockchain : deux bascules de branche,
trois lectures froides dues aux racines et sept lectures chaudes, avec
comparaison des points de contrôle pour prouver que le parcours SSZ est évité.
Les index exacts de deux clés et l'absence d'une troisième sont contrôlés.
Chaque changement d'ensemble de clés retourne seulement le sous-ensemble
attendu. Une annulation sur un cache chaud est propagée.

Ces branches gardent les mêmes clés aux mêmes indices. Le test prouve le
recalcul au changement de racine ; il ne simule pas deux dépôts concurrents
attribuant des indices différents à une même clé.

## Limites

Le cache évite les recalculs entre deux changements d'état. Il ne réduit pas
le coût froid de 1,43 s mesuré précédemment à un million d'entrées, et la
sélection de tête reste exécutée à chaque appel. Aucun débit réseau ou gain
chiffré global revendiqué. Les copies sont proportionnelles aux clés locales.

Tests locaux, pas de déploiement VPS ni réseau à grande échelle. Les crashes
au commit, la disponibilité pendant une réorganisation concurrente avec une
signature et le runtime à un million de validateurs restent ouverts.
Aucun EIP ni profil réseau ni objectif production clôturé.

## Résultat final

**3/3 tests passent en 163,77 s** de temps réel, avec deux processus concurrents.
Outre le scénario enrichi de réorganisation, le runtime configuré et le parcours
de dépôt/activation passent : clé absente attendue, dépôt consommé une fois au
slot 80, activation époque 17, trois attestations de la nouvelle clé, poursuite
jusqu'au slot 161 et réouverture des deux propriétaires. Ce rejeu vérifie que
l'absence mise en cache ne bloque pas la découverte d'un nouveau validateur.
Les empreintes, logs et JUnit sont conservés ; aucune campagne globale rejouée.
