# Raccordement des preuves au futur scénario WAN — 2026-10-02

Contrôleur exécuté réellement, exit 2 : refus prévu faute de résultats terminaux Linux. Aucun nouveau nœud WAN créé. Ce contrôle prépare l’attestation des binaires, il n’est pas un lanceur WAN ni une preuve de production.

Deux portes actuellement vérifiées : différence exacte d’une seule source de test entre Docker `0f53b42d` et natif `016ab243` (15 314 autres fichiers identiques), et réexécution de l’auditeur indépendant des treize cas natifs/205 inputs et quatre audits signés/99 blocs/97 transactions. Les deux révisions complètes restent distinctes ; aucune preuve Linux automatiquement renommée au nom du descendant.

Portes obligatoires restantes : régression complète ARM64 427 cas ; régression complète AMD64 427 cas ; audit signé lecture seule du binaire AMD64 exact de la campagne complète. Chaque régression impose sources avant/après égales au manifeste gelé, commandes/programmes/bibliothèques inchangés, zéro skip, délais originaux, registre et JUnit exacts. Les sorties EEST seront relues cas par cas : sélection connue de 5 387 variantes (4 882 exécution, 505 validation), 317 fichiers, sans revendication de conformité de fork complète.

L’audit AMD64 futur devra être lié au hash du binaire complet, à la même fixture publique que le natif, aux engagements/racines attendus et aux trois refus invalides. Les copies terminales AMD64 ne seront consommées qu’après copie complète attestée. À cette date ces portes futures n’ont pas été exercées positivement ; le contrôleur échoue sur leurs preuves manquantes et conserve `readyForArtifactAttestation:false`.

Contrôle indépendant actuel sous `/tmp/nano-0f53-runtime-wan-gate-20261002/check.py`. ARM64 conteneur `nano-full-coherent-0f53b42d-arm64` actif, 59/427 tests terminés ; AMD64 conteneur `nano-full-coherent-0f53b42d-amd64` actif, compilation à 74 %. Observations bornées, aucun résultat global anticipé. L’attestation des hashes réellement présents, la reprise des duties et la finalité commune à trois propriétaires restent à établir après ces prérequis.

## Lanceur raccordé, voie de refus exercée

`launch.py` adapté aux deux builds complets `full-coherent-0f53b42d-{arm64,amd64}-20261002` et au natif `016ab243`. Exécution réelle : exit 2, phase `preflight-refused`, `newWanNodesStarted:false` ; aucun répertoire de scénario ni nœud créés. La voie positive d’attestation et le scénario WAN n’ont pas encore été exercés.

Après les gates : comparaison des hashes présents dans les volumes aux manifestes terminaux complets, identité des images figées, provenance Docker conservée à `0f53b42d`, provenance native réellement construite conservée à `016ab243`. Les arbres doivent correspondre aux manifestes gelés avec l’unique différence de test connue ; les 205 entrées natives sont vérifiées avant/après. Deux révisions sources explicitement enregistrées, aucune réécriture de la provenance native pour prétendre à une régression complète sur la même révision. Runtime source commun contrôlé par le harness existant, journaux/fixtures/scénario non modifiés.

Le futur scénario reste `tests/multihost/run.mjs` avec restauration intégrale, engagements séparés, anti-double-signature, reprise des duties et trois époques de finalité commune. Programmes/sources/inputs sont comparés autour de l’invocation réelle ; les bases et journaux qui doivent avancer sont exclus de cette immobilité. Aucun rebuild d’image ajouté. L’attestation des hashes réels et la qualification WAN restent ouvertes jusqu’à leur exécution effective.

## Audit EEST lié à l’inventaire réel des fixtures

L’audit des compteurs ne garantissait pas que l’ensemble des identités rapportées était exactement celui du corpus. Nouvel auditeur `eest-fixture-audit.py` : SHA-256 des 317 fichiers comparés au manifeste figé ; ensemble exact des 5 387 quadruplets fichier/cas/fork/variante ; racines réelles et attendues liées à la fixture, logs d’exécution, exception autorisée et refus sans mutation/exécution pour validation ; gas et indicateurs de reçu lorsque présents. Les booleans de transactionHash/reçu ne sont pas une réexécution cryptographique indépendante ; cette limite est explicitement conservée.

Exercice positif sur le rapport ARM64 historique `2d2764de`, sans relancer l’EVM : 4 882 variantes d’exécution/505 validation concordent avec le corpus figé. Sept altérations rejetées : variante manquante, doublon, variante étrangère, racine fausse auto-cohérente, mauvaise raison de validation, transaction de validation exécutée, gas faux marqué comme correspondant. Hash du rapport historique inchangé. Ceci exerce l’auditeur ; ne requalifie pas le runtime courant avec un ancien résultat.

Le gate applique désormais cet audit aux sorties terminales de chaque campagne complète actuelle, en plus des audits registre/JUnit/programmes/sources/bibliothèques et compteurs. Gate renforcé exécuté : exit 2, preuves Linux terminales toujours manquantes. Voie positive sur campagnes courantes non acquise ; aucune nouvelle instance WAN. ARM64 observé actif à 73/427, AMD64 compilation active à 98 %, sans OOM.
