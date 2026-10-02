# Régression ARM64 complète et revue terminale de d1597d9a

Image e30 / volume `nano-clean-arm64-build-20261002`, build neuf `rpc-full-d1597d9a-arm64` après reset, sans instrumentation RPC : 427/427 cas exécutés sans échec/skip en 5 805,22 secondes. CTest, manifest après tests, audit et contrôleur terminaux à zéro. Même registre, commandes et délais d’origine que la baseline figée. Aucun retry ni délai relevé.

Supplément des 21 cas réseau autonome : tous passent, audits/CTest/conteneur à zéro, programmes avant/après identiques. La revue indépendante terminale relit les 15 315 sources et les 208 programmes depuis le volume existant en lecture seule ; empreintes correspondant aux manifests de la campagne. Référence d159 exacte, aucun helper P2P du vieux build partagé.

Audit exact du corpus EEST sélectionné : 317 fichiers, 5 387 variantes (4 882 exécution / 505 validation), identités, racines, logs, erreurs attendues et gas disponible rattachés aux fixtures. Les booléens hash de transaction/reçu ne sont pas réexécutés indépendamment par ce vérificateur ; conformité exhaustive de fork non prouvée. Entrées statiques de liaison Docker non entièrement auditées. Ces limites restent présentes dans les rapports.

Le scénario `consensus_capella_exit_network` passe en 426,81 secondes : admission au pair non validateur hors ligne, reprise durable et relais libp2p, retrait capital, finalité identique et comptabilité, réouverture des deux owners, absence de proposition/attestation après sortie. Limites test-only de tenure/délai raccourcies et même hôte physique ; ce n’est pas la qualification WAN publique.

L’échec historique WebSocket Paris de 9e9 reste conservé : ce scénario passe ici sans instrumentation et avec sa limite d’origine. Pas de déduction de cause unique à partir de cette seule réussite. AMD64 encore en cours, aucun WAN/duty restauré démarré et aucun runtime promu sur main. Objectif production non atteint ; tous les autres gates persistent.

Logs build/CTest/JUnit, registres, sources/programmes avant/après, suppléments et revues dans l’archive ; SHA dans `SHA256SUMS`. Aucun volume de données, image Docker ou clé privée n’est archivé.
