# Capella : panne des processus P2P avec validateurs actifs — 27 septembre 2026

**PASS** : deux nœuds réels locaux, chacun avec 32 clés synthétiques distinctes. Les deux processus enfants `nano-p2p` sont suspendus par SIGSTOP pendant 96 secondes ; les cœurs, horloges, validateurs et bases restent actifs. SIGCONT rétablit les échanges. Le harnais restaure les processus avant son nettoyage, y compris en cas d’exception.

## Critères et résultat

- Tête commune au slot 10 avant coupure.
- Pendant la panne, la finalité de chaque nœud est interrogée chaque seconde et ne doit pas dépasser l’époque de début de coupure (1). Le harnais ne bloque ni les propositions ni les signatures locales.
- Les deux branches continuent : têtes distinctes aux slots 24 et 23 à la reprise.
- Après reprise, une finalité commune d’époque >= 3 est requise en 240 secondes maximum ; les en-têtes finalisés doivent confirmer le même root pendant trois observations consécutives.
- Résultat : **époque 4 commune**, root `0x0be08d27d989684dcec5b0c9b83ab2bc195ee8f54189ab8b3a1ed03c79b0ccc6`. Les deux workers et horloges sont sains à l’arrêt. Exit code 0.

Les empreintes des sources et binaires ont été capturées avant le run puis revérifiées. Le patch utilise `git apply --unidiff-zero`. Aucun changement des règles de consensus ni déploiement public n’accompagne cette qualification.

## Échec initial du prérequis conservé

Le premier essai n’a pas atteint le slot de départ dans le délai arbitraire de 40 secondes : il s’est arrêté **avant toute suspension P2P**. La fixture initialise l’horloge près du genesis et les premiers devoirs observés se situent aux slots 8–10. Cet échec appartient au harnais, pas à une récupération tentée. `preflight-failed.log` et ses empreintes sont conservés.

Le prérequis attend désormais jusqu’à deux epochs du profil, et exige une tête identique avant suspension. Le délai de panne (96 secondes), le délai de récupération (240 secondes) et les assertions de finalité n’ont pas été assouplis après une panne observée.

## Limites

Une seule campagne après correction du prérequis. Il s’agit d’une **panne des processus de transport**, pas d’une coupure de paquets avec daemons et timers réseau actifs. Les connexions TCP peuvent conserver des données en attente pendant SIGSTOP. Les assertions de finalité portent sur les observations RPC ; ce test n’est pas une preuve formelle de sûreté byzantine.

La partition Docker avec daemons actifs, le WAN, la perte de paquets, les latences asymétriques, la charge et les grandes populations restent à qualifier sur Capella. Les bases temporaires sont supprimées par le harnais ; les journaux et hashes sont conservés, pas une archive SSZ complète rejouable. Ce succès ne clôture pas la préparation à la production.
