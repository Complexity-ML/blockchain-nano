# Contrôle terminal de la candidate RPC d1597d9a

Reprise du vérificateur indépendant déjà utilisé sur 9e9 : seule la révision attendue passe à `d1597d9a6987d0839ed2f0c7ded268aead35d071`, avec référence des 15 315 fichiers de cette candidate. Les autres assertions, baseline de commandes/propriétés inchangées, délais originaux, 427 cas uniques exécutés sans échec/skip, empreintes sources/programmes stables, supplément réseau 21, audit exact EEST et relecture des binaires en volume sont conservés. Les preuves historiques et leur vérificateur ne sont pas modifiés.

Les contrôles synthétiques utilisent la sélection et le manifest avant tests de la campagne ARM64 courante ; leur JUnit positif est artificiel et ne constitue jamais un résultat de la campagne. Les mutations négatives couvrent révision erronée, dérive sources/programmes, helper ancien/manquant, délais relevés, skip, cas absent/dupliqué/non exécuté, échec caché et statuts terminaux incorrects. Les rapports réels incomplets doivent être refusés.

La campagne en cours n'est pas terminale. Aucun lancement WAN ou statut production ne découle de cette préparation ; `staticLinkInputsAudited`, `productionQualified` et autres limites historiques restent explicitement distincts. Les contrôles et leur résultat seront enregistrés séparément.

Contrôles : 15 réussites (14 mutations refusées, un positif synthétique). Le rapport ARM64 réel incomplet est refusé pour absence du manifest après tests ; ce refus est une vérification du garde, pas un échec de la campagne. Journaux conservés dans ce dossier.
