# Proposition publique : budget steady de démarrage Capella

Quatre chemins seulement : runtime CPP, helper privé nouveau, CTest nouveau et CMake. Le header public `consensus_capella_runtime.hpp`, signature, clock/replay/epoch/floors, contrôleur et templates demeurent inchangés. Baseline/SHA dans `scope.json` et `protected-source-bindings.json`. Aucune compilation, exécution, import au dépôt ou qualification réelle par l’auteur.

`clock.maximumStartupElapsedMs` est optionnel et canonique positif, représentable en steady-duration. Absent : pas de nouveau plafond implicite, même invocation des callbacks ordinaires (la vérification de shape clock invalide est désormais plus précoce). Présent : mesure depuis l’entrée runtime, contrôlée avant/après chaque callback startup et à `finish()` avant workers. Comparaison sans soustraction signed overflow, fin explicite/RAII, wrapper possédé via shared_ptr ; après fermeture, les copies deviennent inertes, sans appeler le callback startup. Exception typée séparée, aucune conversion générale d’erreur.

La lecture CLI de config et construction préalable de Blockchain restent hors du budget runtime ; les I/O sont coopératives, watchdog ROOT requis. Aucun grant de fenêtre de panne ni valeur opérationnelle/template sélectionné. Les seuls budgets choisis sont des valeurs de test. La politique autorité/controller demeure2h/60, jamais élargie ni contournée.

## CTest proposé, pas encore exécuté

`consensus_capella_startup_deadline --self-test` : cinq groupes + résumé,70s CTest,60s/128MiB internes. Il charge uniquement `tests/fixtures/capella-bootstrap-anchor-cb532` (manifest public ec150c28…, wire public9575c3a…). Réutilise l’authentification complète déjà implémentée, zéro nouvelle signature/clé/environment/network ; UTC historique constant uniquement dans la fixture de test, pas une mesure de reprise actuelle.

1. Frontières/helperoverflow/regress, encodages runtime invalides avant callback/file I/O, absencelegacy.
2. Budget/config possédés, catégorie cancellation extérieure, mauvaisprofilrefusé.
3. Vraie échéance steady au runtime après latence de callback ; aucun handle retourné, stores logiquement inchangés avantpublication.
4. Vraie publication de préfixe par startup_catch_up avec le même helper et un steady test déterministe, oracle canonique, PRIMARY/WITNESS conservés, ancien pin refusé et réouverture actuelle. Pas de qualification des vrais journaux/floors privés.
5. Copies du callback après finish/destruction, deux retours runtime réellement observés (legacy/budgeté), coldlease/publicread/clock après dépassement réel du budget d’origine. Aucun compteur de workers/providers inventé : ces fixtures n’ont pas de validator configuré.

Root devra relier aussi le test startup existant au nouveau cœur et requalifier ses six cas de raccord ; pas de répétition des dixCAS ni du pending inchangé. Le compte des objets frais/réutilisés est à dériver des MD/flags réels. Une expiration aprèspublication exige garder les nouveaux engagements, pas restaurer les anciens pins. La longue panne, UTC réel, duties/finalité, copiesprivées et production restent ouvertes.

## V2 : readers hors durée de vie RW

Le SDK local public `db.h:195–199` documente undefined pour OpenForReadOnly simultané à OpenRW. Les trois comparaisons de rows du test V1 étaient donc impropres : invalidconfiguration, cancellation extérieure et vraie échéance. V2 initialise puis ferme l’owner, observe rowsRO, ouvre ReadWriteExisting pour le runtime, ferme cet owner puis observe rowsRO. Aucun readerRO ne chevauche ces owners. Les autres groupes gardent leurs lectures par le ChainStore existant ou après fermeture. Le headerpublic, runtime8e6, helpera585, CM011a et registration sont exactement V1. V1 n’a pas été compilée/exécutée ; elle reste immuable. Aucune nouvelle exécution.
