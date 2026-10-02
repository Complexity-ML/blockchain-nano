# EEST du candidat courant ARM64 — bloc terminé, campagne globale en cours

Candidat `0f53b42d` : test 125 `ethereum_official_regressions` passé en 3,45 s / délai original 30 s. Journal provisoire CTest copié en lecture seule pendant la poursuite du run complet ; bloc EEST complet et terminé extrait de ce snapshot. Aucun rejeu EVM.

Audits exacts exécutés : 317 fixtures liées au manifeste gelé, 5 387 variantes exactes (4 882 exécution / 505 validation), racines/logs/erreurs/gas disponibles liés aux fixtures, compteurs cohérents. Binding exécuté : 15 315 sources actuelles identiques au manifeste/avant campagne, hash du runner présent égal au hash avant campagne. Source actuelle vérifiée, aucune qualification par héritage de l’ancien run `2d2764de`.

Première invocation du script de compteurs via `/tmp` : sortie vide, exit 0 ; son garde CLI compare import.meta.url résolu en `/private/tmp` au chemin argv non canonique et n’exécute pas le main. Première étape de binding refusée par JSONDecodeError sur ce résultat vide. Appel répété uniquement de l’auditeur avec chemin canonique `/private/tmp` : sortie valide, audit/binding réussis. La chaîne de qualification corrige le caller via `Path.resolve()`, pas le runtime ni les tests déjà exécutés.

Limites : snapshot provisoire et bloc EEST terminé uniquement ; campagne 427 cas encore active, invariance terminale complète encore à vérifier, EEST AMD64 non qualifié, profils Genesis public/forks activés et conformité Ethereum globale non déduits. Aucun statut EIP ni critère global de production clôturé.
