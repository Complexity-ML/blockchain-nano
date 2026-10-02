# Record JSON canonique réutilisé après validation complète — candidat f9a3ee1a

## Échec parent non résolu

2d2764de AMD64 sélection terminale **14/15**, zéro skip, **1 896,31 s**. Source/huit programmes/sept entrées statiques inchangés, registre et cas exacts, délais originaux. Activation 346,72 s, restart 350,86 s, auto-registration 347,50 s ; seul échec réindexation **360,05 s**, dernier marqueur slot 152/finalité 17/six nouveaux votes. Le changement hex n'a pas fermé cet écart. Le contrôleur WAN 78086, encore en attente, est arrêté sur son gate 15/15 impossible : aucun nœud WAN n'a été lancé, aucune campagne C++ active arrêtée. Régression ARM64 continue sur source 2d2764de inchangée.

## Nouveau périmètre de code

Candidat Git isolé f9a3ee1a, parent 2d2764de ; aucune promotion runtime main. Le memo privé conserve en plus le dump canonique de chaque ligne. Réutilisation seulement après **égalité du record JSON complet**, justification et statut d'ancre, profil identique ; comparaison SSZ de tous les octets conservée. Le dump n'est jamais fourni par un pair ni sérialisé dans le checkpoint. Map<Root,Json>/layout du cache inchangés, aucune modification d'ABI ; tuple interne quatre éléments au lieu de trois.

L'objet final est assemblé dans l'ordre canonique des clés, toutes valeurs hors blocks utilisent leur `.dump()` ; blocks emploie les seuls dumps prévalidés. Limite d'enveloppe vérifiée avant chaque ajout, annulation par tranches 4096 octets. Checkpoint v1-v3 et validations froides/invalidation/profil/pruning inchangés. String views valides pendant l'invocation, les autres inserts map n'invalident pas les entrées et aucune ligne n'est réécrite après capture. **Un dump supplémentaire par record augmente la mémoire du memo** ; RSS et effet sur le nœud restent à mesurer. JSON d'exécution/receipts/deposit-history est toujours reconstruit et comparé, aucun raccourci par racine/pointeur.

## Preuves acquises et limites

Helper Docker ARM64 : **769 comparaisons exactes**, **768 refus d'enveloppe**, **384 annulations**, schémas v1-v3, Unicode/escapes, nested blocks et formats invalides. Sources/helper inchangés. Compilation directe `g++ -std=c++20 -O2 -Ithird_party tests/test_capella_snapshot_json.cpp -o /build/checkpoint-row-dump-arm-20261002/helper`, timeout60, image ARM existante, réseau none/root readonly, un CPU/1 GiB. Benchmark cinq fenêtres alternées, médiane **4,52×** pour assemblage de 32 lignes 64 KiB, comparaison des sorties incluse. **Pas de gain end-to-end déduit**.

Intégration directe ARM64 : nouveau module snapshot et test recompilés contre core/libs **2d2764de readonly**, sept entrées statiques/binaire/sources inchangés. Trois variantes signées locales passent (native Capella, migration Bellatrix→Capella, slots sautés). Comparaison cold/warm byte-identique et restauration, désormais sept catégories de mutation dont cinq nouvelles : header exécution, receipts, époques realized/unrealized, justification ; SSZ/processed-epoch et corruption de state conservées. **Lien partiel, pas build CMake complet, pas EVM full, pas réseau ni réindexation AMD64**. Aucun délai de scénario augmenté.

## Audit arrêté AMD64 du parent

Premier contrôleur s'arrête **avant Nano** : grep du chemin nano capture aussi les autres exécutables préfixés nano, cmp refuse. Sources/base unchanged, preuves conservées. Sélecteur corrigé à égalité exacte du chemin via awk ; aucun rejeu parent répété. Deuxième tentative : **4/4**, histoire signée **99 blocs / 97 transactions**, checkpoint externe exact (epoch0xc/root af6a0f3a...), digest/sequence1147 égaux, head attendu ; trois refus exacts. Base/source/binaire inchangés, binaire identique à la sélection parent. Ce résultat ne transforme pas 14/15 en réussite et **ne démontre ni reprise des duties ni nouvelle observation WAN**. Même fixture historique que l'audit natif Mac, votes autonomes/fork-choice complet toujours non qualifiés.

Tous échecs antérieurs, gate architectures/régression, restauration WAN, EVM activée, lancement/bridge et revue indépendante restent ouverts. Prochaine porte : compilation/qualification cohérentes de f9a3ee1a sur les deux architectures et cas de réindexation original, sans changer le délai.
