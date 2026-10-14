# Observation des journaux et du registre — 14 octobre 2026

Le code livré expose deux observations C++ en lecture seule : la paire journal/témoin avec ses réservations conservatrices, et le contexte du head déjà authentifié par le décodeur complet Capella. Le registre est retourné par valeur. La description et les formats durables existants restent inchangés ; aucun signer, récupération ou duty n’est autorisé par ces observations.

| Lot réellement exécuté | Docker ARM64 | Docker AMD64 VPS |
| --- | --- | --- |
| Politique mémoire locale, deux stores génériques synchrones et réouverture | PASS | PASS |
| Nouveau journal observer : six groupes et résumé | PASS | PASS |
| Régression du signer : réservations, concurrence, SIGKILL, défaut sync, import/retrait | PASS | PASS |
| Nouveau contexte du head : six groupes et résumé | PASS | PASS |

Le lot head a recompilé exactement deux unités cœur et un consommateur par architecture, réutilisé 141 objets inchangés et vérifié les 143 membres réels de chaque archive. Compilation ARM : 22,628 s ; AMD : 69,905 s. Les campagnes du nouveau test et de ses gardes ont duré 5,156 s et 16,249 s. Les sept lignes JSON sont six groupes plus un résumé ; ce sont deux invocations C++ réelles au total, sans ancien test rejoué. Le pic RSS exact de ce test n’a pas été retenu dans les sorties ; le budget interne ne qualifie pas celui d’un nœud complet.

Les logs publics ARM sont relus ; les résultats AMD sont filtrés et attestés par ROOT en situ. Les nouvelles métadonnées de compilation et logs complets AMD restent sur le VPS. La régression du signer crée ses propres signatures et bases synthétiques ; leur nombre exact n’est pas émis. Les deux nouveaux observateurs ne créent aucune signature ni base.

La première tentative ARM a été refusée par le bac à sable au socket Docker avant toute compilation, copie d’objet, lien ou test. Ce refus est conservé ; la réussite utilise un dossier distinct et le même contrôleur relu. Les défauts source des premières versions du runner restent conservés, sans exécution de ces versions.

Cette preuve concerne le lot de source ciblé et ses programmes. Elle ne constitue pas un build complet/reproductible de release, une qualification macOS native, de nouveaux tests positifs de queues signées non vides, un audit de sécurité externe ou une preuve de production. L’outil de lecture de tous les journaux privés n’est pas encore qualifié. Les minima numériques ne prouvent pas l’ascendance historique ; les réservations ne prouvent pas l’envoi de signatures. Reprise des duties et finalité commune qui avance restent ouvertes.

Le profil réseau reste Shanghai et les statuts EIP restent inchangés. Cette livraison n’active ni réseau, ni horloge, ni vente, ni bridge, ni fonds.
