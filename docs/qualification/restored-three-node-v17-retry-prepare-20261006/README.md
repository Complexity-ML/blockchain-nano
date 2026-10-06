# V17 nouvelle tentative après correction des permissions

L'essai initial 92712 est terminal 1 avant ouverture du stockage VPS et avant tout démarrage Mac. Les répertoires Mac existants sont vérifiés contre leurs empreintes puis réutilisés par liens locaux, sans copie de base supplémentaire. Les sources post-v16 indépendantes restent conservées. Le volume VPS est inchangé (1926 fichiers), conservé sur son hôte. Aucun signer actif au moment de cette préparation.

Binaires C++ qualifiés du commit d8d5b46c, correctif de permissions 5051a6bd. Répertoire de configuration 0755 et environnement des clés 0600 ; dérivation obligatoire depuis l'audit local VPS. Journal et témoin indépendants de chacun des propriétaires conservés. Profil, limites et critère de trois nouvelles finalités communes en 900 s inchangés. Préparation seule ; production et reprise réseau non qualifiées.
