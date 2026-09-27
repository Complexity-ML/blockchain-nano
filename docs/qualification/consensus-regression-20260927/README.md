# Régression consensus et corpus officiels — 27 septembre 2026

**185 réussites, 3 échecs, 0 ignoré, sur 188 tests sélectionnés.**
Durée réelle CTest : 2763,33 s. Code de sortie : 8. Cette campagne n’est pas verte.

Binaire de référence : `af7f99e59504709f8ae3e3b5ed8c4951e46f48d0`.
Les exécutables sélectionnés et `nano` ont été recompilés ; `nano-p2p` a été
vérifié par son build séparé. `manifest.json` contient les SHA-256 des binaires.
Ils sont restés inchangés pendant la campagne. Les correctifs du banc d’essai
ont été préparés à part et ne sont pas inclus dans ce verdict initial.

```sh
ctest --test-dir build-release -R '^(ethereum_official_|consensus_)' -j 2 --output-on-failure --output-junit /private/tmp/nano-consensus-regression.xml
```

| Échec | Durée | Observation et suite |
| --- | --- | --- |
| consensus_bellatrix_partition_recovery | 516,394 s | Finalité 2 / justification 6 ; drainage conditionnel des traces corrigé, rejeu requis |
| consensus_bellatrix_partition_sustained | 517,351 s | Même prédicat de reprise en échec, avant la phase de stabilité ; rejeu requis |
| consensus_bellatrix_assertoor_finality | 296,015 s | Base Assertoor : table task_logs absente ; finalité native commune 4 ; configuration corrigée, rejeu requis |

Les défauts de banc d’essai ont leurs contre-exemples indépendants :
[drainage des sorties](../bellatrix-partition-trace-drain-20260927/README.md),
[pool de connexions Assertoor](../assertoor-result-database-20260927/README.md).
Leur reproduction ne transforme pas les trois échecs en réussites : les rejeux
réels doivent encore passer.

Le test `consensus_bellatrix_crash_sustained` passe en 325,44 s. Les autres
résultats individuels et sorties se trouvent dans `results.xml` et `tests.log.gz`.
Les journaux de build, l’inventaire initial et les empreintes des artefacts sont
conservés dans ce répertoire.

## Limites

La sélection couvre 188 des 389 tests CTest enregistrés au lancement ; les
201 autres ne sont pas exécutés ici. La liste exacte est dans
[le relevé de périmètre](../bellatrix-partition-trace-drain-20260927/campaign-scope.json).
Les nouveaux tests ajoutés pendant le diagnostic ne font pas partie de ces 389.
Aucune campagne WAN, aucun déploiement VPS, aucune ouverture aux fonds réels.
Cette régression ne clôture pas la conformité de chaque EIP ni la production.

## Résultat ciblé ultérieur

Le rejeu de `consensus_bellatrix_partition_recovery` avec le drainage corrigé
passe : finalité commune d’époque 6, retrouvée en 95 s après la reconnexion.
Voir le rapport de drainage lié ci-dessus. Le bilan initial 185/188 reste
inchangé ; les deux autres rejeux sont encore attendus.

Le rejeu `consensus_bellatrix_assertoor_finality` passe aussi en 198,32 s avec
la base de résultats corrigée et les seuils inchangés. Quatre tâches réussies
et correspondance native Beacon/exécution confirmée sur les deux nœuds. La
partition prolongée reste à vérifier.

La variante `consensus_bellatrix_partition_sustained` passe enfin en 565,85 s :
reprise commune d’époque 7 en 144 s, puis trois avancées communes jusqu’à
l’époque 10. **Les trois échecs initiaux ont chacun un rejeu réussi**, après
les deux corrections du banc. Il ne s’agit pas d’une nouvelle campagne unique
188/188 : conserver le bilan initial et les trois preuves ciblées séparément.
Aucun code de consensus/exécution ni critère de succès n’a été modifié ici.
