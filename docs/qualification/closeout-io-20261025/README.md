# Clôture : checkpoints I/O et erreurs primaires

Les changements sont qualifiés dans un Docker AMD64 existant, sans réseau, nouvelle image, acteur Nano ou donnée de campagne. Le runtime est `f943a69b290502facc26ff9588c68d72163f08d515564c1bf75d71a1bae4b5a9` ; la candidate finale est `21a92877e14c37f46d38eda9c8ea62687af801f5`.

`observe`, `save` et `fence` vérifient le budget original et conservent le signal/phase d’entrée. Une ancienne I/O WORK ne peut pas emprunter le signal neuf de clôture. Une nouvelle I/O terminale peut publier après la première interruption de travail, dont le verdict demeure FAILED ; une deuxième interruption de clôture reste bloquante. Le premier `fstat` est désormais dans le `try/finally` : son refus tente aussi la fermeture. Une erreur de fermeture ne masque plus la cause primaire. L’admission d’une nouvelle autorité n’a lieu qu’après fermeture et contrôle, sans remplacer une autorité conservée.

Les deux rapports prospectifs de travail terminal dénombrent les étapes réellement présentes. Ils sont **non admissifs** : `completeRequiredReserveMs:null`, `admissionComplete:false` et `reserveSufficiencyQualified:false`. Les attentes physiques d’E/S/OS, la croissance des collections et la RAM ne sont pas artificiellement déclarées bornées. Les anciens refus d’interruption de `captureOwner` et `lifecycleCheck` sont inchangés : la capture terminale complète après SIGINT reste ouverte.

| Lot conservé | Terminal réel | Résultat et portée |
|---|---|---|
| [Source01](source-01-no-go.md) | Non exécuté | NO_GO de revue : nouveau garde empêchant les I/O terminales après interruption. Corrigé avant qualification runtime. |
| [Root01](root-01-result-public.json) | Docker exit1 | [41 groupes](root-01-stdout.log) : 31PASS, 5FAIL, 5SKIP. Les 12 nouveaux groupes I/O, six gardes et un contrôle de complétude des dépendances passent. Cinq fixtures terminales omettent `specSha` et le vrai template Docker ; FAILED conservé. |
| [Root02](root-02-result-public.json) | Docker exit0 | [17/17 groupes terminal](root-02-stdout.log), sans skip, après correction de leur contexte partagé. Runtime inchangé ; les assertions originales restent présentes et les raccordements sont vérifiés. |

La [réutilisation de périmètre](scope-reuse-public.json) vérifie que le seul input modifié entre les lots est `terminal-evidence.test.mjs`. Les 19 groupes indépendants déjà verts sont conservés à leurs octets ; les 17 scénarios utilisant la fixture commune ont tous été rejoués, y compris les douze précédemment verts. Le total de 36 groupes distincts n’est pas un nouveau passage monolithique de 36 tests. Chaque contrôleur a suivi et fermé ses 241 FD de sources/fournisseurs, avec fences finales strictes, Docker arrêté et aucune nouvelle image. Node18.19.1 et Python3.12.3 existants étaient montés en lecture seule ; la fermeture intégrale de leurs ressources système n’est pas revendiquée.

Les nouveaux tests extraient les vraies fonctions et injectent des I/O, horloges, erreurs et signaux de fixture. Ils ne prouvent pas des bornes physiques d’E/S, une restauration d’acteurs réels ni une nouvelle finalité inter-hôtes. L’annulation du Core est qualifiée séparément. La campagne04 demeure signée, terminée et FAILED ; aucune nouvelle campagne n’est lancée. L’audit indépendant, la CI bloquée et les autres critères de production restent ouverts.

Le booléen agrégé historique `runtimeAndSourceFilesStrictlyFenced:false` de Root01 demeure inchangé : il dépendait du verdict global du lot, déjà FAILED pour les tests. L’absence de refus de fence est attestée séparément par le code du contrôleur, ses `secondaryFailures:[]` et les 241 fermetures ; ce champ agrégé n’est pas repeint en true.
