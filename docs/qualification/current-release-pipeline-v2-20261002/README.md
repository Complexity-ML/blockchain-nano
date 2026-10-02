# Contrôleur v2 : chemin CLI Node canonique

Défaut observé lors du bloc EEST courant : le helper compteurs appelé via `/tmp` sortait 0 sans main ni résultat, en raison du garde import.meta.url/argv sous macOS. Le gate aurait refusé ensuite le JSON vide ; aucun faux succès ni lancement WAN n’était possible.

Remplacement uniquement du contrôleur d’attente : ancien PID 60971/session 3141 terminé explicitement SIGTERM, exit 143. Scripts précédents conservés. Nouveau contrôleur figé PID 61450/session 34463 vivant confirmé ; mêmes handles de campagnes Docker 80870/582618 et file d’audit VPS 587064 inchangés. Seule correction fonctionnelle : caller de l’auditeur Node résolu par `Path.resolve()` avant invocation. Tous les garde-fous/copieur/audits/lanceur conservés ; aucun résultat de test relancé.

La voie CLI corrigée est effectivement exercée sur le bloc EEST courant (preuves séparées), résultats cohérents 5387/4882/505/317. Contrôleur v2 en attente réelle ; copies terminales et voie positive de restauration toujours non exercées. Il conserve la provenance distincte native/Docker et tous les critères de production.
