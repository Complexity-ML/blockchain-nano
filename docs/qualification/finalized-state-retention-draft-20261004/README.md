# Rétention historique de l’état : draft isolé, 4 octobre 2026

Branche `feature/finalized-state-retention`, révision `4734f285ee2861a4375796243fda9ad531e2bb8a`,
base figée `efbeb9bc`. Le runtime main et la candidate restent inchangés.
Activation Capella `historicalStatePruning`, désactivée par défaut.

Suppression bornée d’états EVM antérieurs à l’ancre vivante, genesis/ancre/head
conservés, checkpoint et frontière dans un même batch synchrone CAS. Les archives
signées, blocs, transactions, reçus, journaux et témoins restent conservés.
L’audit rejoue les états manquants depuis un genesis de confiance, sous la
frontière du snapshot authentifié. Le marqueur n’est pas une preuve autonome.
Voir [design et limites](DESIGN.md) et le patch exact `draft.patch`.

Syntaxe Docker ARM64 exit 0 sur six unités C++ et les tests checkpoint.
Deux erreurs de compilation antérieures restent archivées : comparaison JSON,
puis appel mutable sur store const. **Pas encore de linkage ni test exécuté.**
Trois nouveaux scénarios : chaîne Capella élaguée/rejouée/réouverte et panne
fsync avec suffixe WAL conservé/perdu, frontière/checkpoint/suppression atomiques
et nouveau commit après reprise. Trois ancres invalides et onze corruptions privées du marqueur/ancre/checkpoint
doivent être refusées. Le RPC historique doit répondre `State unavailable`
pour un état élagué et conserver le résultat exact pour latest.
La perte de suffixe est simulée, sans revendication de coupure électrique.

Le contrôleur `run.py` est actif en attente du contrôleur de reproductibilité,
lui-même après la restauration multihost. Il refuse une prerequisite échouée
ou un contrôleur disparu, ne chevauche pas les jobs Docker, configure un dossier
isolé dans le volume existant et utilise l’image épinglée sans build d’image.
Sept tests ciblés, résultats JUnit et hash des sources/programmes sont exigés.
Logs vivants : `/tmp/nano-finalized-retention-qualification-v2-20261004/`.
`queue-observation.json` est un instantané, pas une preuve de réussite terminale.

Restent : exécution/linkage, corruptions, disponibilité RPC/sync, ressources,
AMD64, sauvegarde/restauration, rétention des archives, procédures de downgrade
et toutes les exigences de production précédentes. Ne pas promouvoir ce draft
sur main avant qualification. Aucun fonds ou déploiement public.

La file initiale a été arrêtée uniquement pendant l’attente, avant tout build,
pour compléter ces cas. Son code/état et la révision initiale sont conservés
sous `initial-*` ; `superseded-before-build.json` consigne le PID et le motif.
La file v2 est active ; aucune réussite fonctionnelle encore déclarée.
