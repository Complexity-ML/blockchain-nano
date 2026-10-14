# Startup steady : qualification Docker limitée

Runtime8e6/helpera585/testV2bf9 : plafond `clock.maximumStartupElapsedMs` optionnel, coopératif et steady ; erreurs de profil/cancellation restent distinctes. Il ne remplace pas UTC ni le plafond administratif de downtime2h/60. Après retour ou RAII, les copies du callback ne gardent pas un budget startup actif. Les engagements d’un préfixe déjà publié doivent être conservés après expiration.

ROOT a réellement passé deux CTests par architecture Docker Linux ARM64/AMD64, sans retry/skip : six cas startup + résumé, cinq groupes deadline + résumé (13 lignes). Deadline : ARM5285ms/23764KiB,AMD5802ms/25600KiB ; batchCTest5.764/7.098s. Deux retours runtime synthétiques, zéro nouvelle signature/clé/entrée privée, aucun validator duty ou UTC réel qualifié. Les clocks de fixture publique sont distincts de la reprise opérationnelle.

Builds : runtime1 frais/141 objets réutilisés, nouveau testdeadline, main/startup objets réutilisés sous argv/flags/MD/SDK identiques, trois liens nouveaux et raw142members mesurés. Source1319 (dont pending courant explicite mais non compilé/testé), imagesexistantes, source/SDK/parents et conteneurs relus fermés après run. Ne pas compter142 objets comme142 compilations fraîches ni qualifier CAS/pending ancien ou une suite entière.

Ancienne source/controller V1 n’ont jamais été compilés/exécutés : leur GO statique ne devient pas un PASS ou un échec CPP. Le SDK interdit RO concurrent à RW ; trois scopes du nouveau test ont été réparés avant les premiers builds. Le gate du CLI maintenance actuel ae37 demeure OUVERT jusqu’à qualification de sa révision sûre : les13PASS historiques maintenance ne sont pas effacés, ni ce lot confondu avec l’outil. Aucune copie privée manipulée.

Les trois sources runtime/helper/test de livraison sont les bytes réellement compilés. CMake de livraison conserve toute la CMake ae37/d0b160051 (CLI/install/3targets) puis LF + registration515 ; SHAmerged a2abd586… et160567bytes. Le build deadline utilisait CMake011a sur le parent plus ancien. Cette fusion exacte est une étape de livraison à vérifier par ROOT ; pas un nouveau configure/build déjà qualifié.

AMD : huit payloads publics seulement, hashes readback remote/local égaux. ARM : preuves publiques build/test/source/SDK uniquement. Aucun binaire/objet/librairie/DB/capsule/env/seed/clé, quatre preuves privées ou hash dérivé de celles-ci. Le package contient les sources, manifestes, revues, terminaux, JUnit/logs et nouveaux préfixes ; les corps historiques restent byte-exacts dans le dépôt, CSV3716 inchangé.

Restent ouverts : grant expiré/UTC/longue panne, journaux/floors/WITNESS privés et whole-node backup, duties/finalité commune, consensus/partitions/lifecycle/charge/exploitation, V37/NAS, natif/install/imagesruntime/CI paiement/audit externe, profil au-delà du Shanghai activé et Base–Nano/USDC/Doppler. Pas de fonds/vente/déploiementpublic ; production non qualifiée, objectif actif.
