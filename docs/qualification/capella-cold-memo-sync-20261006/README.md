État courant : fixture intégrée sur main, parentfa93bc17. ARM6472535 et AMD6468450 terminaux0 ; quatre dispositions WAL chacune, programmes/parent inchangés. Cinq gates enregistrés dans CTest ; configuration CMake ARM64 et inventaire des commandes vérifiés, sans rejouer les tests identiques. Les paragraphes suivants conservent l’historique de préparation.

État courant : ARM6472535 terminal0 ; quatre dispositions WAL passent, hashes parent/test inchangés. AMD64 source transférée et vérifiée, test pas encore lancé. Sources non intégrées au main.

# Panne WAL du réacteur froid et mémo — préparation isolée

Non intégré au main, non qualifié. Compilation ARM64 active72535 ; AMD64 non lancé. Parent runtime mémo ARM64 quatre scénarios terminaux0 ; parent AMD64 dernier scénario en cours85922.

La nouvelle fixture exécute des processus enfants séparés, uniquement sur des copies de bases synthétiques dont les propriétaires sont fermés avant la copie. Aucun fork avec RocksDB ouvert, aucune donnée de validateurs réels, aucun changement de bibliothèque de faute. Une exécution sans faute construit l'oracle indépendant du checkpoint durable ; chaque scénario recharge la même base, exerce le mémo avant EIO fsync, puis vérifie la récupération exacte et les archives signées.

Quatre dispositions : faute WAL principale/témoin, suffixe conservé/tronqué. La perte de suffixe est une simulation sur WAL synthétique, pas une preuve de panne matérielle. Reprise avec témoin, répétition idempotente et réouverture avec engagement attendu doivent être vérifiées. Ce scénario ne qualifie pas une migration interrompue, SIGKILL, panne physique, grand graphe, RSS ou finalité WAN.

Échecs de préparation conservés :95194/1 include support absent par liens de montage macOS ; sortie déjà créée, code1 avant compilation. Réparation des mounts et mkdir idempotent uniquement ;72535 suit la même source fonctionnelle.

Ce lot ajoute tests/test_capella_transition.cpp et tests/capella_cold_memo_sync.hpp uniquement côté fixture ; aucun core reconstruit/modifié. CMake enregistre writer, guards, cold runtime automatic/BLS et cold memo sync séparément. Les commandes/env correspondent aux campagnes qualifiées ; les gates runtime ont un timeout1200s, les gates prepared240s. La configuration CMake AMD64 n’est pas encore exécutée. Le contrôle whitespace global du lot précédent détectait les espaces finaux bruts des commandes de lien archivées ; ces logs originaux sont conservés, le contrôle sur sources et quatre documents est passé. Aucun déploiement réseau/EIP/bridge/production promu.

Complément : configure/inventaire CTest AMD64 du main5f2977a6 terminaux0, cinq gates présents. Ancienne mention non exécutée conservée comme historique ; aucune répétition des tests nécessaire pour cet inventaire.
