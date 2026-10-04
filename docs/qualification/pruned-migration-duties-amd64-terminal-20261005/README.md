# Duties après migration — AMD64 terminal, 5 octobre 2026

Scénario source figé 477e5ba4, cœur 59fa6530 et helper 23df7f63. Même nouveau parcours que ARM64 : copie prunée auditée, format legacy modélisé par la seule clé de schéma de la copie, refus du nouveau lecteur, helper réel lié à l'engagement RAM, idempotence, audit canonique, puis reprise avec journaux/témoins indépendants. 64/64 journaux avancés, conflit refusé avant signer, finalité locale 2 → 7, source/backup inchangés. Programmes et bibliothèque re-hachés indépendamment, résultat identique.

Le conteneur a terminé exit 0 : témoin obtenu par docker wait enregistré pendant qu'il était encore actif. Le wrapper Python/SSH a échoué à sa limite totale 300 s compilation + test (TimeoutExpired), avant la fin du même conteneur. Échec conservé ; aucune relance, assertion ou limite étendue. L'exécution fonctionnelle est qualifiée par la sortie réelle 0 et les assertions ; le budget combiné de contrôle de 300 s n'est pas qualifié. Il faudra séparer la compilation de l'observation bornée dans les prochains contrôleurs.

Portée locale de deux propriétaires et 64 clés synthétiques, injection directe. Pas de nouvelle preuve WAN, migration publique en place, panne électrique ou production globale. Anciennes sources/programmes/erreurs restent conservés ; aucun rebuild d'image.
