# Rétention des états d'exécution finalisés

Statut : implémentation isolée, syntaxe Docker ARM64 vérifiée seulement.
Base : efbeb9bca3d42e41edb9cb81afba98c0b953a90f. Aucun runtime main, volume
historique ou fixture de qualification modifié. Désactivé par défaut.

`historicalStatePruning: true` dans le profil Capella autorise le retrait des
états EVM strictement antérieurs à l'ancre authentifiée du fork store vivant.
Le genesis reste disponible. Le pruning du fork store demeure contrôlé
séparément par `automaticPruning` ; le paramètre historique seul ne fait pas
progresser cette ancre.

Le coordinateur fournit l'ancre du candidat validé. Le stockage vérifie son
hash et sa racine d'état et refuse tout recul de la rétention ou remplacement
canonique sous l'ancre déjà retenue. La suppression est bornée par le budget
de blocs du checkpoint ; sa frontière et le checkpoint sont écrits dans le
même WriteBatch synchrone protégé par les CAS existants. Un batch trop grand,
un état absent hors frontière ou une ancre invalide est refusé avant I/O.
Aucune suppression de bloc, transaction, reçu, archive signée, journal ou témoin.

Le marqueur versionné n'est pas une preuve de consensus autonome. Au redémarrage,
il doit rester en deçà de l'ancre du snapshot Capella authentifié. L'audit depuis
un genesis de confiance rejoue les archives signées et recalcule les racines
EVM, même lorsque les états intermédiaires ne sont plus matérialisés ; il refuse
une absence hors frontière, vérifie l'ancre sur le rejeu signé et conserve
l'absence de qualification exhaustive des votes historiques/fork-choice.
Les RPC historiques gardent leur erreur `State unavailable` pour ces états,
sans fallback vers latest. Archives signées/index/receipts demeurent conservés.

Nouveau scénario `consensus_capella_historical_state_pruning` : deux propriétaires,
finalité et signatures, avancement d'ancre, réouverture, disparition d'états
anciens avec genesis/ancre/head conservés, rejeu signé des états élagués,
et refus d'une ancre altérée, régressive ou future sans changement de token.
L'audit utilise une ouverture séparée en lecture seule. Un second scénario
`storage_historical_state_pruning_sync` injecte une panne de fsync : conservation
ou perte simulée du suffixe WAL doivent garder marqueur, suppression et checkpoint
atomiques, préserver genesis/head/archives/reçus et permettre un commit suivant.
Cela ne reproduit pas une coupure électrique physique. Le scénario est ajouté
mais **pas encore exécuté**. Les états existants restent tous conservés quand
le paramètre est faux/absent. Une réouverture avec l'ancien logiciel ne constitue
pas une procédure de downgrade qualifiée pour une base élaguée.

Travail restant : build/link Docker, nouveau scénario et régressions pertinentes,
corruption du marqueur/state récent, frontière des pannes d'écriture et reprise
avec témoins indépendants, comportement RPC, bornes et mesures disque,
portabilité AMD64, sauvegarde/restauration et procédure de downgrade. Les écritures
sync gardent leur politique ; la suppression logique ne garantit pas une baisse
immédiate de taille des SST et n'élague pas les archives signées conservées.
