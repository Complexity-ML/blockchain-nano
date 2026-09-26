# Assertoor — observation native de finalité, 26 septembre 2026

## Diagnostic conservé avant correction de la sonde

La campagne de 240,88 s échoue dans la sonde Node, **après** réussite des quatre
 tâches du binaire officiel Assertoor (voir `before-probe-fix-runs.json` et
`before-probe-fix-run-1.json`). La lecture de bloc demandait implicitement
`Accept: */*`, obtenait du SSZ, puis tentait un décodage JSON. Les lectures
JSON explicites corrigent cette erreur de banc d’essai.

Les bases synthétiques ont été arrêtées puis copiées. L’exporteur a vérifié les
engagements de fermeture indépendants avant lecture. Le résumé conservé montre
un checkpoint finalisé commun époque 2, mais un retard du second nœud : blocs
stockés jusqu’aux slots 41 et 35. Les captures REST antérieures à l’arrêt
montrent encore une finalité de tête 2/0 ; le checkpoint du Store peut avancer
avant celui du dernier BeaconState reçu. La tâche Assertoor utilise un cache
partagé : son succès seul ne prouve pas la convergence des deux vues.

## Correction

La sonde demande `application/json` pour chaque lecture REST. Après Assertoor,
elle exige sur **chacun** des deux nœuds une finalité de tête au moins égale à 2,
le bloc Beacon correspondant et le même hash d’exécution que le tag RPC
`finalized`. Elle attend au maximum 30 s leur convergence, conserve chaque
tentative et échoue si les observations diffèrent. Les quatre tâches et leur
seuil de finalité restent inchangés. Les diagnostics ne transforment jamais un
échec en succès.

Les bases, clés publiques de fixtures et journaux complets restent en stockage
temporaire local ; ils ne sont pas incorporés dans Git. Aucun fonds réel.
Les précédents échecs intermittents de progression restent ouverts.

## Répétition après correction de représentation

Échec en 276,69 s : la tâche de finalité Assertoor expire et les deux reçus
natifs restent à l’époque finalisée 0, justifiée 2. Les blocs JSON sont maintenant
lisibles. La divergence commence après le slot 22. Le rejeu hors réseau des
archives du pair accepte tous les blocs manquants : 8 côté validateur et 6 côté
observateur, vers une union de 51 blocs. Il ne modifie pas les bases source et ne
prouve pas leur livraison en temps réel. La propagation reste à corriger.

## Critère prolongé ajouté

`consensus_bellatrix_assertoor_sustained` exige quatre avancées consécutives de
checkpoint commun puis répète la campagne après un redémarrage ordonné. Le
moniteur rejette les racines finalisées contradictoires, la régression de
finalité, un saut d’époque qui masquerait une observation manquée, une tête
immobile pendant trois slots et une finalité commune immobile pendant deux
époques. Chaque nouveau checkpoint est lié à son bloc canonique d’exécution.
Le réseau reste local, deux nœuds, 64 clés synthétiques réparties 32/32.

Six tests déterministes valident le moniteur, dont cinq refus de scénarios
incorrects. **Ils ne valident pas le réseau.** La campagne prolongée est désormais réussie sur la fenêtre ci-dessous ; les
partitions/pannes brutales restent des qualifications séparées.

## Mesures et correction du coût de publication

La campagne instrumentée échoue en 277,10 s. Les traces montrent des relais
acceptés prenant jusqu’à 10 590 ms, sans blocage définitif du dispatcher. Les
inventaires vides eux-mêmes attendent parfois plus de deux secondes. Le
profilage de 5 s d’une copie synthétique arrêtée puis reprise montre les
revérifications historiques dans `tick -> publish -> durable_snapshot ->
bellatrix_store_snapshot`. Le cœur de `build-cmake` était compilé **sans
optimisation** ; ces durées ne qualifient pas les performances de production.

Un cache privé de validations par enregistrement est ajouté à la publication.
Il compare le contenu complet sérialisé, les métadonnées de justification et
le rôle d’ancre, sous le même profil. Tout changement force une nouvelle
validation. Les contrôles globaux, les limites, le WAL synchrone, la validation
au chargement et le contenu du checkpoint persistent. Ce cache n’est jamais
accepté depuis le réseau ni sauvegardé comme autorité.

`release-cache-tests.log` : trois cibles réussies en 9,48 s. Elles comprennent
les six scénarios du moniteur, la reprise et les mutations d’état/exécution/
checkpoint refusées malgré un cache chaud. La sérialisation reste identique.
Les binaires et flags `-O3 -DNDEBUG` sont consignés dans `release-binaries.json`.
Ce changement et l’optimisation de compilation sont deux facteurs distincts :
un gain de cadence global ne sera pas attribué au seul cache.

## Résultat de la campagne prolongée Release

**Réussite en 616,36 s** (`release-sustained.log`), sans répétition automatique
pour ignorer les échecs. Deux nœuds, deux relais P2P, 64 clés synthétiques,
aucun fonds réel ni VPS.

| Fenêtre | Finalité initiale → finale | Avancées consécutives | Durée du moniteur | Observations |
| --- | --- | --- | --- | --- |
| Avant redémarrage | 2 → 6 | 4 | 193,443 s | 33 |
| Après redémarrage ordonné | 7 → 11 | 4 | 183,187 s | 31 |

Les quatre tâches officielles Assertoor passent dans chacune des deux phases.
Les 64 observations du moniteur ont un écart nul de slot de tête et d’époque
finalisée entre les deux nœuds. Chaque avancée est liée au même bloc canonique
d’exécution. Ce sont des observations périodiques, pas une preuve d’absence de
toute divergence transitoire entre deux mesures. Les journaux, résultats et
échantillons sont dans `before-restart/` et `after-restart/`.

Cette réussite qualifie cette fenêtre locale en Release avec le cache. Elle ne
prouve ni une tenue indéfinie, ni la production publique, ni les partitions,
SIGKILL, saturation, grandes populations ou le pruning de l’historique. Les
échecs précédents sans optimisation sont conservés, et le gain ne peut être
attribué au seul cache sans comparaison séparée.

La garde explicite de taille SSZ (au moins huit octets avant lecture du temps
Genesis) est ajoutée après cette campagne et contrôlée séparément par un test
de mutation tronquée. Elle ne change pas le traitement des états valides ; les
empreintes de la campagne et du build final sont conservées séparément.


Vérification finale après la garde SSZ : **4/4 cibles réussies en 9,59 s**
(`final-regression.log`), incluant quatre mutations refusées avec cache chaud,
les annulations avant commit, l’erreur de synchronisation disque, la reprise,
le progrès des votes après refus/perte de réponse et les six cas du moniteur.
Les empreintes finales figurent dans `final-binaries.json`. Aucun déploiement
VPS ni push Git n’est effectué par cette campagne.
