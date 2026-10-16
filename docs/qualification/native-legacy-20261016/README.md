# Native Legacy — qualification limitée du 16 octobre 2026

Deux exécutables Native arm64 ont été reliés au Core `7264c212…` depuis les sources publiques `7c004a00…`. Leur chargeur et leur entrée principale ont été exercés par deux refus de politique attendus. Ce résultat ne correspond ni à un export/import réel de fixture, ni au redémarrage d’un validateur.

Le [terminal réel](evidence/native-legacy-terminal.public.json) confirme trois nouveaux calculs `-M` égaux aux fichiers de dépendances historiques, une recompilation du support avec son nouveau MD égal au M et au MD historique, puis un lien du support. L’objet et la bibliothèque du support sont identiques octet pour octet aux originaux. Deux objets consommateurs conservés ont ensuite été reliés au Core actuel, en préservant l’ordre canonique des cinq archives et deux objets assembleur. Aucun configure, rebuild du Core ou changement d’archive n’a été exécuté. La vérification des sources atteint 400 blobs Git publics.

| Exécutable Native produit | SHA-256 | Octets |
| --- | --- | ---: |
| nano-capella-transition-tests | `568cca3c16f8a864b6994b610ba19f2f8282058012664590d4386421ed8074c9` | 20 964 784 |
| nano-capella-isolated-bootstrap-import | `dedd1f6da955f6dacf272ec4d54ffd80611e5b2ed8090ce8e75d493ebf9a0fd4` | 4 816 760 |

Les deux sondes CLI ont quitté avec le code 1 attendu par la politique de sources. Elles qualifient le chargement et l’entrée principale de ces nouveaux exécutables ; elles ne prouvent pas toutes les résolutions de symboles différées. Aucun export/import de fixture, aucune signature BLS de fixture ou opérationnelle, aucune suite ancienne, aucun flux long, aucun Docker, aucune reprise de duties et aucune finalité commune n’ont été exécutés. La nouvelle admission privée des logiciels a été produite après trois échantillons stables sur au moins deux secondes ; ses fichiers privés ne sont pas publiés ici.

Le [préflight des matériaux originaux](evidence/original-material-preflight.public.json) vérifie 16 lignes sans les rafraîchir. Pris isolément, il ne ferme pas la conformité des dépendances au source courant et ne restitue pas la qualification historique du producteur du support. Le terminal final distingue cette limite de la nouvelle reproduction identique et des nouveaux liens. Les anciennes versions des deux exécutables ne sont pas requalifiées rétroactivement.

L’[historique d’orchestration](evidence/orchestration-history.public.json) conserve deux refus des helpers avant toute compilation : borne de lecture trop petite, puis mauvaise clé du manifeste. Il conserve aussi les deux brouillons source non exécutés corrigés avant le contrôleur final. Ces refus ne sont pas présentés comme des échecs de la blockchain.

Ce lot complète la [qualification Native effective précédente](../native-effective-20261016/README.md), sans la recopier. L’échec global des entrées originales, les sept fichiers hors fermeture sélectionnée, la liaison d’alias historique inconnue et la limite de provenance restent conservés. La fermeture C++/SDK de P2P, la configuration CMake complète et les admissions de campagne des 24 rôles restent ouvertes. Les 24 identités connues ne valent pas 24 admissions. Aucun EIP ou fork supplémentaire n’est qualifié. Le header de support `6681926f…`, le CSV EIP et le comportement RPC restent inchangés.

L’[observation CI](evidence/ci-observation.public.json) concerne précisément `7c004a00…` : GitHub a terminé le workflow en échec pour facturation ou plafond de dépense, avant le démarrage des jobs concernés ; les contrôles Core et réseau sont indiqués skipped. Ce reçu n’établit aucun succès CI et n’annonce aucun changement de facturation.

Le goal de production reste actif. Le cycle complet des validateurs, le vrai transport HTTP/exec, la reprise des duties, la finalité multi-propriétaire et la production ne sont pas qualifiés par ce lot. Les sources de l’opérateur et du préflight sont publiées pour revue ; leur usage nécessite les autorités privées originales, qui sont exclues du paquet avec les SDK, programmes, bases et journaux opérationnels.
