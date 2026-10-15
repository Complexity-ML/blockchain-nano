# Bootstrap natif/Linux partagé — qualification locale

Le lot adapte le launcher existant et ajoute un mode d’export natif explicite, sans créer de second lanceur réseau. Il livre quatre overlays fonctionnels, le préparateur complet et ses deux nouveaux refus, les deux sources Perl inchangées nécessaires à ses locators, et le contrôleur d’intégration public avec son schéma. L’ancien préparateur V7 et toutes les preuves historiques restent inchangés.

Le [résultat réel](evidence/real-local-bootstrap.json) passe en **172 742 ms**, après un seul export natif et consommation d’un nouveau domaine synthétique local :

| Contrôle réel | Résultat qualifié |
| --- | --- |
| Import natif | 65 paires, 65 réouvertures ; 4 signatures BLS de bootstrap vérifiées |
| Import Linux | 65 paires liées au même domaine, aux historiques et au plan natif |
| Copie/audit C++ à froid | Indices 16–31 : 16 paires, 32 réouvertures, preuves/inventaires et montages en lecture seule vérifiés |
| Nettoyage et conservation | 51/51 noms de conteneurs vérifiés absents ; 6/6 volumes conservés et vérifiés |
| Autorités d’origine | Anciennes identités/bytes et 16 aliases conservés, sans rafraîchissement |

Il s’agit de **deux imports indépendants du même export**, avec le même plan et les mêmes engagements logiques. La copie physique des 16 paires et ses 32 réouvertures concernent uniquement des volumes Linux ; aucune SST native Mac n’a été transférée en Linux, aucune portabilité physique cross-OS ou qualification AMD n’est démontrée. Les journaux historiques manquants du Mac/NAS ne sont pas restaurés par ce domaine frais.

Les imports, la copie et l’audit créent zéro signature supplémentaire ; les quatre présignatures de l’exporteur sont comptées séparément. Le cold-audit ne lit aucun env/clé ; les importeurs vérifient les entrées privées locales de la fixture sans les publier. La capsule privée et ses engagements restent sur l’hôte ; la preuve publique n’en expose que SHA/bytes. Ce domaine consommé ne doit pas être réexporté ou réutilisé.

Les [tests ciblés](evidence/changed-source-tests.json) passent séparément : six syntaxes, deux nouveaux cas purs réutilisés sans replay, un guard/3 refus et deux refus du préparateur ; continuation de 9 785 ms, 52 copies de sources et anciennes autorités strictes. Le premier collecteur reste FAILED sur son attente erronée de sept SKIP : Node avait réellement exécuté deux cas correspondants, tous deux réussis, sans les sept cas non sélectionnés. Aucun ancien test C++ ou cas pur inchangé n’a été rejoué.

Le [préflight logiciel](evidence/local-software-preflight.json) vérifie 63 anciennes autorités, deux programmes natifs supplémentaires, 16 aliases et les 13 réservations préexistantes avant export. **L’empreinte brute historique du checkpoint privé07 n’avait pas été enregistrée ; cette limite demeure ouverte.** Une lecture actuelle n’est pas présentée comme une preuve historique. Les statuts source-only/pending des manifestes et revues reflètent leur moment de gel ; le reçu réel ci-dessus qualifie ensuite ce composant local seulement.

Le contrôleur V2 est resté HOLD/non exécuté après détection d’une comparaison sensible à l’ordre des clés JSON ; V3 compare les valeurs et les neuf champs explicitement. Le reçu source V3 supersède le GO V2 prématuré qui avait manqué ce joint. Le double appel `verifyCopy` avait aussi été retiré avant l’essai : l’audit partagé l’exécute déjà dans son finally. Les comptes d’un export tenté mais interrompu restent inconnus ; les quatre signatures attendues sont distinguées des quatre réellement vérifiées. Tous les FAILED et limites historiques restent conservés dans les quatre documents complets.

**Ce PASS ne qualifie aucun runtime Nano/P2P, configuration runtime, reprise des duties, coordinateur complet, finalité commune ou cycle complet des validateurs.** Aucun accès VPS privé, build/pull d’image, fonds réel ou vente n’a eu lieu. La preuve antérieure de transport synthétique conserve son propre périmètre. Reprise effective/finalité des trois propriétaires, cycle long/inclusion/sanction/retraits, WAN/ressources, release/CI/revue de sécurité indépendante et conditions Base–Nano/bridge/Doppler restent ouverts. L’autorisation dédiée AMD/VPS demeure en attente après le refus de revue automatique préalable à toute lecture rejetée. Goal actif ; production non qualifiée ; aucun EIP, fork ou profil promu.

Seules les sources et preuves publiques typées sont livrées : aucun contrôle privé, métadonnée brute9, journal de commande, clé, seed, base de données, binaire ou contrôleur Root Python. Les quatre corps historiques restent byte pour byte en préfixe puis reçoivent un appendice ; CSV et comportement RPC sont inchangés. Les sources utilisent le même adaptateur partagé, avec ses dépendances publiques déjà versionnées au commit de base : aucun nouveau framework ni nouvelle inscription CMake ne sont ajoutés.
