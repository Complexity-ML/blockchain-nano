# Reprise v17 — échec de permissions avant ouverture du stockage

Session originale 92712, sortie terminale 1, checkout 2e8d9c01. Préflight réussi et configuration construite exclusivement sur le VPS, liée par empreinte au plan indépendant. Le conteneur n'a pas pu lire `/config/genesis.json` : répertoire créé en mode 0700 et utilisateur du conteneur différent. Aucun nœud Mac lancé, aucun reçu d'arrêt de consensus, aucune conclusion sur le critère de finalité en 900 s.

Conteneur VPS arrêté exit 1 sans OOM. Les 1926 fichiers du volume sont identiques à l'audit post-v16 ; aucun writer. Engagements et journaux inchangés. L'état Docker archivé exclut environnement et clés.

Correction du préparateur : répertoire 0755, fichier d'environnement des clés maintenu 0600. Préparation exclusivement locale sur le VPS ; l'image réelle lit genesis/profil/état initial/configuration, sans monter la base ni lancer de signer. Les sorties et empreintes sont conservées. Ce contrôle ne qualifie pas une reprise complète : une nouvelle tentative contrôlée est encore nécessaire.

Aucune recompilation C++, reconstruction d'image, export de configuration ou clé depuis Mac. Production et toutes les exigences restantes restent ouvertes.
