# Lanceur WAN cohérent préparé, non exécuté

Le lanceur vise exclusivement la restauration de la devnet privée Capella (Mac natif, Docker Mac, Docker VPS). Sources/binaries : Docker 9e9ebed3, cœur natif 016ab243 (seul contrôleur CI différent), réseau natif reconstruit depuis 9e9ebed3, pilote JS dea3f827. Aucun runtime ne vient de main.

Avant démarrage : résultats terminaux des deux campagnes 427 cas et suppléments 21 cas, quatre résultats signés AMD64, revues indépendantes complètes/EEST exact/SHA actuels, qualification native 13 cas et quatre cas signés, réseau natif 21 cas/23 programmes inchangés, validation des trois deltas JS du pilote. Les tags/images doivent correspondre aux deux digests gelés. Les chemins réseau sont internes au build propre de chaque architecture. Les métadonnées de provenance ne sont ajoutées qu’après contrôle des programmes, avec l’UID du propriétaire du build. Un nœud WAN ou RPC natif préexistant entraîne un refus.

Le scénario conserve sources suivies, 205 entrées natives et programmes avant/après. Le transport est IPv6 à empreinte SSH conservée. Aucune compilation ni transfert de logiciel par SCP. Il ne prétend pas terminer la production à lui seul.

**État : code préparé, syntaxe analysée ; lanceur jamais exécuté et non qualifié.** L’approbation automatique a refusé la tentative d’exécution avant résultats terminaux, car ce programme peut démarrer des nœuds et modifier l’état distant. La commande refusée n’a pas été lancée. Le contrôleur séparé `read-only-gates.py`, qui n’importe jamais le lanceur, a été exécuté en lecture seule : tous les résultats terminaux requis sont encore absents. Aucun nœud lancé. Aucune tentative indirecte du lanceur n’a suivi le refus.

`launch.py` exige désormais un argument explicite `--execute-qualified-wan` et conserve toutes les vérifications ; ne pas le lancer avant acquisition et revue des preuves terminales. Les scripts d’inspection et les preuves de refus sont archivés. Tous les critères de production restent ouverts.
