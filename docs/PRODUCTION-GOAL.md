> **17 octobre 2026 — statuts P2P bornés, trois environnements.** Handshake/chainsync limités à 4 KiB avant analyse, `best_hash` canonique et projection avant conservation ; plafond blocs/données 16 MiB inchangé. Trois nouveaux P2P et les deux tests concernés par environnement passent ; les deux selectors reproduisent les acceptations de l’ancien code. Le refus Python initial et la clôture baseline FAILED sur le seul `ctime` du contrôle neuf restent conservés, sans rejeu ; le nouveau contrôle ARM est stabilisé avant première admission. Zéro image/Core reconstruit, aucune duty ni finalité commune reprise ; anciens index/admissions non retargetés. Le risque exits/reorg/pruning est encore source seulement. Campagne 03, consensus/production, CI/facturation, revue externe et termes Base–Nano restent ouverts. [Preuves et limites](qualification/p2p-status-bounds-20261017/README.md).

> **17 octobre 2026 — outils opérationnels et images candidates.** Le moniteur refuse les redirections/enveloppes RPC ambiguës (20 résultats ciblés). Le packaging accepte deux artefacts physiques séparés ; ses dix contrôles concernés passent après diagnostic/admission initiale du nouvel IID. Une image réelle par architecture, aucune recompilation C++ : packaging VPS PASS ; contenu ARM vérifié séparément, clôture du packaging ARM FAILED conservée, cause non localisée. Aucun rebuild pour le correctif du contrôleur, aucune duty reprise, aucune identité créée. La tentative 03 attend son accord distinct ; CI bloquée par facturation, critères de consensus/production, revue indépendante et termes Base–Nano restent ouverts. [Preuves et limites](qualification/operational-candidate-images-20261017/README.md).

## 17 octobre 2026 — candidat runtime RPC, 24 actifs et trois admissions de code

Les [preuves ciblées](qualification/rpc-runtime-assemblies-20261017/README.md) composent neuf actifs Native, huit ARM et sept AMD, dont trois P2P rejoints sans exécution. Les assemblages sélectionnés passent : sept/six/cinq liens runtime et huit/sept/six CLI ; Native valide aussi 476 checks/126 refus/258 cas. Un sourceMF commun et trois admissions de code exactes à 13 champs sont émis, liés au commit de portabilité du test `7bd24dea…` sans circularité. Le spec complet reste non validé et la campagne 03 non autorisée. Les refus Native 01/02 et l’ancien échec hôte ARM restent conservés, sans réparation d’identités anciennes. Les preuves inchangées restent acquises dans leur périmètre. Goal actif ; reprise/duties/finalité, parité RPC globale et production restent non qualifiées.
## 17 octobre 2026 — comparaison RPC Geth complète, non-parité conservée

Le [rapport public](qualification/geth-rpc-context-20261017/README.md) conserve 258 comparaisons Paris/Shanghai sur Geth authentifié : 208 conformes aux critères, 48 écarts d’extensions/classes et 2 listes d’accès redondantes. L’oracle reste FAILED ; 18 devis supplémentaires diffèrent sans être bloquants. Aucun returnData comparable ne diffère. Six diagnostics séparés confirment l’effet du nonce fourni sur l’adresse redondante. Geth 03 termine avec clôtures intactes ; Geth 01, Geth 02 et Anvil (116 divergences) restent conservés. Aucun test C++/CI n’est rejoué, aucune parité globale ni promotion EIP n’est annoncée. Les preuves historiques restent acquises dans leur périmètre. Goal actif ; admissions runtime, cycle/reprise/duties/finalité et production restent non qualifiés par ce lot.

> **17 octobre 2026 — Contexte des simulations RPC corrigé ; comparaison Anvil encore en échec.**
> [Sources, résultats et historique](qualification/rpc-simulation-context-20261016/README.md). `eth_call`, `eth_estimateGas` et la découverte de liste d’accès partagent l’achat de gaz, le nonce et l’état copié ; les quantités sont bornées avant capture. Docker ARM64 et AMD64 passent chacun 476 vérifications/126 refus attendus, les quatre baselines anciennes échouent comme prévu, et les trois régressions concernées passent (RPC 1 053, listes d’accès sept groupes, EIP-1898 120 vérifications/58 refus). Les 258 réponses ciblées sont identiques entre architectures. Seul `rpc.cpp.o` remplace un membre sur 143 ; aucune image reconstruite. Les dix contrôles du garde CI passent en deux exécutions locales clôturées, sans prétendre à une CI GitHub verte.
> **Anvil : 258 comparaisons terminées, 116 divergences ; parité non qualifiée.** La priorité suivante est leur classification contre Geth réellement exécuté, sans modifier Nano sur la seule autorité d’Anvil. Sept refus d’outillage et un oracle CREATE erroné restent conservés. Le profil EVM, les preuves EEST/Doppler antérieures et les autres statuts EIP restent dans leur périmètre acquis, sans rejeu automatique ni promotion. Assemblage Mac natif/runtime et admissions après ce Core, cycle des validateurs, reprise/finalité/WAN/charge restant, opérations, bridge et termes Base–Nanobees/Doppler, CI/facturation et revue indépendante demeurent ouverts. Aucune nouvelle duty, clé de consensus, vente, fonds réel ni mise en production ; goal actif. Historique intégral conservé ci-dessous.

> **16 octobre 2026 — Bridge commun implémenté et qualifié en synthétique.**
> [Sources, 153 cas par moteur et preuves](qualification/bridge-common-20261016/README.md). Les endpoints abstraits ERC-20 couvrent lock/mint/burn/release, engagement des 14 champs, effet unique par nonce source, conservation, plafonds configurés et rollback. Les mêmes 153 cas passent dans Anvil et le véritable `ExecutionEngine` C++ Paris : 77 refus attendus chacun, comptabilité identique, helper natif sortie 0. Un seul helper et une compilation Solidity, aucune image/Core reconstruite ni ancienne suite rejouée. Les échecs du contrôleur sont conservés et sa clôture réconciliée sans rejeu ; 1056 entrées inchangées. Vérificateur/finalité interchaînes, route et termes Base–Nano/USDC restent absents ; aucun EIP/fork, RPC, consensus ou production promu. Cycle des validateurs, autres critères, CI/facturation et revue indépendante restent ouverts ; goal actif. Historique intégral conservé ci-dessous.

> **16 octobre 2026 — Observations Base : trois collisions de cache reproduites et corrigées.**
> [Sources et preuves](qualification/bridge-source-serial-20261016/README.md). L’ancienne version accepte trois divergences effacées par des requêtes suivantes ; le correctif termine chaque lecture et sa projection avant la suivante. Le fichier concerné passe 35 résultats Node (33 cas feuilles et deux parents), sans skip, en Docker sans réseau. Les erreurs causales de l’ancienne source restent conservées. Treize entrées hôte et quatre copies Linux sont inchangées ; aucun build d’image/C++, transaction ni nouvelle identité de consensus. Le transport HTTP inchangé n’est pas rejoué. Le [protocole de bridge en préparation](BRIDGE-PROTOCOL-DRAFT.md) distingue réserve, émission, destruction, restitution et authentification manquante ; aucun contrat ou droit de conversion n’est implémenté par ce lot. Aucun EIP/fork ni production promu. Modèle de preuve, cycle des validateurs, autres critères et revue indépendante restent ouverts ; goal actif. Historique intégral conservé ci-dessous.

> **16 octobre 2026 — Cycle Doppler local qualifié avec le vérificateur LP renforcé.**
> [Rapport et preuves](qualification/doppler-real-anvil-20261016/README.md). Sur un fork Anvil de Base, 19 étapes passent : enchère Dynamic V4, création, achat, vente et migration historique vers une paire V2 ; trois transactions adverses sont refusées sans changement des projections contrôlées. Six lectures RPC au même hash canonique concordent ; les événements lient le mint LP, le crédit bénéficiaire puis Migrate (17 < 20 < 24). Sortie0, processus arrêtés, conteneur absent et quinze entrées hôte inchangées. Image existante réutilisée, sans compilation C++ ni rejeu des suites unitaires. Token TEST non adossé, USDC et comptes synthétiques : ni migration V3, ni Nano natif/bridge/finalité cryptographique/production qualifiés. Le refus initial, la campagne02 FAILED et les corps historiques ci-dessous restent conservés. Cycle complet des validateurs, autres critères de production, CI/facturation et revue indépendante demeurent ouverts ; goal actif.

> **16 octobre 2026 — Campagne lifecycle 02 : échec conservé, transport et clôture corrigés.**
> [Observations et échec de campagne](qualification/lifecycle-private-attempt2-20261016/README.md), [correctifs et 25 tests Docker ciblés](qualification/lifecycle-control-stage-20261016/README.md). La campagne privée autorisée reprend réellement après sa première restauration et observe une finalité commune 20→24→25→26, puis échoue à la préparation de la sortie volontaire : deux SSZ publics manquent au transfert initial. Tous ses runtimes sont arrêtés. Les unités du correctif ne requalifient pas cet essai ; sorties, sanctions, retraits et deuxième restauration restent ouverts. Chaque capture publiera désormais ses engagements immédiatement et les reçus d’un propriétaire déjà arrêté seront conservés jusqu’à sa prochaine tentative de reprise.
> [Doppler : attribution LP renforcée, 29/29 unités](qualification/doppler-lp-attribution-20261016/README.md), sans migration Base réelle ou backing/bridge qualifié. Aucune nouvelle image ni compilation C++, aucun fork/EIP promu et aucun ancien domaine/journal réinitialisé. Profil réellement activé inchangé ; limites et échecs historiques conservés intégralement ci-dessous. CI/facturation, revue indépendante, consensus/WAN/charge restant et conditions Base–NANO/Doppler demeurent ouverts. Production non qualifiée, goal actif.

> **16 octobre 2026 — Réponses Base et admission ARM corrigées dans leur périmètre.**
> [Sources, résultats et limites](qualification/base-arm-admission-20261016/README.md). Trois défauts de références RPC mutables sont reproduits sur l’ancienne source ; le correctif passe les 30 cas feuilles du fichier et leur groupe parent (31 résultats Node). Cinq contrôles métadonnées passent pour l’admission prospective des JSON ARM et le refus des mutations après admission.
> Trois paires de nouvelles copies montrent une dérive de ctime à octets inchangés. Le contrôleur complet corrigé n’est pas exécuté ; l’ancien échec hôte ARM reste conservé et sa ligne exacte non récupérée. Aucune ancienne autorité rafraîchie, duty repris, finalité commune démontrée ou image reconstruite.
> Campagne fraîche sans autorisation dédiée, restauration/consensus/WAN/charge, cycle complet des validateurs, opérations prolongées, bridge et termes Base–NANO/Doppler, CI et revue indépendante restent ouverts. Observation RPC seulement : mint et finalité cryptographique non qualifiés. Historique intégral conservé ; aucun EIP/fork ni production promu. Goal actif.

> **16 octobre 2026 — Programmes assemblés au Core corrigé.**
> [Rapport, sources et identités](qualification/runtime-core-relink-20261016/README.md). Nano et les outils sont reliés au Core courant sur Mac natif, Docker ARM et Docker AMD ; les rapports bornent les dépendances, réutilisations et vérifications CLI. Le P2P inchangé reste joint séparément à ses preuves conservées.
> Le worker ARM passe ses vérifications mais son contrôleur hôte échoue à la dernière barrière d'identités ; la cause reste inexpliquée et cette qualification complète reste ouverte. Un rapprochement séparé vérifie 1 226 entrées conservées et sept fichiers logiciels, sans rejeu ni rafraîchissement des anciennes identités.
> Aucune reprise réseau/duties ni finalité commune démontrée par ce lot ; aucune image reconstruite, suite antérieure rejouée ou ancienne admission retargetée. Campagne privée, consensus/WAN/charge, cycle des validateurs, opérations prolongées, bridge/lancement, CI et revue indépendante restent ouverts. Historique intégral conservé ; production non qualifiée, goal actif.

> **16 octobre 2026 — Correctif de démarrage et essais de pruning ARM64/AMD64.**
> [Rapport et preuves](qualification/pruning-operation-20261016/README.md).
> Source `68d64a5` : les 31 refus attendus passent sur les deux architectures ; le même objet de test reproduit le défaut avec l’ancien Core. Un seul objet est remplacé parmi 143 membres du Core, les 142 autres restent identiques. Aucune ancienne suite ni image reconstruite.
> Quatre essais SST (deux architectures, tmpfs puis volume Linux), chacun avec 64 transactions, 65 536 SSTORE et 63 états historiques supprimés : gain apparié de 4 934 734 octets SST et 4 935 680 octets alloués aux fichiers. Blocs, reçus, états conservés et engagements vérifiés. Les essais sur volume réutilisent les binaires, sans compilation ni test de démarrage rejoué. Ces gains ne garantissent pas la réduction du backing Docker/APFS, la capacité NVMe ou la résistance à une perte électrique.
> Les anciennes admissions gardent leurs identités. Restent ouverts : nouveau Core Native et assemblage Nano/admissions, campagne autorisée puis restauration/duties/finalité, consensus WAN/charge et cycle des validateurs, pruning prolongé, bridge/lancement Base–NANO, CI et revue indépendante. La CI source est refusée pour facturation avant toute étape. Historique intégral conservé ; aucun EIP/fork ni production promu. Goal actif.

> **16 octobre 2026 — rétention RPC et chemin Doppler par hash : preuves ciblées.**
> [Rapport et artefacts](qualification/pruning-base-hash-20261016/README.md).
> Docker ARM : nouveau mode dédié, 3 scénarios, 112 vérifications et 20 refus attendus ; vraie rétention before3→5/ancre6, simulation concurrente, réouverture et engagements externes. Un M/CXX/lien/loader ; Core bd11 inchangé, aucune image construite ni ancien groupe rejoué. Nouveau test du commit 583c5895 ; Mac natif/AMD non rejoués pour ce mode.
> Base public : 10 lectures qualifient la surface observée des sélecteurs par hash ; puis une création Doppler synthétique via le wrapper strict réussit en 12 lectures, adresses concordantes, gas 10 912 762. Aucun wallet, signature, transaction envoyée ou fonds. L'estimation par hash reste une extension hors EIP-1898.
> Le wrapper de preuve termine ensuite exit1 sur JSON.stringify(BigInt), après sauvegarde des rapports réussis : échec console conservé, contenu vérifié séparément, aucun rejeu. Les termes réels, le bridge, l'exécution historique arbitraire/reorg réelle et la production ne sont pas qualifiés.
> Pruning opérationnel/reclaim SST, campagne privée à trois propriétaires, duties/finalité commune, consensus/WAN/charge, CI complète et revue de sécurité externe restent ouverts. L'accord d'audit antérieur n'autorise pas la nouvelle campagne. Tout l'historique ci-dessous est conservé.

## Test RPC courant qualifié sur les trois environnements — 16 octobre 2026

La variante courante RocksDB du test RPC (`3f3dc2bc…`) passe maintenant Linux ARM64 et AMD64 : chaque essai réalise une M indépendante, une compilation, un lien au Core conservé, un `ldd` et le seul sélecteur ciblé ; cinq groupes, 120 checks et 58 refus attendus passent par environnement (M=MD : 463/461 dépendances). La réussite Native antérieure est réutilisée sans la rejouer. Les Core à 143 membres, les sept arbres source de base `bd11…`, le manifeste `d30e…` et les trois admissions de code restent conservés.

Le refus ARM01 avant M/CXX reste dans l’historique, avec son point précis non instrumenté ; ARM02 valide le montage sans copie et ses permissions avant le worker. Les tests utilisent seulement leurs DB synthétiques `/tmp`, nettoyées ensuite. Le probe de la garde ressources passe avec trois échantillons de démarrage, sans coordinateur lancé ni chemin d’arrêt exercé. Aucun ancien test, Core ou image n’est reconstruit. Ce lot ne qualifie pas HTTP, CMake global, tout le SDK/loader, ni une campagne fraîche. Les échecs historiques et la facturation CI restent conservés ; nouvelle campagne sans autorisation, reprise des devoirs, finalité commune et production demeurent ouvertes.

[Sources et preuves](qualification/rpc-current-linux-20261016/README.md). Goal actif.

## 16 octobre 2026 — Cohérence Doppler corrigée et garde de ressources vérifié

43 cas ciblés distincts passent, dont 18 nouveaux : 33 Doppler/transport (32 au premier passage puis le seul test corrigé), 8 de politique ressources et 2 de sélection CI. Le premier échec BigInt d'assertion reste conservé. Trois incohérences sont corrigées : configuration mutable, reçu mutable et lectures de blocs mélangées. Transport borné, méthodes fermées, selectors hash canoniques sans fallback ; estimateGas exige une extension fournisseur dont le support réel Base reste non qualifié. Nouveau workflow Doppler verrouillé/YAML parsé ; aucun PASS GitHub CI n'est déduit des tests locaux.

Le probe réel prend trois mesures Mac/VM Docker/VPS : seuils de démarrage passés, environ 6,4 Gio de disque libre VPS. Conteneur de mesure supprimé, volume existant RO conservé, aucun fichier de données lu, aucun nœud/DB/seed/signature ou campagne lancé. Le garde reste soumis aux permissions du launcher ; ses 8 tests utilisent des enfants injectés. Ces mesures courtes ne prouvent pas sept heures de tenue. Zéro build Core/image et sources C++ bd11, manifeste d30e/admissions inchangés.

La nouvelle campagne privée et ses transferts attendent toujours l'autorisation dédiée ; l'ancien audit de 3,8 Ko ne l'autorise pas. Reprise/finalité commune, cycle/sanctions, partitions/WAN/charge, variante RPC RocksDB Linux, release/CI et revue indépendante demeurent ouverts. Aucun bridge ou backing Base–NANO n'est ajouté ; finalité source, retraits, conservation de l'offre, Doppler et visibilité à T0 restent à finaliser. V37/campagne08 FAILED, journaux Mac absents/NAS refusé et tous les historiques sont conservés. Aucun EIP/fork/profil ni production promu ; CSV/RPC inchangés. [Preuves et limites](qualification/doppler-resource-guard-20261016/README.md). Goal actif.

## 16 octobre 2026 — Helpers courants reliés ; campagne et production non qualifiées

Les cinq helpers Native, cinq ARM et quatre AMD passent : 14 M indépendants, 6 CXX, 14 liens et 14 refus d’arité attendus, plus neuf ldd Linux ; sans Core/image rebuild ni anciennes suites rejouées. Les comparaisons canoniques/remappées restent distinctes de l’égalité littérale MD. Native vérifie les huit programmes/Core143, 3731 fichiers, vingt fixtures Git et six aliases loader prospectifs ; historique absent non récupéré, dépendances directes seulement. Les refus de collecteur, démarrage, stabilité de nouvelles copies et aliases sont conservés.

Source bd11 sans overlay ; bundles Linux prêts et trois admissions de code émises pour 24 actifs (9 Native, 8 ARM, 7 AMD), structures/locateurs validés. Manifeste de sources antérieur d30e inchangé ; prérequis logiciels clos par reçu séparé, sans circularité. Spec complet non validé, campagne non autorisée. Les nouvelles clés synthétiques/transferts privés attendent une autorisation dédiée. Aucune campagne, DB, signature, reprise de duties ou finalité commune n’est exécutée ; les vrais 33 P/W/66 réouvertures, rollback, cycle complet, journaux Mac/NAS refusé, V37 échoué, CI bloquée par facturation et revue de sécurité indépendante restent ouverts. Aucun EIP/fork ni qualification globale SDK/DAG/CMake/loader/production n’est promu. [Sources et preuves](qualification/current-runtime-assemblies-20261016/README.md).

## 16 octobre 2026 — Acteurs publics avant réservation et préparation Linux des journaux VPS

La preview unsigned Native passe sur un seed PUBLIC : 1 TU, M=MD 2539 et Core 143 inchangé ; 65 identités validateurs + 64 retraits comparés au vrai genesis non réservé, 3 refus et 0 signature. Le P2P Native courant est réutilisé après rapprochement sélectionné, sans compilation/lien/exécution ni qualification du SDK/DAG entier.

Le launcher prépare les 33 paires Linux 32–64 pour le VPS. Les 6 syntaxes initiales et les 3 tests purs/guard finaux passent ; le lecteur modifié justifie le second passage. Le probe physique public passe : 65 paires / 131 fichiers, sélection 33/67, 1 corruption refusée, inventaires conservés et conteneur absent, 0 DB/seed/signature/transfert privé. Il qualifie seulement ce lecteur synthétique, pas les SST, 33 vrais P/W, 66 réouvertures C++ ou la portabilité ARM Linux→AMD Linux. Les échecs de staging, ctime, collecteur et probes 01/02 restent conservés ; les autorités physiques restent distinctes.

Les 24 admissions, autorisations privées de la campagne fraîche, reprise, duties, finalité commune, cycle complet et production restent ouverts ; goal actif. Aucun EIP/fork ni loader/DAG/CMake global promu ; historiques intégraux et portées antérieures conservés. [Sources et preuves](qualification/runtime-preparation-20261016/README.md).

## 16 octobre 2026 — Nano lié dans trois environnements (deux architectures CPU) ; reprise réseau encore ouverte

Les Nano courants passent deux smokes CLI sur Mac natif, Docker ARM et Docker AMD64 : aide réussie et refus d’arité RPC attendu, avant DB/réseau. Native compile trois TUs et vérifie Core 143/142 puis RPC causal 5 groupes/120 checks/58 refus. ARM compile seulement Main ; AMD le réutilise après un vrai M=MD. Les deux Linux relient chacun Nano au Core RPC déjà qualifié, sans rejouer les suites RPC. Le tail ARM dérive d’un autre consommateur canonique nano_core retenu et des déclarations Nano actuelles, sans configure CMake complet.

Le test adapté à RocksDB est qualifié uniquement sur Native ; les preuves RPC Linux gardent leur ancien test. Les échecs Native01 (API de compilation) et ARM01 (nom de contrôle, avant programme projet) restent conservés, avec leurs fences et nettoyages bornés. Les historiques ci-dessous restent intégraux. Ce lot ne qualifie pas globalement loader/SDK/DAG/CMake, test adapté Linux ou HTTP ; les preuves historiques gardent leurs portées. Les 24 admissions, la reprise runtime, la restauration, les duties, la finalité commune et la production restent ouvertes ; goal actif.

[Identités Nano/Core, sources et preuves](qualification/complete-nano-cross-arch-20261016/README.md).

> **15 octobre 2026 — Coordinateur fresh Mac/Linux : tests ciblés et transport synthétique validés, production non qualifiée.**
>
> Le [lot Mac/Linux](qualification/fresh-campaign-mac-linux-20261015/README.md) livre 8 overlays du launcher existant. Root a passé 8 contrôles de syntaxe puis 7 nouveaux tests purs et le guard avec 3 refus avant seed/sortie/réseau (phase de tests : 9 486 ms ; 46 MJS + 2 Perl). Le probe physique passe séparément en 27 770 ms : 4 syntaxes Perl, 3 lecteurs, 3 refus spécifiques, 16 paires/64 fichiers synthétiques, 128 comparaisons de contenus préalablement connus et 256 identités distinctes ; 38 conteneurs vérifiés absents, 8 volumes conservés. Les anciennes autorités Node/Docker et les nouvelles copies de sources restent strictes.
>
> **Ce PASS physique qualifie uniquement le transport synthétique : ni les 65 journaux/tokens de consensus, ni le bootstrap C++, le coordinateur complet, les duties ou la finalité commune.** Aucun binaire du projet, seed/export, nouveau domaine, signature, VPS ou build d’image n’a été utilisé. Le premier wrapper du probe, refusé en revue source pour une autorité non ancrée, n’a jamais été exécuté ; son successeur conserve les anciens pins sans utiliser l’admission privée précédente.
>
> Préflight 01 refusé avant Node et lot 02 FAILED restent conservés (test pur réussi, ctime du nouveau fichier synthétique différent, cause physique inconnue). Le guard 03 passe séparément après admission stable des seuls nouveaux fichiers. AMD courant attend toujours une autorisation dédiée. Reprise effective/finalité commune des trois propriétaires, cycle long/inclusion/sanction/retraits, WAN/ressources, release/CI/revue indépendante et Base–Nano/bridge/Doppler demeurent ouverts. Goal actif ; aucune production ou compatibilité EIP/fork/profil promue. CSV/RPC inchangés ; quatre corps historiques intégralement conservés.

> **15 octobre 2026 — API validateur et logiciels courants qualifiés sur Mac natif ; AMD encore ouvert.**
>
> Le [lot natif](qualification/validator-read-native-20261015/README.md) passe avec5 compilations/M==MD, archive143/142 inchangés, quatre liens Mach-O arm64 et une seule exécution du nouveau test :5 groupes,9 statuts,21 lectures directes/16 refus,2 signatures BLS de fixture,0 API/audit,14 112KiB RSS,516ms. Les cinq fichiers Core/programmes correspondent aux engagements puis reçoivent une première admission9 après3 observations≥2s ; aucune ancienne autorité rafraîchie ni ancien test rejoué. Aucun runtime, transport HTTP natif ou finalité n’est qualifié par ce test. Les preuves ARM et du vrai superviseur local07 restent séparées et conservées.
>
> AMD reste en attente d’autorisation dédiée et d’audit de ses anciens schémas/provenances ; le refus de revue automatique a précédé toute lecture rejetée. Aucun build/test AMD courant n’est revendiqué. Reprise/duties/finalité commune, cycle long, inclusion/sanction, WAN/ressources, release/CI et revue indépendante, Base–Nano/bridge/Doppler restent ouverts. V37/campagne08 et tous les FAILED antérieurs demeurent. Aucun EIP/fork/profil ni production promu ; CSV/RPC et quatre corps historiques intégralement conservés.

> **15 octobre 2026 — Superviseur local Linux réellement qualifié, production toujours ouverte.**
>
> La [campagne07](qualification/local-supervisor-linux-20261015/evidence/preparation-07-real-local-supervisor-pass.json) passe : deux vrais Nano, parent/exécutable P2P contrôlés dans `/proc`, identité IPC, deux arrêts sains, HTTP200 et404 attendu, quatre captures complètes de16 paires chacune, témoins indépendants et14 autorités Linux vérifiés jusqu’au fence final. Le vrai RPC du superviseur accepte l’action synthétique de proposer-slashing et lie le reçu à ses hashes ; ses deux signatures d’action ne constituent pas un total audité de signatures runtime. **Inclusion canonique, sanction économique, cycle complet et finalité multiowner ne sont pas qualifiés.** Nouveau propriétaire synthétique, aucune restauration V37 ni exécution VPS privée.63 anciens matériaux et16 aliases conservés ; aucun build d’image/core, profil/clock/limites inchangés, conteneurs possédés arrêtés.
>
> [Sources et historique](qualification/local-supervisor-linux-20261015/README.md) conservent tous les FAILED01–06 et le refus de source non exécuté : cause/fichier04 inconnus, défaut LF05 et assertion de profil06 corrigés sans normaliser d’ancienne autorité. PréparateurV7/driverV4/helperV2 et revues source sont distingués du PASS réel07. Équivalence indépendante source/ELF P2P UNKNOWN dans les audits de schéma, autorité de bytes ROOT séparée ; incident de deux snippets de métadonnées build signalé par un auditeur, sans clé/actif opérationnel ni fait dérivé.
>
> Le build ARM de trois consommateurs et la revue source du registre restent bornés ;100K/1M synthétiques ne qualifient pas autant de nœuds. Reprise/duties/finalité de la candidate sur trois runtimes, cycle long, consensus WAN/ressources, release/CI/reproductibilité, procédures/supervision et revue indépendante, Base–Nano/bridge/Doppler restent ouverts. V37/campagne08 restent FAILED. Aucun EIP/fork/profil activé ni statut production promu ; CSV/RPC et les quatre corps historiques restent intégralement conservés.

> **15 octobre 2026 — Audit historique à froid complété sur trois runtimes ; pilote lifecycle implémenté, cycle réel encore ouvert.**
>
> Après autorisation humaine explicite, la configuration privée existante de3788 octets a été transférée inchangée et l'audit VPS exécuté sur place, avec retour des seuls résultats typés. Les audits natif Mac, Docker Mac ARM64 et Docker VPS AMD64 concordent sur34 champs calculés hors owner :178 blocs signés rejoués,507300 octets d'archive,head196 et finalité epoch20. Le replay VPS dure13707 ms ; les copies physiques et originaux gardent leurs vérifications strictes. Zéro nouvelle signature et aucun démarrage de runtime. Cette preuve historique à froid3/3 remplace l'état antérieur2/3 pour ce seul contrôle ; elle ne transforme pas la campagne08 FAILED ou V37 en réussite et ne prouve pas la reprise opérationnelle. Les trois runtimes restent sur deux hôtes physiques. NAS refusé/hors périmètre.
>
> Sept sources JavaScript implémentent le pilote du cycle frais : dépôt/activation, sortie volontaire, slashing distinct, arrêts/restaurations, inclusions SSZ authentifiées et paiements. Sept groupes protocolaires passent avec55 refus attendus ; le premier collecteur ROOT reste FAILED sur son format de résumé. Le guard séparé passe1 test/3 enfants en372 ms, sans rejouer ces sept groupes ;43 copies publiques et fences stricts9+SHA passent. Ces preuves ne lancent aucun outil C++, nœud ou campagne privée. Le runtime reste désactivé par défaut ; l'autorisation du seul audit historique n'autorise ni la nouvelle campagne privée VPS ni les transferts cross-owner. Le cycle réel conserve64 epochs de tenure et au moins325 epochs activation→paiement complet, environ4h20 plus files/délais.
>
> Prochaine qualification : vrai superviseur Nano→soumission RPC→nano-p2p après arrêt/réouverture, puis cycle complet et finalité commune effective. Cohérence RC toutes architectures, partitions prolongées/asymétriques, WAN A, ressources, CI, audit indépendant et lancement Base/bridge/Doppler restent ouverts. CI précédente38772724674 refusée avant tout step pour billing/spending, aucun PASS CI revendiqué. Aucun statut EIP/fork, profil EVM ou niveau de production n'est promu ; CSV EIP et documentation RPC inchangés. [Sources et preuves du lot](qualification/lifecycle-runtime-cold-20261015/README.md). Les quatre corps historiques restent intégralement conservés ci-dessous.

> **15 octobre 2026 — Deux composants supplémentaires qualifiés sur Docker ARM64 uniquement.**
>
> Le shim lifecycleSubmit inchangé est exercé par un vrai serveur HTTP loopback et un vrai enfant : 5 groupes, 11 requêtes HTTP, 6 enfants, un passage exec vers un daemon contrôlé et 5 refus ou arrêts attendus. Même binaire de test au second essai, sans recompilation ni link ; 10495 ms, pics RSS distincts parent/enfant de 14372/17480 KiB. Le premier essai reste FAILED ; une sonde séparée constate /tmp noexec et un refus exec126, sans prouver son point de throw exact. Le second essai utilise un tmpfs exécutable privé. Le parent est synthétique et le daemon contrôlé : ni vrai superviseur Nano, ni vrai nano-p2p, ni admission cryptographique de l'opération ne sont qualifiés. Ce HTTP concerne le protocole du shim, pas le transport de l'API de lecture validateur.
>
> Le nouveau test d'observation HEAD authentifié passe 3 groupes, 4 cas positifs et 9 refus ; 4 signatures de fixture et 5 lectures d'archive, 239 ms. Un seul TU et link sont produits ; les anciens tests de politique et de replay long ne sont pas rejoués. La découverte HEAD null reste limitée à l'observation RO non signante après les closures indépendantes P/W ; les requêtes d'action et terminales gardent leur HEAD exact. Aucun audit privé ni runtime n'est exécuté par ce test.
>
> Ces preuves ARM64 ne qualifient pas les composants natifs Mac ou AMD. Les contrôles réels M/MD, SDK avant/après, matériaux déjà admis et absence du conteneur sont conservés. Le guard JavaScript d'indisponibilité du runtime lifecycle reste inchangé ; le vrai passage superviseur→shim→P2P, la campagne privée et le cycle complet restent ouverts. Le profil réel reste inchangé : minimum 325 epochs après activation, environ 4 h 20 plus files et délais ; aucune tenure n'est raccourcie.
>
> Autorisation humaine de transfert privé VPS toujours en attente, audits à froid PARTIELS 2/3 ; V37 et campagne08 restent FAILED, aucune restauration des originaux revendiquée, NAS refusé/hors périmètre. Le blocage CI billing/spending demeure OPEN et n'est pas un PASS CI. Aucun statut de fork/EIP, profil EVM activé ou niveau de production n'est promu ; CSV EIP et documentation RPC inchangés. Les corps historiques ci-dessous sont conservés intégralement.

> **15 octobre 2026 — Objectif actif, preuves fonctionnelles ARM limitées.**
>
> Docker ARM64 : la fonction C++ de lecture validateur passe 5 groupes, 21 lectures et 16 refus attendus ; 2 signatures de fixture, aucune signature créée par l’API. Archive core de 143 objets, un recompilé et 142 inchangés ; le test causal utilise le même objet de test. Le transport HTTP n’est pas testé.
>
> Les outils synthétiques passent sur ARM64 : politique d’action 3 groupes/12 refus/3 signatures de fixture ; replay canonique de 1026 blocs réellement signés, 4 groupes/9 refus, 125 prunings, 10783 signatures de fixture, en 95215 ms. Pic de 33 records ; les mesures SSZ et de réserve ne sont pas une mesure RSS. L’échec de compilation GMP V1 sans test est conservé ; le PASS V2 est distinct.
>
> Les 8 groupes JavaScript purs passent sans réseau ni transfert privé. Ces preuves de fonctions et composants ne démontrent ni audit privé réel, ni soumission runtime, ni cycle long ; le shim Submit/Job est un lot suivant OPEN.
>
> VPS privé : autorisation humaine toujours en attente ; audits à froid conservés PARTIELS 2/3. V37 et campagne08 restent FAILED ; les originaux Mac ne sont pas récupérés et le NAS est refusé/hors périmètre. CI 38755615669 est refusée avant jobs pour billing/spending ; ce n’est pas un PASS CI.
>
> Le cycle frais complet garde le profil réel : au plus tôt 325 epochs après activation (environ 4 h 20, plus files et délais) ; il n’est pas exécuté. Aucune promotion de fork/EIP ou de production ; CSV EIP et documentation RPC inchangés. Tout l’historique intégral suit sans modification.

# Partition et rotation — qualification à froid partielle2/3

Objectif ACTIVE. Le présent lot qualifie deux audits à froid sur trois et un composant FS V7 ; il ne clôt pas la production. La campagne08 reste définitivement FAILED : exit1 après1188201ms, sans timeout, au premier auditeur RO natif. La précondition immédiate est identifiée : répertoire0755 au lieu des0700 requis ; les refus de métadonnées ultérieurs gardent une cause physique UNKNOWN. Ses observations acquises sont conservées : deux avancées communes initiales et trois échantillons stables ; coupure de102s entre slots125 et142/142/141, franchissant la frontière128 ; puis trois avancées communes aux epochs18/19/20 et trois échantillons stables, sans conflit de finalité observé. Elles ne remplacent pas le résultat global refusé.

Deux audits à froid distincts du candidat C++c016V2 sont qualifiés dans leur périmètre : natif (2843ms) et Docker ARM03 (3683ms C++,13866ms wrapper). Ils rejouent178blocs/507300octets d’archive sur copies physiques indépendantes, avec closures P/W complètes et pins externes, poids32/32 et comité tourné authentifiés. Leurs35champs calculés égalent exactement ceux de l’autre owner sauf owner ; les originaux complets, copies, sources, logiciels et inputs gardent leurs vérifications strictes. Aucun nouveau runtime ni signature d’audit n’est créé. Le V3 natif est une preuve diagnostique supplémentaire ; il ne remplace pas le candidat c016. Cette qualification est PARTIELLE2/3.

L’audit à froid réel VPS/AMD n’est pas exécuté. Le transfert de3,8Ko de configuration privée reste soumis à une autorisation humaine explicite : la revue automatique l’a refusé et la question asynchrone est encore en attente. Aucun retry ni contournement n’est autorisé par ce brouillon. Les petits tests Perl synthétiques ARM et AMD passent (2665/3595ms), avec leurs deux petits reçus publics : trois scénarios plus résumé,22lignes de matériel gardant neuf chaînes, aucune DB/signature/réseau. Le reçu AMD est une transcription numérique filtrée d’un résultat déjà retourné ; aucune nouvelle lecture VM n’est effectuée ici. Ces tests sans DB, signature ou réseau ne qualifient pas l’audit de la capsule VPS. Aucune configuration privée, map, SSZ, capsule ou journal n’est publié.

Les refus stricts copie1–3, l’échec Perl avant C++ et le refus de fence du premier test Perl restent conservés. La conversion de chaînes numériques JSON::PP est reproduite et corrigée ; la cause physique des variations de métadonnées demeure UNKNOWN. Le lanceur V7 à répertoire JSON portable est gelé et revu source seulement : aucune campagne entière n’a été exécutée avec lui. Son nouveau test FS ciblé passe réellement : six lignes/cinq cas/quatre refus sur dix fichiers opaques totalisant225octets,13349ms wrapper/13252ms consumer, anciennes pins strictement inchangées. Cette preuve de composant ne qualifie ni le montage Docker réel, ni le C++, ni le caller entier. Le core, les images existantes, les clocks et le profil Capella/Shanghai ne changent pas.

La campagne07 conserve son PASS de récupération synthétique distincte ;01–06 et08 restent FAILED. V37 et la récupération originale restent FAILED/non qualifiés ; les journaux Mac originaux ne sont pas récupérés. Le NAS est explicitement refusé/hors périmètre et n’est pas consulté. CI/billing, revue de sécurité externe, cycle complet des validateurs, partitions asymétriques prolongées, bridge/Doppler et production globale restent OPEN. Les statuts EVM/EIP, le CSV et l’API RPC ne sont pas promus.

Le prochain point de décision est l’autorisation de transfert VPS, puis son audit à froid et la revue du lot exact. Ce lot partiel ne clôt pas l’objectif de production, n’autorise aucun déploiement public et n’engage aucun fonds. L’historique antérieur est conservé intégralement ci-dessous.

> **15 octobre 2026 — Reprise synthétique Capella qualifiée, trois runtimes sur deux hôtes.** Run07 passe (ROOT 0, 482741 ms, sans timeout/interruption) : deux avancées communes distinctes et trois échantillons stables avant arrêt, restauration VPS des deux racines primaire/witness (33 paires), puis trois avancées et trois échantillons stables avec devoirs de validation repris et même identité de pair. Les tokens witness externes, l’histoire canonique et l’ancêtre finalisé, les trois captures à froid (16/16/33 paires), tous les fences finaux logiciel/anciens inputs 9+SHA/nouveaux octets et l’absence courante Mac/VPS/natif/tunnel sont confirmés par les petits reçus publics ROOT. **Portée limitée à cette reprise synthétique neuve** ; les échecs01–06, V37, la restauration des originaux et la production restent ouverts.
>
> Outils de reprise Capella livrés : Le captureur arrêté authentifie les checkpoints hot/cold/fragments, y compris v5, et les engagements primaire/témoin sans signer. La restauration associe les signataires automatiques d’index nul à une autorité publique indépendante keyEnv/clé publique ; la copie explicite à deux racines conserve séparément chaîne/journaux et témoins. Arrêt, quiescence, reçu et engagements externes restent requis.
>
> Mac natif, Docker ARM64 et Docker AMD64 : deux CTests PASS par environnement, dix captures (six acceptations/quatre refus), quatre signatures de préparation et zéro signature de capture. RSS du processus parent : 21472/22224/23608 KiB ; RSS des enfants non mesuré. JavaScript : 67 tests Mac et 86 VPS ; les 19 cas shell Linux utilisent des fichiers opaques et un double d’audit, sans preuve de DB réelle. Ces vérifications d’outils ne prouvent pas la reprise des duties.
>
> Le transport P2P natif a un build CMake courant distinct ; les binaires Linux sont réutilisés après vérification de leur fermeture source/MMD. Les refus SDK et postflight source restent conservés, sans revendication d’herméticité ni de reproductibilité. Run01 reste FAIL avant runtime (74,455 s) après export/import et audits physiques : dates de 125 répertoires/racine différentes, matériaux enregistrés et octets des fichiers inchangés. Run02 reste FAIL avant runtime (74,855 s) : 25 ctime de fichiers différentes, octets et autres champs identiques. Causes inconnues ; les gardes fichiers restent stricts. Le diagnostic suivant observe des changements sans copie, puis un inventaire entier inchangé avec le même fs.cp après stabilisation ; seule une attente bornée avant la première admission de nouveaux journaux synthétiques est proposée. Run03 reste FAILED avant runtime (90,226 s) : pré-admission stable réussie, puis timeout SCP récursif de 60 s. Aucun runtime ni résultat de finalité/restauration/duties n’est attribué à ces campagnes. Run04 démarre réellement trois runtimes avec identités distinctes et genèse d’exécution commune, puis échoue sur HTTP 404 d’un ancêtre requis, avant toute finalité initiale qualifiée et sans restauration VPS tentée. Le diagnostic source identifie le parent juste sous l’ancre finalisée prunée, non servi par la route limitée au Store retenu ; le correctif Beacon historique V2 et son nouveau sélecteur sont gelés. Les builds natif et ARM, leurs148 paires-M/MD et143 membres d’archive ont passé ; le nouveau CTest ciblé a passé une fois sur chaque environnement (3groupes/4nouvelles lignes,98lectures,361signatures BLS de fixture,0signature de lecture). RSS consommateur natif623904KiB, ARMnonmesuré/null : aucun plafond128MiB ni4MiB n’est qualifié. AMD a ensuite passé build et nouveau CTest (143MMD/membres, mêmes5programmes/archive,4lignes/3groupes/98lectures/361BLSfixture/0signature de lecture), uniquement selon projection ROOT fixe ; les105073ms sont la phase unit entière, pas un temps JUnit. Les3P2P actuels restent compatibles avec L sans recompilation (80sources/35MMD par owner, header modifié absent). La campagne05 termine ensuite FAILED, code1 en466804ms hors timeout : deux avancées communes et3échantillons stables initialement, puis3avancées et3échantillons stables après la phase de restauration VPS. Le fence final refuse un input public précédemment admis ; chemin/champs/cause inconnus. Ces observations ne sont pas promues en PASS de campagne ni en qualification globale de restauration/reprise. Le résumé public ROOT conserve les observations de 33 paires primaire/witness restaurées sur deux racines physiques, les contrôles des tokens witness externes, de l’histoire canonique et de l’ancêtre finalisé, et des devoirs de validation admis avec validateurs/horloges sains à l’arrêt (16/16/33 clés). Aucun conteneur courant de cette campagne ne reste actif sur Mac/VPS ; natif et tunnel sont fermés. Ces observations restent distinctes du verdict FAILED de run05. V7 ajoute seulement des diagnostics publics pour les futurs refus, sans attribuer rétrospectivement run05 ni changer les gardes ; aucune recompilation, image ou répétition de test. Run06 termine ensuite FAILED (code 1, 468731 ms, sans timeout/interruption) malgré 2 avancées initiales et 3 après restauration avec 3 échantillons stables, et audit finalisé hors ligne présent. Son diagnostic final-held-fence vise exactement le chemin public run-06/assets/isolated-domain.json : ctimeNs seule différente ; SHA/octets/8 autres champs ont passé. Cause physique inconnue, terminal-receipts.json non publié, aucun PASS global et aucune attribution rétrospective à05. Les trois petits reçus publics exacts conservent les observations de reprise (33 paires P/W, tokens externes/histoire canonique/ancêtre finalisé), l’absence courante et le code réel du wrapper ; le refus06 reste conservé. Le probe public neuf reproduit ctime variable même sans copie, puis 9+SHA stables après une unique admission (11 échantillons avant admission par méthode idle/copyFile3, octets exacts). Cause OS précise inconnue ; aucune attribution05 ni reclassification06. Le candidat courant V8 (run79ba7e…, MF8e05…, peer4e3abd6c…) est gelé et appliqué : quiescence bornée des six nouveaux exports avant leur première admission, sans réadmettre d’anciens inputs. Les 21 autres fichiers, le C++/CMake et les logiciels restent identiques. ROOT a créé l’autorité07 avec code 0 puis lancé les runtimes. Le reçu public de pré-admission a passé (6 fichiers,4 probes/3011 ms,3 échantillons stables/2007 ms sous30000 ms) et le contrôle immédiat a passé ; les fences finaux ont ensuite tous passé et run07 est qualifié uniquement dans son périmètre synthétique. Le réacteur composé conserve les 825 octets de l’observation scellée antérieure. Pour run04, à l’arrêt, ROOT observe le même checkpoint finalisé époque 2 sur les trois owners, justification époque 3, horloges et validateurs sains, avec propositions/attestations/messages sync effectivement admis. Zéro conteneur courant reste sur Mac/VPS, natif et tunnel sont sortis/fermés ; trois paires de captures sont observées, sans audit canonique complet revendiqué. Ces seuls faits de run04 ne remplacent ni le suivi de plusieurs avancées communes en direct ni une restauration VPS, non tentée pendant run04. Tout nouvel essai exige un domaine réservé distinct.
>
> V37 FAILED, restauration des originaux Mac, NAS décliné par l’utilisateur et anciennes clés arrêtées restent inchangés. CI/facturation, audit de sécurité externe, partitions/reconnexion, rotations, cycle complet des validateurs, bridge/Doppler et production restent ouverts. Profil Capella/Shanghai conservé, sans nouveau fork. Les quatre corps historiques suivent intégralement ; CSV EIP et documentation API RPC inchangés.

> **15 octobre 2026 — Copies de maintenance : admission physique avant RocksDB.** Le garde partagé refuse les fichiers réguliers à plusieurs liens, les liens symboliques et les entrées spéciales avant l’ouverture mutable. La migration vérifie aussi avant la lecture initiale puis lors du passage RO→RW ; le marqueur reste un garde d’usage borné. Racine 0755 et sous-répertoire ordinaire restent permis ; aucun plafond de validateurs, format durable, règle de consensus, fork, frais ou API RPC ne change.
>
> Docker ARM64 et AMD64 : deux CTests PASS par architecture, dix lignes/neuf groupes et 21 appels CLI (3 succès/18 refus), zéro signature. RSS propre 16768/18172 KiB ; contrôleurs d’unité 7123/30844 ms ; consommateur ARM 705 ms, temps consommateur AMD inconnu dans la sortie filtrée. Usage final 212 KiB par volume, sans mesure de pic ni quota dur. La baseline ARM séparée constate deux anciens CLI acceptant un alias supplémentaire ignoré par RocksDB, avec originaux inchangés, sans corruption revendiquée. Les preuves ARM directes et AMD filtrées restent distinctes.
>
> Les refus de préparation/collecte restent historiques ; les quatre ctime de surcouches neuves ont une admission distincte, les anciennes entrées restent strictes. La quiescence jusqu’à fermeture des DB reste requise. Originaux Mac/NAS, V37 FAILED, anciennes clés arrêtées, restore entier, duties/readiness/finalité commune de trois nœuds, WAN, perte électrique, strict 4 MiB, CI/facturation, audit externe et production restent ouverts. Aucun accès privé ou relance du probe 28722. [Périmètre et preuves](qualification/owned-copy-physical-admission-20261015/README.md). Les corps historiques entiers suivent ; CSV EIP et documentation API RPC restent identiques.

> **15 octobre 2026 — eth_createAccessList, preuves publiques ciblées.** Méthode RPC native avec point fixe monotone, état isolé, gaz net de la liste finale, latest/pending/historique/EIP-1898, surcharges et limites opérateur. CMake reconstruit la fermeture des nouveaux headers ; aucune compatibilité binaire inchangée n’est présumée.
>
> Docker ARM64 et AMD64 : deux CTests PASS par architecture ; nouveau consommateur de 7 groupes, 8 lignes et 4 signatures synthétiques, RSS de 23108/23680 KiB. Geth 1.16.7 ARM64 : 16/16 cas et 5/5 surcharges, sans changement du binaire Nano entre ces campagnes. AMD est décrit par les compteurs ROOT filtrés ; aucun différentiel Geth AMD n’est attribué. Le test stack historique Cancun reste une fixture de régression, sans activation du profil.
>
> Anvil 1.5.1 V1 reste FAILED 11/16 : collecte unique sans point fixe, REVERT en erreur RPC et seeds/doublons différents. Dans ce lancement, timestamp EVM 1 malgré header 100 et GASPRICE de 2 gwei par eth_call contre 0 par eth_createAccessList restent documentés. Geth est la référence primaire, sans équivalence globale ni réussite Anvil inventée. Les frais nuls sont refusés avec des codes différents, Nano -32602/Geth -32000, hors des 21 comparaisons strictes. [Sources, résultats et limites](qualification/rpc-access-list-20261015/README.md).
>
> Aucun build d’image, fonds réels, chaîne privée ou reprise opérationnelle. Les corps historiques intégraux sont conservés. Le CSV change seulement Preuves/Reste_a_faire EIP2930 ; statuts, périmètres, priorités et autres lignes inchangés. Originaux Mac/NAS, V37 FAILED, anciennes clés arrêtées, readiness/duties/finalité commune de trois nœuds, WAN, strict 4 MiB, restauration/perte électrique, CI/audit externe et tous critères non démontrés restent ouverts. Pas de qualification RC complète/EIP complet/production ni promotion de fork.

> **Correctif admission durable —15octobre2026.** Quatre CPP possèdent les entrées bornées avant callbacks pour authentifier et persister les mêmes octets. Nouveau CTest final PASS sur Docker ARM64/Mac et AMD64/VPS :9familles/10lignes/57signatures chacune, RSS25964/26280KiB. Causal V1 sur ancien cœur et échecs des harnais V1/V2 conservés ; consumer final V3 différent, sans claim sameobject.
>
> Les blocs qualifiés sont des branches authentifiées retenues, pas une progression de HEAD/finalité. Aucune modification des profils, règles/frais, formats v1–v5, API/layout ou ordre témoin→primaire. Aucun ancien groupe rejoué, aucun build d’image. Les quatre corps documentaires historiques restent intégralement conservés ci-dessous. CSV1559/3198 corrige seulement Preuves/Reste_a_faire de l’oracle de pourboire existant, sans promotion de statut/fork.
>
> **Production toujours ouverte** : journaux privés originaux/V37FAILED/reprise des duties/finalité commune3nœuds, gates de consensus et résilience, CI et audit externe. Les spans de transactions, sync_current et références signer/journal/DB restent hors qualification ownership. Aucune vente, dépense, fonds réels ou déploiement public. [Preuves du lot](qualification/capella-owned-admission-20261015/README.md).

> **Qualification CLI/WAL publique — 15 octobre 2026.** Trois surcouches de tests/CMake qualifient six interruptions SIGKILL cold/fragmented et trois fautes fsync ciblées, sans changement du code CLI de production ni des 143 unités core. Deux compilations et deux liens ont terminé par architecture ; aucun ancien groupe OPS ou de treize scénarios n’est rejoué.
>
> Deux CTests sont PASS par profil Linux : AMD64, 11 lignes / 36 appels CLI / 6 SIGKILL joints / RSS 25372 KiB ; ARM64 sur un nouveau volume local Linux Docker possédé, mêmes nombres et RSS 23804 KiB. AMD a été admis par le finaliseur distinct 64213 sans rejeu, en conservant l’EXIT1 de collecte initial. ARM 84928 est EXIT0 ; ses 68001 anciens matériaux/SHA restent exacts, les nouveaux octets sont relus et volume/conteneurs supprimés. Usage disque maximal observé : 6924 KiB sur 13 observations, sans prétention de pic exhaustif ou quota dur.
>
> L’ancienne campagne ARM Mac bind reste FAIL dans la copie des fixtures DB, cause inconnue ; elle n’est pas qualifiée par le nouveau profil. Les six CTests réellement exécutés comptent donc quatre PASS qualifiés et ces deux échecs historiques. Les refus de préparation, de collecte et le refus 62891 avant volume/tests restent conservés dans le rapport.
>
> Fixtures publiques sans nouvelle signature ni données privées. Ce lot ne qualifie pas la perte électrique physique, les files non vides, la restauration complète des originaux, les duties, la finalité, l’exploitation ou la production et n’active aucun fork/EIP. NAS, V37, anciennes clés arrêtées, audit externe et autres critères historiques restent ouverts. Voir le [rapport et les commandes de sélection CMake/CTest](qualification/bootstrap-maintenance-observed-wal-20261015/README.md). Le corps historique intégral suit sans modification ; le CSV reste byte exact.

## 15 octobre 2026 — Installation et chargement du paquet : Linux ARM64 et AMD64 passés

Quatre sources OPS corrigent le contexte Docker (sources d’outils et manifeste de licences), les cibles CMake construites avant installation et le contrôle de bibliothèques des cinq exécutables installés. Le nouveau selector `--runtime-install-smoke` installe dans un préfixe neuf puis charge cinq points d’entrée sans chemin de base, requête, identité ou clé ; les refus d’arguments sont vérifiés avant leurs opérations. La branche historique avec base/RPC/nœud n’est pas sélectionnée. Aucun changement de consensus, ABI, format durable ou quota de reprise.

Les reçus ROOT publics ARM64 passent la configuration CMake neuve (4,864 s), le SDK/P2P (22,479 s), l’admission indépendante du cache (6,083 s), le build canonique (20,150 s) et le nouveau CTest corrigé 1/1 (enveloppe 3,666 s). AMD64 passe son initialisation publique (140,338 s ; transport 141,497 s), son cache (17,252 s), le build canonique (69,725 s) et le CTest corrigé (2,262 s). Les deux preuves canoniques attestent 143 membres aux octets qualifiés, l’ordre de la recette CMake actuelle, zéro recompilation du core inchangé et cinq compilations/liens de consommateurs ; cinq ELFs installables et le test sont construits. Les points d’entrée sont chargés sans DB/réseau. Résultats AMD filtrés seuls, bindings binaires/logs bruts conservés sur la VM. Containers possédés absents, aucune construction d’image ou runtime privé.

Le premier CTest ARM 8314 reste EXIT8/3,881 s : ROOT confirme PermissionDenied du binaire installé sur tmpfs noexec. Le mode corrigé rend uniquement le tmpfs du test exécutable et garde nosuid/nodev ; il a son reçu distinct sans réécrire l’échec. Le refus cache V1 83262 reste distinct, objet partiel conservé/compteur achevé 0/CPP 0/champ matériel individuel UNKNOWN. Cache V2 76076 refuse à sa relecture finale après 143 objets et 143 MD copiés, champ individuel également UNKNOWN. L’admission ROOT 8971 accepte ensuite indépendamment les 143 objets/MD et 143 recettes après 21419 lignes anciennes+SHA et 21707 relectures ; le contrôleur V2 ne devient pas PASS. Aucun champ historique inconnu n’est réinventé à partir du relevé synthétique du répertoire.

Le snapshot de documentation filtrée et ces commandes dans les environnements de dépendances existants ne qualifient pas le frontend Docker, une image runtime Ubuntu complète, tous les outils ou une installation en exploitation. Aucun résultat du packaging candidate à deux binaires n’est étendu. Le healthcheck RPC n’est pas une preuve de readiness. La livraison Git reste une vérification séparée des sources épinglées.

Aucune résolution de readiness/duties, du délai privé 60000 ms, des refus 85895/82701, du sealer 72046 ou de l’admission metadata 28722 n’est déduite. La cause de 28722 reste inconnue, avant tout nouveau C++. Les critères V37 FAILED, finalité commune de trois nœuds, anciennes clés arrêtées, originaux Mac/NAS non autorisé, WAN A, strict 4 MiB, WAL/powerloss/reindex, histoire/livraison/ascendance des signatures/nonces, native/CI/audit indépendant et production restent ouverts. Aucun fork/EIP/statut des matrices ou du CSV n’est promu. Aucun probe privé ni accès aux logs privés n’est autorisé par ce texte.

Voir le [périmètre d’installation et ses reçus publics](qualification/fullnode-install-loadability-20261015/README.md). Le corps historique ENTIER et l’unique CSV restent inchangés ci-dessous.


## 15 octobre 2026 — Batch Merkle de slots : qualification publique bornée, observation privée non exécutée

Le nouveau batch local détient ses octets/règles/limites avant les callbacks, conserve l’ordre des deux anneaux, rejoue les epochs sans raccourci et vérifie la racine finale par le hasher strict. Aucun cache global, format durable, header public, ABI ou quota ne change. L’oracle manuel compare SSZ et métadonnées, contextes, frontières epoch/historique/sync/eth1, deux tours d’anneaux et padding non puissance de deux ; trois racines publiques indépendantes et un état officiel post-slot contrôlent aussi les hashers observé et non observé. Refus, annulation et propriété des entrées sont testés.

Une seule nouvelle invocation publique par architecture passe trois groupes + résumé (4 lignes), zéro CLI/base et deux signatures de fixtures synthétiques publiques : ARM 4,425 s / 18056 KiB ; AMD 16,601 s / 18028 KiB, plafond 131072 KiB et 180 s inchangés. 402 transitions sont exécutées de chaque côté de l’oracle ; 348340 paires BeaconState/batch sont comptées contre 742045 pour la référence stricte. Ces compteurs excluent certains helpers inchangés et ne mesurent ni CPU total ni gain chronométrique/privé. ARM est relu sur log public ; AMD sur résultats ROOT filtrés seuls. Les anciennes campagnes 9 lignes / 11 CLI ne sont pas rejouées.

Les builds réussis (ARM 21,689 s ; AMD 56,573 s) compilent deux objets core et un consumer, reprennent 141 membres inchangés sur 143 et relient deux ELFs ; l’objet CLI et ses dépendances restent identiques. Sept fixtures publiques sont épinglées. SDK/inputs et absence des containers possédés sont vérifiés. Le refus du sealer ARM 72046 reste EXIT1, champ divergent inconnu : une admission indépendante de l’enveloppe courante permet ensuite le test, sans transformer le refus en succès.

La tentative ROOT 28722 s’arrête en admission des métadonnées (EXIT1), avant toute tentative collecteur/scanner/producteur et sans dispatch du probe. Aucun nouveau C++ privé n’est exécuté ; cause inconnue, aucune mesure de vitesse privée et aucune défaillance C++ déduite. Le stdout ROOT filtré seul est conservé. Les résultats publics ne closent aucun gate privé. Les refus privés antérieurs 85895 et 82701 au délai global de 60000 ms restent historiques ; pour 82701, préparation murale 59488 ms et travail partiel inconnu, sans identification de fonction CPU chaude. Ce résultat public ne résout pas ce délai et ne qualifie ni readiness, duties opérationnelles, finalité ni production. Aucun fork/EIP/statut des matrices/CSV n’est promu. V37 FAILED, finalité commune de trois nœuds, anciennes clés arrêtées, originaux Mac manquants/NAS non autorisé, WAN A, strict 4 MiB, WAL/powerloss/reindex, histoire/livraison/ascendance des signatures/nonces et audit externe restent ouverts.

Voir les [sources et résultats bornés](qualification/capella-duty-slot-merkle-20261015/README.md). Ce rapport fixe les sources du lot par leurs hashes ; la livraison Git se vérifie séparément. Le corps historique ENTIER et le CSV restent inchangés ci-dessous.

## 15 octobre 2026 — Préfixe de projection partagé et observation bornée : qualification publique ARM/AMD

Le nouveau build cohérent conserve l’archive de143 membres :30 objets core recalculés,113 réutilisés byte-exacts, deux TU de CLI/consumer et deux liens, sans nouvelle image. Builds ARM69,097s et AMD246,366s réussis. Une seule invocation publique par architecture passe huit groupes+résumé, onze enfants CLI (2 acceptés,9 refus) et un événement d’annulation : ARM4,901s/24636KiB ; AMD18,172s/25872KiB, plafond131072KiB inchangé. ARM est relu sur log public brut ; AMD sur le seul résultat ROOT filtré. L’oracle strict compare SSZ/métadonnées/committees/proposer/sync et mesure85 transitions calculées contre243 pour les chemins indépendants par architecture ; cela ne mesure pas un gain de durée privée.

Le partage exige le même parent authentifié et le profil entier, respecte les frontières epoch et slot précédant le bloc, conserve l’ancêtre target distinct et les quotas. Une projection définitivement indisponible s’arrête sans boucle vide ; callbacks/cancellation restent refusés. Durées par phase et comptes partiels restent explicites, inconnus/null si non atteints. Aucun état projeté n’est transformé en HEAD signé ni persisté.

Le refus privé antérieur 85895 reste historique : délai GLOBAL de 60000 ms, 25176 KiB, 33 clés configurées, résultat des contextes inconnu/non produit (null) ; son temps propre n’avait pas été mesuré. Une seule observation du nouveau candidat (ROOT 82701) reste EXIT1 au même délai : 512 ms avant les duties, 59488 ms de préparation murale, pic 25876 KiB sous 131072 KiB. Les compteurs achevés retiennent 6504 transitions HEAD et 6504 préfixes target/block réutilisés chacun, mais incompleteWorkUnknown=true interdit d’en faire un total exact du travail. Contextes et affectations restent inconnus/null ; la fonction CPU chaude n’est pas identifiée. Inputs, inventaires complets, journaux et quiescence avant/après sont déclarés conservés, conteneur absent. Le partage ne résout pas ce délai et ne qualifie ni readiness ni duties opérationnelles. Aucun signer/provider, duty ou signature opérationnelle ; seules les issues ROOT typées filtrées sont publiques.

Production, finalité commune de3nœuds, V37FAILED, originaux Mac/NASnonautorisé, anciennes clés arrêtées, WAN A, WAL/powerloss/reindex, histoire/livraison/ascendance des signatures/nonces, audit de sécurité externe et autres critères historiques restent ouverts. Aucun fork/EIP/CSV promu. Voir les [sources et résultats bornés](qualification/capella-duty-prefix-20261015/README.md). Ce rapport fixe les sources par leurs hashes ; la livraison Git se vérifie séparément. Le corps historique ENTIER et le CSV restent inchangés ci-dessous.

## 14 octobre 2026 — Préparation de duties readonly qualifiée sur fixtures, reprise non autorisée

Les sources passent une invocation publique ARM64 et AMD64 chacune : huit groupes + résumé par architecture, dix-huit lignes JSON, vingt-deux appels CLI (quatre observations acceptées, dix-huit refus) et deux événements de cancellation. ARM : 3,595 s / 24248 KiB ; AMD : 12,807 s / 25720 KiB, plafond131072 KiB inchangé. ARM est relu sur preuves publiques brutes ; AMD est rapporté par ROOT filtré, sans export des métadonnées brutes VPS.

L’outil readonly authentifie les CURRENT PRIMARY/WITNESS et l’exécution canonique, prépare les contextes HEAD/epoch cible/block/sync sous budgets finis et distingue observation terminée, contextes complets, éligibilité et nonassignation normale. Aucun signer/provider, runtime ou duty n’est lancé ; aucune projection HEAD n’est persistée. Le rapport ne vaut jamais autorisation de reprise.

Genesis absent/entier1 garde exactement le hash historique ; le nouveau mode JSON entier2 choisit les deux racines MPT canoniques. Hash legacy, littéraux indépendants, fonctions natives, roundtrip/input conservé et types invalides passent dans la fixture courte. Aucun Genesis existant n’est migré. Un seul objetchain remplace l’ancien dans l’archive143, le consumer est compilé et les deux ELFs sont réellement reliés ; le CMake livré n’est pas configuré/installé dans ce lot. Ancien refus tardif du contrôleur ARM et premier essai0,223s/0row/0CLI restent historiques, sans inventer l’exception perdue.

Duties réelles/finalité commune de trois nœuds, histoire/livraison des signatures, ascendance/nonces, nouveau clock143 et durable-v5 non vide, strict4MiB longue finalisation, powerloss/WAL/reindex, natif/install/nœud complet, WAN A et audit de sécurité externe restent ouverts. Originaux Mac/NAS non autorisé, V37FAILED et bridge/Base/Doppler restent inchangés. Shanghai actif ; aucun fork/EIP/CSV promu, production non qualifiée.

Voir les [résultats publics et leurs limites](qualification/capella-duty-readiness-20261014/README.md). Le corps historique entier ci-dessous et l’unique CSV sont conservés byte-exacts.

---

## 14 octobre 2026 — Rattrapage clock-only accompli, reprise opérationnelle non qualifiée

ROOT rapporte un rattrapage d'horloge terminé sur copies VPS arrêtées : une lecture froide préalable, un seul worker puis un retainer froid indépendant après wait/join, en 66,084 s. Le worker publie 1403 engagements PRIMARY/WITNESS appariés ; pics 22784 / 36068 / 26108 KiB, plafond 131072 KiB inchangé. Originaux, références, journaux, peer/configuration et SDK sont conservés ; aucun signing, clé, runtime ou duty.

Qualification limitée à l'ELF historique/archive 142 sous admission de sources 591f, sur résultat ROOT filtré ; données et métadonnées privées restent VPS. Pas de projection HEAD persistée ni qualification du cœur 143, des queues durable-v5, des duties, de l'histoire/livraison des signatures ou de la finalité commune. V37, originaux Mac manquants/NAS non autorisé, WAN A, natif/CI/audit et autres gates restent ouverts. Production non qualifiée ; Shanghai/EIP/CSV et tous corps historiques inchangés.

Voir le [résultat public et ses limites](qualification/clock-catchup-complete-20261014/README.md).

---

## 14 octobre 2026 — Sanction de double proposition : qualification publique limitée

Le candidat source proposer-slashing passe six groupes cœur + résumé sur Docker Linux ARM64 et AMD64 : admission, queue durable/reopen des trois formats, inclusion/sanction, reorg/réinclusion, retrait finalisé et refus. ARM : 11,609 s / 103680 KiB ; lot AMD cœur/réseau : 47,984 s / 102168 KiB. Plafond 131072 KiB, cache zéro explicite et 295 signatures synthétiques par architecture. Six groupes mock réseau et trois programmes de propriétés passent aussi ; aucune propagation WAN réelle n'est prouvée. Ces résultats portent sur les sources indiquées ; production non qualifiée. Aucun fork Ethereum/EIP promu.

Le build ARM assemble 21 objets cœur frais + 122 repris (archive 143) ; V4 ne reconstruit que son consommateur CPP/lien, sans rejouer le réseau. AMD est rapporté par ROOT filtré, logs/hashes bruts conservés VPS. V1 STOP, V2 compilation refusée et V3 cache strict 4 MiB refusé restent historiques.

Profil 4 MiB, natif, WAN A, duties/finalité commune, pending witness/WAL, réindexation, autres domaines historiques et audit de production restent ouverts. V37, originaux Mac manquants et NAS non autorisé sont inchangés. Production non qualifiée ; Shanghai/EIP/CSV et tous critères antérieurs conservés.

Voir les [résultats et limites](qualification/proposer-slashing-complete-20261014/README.md). Le corps historique ci-dessous reste intégral.

---

## 14 octobre 2026 — Journaux configurés : qualification publique et observation VPS limitée

Wrapper V5/banc H3 passent une invocation publique ARM64 et AMD64 chacun : huit groupes + résumé par architecture ; total 18 lignes JSON, 76 appels (8 acceptés, 68 refusés), 10 événements E/S, zéro CTest.

ROOT a ensuite observé une fois les 33 paires configurées VPS sur copies arrêtées : deux lectures froides, pic 23 004 KiB. Assets, originaux/références/copies, SDK et contrôles de quiescence restent conformes ; containers absents. Aucun signing, clé chargée, runtime ou duty. L’audit externe ici porte sur la source et le résultat ROOT filtré ; rapports/tokens privés restent VPS.

Ascendance des digests, livraison de signatures, nonces, clock/reprise, duties/finalité commune et production restent non qualifiés. Journaux Mac manquants ; aucun accès NAS autorisé. Refus antérieurs conservés ; CMake Linux ARM64 génère désormais les cibles et découvre 0/1 test en OFF/ON ; aucune cible compilée/exécutée dans ce contrôle. Shanghai/EIP/CSV et corps historiques inchangés.

---

## 14 octobre 2026 — Observation pure des journaux et du registre qualifiée

Les observations publiques des journaux et du registre de validateurs sont désormais qualifiées sur Docker Linux ARM64 et AMD64. Les nouveaux tests vérifient les formats chaud/froid/fragmenté, les engagements courants et minima numériques, les réservations conservatrices, la recherche de clés dans le registre complet, les copies détenues et les refus/annulations. La régression du signer passe aussi avec ses propres clés et bases synthétiques. Ces résultats ne prouvent pas l’ascendance historique, la livraison de signatures, les nonces d’exécution ni la reprise des duties.

La lecture actuelle du checkpoint VPS a réussi séparément sur copies arrêtées, sans publication d’horloge ni signature. La lecture réelle de tous ses journaux reste à qualifier : l’outil est en correction et ses tests d’E/S sont à produire. Journaux Mac/NAS manquants, V37, finalité commune qui avance, autres scénarios de consensus, qualification native, livraison complète/CI, exploitation et revue de sécurité externe restent ouverts. Profil réseau Shanghai et statuts EIP/CSV inchangés ; production non qualifiée. Les quatre preuves privées autorisées ont été transférées ; les nouveaux engagements et données privés restent sur le VPS.

Voir les [preuves publiques et leurs limites](qualification/journal-head-observers-20261014/README.md). Les critères et corps historiques ci-dessous sont conservés intégralement.

---

Mise à jour du 14 octobre 2026 — diagnostic UTC en lecture seule.

Le CPP pur d496 a été compilé et lié une fois sur Docker Linux ARM64 et AMD64, sans recompilation du cœur. Chaque architecture a exécuté huit nouveaux appels : sept refus exacts et une observation UTC projetée, sans succès forcé. Calcul ARM64 : 8 946 ms / 17 860 KiB ; AMD64 : 43 729 ms / 17 408 KiB. Les preuves AMD sont filtrées et attestées par ROOT in situ ; les logs complets restent sur le VPS.

La nouvelle cible CMake est seulement proposée, non configurée ni installée. La fenêtre startup 2 h / 60 pas et l’horloge publiée ne changent pas. Ces essais synthétiques n’authentifient aucune capture actuelle ; privé, signatures, reprise réelle, V37/NAS, CI billing et audit externe ne sont pas promus. La production reste non qualifiée ; le diagnostic ne démarre aucun runtime ni duty.

---

## 14 octobre 2026 — Archive initiale appliquée à une nouvelle copie VPS détenue

Le CLI V6 qualifié a planifié puis appliqué l’archive initiale déjà signée sur une nouvelle copie intégrale des owner roots VPS. Deux NEXT indépendants ont été conservés séparément et synchronisés avant publication. Réouverture froide et idempotence typée réussissent ; originaux, références physiques, journaux/témoins, identité du pair et snapshot/horloge/queues/exécution sont conservés. Aucun nouveau signing ou duty. Les racines DB anciennes n’étant pas toutes privées, seules les nouvelles feuilles de références/copies sont passées en 0700, avec transformation explicitement attestée ; aucun chmod des originaux. [Statut observé et limites](qualification/vps-owned-bootstrap-archive-20261014/status.json).

Les rapports, capsules et engagements réels restent sur VPS ; seuls les résultats de contrôle sans hashes/pins privés sont rapportés ici. Ceci ne qualifie ni sauvegarde/restauration complète, ni journaux réauthentifiés après reprise, ni UTC courant, grant expiré, duties/finalité commune, WAL interrompu ou production. V37/NAS et tous autres critères/EIP/CSV restent ouverts et conservés ; profil Shanghai inchangé. Diagnostic UTC V2 : source relue seulement, non compilée/exécutée dans ce lot.

## 14 octobre 2026 — Livraison combinée timer et maintenance par répliques

Le CLI V6 inspecte uniquement des répliques physiques privées scellées et revalidées ; il acquiert ensuite le vrai owner Existing/LOCK des copies ciblées. Le banc conserve les engagements distincts et leurs fsync avant publication, les formats chaud/froid/fragmenté et l’idempotence. Deux CTests maintenance/réplique passent une fois sur chaque architecture Docker : 13 lignes wrapper et 3 lignes tiny, zéro nouvelle signature. ARM : wrapper 2 691 ms/23 752 KiB, tiny 17 ms/17 668 KiB ; AMD64 : wrapper 8 987 ms/25 140 KiB, tiny 78 ms/17 920 KiB. Le scénario RO/RW indéfini de ae37 est fermé pour ce chemin d’outil qualifié ; ni absence de writers privilégiés ni écriture concurrente sûre ne sont garanties.

Les deux CTests startup/deadline précédents n’ont pas été rejoués : cœur/archive et trois ELF sont byte-exacts dans le composite. Quatre CTests distincts passent une fois par architecture, huit exécutions réelles au total sur deux lots ; les lignes JSON sont des résultats sémantiques, pas des CTests. Sources finales runtime8e/helpera585/testbf9/CLI4a53/réplique3869/tinyafb6 et CMake fcc de 161 145 octets, réellement configurée, préservent CLI/install/targets et les anciens argv. La capsule standalone deadline50fe, sa CMake a2 non configurée et tous findings V3–V6 restent historiques. [Preuves combinées](qualification/startup-replica-composite-20261014/README.md).

Pas d’application aux copies privées ou de nouvelle signature. UTC courant/grant 2h/60, journaux/floors/duties/finalité commune, WAL interrompu/whole-node backup, V37/NAS, natif/clean/install/CI facturation, audit externe et autres critères demeurent ouverts. Profil Shanghai et matrices EIP/CSV inchangés ; production non qualifiée. Le préfixe deadline antérieur et tous corps ae37 restent intacts ci-dessous.

## 14 octobre 2026 — Budget steady de startup qualifié sur Docker ARM et AMD64

Le runtime accepte un plafond optionnel `clock.maximumStartupElapsedMs`, mesuré en steady time, distinct de l’UTC et du grant de panne. Il contrôle les callbacks synchrones et expire par exception typée ; absence du champ conserve le comportement sans plafond ajouté. Après retour/RAII, ses callbacks retenus deviennent inertes avant les workers. Une publication de préfixe suivie d’expiration conserve les engagements actuels, pas les anciens pins.

Deux CTests réels passent sans retry ni skip par architecture : startup (six cas + résumé) et deadline (cinq groupes + résumé), 13 lignes, zéro nouvelle signature. ARM : 5,764 s ; deadline 5 285 ms/23 764 KiB. AMD64 : 7,098 s ; deadline 5 802 ms/25 600 KiB. Une unité runtime fraîche + 141 objets réutilisés, un test neuf, trois liens ; SDK, sources, raw archive142 et inputs/conteneurs contrôlés. Ancienne V1 jamais compilée/exécutée, réparation V2 des scopes RO/RW de test conservée. Source runtime/helper/test exacts ; CMake de livraison préserve les targets/install maintenance et ajoute seulement la registration deadline. [Preuves publiques](qualification/startup-deadline-20261014/README.md).

Ce plafond coopératif n’élargit pas le grant 2h/60 et ne qualifie ni UTC courant, longue panne, journaux/floors, duties ni finalité commune. Le gate RO/RW concurrent du CLI maintenance à ae37 reste ouvert jusqu’à sa nouvelle qualification ; ces tests n’appliquent rien aux copies privées. Profil Shanghai, EIP/CSV et tous autres critères conservés : natif/install, WAL/backup/V37/NAS, consensus/charge/exploitation, CI facturation et audit indépendant externe restent ouverts. Aucune activation opérationnelle de budget ou production qualifiée.

## 14 octobre 2026 — Lecture RocksDB concurrente : critère ouvert

Le SDK RocksDB natif et le SDK ARM compilé définissent la lecture RO simultanée à un propriétaire RW comme indéfinie. Le préflight du CLI de maintenance actuel fait cette lecture avant sa prise de LOCK dans le scénario concurrent. Ses PASS historiques restent observés ; le refus sûr sous concurrence n’est pas qualifié. Aucune perte constatée ni application aux copies privées. Correction par répliques physiques privées indépendantes et nouveaux tests à livrer.

Trois scopes du nouveau test deadline sont corrigés en proposition V2, avant toute compilation ; runtime/helper inchangés et ancien startup correctement scoped. Aucun budget opérationnel activé. Duties, finalité commune, journaux/UTC/V37/NAS, WAL interrompu, natif/install/CI/audit et tous critères restent ouverts. EVM Shanghai et EIP/CSV inchangés ; production non qualifiée. [Constat et limites](qualification/rocksdb-readonly-concurrency-20261014/README.md).

## 14 octobre 2026 — Maintenance bootstrap qualifiée sur copies synthétiques

Le CLI C++ de maintenance planifie, conserve deux engagements distincts puis applique/reprend une archive initiale signée sur copies arrêtées détenues. Un CTest réel passe par architecture Docker ARM/AMD64 : 13 lignes avec résumé, formats chaud/froid/fragmenté, verrou exclusif, refus de provenance/plans périmés et trois défauts fsync avant publication. ARM : 2 325 ms/23 384 KiB ; AMD64 : 7 076 ms/25 120 KiB. Zéro nouvelle signature. État canonique requis ; absence du seul état historique admissible.

Cœur et CLI inchangés après le premier build ; seul banc corrigé recompilé/relié. Premier échec state0/state1, comptage JUnit conservateur et refus de métadonnées conservés. WAL interrompu de ce CLI, copie/journaux privés, UTC, duties/finalité commune, natif/install/images, V37/NAS, CI facturation et audit externe restent ouverts. Budget steady séparé : proposition relue, non compilée/activée. EVM Shanghai, EIP/CSV et critères inchangés ; production non qualifiée. [Preuves publiques](qualification/bootstrap-maintenance-20261014/README.md).

## 14 octobre 2026 — Raccordement bootstrap et pending qualifiés en laboratoire

Sur fixtures publiques/synthétiques, le raccordement owner existant, entrées authentifiées et reprise avant horloge passe six scénarios de démarrage sur Docker Linux ARM/AMD64. Pending non vide : cinq cas passent par architecture, quatre SIGKILL, deux pertes WAL explicitement simulées, queue/enveloppe et WITNESS conservés, reprise typée idempotente. Trois signatures synthétiques, enfants zéro. ARM : 1 065 ms/22 892 KiB ; AMD64 : 3 768 ms/24 364 KiB.

Candidat final `b6b7c450` : code de production compilé dans le lot `99de4496`, archive de 142 objets ; seul test pending V4 recompilé/relié. Échecs include/Genesis/horloge de fixture conservés. CLI offline V2/banc relus uniquement, non compilés/testés/appliqués. Copie privée, fenêtre de reprise UTC expirée, journaux, duties/finalité commune, V37/NAS, natif, reprise naturelle AMD64, CI facturation et audit externe restent ouverts. Quatre preuves locales non publiées. Profil Shanghai, EVM/RPC, forks, EIP/CSV et critères inchangés ; production non qualifiée.

## 13 octobre 2026 — Archive CAS/témoin qualifiée sur Docker ARM et AMD64

Le candidat C++ V3/banc V4 passe trois CTests sur chaque architecture : conservation CAS/témoin de l’ancre déjà signée, annulation et reprise cold/memo SIGKILL. ARM : 7,143 s ; AMD64 : 27,169 s. Nouveau banc : ARM 637 ms/25 236 KiB, AMD64 2 237 ms/26 520 KiB. Builds cohérents : 141 objets, trois liens. Snapshot exact, formats cold/fragmented, deux engagements suivants, idempotence et refus adverses ; deux SIGKILL par banc. Perte WAL explicitement simulée, panne physique non qualifiée. Échecs et findings antérieurs conservés.

L’application à la copie privée et le raccordement runtime restent ouverts : aucune duty ou finalité commune reprise. Pending non vide/cold, horloge expirée, crash natif, naturel AMD64, V37/NAS, CI facturation et revue externe restent ouverts. Quatre preuves privées locales non publiées. Profil, EVM/RPC, forks, EIP, CSV et critères inchangés ; production ouverte, corps historiques intégralement conservés.

## 13 octobre 2026 — Ancre signée qualifiée, archive PRIMARY absente

Le helper V3 passe ses self-tests réels sur Docker ARM (50 ms, 18 728 KiB) et AMD64 (149 ms, 18 688 KiB) : ancre publique déjà signée, transition complète, 13 refus adverses, cinq contrôles d’archive et annulation typée. Une compilation et un lien par architecture ; aucun cœur recompilé, nouvelle signature ou ancien test rejoué. Refus antérieurs conservés.

Sur la copie VPS arrêtée, l’absence de l’archive signée initiale PRIMARY est mesurée ; l’égalité reste refusée. Sa conservation atomique par CAS avec témoin est non implémentée et non appliquée. Aucun rapport ou engagement privé publié. Journaux, pending, horloge de deux heures expirée, duties et finalité commune restent ouverts. V37/données Mac/NAS sont distincts. Naturel AMD64, image/install, CI facturation, audit externe et production restent ouverts. Tous critères, matrices et corps historiques sont conservés.

## 13 octobre 2026 — Source Capella qualifiée sur ARM, naturel AMD64 ouvert

Candidat : baseline `8be93c69e144ed6e7f8b8a33ecb308c6fe3d4fdd` + six fichiers exacts coreV2 `da39fde2` / testV4 `27667420` (patch `70b062b5`). La copie détenue du state est authentifiée avant certification ; le fingerprint privé bytes/metadata/header/profil évite une validation complète répétée de la même source. Toute différence conserve la validation complète et ses refus ; signatures, admission et journaux restent contrôlés. Douze catégories synthétiques couvrent oracle, alias HOT/COW, framing/annulation, vraie admission de votes et contre-exemples constructeur BAD→GOOD/GOOD→BAD. Le tiny ne qualifie pas cold→hot.

**Docker ARM : neuf CTests réels PASS sans skip** ; audit sémantique des nouvelles sorties `f05c545c`, build `dd0242dd`, total88,53s. Naturel78,8307s : GC40,16 blocs ordinaires jusqu’à56 et réouverture froide finale ;30 frontières adverses/2fsync/SIGKILL entre stores,5 refus CLI et reçu0600/nlink1,3 scénarios disponibilité et6 cache. TinyV4 :12PASS,178ms,RSS21200KiB,3 signatures synthétiques. Bytes, identité stable, owner/mode/mtime et SDK conservés ; écarts ctime consignés dont2 pendantrun, égalité ctime non qualifiée et cause non établie. Le refus préflight du runnerARM V1 reste conservé.

**AMD64 reste partiel** : batch9 avec7PASS/2échecs ; naturel180,029s en timeout, dernier jalon slot32/200votes/époque2, sans GC40 ni fin56. Le tinyV3 refusait un profil natif invalide du test ; son seul remplacement V4 passe ensuite, une compilation/un lien/même archive, sans rejouer les huit anciens tests :12PASS,659ms,CPU702146µs,RSS19968KiB,3 signatures synthétiques. Mesures de8itérations sur une même source : validation complète94,750ms,matchs2,623ms,contexte48,044ms ; aucun délai de chaîne naturelle n’est prouvé par ce benchmark. Aucun ancien PASS ARM/AMD ou autre cœur n’est réutilisé comme nouveau résultat.

Profil EVM/RPC, forks, EIP, CSV et critères antérieurs inchangés. Production, qualification native de ce lot, image/install, restauration privée opérationnelle/duties/finalité commune, garde de confiance2h expirée, V37 FAILED/journaux manquants, NAS, CI facturation et revue de sécurité externe restent ouverts. Le transfert autorisé des quatre preuves privées VPS est effectué et leurs hashes ont été contrôlés localement ; aucun payload brut ou dérivé n’est publié ici. Cette note remplace leur ancien statut en attente d’autorisation dans les paragraphes historiques. Git ne reconstitue pas les journaux privés. Tous corps et échecs historiques ci-dessous restent intégralement conservés.

## 13 octobre 2026 — Cache BLS : ARM qualifié dans ce lot, naturel AMD64 ouvert

Le candidat f9 + cache BLS positif borne 256 entrées/64 KiB volatils et mémorise seulement les tuples exacts de 176 octets authentifiés. Les builds courants lient 140 objets cœur et 9 programmes. **ARM : 8 CTests réels sans skip passent**, dont naturel 116,077 s, disponibilité 6,792 s et petit cache 0,485 s ; audit sémantique `2dfd6ca3` :30 frontières/2 fsync/SIGKILL entre stores,5 refus CLI avec reçu 0600/nlink1,GC au slot 40 + 16 blocs ordinaires jusqu’à 56 et réouverture froide, sources/témoins synthétiques conservés. Bytes actuels liés au reçu `3a481e40` ; ancien refus metadata `f5b25ac1` et stats initiaux absents restent préservés, 2 écarts ctime actuels consignés sans cause établie. Cela ne qualifie ni panne matérielle, ni réseau, duties ou reprise privée complète.

**AMD64 reste 7/8 : naturel en timeout à 180,03 s**, dernier jalon imprimé : slot 24, 136 votes, époque finalisée 0. Diagnostic séparé du même ELF, 35 échantillons : à 179,054 s, 178,6 s CPU utilisateur + 0,55 s système, aucun throttling observé ; RSS échantillonné : 145252 KiB contre repère 128 MiB/131072 KiB. L’observateur perturbe le run ; maximum exhaustif et fonction responsable non prouvés. Aucun ancien PASS réutilisé ; ancien bilan f9 de 6/7 sur deux runs en sortie 8 conservé. Profil EVM Shanghai/statuts EIP inchangés. Production, GC native, image/install, duties/finalité commune et tous critères antérieurs restent ouverts ; quatre preuves privées VPS attendent leur autorisation, distinctement du V37 FAILED/journaux Mac manquants. Git n’est pas un backup privé intégral ; CI facturation sans étapes et audit externe non réalisé.

## 13 octobre 2026 — GC de checkpoints, qualification limitée Docker ARM

Le nettoyage hors ligne des blobs de checkpoints Capella est qualifié dans le périmètre des [preuves publiques](qualification/checkpoint-blob-gc-arm-20261013/README.md) : copies arrêtées détenues exclusivement, racines primaire/témoin et publication pending conservées, schémas/namespaces inconnus refusés. Ce lot ne qualifie ni un pruning complet de l’état EVM ni la production.

Les preuves combinent quatre réussites conservées du run05 (terminal général `exit8`) et deux scénarios corrigés exécutés dans le run06 (`exit0`) :30 branches adverses avec SIGKILL réel entre les stores et deux injections fsync ; GC naturel au slot40,23 blobs supprimés par store,16 blocs suivants jusqu’à56 puis réouverture du checkpoint suivant. Le scénario naturel dure174.01s sous180s. Il n’y a pas eu de nouvelle exécution unique des six tests. Le dernier build réutilise l’archive140 membres `02b718…` byte exacte et recompile/relie uniquement le consommateur de test modifié.

Les échecs restent conservés : refus source/metadata/E2BIG, course du contrôleur b1/42 et reconstruction postérieure, dossier absent créé puis corrigé pour `ReadWriteExisting`, timeout30s du child et condition naturelle échouée à48 (GC réellement observé à40). Le gateWAL réduit les ouvertures RO et le scénario corrigé passe1.21222s ; la cause exclusive du timeout précédent n’est pas mesurée.

Restent ouverts : GC natif/AMD64, panne électrique réelle, sauvegarde/restauration complète avec journaux/témoins, pruning EVM complet, reprise des duties/finalité commune réelle, transfert des preuves privées VPS/V37 encore pending, revue indépendante et intégralité du goal de production. Aucun statut EIP, critère historique, règle de rattrapage ou autorisation de dépense/vente/déploiement n’est changé par cette note.

---

Le packaging ajoute désormais `nano-storage-maintenance` à côté de `nano`. Le [configure de livraison](qualification/checkpoint-blob-gc-arm-20261013/packaging-addon/configure-terminal.json) réussit : base de commandes complète,147 commandes sélectionnées et16 fichiers flags/link inchangés, deux cibles install présentes. Aucune unité Nano, test, image runtime ou installation réelle n’a été exécuté dans cette étape. L’image runtime n’est pas reconstruite et l’outil n’est pas installé publiquement ; la livraison effective reste ouverte.

Les quatre preuves privées VPS concernent la campagne courante et leur transfert nécessite encore une autorisation distincte. V37 reste `FAILED`, avec les journaux Mac requis manquants et une restauration incomplète. Ces deux blocages distincts ne sont pas résolus par les tests GC ni par le contrôle de packaging.

---

État du 13 octobre 2026 — contrôleur de reprise candidat raccordé, sans lancement de nœud.

Le nouveau mode exige des pins externes et vérifie séparément sources, objets compilés, consommateurs, relais et captures. Les montages Docker refusent les arbres privés et leurs parents ; leurs chemins physiques correspondent aux preuves de compilation. Six nouveaux tests d’intégration synthétiques passent ensemble (6 PASS, zéro FAIL/SKIP), après conservation des refus initiaux et revue des corrections. Les anciens tests ne sont pas rejoués pour ce lot.

Les 139 membres de chaque archive ont été comparés réellement aux objets sur les trois environnements. Quatre unités ARM recompilées avec les 13 headers manquant au snapshot historique sont byte-identiques aux objets qualifiés ; aucun test/link/archive répété, parents inchangés. Cette équivalence actuelle ne reconstitue pas le snapshot initial perdu. [Preuves compactes et limites](qualification/candidate-runtime-binding-20261013/README.md).

Aucune source du cœur C++ ou règle de consensus/EVM n’est modifiée. Les autorités et assets réels, les commandes d’audit des captures actuelles, les quatre preuves privées VPS et la reprise après expiration des deux heures restent ouverts. Aucun writer, duty, nouveau réseau/finalité, fonds ou déploiement public lancé ; aucun fork/EIP promu. Le lot C++ GC reste séparé, non compilé/non exécuté. V37 FAILED, refus RSS historique, CI/release, consensus/exploitation/pruning, Base–Nano/USDC/Doppler et revue de sécurité indépendante sont conservés sans clôture. Tous les critères et matrices historiques restent intacts ; objectif actif.

État précédent conservé :

État qualifié — 13 octobre 2026 : compilation et contrôles ciblés sur trois architectures, production toujours ouverte.

Cœur de production `c672a30d`, port de test livré `d7cc1bdb` (SHA `44febdb9`), six sources et CMake `28fe657b` inchangés : 139 unités du cœur et main recompilés sur Mac natif Darwin ARM64, Docker Linux ARM64 et Docker Linux AMD64, avec consommateurs et bibliothèques de fault-injection cohérents.

Sur Mac natif : trois CTests réels avec JUnit sans skip, puis `prepared-block` direct réussis. Les quatre sélections ciblées du lot Linux passent ; elles ne constituent pas la sélection quotidienne complète. Le port Linux conserve objet et exécutable byte-identiques, mesurés avant décision de réutiliser les résultats ; aucun retest identique n’est ajouté. Les scénarios nouveaux sont synthétiques, dont 48 blocs admis puis une préparation reprise, clé locale pending et fournisseur local à zéro.

Binaires nano identifiés : natif `2b4856b1`, ARM64 `64ca2a4b`, AMD64 `31887383`. [Preuves Mac/ARM et revues indépendantes](qualification/duty-availability-platforms-20261013/mac-platforms-public/README.md) ; [preuves AMD64 et port identique](qualification/duty-availability-platforms-20261013/amd64-public/README.md). Refus du test natif `8690`, collecteurs, aliases, headers complémentaires et perte du snapshot initial ARM restent documentés ; aucun historique n’est réécrit.

CI [38325619830](https://github.com/Complexity-ML/blockchain-nano/actions/runs/38325619830) de `d7cc1bdb` refusée avant toute étape : paiement échoué ou limite de dépenses selon les annotations, sans préciser lequel ; zéro étape et zéro test CI exécuté. [Reçu actuel](qualification/duty-availability-platforms-20261013/source-ci-refusal-d7/terminal-review.json).

Aucune reprise réelle des duties, nouvelle finalité commune, activation de fork ou production publique n’est qualifiée dans ce lot. Les quatre preuves privées VPS attendent toujours l’autorisation d’export ; le garde v2 et l’allowance administrative proposée de 4 h ne sont pas activés. V37 FAILED, refus du format plat à 137524 KiB sous limite 128 MiB, CI/release, consensus/ressources, pruning/exploitation, Base–Nano/USDC/Doppler et revue de sécurité indépendante restent ouverts ; aucun fonds ni déploiement public.

Tous les critères, matrices EIP, statuts et preuves historiques sont conservés byte-for-byte, sans promotion. Objectif de production actif.

État précédent conservé :

# Objectif : Nano C++ de niveau production

État courant — 13 octobre : GitHub main vérifiée à `b7719685` avant ce lot ; sources et rapports récupérables, aucune sauvegarde complète de validateur V37 retrouvée dans le périmètre examiné. La projection pure du vrai checkpoint commun Mac à 22:30:29 UTC montre 64 actifs et des comités/proposant disponibles ; elle ne démontre aucune reprise ni signature et sa fenêtre administrative de 2 h est expirée. [Preuves, refus et limites](qualification/resumption-priorities-20261013/README.md).

Le défaut local de disponibilité est corrigé dans six sources, avec admission entrante toujours stricte. Le cœur et le binaire ont été entièrement recompilés sur Docker ARM64 (139 unités du cœur et l’unité principale). Les deux régressions concernées passent ; le nouveau CTest réel passe ses trois scénarios en 7,11 s, dont 48 blocs admis normalement puis une préparation disponible sur la nouvelle tête. Ce scénario utilise un état et des clés synthétiques, un validateur local en attente d’inscription et le stockage par références : zéro appel au fournisseur de signature locale, aucune reprise de réseau ou finalité WAN. Le refus à 128 MiB RSS de l’ancien format reste ouvert. Le test est ajouté à la sélection CI de 14 noms ; huit contrôles du harnais puis un nouveau contrôle ciblé des six substitutions interdites passent, sans revendiquer une exécution complète de cette sélection ou de la CI GitHub.

Sources livrées sur main et récupérées sur VPS à `4cdff035`. La CI [38322694342](https://github.com/Complexity-ML/blockchain-nano/actions/runs/38322694342) est refusée avant toute étape pour paiement échoué ou limite de dépenses, sans précision entre ces deux causes ; zéro test CI exécuté, aucun réglage de facturation modifié. [Reçu de livraison et de CI](qualification/resumption-priorities-20261013/git-ci-source-delivery-4cdff035.json). Sa compilation native/AMD64, sa reprise effective des duties et la finalité commune entre trois environnements restent ouvertes. Les quatre preuves VPS restent sur VPS, en attente d’autorisation dédiée d’export. V37 FAILED, tous les critères initiaux de production/Base–Nano/Doppler, l’exploitation, le pruning, la revue de sécurité indépendante et les statuts EIP sont conservés sans promotion. Goal actif ; aucun fonds ni déploiement public.

État précédent conservé :


CI du lot d’outillage livré8e053d99 : run38313225000 terminé avant toute étape ; operations/changes refusés, core/network skipped. Les annotations indiquent un paiement récent échoué ou une limite de dépenses, sans préciser lequel. Zéro test CI exécuté, aucun réglage de facturation modifié. [Reçu terminal actuel](qualification/isolated-recovery-tooling-ci-20261012/ci-8e053d99-terminal.json). Sources livrées sur main et récupérées sur VPS ; reprise des duties, finalité commune et toutes les exigences restantes restent ouvertes.


État courant — 12 octobre, outillage de reprise et CI : les deux auditeurs sont raccordés à CMake, avec un nouveau CTest de25 contrôles purs exécuté sur Docker ARM64. Les220 cibles existantes et la fermeture de production restent identiques ; le cœur85eb492f et le relais4b2165b8 sont conservés. Le job core sélectionne désormais13 noms et refuse les omissions, skips et options masquant un échec ; ses résultats JUnit et sa bibliothèque de fault-injection sont contrôlés. Le contrôleur lit/hache/parse chaque rapport sur les mêmes bytes, exige les pins des captures et accepte uniquement l’ajout CMake exact revu. [Preuves ciblées et premiers refus conservés](qualification/isolated-recovery-tooling-ci-20261012/README.md).

La sélection quotidienne complète et la CI GitHub actuelles ne sont pas qualifiées par ces contrôles locaux. Les quatre preuves VPS n’ont pas été transférées ; autorisation dédiée toujours en attente. Aucun writer/duty/réseau restauré lancé dans ce lot, aucune nouvelle finalité commune et aucun statut EIP/production/Base–Nano/Doppler promu. Tous les critères et échecs historiques, ainsi que la revue indépendante ouverte, sont conservés.


CI du code livré `a106e3e1` : run38310206360 terminé avant toute étape ; changes/operations refusés, core/network skipped. Les annotations indiquent des paiements récents échoués ou une limite de dépenses, sans préciser lequel. Zéro test CI exécuté, aucun réglage de facturation modifié. [Reçu actuel](qualification/isolated-runtime-copies-20261012/ci-a106e3e1-terminal.json).

État courant — 12 octobre, copies physiques et tentative privée : cœur source `85eb492f` et relais `4b2165b8` inchangés ; les six binaires sont identifiés. Les 65 paires de journaux/témoins du bootstrap ont été copiées physiquement et auditées sur Mac natif, Docker ARM64 et AMD64 (130 réouvertures, fichiers intacts, sources préservées). Le relais natif reconstruit passe ses contrôles locaux ; son nouveau SHA n’est pas assimilé à l’ancien binaire perdu. [Preuves et limites](qualification/isolated-runtime-copies-20261012/README.md).

Le contrôleur privé original `81954/1` demeure FAILED : trois nœuds prêts, puis refus HTTP404 du header de l’ancre trusted sans archive signée ; trois arrêts propres et zéro duty admise, aucune nouvelle finalité. Le prérequis d’observation est corrigé (23 cas ciblés passent), sans fabriquer de header ni masquer de futurs404. La revue corrige aussi le raccordement du commitment de reprise : le core attend le principal `next`, le témoin reste conservé séparément. Aucun nouveau cœur n’est compilé ni règle modifiée.

Les deux sources Mac sont capturées aux engagements actuels et copiées intégralement (367 fichiers par nœud, données/journaux/témoins/identités P2P) ; les destinations sont auditées et concordantes. L’audit VPS s’est terminé localement à0 après correction d’un montage de bibliothèque manquant ; ses preuves n’ont pas été transférées ni lues sur Mac. La revue automatique a refusé l’export des capsules puis des engagements/métadonnées dérivés ; autorisation humaine dédiée en attente pour quatre preuves seulement, sans clés, bases ou capsule brute. La copie complète et l’audit de destination VPS se sont ensuite terminés localement à0 (original11319), sans export des quatre preuves. Leur autorisation/transfert et concordance centrale, la reprise effective des duties et les nouvelles finalités communes restent à démontrer.

Tous les échecs et critères antérieurs sont conservés : V37 FAILED, rattrapage prolongé, CI/release, consensus/ressources, Base–Nano/USDC/Doppler et revue de sécurité indépendante restent ouverts. Aucun statut EIP ou production n’est promu, aucune image reconstruite, aucun fonds réel ni déploiement public ; objectif actif.

État précédent conservé :

CI du code livré `d5df403e` : run 38301228455 terminé en échec avant toute étape (operations/changes refusés, core/network skipped), pour paiements récents échoués ou limite de dépenses selon les annotations GitHub. Aucun test CI exécuté et aucun réglage de facturation modifié. [Reçu actuel](qualification/isolated-capella-bootstrap-20261012/ci-d5df403e-terminal.json).

Qualification isolée — 12 octobre : nouveaux jeux privés avec clés BLS et racines de genesis des validateurs distinctes. Compilations ciblées natif, Docker ARM64 et AMD64 réussies, images/cœur/bibliothèques réutilisés et parents inchangés. Les exports réels et les 65 correspondances privé/public, quatre signatures BLS, agrégats sync et dépôt ECDSA sont vérifiés dans leur périmètre. Le jeu natif canonique a été produit avec le premier brouillon propre ; les deux exports Docker qualifient le correctif final de copie POSIX. Le parcours Capella/Beacon historique passe avec le dernier exécutable natif. [Preuves et limites](qualification/isolated-capella-bootstrap-20261012/README.md).

Les 65 paires journal/témoin du bootstrap natif ont été importées via EIP-3076 puis réouvertes aux engagements exacts, sans nouvelle signature. Le plan pur répartit 16/16/33 clés entre les trois owners, conserve le dépôt en attente, relie la proposition présignée au slot 1 et recalcule séparément les 65 engagements principaux et 65 intentions témoins. 65 cas du modèle passent ; cette préparation ne démontre ni transfert complet, reprise des duties, ni avancement de finalité commune. Les anciens helpers stricts ne sont pas assouplis.

Échecs conservés : signatures contradictoires du premier brouillon supprimées avant tout export réel ; erreur de compilation de comparaison JSON ; refus de permissions du montage Mac Docker puis échec de copie après réservation. Cette identité consommée ne sera pas réutilisée. Le parseur des balances EVM a été corrigé en uint256, sans élargir les quantités consensus uint64. Le registre de réservation protège dans le périmètre du ledger explicitement géré par l’opérateur ; aucune exclusivité globale n’est revendiquée.

GitHub a été examiné en lecture seule : aucune sauvegarde complète V37 retrouvée dans les références locales accessibles, les releases et les artefacts examinés. Les capsules publiques ne remplacent pas les données ni les journaux. La sauvegarde NAS indiquée en sommeil reste à localiser. V37 FAILED, refus réel du rattrapage prolongé, finalité WAN, qualification release/CI, consensus/ressources, Base–Nano/USDC/Doppler et revue de sécurité indépendante restent ouverts. Aucun statut EIP promu, règle de consensus changée, signataire V37 relancé, fonds réel utilisé ou déploiement public effectué ; objectif actif.

État précédent conservé :

État courant vérifié — 12 octobre : le build Mac a été refait dans un dossier durable (source `0842078e`, Release réussie, trois CTest ciblés passent). Le nouveau garde des chemins refuse une campagne en stockage temporaire avant toute création ou démarrage ; 11 nouveaux cas utiles passent. Les 14 anciens volumes Mac ont 14 genesis d’exécution différents : aucun des 154 couples journal/témoin existants ne correspond à V37, et 294 autres couples demandés sont absents. Les journaux Mac actuels manquent toujours. Le domaine seed/tête du VPS est authentifié.

**Projection réelle après 5 j 15 h 58 min : FAILED.** Sept chunks atteignent le slot 34520 / époque 4315, avec 58 validateurs actifs à ce dernier état terminé. La sélection d’un comité futur refuse « empty Altair sync active set » dans le chunk suivant ; son époque exacte reste inconnue. Les 3241 fichiers de projection et la bibliothèque qualifiée sont inchangés ; 64,946 s, pic de 89 128 KiB, aucune panne de ressources. L’augmentation du délai administratif ne permet pas de revendiquer une reprise. Duties et progression de finalité commune restent à démontrer. [Preuves terminales et limites](qualification/resume-terminal-state-20261012/README.md).

La CI du commit `522c1951` (run 38285487283) n’a exécuté aucune étape, avec un refus actuel pour paiements ou limite de dépense. Revue EVM/RPC statique : Capella exige Shanghai/beacon ; les genesis/Compose génériques legacy sont distincts, Cancun/Prague/Osaka ne sont pas activés réseau. Aucun statut EIP promu. Aucun cœur, règle, pin ou horloge modifié ; aucune image reconstruite ni signataire V37 relancé. Préparer une campagne privée distincte avec nouvelles clés BLS, racine de genesis des validateurs et stockage persistant, sans rebaptiser V37 en succès. Les qualifications acquises restent conservées dans leur périmètre daté. Les exigences restantes de production, release/CI, consensus/ressources, Base–Nano/USDC/Doppler et revue indépendante demeurent ouvertes ; objectif actif.

État précédent conservé :

État courant vérifié : Reprise du12 octobre : aucun writer actif, mais dossiers Mac V37 désormais vides (0/0 fichiers) ; anciens volumes Docker conservés sans correspondance qualifiée aux derniers pins. Journaux Mac à retrouver avant toute reprise sûre ; aucun reset de signatures. [Audit et corrections de préparation](qualification/resume-persistent-state-20261012/README.md). Source VPS3216 et copie récupérée3220 fichiers revalidés et préservés durablement hors /tmp, inodes indépendants/originaux inchangés. Nouveau garde de copie réelle, inventaires exacts et profils liés aux hashes ; option startupCatchup finie explicite, pas de règle de consensus modifiée. Tests ciblés77 planner,76 helper puis2 containment/legacy,18 bindings passent ; échecs de banc conservés. Admission réelle du helper persistent validée sur VPS après push/pull522c1951 : deux arbres complets, source-volume actuelle et capsule liée vérifiés, aucune destination/signature créée. Rejeu après cinq jours, survie des validateurs, duties/finalité commune, CI/release et tous critères production/Base–Nano/Doppler/revue indépendante restent ouverts. C++ et bibliothèques qualifiés inchangés, aucune image reconstruite ni nœud relancé.

État précédent conservé :

État courant vérifié : Récupération hors ligne V37 terminée sur copie physique VPS : principal0x410b→intention témoin0x410c, réouverture idempotente0, rejeu canonique0,33 paires journal/témoin et minima externes pré-V37 vérifiés ; fichiers signataires/pair inchangés sur copie et3216 fichiers originaux inchangés. [Preuves, refus initial conservé et limites](qualification/v37-offline-copy-recovery-20261007/README.md). Aucun duty/signature/réseau lancé ; reçu terminal VPS manquant reste explicite. Garde de source runtime distincte intégrée,75 tests ciblés0 et audit réel accepté, anciens cas startup/kill conservés. Cœur85eb492f poussé/pull VPS ; CI37562317444 refusée avant étapes pour facturation. Prochaine étape : restauration physique préparée et reprise des duties puis finalité commune en progression. V37 FAILED, tous critères production/Base–Nano/Doppler et revue indépendante restent ouverts.

État précédent conservé :

État courant vérifié : Correctif de frontière d’annulation intégré après comparaison parent1/correctif0 sur Mac natif, Docker ARM64 et AMD64 ; trois mutations externes maintiennent l’arrêt de sécurité, reprises cold passent, binaires parent inchangés. [Preuves ciblées](qualification/publication-cancel-boundary-integrated-20261007/README.md). Test CTest dédié et sélection CI stricte ajoutés ; configuration CMake native vérifiée avec12 enregistrements exacts et nouveau gate exécuté à0 avec le binaire qualifié, sans refaire les11 autres ni revendiquer un build propre ; aucune qualification release/WAN promue. Audits Mac0 :1986/1971 fichiers inchangés, pins actifs16/16 et historiques16/16 exacts. Audit VPS1 :3216 fichiers inchangés mais principal/témoin à des engagements différents, reçu terminal absent ; copie indépendante vérifiée, récupération hors ligne encore à faire. V37 FAILED conservée. Finalité commune, production/Base–Nano/Doppler, CI facturation et revue indépendante restent ouverts.

État précédent conservé :

État courant vérifié : V37 terminée FAILED (contrôleur original76530/1) avant la fenêtre900s : zéro nouvelle finalité commune ; deux reçus d’arrêt seulement, Mac natif/Docker Mac à0 et Docker VPS à1 sansOOM. Le VPS a renvoyé HTTP503 « Beacon state unavailable », puis « Capella reactor unavailable; reopen required; initial failure: Capella message cancelled ». [Preuves terminales](qualification/restored-three-node-v37-terminal-20261007/README.md). Inspection du code : un lease(check) annulable subsiste après witness_prepare dans publish_impl ; un brouillon déplace la préparation avant les écritures. La nouvelle régression native reproduit le callback après écriture du témoin sur le parent (exit1) et passe avec le correctif (exit0), puis vérifie les reprises cold existantes. [Comparaison et premier échec du banc conservé](qualification/publication-cancel-boundary-native-20261007/README.md). Correctif non intégré ; Docker ARM64/AMD64 et attribution complète du cas WAN restent à qualifier. Audits distincts des deux arrêts propres et du refus runtime VPS requis avant tout nouveau writer ; aucun reçu terminal VPS ne doit être inventé. Finalité réelle, CI facturation, release, tous critères production/Base–Nano/Doppler et revue indépendante restent ouverts.

État précédent conservé :

## État courant vérifié — 7 octobre 2026

V37 en cours, original76530 confirmé vivant : préflight exact et trois nœuds prêts/connectés ; finalité1284 encore observée. [Observation non terminale](qualification/restored-three-node-v37-running-observation-20261007/README.md). Relais4b2165b8 intégré/qualifié dans ses parcours ciblés, cœur4ce3e4e2 inchangé ; même fenêtre900s et trois nouvelles finalités exigées. CI37560013999 du nouveau commit refusée avant étapes pour facturation, aucun test GitHub exécuté. Production/Base–Nano/Doppler, release/CI et revue indépendante restent ouverts.

État précédent conservé : Préparation V37 terminée : copies indépendantes Mac1971/1956 et sauvegarde VPS3198 fichiers exactes ; pins actifs16/16/33 et historiques16/16/0 conservés. [Preuves](qualification/restored-three-node-v37-bounded-route-prepare-20261007/README.md). Relais4b2165b8 qualifié sur les trois environnements, cœur4ce3e4e2 inchangé ; audits V36 réutilisés sur identité exacte des fichiers, sans nouvelle capture revendiquée. Aucun signataire lancé ; budgets/profil/horloge/délais conservés, images réutilisées. Finalité commune, CI facturation/release, production/Base–Nano/Doppler et revue indépendante restent ouverts.

État précédent conservé : Relais à chemin de dépendances borné intégré :13 consommateurs concernés qualifiés en natif/Docker ARM64/AMD64 ; faux ACK, annulations, reprises, tête mobile et changement de session couverts dans leurs modèles. [Sources, reçus et limites](qualification/bounded-dependency-route-integrated-20261007/README.md). Deux nouveaux CTest passent sur les trois environnements ; inventaire49 vérifié sur Docker, ancien gate21 corrigé et contre-exemple conservé. Ce ne sont pas49 exécutions CTest complètes. Cœur4ce3e4e2 inchangé, aucune image reconstruite, aucun writer relancé. V36 FAILED et audits exacts conservés ; nouvelle finalité commune, CI GitHub/facturation, release, production/Base–Nano/Doppler et revue indépendante restent ouverts.

État précédent conservé : Audits V36 terminés0 sur les trois environnements :1972/1957/3198 fichiers inchangés, pins actifs16/16/33 et historiques16/16/0 vérifiés séparément ; engagements exacts arrêt, principal/témoin indépendant et rejeu canonique conservés. [Preuves](qualification/v36-post-stop-readonly-20261007/README.md). V36 FAILED demeure, aucune nouvelle finalité. [Relais proposé : qualification native ciblée](qualification/bounded-dependency-route-native-20261007/README.md) sur13 consommateurs, modes configurés et HTTP local ; erreurs de définitions/permission/pilote conservées et réparées, aucune source production modifiée. Docker ARM64/AMD64 actifs79029/88478, mêmes images/volumes réutilisés ; pas encore terminés ni intégrés. CI facturation, finalité réelle, release, critères production/Base–Nano/Doppler et revue indépendante restent ouverts.

État précédent conservé : V36 terminée FAILED, original73188/1 : zéro nouvelle finalité commune en900s ; trois reçus et trois arrêts0 sansOOM. Duties propositions35/37/54, attestations301/281/71, sync1130/1198/9 ; ACKs de dépendances62/52/214 et aucune tête sélectionnée confirmée dans les tours terminés. [Résultat terminal](qualification/restored-three-node-v36-terminal-20261007/README.md). Audits de lecture seule5836/89720 lancés après arrêt. [Brouillon de chemin borné](qualification/bounded-dependency-route-model-20261007/README.md) qualifié seulement sur modèles natifs : avec éviction du cache, exports43009→13249 pour2049 blocs, mêmes293 tours et limites ; refus/ACK contradictoire, annulation/ACK tardif et reprise passent. Non intégré, pas de cause WAN globale démontrée. CI refusée avant étapes pour facturation. Finalité, production/Base–Nano/Doppler et revue indépendante restent ouvertes.

État précédent conservé : V36 en cours, original73188 confirmé vivant : trois nœuds prêts, finalité1284 encore inchangée. Instantané non atomique des tours terminés : ACKs de dépendances31/23/73 ; aucune tête sélectionnée confirmée. [Observation et refus CI](qualification/restored-three-node-v36-running-observation-20261007/README.md). CI37557068510 refusée avant toute étape pour facturation ; aucun test CI exécuté. [Modèle de rattrapage](qualification/long-dependency-model-20261007/README.md) : limites V36 inchangées,513/935/2049 blocs demandent68/123/293 tours avec ACKs simulés ; ce modèle ne prouve ni délai WAN ni cause globale. Campagne active non redémarrée, images/volumes réutilisés. Finalité, release/CI, production, Base–Nano/Doppler et revue indépendante restent ouverts.

État précédent conservé : Préparation V36 terminée depuis V35 auditée : copies Mac1914/1945 et sauvegarde VPS3113 fichiers exactes ; pins actifs16/16/33 et historiques16/16/0 conservés. Nouveau cœur avec inventaires signés publiés qualifié sur trois environnements ; relais inchangé. [Preuves](qualification/restored-three-node-v36-signed-inventory-prepare-20261007/README.md). Aucun signataire lancé ; budgets/profil/horloge/délais conservés. Finalité commune, production, Base–Nano/Doppler et revue indépendante restent ouverts.

État précédent conservé : Inventaires signés publiés intégrés après qualification ciblée Mac natif/Docker ARM64/AMD64 : contre-exemple parent attendu1 puis six parcours0 par environnement,16 consommateurs core identifiés sur139 sources et recompilés, nano/test/capture reliés et parents inchangés. HTTP/RPC passe sur les trois environnements. [Sources, commandes et limites](qualification/capella-published-signed-inventory-20261007/README.md). Nouveau gate ajouté à la sélection CI stricte, exécution CI encore à vérifier. Aucun format disque, règle de signature, budget ou délai modifié ; images existantes réutilisées. V35 FAILED conservée. Reprise réseau, finalité commune, release propre, tous critères production, Base–Nano/Doppler et revue indépendante restent ouverts.

État précédent conservé : Audits V35 terminés à0 sur les trois environnements :1915/1946/3113 fichiers inchangés, pins actifs16/16/33 et historiques16/16/0 conservés séparément ; engagements arrêt, principal/témoin et rejeu canonique vérifiés. [Preuves](qualification/v35-post-stop-readonly-20261007/README.md). V35 FAILED conservée, aucune nouvelle finalité. Correctif inventaires signés privé en qualification : contre-exemple parent reproduit et premiers gates ARM64 passent ; parcours ARM64 encore actif62036, AMD64 lancé64089 après audit, garde inchangée. Pas encore intégré ni qualifié sur réseau/natif/release propre. Tous critères production, Base–Nano/Doppler et revue indépendante restent ouverts.

État précédent conservé : V35 terminée FAILED, original76628/1 : zéro nouvelle finalité commune en900s, trois arrêts0 sansOOM et trois reçus. Duties effectives : propositions36/43/45, attestations304/299/136, sync1150/1015/0 ; ACKs de dépendances106/66/224, aucune tête sélectionnée confirmée dans les tours terminés. Les64 diagnostics bornés par nœud attribuent les refus mais ne mesurent pas leur nombre global. [Résultat terminal](qualification/restored-three-node-v35-terminal-20261007/README.md). Audits en lecture seule66051/52488 lancés ; brouillon inventaires signés non intégré, build ARM64 démarré, garde AMD64 attend la fin de son audit sans être relâchée. Finalité, production/Base–Nano/Doppler et revue indépendante restent ouvertes.

État précédent conservé : V35 en cours, original76628 confirmé vivant : trois nœuds prêts/connectés,65 clés16/16/33. Snapshot : finalité1284 inchangée, admissions de dépendances31/8/32 dans les tours terminés, aucune tête sélectionnée confirmée. VPS64 diagnostics bornés, tous attestation/inventory-capture-busy ; autres catégories observées sur Mac. [Observation non terminale](qualification/restored-three-node-v35-running-observation-20261007/README.md). Brouillon privé inventaires signés immuables préparé mais non compilé/intégré ; recompilation des consommateurs requise. Campagne, sources actives, règles et délais inchangés. V34 FAILED, verdict V35 non acquis et critères production/revue indépendante ouverts.

État précédent conservé : Préparation V35 terminée : copies complètes Mac1845/1879 et sauvegarde VPS3008 fichiers exactes depuis V34 auditée ; pins actifs16/16/33 et historiques16/16/0 conservés. Cœur c4996498 inchangé, relais b2af22df qualifié sur les trois environnements, config VPS dérivée localement et liée au plan indépendant. [Preuves](qualification/restored-three-node-v35-trace-bound-prepare-20261007/README.md). Aucun signataire lancé par la préparation, budgets/horloge/délais inchangés. La reprise sert à attribuer les refus de catalogue et vérifier la finalité ; V34 FAILED et tous critères production/revue indépendante restent ouverts.

État précédent conservé : Diagnostic RPC corrigé et qualifié sur Mac natif/Docker ARM64/AMD64 : compteur partagé saturant entre connexions, catégories autorisées et paramètres privés exclus. Nouveau test réel HTTP100 connexions/4 diagnostics,18 variantes keepalive et9 scénarios busy passent ; relais compilés/reliés, parents inchangés. [Preuves](qualification/core-trace-bound-http-20261007/README.md). Aucun nœud relancé, aucune règle/budget/délai de consensus modifié. V34 reste FAILED ; cause globale et nouvelle finalité commune non démontrées. Aucun statut EIP/production promu, Base–Nano/Doppler et revue indépendante ouverts.

État précédent conservé : Audits V34 post-arrêt terminés à 0 sur Mac natif/Docker Mac/VPS :1846/1880/3008 fichiers inchangés, pins actifs16/16/33 et historiques16/16/0 conservés séparément ; stockage principal, témoin indépendant et rejeu canonique vérifiés. [Preuves](qualification/v34-post-stop-readonly-20261007/README.md). Aucun writer relancé. V34 reste FAILED, aucune nouvelle finalité commune. Le brouillon de diagnostic borné/catégories passe ses primitives ARM64/AMD64 ; compilation complète et échanges HTTP restent à qualifier avant intégration. Aucun statut EIP ou production promu ; Base–Nano/Doppler et revue indépendante restent ouverts.

État précédent conservé : V34 terminée FAILED, original14735/1 : zéro nouvelle finalité commune en900s, checkpoint1284 inchangé. Trois arrêts0 sansOOM et reçus, propositions36/37/50, attestations299/296/109 ; ACKs de dépendances104/85/192 mais aucune tête sélectionnée confirmée dans les tours terminés. VPS10906 refus temporaires inventory-capture-busy, type de catalogue non attribuable aux logs actuels ; ce constat ne prouve pas encore la cause globale. [Résultats](qualification/restored-three-node-v34-terminal-20261007/README.md). Audits post-arrêt en lecture seule lancés76517/79302 ; aucun writer relancé. Diagnostic borné/catégories préparé séparément, non compilé/intégré. Tous critères production, Base–Nano/Doppler et revue indépendante restent ouverts.

État précédent conservé : Préparation V34 : cœur c4996498 qualifié natif/ARM64/AMD64, relais77848 inchangé ; copies Mac1842/1832 et sauvegarde VPS2936 fichiers exactes, pins actifs16/16/33 et historiques16/16/0 vérifiés. Source VPS137 auditée, capsule courante0x3b94 liée par SHA ; outil explicite après crash et57 tests passent. [Preuves](qualification/restored-three-node-v34-startup-memo-prepare-20261007/README.md). Aucun signataire lancé ; budgets/délais conservés. Reprise effective et trois finalités communes restent à démontrer, V33 FAILED et tous autres critères/revue indépendante conservés.

État précédent conservé : Profil complet VPS après correction c4996498 : original95451 terminal0, nœud arrêté0 sansOOM, runtime prêt35500ms sur copie indépendante2936 fichiers ; source exacte inchangée et33 engagements de signature conservés. Restauration32883ms/migration911ms. [Preuves](qualification/capella-startup-memo-full-state-profile-20261007/README.md). Mesures uniques non appariées, zéro duty/finalité nouvelle ; pas de gain WAN garanti, V33 FAILED et autres critères/revue indépendante ouverts. Préparer maintenant la reprise des trois nœuds, mêmes limites et délais.

État précédent conservé : 2026-10-07 — [Startup reference memo intégré](qualification/capella-startup-reference-memo-20261007/README.md) : décodage/authentification complet initial, réutilisation process-local bornée des métadonnées identiques avec chaque blob PRIMARY/WITNESS relu et hashé. Nouveau scénario2 records et contre-exemple parent ; trois parcours Docker ARM64/AMD64, startup natif passent, annulation sans mutation/signataire préservé. Échecs initiaux compilation conservés, images/parents inchangés, builds incrémentaux. Aucun nœud démarré ni gain grand état/WAN démontré ; V33 FAILED, finalité commune, release propre, autres critères production et revue indépendante restent ouverts. Aucun statut EIP, bridge ou Doppler promu.

État précédent conservé : Profilage VPS sur copies isolées terminé : refus initial86734 (orchestrateur0/nœud1) conservé pour engagement périmé ; seconde copie avec engagement exact de l’audit49180/0, runtime prêt138973ms, IPC observé et arrêt0,33 clés saines mais zéro duty/finalité nouvelle. Restauration83990ms, migration/configuration32005ms, journaux464ms, catch-up21392ms, premier poll1095ms. Source2936 fichiers strictement inchangée. [Mesures et différences avec V33](qualification/capella-startup-phase-profile-20261007/README.md). Coût dominant localisé ; optimisation des revalidations authentifiées encore à implémenter/qualifier. Mesure isolée unique, aucune conclusion de performance WAN ou production ; V33 FAILED et tous critères restants conservés.

État précédent conservé : Audit V33 après arrêt forcé terminé34298/0 :2936 fichiers inchangés, checkpoint cœur0x3b93→0x3b94 et témoin indépendant vérifiés,33 engagements de signature exactement conservés, finalité inchangée. [Audit](qualification/v33-post-stop-readonly-20261007/README.md). Diagnostic par phases de démarrage ajouté sous NANO_TEST_NETWORK_TRACE=1 ; compilation/relink Docker ARM64 et AMD64 et test configured-runtime16 blocs passés, binaires parents inchangés, aucun nœud lancé. [Périmètre](qualification/capella-startup-stage-diagnostic-20261007/README.md). Ce diagnostic ne corrige pas encore le coût de reprise ; source du retard à mesurer, natif Mac non recompilé pour ce changement, V33 FAILED et tous critères production/Base–Nano/Doppler/revue indépendante conservés.

État précédent conservé : V33 terminée FAILED, processus original5043/1 : précontrôle passé, mais disponibilité IPC du VPS absente dans180s. Arrêt VPS137 après20s de grâce, sans OOM ; aucun nœud Mac lancé, aucune duty ni finalité nouvelle démontrée, aucun reçu d’arrêt. Catch-up initial journalisé, phase coûteuse encore à identifier. [Preuves terminales](qualification/restored-three-node-v33-terminal-20261007/README.md). Audit après arrêt forcé lancé en lecture seule ; aucune relance ni élargissement du délai. Correction d’inventaire df31cd48 seulement qualifiée localement. Tous critères production et Base–Nano/Doppler demeurent ouverts.

État précédent conservé : Préparation V33 : cœur df31cd48 (capture d’inventaire de blocs depuis une publication immuable), relais77848f1f conservé ; binaires natif/ARM64/AMD64 rehashés et liés aux qualifications ciblées. Captures V32 et engagements actifs16/16/33, historiques16/16/0 vérifiés ; copies Mac1842/1832 et sauvegarde physique VPS2871 fichiers exactes, journaux/témoins conservés. Configuration VPS dérivée localement, budgets et délais inchangés. [Preuves](qualification/restored-three-node-v33-published-inventory-prepare-20261007/README.md). Aucun nouveau signer lancé par la préparation ; duties et trois nouvelles finalités communes restent à démontrer. V32 FAILED conservé. Aucun critère de production supplémentaire clos par cette préparation.

État précédent conservé : Correctif de catalogue de blocs intégré et qualifié localement Docker ARM64/AMD64 et Mac natif : lecture depuis une seule publication durable cohérente, sans contention avec le writer ; contre-exemple sur ancien cœur, APIs bloquante/non bloquante, inventaires mutables, quotas, annulation, générations de catalogue et erreurs initiales préservés. [Preuves et limites](qualification/published-block-inventory-20261007/README.md). Builds incrémentaux ciblés, pas une release propre complète ; aucun nœud relancé et aucune finalité WAN qualifiée. Audits après arrêt V32 terminaux0 : fichiers1843/1833/2871 inchangés, pins actifs16/16/33 et historiques16/16/0 exacts, principal/témoin vérifiés séparément. [Audits](qualification/v32-post-stop-readonly-20261007/README.md). V32 FAILED et tous critères restants production/Base–Nano/revue indépendante demeurent ouverts ; aucun statut EIP promu.

État précédent conservé : V32 terminale FAILED9157/1 : zéro nouvelle finalité commune en900s, checkpoint1284 inchangé. Reprise effective : propositions43/36/67, attestations306/298/161, clés16/16/33 ; trois reçus, arrêts0 sansOOM, contrôleur inchangé. Relais77848 : ACKs de dépendances111/125/191 mais aucune tête sélectionnée confirmée ; VPS10839 refus temporaires inventory-capture-busy. [Résultat et limites](qualification/restored-three-node-v32-terminal-20261007/README.md). Audits indépendants après arrêt en cours ; correctif de catalogue de blocs depuis publication durable en qualification isolée ARM64, non injecté et non qualifié. Aucun autre writer relancé. Tous critères production/Base–Nano/revue indépendante restent ouverts ; aucun statut EIP promu.

État précédent conservé : V32 refusée au préflight50517/1 avant tout acteur : champ vpsCopiedVolume absent du manifeste de préparation. Échec et manifeste initial conservés ; contrat complété depuis reçus volume/arrêt vérifiés et hashes input/plan, contrôleur inchangé. Données/copies/pins intacts, aucun duty ni nouveau writer. [Détails](qualification/restored-three-node-v32-preparation-priority-prepare-20261007/README.md). Reprise avec manifeste corrigé à effectuer ; V31 FAILED et tous critères production/Base–Nano/revue indépendante ouverts.

État précédent conservé : Préparation V32 terminale96678/0 : nouvelles captures V31/pins actifs16/16/33 et historiques16/16/0 exacts, clones Mac1835/1817 fichiers et sauvegarde VPS2850 vérifiés. Relais77848f1f compilés natif/ARM64/AMD64, chargement vérifié ; core7329 inchangé. Refus initial de chemin85115/1 conservé, placement corrigé par copies binaires identiques, garde non assouplie. [Preuves et limites](qualification/restored-three-node-v32-preparation-priority-prepare-20261007/README.md). Aucun signer lancé ; prochains duties et trois nouvelles finalités communes en900s restent à démontrer. V31 FAILED et tous critères production/Base–Nano/revue indépendante restent ouverts.

État précédent conservé : Audits après arrêt V31 terminaux0 sur les trois environnements : fichiers1836/1818/2850 inchangés, principal/témoin exacts, pins actifs16/16/33 et historiques16/16/0 comparés séparément. [Preuves](qualification/v31-post-stop-readonly-20261007/README.md). Priorité bornée de préparation implémentée au source : un tour bloc supplémentaire avant chaque tour obligatoire, toutes catégories servies sous20 opportunités même sous changements adversariaux ; contre-exemple parent puis20971520 fenêtres passent Docker ARM64/AMD64 et planning normal préservé. [Périmètre](qualification/capella-catchup-priority-20261007/README.md). Nouveau relais réel/reprise WAN à qualifier, V31 FAILED et tous critères production/Base–Nano/revue indépendante ouverts ; aucune règle, limite ni preuve historique effacée.

État précédent conservé : V31 terminée FAILED50685/1 : zéro nouvelle finalité commune en900s, checkpoint1284 inchangé. Reprise effective des trois owners : propositions45/37/74, attestations306/302/145, clés16/16/33 ; reçus sains, arrêts0 sansOOM, contrôleur inchangé. [Résultat et périmètre](qualification/restored-three-node-v31-terminal-20261007/README.md). Le relais admet partiellement les dépendances mais ne ferme pas le rattrapage/finalité ; audits indépendants après arrêt lancés séparément, avant tout nouveau writer. Diagnostics/tests profonds intégrés au source0d21e782, non injectés dans V31. Tous critères production/Base–Nano/revue indépendante restent ouverts ; aucun statut EIP promu.

État précédent conservé : Nouveau test profond du relais et diagnostics locaux qualifiés Docker ARM64 :4096 blocs/tête mobile/reset/refus/ancre, ACKs et caches bornés vérifiés ; le modèle statique exige1263 tours, sans promesse WAN. Compteurs de préparation/export/envoi/ACK intégrés au source, aucune règle ni borne modifiée. [Preuves et limites](qualification/capella-deep-relay-diagnostics-20261007/README.md). V31 reste active sur ses binaires core7329/relais e33 d’origine, finalité commune encore1284 à22:43UTC ; nouveau relais intégré/AMD64/WAN non qualifiés. Tous critères production/Base–Nano/revue indépendante restent ouverts.

État précédent conservé : Préparation V31 terminale : sources V30 arrêtées/auditées, pins actifs16/16/33 et historiques16/16/0 exacts, clones Mac complets1795/1806 fichiers et sauvegarde VPS2784 vérifiés. Core7329b7ad/relais e33 identifiés ; dérivation locale VPS égale au plan indépendant. Budgets4k/cache8/catalogue128KiB conservés, votes/journaux/témoins préservés, aucun signer lancé. [Preuves et périmètre](qualification/restored-three-node-v31-canonical-prepare-20261007/README.md). Prochaine campagne : trois nouvelles finalités communes en900s ; V30 FAILED et tous critères production/Base–Nano/revue indépendante restent ouverts.

État précédent conservé : Audits après arrêt V30 terminés0 sur les trois environnements avec le core corrigé7329b7ad : principal/témoin exacts, pins actifs16/16/33 et historiques16/16/0 comparés séparément, fichiers1796/1807/2784 strictement inchangés. [Capsules et périmètre](qualification/v30-post-stop-readonly-20261007/README.md). Aucun writer redémarré ; checkpoint finalisé1284 inchangé. Copies locales V31 préparées, préparation complète/sauvegarde VPS et nouvelle démonstration de finalité commune restent requises. Tous critères production/Base–Nano/revue indépendante restent ouverts ; aucun statut EIP promu.

État précédent conservé : Correction du contrôle canonique de l’enveloppe durable froide qualifiée Mac natif/Docker ARM64/Docker AMD64 : le mémo local déjà authentifié est transmis au dernier passage, sans supprimer égalité canonique, lecture/hash des blobs ou validation des opérations signées. Nouveau test avant/après : ancien moteur refusé au point attendu, moteur corrigé0 sur32 blocs/pruning/reprise, enveloppe80 votes et18 lignes, trois passages certifiés sans revalidation lourde. Docker : writer/préflight adversariaux, quatre EIO et trois SIGKILL chacun passent. Sources compilées et programmes parents vérifiés ; une TU core recompilée, aucune image reconstruite. [Preuves et limites](qualification/durable-canonical-reference-memo-20261007/README.md). Ce périmètre ne prouve ni graphe historique sous charge ni finalité WAN ; V30 FAILED conservée et nouvelle reprise après audits/pins reste requise. Tous critères production/Base–Nano/revue indépendante restent ouverts ; aucun statut EIP promu.

État précédent conservé : V30 terminée FAILED (session82206/1, délai900s) : zéro nouvelle finalité commune, checkpoint1284 inchangé. Les trois owners restaurés ont néanmoins repris effectivement les duties : propositions1/2/3, attestations8/8/7, clés16/16/33 ; reçus sains, arrêts0 sans OOM. Hashes du contrôleur inchangés. [Résultat, reçus et limites](qualification/restored-three-node-v30-terminal-20261007/README.md). Cela ne ferme pas la reprise réseau : nouvelles captures/pins indépendants requis après ces publications, puis reprise avec correction ciblée du coût de sérialisation froide en cours de qualification. Tous critères production/Base–Nano/revue indépendante restent ouverts ; aucun statut EIP promu.

État précédent conservé : Préparation V30 terminale : sources Mac V28 intactes et VPS V29 migré/arrêté1, budgets sources1k/1k/4k déclarés et vérifiés, aucune capacité abaissée. Copies Mac/pins actifs et historiques vérifiés ; sauvegarde physique VPS migré2716 fichiers indépendante et dérivation locale égale au plan.105 tests planner/helper/garde passent ; exit1 exige un audit spécifique lié par hash, sans duties et pins inchangés. Binaires qualifiés9e696b50 réutilisés, aucune image reconstruite. [Preuves et périmètre](qualification/restored-three-node-v30-cold-prepare-20261006/README.md). Aucun duty/signature/finalité encore démontré pour cette reprise ; tous critères production/Base–Nano/revue indépendante restent ouverts.

État précédent conservé : Garde catalogue avant recovery/migration qualifiée ARM64/natif/AMD64 :14 refus sans publication ni modification principal/témoin, parents inchangés ; une TU core recompilée, aucune image reconstruite. Planner/helper exigent un budget catalogue cohérent explicite ;78 tests passent. [Sources, commandes et limites](qualification/runtime-catalog-preflight-20261006/README.md). V29 FAILED conservé, checkpoint VPS migré/pins33 vérifiés après arrêt, Mac non démarrés. Préparer une nouvelle reprise depuis ces engagements exacts ; aucun duty ni nouvelle finalité WAN encore qualifié. Tous critères production/Base–Nano/revue indépendante restent ouverts.

État précédent conservé : V29 terminal82373/1 : refus catalogue VPS (65536 octets insuffisants pour4096 records), après publication de migration ; Mac non démarrés. Relecture actuelle VPS0 : token principal/témoin avance d’une séquence, pins33 inchangés et rejeu canonique exactement identique hors token ; fichiers2716 inchangés pendant audit. [Échec et preuves](qualification/restored-three-node-v29-catalog-capacity-refusal-20261006/README.md). Correction préflight/runtime et budget catalogue en cours : nouveaux14 refus sans écriture passent ARM64/natif, AMD64 actif25869 ;78 tests planner/helper passent. Aucun nouveau réseau/duty/finalité qualifié. Tous critères de production/Base–Nano/revue indépendante restent ouverts.

État précédent conservé : Préparation V29 terminale : snapshots V28 relus avec core cohérent, pins actifs/historiques comparés séparément, copies Mac complètes vérifiées et sauvegarde physique VPS2716 fichiers/inodes indépendants. Dérivation locale VPS égale au plan indépendant ; binaires qualifiés identifiés. Plan administratif4096 records/cache froid8, règles/clock/pins inchangés ; pas encore de migration/duty/signature. [Preuves et limites](qualification/restored-three-node-v29-cold-prepare-20261006/README.md). Nouvelle finalité commune et tous critères production/Base–Nano/revue indépendante restent ouverts.

État précédent conservé : Relecture historique terminée sur Mac natif/Docker Mac/Docker VPS avec core cohérent e33ad89e et capture e4ce39eb : sorties0, checkpoints/journaux/témoins/rejeu identiques aux engagements après arrêt V28 et fichiers inchangés (1728/1759/2716). [Preuves](qualification/current-core-historical-readonly-20261006/README.md). Préparation VPS raccordée aux options locales retention/cold et comparée au planner indépendant par25 tests (74 avec la régression du planner) ; aucun changement de consensus/clock/pins, aucun démarrage de writer. [Périmètre](qualification/vps-local-runtime-options-20261006/README.md). Restauration active/migration froide des données historiques, duties et nouvelle finalité WAN commune restent ouvertes, ainsi que l'ensemble des critères techniques, Base–Nano et revue indépendante. Aucun statut EIP promu.

État précédent conservé : Outil de capture/planner : budget local explicite `captureBudget {blocks,envelopeBytes}` qualifié sur les trois environnements (capture compilée,26 refus C++,49 tests de planner ; gate CTest Docker ARM64/AMD64). Les anciennes demandes gardent leur politique historique. Pins et règles de consensus inchangés. [Preuves et limites](qualification/private-capture-resource-budget-20261006/README.md). Le plafond de capture1024 n’empêche plus une demande explicite bornée, mais le graphe réellement restauré au-delà de cette frontière et la finalité WAN ne sont pas encore qualifiés. La candidate core cohérente et tous les critères de production/Base–Nano restent dans leur périmètre documenté ci-dessous.

État précédent conservé : Candidate core `e33ad89e` compilée entièrement sur Docker ARM64/AMD64 et Mac natif ; quatre gates froids passent sur chaque Docker, garde et runtimes32 passent natifs. Fixture réparée pour RocksDB11 (API unique_ptr détectée), qualifiée sur les trois environnements ; core inchangé par cette réparation. Échecs natifs de compilation/pilote et bind sandbox conservés (seul BLS rejoué avec le même binaire), hashes des programmes et sources vérifiés. [Sources et périmètre](qualification/coherent-cold-core-e33ad89e-20261006/README.md). Il ne s’agit pas de la suite complète427/EEST ni d’une restauration des nœuds historiques. Aucun déploiement ; graphe/capture1024 et nouvelle finalité WAN restent ouverts, ainsi que tous les critères de production et Base–Nano.

État précédent conservé : Main parent `7121cb60` : préflight de publication froide raccordé au mémo local, qualifié Docker ARM64/AMD64. Deux lignes certifiées réutilisées, chaque blob toujours lu et hashé ; dix refus adversariaux et repli vers le décodeur complet vérifiés. Quatre scénarios distincts par architecture : writer/restorer, quatre EIO et trois SIGKILL, runtime automatic/BLS32 blocs avec pruning/reprises. Hashes parent/programmes inchangés ; deux core TU et fixture recompilés, pas un build propre de release. Démarrage et opérations signées gardent leurs validations complètes. [Sources, preuves et limites](qualification/capella-reference-preflight-reuse-20261006/README.md). Coût des signatures, graphe1024, restauration physique et finalité WAN restent ouverts ; aucune candidate déployée, aucun statut EIP promu.

État précédent conservé : Main parent `5f2977a6` : couverture SIGKILL froide intégrée, trois frontières WAL/commit qualifiées ARM64/AMD64 ; quatre EIO de régression chacun car shim de test modifié. Archives signées, oracle exact, retry/réouverture idempotents ; hashes parent/test/shim inchangés. Gate CTest Linux enregistré et configure/inventaire vérifiés sur les deux architectures. [Sources et limites](qualification/capella-cold-memo-sigkill-20261006/README.md). Core inchangé, aucune nouvelle candidate déployée. Ce SIGKILL ne prouve pas panne électrique ni restauration d’engagements réels ; préflight complet, graphe1024 et finalité WAN restent ouverts.

État précédent conservé : Main parent `fa93bc17` : nouvelle couverture WAL froide intégrée et qualifiée ARM64/AMD64, quatre dispositions par architecture (principale/témoin, suffixe conservé/perdu). Oracle exact, archives signées, fail-closed et reprise idempotente. Cinq gates CTest enregistrés ; configure/inventaire ARM64 vérifiés sans répétition des tests. [Sources, preuves et limites](qualification/capella-cold-memo-sync-20261006/README.md). Core mémo qualifié précédemment inchangé. SIGKILL, panne physique, graphe1024 et restauration/finalité WAN restent ouverts ; aucune nouvelle candidate déployée.

État précédent conservé : Main parent `1c84e7b3` : mémo transactionnel du writer froid raccordé au réacteur, qualifié Docker ARM64/AMD64 par quatre scénarios séparés chacun. Runtime32 blocs/cache8, 64 journaux, pruning/reprises et engagements externes9 conservés ; hashes parent/programmes inchangés et1616 sources publiques identiques au lot intégré. Préflight durable complet conservé. [Sources, preuves et limites](qualification/capella-reference-memo-runtime-20261006/README.md). Nouvelle couverture de panne WAL : quatre dispositions ARM64 passent, AMD64 active68450 ; pas encore intégrée. Aucune nouvelle candidate déployée.

État précédent conservé : Main parent `681f29eb` : runtime froid et duplicate-fastpath livrés. Lot writer par en-têtes qualifié sur ARM64/AMD64 : première passe complète conservée, seconde passe sans rechargement des états ; oracle/cache1/refus7, corruption réacteur3 et runtime32 blocs/cache8 avec pruning/reprises terminaux0. Hashes parent/programmes inchangés. [Sources, preuves et limites](qualification/capella-cold-header-links-20261006/README.md). Aucune nouvelle candidate déployée.

Production reste ouverte : capacité du graphe (1024 blocs retenus dans V28, distincte du nombre de validateurs), coût des publications froides, GC/RSS/crashs, reprise physique et finalité WAN commune, autres critères techniques, lancement Base–Nano et revue indépendante. Les sections ci-dessous sont l'historique de preuves : leurs états anciens ne remplacent pas cet état courant. Aucun statut EIP supplémentaire promu.

## Codec du manifeste compact Capella : Docker ARM64/AMD64 — 6 octobre 2026

Le codec local remplace les états complets par des références racine/hash/longueur, conserve les métadonnées et opérations, authentifie le manifeste avant tout fetch puis restitue exactement le checkpoint engagé. Fixture synthétique de trois records : 25269 octets de checkpoint, 854 de manifeste, fragments séparés ; quinze refus adversariaux et réouverture RocksDB réussis sur ARM64/AMD64. Ce n’est pas une réduction mesurée du stockage/IO total. [Sources et preuves](qualification/capella-fragment-manifest-codec-20261006/README.md).

**Codec qualifié, réacteur/runtime non raccordés.** Publication/témoin/migration et cache de branches froides restent à intégrer. La candidate runtime `02ec0ab0`, son format complet et le plafond 1024 restent inchangés ; saturation V28 non corrigée. Aucune qualification EIP/consensus/WAN ou production déduite ; tous les critères et échecs historiques, revue indépendante et lancement Base–Nano restent ouverts.

## Stockage des fragments immuables : Docker ARM64/AMD64 — 6 octobre 2026

Le backend RocksDB publie maintenant des fragments opaques SHA256 dans le même batch synchrone que le checkpoint/token et, si demandé, l’exécution. Contenu immutable, liaison chainId/genesis, CAS, budgets et refus de corruption ; une écriture incertaine bloque ce handle jusqu’à réouverture. Deux nouveaux modes et la régression d’exécution actuelle passent sur les deux architectures : duplication/CAS/budgets/copie readonly/altération, fsync avec deux dispositions du WAL, puis CAS/reorg/archives et SIGKILL. Les deux erreurs de préparation des fixtures sont conservées. [Sources, preuves et périmètre](qualification/checkpoint-immutable-blobs-docker-20261006/README.md).

**Ce lot est la base de stockage, pas la correction de saturation V28.** Aucun nœud/image remplacé ; candidate runtime `02ec0ab0` et limite 1024 inchangées. Raccorder le manifeste authentifié, le cache borné/rechargement des branches froides, le témoin séparé et la migration, puis qualifier fork choice/saturation/pannes et une nouvelle finalité commune. Aucun statut EIP promu ; tous les critères originaux, autres échecs, qualification native de ce nouveau lot, portabilité physique, WAN A, bridge/Doppler et revue indépendante restent ouverts. Production non qualifiée.

## V28 terminé : saturation du graphe sur les deux Mac — 6 octobre 2026

Main `ee08b6ce`, cœur `02ec0ab0`, relais compilé `2f93dda0` réutilisé par ses entrées de compilation. Contrôleur original 7884 : sortie 1, aucune avancée de finalité commune dans les 900 secondes, baseline 1284. Les trois arrêts sont propres ; audits 10017/73931 réussis, fichiers inchangés et engagements de reprise/signature comparés séparément. [Preuves terminales et diagnostic](qualification/restored-three-node-v28-terminal-graph-capacity-20261006/README.md).

Le graphe restauré contient exactement 1024 blocs/1024 nœuds sur chacun des deux Mac, soit leur limite configurée. Le VPS conserve 627 blocs sur 1024 et n’est pas saturé. Le code refuse les nouvelles admissions et propositions locales à capacité atteinte : cause suffisante de refus sur les Mac, pas explication unique du WAN ni du VPS. Les nombres du rejeu canonique ne mesuraient pas la totalité de ce graphe. Ancre déjà au slot 10271 et un seul bloc avant/à la frontière finalisée ; le pruning conditionné à une nouvelle ancre ne libère pas ces descendants non finalisés. Snapshots : 90042255/89807770/54161501 octets (natif/Docker Mac/VPS). Lecteurs diagnostiques en lecture seule, mêmes engagements et fichiers inchangés ; aucun nouveau binaire de production ni image.

**Prochaine correction : rétention persistante authentifiée du graphe/états, rechargement borné de branches froides et publication/restauration atomiques**, sans supprimer arbitrairement des branches valides ni seulement augmenter le plafond. Ajouter les scénarios saturation/fork choice/rechargement/archives altérées/panne, puis qualifier une nouvelle finalité commune après correction pertinente. Cette correction n’est pas implémentée ni qualifiée à ce stade. Aucun statut EIP n’est promu par ce diagnostic. Tous les critères originaux, échecs, cycle/rotation des validateurs, WAN A, ressources, bridge/Doppler et revue indépendante restent ouverts ; production non qualifiée.

## Objectif courant et ordre de livraison — 4 octobre 2026

**Cible : livrer Nanobees/Nano comme L1 de production et préparer son fair
launch sur Doppler/Base en USDC, avec une relation définie et vérifiable entre
le token Base et le NANO natif.** Ni une suite verte, ni un nœud sur VPS, ni
une cotation ne suffisent à clore cet objectif. Aucun lancement public, fonds
réels ou ouverture de vente n’est autorisé par cette feuille de route.

Les exigences originales et les quatre matrices ci-dessous restent intégrales.
Les observations historiques sont des preuves datées, pas les prochaines tâches
courantes. Le goal reste actif. Travailler par lots fonctionnels complets ;
réutiliser les preuves dont la révision et le périmètre concordent. Ne relancer
une qualification que pour un changement pertinent ou un écart non résolu.

1. **Version C++ cohérente.** Achever les deux campagnes Docker existantes de
   `efbeb9bc` sans les redémarrer ; conserver leurs sorties terminales, audits
   et manifestes. Compléter la portabilité physique ARM64/AMD64 et la restauration
   effective de cette version, journaux et témoins compris, puis démontrer la
   reprise des duties et l’avancement de finalité commune Mac natif/Docker Mac/VPS.
   Les preuves WAN de `d1597d9a` et les lectures de journaux ne suffisent pas à
   attribuer toutes ces réussites à la nouvelle candidate.
2. **Consensus et exploitation.** Fermer les critères encore ouverts du cycle
   des validateurs, entrée/sortie, rotation, sanctions, sync, charge bornée,
   ressources, pruning et anomalie WAN A ; réutiliser les partitions/asymétries
   déjà qualifiées dans leur périmètre exact. Finaliser sauvegardes, mises à jour,
   supervision/alertes et CI reproductible. Conserver la revue indépendante
   comme exigence ouverte tant qu’elle n’est pas réalisée.
3. **Actif Base–Nano et bridge.** Définir allocations, réserve canonique, ratio,
   droits de conversion/remboursement et autorité des messages. Implémenter puis
   qualifier dépôts/retraits, finalité source, reorgs, rejeu, compromission et
   reprises idempotentes, sans promettre une sécurité non démontrée.
4. **Lancement Doppler.** Terminer la configuration réelle : identité, offre,
   inflation, dates, ticks/prix, frais, gouvernance, migration, invendus et seuils.
   Utiliser le SDK officiel et l’outil de préparation livré ; simuler création,
   achats, ventes et migration avec ces termes. L’exemple synthétique actuellement
   rejeté sur Base reste un échec conservé. Vérifier séparément visibilité à T0
   et indexation externe ; ne présumer ni bridge ni retrait des réserves Doppler.
5. **Clôture production.** Réconcilier le profil EVM/RPC réellement activé avec
   les matrices EIP, livrer la candidate identifiée par Git et Docker et publier
   les preuves et limites. Déploiement public et vente seulement après autorisation
   dédiée, lorsque tous les critères applicables sont démontrés.

Livraison immédiate : outil Doppler sans signature/diffusion, puis clôture des
campagnes déjà actives et du lot de restauration cohérente. Pas de reconstruction
d’image pour les seules modifications de documents ou de cet outil JavaScript.

## WAN Q : participation canonique incluse à temps et rejeu vérifié — 1er octobre 2026

Le rejeu canonique Q arrêté passe : 118 blocs avec ancre, 322989 octets, une transaction. Attribution BLS individuelle et contrôle de transitions sont séparés. L’époque 14 compte 62 signataires canoniques distincts, dont 50 inclus avant slot 120 (quorum requis : 43), contre 40 avant cette frontière dans P. L’époque 15 compte 59 canoniques, dont 51 avant slot 128. Les groupes de cibles sont disjoints et couvrent les 64 clés de la fixture. [Preuves et limites](qualification/capella-wan-q-inclusion-timing-20261001/README.md).

Cela concorde avec la justification 14 à temps et la reprise WAN Q réussie ; aucun gain causal contrôlé ni cause unique déduit. Votes hors bloc, historique du fork choice et temps d’arrivée/sélection non rejoués. Prochaine couverture : majorité connectée pendant l’isolement du VPS, puis critères originaux restants. Production non qualifiée ; P/O/N/M et tous les échecs historiques conservés.

## WAN Q : deux reprises sous 180 s et stabilité démontrées — 1er octobre 2026

Campagne 418aceb8 / harness dd17cad, exit 0. Coupure 96 s : finalité commune 2→7 en 135,281 s monotones ; coupure 192 s : 7→14 en 133,181 s. Les deux fenêtres originales de 180 s passent, sans extension tardive ni assouplissement. Stabilité 14→17 en 143,339 s, signataires/horloges sains, conteneurs arrêtés exit 0 sans OOM. [Preuves exactes et limites](qualification/partition-cycles-20261001/attempt-q/README.md).

Ce scénario borné de partitions répétées est qualifié sur cette révision et cette topologie. Il ne qualifie pas tout le réseau : les trois nœuds perdent leurs pairs pendant les coupures. Majorité connectée, asymétrie prolongée, cycle des validateurs, inclusion/rotation sync, charge/échelle, exploitation et revue indépendante restent ouverts selon les critères originaux. P/O/N/M et tous les échecs antérieurs sont conservés ; aucune cause unique ni gain causal contrôlé déduit. Prochaine étape : rejeu canonique Q, puis nouvelle couverture de majorité connectée. Production non qualifiée.

## Attestations intercalées : amd64 et candidats identifiés — 1er octobre 2026

Quatre nouveaux scénarios et la finalité réelle locale passent aussi sur amd64 : deux daemons de 32 clés, époque 2 commune observée trois fois, signataires/horloges sains et arrêts propres. Les traces montrent toutes les catégories de fond servies et les attestations intercalées. Cœur 967a0dc réutilisé par équivalence des sources, P2P recompilé à 418aceb. Une image runtime par architecture est assemblée à partir des binaires qualifiés ; identité, architecture, révision, utilisateur et empreintes sont vérifiés. [Preuves et candidats](qualification/capella-attestation-schedule-portable-20261001/README.md).

Prochaine mesure : campagne WAN avec coupures 96/192 s et reprise monotone 180 s inchangées. Aucun gain WAN encore établi ; P et tous les échecs antérieurs sont conservés. Majorité connectée, cycle des validateurs, autres critères originaux et revue indépendante restent ouverts. Production non qualifiée.

## Attestations intercalées : natif et ARM64 qualifiés — 1er octobre 2026

Le scheduler propose désormais un catalogue d’attestations après chaque catégorie de fond ; tête/blocs, slashing, changements de clés, sync et sorties restent servis dans une fenêtre de dix départs. État borné par pair, budgets/pacer/ACK et intervalle de polling inchangés. Quatre nouveaux scénarios passent en natif et Docker ARM64. Deux vrais daemons de 32 validateurs par environnement observent trois fois une finalité commune à l’époque 2 ; signataires/horloges sains, arrêts propres. Les traces montrent toutes les catégories servies et les attestations intercalées. [Sources, preuves et limites](qualification/capella-attestation-schedule-local-20261001/README.md).

AMD64 et effet sur la partition WAN restent à qualifier. Cœur et helpers de transport inchangés réutilisés avec leur périmètre antérieur ; aucun gain causal ou conformité globale déduit. Anciennes images 86af5f8 conservées, provenance non réutilisable pour les nouveaux P2P natif/ARM64. P et tous les critères de production restent ouverts ; limite WAN de 180 s inchangée.

## WAN P : quorum canonique inclus trop tard à l’époque 14 — 1er octobre 2026

Le rejeu canonique P en lecture seule passe (123 blocs avec ancre, 340748 octets, une transaction). Les signatures individuelles incluses des époques 12–15 sont identifiées par clé BLS ; le rejeu valide séparément comité et transitions. À l’époque 14, 56 clés distinctes ciblent la frontière canonique, mais seulement 40 sont incluses avant le bloc/transition de slot 120 ; les 16 suivantes arrivent dans les blocs 120/121. Le quorum de cette fixture de 64 poids égaux exige 43 : la première justification manque donc de participation incluse à temps. À l’époque 15, 50 des 58 signataires canoniques sont inclus avant slot 128. [Preuves et limites](qualification/capella-wan-p-inclusion-timing-20261001/README.md).

Prochaine priorité : mesurer/réduire le délai de relais, d’arrivée et de sélection des attestations, sans changer quorum ni délai WAN. La cause unique n’est pas attribuée ; votes hors bloc, historique du fork choice et temps de sélection ne sont pas rejoués. Échecs de préparation de requête conservés. Aucune campagne inchangée relancée, aucune clôture de production ou des exigences originales.

## WAN P : priorité de tête exercée, seconde reprise encore hors délai — 1er octobre 2026

Campagne 86af5f8 / harness cddea6c, exit 1. Première reprise 2→7 en 137,847 s monotones. Seconde fenêtre de 180 s échouée : dernier contrôle complet encore époque 7 à 180,245 s, coût 1,718 s ; ce nombre est le moment du constat, pas une durée de récupération réussie. L’époque 15 commune est observée tardivement, 190,589 s après reconnexion en timestamps muraux ; ses 10,342 s monotones appartiennent à une nouvelle fenêtre. Stabilité tardive 15→18 en 137,729 s, signataires/horloges sains, conteneurs exit 0 sans OOM. [Preuves P et limites](qualification/partition-cycles-20261001/attempt-p/README.md).

La priorité de livraison de tête ne ferme pas l’écart WAN ; aucun gain causal n’est démontré. Inspecter les votes canoniques 14/15 et leur inclusion avant de modifier ou répéter la campagne. Les coupures isolent les trois nœuds, sans majorité connectée. Tous les critères originaux, statuts EIP, échecs antérieurs et revue indépendante restent ouverts. Production non qualifiée.

## Livraison prioritaire : amd64 et candidats Docker identifiés — 1er octobre 2026

Les 17 nouveaux scénarios et les régressions catalogue/blocs/attestations passent aussi sur Docker amd64. Deux vrais daemons de 32 validateurs observent trois fois la même finalité à l’époque 2, signataires/horloges sains et arrêts propres. Le cœur 967a0dc est réutilisé par équivalence des sources ; P2P recompilé à 86af5f8. Une image par architecture est assemblée à partir des binaires qualifiés, puis identité, architecture, révision, utilisateur et deux empreintes sont vérifiés. [Preuves, candidats et limites](qualification/capella-head-priority-portable-20261001/README.md).

Prochaine campagne : coupures WAN de 96/192 s et reprise monotone de 180 s inchangées. Aucun gain WAN encore démontré ; O et tous les échecs antérieurs sont conservés. Les autres exigences originales, EIP, cycle des validateurs, opérations et revue indépendante restent ouvertes ; production non qualifiée.

## Livraison prioritaire de la tête : natif et ARM64 qualifiés — 1er octobre 2026

Le relais de blocs livre désormais la tête locale authentifiée et ses ancêtres avant le catalogue historique conservé. ACK du récepteur, quota de dépendances partagé et échéance totale restent obligatoires ; un callback tardif ne peut annoncer une réussite. Les 17 nouveaux scénarios passent en natif et Docker ARM64. Deux vrais daemons de 32 validateurs par environnement observent trois fois une finalité commune à l’époque 2, signataires et horloges sains. Régressions catalogue/blocs/attestations recompilées : 41/25/14 cas. Le cœur de référence privée 967a0dc passe aussi Beacon/santé sur Docker AMD64. [Sources exactes, itérations, empreintes et limites](qualification/capella-head-priority-local-20261001/README.md).

Le raccordement P2P AMD64, les nouvelles images identifiées et le gain après partition WAN restent à qualifier. Une tentative native de tête est non confirmée et conservée ; les compteurs mesurent des événements, pas des blocs uniques. Quorum et limite de reprise de 180 s inchangés ; O et tous les échecs historiques restent ouverts. Les anciennes images et marqueurs ne désignent pas les nouveaux binaires. Aucune clôture d’EIP ou de production ; exigences originales, bridge/fair launch et revue indépendante conservés.

## Référence de tête privée cohérente : natif et Docker ARM64 — 1er octobre 2026

Le statut privé Capella fournit désormais headSlot/headRoot issus d'une même publication durable authentifiée. Le daemon attendait déjà headSlot, absent auparavant. Nouveaux contrôles de cohérence, annulation, lecture sous admission retenue et champs HTTP après panne du signataire passent sur natif et Docker ARM64 ; les régressions Beacon/santé passent aussi. [Sources, journaux, empreintes et limites](qualification/capella-relay-head-arm64-20261001/README.md).

Aucune priorité de livraison P2P encore ajoutée, aucun gain WAN démontré. AMD64 et raccordement du transport restent à qualifier. Images runtime existantes inchangées ; le binaire natif et le cœur du volume ARM64 diffèrent maintenant du candidat 9513ee9, marqueur non avancé : ne pas réutiliser son ancienne provenance. O et toutes les exigences originales restent ouvertes. Production non qualifiée.

## WAN O : signataires individuels des cibles identifiés — 1er octobre 2026

L'inspecteur hors ligne en lecture seule identifie chaque signature BLS individuelle incluse des époques 12–15 avec le domaine du fork et le registre fixe de la fixture ; correspondance unique, champs et comptes précédents inchangés. Le rejeu canonique valide séparément comité et transitions. Pour l'époque 14, la cible retenue compte 35 clés distinctes (Mac natif 22, Docker Mac 11, VPS 2), la cible antérieure 19 (VPS 18, Docker Mac 1). Dix clés ne figurent pas dans ces votes inclus ; leur absence générale du réseau n'est pas démontrée. [Preuves et limites](qualification/capella-wan-o-signer-identification-20261001/README.md).

La majorité des signatures VPS incluses vise encore l'ancienne cible pendant l'époque 14. Cause de convergence tardive, votes hors bloc et choix historique de branche restent à diagnostiquer ; aucune sanction ou double-signature n'est déduite, aucun signataire ne figure sur les deux cibles dans cet extrait. Aucun binaire runtime/image reconstruit ni campagne répétée. Tous les critères initiaux et échecs restent ouverts ; production non qualifiée.

## WAN O : dépendances livrées, seconde reprise toujours hors délai — 1er octobre 2026

Campagne `9513ee9` / harness `eb3cf64`, exit 1, images existantes réutilisées. Première reprise 2→7 en 139,501 s monotones. Seconde fenêtre 180 s expirée : dernière lecture complète à 179,243 s encore époque 7, coût 3,044 s ; expiration journalisée à 180,002 s. Ce dernier nombre est le moment du constat, pas une durée de récupération. La lecture tardive 7→15 arrive 181,946 s après reconnexion en timestamps muraux ; ses 1,941 s monotones appartiennent à une nouvelle fenêtre. Stabilité tardive 15→18 en 141,746 s, trois signataires/horloges sains, conteneurs exit 0 sans OOM. [Preuves O et diagnostics exacts](qualification/partition-cycles-20261001/attempt-o/README.md).

Le transport des dépendances d'attestation ne ferme pas la seconde reprise. Aucun gain comparatif ni cause unique démontré ; analyser votes canoniques et convergence avant une nouvelle campagne. Pendant les coupures les trois nœuds ont zéro pair : ce scénario ne qualifie pas une majorité connectée qui continue de finaliser. Les 47 mesures de ressources sont descriptives ; BlockIO ne mesure pas l'occupation disque. Tous les échecs antérieurs, exigences originales et statuts EIP restent conservés. Production non qualifiée ; bridge/fair launch et revue indépendante restent ouverts.

Le rejeu canonique natif O arrêté passe (117 blocs avec ancre, 332463 octets, une transaction). À l'époque 14, 35 occurrences de participation ciblent la frontière retenue, contre 19 une autre cible ; 35 ne peuvent fournir les 43 participants distincts requis dans cette fixture de 64 poids égaux. Ce sont des occurrences, pas des comptes uniques ; la cause de la répartition reste ouverte. [Inspection O en lecture seule](qualification/capella-wan-o-vote-inspection-20261001/README.md). Votes hors bloc et choix historique de branche non rejoués.

## Dépendances des votes : amd64 qualifié et images identifiées — 1er octobre 2026

Les 14 nouveaux modèles, 41 scénarios de catalogue et 25 de blocs passent aussi dans Docker amd64. Deux nœuds de 32 validateurs finalisent une racine commune à l'époque 2 (trois observations stables, 204 admissions de payloads d'attestation, signataires sains). Les images ARM64/amd64 ont été assemblées une fois et leurs empreintes/identités vérifiées ; le cœur eb8d3ed est réutilisé avec preuve d'équivalence des sources à 9513ee9, P2P recompilé. [Preuves et limites](qualification/capella-attestation-portable-images-20261001/README.md).

Prochaine qualification : mêmes coupures WAN 96/192 s, reprise monotone 180 s, budgets et quorum inchangés. Aucun gain WAN encore démontré ; M/N, les autres scénarios de production et toutes les exigences originales restent ouverts.


## Transport des attestations avec dépendances : natif et ARM64 qualifiés — 1er octobre 2026

Le daemon livre désormais le bloc voté et ses ancêtres authentifiés avant l'attestation, en exigeant les ACK réels et en conservant les budgets configurés et la séparation des caches par pair/session. 14 nouveaux cas de dépendances/annulation/refus/quotas passent ; les régressions recompilées passent 41 cas de catalogue et 25 de blocs sur Mac natif et Docker ARM64. Les premiers exécutables natifs de régression étaient anciens ; journaux et périmètres corrigés sont conservés.

Le vrai daemon démontre séparément une finalité locale commune à l'époque 2 (trois observations stables) avec deux groupes de 32 validateurs sur natif et Docker ARM64. [Sources, journaux et limites](qualification/capella-attestation-dependencies-20261001/README.md). L'effet sur les coupures WAN et la qualification amd64 restent ouverts ; la répartition des votes N n'est pas attribuée définitivement à ce seul mécanisme. Aucune règle de consensus, quorum, signature ou échéance de reprise n'est modifiée. Pas de nouvelle image runtime ni de production qualifiée.


## WAN N : attestations incluses inspectées hors ligne — 1er octobre 2026

Le rejeu canonique de la base native N arrêtée passe : 114 blocs avec ancre, 328604 octets archivés, états/index/reçus vérifiés. L'extraction des attestations compte à l'époque 14 seulement 38 occurrences de participation visant la frontière retenue, contre 22 visant une cible concurrente ; la cible canonique 15 en compte ensuite 58. Les cibles concurrentes ne peuvent pas être additionnées pour prétendre au quorum. [Archives, requête et limites](qualification/capella-wan-n-vote-inspection-20261001/README.md).

Les votes hors bloc et le choix historique de branche ne sont pas rejoués. La cause de leur répartition reste à qualifier ; le transport envoie actuellement les attestations sans résoudre les blocs dépendants, contrairement aux livraisons de blocs. Un raccordement des dépendances est en développement ; aucun gain WAN n'est encore démontré. M/N et tous les critères de production restent ouverts.


## Signataire qualifié sur trois environnements ; diagnostic WAN précis — 1er octobre 2026

Les nouveaux scénarios `--signer-health` et la régression `--managed-cli` passent sur Mac natif, Docker ARM64 et Docker amd64 VPS à `eb8d3ed`. Les sources et empreintes des binaires sont vérifiées, les journaux complets archivés. [Preuves et limites](qualification/capella-signer-health-portable-20261001/README.md). La panne réelle du daemon distribué, cpp-libp2p sous cette panne et la livraison externe de l'alerte restent ouverts.

Le banc WAN conserve maintenant la durée monotone exacte des échecs et le coût de la dernière lecture, sans ajouter de RPC, modifier les budgets ou convertir une réussite tardive en succès. Quatre nouveaux cas déterministes et 37 tests pertinents passent. [Diagnostic et preuves](qualification/partition-monotonic-diagnostics-20261001/README.md). Aucun nouveau gain réseau : M/N et tous les critères originaux demeurent ouverts. Les images WAN n'ont pas été reconstruites et restent sur `1e1e732`.


## Santé du signataire et alerte locale : ARM64 qualifié — 1er octobre 2026

Santé non bloquante du signataire ajoutée au RPC privé `nano_capellaStatus` (`validatorHealthy`, null sans signataire), et alerte unique via le point d'entrée partagé du monitor. Deux nouvelles pannes réelles du fournisseur passent : avant réservation et après réservation durable d'une proposition. HTTP privé, mauvais token, Origin 403, absence de fuite, annulation saine, aucune nouvelle tentative du slot réservé, admission du slot suivant avec les mêmes journaux et réouverture de 64 témoins/journaux par cas sont vérifiés. La régression CLI utilise un enfant P2P simulé et conserve l'arrêt du nœud sur panne de l'enfant. [Preuves ARM64](qualification/capella-signer-health-arm64-20261001/README.md).

L'admission continue après panne du signataire ; aucune reprise automatique du fournisseur n'est ajoutée. Mac natif, amd64, alerte depuis le daemon en panne réelle et livraison externe restent à qualifier. L'alerte locale est best-effort ; la santé ne garantit pas la liveness. Aucun gain WAN : N et tous les critères originaux de production restent ouverts.


## WAN N : le scheduler ne résout pas la seconde reprise — 1er octobre 2026

Campagne `1e1e732` / harness `06004e2`, exit 1. Première coupure 96 s : reprise monotone 140,895 s, finalité commune 2→7. Après coupure 192 s, la première lecture complète 7→15 est rejetée par le seuil monotone strict 180 s ; différence des timestamps muraux 180,127 s, durée monotone complète exacte non journalisée. Les 2257 ms du diagnostic tardif correspondent à une nouvelle fenêtre. Aucune erreur HTTP Beacon n'interrompt N. La stabilité ultérieure 15→18 passe en 144,311 s ; trois signataires/horloges sains, conteneurs exit 0 sans OOM. [Preuves N](qualification/partition-cycles-20261001/attempt-n/README.md).

Aucun gain WAN démontré, aucun échec antérieur effacé. La protection des signatures existait déjà ; le filtrage du scheduler ne ferme pas la reprise. Prochaine analyse : convergence, votes/inclusion et coût des observations, avec journalisation précise de la durée monotone manquante. Ne pas rejouer le même candidat pour obtenir du vert, ni relever les budgets. Tous les critères originaux et la production restent ouverts.


## Correction du périmètre de timing — 1er octobre 2026

La vérification du code antérieur `030871a` confirme que `sync_current` rejetait déjà les signatures avant un tiers de slot sans bloc courant. Pour le profil actif (6000 ms/slot, échéance 2000 ms), cette protection et celle des attestations existaient avant `1e1e732`. L'affirmation précédente d'une protection sync absente était incorrecte. Le nouveau garde du scheduler évite des calculs/tentatives prématurés ; le bornage du polling qualifie séparément la réactivité du worker. Les cas passant ne démontrent pas une vulnérabilité de signature antérieure corrigée. [Code antérieur et rectification](qualification/capella-duty-timing-scope-20261001/README.md).

Aucun gain WAN ni cause de M n'est établi. N continue sur les sources, images, harness et budgets figés ; cette correction est documentaire uniquement. Les preuves historiques sont conservées et aucun critère EIP/production n'est fermé.


## Timing et candidats qualifiés sur les trois environnements — 1er octobre 2026

La révision `1e1e732` passe les quatre nouveaux cas de timing, le scheduler (40 blocs, finalité 3, 64 journaux repris) et la reprise automatique (16 blocs, témoins des signataires) sur Mac natif, Docker ARM64 et Docker amd64. Les deux images ont été assemblées une fois puis vérifiées indépendamment ; les empreintes du binaire natif sont conservées. [Preuves et limites](qualification/capella-duty-timing-images-20261001/README.md).

Prochaine qualification : partitions WAN 96/192 s, reprise monotone strictement 180 s et budget Beacon 2 s inchangés. Aucun gain WAN encore démontré ; M et tous les écarts historiques demeurent ouverts. Ce correctif applique le délai du profil aux duties sync ; il ne ferme aucune nouvelle EIP ou exigence globale de production.


## Timing local des duties : Docker ARM64 qualifié — 1er octobre 2026

Le scheduler applique aux messages sync le délai du profil déjà utilisé par les attestations : bloc admis du slot courant ou échéance configurée. Le polling revisite les duties différées avant la fin du slot. Quatre nouveaux cas passent, ainsi que le scheduler (40 blocs, finalité 3, 64 journaux repris) et la reprise automatique (16 blocs, témoins des signataires). Les trois échecs de fixture sont conservés. Preuves : [timing ARM64](qualification/capella-duty-timing-arm64-20261001/README.md).

Qualification locale ARM64 uniquement ; Mac natif, amd64 et gain WAN restent à démontrer. M reste en échec et les critères originaux, matrices EIP et budgets sont inchangés. Production non qualifiée.


## WAN M : lectures disponibles, seconde reprise tardive — 1er octobre 2026

La campagne `4343023` / harness `21142f1` termine exit 1. Première reprise après coupure 96 s : 138,377 s, finalité 2→7. Après coupure 192 s, la première lecture complète de finalité commune 7→15 est rejetée par le seuil monotone strict 180 s. Les timestamps muraux donnent 180,040 s ; la durée monotone complète exacte n’est pas journalisée. Les 1888 ms du diagnostic tardif mesurent une nouvelle fenêtre, pas la reprise originale. Aucune erreur HTTP Beacon n’interrompt M. Stabilité ultérieure 15→18 en 144,145 s ; trois signataires/horloges sains, conteneurs exit 0 sans OOM. Preuves : `qualification/partition-cycles-20261001/attempt-m/`.

Le correctif de publication permet cette observation complète dans M, sans qualifier universellement les lectures ni fermer L/J/I/K/A. La reprise doit gagner une marge réelle : analyser convergence, inclusion des votes et frontières de finalité, sans relever le seuil, réduire les règles de consensus ou refaire un essai identique pour obtenir du vert. Toutes les exigences originales et les échecs historiques restent conservés ; production non qualifiée.

## Candidats Beacon portables prêts : deux architectures qualifiées — 1er octobre 2026

La révision `4343023` passe les tests Beacon/HTTP sur Mac natif et les tests Beacon/HTTP, horloge et reprise automatique sur Docker ARM64 et amd64 (16 blocs, 64 clés, témoins/signataires repris). Limite de fichiers Docker explicite 65536. Deux images assemblées une fois puis vérifiées indépendamment : architecture, révision, utilisateur et empreintes. Preuves : `qualification/beacon-portable-images-20261001/`.

Prochaine mesure : trois nœuds, partitions 96/192 s, reprise strictement 180 s et budget HTTP deux secondes, trace détaillée désactivée comme L. Aucun gain WAN qualifié ; tous les critères originaux et les échecs historiques restent ouverts.

## Publication Beacon portable : Mac natif et Docker ARM64 qualifiés — 1er octobre 2026

Les deux builds et tests Beacon/HTTP terminent exit 0 : capture et archive disponibles pendant une admission retenue, publication ancienne conservée, admission annulée non publiée, budget HTTP et confidentialité inchangés. Docker ARM64 qualifie aussi les quatre cas d’horloge et la reprise automatique (16 blocs, 64 clés, témoins et signataires repris). Preuves et empreintes exactes : `qualification/beacon-portable-arm64-20261001/`.

Le test de reprise amd64 de la source précédente `6799cb4`, rejoué sans build avec nofile=65536, passe : l’échec initial 1024 reste conservé. La modification portable doit encore être qualifiée sur amd64 et en WAN. Aucun critère original ni échec historique retiré ; production non qualifiée.

## Portabilité Beacon et qualification amd64 incomplète — 1er octobre 2026

La source `6799cb4` compile dans Docker ARM64 ; le Mac natif rejette `atomic<shared_ptr>` avec sa libc++. Sur amd64, lectures sous admission retenue, archives, phases, budget HTTP et horloge passent ; la reprise automatique échoue à l’ouverture RocksDB du journal key-63 (limite de fichiers 1024 du conteneur de test). Le harness réseau utilise déjà 65536. Échecs conservés dans `qualification/beacon-atomic-portability-20261001/`.

Le code emploie désormais les opérations atomiques standard sur `shared_ptr`, compatibles avec la libc++ locale (sonde compilée/exécutée), avec ordres acquire/release identiques et aucun accès non atomique. Nouvelle qualification trois environnements à faire, avec limite de fichiers explicite 65536. Aucun résultat WAN ni production clôturé ; tous les critères et preuves antérieurs conservés.

## Publication Beacon atomique : concurrence ARM64 qualifiée — 1er octobre 2026

La publication immuable état/token est préallouée avant commit et installée atomiquement après réussite durable. La capture Beacon n’attend plus le verrou d’admission ; les archives restent vérifiées avec le stockage et l’état capturé. Le nouveau test suspend réellement une admission sous mutex : lectures publication/archive terminées, ancienne réponse conservée, candidat annulé non publié. Docker ARM64 exit 0 ; quatre cas de fermeture sur panne d’horloge et reprise automatique par témoin passent (16 blocs, 64 clés). Preuves : `qualification/beacon-atomic-arm64-20261001/`.

Le mutex DB et le coût de projection demeurent ; amd64 et effet WAN restent à qualifier. Budget HTTP deux secondes, reprise 180 s et tous les critères originaux conservés. L et les échecs précédents ne sont pas clôturés par cette preuve locale.

## WAN L : capture de publication à 3,102 s identifiée — 1er octobre 2026

L termine exit 1 : première reprise 140,185 s, finalité commune 2→7 ; après la seconde coupure, HTTP 503 VPS headers/head, phase `publication-pin`, durée totale 3102 ms. L’annulation est constatée immédiatement après `current()` sous verrou d’admission, avant le calcul Beacon. Cette durée murale ne mesure pas exclusivement l’attente du mutex. Trois signataires et horloges sains ; seconde reprise et stabilité non qualifiées. Preuves : `qualification/partition-cycles-20261001/attempt-l/`.

La trace détaillée est désactivée dans L, activée dans K : aucun gain de performance causale revendiqué. Prochaine correction : capture cohérente de la dernière publication immuable sans verrou d’admission, avec vérification des archives conservée. Budget HTTP deux secondes, reprise 180 s, historique des échecs et tous les critères originaux inchangés.

## Candidats de mesure Beacon prêts — deux architectures qualifiées — 1er octobre 2026

Docker amd64 termine exit 0 : les nouveaux diagnostics publication/commitment, checkpoint inchangé, concurrence lecture/admission et réponse HTTP par phase passent. Les images ARM64/amd64 et le binaire Mac de source `72ea2ed` sont prêts ; identité, architecture, révision, utilisateur et empreintes vérifiés après assemblage. Preuves : `qualification/beacon-phase-images-20261001/`.

La prochaine campagne mesure la phase où l’annulation est constatée avec la durée totale de lecture. Partitions 96/192 s, budget HTTP deux secondes et seuil de reprise 180 s inchangés. Aucun gain WAN ni correction de performance démontrés ; K et tous les critères originaux restent ouverts.

## Mesure par phase des annulations Beacon — ARM64 qualifié — 1er octobre 2026

Les réponses d’annulation indiquent désormais une phase fixe et la durée totale de lecture. La phase identifie où l’annulation a été constatée, sans prouver à elle seule quelle phase consomme le temps. Docker ARM64 exit 0 : annulations publication/commitment, checkpoint inchangé, budget HTTP réel inchangé, phase transmise et détails privés masqués. Sept tests de l’observateur passent, dont deux nouveaux cas phase/durée. Preuves : `qualification/beacon-phase-arm64-20261001/`.

Qualification amd64 et mesure WAN restent à faire. Aucun correctif de performance n’est revendiqué ; K et les échecs précédents restent ouverts, ainsi que tous les critères originaux.

## WAN K : annulation/budget Beacon identifié, seconde reprise non qualifiée — 1er octobre 2026

K (`a3bd216`, harness `4d93202`) termine exit 1. Première reprise acceptée en 142,372 s, finalité commune 2→7. Après la seconde coupure, le VPS répond HTTP 503 sur `states/head/finality_checkpoints` avec « Beacon read cancelled or execution budget expired ». Le nouveau diagnostic identifie la branche `ExecutionCancelled` ; il ne distingue pas encore annulation/budget ni la phase coûteuse. La campagne s’arrête avant la seconde reprise complète et la stabilité. Trois signataires et horloges sains, Docker exit 0 sans OOM. Preuves : `qualification/partition-cycles-20261001/attempt-k/`.

La projection hors verrou est qualifiée localement, mais elle ne suffit pas à éliminer les erreurs de lecture WAN. Le budget HTTP de deux secondes, le seuil de reprise 180 s et tous les critères originaux restent inchangés. K/J/I/H/A/B/G et les autres gates ne sont pas clôturés.

## Candidats Beacon vérifiés sur les deux architectures — 1er octobre 2026

La qualification Docker amd64 termine exit 0 : admission concurrente à une lecture suspendue, réponse de publication conservée, 14 contrôles Beacon, budget HTTP inchangé et capacité réutilisée. Les nouveaux exécutables Mac et images ARM64/amd64 de la source `a3bd216` sont prêts ; identités, révision, utilisateur et empreintes vérifiés après assemblage. Preuves : `qualification/beacon-publication-images-20261001/`.

La nouvelle campagne garde les partitions 96/192 s et le seuil strict de reprise 180 s. Aucun effet WAN du correctif n’est encore qualifié ; les échecs J/I/H/A/B/G et tous les critères originaux restent ouverts.

## Lectures Beacon : concurrence ARM64 qualifiée, WAN J en échec — 1er octobre 2026

J termine exit 1 : première reprise 136,153 s, finalité 2→7 ; après la seconde coupure, HTTP 503 sur les headers du VPS avec diagnostic « Beacon state unavailable ». Trois signataires et horloges sains à l’arrêt ; seconde reprise et stabilité non mesurées. Preuves : `qualification/partition-cycles-20261001/attempt-j/`. L’exception exacte reste inconnue : l’ancien binaire confond annulation/budget avec les autres exceptions.

Le correctif capture une publication authentifiée immuable puis libère le verrou pour le calcul Beacon ; l’archive reste vérifiée séparément, sans fallback. Docker ARM64 exit 0 : nouveau bloc admis pendant une lecture suspendue, ancienne réponse conservée, 14 contrôles Beacon, expiration HTTP réelle à budget inchangé, détails privés masqués et capacité réutilisée après douze erreurs. Deux brouillons de sélecteur de fixture échoués sont conservés. Preuves : `qualification/beacon-publication-arm64-20261001/`. Qualification amd64 et nouveaux binaires/images restent à faire ; aucun gain WAN ni cause J/I/H/A/B/G clôturée, tous les critères originaux conservés.

## WAN I : première reprise acquise, lecture Beacon 503 en échec — 1er octobre 2026

L’essai `67c4103` / harness `de8f892` termine exit 1. Première reprise après partition 96 s acceptée en 140,657 s, finalité commune 2→7. Après la seconde partition 192 s, l’observateur reçoit HTTP 503 sur les headers Beacon du VPS et s’arrête avant la mesure complète de reprise ; aucune stabilité finale qualifiée. Trois signataires et horloges sains à l’arrêt, conteneurs exit 0 sans OOM. Cache utilisé (129/119/134 constructions, 4749/3243/4288 hits) : ceci ne prouve aucun gain WAN. Preuves : `qualification/partition-cycles-20261001/attempt-i/`.

`curl -f` a supprimé le corps 503 : capacité, expiration de file et état indisponible ne sont pas distinguables dans cette trace. L’observateur conserve désormais le statut et le diagnostic : cinq nouveaux tests passent, dont un vrai serveur HTTP/curl. Tout échec reste fatal, sans réessai automatique ni seuil relâché. Preuves : `qualification/http-read-diagnostics-20261001/`. H/A/B/G et tous les autres critères restent ouverts ; I ne valide pas la seconde reprise.

## Candidats cache sync prêts pour mesure WAN — 1er octobre 2026

Les targets Nano natif et Nano/P2P Docker ARM64/amd64 ont terminé. Les deux images empaquetées sont contrôlées séparément après assemblage : révision `67c4103`, architectures, utilisateur non-root et empreintes des exécutables. Manifest : `qualification/sync-context-images-20261001/manifest.json`. Les tests locaux du cache et des trois rotations passent sur les deux architectures.

La préparation ne démontre pas le gain WAN. Le prochain essai utilise les trois nœuds et les mêmes partitions 96/192 s, seuil de reprise strict 180 s et stabilité commune. H/A/B/G, historique du choix de branche/votes, autres gates opérationnelles et sécurité indépendante restent ouverts.

## Cache sync : rotations locales qualifiées sur les deux architectures — 1er octobre 2026

La rotation amd64 termine exit 0 : 216 blocs, trois rotations, 215 agrégats vérifiés, finalité 25, reprise témoin au slot 127 et 64 journaux rouverts. Les suites locales ARM64 et amd64 passent les nouveaux cas du cache. Preuves complémentaires : `qualification/sync-context-amd64-20261001/`.

Le candidat amd64 est en compilation ; la mesure WAN du cache reste à faire. Ces tests isolés n’utilisent pas le transport réseau, ne clôturent pas H/A/B/G et ne qualifient pas le choix de branche historique ou le rejeu des votes autonomes. Le seuil de reprise strict 180 s et tous les critères de production sont conservés.

## Cache sync : amd64 local et candidat ARM64 vérifiés — 1er octobre 2026

La suite amd64 termine exit 0 : les deux nouveaux scénarios du cache passent, 40 blocs, finalité 3, deux stores et 64 journaux rouverts. Le Nano natif et les targets Docker ARM64 sont compilés ; l’image ARM64 `ce3cabd0fa6a` contient le cache, avec révision, utilisateur et empreintes vérifiés. Preuves : `qualification/sync-context-preparation-20261001/` (source `67c4103`, identique à `7bba333` pour le code).

À cette photographie, la rotation amd64 tourne encore ; binaire/image amd64 et nouvelle qualification WAN restent à terminer. Aucun gain réseau démontré, aucune cause H/A/B/G clôturée. Le seuil strict de 180 s et tous les autres critères de production sont conservés.

## Cache du contexte sync : ARM64 local qualifié — 1er octobre 2026

`7bba333` réutilise une entrée dérivée du parent admis et du slot d’inclusion entre messages individuels et agrégats, sans supprimer de vérification BLS, temporelle ou de quorum. Les nouveaux contrôles couvrent réutilisation, signature forgée, changement/retour de slot et parent changé au même slot avec ancien root rejeté. Suite ARM64 exit 0 : 40 blocs, finalité 3, deux stores et 64 journaux rouverts. Rotation ARM64 exit 0 : trois rotations, 215 agrégats indépendamment vérifiés, finalité 25 et reprise témoin. Preuves : `qualification/sync-context-arm64-20261001/`.

Qualification amd64 en cours, images/natif à actualiser ; aucun gain WAN démontré ni cause H/A/B/G clôturée. L’audit canonique ne qualifie toujours pas le choix de branche historique ou le rejeu des votes autonomes. Tous les autres critères de production et le seuil strict 180 s sont conservés.

## WAN H : stabilité saine, délai toujours en échec — 1er octobre 2026

L’essai `b76cc1d` termine exit 1 : première reprise acceptée en 135,742 s ; seconde première lecture complète commune enregistrée à 181,780 s (murale), rejetée par le seuil strict 180 s. Lecture tardive suivante à 183,474 s ; instant physique exact inconnu. Stabilité commune tardive 15→18 en 145,204 s, trois signataires et horloges sains, sorties Docker 0 sans OOM. Preuves : `qualification/partition-cycles-20261001/attempt-h/`.

La seconde tête commune est d’abord lue à 81,246 s (slot 118), la justification nouvelle à 133,978 s (slot 128), puis la finalité au slot 136. Analyser la propagation et la participation pendant ce rattrapage avant nouvelle répétition identique. Le défaut sync de G n’est pas reproduit et sa cause demeure inconnue ; WAN A/B et tous les autres critères de production restent ouverts. Aucun seuil relâché ni gain causal revendiqué.

## Binaires du diagnostic sync — 1er octobre 2026

Les candidats Docker ARM64/amd64 et le Nano natif intègrent `b76cc1d`. Targets réussis avant marqueurs, empreintes et identités d’images contrôlées après assemblage ; P2P inchangés. Préparation : `qualification/sync-clock-images-20261001/`. Cause WAN G et effet réseau non qualifiés : les critères de reprise ≤180 s, stabilité et santé des signataires restent ouverts, ainsi que tous les autres critères historiques.

## Diagnostic de la signature sync — 1er octobre 2026

`2e16522` ajoute les valeurs exactes et la phase du refus `Sync duty clock regressed`, sans modifier ses prédicats. Trois nouveaux cas par architecture passent dans Docker ARM64 et amd64 : entrée, avant fournisseur et après fournisseur ; aucun message publié, engagements reactor/journal conservés. La suite locale termine exit 0 (40 blocs, 272 votes, finalité 3, deux stores et 64 journaux rouverts). Preuves : `qualification/sync-signing-clock-20261001/`.

Diagnostic et refus local qualifiés uniquement. Cause WAN G encore inconnue, images/natif à actualiser avant le prochain essai réseau. Aucun gain WAN ou clôture production revendiqué ; tous les critères et échecs antérieurs restent ouverts.

## WAN G : reprise, stabilité et signataire en échec — 1er octobre 2026

L’essai `9859768` termine exit 1. Première reprise acceptée en 138,965 s ; seconde reprise hors délai, finalité commune tardive enregistrée après 281,270 s. La stabilité tardive échoue également (retard supérieur à quatre époques). Le validateur Docker Mac rapporte `Sync duty clock regressed`, healthy=false, dernière attestation tracée au slot 70 ; son service d’horloge demeure clockHealthy=true. Les deux autres signataires sont sains. Conteneurs sortis 0 sans OOM : ceci ne valide pas la santé des signataires. Preuves : `qualification/partition-cycles-20261001/attempt-g/`.

Le quota des dépendances est respecté dans les traces complètes des trois nœuds (au plus huit ACK de blocs admis par catalogue). Aucun gain de délai causalement démontré. Priorité : chemin de signature sync et valeurs d’horloge manquantes, sans relâcher le refus de signature, le seuil 180 s ni la stabilité. Les causes historiques WAN A/B et tous les autres critères restent ouverts.

## Images du quota des dépendances — 1er octobre 2026

Les candidats Docker ARM64/amd64 et le P2P natif intègrent `9859768`. Les targets Nano et P2P ont été validés avant les marqueurs de provenance des volumes ; empreintes des exécutables et identités d’images contrôlées après assemblage. Le cœur Nano conserve ses empreintes ; le P2P est actualisé. Préparation : `qualification/dependency-quantum-images-20261001/`. Effet sur la reprise WAN non qualifié, budget strict 180 s et tous les critères restants inchangés.

## Quota des ancêtres du relais — 1er octobre 2026

Les traces archivées de WAN F montrent 30 blocs envoyés pour trois entrées comptées, durant 22,768 s : les ancêtres n’entraient pas dans le quota de page. `e866cb6` partage désormais le budget configuré entre cibles et dépendances réellement admises, cède localement sans prétendre avoir admis une cible restante, puis reprend grâce aux ACK conservés. Sept nouveaux scénarios ; 25 cas d’ancêtres et 41 cas de catalogue passent en Docker ARM64 et amd64, P2P compilés. Preuves : `qualification/dependency-quantum-20261001/`.

Transport simulé uniquement : effet WAN non qualifié et causalité du délai complet non démontrée. Les images empaquetées/natif doivent être actualisés ; les marqueurs anciens des volumes de build ne qualifient pas leur nouveau P2P. Aucun seuil, règle de consensus ou critère production relâché. Tous les écarts historiques restent ouverts.

## WAN F avec délai strict — 1er octobre 2026

Le harnais corrigé `b158628` et les images existantes `5165e51` terminent l’essai en échec (exit 1). Première coupure : reprise commune acceptée en 141,559 s. Seconde coupure : la première lecture complète de finalité commune est enregistrée 180,183 s après reconnexion et refusée par le contrôle strict ≤180 s ; instant physique exact dans l’intervalle de lecture inconnu. L’observation tardive conserve cet échec. Stabilité 15→18 en 144,742 s, trois validateurs sains, conteneurs sortis 0 sans OOM. Preuves : `qualification/partition-cycles-20261001/attempt-f/`.

Aucun rebuild ni relâchement de règle ; aucune nouvelle répétition identique avant analyse du rattrapage et de la participation au quorum. Les quotas du relais sont respectés dans les traces. Le délai de reprise, les causes WAN A/B et tous les autres critères de production restent ouverts.

## WAN E et correction de mesure du délai — 1er octobre 2026

L’essai `5165e51` sort 0 et le harnais brut marque les deux cycles réussis. Stabilité commune 15→18 en 143,649 s, trois validateurs/horloges sains. Cependant la seconde reprise est enregistrée 181,001 s après reconnexion : le contrôle antérieur au RPC peut accepter une lecture terminée hors délai. Le seuil strict ≤180 s n’est donc pas qualifié, sans inférer l’instant physique exact de la finalisation. Les quotas 8..15 admissions sont vérifiés dans les traces des trois nœuds. Preuves : `qualification/partition-cycles-20261001/attempt-e/`.

`443b19a` ancre le délai avant logging, utilise une horloge monotone et recontrôle après lecture. Trois nouveaux cas aux limites, 33 tests du harnais réussis : `qualification/recovery-deadline-20261001/`. Images C++ inchangées et réutilisables ; une observation réseau avec ce contrôle reste requise. Tous les autres critères et causes historiques demeurent ouverts.

## Images du quota d’admissions — 1er octobre 2026

Les candidats Docker ARM64/amd64 et le P2P natif intègrent `5165e51`. Les empreintes du cœur Nano sont inchangées, les P2P sont actualisés et les identités/binaires des images sont vérifiés. Préparation : `qualification/payload-quantum-images-20261001/`. Aucun résultat réseau encore acquis pour cette version ; seuil de reprise 180 s et tous les critères ouverts conservés.

## Quota réel des admissions du relais — 1er octobre 2026

`d9575aa` compte uniquement les nouveaux messages dont l’ACK d’admission est validé. Les entrées déjà connues ne consomment plus le quota ; la cession attend le quota configuré et une fin de page, avec dépassement borné à une page moins une entrée. Le nouveau cas de page partiellement connue échoue avant correction puis passe ; les 40 scénarios totaux passent dans Docker ARM64 et amd64, et les P2P compilent. Preuves : `qualification/relay-payload-quantum-20261001/`. Transport simulé seulement : images/natif à actualiser et effet sur le délai WAN à qualifier. Les échecs et causes historiques ainsi que tous les critères production restent ouverts.

## WAN D : cession de page insuffisante — 30 septembre 2026

L’essai du relais `d0cc038` échoue au premier cycle : reprise commune observée en 189,055 s après la coupure 96 s, seuil inchangé de 180 s. La seconde coupure n’est pas exécutée. Stabilité tardive 8→11 réussie en 139,944 s, reçus validateurs/horloges sains, sorties Docker 0 sans OOM. Les traces montrent des cessions après moins de huit exports car une page partiellement connue suffit à déclencher la cession. Cette inefficacité doit être corrigée ; sa responsabilité dans le délai n’est pas démontrée. Preuves complètes : `qualification/partition-cycles-20260930/attempt-d/`. Aucun gain réseau ni clôture des causes WAN A/B ou production revendiqué.

## Images du relais à pages bornées — 30 septembre 2026

Les deux images Docker et le P2P natif intègrent désormais `d0cc038`. Les empreintes du binaire Nano restent identiques aux candidats d’horloge ; seul le P2P change. Préparation et contrôles d’identité : `qualification/page-yield-images-20260930/`. L’effet réseau n’est pas encore qualifié ; les seuils et tous les critères ouverts restent inchangés.

## Équité des pages du relais Capella — 30 septembre 2026

`488c42c` fait céder le catalogue après une page portant des messages admis, afin de laisser le planificateur servir une autre catégorie. Les pages déjà connues restent parcourables ; la cession nécessite un cache configuré au moins égal au budget de catalogue. Les quatre nouveaux scénarios et les 39 scénarios totaux passent dans Docker ARM64 et amd64 ; les P2P compilent. Preuves : `qualification/catalog-page-yield-20260930/`. Transport simulé uniquement : l’effet sur la reprise WAN n’est pas qualifié, les images réseau et le P2P natif doivent être actualisés, et les échecs/cause d’horloge historiques restent ouverts.

## Partition WAN C : stabilité tardive, délai toujours en échec — 30 septembre 2026

Les nouveaux binaires de diagnostic `10e2f9e` ont terminé l’essai sur Mac natif, Docker ARM64 et Docker VPS amd64. Coupure 96 s : reprise commune en 138,730 s, réussie. Coupure 192 s : reprise observée en 185,801 s, au-delà du seuil inchangé de 180 s. La stabilité tardive 15→18 passe en 140,482 s ; les trois reçus validateurs/horloges sont sains et les conteneurs sortent 0 sans OOM. Aucun incident d’horloge reproduit : sa cause WAN B reste inconnue, ainsi que WAN A. Preuves : `qualification/partition-cycles-20260930/attempt-c/`. L’essai global reste en échec ; aucune correction de reprise ni qualification production revendiquée.

## Images du diagnostic d’horloge — 30 septembre 2026

Les binaires Nano ont été reconstruits dans les volumes Docker ARM64 et amd64 depuis `10e2f9e`; les deux images portent cette révision et leurs empreintes ont été vérifiées après assemblage. Le P2P conserve ses empreintes précédentes. Les quatre cas d’horloge passent également en natif Mac. Preuves : `qualification/clock-runtime-images-20260930/`. Il s’agit du diagnostic, pas d’une correction de la cause WAN B ni d’une qualification réseau. La reprise >180 s, l’anomalie WAN A et tous les autres critères de production restent ouverts.

## Diagnostic C++ d’horloge qualifié sur les deux architectures — 30 septembre 2026

La garde indique désormais temps lu, temps durable, dernière observation et
borne, avec trois prédicats séparés. Quatre nouveaux scénarios passent dans
Docker ARM64 et x86-64 : retours avant temps durable/observation coalescée,
saut excessif et limite exacte acceptée. Les défauts ferment les accès et
conservent le checkpoint et la première erreur. Aucun seuil relâché.
Cela qualifie le diagnostic ; cause physique WAN B, reprise >180 s et WAN A
restent ouverts. Les images réseau historiques sont encore à `ca780d5`.
[Preuves ciblées](qualification/capella-clock-diagnostics-20260930/manifest.json).

## Diagnostic WAN B : reprise lente et panne d’horloge — 30 septembre 2026

Premier cycle : reprise commune en 135,381 s. Second : head commun observé
à 105,612 s, justification plus récente à 142,005 s, finalité à 191,767 s :
le délai initial de 180 s manque à nouveau. La fenêtre tardive échoue ensuite
sur Beacon HTTP 503, Docker Mac : `Capella local clock discontinuity`.
Ce conteneur sort à 1 sans OOM et sans reçu terminal de validateur ; le VPS
sort à 0 sans OOM. La branche exacte et l’amplitude du défaut d’horloge ne sont
pas enregistrées. Elles doivent être distinguées avant correction, sans
contourner le fail-closed. Les deux échecs restent ouverts ; WAN A n’est pas
rétroactivement expliqué. Binaires, profils et budgets inchangés.
[Diagnostic et journaux complets](qualification/partition-cycles-20260930/attempt-b/manifest.json).

## Diagnostic de la reprise préparé — 30 septembre 2026

Les relevés de la tentative A ne permettent pas de séparer rattrapage du head
et attente FFG. Un mode optionnel collecte maintenant head/slot/état immuable,
checkpoints justifié/finalisé et durée de lecture, sans changer les budgets,
le verdict ou le cœur C++. Quatre nouveaux tests ciblés passent. Les échecs de
lecture restent explicites. La cause du dépassement et la qualification des
partitions répétées restent ouvertes.
[Préparation](qualification/fault-progress-20260930/manifest.json).

## Second cycle de partition : délai de reprise dépassé — 30 septembre 2026

La coupure de 96 s passe. Après celle de 192 s, les pairs se reconnectent mais
la finalité commune plus récente arrive après le budget initial de 180 s.
L’observation tardive retrouve l’époque 15 sur les trois nœuds ; elle ne
transforme pas le dépassement en réussite. La fenêtre tardive 15→18
passe en 138,883 s ; signataires sains, conteneurs arrêtés sans OOM. Le lanceur
termine en échec (exit 1). La cause du retard reste à analyser ; aucun
changement de runtime ou de budget n’a été appliqué pendant la campagne.
[Verdict terminal et journaux](qualification/partition-cycles-20260930/attempt-a/manifest.json).
Le critère de partitions répétées reste ouvert.

## Intégrité des archives contrôlée — 30 septembre 2026

Un contrôle indépendant compare cinq manifestes ancrés dans les objets Git
aux 78 fichiers qu’ils référencent : toutes les empreintes correspondent.
Huit tests ciblés passent, notamment altérations, liens symboliques et budgets
de lecture. Cela ferme ce contrôle d’intégrité des archives sélectionnées ;
aucune sémantique de test, provenance binaire, EIP ou production globale n’est
qualifiée par ce seul contrôle. La campagne de partitions continue sans
modification de ses dépendances.
[Preuves](qualification/evidence-integrity-20260930/manifest.json).

## Partitions répétées et prolongées : préparation historique — 30 septembre 2026

Un programme fini de deux coupures (96 puis 192 s) est ajouté au lanceur des
images finales. Chaque cycle exige une finalité commune plus récente et des
pairs reconnectés. L’historique des racines et des progressions reste commun
aux deux cycles ; un échec interdit la coupure suivante. Sept tests locaux
passent, dont conflit/recul entre cycles et reprise sans progrès à refuser.
Les ressources des trois nœuds sont échantillonnées ; une collecte incomplète
fait échouer la qualification. La campagne réelle est démarrée, pas encore
qualifiée. Les budgets du scénario ne limitent pas les validateurs du protocole.
[Preuves préparatoires](qualification/partition-cycles-20260930/manifest.json).
Aucun écart global, résultat réseau, revue indépendante ou production clôturé.

## Latence WAN asymétrique des images finales qualifiée — 30 septembre 2026

500 ms de retard sortant sont appliquées au seul conteneur VPS. Les trois nœuds
finalisent ensemble de 2 à 4 sous défaut en 168.754 s ; 2 102 paquets,
aucun drop dans le relevé NetEm. Le retrait est vérifié. Après deux sauts FFG
4→6 et 6→8, sans crédit et sans prolonger le délai, la fenêtre consécutive
8→9→10→11 passe avec rattachement commun à l’exécution en
250.540 s depuis le début de l’observation stricte. Les signataires finissent
sains (273/255/256 attestations) ; conteneurs sortis à zéro sans OOM.
Profil Capella/Shanghai, trois processus sur deux hôtes, 64 clés synthétiques.
Les anciens échecs de lecture restent archivés. Cette campagne ferme ce défaut
borné sur les images finales ; cycles prolongés/répétés, perte de paquets,
charge/ressources, nettoyage avec membre absent lors d’une nouvelle finalité,
origine WAN A, autres critères et revue indépendante restent ouverts.
[Preuves](qualification/packaged-asymmetric-20260930/manifest.json).

## Remplacement avec engagements indépendants qualifié — 30 septembre 2026

Le conteneur VPS est remplacé avec la même image candidate, le même volume et
peer ID, en 6,355 s. Engagement de chaîne et 21 journaux conservés séparément ;
la réconciliation commune reprend en 41,535 s. Trois avancées strictes de 3 à 6
passent en 146,812 s avec rattachement à l’exécution. Après reprise, 87
attestations VPS et nouvelles propositions admises ; ses 21 pins avancent.
L’ancienne configuration est ensuite refusée sur une copie arrêtée du nouvel
état, avant signatures ; journaux copiés et source originale inchangés octet
pour octet. Les conteneurs finissent à zéro sans OOM. L’image historique c2ec4d7
refuse le genesis Shanghai sur fixture isolée : elle n’est pas une cible de
retour pour ce profil. Retour entre versions différentes, autres pannes/charge,
écarts historiques et revue indépendante restent ouverts. Aucune qualification
de production globale ni d’un nouveau fork EVM.
[Preuves](qualification/pinned-replacement-20260930/manifest.json).

## Réseau des images finales qualifié — 30 septembre 2026

Les deux images candidates finales, sans volume de compilation, passent avec
Mac natif : trois avancées communes de finalité de 2 à 5, liées aux mêmes blocs
d’exécution, en 142,963 s. Le vrai moniteur effectue 24 observations saines et
refuse le faux genesis sur les trois endpoints. Les signataires terminent sains
avec 137/134/137 attestations admises ; conteneurs sortis à zéro sans OOM.
Identités/empreintes des images vérifiées avant génération des fixtures.
64 clés synthétiques, trois processus sur deux hôtes ; profil Capella/Shanghai.
Cette campagne valide le réseau stable des images finales. Mise à jour/retour,
pannes des images, charge/ressources, écarts historiques, alertes externes,
provenance complète et revue indépendante restent ouverts. Aucun fork plus
récent ni aucune production globale ne sont déduits de ce résultat.
[Preuves](qualification/packaged-network-20260930/manifest.json).

## Images candidates assemblées, qualification réseau ouverte — 30 septembre 2026

Les images finales ARM64 et x86-64 embarquent les deux binaires attendus issus
du volume existant, sans recompilation. Le précontrôle réel valide identité,
révision, architecture, empreintes et bibliothèques ; `productionQualified`
reste faux. Nano démarre sur `--help` en utilisateur 10001, sans réseau ni base.
Les sources runtime suivies sont identiques à la révision compilée ca780d5.
Le premier essai BuildKit échoué reste conservé ; l’assemblage corrigé utilise
la base locale résolue par son digest. Les manifestes de smoke utilisent des
fixtures publiques et un exemple node sans validateurs : aucune configuration
Capella de production n’est validée ici. Qualification réseau des images,
cycle mise à jour/retour, régression pertinente, provenance complète des
dépendances et revue indépendante restent ouverts.
[Preuves et limites](qualification/candidate-images-20260930/manifest.json).

## Livraison : précontrôle d’intégrité et procédure Docker — 30 septembre 2026

La sélection Compose accepte `NANO_IMAGE` explicite. Le précontrôle refuse tags
mutables, plateforme/source divergentes, fichiers/binaires altérés et dépendances
manquantes ; aucune base ni clé n’est montée. Onze tests passent et une image
locale de dépendances est effectivement refusée avant les probes. La
[procédure](DOCKER-UPDATES.md) protège les engagements et interdit le retour à
d’anciens journaux après signature. L’image candidate actuelle, son cycle de
promotion/retour, la régression complète et la revue indépendante restent ouverts.
Une vérification d’intégrité ne constitue pas une qualification de production.
[Preuves](qualification/candidate-integrity-20260930/manifest.json).

## Supervision du réseau vivant qualifiée — 30 septembre 2026

Le vrai CLI effectue 25 observations sur Mac natif, Docker ARM64 et Docker VPS,
avec état conservé entre processus. Le genesis incorrect est refusé sur les trois
nœuds. Avec le genesis/profil de la fixture indépendante, aucune alerte ; chaque
nœud avance de l’époque 2 à 5. La fenêtre stricte confirme trois avancées communes
avec rattachement aux blocs d’exécution (142,076 s). Signataires terminés sains,
conteneurs sortis à zéro sans OOM ; 64 clés synthétiques, deux hôtes physiques.
[Preuves](qualification/finality-monitor-live-20260930/manifest.json).
Qualification d’exploitation complète, livraison externe des alertes, disque,
ressources, duties, anomalies historiques et revue indépendante restent ouverts.
Aucune disponibilité sous panne ni production globale n’est déduite de ce run.

## Supervision privée multihôte : intégration prête, qualification en attente — 30 septembre 2026

Le lecteur ajoute Docker exec et SSH Docker exec, sans port RPC supplémentaire.
Vingt et un tests locaux passent (identités distinctes, arguments et quoting).
Le nouveau scénario fini utilise le vrai CLI : mauvaise identité genesis à
refuser sur les trois nœuds, puis configuration issue de la fixture indépendante,
historique persistant et observation parallèle aux trois époques strictes.
Aucune réussite réseau n’est encore revendiquée pour le moniteur.
[Implémentation et limites](FINALITY-MONITOR.md).

## Supervision de finalité implémentée, intégration réseau ouverte — 30 septembre 2026

Le [moniteur local](FINALITY-MONITOR.md) vérifie chaîne/genesis et profil,
checkpoint finalisé par état immuable, pairs, stagnation, conflits et régressions.
Les alertes de sécurité persistent entre exécutions ; seuils et budgets sont
configurables, RPC en loopback, aucune signature ni reprise automatique.
Dix-sept tests passent, dont cinq processus CLI face à deux serveurs synthétiques.
Qualification sur réseau Nano vivant, rattachement à l’exécution, ressources,
duties et livraison externe des alertes restent ouverts. Aucun critère global
de production n’est clôturé par ces tests de l’outil.
[Preuves](qualification/finality-monitor-20260930/manifest.json).

## Preuves de sanction falsifiées : nouveau scénario — 30 septembre 2026

Seize refus adversariaux passent en natif : signature altérée, mauvaise clé,
index inconnu et index dupliqué, chacun sur deux propriétaires durables avant
admission valide puis après leur réouverture. Checkpoint et archive restent
inchangés après chaque refus ; la preuve valide demeure conservée jusqu’à la
sanction finalisée. Le cycle complet dépôt/activation/sanction passe en natif et
Docker ARM64 et VPS x86-64 (sorties zéro, sans OOM). Le compteur `unknownFinalityAdvances` reste
zéro : la branche de nettoyage avec membre absent et nouvelle finalité demeure
ouverte, sans assimilation à ces contrôles de falsification.
[Preuves natives](qualification/capella-forged-slashing-20260930/native-result.json).

## Audit de sauvegarde : slot d’archive signé vérifié — 30 septembre 2026

Un nouveau cas de corruption sur copie arrêtée montre que l’audit acceptait un
slot d’archive RocksDB augmenté de un, sans modification du bloc signé. Le
correctif impose l’égalité avec le slot de l’en-tête vérifié. En natif, les cinq
cas sont refusés ; la base source et chaque copie restent inchangées. Docker
ARM64 et VPS x86-64 passent également les cinq cas, sans OOM ; les fixtures
canoniques comportent respectivement 99 et 100 blocs, avec 97 transactions. Cela ne prouve ni une
faille du consensus actif ni la correction des anomalies réseau historiques.
[Preuves](qualification/capella-audit-slot-20260930/native-fixed-result.json).

## Restauration complète avec finalité commune qualifiée — 30 septembre 2026

La tentative B passe : restauration du VPS, engagements de chaîne et de journaux
conservés séparément, reprise de 117 attestations et 24 propositions, puis trois
avancées communes de finalité (époques 5 à 8, fenêtre stricte de 185,140 s).
Les données sources restent inchangées. Les trois signataires terminent sains.
Le périmètre comprend 64 validateurs synthétiques, trois processus et deux hôtes
physiques. Une réconciliation bornée précède la fenêtre stricte inchangée ;
l'échec de la tentative A est conservé et sa cause n'est pas déclarée corrigée.
[Preuves B](qualification/capella-restored-validators-20260930/attempt-b/manifest.json).
La régression complète du candidat, les anomalies historiques et la revue de
sécurité indépendante restent des critères ouverts de production.

## Première restauration réelle : audit et signatures repris, finalité non qualifiée — 30 septembre 2026

Tentative A : copie complète et audit canonique (32 blocs, 1 transaction) passent.
VPS en ligne en 19,123 s, identité P2P conservée, 21 engagements de journaux fournis
séparément ; 9 nouvelles attestations et 3 propositions admises. La campagne
échoue ensuite : `Native head stalled for three slots`. Les trois horloges et
boucles de validation terminent saines, sans nouvelle finalité commune démontrée.
[Échec conservé](qualification/capella-restored-validators-20260930/attempt-a/manifest.json).
Une phase de réconciliation bornée à 180 s, avec pairs et slots observés, précède
désormais la fenêtre stricte de stabilité inchangée. Son résultat reste à vérifier ;
aucun assouplissement du verdict A ni de qualification de production.

## Objectif persistant relancé : restauration des signataires — 30 septembre 2026

Le lancement réel en production reste la cible, avec tous les critères et
échecs historiques conservés. Le banc multi-hôte ajoute une restauration complète
du VPS arrêté : données et identité P2P, audit canonique, engagements de chaîne
et de chaque journal conservés séparément, reprise de duties et trois avancées
de finalité commune exigées. Neuf contrôles de configuration passent ; aucune
réussite réseau n'est encore revendiquée pour ce nouveau scénario.
Les binaires qualifiés sont réutilisés, sans reconstruction d'image Docker.
Production globale, anomalie A et revue indépendante restent ouvertes.

## Reprise avec engagement externe et budget Docker qualifiés — 30 septembre 2026

Le cœur `172b245`, fixture `cfbf057` et configuration `e7a4c1f` passent les deux
gates runtime Docker : ARM64 8,88 / 10,33 s, VPS 32,88 / 35,54 s. Neuf contrôles
d'engagement externe, dont refus d'une paire base/témoin ancienne, passent sur
les deux architectures. Même binaire VPS : deux échecs à 1024 descripteurs,
puis succès à `nofile=65536`; les échecs et empreintes sont conservés.
Le budget est explicite et configurable dans Compose, smoke et multi-hôte ;
il ne plafonne pas le registre de validateurs. Conteneurs sortis à zéro sans OOM.
[Preuves et limites](qualification/capella-recovery-commitment-20260930/README.md).
La complétion d'une publication interrompue avec engagement est qualifiée en
natif seulement. Conservation indépendante des engagements, restauration des
signataires avec finalité commune, anomalie A et critères globaux restent ouverts.

## Engagement externe de reprise implémenté et qualifié en natif — 30 septembre 2026

`capella.recoveryCommitment` vérifie le checkpoint exact attendu dans le témoin
avant récupération, horloge et signataires. Une paire base/témoin ancienne mais
cohérente est refusée avec l'engagement récent ; aucun checkpoint primaire n'est
publié lors du refus. Reprise avec engagement correct et interruptions de
publication qualifiées en natif (23,82 / 5,21 s), régression runtime 19,37 s.
[Preuves et limites](qualification/capella-recovery-commitment-20260930/README.md).
Première tentative Linux : ARM64 vert, VPS échoué (`Too many open files`).
La fixture ferme désormais sa copie restaurée avant les signataires courants ;
VPS toujours en échec à 1024 descripteurs. Budget Docker explicite et configurable
à 65536 ajouté ; qualification sous ce budget en attente, deux échecs conservés.
L'engagement doit être conservé indépendamment et actualisé explicitement ;
les journaux des signataires ont leurs propres engagements. Linux, reprise des
signataires restaurés, finalité commune et autres critères restent ouverts.

## Sauvegarde/restauration hors ligne qualifiée — 30 septembre 2026

Sur ARM64 et VPS, le répertoire complet d'un nœud arrêté de C est archivé puis
restauré : tous les fichiers et SHA-256 concordent (chaîne, témoins, journaux,
identité P2P). Audit canonique restauré réussi, source et copie inchangées.
[Preuves et procédure](qualification/capella-backup-restore-20260930/README.md).
Cette preuve reste hors ligne et locale : reprise effective des signataires,
refus de sauvegardes anciennes/incomplètes, finalité commune après restauration,
stockage hors site et résistance de l'archive à une coupure restent ouverts.

## Audit canonique qualifié sur Linux ARM64 et VPS — 30 septembre 2026

Le cœur `6cace64` passe le scénario Capella avec rejeu canonique et le test
lecture seule sur ARM64 (26,93 / 0,03 s) et VPS x86-64 (104,18 / 0,04 s).
L'audit des bases arrêtées de C vérifie 99 blocs sur ARM64, 100 sur VPS et
97 transactions sur chacune. Leurs têtes terminales diffèrent car les arrêts
étaient successifs ; aucune nouvelle preuve de finalité commune n'en est déduite.
Les quatre corruptions sont refusées sur chaque architecture, sans mutation des
bases ni des copies. Conteneurs sortis à zéro sans OOM, empreintes archivées.
[Preuves](qualification/capella-offline-audit-20260930/linux/manifest.json).
Les limites restent celles de l'audit canonique : pas de rejeu de l'historique
complet des votes autonomes et du choix de chaîne. Sauvegarde/restauration,
anomalie A et autres critères globaux restent ouverts.

## Rejeu canonique Capella hors ligne implémenté — 30 septembre 2026

La commande `audit-capella` vérifie les archives depuis une ancre explicite, sans
modifier la base. Qualification native : scénario signé avec audit (33,76 s),
base WAN C auditée (99 blocs, 97 transactions), quatre corruptions refusées
(archive, signature, reçu, index), empreintes des fichiers inchangées.
[Preuves et limites](qualification/capella-offline-audit-20260930/README.md).
L'audit canonique est implémenté ; les votes historiques autonomes et le choix
de chaîne historique ne sont pas rejoués. Linux, sauvegarde/restauration et ces
compléments restent ouverts, ainsi que l'anomalie A et la production globale.

## Préalable à l'audit Beacon : stockage en lecture seule — 30 septembre 2026

Le mode `ChainStore::OpenMode::ReadOnly` est implémenté. Son nouveau test natif
passe (0,47 s) : récupération WAL après `_exit`, checkpoint lu, écritures refusées,
identité vérifiée, empreintes de tous les fichiers inchangées ; mode écriture
réouvert avec succès. [Preuves](qualification/storage-read-only-20260930/README.md).
Qualification Linux et raccordement du rejeu complet Beacon/Capella encore requis.
L'API de lecture seule ne constitue pas à elle seule l'audit ni une qualification de production.

## Audit hors ligne du profil Beacon : écart confirmé — 30 septembre 2026

L'examen en lecture seule des 44 journaux RocksDB de la tentative A ne trouve
aucune ligne correspondant aux motifs ERROR/FATAL, IO error, Corruption,
No space left on device ou Background Error ; les 44 enregistrent une fermeture
terminée. Cela ne prouve ni l'intégrité historique ni la cause de l'arrêt.
L'appel `nano audit` sur une copie de sa base sort à 1 :
`Beacon audit requires consensus archive replay, not legacy block replay`.
Le refus est intentionnel (`src/audit.cpp`, test `test_beacon_execution_gate.cpp`).
**Critère ouvert : fournir et qualifier un audit hors ligne Beacon/Capella**,
avec profil et ancre de confiance explicites, archives/pruning pris en compte,
rejeu des transitions de consensus et d'exécution, contrôle des index et reçus,
cas négatifs de corruption et preuve de non-mutation de la source. Ne pas assimiler
une réouverture de checkpoint à un audit complet depuis le genesis.
Preuves : [forensique A](qualification/capella-partition-load-20260930/attempt-a-forensics/manifest.json).
L'arrêt A reste non expliqué ; aucun résultat de production n'est déduit de cette recherche.

## Charge pendant partition WAN qualifiée — 30 septembre 2026

La tentative C sur `22b4b65` réussit : 96 transactions admises pendant une coupure
réelle de 97,882 s, finalité commune reprise en 136,834 s ; reçus finalisés et
états identiques vérifiés en 350,273 s, puis époques communes 9 à 12 en 124,230 s.
Les trois validateurs terminent sains, les deux conteneurs sortent à zéro sans OOM.
Les 33 mesures par nœud sont complètes. Preuves et vérification hors ligne :
[rapport](qualification/capella-partition-load-20260930/README.md).
Cette charge est bornée (SSTORE constant), sur trois instances et deux machines ;
aucune capacité maximale ni endurance prolongée n'est démontrée.
L'arrêt du réacteur de la tentative A reste inexpliqué ; la production globale reste ouverte.

## Charge sous partition : transactions bloquées, correctif natif — 30 septembre 2026

Le [rejeu B](qualification/capella-partition-load-20260930/README.md) rétablit la
finalité commune en 129,867 s après une coupure de 97,607 s, mais les 96 transactions
restent en mempool : échec applicatif à 617,977 s. Le budget de 16 candidats était
appliqué avant la résolution des nonces, pouvant exclure leur préfixe indéfiniment.
La sélection corrigée respecte les dépendances de nonce sans augmenter ce budget.
Tests natifs réussis : mempool (0,36 s), 32 transactions reçues en ordre inverse et
finalisées par Capella (34,44 s). Qualification Linux réussie sur ARM64 (25,58 s) et VPS x86-64 (97,36 s),
avec le cœur `22b4b65`, journaux et empreintes archivés dans
`qualification/capella-partition-load-20260930/nonce-ready-linux/`.
Le rejeu WAN C est réussi dans le périmètre détaillé ci-dessus.
L'arrêt du réacteur en tentative A reste distinct et non expliqué ; aucune clôture
globale de production n'est revendiquée.

## Charge pendant partition WAN — échec à corriger, 30 septembre 2026

La [campagne combinée](qualification/capella-partition-load-20260930/README.md)
a échoué après 64 soumissions : Docker Mac s'est fermé avec « Capella reactor
unavailable; reopen required », sortie 1 sans OOM. La coupure a été restaurée
après 49,570 s, avant les 96 s prévues. La cause initiale n'était pas conservée ;
le diagnostic est renforcé sans assouplir les règles. Aucun succès de charge sous
partition n'est revendiqué. Les preuves et bases initiales restent conservées.
Le diagnostic passe les tests ciblés sur Mac, Linux ARM64 et VPS x86-64 ;
le rejeu sous partition avec `406227f` a échoué par blocage d’inclusion (voir ci-dessus).

## Charge applicative WAN bornée — qualifiée le 30 septembre 2026

Le [nouveau scénario multihôte](qualification/capella-bounded-load-20260930/README.md)
passe : 96 transactions type 2 finalisées sur les trois nœuds, reçus identiques,
nonce/solde/stockage vérifiés, puis finalité commune des époques 8 à 11. Le lot
est vérifié en 290,781 s et la stabilité post-charge en 131,089 s. Les 31 mesures
par nœud ne présentent aucun manque ; les conteneurs sortent à 0 sans OOM.
C'est une charge finie de transferts et SSTORE constant, sans partition injectée,
ni croissance du stockage, ni mesure de capacité maximale. Le cœur est inchangé ;
aucun fork supplémentaire ni critère global de production n'est déclaré clôturé.

## Nouveau validateur : preuve conservée avant enregistrement finalisé — 30 septembre 2026

Le [nouveau parcours dépôt/sanction](qualification/capella-new-validator-slashing-20260930/README.md)
passe sur Mac (162,25 s), ARM64 (151,55 s) et VPS (532,84 s) : dépôt consommé
au slot 80, preuve admise au slot 88 quand le
registre justifié connaît le validateur mais pas le registre finalisé, puis
réouverture des deux bases avec preuve intacte. Activation à l’époque 17,
sanction et retrait après finalisation ; époque finale 18. Régression native du
dépôt/activation passée (160,07 s). Aucun avancement de finalité pendant l’absence du membre n’est observé
ici : cette branche précise du nettoyage reste non couverte. Production globale ouverte.

## Sanctions multi-validateurs et pannes d’écriture — 30 septembre 2026

La [finalisation partielle des preuves](qualification/capella-slashing-multi-20260929/README.md)
est qualifiée sur Mac, Docker ARM64 et VPS x86-64, avec la régression mono-auteur.
Le [nouveau test de panne WAL](qualification/capella-slashing-sync-20260930/README.md)
passe sur Mac (137,75 s), ARM64 (119,86 s) et VPS (447,12 s) : huit cas par
plateforme, retrait individuel puis agrégé, WAL principal/témoin, suffixe
conservé/perdu, reprise exacte et rejeu idempotent. Les 24 cas sont vérifiés et
archivés ; époque finalisée 8 sur les trois plateformes. Aucun code du cœur
n’a changé. Les validateurs absents du
registre finalisé et les autres critères de production restent ouverts.

## Sanctions après réorganisation qualifiées — 29 septembre 2026

Le [nouveau scénario](qualification/capella-slashing-reorg-20260929/README.md)
passe sur Mac (5,14 s), Docker ARM64 (3,99 s) et VPS x86-64 (16,87 s) : deux
changements de branche signés, preuve conservée puis réincluse automatiquement,
sept réouvertures avec état exact et absence de double pénalité. La régression
du scénario partagé passe sur les trois plateformes. L’échec initial du test
(horloge non avancée) est conservé ; aucun code du cœur n’a changé.
Le critère borné mono-auteur est fermé. Multi-auteurs, finalisation partielle et
échecs d’écriture restent ouverts, ainsi que l’objectif global de production.

## Rotation WAN qualifiée avec audit corrigé — 29 septembre 2026

La [campagne de rotation](qualification/capella-wan-rotation-20260929/README.md)
traverse le slot 128 sous partition : coupure 97,460 s, reprise commune en
131,132 s puis finalité **18 → 19 → 20 → 21**. Les trois bases rouvertes donnent
le même état et le nouveau comité ; **19 blocs finalisés** portent des agrégats
BLS valides avec contributions distantes. L'échec du premier auditeur, qui
refusait les agrégats vides canoniques, reste conservé ; son correctif est validé
sur les mêmes données Mac/ARM64/VPS, sans modifier le cœur ni rejouer le réseau.
Ce scénario borné ferme l'écart de rotation WAN correspondant. Ressources
échantillonnées et limites sont documentées ; sanctions/cycle complet, charge,
exploitation, EVM/RPC, bridge et revue indépendante restent ouverts.

## Rotation WAN : nouveau banc préparé — 29 septembre 2026

Le [collecteur et l’auditeur de rotation](qualification/capella-wan-rotation-harness-20260929/README.md)
sont raccordés au scénario Mac/Docker/VPS : partition traversant le slot 128,
capture SSZ avant élagage et vérification des agrégats après reprise sur les
bases rouvertes. Compilation native, 13 tests nouveaux du collecteur et contrôle
négatif d’une ancienne base passés. **Campagne WAN et chemin positif de l’audit
encore à exécuter** ; aucun statut EIP ni critère de production fermé par ce seul
outillage. Le cœur reste inchangé.

## WAN asymétrique qualifié avec lecture immuable — 29 septembre 2026

La [campagne sur `940736f`](qualification/capella-wan-pinned-20260929/README.md)
passe sur Mac natif, Docker ARM64 et Docker VPS x86-64 : 500 ms sortants,
finalité 2 → 4 en 146,980 s sous défaut, puis rattrapages 4 → 6 → 8 et fenêtre
**8 → 9 → 10 → 11**. Observation après restauration 271,743 s sous la borne
inchangée de 288 s ; 114 lectures par état, trois validateurs sains à l'arrêt.
Le correctif Beacon passe aussi en tests natifs sur les trois plateformes.
Les deux échecs précédents restent conservés. Ce scénario borné est qualifié ;
rotation WAN, sanctions/cycle complet à consolider, charge, exploitation,
écarts EVM/RPC, bridge et revue indépendante restent dans l'objectif global.

## Lecture Capella par état implémentée — 29 septembre 2026

Le [correctif de lecture immuable](qualification/capella-pinned-beacon-20260929/README.md)
sert root/fork/finalité par state root retenu et vérifie l'engagement SSZ. Le
moniteur associe le header à cet état, sans exiger un head immobile. Racines
invalides, inconnues/élaguées, conflits et régressions restent des erreurs.
Test natif passé (5,42 s), 24 tests du moniteur passés ; compilation native
réussie. La qualification Linux/WAN du nouveau binaire reste à faire. Aucun
quorum ni délai assoupli ; les échecs précédents et l'objectif global restent ouverts.

## Lecture Beacon cohérente encore manquante — 29 septembre 2026

La [nouvelle campagne WAN](qualification/capella-wan-asymmetric-settle-20260929/README.md)
atteint 2 → 4 sous retard en 159,741 s, puis échoue : le head VPS alterne entre
deux branches pendant les trois lectures, malgré un checkpoint finalisé identique.
Les validateurs restent sains, mais la stabilité n'est pas qualifiée. L'API ne
fournit actuellement cette finalité que sous `states/head` ; prochaine correction :
lecture par state root identifié, tests de concurrence/élagage/engagement, puis
nouvelle qualification. Aucun retry supplémentaire ou seuil assoupli. Les deux
runs restent en échec dans leurs preuves ; le cœur n'a pas été modifié.

## Saut FFG expliqué, moniteur de récupération corrigé — 29 septembre 2026

Le [rejeu du checkpoint réel](qualification/capella-wan-ffg-analysis-20260929/README.md)
authentifie la base par le digest du reçu terminal et reproduit exactement
4 → 6 au passage du slot 71 à 72. Les poids de cibles 54/64 et 47/64 satisfont
les conditions FFG ; le saut n'est pas en lui-même une erreur de consensus.
Le mode de récupération du moniteur recommence sa fenêtre après un saut sans
lui donner de crédit, sans effacer l'historique de sécurité et sans prolonger
le délai absolu. Les trois avancées consécutives liées à l'exécution restent
exigées. 19 tests du moniteur passent, dont six nouveaux. Le cœur est inchangé ;
l'échec historique reste conservé et une campagne distincte qualifie le correctif.

## WAN asymétrique : défaut testé, stabilité encore ouverte — 29 septembre 2026

La [nouvelle campagne réelle Mac/Docker/VPS](qualification/capella-wan-asymmetric-20260929/README.md)
applique 500 ms de retard sortant au seul conteneur VPS. Les trois nœuds
progressent de la finalité 2 à 4 en 159,156 s sous défaut (2 092 paquets mesurés).
Après retrait, le moniteur échoue sur le saut 4 → 6 avant ses trois avancées
consécutives : **campagne globale en échec**, malgré les trois validateurs sains.
Les traces internes confirment les observations 4 aux slots 56–71 puis 6 aux
slots 72–73 ; cause à reconstruire depuis justification/participation, aucun
seuil relâché. 15 nouveaux cas de sécurité du banc, 41 cas au total, passent.
Rotation WAN, charge, exploitation, bridge et production restent ouverts ;
les preuves de rotation locale existantes sont référencées dans l'audit candidat.

## Nouvel objectif : candidat production — 29 septembre 2026

L'[audit de départ](qualification/production-candidate-20260929/README.md)
conserve les acquis et cible d'abord la finalité WAN sous partition. Le cas
local à 500 ms est déjà passé ; l'ancien WAN à 189,2 s reste un échec face au
seuil de 180 s. Le préparateur de topologie prend désormais en charge Capella
avec trois instances et 22/21/21 clés synthétiques. Configuration et 26 tests du banc vérifiés. La [campagne Capella WAN](qualification/capella-wan-partition-20260929/README.md)
passe : coupure observée 97,239 s, reprise commune en 136,691 s (seuil 180 s),
puis trois avancées 7 → 8 → 9 → 10 avec accord des blocs d’exécution. Trois
groupes de validateurs sains à l’arrêt. Les anciens échecs sont conservés ;
asymétrie WAN, charge et qualification globale de production restent ouvertes.

## Jalon sorties : qualification technique acquise — 29 septembre 2026

L'[audit des six critères](qualification/capella-exit-audit-20260929/README.md)
relie chaque exigence à ses sources, tests et limites : admission authentifiée,
livraison autonome après redémarrage, inclusion/réinclusion, finalité et paiement
sur Mac/VPS, cohérence après WAL/SIGKILL, retrait des archives et élagage.
La dernière sélection passe (3/3, 16,45 s), le réseau local après reprise passe
(1/1, 427,44 s), et les seize cas WAL passent aussi sur Linux (3/3 tests,
476,02 s). Les échecs initiaux et les versions exactes des bancs sont conservés.

**Cela termine la qualification technique de ce jalon, pas la production de
Nanobees.** Les étapes globales EVM/RPC, consensus sous partitions/charge,
exploitation, bridge et lancement économique restent ouvertes. La publication
Git du livrable est contrôlée séparément avant clôture de l'objectif actif.
Les sections suivantes sont le journal historique des étapes et de leurs
limites à la date du run ; l'audit ci-dessus fixe l'état consolidé du jalon.

## Audit des sorties en cours — 29 septembre 2026

Les [huit rejets explicites de contexte BLS](qualification/capella-exit-authentication-20260929/README.md)
passent sur Mac (deux tests, 10,92 s), sans mutation du checkpoint ni de la file.
La qualification WAN existante démontre la propagation de la sortie dans les
blocs ; ses journaux ne démontrent pas une livraison autonome `voluntary_exit`.
Cette livraison autonome est prouvée dans le scénario local RPC/libp2p. Son
[extension hors connexion](qualification/capella-exit-offline-relay-20260929/README.md)
passe en 427,44 s : redémarrage sans nouvelle soumission, livraison autonome,
retrait finalisé et contrôle des deux bases rouvertes. Ne pas confondre
ces deux portées ni assimiler cette extension à une partition WAN.

## Sortie entre Mac et VPS qualifiée — 29 septembre 2026

Le [scénario Mac/VPS](qualification/capella-exit-multihost-20260929/README.md)
passe avec 32 validateurs par hôte et le vrai cpp-libp2p dans un tunnel SSH
privé : soumission sur le VPS, propagation, inclusion, SIGKILL avant finalité,
reconnexion et retrait finalisé identique au bloc 55 (époque 8). Un seul paiement
du capital ; comptabilité exacte comparée sur les deux RPC. La réouverture du
Mac confirme l’archive retirée, la cohérence EVM/consensus et l’absence de
proposition/attestation après l’époque de sortie. Les deux échecs du banc sont
conservés. La réouverture hors ligne du VPS passe sur `24971b8`, avec les
mêmes montants, finalité et archive que le Mac ; le nouveau test d’élagage
passe également sur Linux (1/1, 89,17 s).

Le [nouveau cas d’élagage d’une sortie jamais incluse](qualification/capella-exit-pending-pruning-20260929/README.md)
passe (32,56 s), avec réancrage, réouverture, doublon idempotent et inclusion
automatique. Les régressions élagage BLS/retrait d’archive passent également
(2/2, 99,92 s). La [campagne aux transitions de sortie](qualification/capella-exit-lifecycle-sync-20260929/README.md)
passe maintenant sur Mac : 16 cas WAL aux publications d’inclusion,
retrait d’archive, paiement et finalisation du paiement (130,29 s), puis
6/6 régressions ciblées (121,81 s). La campagne Linux/VPS passe également sur `2d7c6da` : 16/16 cas
(447,33 s), avec 3/3 tests au total (476,02 s). L’audit final de couverture
du jalon reste ouvert.
**Le jalon complet et la production ne sont pas déclarés terminés.**


## Qualification réseau des sorties — locale (29 septembre 2026)

Le scénario RPC/libp2p avec deux vrais nœuds locaux passe : retrait complet,
finalité commune au bloc 56, comptabilité des paiements après réouverture et
contrôle des dernières propositions/attestations du validateur sorti. Un seul
processus possède les clés des validateurs ; cela ne qualifie pas un consensus
entre hôtes indépendants. Les [quatre cas WAL de l’admission](qualification/capella-exit-wal-20260929/README.md) passent aussi sur Mac, avec la régression BLS du banc partagé (2/2 tests).
La [campagne WAL Linux/VPS](qualification/vps-exit-wal-20260929/README.md)
passe sur `ebe1d16` (2/2, 26,48 s). Le [contrôle direct de l'archive](qualification/capella-exit-retirement-20260929/README.md)
passe sur Mac : conservation avant finalité, retrait après finalité, puis
élagage et réouvertures (64,81 s). Le [cas après réindexation](qualification/capella-exit-reindex-20260929/README.md) passe également (174,47 s) : ancien index refusé, bon index admis et inclus, autre validateur inchangé. La preuve Mac/VPS acquise ensuite est décrite ci-dessus.
Voir les [preuves, échecs et limites](qualification/capella-exit-network-20260929/README.md).
La production et le jalon complet restent non démontrés.


## Objectif prioritaire fixé le 29 septembre 2026

**Livrer et démontrer le parcours complet de sortie d'un validateur sur le
réseau Nano : soumission signée → propagation → inclusion → finalité → retrait.**
Ce jalon fait partie de l'objectif production existant ; il ne remplace aucun
critère EVM, consensus, exploitation, bridge ou lancement économique.

État de départ vérifié : `c1606fd`. La sélection locale et la file durable des
sorties sont implémentées et qualifiées sur des scénarios locaux. Le parcours
RPC/P2P, la finalisation et le retrait complet ne sont pas encore démontrés.

### Critères de clôture du jalon

1. Soumission via RPC/service réseau avec authentification de chaîne, clé/index,
   domaine BLS et éligibilité ; rejets invalides sans mutation et ressources bornées.
2. Diffusion par le vrai P2P vers d'autres nœuds, inclusion automatique,
   idempotence et reprise des envois après déconnexion.
3. Conservation de la demande avant finalité, réinclusion si son bloc est
   réorganisé, absence d'application au mauvais validateur après réindexation.
4. Finalité commune de la sortie ; retrait exact vers les credentials autorisés,
   conservation comptable, aucun double paiement ni signature après inéligibilité.
5. Reprise cohérente après arrêt brutal ou erreur d'écriture, avec vérification
   du registre, de la file et de l'état EVM ; élagage sans perte d'opération utile.
6. Preuve de bout en bout Mac/VPS avec sources et binaires identifiés, puis
   régression pertinente sur cette même version. Conserver les échecs initiaux,
   résultats, limites et commandes de reproduction dans la matrice EIP.

Les sous-étapes ne valent pas clôture du jalon. Aucun délai ou pourcentage
arbitraire annoncé, aucun fonds réel ni déploiement public autorisé par ce jalon.
La qualification globale de production reste nécessaire après sa réussite.


## Mise à jour de l'objectif — 27 septembre 2026

**Statut : actif, production non démontrée.** Le périmètre complet de l'objectif
est conservé. La [matrice EIP](EIP-IMPLEMENTATION-STATUS.md), la
[feuille de route validateurs](VALIDATOR-EIP-ROADMAP.md) et celle de
[l'exécution](EVM-PRODUCTION-EIP-ROADMAP.md) restent les références de suivi.
Une preuve ciblée ferme seulement le scénario qu'elle couvre.

### Acquis récents vérifiables

- Résolution automatique des validateurs par clé publique ; contrôle de la
  paire clé/index avant signature et refus des index périmés après reorg.
- Deux branches à dépôts inversés : réindexation 64→65, activation à l'époque
  13, neuf attestations et finalité commune époque 20 conservée après réouverture.
  [Preuves](qualification/capella-reindexed-activation-20260927/README.md).
- Panne WAL au vote de bascule : état ancien ou nouveau cohérent, fermeture des
  accès incertains et rejeu qualifiés sur macOS, Linux ARM64 et désormais sur le
  VPS Ubuntu x86-64 (99,87 s). [Preuve VPS](qualification/vps-deposit-reorg-sync-20260927/README.md).
- Recherche de clés dans des états SSZ de 100 000 et un million de validateurs,
  avec cache lié à la racine authentifiée. Ces mesures ne représentent pas des
  réseaux de cette taille. [Preuves et coût restant](qualification/capella-registration-ssz-scale-20260927/README.md).

### Travail restant, dans l'ordre de priorité

1. Achever le cycle des validateurs intégré : entrées, activation, sorties,
   retraits, sanctions, réorganisations et reprise. Étendre les preuves aux
   points d'interruption non couverts et aux témoins indépendants ; contrôler
   les signatures et la finalité après reprise, pas seulement le redémarrage.
2. Qualifier la finalité durable via le vrai transport entre Mac et VPS :
   partitions asymétriques, rotation des comités, reconnexion et charge bornée.
   Mesurer mémoire, disque, latence et coûts de registre ; traiter les limites
   implicites sans les masquer par un plafond arbitrairement augmenté.
3. Fermer les écarts du profil EVM/réseau retenu et de ses dépendances. Séparer
   forks activés, runners de conformité et propositions expérimentales ; conserver
   les preuves Ethereum/Anvil/Geth, RPC, OpenZeppelin et Uniswap. Les succès EEST
   ciblés ne prouvent pas l'activation réseau complète de Cancun/Prague.
4. Exécuter une qualification de version candidate cohérente, avec empreintes
   des mêmes sources/binaires, régression pertinente, pannes et ressources,
   sauvegarde/restauration et procédures d'exploitation. Conserver tous les
   échecs, exclusions et limites ; aucune somme de petits runs ne vaut une
   campagne complète non exécutée. La revue indépendante reste un point ouvert.
5. Finaliser le lancement Base–Nano : utilité et relation du token Base avec
   l'actif natif, couverture USDC, confiance et finalité du bridge, retraits,
   allocation et migration de liquidité/fair launch. Qualifier le parcours
   complet avant une ouverture de vente. Aucun référencement tiers garanti.

Les travaux de bridge et de lancement restent dans l'objectif, sans fonds réels
ni déploiement public implicites. Le budget de préparation de 200 euros n'est
pas une autorisation de dépense. Le code, les preuves et l'historique doivent
être préservés ; push sur main après revue et qualification autorisé.


Décision utilisateur du 21 septembre 2026 : le portage doit corriger ce qui
n'était pas adapté à la production dans le Python. La référence Python reste
intacte. La parité avec ses défauts n'est plus un critère de réussite.
Le prototype ne doit plus imposer de dépendance d’exécution ni de pilote de
tests Python à Nano ; les tests du projet sont natifs C++.

## Décision prioritaire du 24 septembre 2026 — validateurs

La cible demandée devient une architecture inspirée du consensus Ethereum :
registre ouvert sous conditions de staking, comités tournants par slots/epochs,
signatures BLS agrégées avec signataires identifiables, règles de finalité et
choix de branche explicites. Cette décision remplace les propositions antérieures
qui conservaient exclusivement le BFT à votes individuels comme cible finale.
Elle ne vaut pas validation ni implémentation de cette architecture.

Avant activation : spécifier et versionner les domaines cryptographiques,
l'aléa et la sélection des comités, le calcul des poids, les transitions du
registre, les sorties et sanctions, le choix de branche, la finalité et les
conditions de synchronisation sûre. Tester les partitions, équivoques,
agrégats forgés, doubles comptages, reprises et changements d'epoch.
Réutiliser les spécifications et primitives éprouvées ; ajouter des groupes
au BFT existant ne suffit pas à reproduire la sécurité d'Ethereum.

Les tests de votes individuels à 100 000 et un million de validateurs mesurent
l'implémentation actuelle. Ils ne qualifient ni le futur protocole ni un réseau
distribué de cette taille. Le lancement public reste conditionné aux preuves
techniques et aux paramètres opérationnels de déploiement.

## Cible retenue

Un réseau public Nano en C++, avec EVM native, cpp-libp2p, RocksDB et staking
par transactions signées vers un smart contract. Les nouvelles règles corrigées
priment sur la reproduction exacte des historiques Python divergents. Toute
rupture de format ou de règles doit être versionnée et documentée ; les données
existantes ne doivent jamais être réécrites silencieusement.

## Critères de clôture

1. Consensus et blocs authentifiés : signature du producteur, séparation des
   domaines de signature par chaîne et version, contrôle de sélection, rejet
   du rejeu et des altérations. Définir et vérifier les règles de concurrence
   de branches, de finalité et de sanctions applicables au réseau public.
2. Exécution déterministe : règles EVM et gaz définies, rejeu identique sur les
   pairs, invariants de soldes et de dépôts, annulation atomique des échecs.
3. Staking : inscriptions, ajouts et retraits signés, règles de verrouillage et
   de sortie explicites, protections adversariales et revue de sécurité.
4. Persistance : atomicité, récupération après arrêt brutal, détection des
   corruptions, sauvegarde/restauration et évolution de schéma vérifiées.
5. Réseau et RPC : contrôle des entrées, limites de ressources, résistance aux
   pairs malveillants, synchronisation vérifiée, séparation des clés et des
   interfaces publiques, comportement opérationnel documenté.
6. Livraison : compilation reproductible, dépendances et licences recensées,
   tests fonctionnels et adversariaux, essais multi-nœuds prolongés, procédures
   d'exploitation et limitations connues. Toute validation externe nécessaire
   mais non obtenue reste explicitement un point ouvert.

7. Entrée et sortie des actifs : bridge bidirectionnel qualifié vers au moins
   une chaîne choisie, avec identification des chaînes et contrats, protection
   contre le rejeu, règles de finalité et modèle de confiance explicites.
   Vérifier la conservation de la couverture des actifs représentés, les retraits,
   les doublons, les pannes de relais, les réorganisations admises par le modèle
   et la procédure de reprise. Le bridge et la pool de liquidité sont distincts :
   traverser le bridge ne fournit pas automatiquement de liquidité à une pool.
   La cible retenue est Base–Nano avec USDC ; les contrats sources exacts, le mécanisme de vérification et son modèle de confiance restent à qualifier.
8. Pool de lancement : permettre l’échange de NANO (ou de sa représentation
   bridgée) contre un actif de contrepartie choisi. Définir le réseau hôte et
   le DEX, puis vérifier l’ajout/retrait de liquidité, les swaps, les décimales,
   les autorisations et les limites de slippage. Vérifier le parcours complet
   bridge vers pool puis retour, sans émission non couverte ni double retrait.
   La liquidité initiale et le prix initial nécessitent des montants décidés
   explicitement ; aucun apport de fonds réel n’est autorisé par le développement.

## Positionnement de lancement

Décision utilisateur : bridge bidirectionnel et pool de lancement. Le produit
ne cible pas un coffre-fort ou une fonction de gestion de trésorerie. Aucun
contrat de coffre supplémentaire n’est requis par ce positionnement.
La cible de bridge retenue est Base–Nano avec USDC. Le réseau hôte et la configuration définitive de la pool restent à arrêter.
Le bridge facilite le passage entre réseaux ; seuls les dépôts de liquidité
alimentent la pool. La présence du bridge ne garantit ni demande ni cotation
sur un service tiers.

Des suites vertes ne suffisent pas à déclarer le réseau prêt pour la production.
Aucun déploiement public ni engagement financier n'est autorisé implicitement
par cet objectif de développement.

## Priorité historique — remplacée par la mise à jour du 27 septembre

Terminer la qualification d’exécution EVM à partir des fixtures Ethereum
épinglées et de transactions signées identiques comparées à des clients de
référence. Conserver les divergences officielles même lorsqu’une variante
diagnostique locale en explique la cause. Poursuivre ensuite la qualification
BFT, reprise, stockage, RPC et réseau ; vérifier les parcours Solidity/pool avant
le développement du bridge. Voir `EVM-CONFORMANCE.md` pour le bilan récent.

L’audit externe payant et l’industrialisation sont reportés à la demande de
l’utilisateur ; leur report ne constitue pas une preuve de préparation
production. Aucun déploiement public ni usage de fonds réels n’est autorisé.

### Étape antérieure : authentification des producteurs

Authentifier les blocs PoS et vérifier le rejet des blocs falsifiés sur le RPC,
l'import, le stockage et le réseau. Le format C++ versionné et les producteurs signés sont maintenant intégrés
(voir `BLOCK-SIGNATURES.md`). Les genesis livrés activent cette règle ; leurs versions historiques sont
conservées séparément (voir `GENESIS-VERSIONS.md`). La protection locale contre la double signature est
implémentée avec les limites décrites ci-dessous.

## Audit de stockage

La commande `nano audit` rejoue la chaîne canonique depuis le genesis dans une
base temporaire et compare états, registres de validateurs, reçus et index.
Les tests couvrent une preuve historique corrompue et un reçu altéré. Voir
`STORAGE-AUDIT.md` pour le périmètre exact ; cela ne remplace ni la finalité,
ni les essais de perte d’alimentation, ni une revue indépendante.
Le test du validateur distant couvre désormais un SIGKILL après réservation
et signature, avant réception du bloc : la reprise conserve le même bloc et
la même preuve. Les pannes disque et pertes d’alimentation restent à éprouver.

### Qualification OpenZeppelin demandée

Couvrir tout le catalogue OpenZeppelin, y compris ERC721/ERC1155 et leurs
extensions, contrôle d’accès, cryptographie, gouvernance, proxies et compositions
de marché. La première version figée est Contracts v5.0.2. Le suivi par composant
est décrit dans `OPENZEPPELIN-QUALIFICATION.md` ; compilation et quelques scénarios
réussis ne constituent pas une validation complète. Le paquet séparé
Contracts Upgradeable doit également être inventorié avant d’en revendiquer la
couverture. Les contrats propres aux marketplaces, au bridge et à l’AMM demandent
des tests de composition et de sécurité supplémentaires.

### Point ouvert RPC / gouvernance ajouté le 2026-09-24

Le nouveau parcours OpenZeppelin ERC20Votes/TimelockController révèle des transactions échouant avec le gaz estimé sur le contexte `latest`, puis réussissant avec une estimation explicite `pending`. Les coûts varient avec les checkpoints de numéro de bloc et l’état temporel du timelock. La campagne `oz-governance-primitives-20260924` valide les primitives sur Paris/Shanghai avec pending, conserve tous les échecs et ne modifie pas le moteur. Il reste à comparer ce comportement aux clients de référence, définir puis qualifier la politique d’estimation par défaut pour les wallets, sans altérer la sémantique d’un bloc explicitement demandé. Le cycle complet de Governor reste distinct et non qualifié par ce test.

### Correction du défaut d’estimation omise — 2026-09-24

Le point précédent est résolu pour le défaut de sélection du contexte : comme Anvil 1.5.1/1.8.3, Nano sélectionne désormais `pending` quand `eth_estimateGas` ne reçoit aucun bloc. Les profils de gouvernance passent avec ethers sans override de gaz, avec 991 vérifications RPC et dix suites complémentaires réussies. Les blocs explicitement demandés et le défaut `eth_call` sont préservés. Les devis sensibles à une évolution ultérieure de l’état, le cycle complet de Governor et la qualification globale de production restent distincts. Preuves et limites : `rpcDefaultPendingQualification` dans le manifeste Ethereum et les deux indices associés.

### Cycle Governor : périmètre local démontré le 2026-09-24

Le cycle principal GovernorVotes/CountingSimple/TimelockControl est désormais testé de bout en bout sur Paris et Shanghai, avec votes historiques après transfert, proposition sans quorum et refus des doubles votes/exécutions. Le manifeste `ozGovernorLifecycleQualification` conserve les preuves. Restent notamment les autres extensions, votes Governor signés, annulation/concurrence, mises à niveau et configuration réelle du réseau ; ce résultat ne clôt pas la préparation production globale.

### Finalisation après correction RPC : charge locale vérifiée (2026-09-24)

`bftDefaultEstimateLoadQualification` ajoute deux réseaux locaux successifs Paris/Shanghai avec quatre validateurs, 48 clients et douze tours bornés. La finalisation reste active, les devis sans bloc demandé sont effectivement exercés, les refus de capacité restent explicites, les RPC récupèrent et huit bases rouvertes passent leur audit. Cela vérifie le nouveau défaut pending sous cette charge ; les limites multi-hôtes, d’endurance et des précédents scénarios stricts restent ouvertes.

### Reprise BFT avec les binaires actuels (2026-09-24)

Le manifeste `bftRecoveryRefreshQualification` référence huit cas réussis : partitions/redémarrages, panne sous charge, changement de poids suivi d’arrêts forcés et seuil strict des deux tiers, sur Paris et Shanghai. Les tests vérifient 720 transferts, seize historiques de pairs et huit reprises de journaux de vote. Ils complètent les preuves après correction du journal de signature et des estimations RPC ; les limites de coupure électrique, réseau multi-hôte, endurance et crash pendant assemblage de grandes propositions restent ouvertes.

### Contrainte de lancement précisée le 2026-09-24

Le budget personnel annoncé est de 200 € au total pour la liquidité initiale et
les frais du bridge/pool. Réserver les frais avant de déterminer la somme
convertie en USDC ; aucun montant de frais n'est encore chiffré. Dans un modèle
lock-and-mint, les USDC bloqués sur la chaîne source garantissent les jetons
bridgés placés dans la pool Nano : ces deux positions ne sont pas deux réserves
indépendantes à financer. Cette contrainte ne vaut ni autorisation d'engager
les fonds, ni preuve de sécurité du bridge, ni financement de l'exploitation
continue. Le modèle de confiance et le parcours de retrait restent à qualifier
avant toute ouverture aux dépôts externes.

### Assemblage BFT interrompu : premier périmètre qualifié (2026-09-24)

Six cas SIGKILL après 1/20/40 fragments reçus sur Paris/Shanghai passent avec
reprise sans vote fantôme, réassemblage intégral, quorum signé, dix reçus et
solde exact par cas, puis audit après réouverture. Deux régressions d'import
passent également. `bftAssemblyCrashQualification` conserve les preuves et
les premiers échecs du nouveau test. Le crash pendant transfert libp2p réel
et sous le superviseur de production reste à tester ; cette campagne ne ferme
pas la qualification réseau/multi-hôte ou de coupure électrique.

### Interruption du téléchargement BFT réel : deux réseaux validés (2026-09-24)

`bftDownloadCrashQualification` complète le test d'assemblage par une panne
observée du cœur et du relais après réception du premier fragment libp2p,
sur Paris et Shanghai. La reprise et l'accord des quatre pairs, les reçus,
les soldes exacts et les bases rouvertes passent les vérifications. Restent
le même parcours sous `nano run`, les autres points d'interruption, les essais
multi-hôtes, l'endurance et la coupure électrique. Le goal reste ouvert.

### Défaut de cycle de vie du superviseur corrigé (2026-09-24)

La préparation du scénario BFT sous `nano run` a révélé puis reproduit un relais
survivant à SIGKILL de son cœur. Le relais surveille maintenant son parent
direct ; les quatre suites de supervision et transferts fragmentés avec pannes
passent. Le manifeste `supervisorParentCrashQualification` conserve le rouge
initial et les preuves finales. Le prochain parcours reste l'interruption
d'une grande proposition BFT distribuée sous le superviseur réel : la correction
du relais orphelin en est un préalable, pas son équivalent.

### Grande proposition sous le superviseur réel : périmètre acquis (2026-09-24)

`bftManagedLargeCrashQualification` valide sur Paris/Shanghai quatre `nano run`,
l'admission RPC des transactions, la panne après le premier fragment et la
reprise réseau sans réinjection manuelle de mempool. Le bloc commun dépasse
16 Mio et les audits/reçus/soldes concordent. Restent les autres points de panne,
les essais multi-hôtes, l'endurance, la coupure électrique et les autres limites
déjà documentées. Le goal n'est pas achevé.

### Passage à l'échelle des validateurs demandé (2026-09-24)

L'utilisateur remet en question le plafond de 1 000 validateurs. Il est présent
à la fois dans NanoStaking.sol, finality.cpp et storage.cpp. Le plafond ne doit
pas être supprimé isolément : mesurer le CPU, les octets de votes rediffusés,
la mémoire et la finalisation quand le nombre de votants augmente ; optimiser
les recherches et validations répétées, puis examiner un protocole de diffusion
plus efficace et ses propriétés de sécurité/vivacité. Distinguer nombre total
de nœuds, nombre de voisins directs et ensemble des validateurs votants. Les
réseaux à quatre validateurs qualifiés ici ne prouvent pas la capacité à 1 000.

### Première optimisation mesurée des validateurs (2026-09-24)

Le collecteur réutilise désormais son ensemble votant immuable au lieu de le
normaliser et le hacher à chaque vote. La mesure locale à 1 000 membres passe
d'environ 1,69 s à 0,27 s pour la réception/participation ; toutes les signatures
restent contrôlées. Cinq suites finales passent, dont les changements de stake
avec redémarrage sur Paris/Shanghai. `voteCollectorScalingQualification` garde
les preuves et limites. Restent le coût externe du réacteur, les parcours de
quorum, la diffusion réseau et des mesures à l'échelle du réseau avant toute
révision cohérente du plafond de 1 000.

### Deuxième coût supprimé : rescans de participation (2026-09-24)

Le contrôle de participation utilise maintenant des poids exacts maintenus
pour les premiers votes validés, séparément par étape. Le modèle de référence
avant/après (776 contrôles) et six suites finales passent, dont les cas réseau
à exactement deux tiers. `voteParticipationQualification` conserve les preuves
et timings locaux. Les certificats, contrôles externes du réacteur et le gossip
restent à traiter ; le plafond de 1 000 et le statut de production sont inchangés.

### Première réduction réelle du trafic de votes (2026-09-24)

Le relais BFT géré filtre par pair les votes déjà connus/acquittés, avec deux
contextes bornés, réémission complète toutes les cinq secondes et remplacement
de l'état lors du changement de session. Les cinq suites de
`bftVoteDeltaQualification` passent, dont les grandes propositions et reprises
sous nano run sur Paris/Shanghai. Des réductions sont observées dans les traces.
Ce résultat ne suffit pas à qualifier 1 000 validateurs ou un coût global
linéaire ; les coûts restants du cœur et les essais réseau à plus grande
échelle demeurent nécessaires avant révision du plafond.

### Encodage des votes et limites restantes (2026-09-24)

Le réacteur conserve désormais les messages canoniques des premiers votes
validés et indexe leurs slots par étape/adresse. Les lectures ne refont plus
l'encodage cryptographique ; les contrôles entrants restent inchangés. Six
suites locales passent : finalité, consensus, lecture répétée et cycle de
proposeur sur Paris/Shanghai. Doublons, votes invalides, équivoques, transition
de round, reprise et disparition des pages après commit sont contrôlés.
Preuves : `bftSnapshotCacheQualification`. La mesure inclut le recalcul coûteux
des hashes de transactions de la proposition ; elle ne prouve pas une capacité
réseau à grande échelle.

La cible utilisateur est une architecture sans plafond arbitraire de 1 000
validateurs. Cette limite demeure actuellement dans le contrat, le décodage
du staking, les ensembles de vote persistés et les certificats RPC. La lecture
complète du staking impose aussi 5 millions de gas par appel et 50 millions
pour la synchronisation, avec copie d'état par appel et recherche linéaire
des enregistrements précédents. Supprimer la constante seule ne supprime donc
pas le plafond pratique. Le gossip doit également traiter des pages bornées
sans tronquer l'ensemble éligible.

Les frais nécessitent une distinction : la base fee suit la formule dynamique
EIP-1559 ; `eth_maxPriorityFeePerGas` recommande encore 1 Gwei fixe et
`eth_gasPrice` ajoute ce même montant à la base fee courante. Cette heuristique
RPC doit évoluer vers une observation de l'activité, avec comportement
explicite à vide. Elle n'impose pas ce pourboire aux transactions signées.

## Liste EIP intégrée au goal — 24 septembre 2026

Le périmètre demandé comprend les EIP 7251, 7002, 7685, 7917, 8045, 8068,
8363, 7805, 7594, 7732 et 8141, ainsi que l’identification des propositions
post-quantiques pertinentes. Voir [la liste et les critères de preuve](VALIDATOR-EIP-ROADMAP.md).
Ces éléments sont des travaux requis ; cette liste ne vaut pas implémentation.

## Compléments EIP recherchés — 24 septembre 2026

Le goal inclut les deux listes : [validateurs](VALIDATOR-EIP-ROADMAP.md) et
[EVM/production](EVM-PRODUCTION-EIP-ROADMAP.md). Auditer l’existant et combler
les écarts ; une entrée dans ces listes n’est ni une activation ni une preuve
de conformité. Les dépendances du fork complet font partie du périmètre.

## Suivi prioritaire de réalisation

La [matrice EIP](EIP-IMPLEMENTATION-STATUS.md) distingue implémenté, partiel et
non commencé, avec périmètre, preuves, priorité et travail restant. Les deux
listes de sources restent des annexes. Clôturer les écarts, sans repartir de
zéro ni considérer une liste comme une implémentation.

Complément Cancun/Prague : suivre 7516, 2537, 2935, 7623, 7691, 7840 et
le périmètre de 7642 dans la matrice ; provisionnement des contrats système,
SSZ consensus distinct de RLP transactions, et eth_createAccessList à qualifier.


### 2026-09-26 — Kurtosis native Beacon qualification

Two real Docker Nano nodes, 32 synthetic validators each, connected and finalized
execution block 15 with the same hash. Evidence: `qualification/kurtosis-20260926/summary.json`.
The initial harness needed RPC-loopback and assertion corrections; the corrected
finality checks passed on the same live nodes. This qualifies a bounded native
Bellatrix/Paris scenario, not ethereum-package/Lighthouse interoperability.
Engine capability RPC returns -32601. Priorities: container restart/partition,
bounded load, accurate Beacon supervision, and completing the selected fork
lifecycle. Assertoor and chaos scenarios remain unqualified by this campaign.

La suite des campagnes et leurs critères de clôture sont suivis dans
`SYSTEM-TEST-PLAN.md` : crash atomique, partitions/quorum, charge bornée,
invariants rejoués sur Nano et intégration Assertoor conditionnée aux API réelles.

### 2026-09-26 — Reprise Docker Beacon sous écritures

Campagne bornée réussie : SIGKILL (137), reprise du même volume avec témoins,
24 transactions SSTORE, reçus réussis, nonce/compteur exacts et finalité commune
au bloc 45. Preuves : `qualification/kurtosis-crash-20260926/`. Aucun test de
coupure électrique ni de frontière exacte d'écriture. Limites mesurées :
~236 s après réouverture avant clôture de finalité ; ~4,9 Go sur un nœud.
Analyser l'amplification de stockage et la latence avant toute conclusion
opérationnelle. Assertoor/Foundry/Chaos Mesh restent des campagnes distinctes.

### Correction d'allocation et comparaison qualifiée (2026-09-26)

`allow_fallocate=false` dans ChainStore supprime la réservation anticipée,
sans désactiver WAL ni sync. Les trois tests ciblés de durabilité passent ;
le rejeu Kurtosis SIGKILL/24 transactions passe aussi (bloc 39 commun).
Empreinte finale : ~114/51 Mio au lieu de ~4,9 Go par nœud. Reprise RPC
~3,2 s dans les deux essais ; finalité après reprise 188 s versus 236 s,
sans attribution causale à partir d'un seul essai. Preuves :
`qualification/rocksdb-allocation-20260926/README.md`. Prochaine priorité :
interfaces Beacon pour Assertoor, décrites dans `ASSERTOOR-INTEGRATION.md`.
Le pruning et la maîtrise de l'amplification d'écriture restent ouverts.

- 2026-09-26 : lectures Beacon head/root, fork et checkpoints intégrées au runtime
  et testées sur HTTP réel avec refus d'accès et indisponibilité. Détails et
  limites : `docs/qualification/beacon-reads-20260926/README.md`. Raccordement
  Assertoor toujours partiel, aucune clôture de la qualification système.

- 2026-09-26 : NodeSyncing natif implémenté avec observations P2P de slots distinctes
  des hauteurs EVM, expiration et refus des annonces futures. Deux processus et
  démons P2P testés avant/après redémarrage ; preuves dans
  `docs/qualification/beacon-sync-20260926/README.md`. Qualification Assertoor complète
  toujours ouverte (notamment corps de blocs et événements).

- 2026-09-26 : événements Beacon `block`/`finalized_checkpoint` publiés après
  commit durable, SSE borné intégré au serveur natif. Cinq tests passent,
  dont réception SDK sur deux processus et validation de 31 événements de
  blocs avec finalité. [Preuves](qualification/beacon-events-20260926/README.md).
  Raccordement Assertoor avancé ; suite complète et qualification production
  toujours ouvertes. Aucun nouveau statut de conformité EIP attribué.

- 2026-09-26 : première exécution réelle d'Assertoor officiel contre deux nœuds
  natifs. Santé EL/CL, progression synchronisée et deux nouveaux corps de blocs
  passent, sans désactivation des contrôles. Cible CTest optionnelle ajoutée.
  [Preuves et limites](qualification/assertoor-native-20260926/README.md).
  Finalité non nulle, partitions et catalogue Assertoor restent à qualifier ;
  aucune EIP ou conformité production supplémentaire déclarée par déduction.

## Échec ouvert : finalité avec Assertoor — 2026-09-26

La nouvelle cible `consensus_bellatrix_assertoor_finality` répartit les 64 clés
sur deux processus. Santé, synchronisation et propositions passent, mais la
finalité reste à l'époque 0 : timeout après 240 s, campagne 265,84 s. Les reçus
natifs confirment ce résultat ; deux lectures Beacon 503 sont également observées.
Cause non établie. Priorité : contention RPC/consensus et propagation des votes,
avec témoin sans Assertoor ; ne pas assouplir le seuil d'époque finalisée 2.
[Preuves et diagnostic](qualification/assertoor-finality-20260926/README.md).
La finalité sous cette supervision n'est pas qualifiée ; les partitions Assertoor
et la clôture production restent ouvertes.

- 2026-09-26, diagnostic de finalité : le témoin **sans Assertoor** échoue aussi
  à retrouver des têtes communes (253,70 s), malgré un checkpoint finalisé commun
  époque 2 à l'arrêt. Un ordonnancement qui saute les catégories vides corrige
  une limite de capacité du relais, mais ne suffit pas : Assertoor reste en échec
  (269,82 s, finalisée 0). Sorties et slashing passent (10,30/5,33 s).
  [Preuves avant/après](qualification/bellatrix-relay-scheduling-20260926/README.md).
  Résolution toujours ouverte. Conservation automatique des bases et des blocs
  de tête ajoutée pour le prochain diagnostic ; aucun changement de quorum.

- 2026-09-26 — Critère de réussite continue : nouvelle cible
  `consensus_bellatrix_assertoor_sustained`, quatre avancées consécutives de
  finalité commune puis quatre après redémarrage, avec cohérence Beacon/EVM.
  Le diagnostic distingue le défaut JSON/SSZ de la sonde, le retard réel du
  relais et un build sans optimisation. Cache de validation historique ajouté
  sans modification des règles ni des checkpoints ; trois cibles Release
  réussies (9,48 s). Campagne prolongée **réussie en 616,36 s** : quatre
  avancées communes 2→6, puis quatre 7→11 après redémarrage ordonné ; 64
  observations sans écart de tête ou d’époque finalisée. Qualification locale
  bornée en Release, sans conclusion de production ou de tenue indéfinie.
  [Preuves et limites](qualification/assertoor-native-finality-20260926/README.md).

- 2026-09-26 — Reprise prolongée : **SIGKILL distribué réussi** (325,89 s),
  finalité commune 2→3→4→5. **Partition prolongée en échec** (521,06 s) :
  sûreté conservée pendant 96 s d’isolement et convergence ultérieure, mais
  retard de finalité supérieur à quatre époques au début du contrôle prolongé.
  Le succès du redémarrage ordonné ne clôt donc pas la reprise après partition.
  [Preuves, critères et limites](qualification/recovery-sustained-20260926/README.md).

- 2026-09-26 — Contre-épreuve après partition : un relais par salves bornées
  améliore la convergence initiale dans cet essai mais échoue au contrôle
  prolongé (501,40 s, finalité immobile deux époques). Hypothèse insuffisante,
  patch expérimental retiré. Prochaine analyse : attestations incluses et
  checkpoints source/cible après reconnexion.
  [Preuves](qualification/recovery-sustained-20260926/README.md#contre-épreuve-du-délai-de-relais).

- 2026-09-26 — Diagnostic hors ligne ajouté (`--attestation-report`) et vérifié
  sur les deux bases arrêtées : rapports identiques, participation utile divisée
  entre branches après partition. Les votes inclus avec une mauvaise cible sont
  distingués des votes utiles. Prochaine correction à cibler : convergence des
  branches / inclusion des attestations, sans modifier quorum ni slashing.
  [Détails](qualification/recovery-sustained-20260926/README.md#diagnostic-des-attestations-incluses).

- 2026-09-26 — Cache borné des acquittements de votes par session ajouté :
  tests de réessai/perte de réponse/session/éviction réussis, retransmissions
  répétées réduites dans le scénario réel. La finalité après partition reste
  en échec (503,39 s). Ne pas assimiler cette correction de transport à une
  résolution du choix de branche ou à la préparation production.
  [Preuves](qualification/recovery-sustained-20260926/README.md#acquittements-de-votes-mémorisés-par-session).

- 2026-09-26 — Nouveau témoin déterministe de partition à livraison directe :
  réussi en 122,04 s, six avancées communes 6→12 après deux époques d’isolement.
  Deux cœurs durables, BLS/EVM/RocksDB réels, horloge simulée et aucun P2P.
  Le contraste avec l’échec réseau cible désormais l’ordre et les délais de
  livraison/rattrapage ; le critère réseau n’est ni remplacé ni assoupli.
  [Preuves](qualification/recovery-sustained-20260926/README.md#témoin-à-livraison-directe-et-horloge-contrôlée).

- 2026-09-26 — Observabilité des délais du validateur ajoutée. Le témoin
  instrumenté passe (134,92 s), sans cycle de six secondes. Une copie arrêtée
  reprend ensuite avec horloge réelle et passe de la finalité 6 à 9 sans pair
  pendant une courte observation. L’échec réseau caractérise un délai excessif
  selon le critère strict ; il ne prouve pas un blocage définitif. Critère intact.
  [Mesures](qualification/recovery-sustained-20260926/README.md#mesure-des-échéances-du-validateur).

- 2026-09-26 — Coalescence des observations d’horloge sans changement de phase
  ajoutée : équivalence d’état et protections d’horloge testées, quatre cibles
  réussies (29,38 s). Reprise courte : 33 publications contre 167, même finalité.
  La partition prolongée échoue en 519,24 s : retard de finalité supérieur
  à quatre époques. Aucun cycle validateur ne dépasse six secondes ; la réduction
  des écritures ne clôt pas le défaut de convergence/finalité.
  [Preuves](qualification/recovery-sustained-20260926/README.md#coalescence-des-observations-dhorloge-bellatrix).

- 2026-09-26 — Diagnostic borné des votes locaux ajouté, inactif par défaut.
  Test dédié réussi (8,56 s) : votes réellement admis, éviction, désactivation
  et accord des deux cœurs. Partition instrumentée en échec (522,42 s) : saut d’époque finalisée.
  Les traces révèlent une stagnation temporaire des messages distants ;
  correction en cours du parcours des votes déjà acquittés.
  Ce diagnostic ne ferme pas le défaut de finalité.
  [Preuves](qualification/recovery-sustained-20260926/duty-trace/README.md).

- 2026-09-26 — Parcours asynchrone borné des votes déjà acquittés ajouté.
  Le test atteint un vote nouveau derrière 70 connus en trois tentatives ;
  perte de réponse et réessai conservés (0,32 s). Relais P2P reconstruit,
  partition en échec (373,12 s), suspicion de course entre lectures RPC.
  La fraîcheur des votes distants progresse bien sur les traces.
  [Preuves](qualification/recovery-sustained-20260926/cached-scan/README.md).

- 2026-09-26 — Course du moniteur reproduite puis corrigée : deux lectures
  de tête encadrent le checkpoint, trois tentatives maximum. Conflits et reculs
  restent fatals pendant une course. Neuf tests passent, seuils inchangés ;
  qualification réseau relancée avec cette mesure corrigée.
  [Preuves](qualification/recovery-sustained-20260926/consistent-sampling/README.md).

- 2026-09-26 — La lecture cohérente seule ne résout pas l’échec (373,31 s).
  Diagnostic prouvé sur le checkpoint arrêté : blocs 62–63, finalité réalisée 2
  mais non réalisée 6 ; bloc 64, réalisée 6. Le scénario exige désormais
  la concordance Beacon réalisé / exécution avant de démarrer sa fenêtre,
  dans le même délai de cinq époques. Neuf tests du moniteur passent ;
  qualification réseau en cours. Les résultats précédents restent des échecs.
  [Preuves](qualification/recovery-sustained-20260926/recovery-bound/README.md).

- 2026-09-26 — **Partition prolongée réussie, 517,99 s** avec le relais corrigé
  et le seuil de reprise lié aux deux projections : coupure 96 s, reprise 95 s,
  trois finalités communes 6→7→8→9, mêmes blocs d’exécution et arrêt propre.
  Deux cœurs / deux P2P, 64 clés synthétiques, un Mac. Clôture de ce scénario
  local uniquement ; les échecs historiques et autres critères production restent
  conservés. Prochaine validation : arrêt brutal avec les binaires actuels.
  [Preuves](qualification/recovery-sustained-20260926/recovery-bound/README.md).

- 2026-09-26 — **Reprise SIGKILL et finalité continue réussies, 325,58 s** :
  deux cœurs / deux P2P, 64 clés synthétiques sur un Mac ; trois avancées
  communes 2→3→4→5 après redémarrage, liées aux blocs d’exécution finalisés.
  Empreintes des binaires inchangées depuis la partition réussie et vérifiées.
  Ce scénario ne couvre pas une panne électrique ni la rétention longue.
  [Preuves](qualification/recovery-sustained-20260926/crash-current/README.md).

- 2026-09-26 — Primitive d’archive atomique de checkpoint implémentée et testée :
  deux cibles passent en 1,49 s, avec réorganisation, SIGKILL et suffixe WAL
  conservé/perdu. **Raccordement Bellatrix/P2P et élagage non réalisés.**
  [Preuves](qualification/checkpoint-archive-20260926/README.md).

- Critère bridge ajouté : avant tout mint, vérifier l’inclusion du dépôt dans
  l’ascendance d’un checkpoint source finalisé selon le protocole choisi.
  Un seuil arbitraire de 12–64 blocs n’est pas une preuve de finalité.
  Tester dépôt réorganisé avant finalité (aucun mint), rejeu (un seul mint),
  finalité bloquée (attente), source RPC mensongère et conflit de finalité
  (arrêt de nouvelles émissions, incident explicite). Une réorganisation
  exceptionnelle après mint ne s’annule pas automatiquement sur Nano : plan
  de traitement des engagements et modèle de confiance à définir avant fonds.
  Références : https://ethereum.org/developers/docs/consensus-mechanisms/pos/faqs
  et https://docs.base.org/base-chain/specs/protocol/consensus/derivation.

## 2026-09-27 — Sélection locale des sorties Capella

Le constructeur peut désormais sélectionner des sorties volontaires candidates
par transition complète, avec budgets et annulation. Nouveau scénario : bloc
indépendant identique, quatre candidats invalides/doublons exclus, deux reorgs
et réouvertures. Test 4,42 s ; régression BLS 4,87 s. Échec initial du banc conservé.
La file persistante, RPC/P2P et le cycle jusqu'au retrait restent à raccorder ;
aucune clôture EIP/production. [Preuves](qualification/capella-local-exits-20260927/README.md).

## 2026-09-27 — File durable de sorties volontaires Capella

Admission authentifiée par clé/index et état de branche, format durable v4
avec revalidation à la reprise, doublons idempotents et sélection automatique.
Nouveau scénario : admission sans modifier le consensus, capacité pleine,
réouverture et deux reorgs rendant la sortie applicable puis non applicable.
3/3 tests passent en 13,98 s ; nouveau test 4,72 s. Le retrait après finalité et
le réancrage à l'élagage sont codés mais restent à qualifier ; RPC/P2P, pannes
WAL, réindexation de dépôts et retrait complet restent ouverts.
[Preuves et limites](qualification/capella-queued-exits-20260927/README.md).

## 2026-09-29 — Sorties Capella raccordées au service et au relais

Admission des sorties par le worker et les reçus RPC privés, catalogue/export
bornés, vérification d'empreinte et rotation de diffusion du daemon P2P ajoutés.
3 tests cœur passent en 14,92 s ; 35 scénarios de relais simulé passent en 0,45 s.
Le daemon compile. La propagation réelle Mac/VPS, HTTP, le retrait final et les
pannes du parcours complet restent à démontrer ; aucun objectif clôturé.
[Preuves et limites](qualification/capella-exit-wire-20260929/README.md).

## 2026-10-01 — Nouveau scénario : majorité connectée pendant partition

Banc ajouté, exécution WAN encore à qualifier : liaison directe privée entre
Mac natif et Docker Mac, coupure VPS de 192 s. Le poids est lu dans les
balances effectives SSZ de la fixture initiale liée par SHA-256 :
1 376 000 000 000 / 2 048 000 000 000 gwei pour les deux Mac, strictement
plus des deux tiers ; 672 000 000 000 pour le VPS. Le nombre de clés ne
remplace pas la vérification du stake. La majorité doit finaliser au-delà
des engagements reçus avant la coupure ; le VPS isolé ne doit pas dépasser
ce plafond conservateur. Reconnexion : finalité commune plus récente que
celle observée dans la majorité, budget strict inchangé de 180 s.

47 tests du banc passent, dont 13 nouveaux cas de stake, faux positifs et
restauration. Ce résultat ne qualifie pas encore la progression WAN majoritaire.
Aucun changement de moteur, EIP, image ou seuil de consensus ; candidats
418aceb réutilisés. Les échecs historiques et tous les autres critères de
production restent ouverts. Voir `tests/multihost/README.md`.

## 2026-10-01 — Îlot majoritaire R : critère strict non atteint

Candidats 418aceb, banc a994bed, sortie 1. Pendant la coupure VPS de 192 s,
les deux Mac restent mutuellement connectés et finalisent 2→4 ; le VPS sans
pairs reste à 2. Le critère exigeait >5 pour exclure les engagements reçus
avant coupure : échec conservé. IP restaurée, trois signers sains, conteneurs
arrêtés sans OOM. Reprise commune et stabilité non exécutées après cet échec.

Rejeu canonique arrêté lié au checkpoint externe : 54 blocs, 143296 octets,
1 transaction. Identification BLS de 180 votes des époques 4–7. Aux époques
5/6/7, les 43 clés majoritaires sont incluses, mais seulement 38/35/39 avant
la frontière suivante ; source 4/4/5. Pas de preuve de blocage permanent ou
d'une cause unique, ni de raison de changer le quorum. Une nouvelle coupure
soutenue de 384 s doit distinguer retard et blocage, sans clôturer le critère
192 s ni modifier la reprise stricte de 180 s. Aucun nouveau build réseau.
Tous les autres critères de production et échecs historiques restent ouverts.
[Preuves et limites](qualification/majority-island-20261001/attempt-r/README.md).

## 2026-10-01 — Îlot majoritaire S : progression qualifiée, reprise en échec

Essai soutenu distinct de 384 s, candidats 418aceb, banc 3e210dc, sortie 1.
Les deux Mac restent connectés et finalisent ensemble 6 puis 7 au-delà du
plafond pré-coupure 5 ; le VPS sans pairs reste à 2. Cette progression
majoritaire bornée passe. Le critère R de 192 s demeure en échec conservé.

Reprise commune : échec au budget strict de 180 s, lecture complète observée
à 182905,931291 ms (ce n'est pas un temps de reprise). La fenêtre diagnostique
supplémentaire de 180 s échoue aussi : finalité 15 sur les Mac contre 2 sur
le VPS, malgré reconnexion. Stabilité commune non exécutée. Signers et
horloges sains ; conteneurs arrêtés 0 sans OOM.

Les deux historiques arrêtés se rejouent avec checkpoints externes : Mac
108 blocs / 314134 octets, VPS 79 / 199854 octets, 1 transaction chacun.
Sur 107 enveloppes canoniques Mac, le VPS n'en archive que 31 identiques
(slots 3–33). Il manque 76 enveloppes suivantes ; la présence a été vérifiée
dans les archives, pas seulement dans l'état actif élagué.

Manque confirmé : après élagage, le fournisseur refuse 85 blocs canoniques
archivés des slots 3–119 ; son ancre signée 120 est exportable mais le
parcours de dépendances s'arrête avant de l'envoyer. Le VPS ignore cette
nouvelle ancre. L'ascendance requise pour admettre les descendants n'est
donc pas fournie. Ce manque précis reste à corriger avec export historique
authentifié et livraison bornée, sans saut de confiance ni relâchement
d'admission. Une cause unique de tout l'échec n'est pas démontrée.
Aucune correction livrée dans cette tranche ; tous les autres critères de
production et historiques demeurent ouverts.
[Preuves et limites](qualification/majority-island-20261001/attempt-s/README.md).


### 2026-10-01 — export historique Capella : correction, qualification native

L’export suit désormais les archives canoniques depuis l’ancre finalisée,
avec vérification des racines et signatures, budget par requête et cache
de préfixes authentifiés. Une ancre signée exportable est livrée avec ses
parents ; les doublons historiques ne sont acquittés que sur égalité avec
l’archive locale authentifiée, sans mutation du checkpoint.

Nouveau test natif : 22 blocs historiques, cinq cas négatifs, reprise
bornée du parcours et refus de signature différente. Régression de
transition signée et modèles réseau pertinents passent. Les erreurs de
cible de build et d’hypothèse initiale sur le budget sont conservées.
Docker ARM64 en cours ; AMD64, rattrapage réel et reprise WAN sous 180 s
restent ouverts. Les critères de production antérieurs sont conservés.
[Preuves natives](qualification/capella-historical-export-20261001/native/README.md).

Qualification ARM64 Docker terminée : quatre tests ciblés passent. La sonde
en lecture seule sur la base WAN S exporte désormais les 85 archives auparavant
refusées (107/107 disponibles), checkpoint externe vérifié. Cela qualifie
la fourniture locale, pas la reprise réseau. AMD64 et WAN restent ouverts.
[Preuves ARM64](qualification/capella-historical-export-20261001/arm64/README.md).


### 2026-10-01 — correction historique : candidates portables

Les quatre régressions ciblées passent désormais sur AMD64 aussi : export
historique 116,84 s, transition signée 42,52 s, livraison de blocs et
catalogue. Sur ARM64, deux vrais démons / 64 validateurs atteignent une
finalité commune 2 stable sur trois observations. Inclusion sync et rotation
ne sont pas qualifiées par ce scénario.

Une seule image runtime par architecture est assemblée à partir des
binaires de 5bd6f95 et vérifiée (révision, utilisateur non root, architecture,
empreintes). Le refus initial d’un marqueur AMD64 mal formaté, avant tout
build d’image, est conservé. Sources et limites consensus inchangées.
Prochaine gate : îlot majoritaire connecté 384 s et retour à une finalité
commune sous 180 s. Cette reprise WAN reste non acquise ; toutes les gates
antérieures de production restent ouvertes.
[Candidates et preuves](qualification/capella-historical-export-20261001/amd64/README.md).


### 2026-10-01 — WAN T : majorité vivante, timeout Beacon à la reprise

Runtime 5bd6f95, banc 9b694ef, sortie 1. La majorité Mac connectée
finalise 2→8 pendant la coupure 384 s ; plafond pré-coupure 5 dépassé,
VPS isolé à 2. Adresse réseau restaurée, puis deux pairs sur chaque nœud.

Une lecture Beacon de finalité sur une racine immuable du VPS dépasse
5 s sans réponse, 30,993 s murales après reconnexion. Ce nombre n’est
ni une durée de reprise réussie ni un échec de la fenêtre entière 180 s :
la campagne s’arrête sur l’indisponibilité de lecture. Stabilité non exécutée.
À l’arrêt : Mac finalité 9, VPS 3 ; progrès partiel, reprise commune non acquise.
Signataires/horloges sains ; deux conteneurs arrêtés 0 sans OOM, natif absent.

Inspection : les méthodes privées Capella sont absentes de simulation_item,
contrairement à Bellatrix. Elles s’exécutent synchroniquement dans la boucle
HTTP, y compris export historique et attentes du verrou d’admission. Manque
concret de dispatch borné à corriger et qualifier ; cause exclusive du timeout
non démontrée. Aucun délai ni quorum assoupli ; S et les autres gates conservés.
[Preuves et sources T](qualification/majority-island-20261001/attempt-t/README.md).


### 2026-10-01 — RPC Capella hors boucle HTTP : qualification native

Les 14 méthodes privées Capella passent désormais dans la file bornée
existante, notifications et lots compris. Aucun worker, budget de file,
délai Beacon ou critère WAN augmenté. Nouveau test HTTP avec verrou
d’admission réel : échec avant correction, réussite après (20,45 s).
Identité publique et finalité Beacon répondent pendant l’attente du RPC
privé, checkpoint inchangé. Régression d’expiration/capacité HTTP et
WebSocket passe aussi (1,84 s). Erreur initiale de compilation du test
conservée, corrigée sans modifier le critère.
Docker et effet WAN restent à qualifier ; T/S et toutes les gates de
production restent ouvertes. Cause exclusive de T non démontrée.
[Preuves natives](qualification/capella-http-queue-20261001/native/README.md).


### 2026-10-01 — attente de santé d’horloge : correction et cas déterministe

Première qualification AMD64 du dispatch : échec conservé, identité prête
mais Beacon non prêt, RPC privé en attente et checkpoint intact. healthy()
prenait le mutex de poll, lequel attendait l’admission. Le défaut irréversible
d’horloge est désormais lu atomiquement ; ordre des polls, bornes de saut,
invalidation sur défaut et annulation inchangés. Cela ne garantit pas la
fraîcheur d’une observation ni la progression de finalité.
Le test force maintenant un poll réel de la même horloge pendant le verrou
d’admission : Beacon et identité répondent dans les fenêtres 500 ms inchangées.
Natif : nouveau cas 18,83 s, quatre diagnostics fail-closed 0,99 s, expiration
de file 1,88 s. Docker et gain WAN restent à qualifier ; anciennes réussites
locales sans contention d’horloge forcée ne ferment pas cette nouvelle gate.
T reste un échec. Rejeux canoniques arrêtés T : Mac 78 blocs / 216556 octets,
VPS 36 / 96407, une transaction chacun, checkpoints externes exacts.
Pas de rejeu des votes hors bloc ni de qualification du fork choice historique.
[Preuves et limites](qualification/capella-http-queue-20261001/native-clock-latch/README.md).


### 2026-10-01 — correction horloge/HTTP qualifiée sur les deux architectures

Source compilée `74784c499cc904eba1d766933bc2017d3e2be9a0`. Le cas
de contention forcée passe sur Docker ARM64 (8,00 s) et AMD64 (26,93 s) :
Beacon et identité publique répondent avant libération du verrou d’admission,
pendant que poll et requête privée attendent. Fenêtres 500 ms inchangées.
Quatre diagnostics d’horloge et expiration de file passent aussi sur les deux
plateformes. Une image candidate par architecture, utilisateur 10001:10001,
révision et empreintes des deux binaires vérifiées. P2P réutilisé par égalité
des sources network/third_party et de son empreinte antérieure.
Le premier échec AMD64 et les échecs WAN S/T restent conservés. Aucun gain
de reprise WAN ni état production n’est encore démontré par ces tests.
[Preuves ARM64](qualification/capella-http-queue-20261001/arm64-clock-latch/README.md)
et [AMD64](qualification/capella-http-queue-20261001/amd64-clock-latch/README.md).


### 2026-10-01 — WAN U : majorité connectée et reprise bornée réussies

Candidat `74784c4` (Capella/Shanghai), harness `3ab9c16`, deux hôtes /
trois processus / 64 clés synthétiques. Même coupure de 384 s, limite de
reprise 180 s et ressources que S/T. Mac natif + Docker, réellement reliés
et >2/3 du stake, finalisent jusqu’à 8 au-delà de la borne 5 ; VPS isolé à 2.
Après restauration du réseau, finalité commune 10 en **105,915 s**.
Les lectures Beacon par racine immuable restent disponibles.
Fenêtre de stabilité : les sauts 11→13 puis 13→15 restent enregistrés et
remettent le compteur à zéro ; trois avancées consécutives 15→18 passent
ensuite en 266,875 s sous le délai absolu inchangé de 288 s. Hashes
d’exécution finalisés communs, trois boucles/horloges saines ; conteneurs
arrêtés 0 sans OOM, processus natif absent.
Ce scénario de majorité connectée/reconnexion est qualifié pour ce candidat.
Cela ne qualifie ni tous les scénarios asymétriques, ni le cycle complet et
l’échelle des validateurs, ni la totalité des critères initiaux de production.
S/T conservés ; pas de cause exclusive affirmée ni de lancement financier.
[Preuves WAN U](qualification/majority-island-20261001/attempt-u/README.md).


### 2026-10-01 — nouvelle couverture : preuve et finalité sans membre enregistré

Le nouveau scénario natif `consensus_capella_unknown_finality_slashing` passe
en 176,14 s. Quatre époques de duties réellement signées sont admises à la
frontière suivante ; aucun checkpoint injecté, règle ou seuil changé.
Preuve admise avec justification 10 / finalité 8, puis **finalité 9 alors
que le membre reste absent**. Archive et checkpoint conservés, rejouabilité
sans mutation, deux propriétaires durables réouverts avant et après cette
avance ; retrait de preuve seulement après sanction finalisée.
`unknownFinalityAdvances=1`, 64 observations sans membre et 16 contrôles
falsifiés refusés. La branche jusque-là non atteinte est couverte en natif.
Premier fixture à deux époques : échec de couverture conservé, aucune
corruption runtime démontrée. Qualification Docker ARM64/AMD64 encore ouverte.
Cœur runtime inchangé ; ce résultat ne ferme pas le cycle global ni les
autres critères initiaux de production.
[Preuves natives](qualification/capella-unknown-finality-slashing-20261001/native/README.md).


### 2026-10-01 — rétention sans membre : qualification portable achevée

Même nouveau scénario de test `c2e9e26` réussi en natif 176,14 s, Docker
ARM64 163,22 s et VPS Docker AMD64 **585,94 s** sous la limite inchangée
de 600 s. Une avance réelle de finalité avec membre absent, preuve retenue,
réouverture des deux propriétaires avant/après cette avance, rejeu sans
mutation et retrait après sanction finalisée ; 16 refus falsifiés sur chaque
plateforme. Cette branche auparavant `unknownFinalityAdvances=0` est désormais
couverte et qualifiée pour le cœur courant. Premier fixture négatif conservé.
Le temps AMD64 est celui de ce fixture comprimé à deux propriétaires durables,
pas une latence de finalité ou de reprise WAN.
Sources runtime identiques à `74784c4` et empreintes core/P2P identiques
aux candidats WAN U : aucune nouvelle image ni relabellisation. Les gates
globales de cycle/scalabilité, version complète, exploitation, revue et
bridge/lancement restent ouvertes ; aucun fonds réel ni production déclarée.
[ARM64](qualification/capella-unknown-finality-slashing-20261001/arm64/README.md)
et [AMD64](qualification/capella-unknown-finality-slashing-20261001/amd64/README.md).


### 2026-10-01 — livraison durable des alertes, qualification locale

Implémenté : file privée fsync fichier/répertoire avant émission, identité du
monitor épinglée depuis sa configuration revue, reprise et identifiant
idempotent conservé après acceptation HTTP suivie d'un échec du commit local.
Déduplication des défauts, notification de retour sain, refus des redirections,
bornes de réponse/temps/file et vidage explicite sans perte des anciens événements.
31 tests réussis en natif et Docker ARM64 : 10 nouveaux cas de livraison
et 21 cas existants du monitor. Receveurs HTTP locaux et rapports synthétiques ;
pas encore de campagne daemon Nano vers destinataire réel. Docker AMD64,
ordonnanceur/destination opérateur, panne électrique et qualification globale
restent ouverts. Cœur C++ et images de chaîne inchangés.
[Preuves](qualification/alert-delivery-20261001/native/README.md).


### 2026-10-01 — alertes : qualification portable de la file achevée

La même source `cb6790d` récupérée par Git sur le VPS passe les 31 tests dans
le Docker AMD64 existant (2 CPU, 512 MiB, réseau externe désactivé),
2 680,22 ms, aucun échec ni test ignoré. Empreinte Node Linux vérifiée avant
exécution. Natif, ARM64 et AMD64 qualifient les cas de file/reprise décrits
ci-dessus ; aucun daemon ou destinataire opérateur réel dans cette campagne.
Le raccordement vivant, la planification et les autres gates globales restent
ouverts. Pas de nouvelle image, de fonds ou de production déclarée.
[Preuves AMD64](qualification/alert-delivery-20261001/amd64/README.md).


### 2026-10-01 — outbox durable entre monitor et file d'alertes

Le cycle `monitor-and-deliver` persiste rapport et état ensemble avant remise,
reprend le rapport en attente avant toute nouvelle observation et refuse
identités incompatibles/collisions de fichiers. Le monitor seul ne peut plus
écraser un rapport en attente. Quatre nouveaux tests d'intégration, dont un
SIGKILL réel du superviseur suivi d'une reprise ordonnée défaut/retour sain,
passent en natif et ARM64 : 35 tests totaux, aucun échec/ignoré. Endpoints RPC
synthétiques ; campagne daemon Nano vivant, destinataire opérateur, AMD64 et
perte électrique restent ouverts. Cœur C++ et images inchangés.
[Preuves](qualification/monitor-handoff-20261001/native/README.md).


### 2026-10-01 — reprise du superviseur : qualification AMD64 achevée

Même source `fafd9ec` récupérée par Git : 35 tests réussis sur le VPS
Docker AMD64, 3 116,09 ms, aucun échec/ignoré, sous 2 CPU/512 MiB.
Les quatre nouveaux cas de raccordement durable sont désormais qualifiés
en natif/ARM64/AMD64. SIGKILL du superviseur réel, RPC synthétiques ;
aucune revendication de panne d'un daemon Nano ou de perte électrique.
Campagne vivante, destination/ordonnanceur et gates globales restent ouverts.
[Preuves](qualification/monitor-handoff-20261001/amd64/README.md).


### 2026-10-01 — campagne d'alertes depuis daemon vivant préparée

Nouveau mode privé `NANO_TEST_ALERT_DELIVERY=1` associé au monitor et aux
images Capella épinglées. Le harness impose une référence saine, pause le
conteneur VPS réel, vérifie `unavailable`, refuse le premier POST par HTTP503,
vérifie la file durable et le même eventId au retry, reprend le daemon et
exige réception du retour sain sous 60 secondes. La fenêtre stricte de
finalité commune et les statuts terminaux des validateurs suivent séparément.
Receveur local de qualification, pas de destination opérateur provisionnée.
Implémentation préparée, campagne non encore qualifiée ; aucun gate fermé
par la simple présence du script. Cœur et images inchangés.


### 2026-10-01 — alertes depuis Nano vivant : campagne bornée réussie

Run complet `e32dce4`, candidats runtime `74784c4` inchangés, terminal 0.
Mac natif + Docker ARM64 Mac + Docker AMD64 VPS, deux hôtes/64 clés.
Pause réelle du process tree Nano/P2P du VPS (1 892 ms observés),
`unavailable` détecté ; premier POST HTTP503 refusé, message conservé,
retry avec même eventId accepté, retour sain reçu après reprise. Trois
POST, deux notifications logiques acceptées par le récepteur local de test.
Ensuite fenêtre stricte indépendante liée à l'exécution : finalité commune
2 → 5, trois avancées en 134 283 ms ; monitor 23 observations saines,
mauvais genesis refusé. Trois validateurs sains à l'arrêt (137/134/137
attestations admises), conteneurs sortis 0 sans OOM/pause, PID natif absent.
Ce scénario ferme la couverture de panne brève/alerte/retry/retour sain
pour un daemon réel avec récepteur de qualification ; pas destination
opérateur provisionnée, ordonnanceur, panne longue/SIGKILL daemon ou
perte électrique. Les autres gates initiales, revue et bridge/lancement
restent ouvertes. Aucune nouvelle image ni fonds ni production déclarée.
[Preuves](qualification/alert-live-20261001/attempt-a/README.md).


### 2026-10-01 — CI de qualification ajoutée, exécution GitHub ouverte

Premier workflow du dépôt : tests autonomes des outils/harness sur changements
concernés, actions officielles épinglées, permissions contents read, logs/révision
conservés, aucun déploiement ni accès VPS/clés. 227 tests autonomes passent
localement. Job manuel de compilation et neuf contrôles C++ avec vérification
exacte des inscriptions CTest, budgets inchangés. Workflow GitHub réel et job
C++ non encore qualifiés. Ce socle n'est ni la CI complète ni une preuve de
reproductibilité bit à bit ; matrice portable/P2P, gates WAN/EEST/régression
globale et protections de branche restent ouverts. [Portée](CI.md).


### 2026-10-01 — première CI GitHub effectivement réussie

Run GitHub `36885989682`, source `d8fa839`, completed/success :
227 tests autonomes réussis, zéro échec/ignoré, artefacts récupérés de GitHub.
Le job C++ était volontairement skipped sur ce push. Premier dispatch C++
non conteneurisé annulé sur demande avant toute qualification ; recette
corrigée `ec5e48b` avec Ubuntu Docker épinglé, 2 CPU/3 GiB.
Qualification C++ CI et CI globale restent ouvertes ; pas de déploiement
ou de production déclaré. [Preuves](qualification/ci-20261001/operations-github/README.md).


### 2026-10-01 — premier build CI Docker : refus Git identifié

Run `36886531310`, source `43e449d`, completed/failure : ownership du
checkout dans le conteneur refusé par Git avant CMake. Aucun build/test C++
n'a exécuté. Exception safe.directory limitée au checkout courant pour la
seule lecture de révision ; aucun wildcard global. Installation explicite
de build-essential ajoutée également (omission distincte, pas cause démontrée
du refus Git). Requalification nécessaire. Échec et annulation terminale
du premier dispatch non Docker conservés. Gates C++/CI globale ouverts.
[Échec](qualification/ci-20261001/core-first-docker-failure/README.md).


### 2026-10-01 — compilation C++ CI Docker effectivement qualifiée

Run `36887129414`, source `ee565c1`, completed/success. Checkout frais
GitHub, Docker Ubuntu épinglé, 2 CPU/3 GiB, Clang 18.1.3, job 14m21s.
Neuf inscriptions CTest vérifiées exactement et neuf tests JUnit exécutés
réussis sans skip, 26,56 s. Nano et les exécutables sélectionnés compilés.
Ce gate est clos pour ce sous-ensemble ; P2P, matrice complète, EEST global,
reproductibilité bit à bit et production restent ouverts. Échecs conservés.
Relevé read-only des deux images runtime : mêmes versions GMP/ICU/RocksDB/
OpenSSL, aucun ldd introuvable ; inventaire de sources/notices partiel, pas
SBOM exhaustif. Action d'archivage CI actualisée vers v6 Node24 épinglée pour
éliminer l'annotation observée ; validation du workflow modifié requise.
[CI C++](qualification/ci-20261001/core-docker-success/README.md),
[inventaire](qualification/dependency-inventory-20261001/README.md).


### 2026-10-01 — archivage CI actualisé qualifié sans rebuild Nano

Run `36889258299`, source `734a289`, completed/success : 227 tests
autonomes réussis, aucun échec/skip, upload-artifact v6 épinglé exécuté
avec succès et artefacts récupérés. Job core volontairement skipped ;
aucune nouvelle compilation/image C++ pour ce changement CI seul.
Le succès C++ distinct `ee565c1` et les gates globaux restants sont conservés.
[Preuves](qualification/ci-20261001/artifact-node24-success/README.md).


### 2026-10-01 — déclenchement automatique de la CI C++ en qualification

Le diff Git sélectionne désormais automatiquement le job Docker C++ sur les
changements de sources, dépendances, recettes, fixtures et workflows. Les
logs deviennent visibles en direct, avec conservation du statut d’échec ;
les neuf exécutables sont hashés et les paquets de build inventoriés.
232 contrôles autonomes réussis localement, dont cinq nouveaux contrôles
du routage et de la propagation d’échec. Validation GitHub encore requise.
Les neuf tests C++ sélectionnés restent le même sous-ensemble ; P2P, EEST
global, reproductibilité, qualification globale et bridge/fair launch restent
ouverts. Aucun runtime Nano ni image de chaîne modifié.


### 2026-10-01 — notices de livraison préparées et contrôlées

Les sept notices existantes sont complétées par les licences JSON/cpp-libp2p
et les cinq copyrights SPDX distincts du header JSON. Les chemins CMake et
Docker sont corrigés, avec vérification des onze empreintes. Contrôles en
lecture seule réussis sur les bases ARM64/AMD64 existantes et refus d’une
notice altérée. Installation réelle et assemblage des images encore requis ;
aucune recompilation runtime ni nouvelle image dans cette vérification.
Le SBOM transitif, la revue et la reproductibilité restent ouverts.
[Portée et traces](qualification/runtime-notices-20261001/README.md).

Installation CMake native ensuite réussie sans compilation, onze notices
vérifiées et binaire inchangé. Assemblage Docker ARM64 réel réussi
(`7b3db08d...`), utilisateur 10001, onze notices et les deux binaires
inchangés revérifiés. Assemblage AMD64 et Dockerfile complet restent requis.
Cette image de livraison ne remplace pas les images des preuves WAN.


### 2026-10-01 — CI C++ automatique et provenance qualifiées

Run `36890479029`, source `dad9a14`, completed/success. Routage push réel
vers Docker, 232 contrôles autonomes réussis, neuf tests C++ réussis sans
skip (27,44 s), neuf empreintes de binaires et paquets de build récupérés
et vérifiés. Job core 12m06s. Portée C++ inchangée : nouveau gate CI/provenance,
pas une nouvelle qualification complète du runtime. P2P/EEST global,
reproductibilité et critères restants de production toujours ouverts.
[Preuves](qualification/ci-20261001/automatic-core-success/README.md).


### 2026-10-01 — livraison des notices sur les deux architectures

Après push/pull de `930bf00`, assemblage AMD64 réel réussi sur VPS sans
compilation, téléchargements ou données de chaîne. Image `6eb8175c...` :
onze notices vérifiées, UID/GID 10001, CLI help réussi, deux binaires
strictement inchangés. ARM64 déjà vérifié (`7b3db08d...`).
Le job operations GitHub `36892286298` réussit : 232 contrôles, aucune
erreur/skip, rapport notices réel (11 empreintes/5 copyrights). Job core
encore en cours au relevé : aucun succès terminal de ce run revendiqué.
Correction de livraison qualifiée ; Dockerfile complet, SBOM transitif,
reproductibilité, régression complète et production globale restent ouverts.
[Preuves](qualification/runtime-notices-20261001/README.md).


### 2026-10-01 — job CI transport ajouté, recette ARM64 qualifiée

Routage réseau automatique et manuel séparé ; Docker Ubuntu épinglé,
2 CPU/3 GiB, builds deux tâches, registre exact des 21 tests réseau,
CTest série avec budgets existants (60 s pour un test sans budget explicite),
logs live, JUnit, révision cpp-libp2p et empreinte nano-p2p archivés.
Recette Docker ARM64 avec dépendances/volume réutilisés : 21 tests passés
sans skip, 1,02 s, binaire inchangé. Huit contrôles de routage/gate passent
localement, dont trois nouveaux. Premier refus pour Python absent conservé ;
contrôles JSON déplacés vers CMake déjà présent, sans nouvelle dépendance.
Construction fraîche GitHub et recette AMD64 restent requises. Ce gate
ne qualifie pas le WAN, la matrice EEST globale ou la production complète.
[Preuves bornées](qualification/network-ci-20261001/arm64-prebuilt/README.md).


### 2026-10-01 — Doppler choisi pour le lancement

Choix utilisateur explicite : Doppler sur Base, numéraire USDC souhaité.
Le fair launch ne doit plus prévoir une courbe maison ni imposer une graduation
V3 : protocole/migrateur et paramètres effectifs à vérifier dans Doppler.
Le lien du token Base avec NANO natif, le bridge, les droits sur la liquidité
et l’indexation à T0 restent des gates distincts. Pas de création, signature,
vente, financement ni transfert réel autorisé par ce choix de plateforme.
[Cadrage actualisé](../contracts/FAIR-LAUNCH.md).

Le gate réseau refuse aussi les propriétés CTest de désactivation/saut.
Vérificateur final appliqué au registre réel sauvegardé, puis huit contrôles
de routage/gate réussis ; aucun rejeu des 21 tests inchangés pour cette garde.


### 2026-10-01 — CI de la correction de notices terminée

Run `36892286298`, source `930bf00`, completed/success : 232 contrôles
autonomes et neuf tests C++ passés sans échec/skip (26,91 s). Rapport réel
11 notices/5 copyrights vérifié. Neuf empreintes de binaires identiques
au run `dad9a14` : répétabilité bornée de deux builds CI, pas encore une
release entièrement figée. P2P CI du commit `85a4a7c` reste à qualifier.
Production globale et ses autres critères conservés ouverts.
[Preuves terminales](qualification/ci-20261001/notices-core-success/README.md).


### 2026-10-01 — recette transport Docker AMD64 qualifiée

Après push/pull `85a4a7c`, registre complet de 21 tests réseau vérifié,
21 tests passés sans skip sur VPS Docker AMD64 (1,24 s), binaire nano-p2p
inchangé. Builder/dépendances/volume réutilisés, aucune nouvelle image.
Deux refus d’environnement avant tests conservés : point de montage puis
propriétaire du répertoire temporaire de rapports. Protections Docker
conservées. ARM64 et AMD64 réutilisés qualifiés ; construction fraîche
GitHub encore requise. Son job operations réussit avec 235 contrôles
autonomes, dont trois nouveaux contrôles du job réseau ; core/network
encore en cours au relevé. Pas de gate WAN ou production globale clos.
[AMD64](qualification/network-ci-20261001/amd64-prebuilt/README.md),
[contrôles GitHub](qualification/network-ci-20261001/github-operations/README.md).


### 2026-10-01 — couverture de régression C++ complète inventoriée

Registre natif configuré : 422 scénarios, 196 exécutables présents. La sélection
CI core de neuf noms laisse 413 scénarios hors de ce job ; le job transport
possède un registre distinct de 21 noms. Quinze scénarios sans TIMEOUT explicite
sont identifiés ; somme des budgets existants 68 215 s, pas une durée mesurée.
Nouveau outil vérifiable de couverture exacte : refuse registres vides/doublons,
propriétés de skip, sélections étrangères et budgets invalides. Quatre nouveaux
contrôles passent ; le gate complet refuse la sélection réelle de neuf tests
(exit 1). Aucun test runtime C++ rejoué et aucune qualification complète
revendiquée. Budgets manquants, régression complète du candidat et critères
originaux de production restent ouverts. CI fraîche existante non interrompue.
[Inventaire et preuves](qualification/regression-inventory-20261001/README.md).


### 2026-10-01 — délais CTest explicites et harness RPC corrigé

Quinze délais manquants ajoutés individuellement sans changer les budgets
existants. Docker ARM64 existant : 14 scénarios réussis et un échec `rpc`
(-32602), conservé. Diagnostic : le helper du test envoyait un objet au lieu
d’un tableau pour un paramètre JSON unique. Helper corrigé, validation runtime
inchangée, deux nouveaux contrôles de forme. Seul RPC modifié rejoué :
993 vérifications réussies (0,58 s), aucune erreur/skip ; erreur intermédiaire
de compilation également conservée. Binaires nano/p2p inchangés.
Registre Docker : 400 noms, tous bornés ; 22 noms natifs absents de cette
configuration sont explicitement recensés, pas qualifiés par Linux.
Régression complète, AMD64 et tous les critères originaux restent ouverts.
Le job core GitHub `36894479093` passe neuf tests, empreintes identiques au
run précédent ; job transport encore en cours au relevé.
[Preuves et limites](qualification/regression-budgets-20261001/README.md).


### 2026-10-01 — régression Docker complète engagée, défaut de portabilité identifié

Compilation de toutes les cibles, puis registre Docker complet de 400 noms
prévu en série, budgets individuels et garde exhaustive (noms, exécutables,
absence de skip). Premier build à `78fd880` arrêté exit 2 avant CTest : fixtures
utilisant la surcharge RocksDB Open(unique_ptr*) absente de Linux 8.9.1.
Échec conservé. Dix-huit appels dans huit fixtures sont adaptés via un helper
RAII utilisant l’API DB** commune ; code de stockage runtime inchangé.
Compilation complète corrigée en cours, aucune réussite globale revendiquée.
Les 22 registrations natives absentes restent un gate séparé.
[Premier refus et recette](qualification/full-regression-arm64-20261001/first-build-refused/README.md).


### 2026-10-01 — écart des registrations Linux expliqué

Les 22 omissions du registre Docker sont classées depuis CMake/cache :
13 scénarios d’erreur disque restent conditionnés à APPLE malgré l’injecteur
Linux existant ; 9 noms requièrent Node, Assertoor ou corpus interchange.
Deux branches Bellatrix d’erreur disque sont aussi réservées à __APPLE__.
Patch de raccordement portable préparé et git apply --check réussi, non appliqué
pendant le build complet gelé. Compilations et véritables injections EIO restent
requises avant toute qualification ; aucun compteur de test absent converti
en réussite. Critères originaux inchangés.
[Audit et patch non appliqué](qualification/linux-registration-audit-20261001/README.md).


### 2026-10-01 — compilation Linux complète réussie, 400 scénarios en cours

Les cibles par défaut compilent après réparation des appels RocksDB des
fixtures. Garde exacte : 400 noms, exécutables présents, délais positifs,
aucun skip. Empreintes nano/nano-p2p identiques au candidat ARM64 précédent.
Campagne des 400 scénarios en série démarrée dans le Docker existant ; journal
instantané archivé, pas de réussite terminale globale revendiquée.
Les 13 injections Linux non enregistrées et 9 gates outils/corpus restent
ouverts ; patch préparé non appliqué durant cette campagne.
[Compilation et snapshot](qualification/full-regression-arm64-20261001/build-complete-tests-running/README.md).


### 2026-10-01 — régression complète : défaut de paramètres RPC client identifié

Snapshot non terminal : events/validator/network/signed_network/bft_network
échouent avec -32602. La signature RpcClient::call(Json params) permet la copie
d’un objet JSON unique passé entre accolades ; le chemin CLI de validation
legacy appelle aussi validator_submitBlock de cette façon. Défaut réel du
client, serveur strict correct. Surcharge de liste construisant explicitement
un tableau et garde négative de non-mutation préparées ; git apply --check
passe, patch non appliqué pendant la campagne gelée. Collecter tous les
échecs, appliquer/corriger en une passe et qualifier le candidat modifié.
Aucun succès complet ni nouvelle qualification runtime revendiqué.
[Diagnostic et patch non appliqué](qualification/rpc-positional-client-audit-20261001/README.md).


### 2026-10-01 — correctif RPC qualifié séparément ; CI transport en échec conservé

Run GitHub `36894479093` completed/failure : dépendances/transport compilent,
20/21 tests passent, altair_core_dispatch échoue (concurrent ticket admission).
Core neuf tests et operations 235 passent. Ne pas transformer ce run en succès.
Checkout isolé 0836378, mêmes builder/volume, sous-répertoires distincts :
correctif RpcClient/std::initializer_list<Json> qualifié sur validator, network,
signed_network et bft_network, quatre réussites sans skip en 31,26 s. Garde
négative paramètres objet/état-pool-événements intacte ; première surcharge
Json::initializer_list_t insuffisante conservée en échec. Events corrigé passe
séparément (0,21 s), pas de nouveau run des cinq. Nouveau nano d5c40202...,
P2P inchangé ; candidat non fusionné et régression complète/AMD64 encore requis.
Fixture dispatcher attend un handler via run_one, assertions conservées :
scénario modifié passe (0,01 s), runtime dispatcher inchangé. Provenance CI
déplacée avant CTest ; nouvelle qualification fraîche requise. Campagne des
400 anciens scénarios poursuit la collecte, dont échecs de charge RPC distincts.
Tous les autres critères originaux restent ouverts.
[Transport en échec](qualification/network-ci-20261001/github-network-failure/README.md),
[correctifs isolés](qualification/rpc-positional-isolated-20261001/README.md).


### 2026-10-01 — nouvelles injections Linux en qualification isolée

Le checkout isolé 886eac8 reçoit le raccordement de treize tests de panne
fsync/fdatasync auparavant exclus du registre Linux. Configuration Docker :
413 noms, exactement treize ajouts et aucun retrait face au registre 400.
Les branches EIO Altair, témoin Beacon et Bellatrix sont également activées.
Compilation/exécution en cours, aucune réussite terminale revendiquée.
Les hooks restent réservés aux fixtures ; pas de liaison au binaire nano.
Le conteneur complet 400 continue séparément, avec ses échecs conservés :
paramètres RPC, refus/timeouts sous charge et gros bloc hors délai.
La CI operations bd25461 passe 239 tests sans skip ; ni ce résultat ni le
registre étendu ne clôturent les critères de production. Sources main gelées
pendant la campagne complète ; candidat RPC corrigé non déployé.
[Preuve CI exploitation](qualification/ci-20261001/operations-bd25461/README.md).


### 2026-10-01 — treize injections Linux manquantes qualifiées en isolation

Registre 400→413 : treize ajouts exacts, zéro retrait ; trois branches EIO
existantes renforcées (Altair, témoin Beacon, Bellatrix). Première exécution
15/16 réussites, exit 8 ; élagage arrêté par ENOSPC sur tmpfs 128 Mio avant
injection, échec conservé. Rejeu du seul élagage sur volume disque existant :
exit 0, 63.11 s, mêmes binaires/délai/assertions. Union vérifiée : seize noms
passent, sans skip. Les quatre dispositions primaire/témoin × suffixe
conservé/perdu sont vérifiées, votes signés préservés et retry idempotent.
Patch 97f4a19d dans checkout isolé seulement. Pas de nouvelle qualification
WAN, panne physique, release, fork EVM ou bridge ; régression complète main
encore active avec échecs ouverts. Tous les critères originaux restent requis.
[Preuves terminales et échec initial](qualification/linux-sync-registration-20261001/README.md).


### 2026-10-01 — diagnostics de capacité et échec RPC maintenu

Sur volume disque existant, exécutables originaux inchangés : gros blocs
Paris/Shanghai réussis en 22.68/22.50 s, installation relocalisée réussie.
/tmp de la campagne complète était noexec et limité à 128 Mio ; ces replays
ne suppriment pas ses échecs historiques ni ne qualifient la production.
Le RPC multi-nœud échoue aussi sur disque : préparation/verrou <1.5 ms,
mais estimations atteignent la limite de 5 s. Expérience de déplacement GMP
qualifiée cœur/RPC mais insuffisante sous charge, donc retirée. Aucun délai,
quorum ou règle EVM assoupli. Build isolé à reconstruire après retrait avant
réutilisation ; sources main et binaires originaux de campagne inchangés.
Les critères initiaux et toutes les exigences restantes demeurent ouverts.
[Diagnostics et expérience retirée](qualification/load-capacity-diagnostics-20261001/README.md).


### 2026-10-01 — admission RPC étrangère corrigée, qualification AMD64 en cours

Branche isolée 9861d94c : prélecture RLP du chainId avant débit du budget local,
sans aucune admission ni suppression de l'ECDSA complet. Test nouveau échoue
avant correction, puis passe : 384 enveloppes étrangères types 0/1/2 rejetées,
admission saine préservée, signatures locales toujours limitées et refill testé.
RPC et restart/charge legacy BFT passent ; profils Paris/Shanghai explicitement
ajoutés, 22.56/22.60 s. Quatre CLI passent aussi sur le nouveau nano c6197c3e….
Aucun quorum, délai ou gas modifié. Consensus Beacon Capella, gossip admission,
charge arbitraire de même chaîne, estimation RPC et régression complète restent
ouverts. Git fetch/worktree puis compilation Docker AMD64 du même candidat
lancés sur VPS, sans déploiement ; résultat terminal encore attendu.
Sources main et binaires originaux de la campagne 400 gelés et préservés.
[Preuves et échecs initiaux](qualification/rpc-admission-isolation-20261001/README.md).


### 2026-10-01 — prélecture RPC sans parcours récursif avant budget

Candidate isolée bf173958 : au plus quatorze en-têtes RLP inspectés ; chainId
extrait sans descendre/copier les charges imbriquées. Le nouveau cas échoue
avant correction, puis quatre tests ARM64 passent sans skip (46.98 s) : RPC,
isolation étrangère, restart/charge legacy BFT Paris et Shanghai. 512 rejets
étrangers, dont 128 imbriqués ; 63 high-s et 64 payloads imbriqués locaux
restent contrôlés par le budget puis le décodeur canonique. ECDSA complet,
quotas, délais, quorum et gas préservés. AMD64 de ce nouveau candidat encore
à qualifier ; campagne originale et ses échecs conservés. Ce périmètre ne
qualifie pas Beacon/WAN, gossip, charge arbitraire ou la production.
Tous les critères initiaux et la revue indépendante restent ouverts.
[Sources et preuves avant/après](qualification/rpc-shallow-preflight-20261001/README.md).


### 2026-10-01 — précédent candidat RPC portable, nouveau parcours AMD64 en cours

La qualification AMD64 9861d94c est terminale : huit noms exacts passent,
aucun skip, 60.76 s (RPC, admission étrangère, quatre CLI, deux scénarios
legacy BFT Paris/Shanghai). Ce résultat ne couvre pas la nouvelle prélecture
bf173958. Après arrêt du banc, checkout isolé actualisé par Git ; qualification
AMD64 du nouveau candidat lancée avec le même builder et volume, sans image
neuve ni déploiement. Sources main restent gelées pour la campagne 400.
CI 36900339735 terminale annulée : opérations réussies, cœur annulé, réseau
skipped ; aucun résultat complet CI déduit. Tous les échecs et huit exigences
originales restent ouverts, production non qualifiée.
[Preuves AMD64 terminales et périmètre](qualification/rpc-admission-isolation-20261001/amd64-9861d94c/README.md).


### 2026-10-01 — nouvelle prélecture RPC qualifiée sur AMD64

Candidate bf173958 : huit noms exacts réussis sans skip sur le VPS (59.90 s),
avec 512 rejets étrangers dont 128 profondément imbriqués, 63 high-s et
64 payloads imbriqués locaux limités, admission saine/refill/état inchangé.
Les quatre tests ARM64 du même correctif sont déjà qualifiés ; leurs quatre
CLI restent une qualification du candidat précédent. Aucun nouveau build
d'image ni déploiement ; récupération Git et compilation incrémentale.
Campagne complète originale active, sources main gelées ; estimation sous
charge, admission gossip/même chaîne, Beacon/WAN, matrice complète et tous
les critères originaux restent ouverts. Production non qualifiée.
[Preuves AMD64 du nouveau candidat](qualification/rpc-shallow-preflight-20261001/amd64/README.md).


### 2026-10-01 — assemblage après SIGKILL qualifié sur scratch disque

Deux tests originaux rejoués sans recompilation ni changement de délai :
Paris 30.71 s, Shanghai 27.34 s, aucun skip. Six frontières 1/20/40 fragments
vérifient absence de votes fantômes, sessions obsolètes refusées, redownload,
quorum, dix reçus/transferts exacts et audit après seconde réouverture.
Journal CTest séparé, volume disque ; les échecs originaux et l’erreur de
montage readonly du premier diagnostic restent conservés, sans cause unique
ENOSPC attribuée. Ces fixtures legacy BFT ne couvrent pas libp2p, Beacon/WAN
ou une panne physique. Tous les critères initiaux restent requis.
[Preuves et périmètre](qualification/bft-assembly-crash-disk-20261001/README.md).


### 2026-10-01 — budget gossip isolé et réservé atomiquement

Candidate ff53e4d1 : enveloppes étrangères exclues du budget local après
prélecture bornée ; éléments locaux/malformés toujours facturés, lot réservé
avant ECDSA/submit. Nouveau test échoue avant correction puis passe : 550
refus étrangers en lots, ordre mixte, refus sans écritures faute de capacité,
malformed limité et refill. Quatre tests ARM64 RPC/BFT passent sans skip
(46.13 s), puis trois parcours transport (27.94 s). Limites, quotas, délais,
quorum et gas inchangés. Onze contrôles AMD64 en cours sur le même candidat,
source transférée par Git ; main/source de campagne originale préservée.
Charge générale, estimation RPC, Beacon/WAN et huit critères initiaux restent
ouverts ; production non qualifiée.
[Sources et preuves avant/après](qualification/gossip-admission-isolation-20261001/README.md).


### 2026-10-01 — gossip portable et lanceur complet préparé

Candidate runtime ff53e4d1 : onze tests exacts AMD64 réussis sans skip
(88.65 s), mêmes contrôles de budget/mixte/atomicité/refill que les sept
ARM64. Preuves terminales conservées ; pas de qualification globale.
Lanceur complet ajouté sur branche isolée : scratch disque exécutable,
inventaire complet, empreintes avant tests et audit exact des résultats.
Quatre gardes Node et deux contrôles vrais CMake/CTest passent ; ce sont
les contrôles du lanceur. Node/Python manquent dans les builders actuels :
usage Docker du lanceur reste à préparer, aucune matrice complète nouvelle
revendiquée. Campagne originale encore active, sources main préservées.
Tous les huit critères initiaux et échecs restent ouverts.
[AMD64 terminal](qualification/gossip-admission-isolation-20261001/amd64/README.md) ;
[contrôles et prérequis du lanceur](qualification/full-runner-controls-20261001/README.md).


### 2026-10-01 — lanceur complet Docker sans outils supplémentaires

Candidate outils aacc56da : Docker compile/exécute dans le builder existant,
inventaire et audit sur hôte ; Node/Python ne sont plus requis dans l’image.
Sources propres/empreintes stables, scratch disque exécutable, tout le registre,
TIMEOUT/executables/skips vérifiés avant tests, exactitude des résultats après.
Contrôles Docker vrais success/failure/missing-timeout passent ; aucun test
Nano global déduit. 246 contrôles opérations réussis, dont sept nouveaux ;
EPERM loopback de l’essai sandbox conservé puis execution autorisée réussie.
Campagne originale encore active et échecs préservés ; nouveau full run après
fin réelle seulement. Tous les critères initiaux demeurent ouverts.
[Contrôles et usage du lanceur Docker](qualification/full-docker-runner-controls-20261001/README.md).


### 2026-10-01 — passage original terminé, candidat intégré

Campagne originale terminale exit 8 : 400 résultats exacts, 314 réussites,
86 échecs, aucun skip. 46 échecs avec trace directe de manque d’espace
sont distingués ; aucune cause disque déduite pour les autres. Exécutables
originaux vérifiés inchangés après fin, rapports/échecs conservés intégralement.
Après fin seulement, merge 23204db2 intègre les correctifs qualifiés et outils
(aacc56da, runtime ff53e4d1) ; sources runtime équivalentes au checkout candidat.
Pas de full candidat encore réussi ni déploiement. Nouveau passage complet
sur disque à lancer depuis checkout immuable, builders/volumes existants.
Tous les critères initiaux, problèmes de charge et revue indépendante ouverts.
[Rapport original terminal](qualification/full-regression-original-20261001/README.md).


### 2026-10-01 — nouveau passage complet ARM64 et CI réellement actifs

Après intégration/revue et publication main bef03980, Docker prepare du
candidat immuable aacc56da est vivant : exec 62528, container
nano-full-rpc-positional-client-20261001-prepare, scratch disque, volume et
builder existants. Sources pertinentes identiques à main ; seule documentation
diffère. Tous les targets compilés puis registre configuré entier contrôlé
avant CTest, aucun résultat terminal candidat encore acquis. Logs Mac :
/tmp/nano-full-candidate-arm64-20261001/prepare.log puis tests.log.
CI GitHub 36908337763 sur bef03980 est in_progress ; aucun vert final déduit.
VPS main récupéré par Git, service/image déployés inchangés. Rapport original
400 terminal (314/86/0 skip), tous échecs et huit critères initiaux préservés.
Production non qualifiée.


### 2026-10-01 — registre candidat entier prêt, CTest actif

Préparation ARM64 aacc56da réussie : 416/416 tests sélectionnés, aucun exécutable
manquant, TIMEOUT explicite partout, aucun skip property. 195 exécutables
mesurés ; manifests sources avant/après build et binaires avant tests identiques.
Container nano-full-rpc-positional-client-20261001-test vivant (exec 62528),
aucun résultat terminal global déduit. Outils/corpus optionnels absents restent
distincts. CI opérations bef03980 réussie : 246 contrôles, zéro skip ; volets
cœur/réseau encore actifs. Original 314/86 conservé, tous critères initiaux ouverts.
[Préparation candidat](qualification/full-candidate-20261001/launch/README.md) ;
[opérations CI](qualification/ci-20261001/operations-bef03980/README.md).

### 2026-10-01 — Estimation profonde : gain mesuré, charge encore en échec

L'expérience isolée conserve toutes ses phases dans `docs/qualification/estimator-experiment-20261001/`. Sur une fixture de quatre CALL imbriqués, les lectures d'horloge de l'estimation passent de 5689 à 2013 (Paris, comparatif direct) ; le devis est vérifié par exécution et respecte la précision de 1,5 %. Les deux scénarios de charge 16 clients/quatre nœuds échouent encore avec expirations de file et d'exécution, malgré un marqueur finalisé pendant la charge. Les limites RPC restent inchangées ; cet écart de production reste ouvert.

La phase supplémentaire pile GMP passe 3741 contrôles cœur, 1007 RPC et le corpus enregistré (5387 correspondances au total (4882 exécutions et 505 validations)), avec nouveaux contrôles de normalisation/DUP/gas/frontière de pile. Aucun succès de charge n'est revendiqué pour cette phase, ni conformité intégrale des forks ; les profils réseau activés ne changent pas. La branche expérimentale n'est pas intégrée à main. CI core bef03980 : neuf tests réussis sans skipped, journal archivé séparément ; CI réseau et régression complète restent à terminer. Le défaut distinct d'admission brut révélé par le test HTTP est traité dans une autre copie isolée.

### 2026-10-01 — Budget brut malformé corrigé ; CI bef03980 terminale

La régression complète aacc56da révèle un défaut réel d'admission HTTP : le rejet d'hex/RLP inconnu précédait la charge du budget local. Correction ciblée : les enveloppes malformées partagent désormais le budget d'admission ; seuls les chain IDs étrangers effectivement extraits passent sans le consommer. Nouveaux contrôles de burst 128, exhaustion, enveloppe étrangère à exhaustion, état inchangé et refill. Trois tests ARM64 (RPC, isolation admission, HTTP/WebSocket) passent ; preuve dans `docs/qualification/raw-malformed-admission-20261001/`. Aucune limite de gas, file ou consensus n'est relâchée.

CI 36908337763 sur bef03980 : tous les jobs terminaux réussis, core 9 tests, transport réseau 21 tests, opérations 246 contrôles. Le corpus CI n'est pas la régression complète 416 ; cette dernière continue sur son SHA figé et garde ses échecs. La nouvelle correction requiert sa propre observation CI/AMD64 ; pas de claim de production ni mise à jour des services publics.

### 2026-10-01 — Réduction d'allocations EVM, qualification de charge distincte

Branche expérimentale : PUSH sans copie intermédiaire des octets (4746cfba), puis ADD/MUL/SUB avec stockage GMP réutilisé (63d7c41f). La nouvelle couverture des 32 largeurs et fins de bytecode, signes/produits/DUP/modulo, gas et frontière de pile passe avec 3869 contrôles cœur, 1007 RPC et le corpus enregistré (5387 correspondances au total (4882 exécutions et 505 validations)). Aucune règle EIP/gas, deadline ou capacité RPC modifiée ; pas de preuve de conformité complète de fork. Les fixtures aux forks futurs n'activent pas ces forks sur le réseau.

Charge ARM64 de la dernière révision lancée, mais non encore qualifiée. La campagne AMD64 distincte reste sur 8a8b080e obtenu par Git ; préparation initiale interrompue avant tests faute de permissions de rapports, conservée, puis reprise dans un sous-dossier du builder avec processus vivant. Ne pas fusionner ces scopes, ni confondre réduction d'allocations et gate de charge fermé. Archives : `docs/qualification/estimator-experiment-20261001/`. CI caeb8d17 en cours ; régression complète aacc56da toujours figée et ses échecs conservés.

### 2026-10-01 — Charges ARM64 réussies, écart BN128 AMD64 isolé et corrigé à froid

Sur 63d7c41f, union de huit scénarios ARM64 Paris/Shanghai many, WebSocket, overload et default-overload réussie (2 + 6 tests, mêmes binaires, limites inchangées). Chaque charge many termine 16×24 tours sans erreur, avec finalité pendant l'activité des 16 clients. Ceci qualifie ce périmètre local, pas les autres gates de production.

AMD64 8a8b080e : quatre échecs sur cinq tests ; cœur BN128, deux charges et trois mismatches de corpus (gas/stateRoot). Un processus crypto neuf isole `bn:initPairing` : 14 vecteurs échouent sur le Haswell OVH dépourvu d'ADX, contre 14 réussis ARM64. Le backend MCL optimisé exige ADX. Une variante LLVM portable sans ADX/JIT, conservant tous les contrôles mathématiques, passe ensuite les 14 vecteurs sur les deux architectures. Les échecs originaux sont conservés, les trois mismatches restent à requalifier dans l'EVM complète. EIP-197 : implémenté, qualification production AMD64 encore partielle ; ce succès à froid ne ferme pas le gate.

Candidate cohérente ea9667f6 (admission corrigée + estimation/allocations + backend portable) publiée séparément et figée pour 14 tests ARM64/AMD64 ; aucune promotion ni mise à jour du service. Régression complète aacc56da toujours active, échecs conservés. Preuves : `docs/qualification/estimator-experiment-20261001/` et `docs/qualification/bn-pairing-portability-20261001/`. Les profils réseau activés et les huit critères finaux restent inchangés.

### 2026-10-01 — CI caeb8d17 terminée et portée vérifiée

Les trois suites CI sont terminales : core 9/9, réseau 21/21, exploitation 246/246, zéro échec/ignoré. Les sélections core/réseau correspondent exactement aux cas XML et ne contiennent aucun doublon. La révision couvre l’admission malformée corrigée ; elle ne couvre pas les nouvelles optimisations EVM ni le backend MCL portable du candidat ea9667f6. Régression complète, consensus WAN, gates de production et échecs historiques restent ouverts. [Preuves](qualification/ci-20261001/terminal-caeb8d17/README.md).

### 2026-10-01 — Candidat portable ARM64 et nouveau parcours volumineux

Candidat ea9667f6 : 14/14 tests ARM64, zéro ignoré, sélection/XML exacts et empreintes exécutables avant/après identiques ; AMD64 reste en cours. Nouvelle couverture locale b39a0122 : blocs de 18 Mo finalisés avec journal réel, annoncés par référence puis importés en 40 fragments avec racine d’état identique, Paris/Shanghai réussis. Les refus des anciennes routes agrégées et la limite de 16 Mo sont conservés. Première préparation sans Python conservée ; aucune preuve P2P distribuée ou production ajoutée. Les échecs historiques et tous les gates ouverts demeurent. [Preuves](qualification/evm-rpc-portable-20261001/README.md).

### 2026-10-01 — Parcours volumineux AMD64 et conversion entière

Le nouveau parcours volumineux b39a0122 réussit également sur AMD64 (Paris/Shanghai, 40 fragments, état exact audité). Le candidat ea9667f6 conserve des échecs de charge RPC AMD64 : expiration de file et timeout d’exécution, campagne toujours en cours ; délais non élargis. Nouvelle correction isolée 6e5c1400 : contrôle de largeur uint64 sans allocation répétée du maximum, core ARM64 4 005 checks / RPC 1 007 / EEST local sans divergence. Microbenchmark de conversion amélioré, sans conclusion de débit réseau ; reconstruction release complète et AMD64 encore requis. Les échecs de préparation sont conservés, ainsi que tous les gates de production. [Conversion](qualification/u64-boundary-20261001/README.md), [transport local](qualification/evm-rpc-portable-20261001/README.md).

### 2026-10-01 — AMD64 terminal et nouvelle correction EVM ciblée

Candidat ea9667f6 AMD64 terminal : 6/14 réussis, huit échecs RPC de charge, zéro ignoré, empreintes identiques. BN128 et sous-ensemble EEST réussissent (5 387 correspondances au total (4 882 exécutions et 505 validations), zéro divergence) ; les trois anciennes divergences BN128 ne se reproduisent pas dans ce périmètre. La charge AMD64 demeure ouverte. Nouvelle révision isolée 98868b93 : ISZERO/JUMP évitent des temporaires, 20 nouveaux contrôles, cœur ARM64 4 025/RPC 1 007/EEST local passent ; même gas/résultat/racine sur le contrat de 600 000 itérations, durée médiane réduite de 17,6 %. Ceci ne prouve pas les délais RPC. Reconstructions CMake cohérentes et campagnes de 20 tests lancées sur les deux architectures ; aucune clôture de production ni promotion du code expérimental. [AMD64 terminal](qualification/evm-rpc-portable-20261001/README.md), [nouvelle couverture](qualification/evm-boolean-jump-20261001/README.md).

### 2026-10-01 — Pile EVM réutilisée et nouveaux contrôles de profondeur

Révision isolée 082143a2 : emplacements GMP bornés à 1024, profondeur active distincte, 44 nouveaux contrôles (tous DUP/SWAP, réutilisation, alias et limites). Version finale ARM64 : core 4 069/RPC 1 007/EEST local sans divergence ; sources et bibliothèques inchangées pendant qualification. Compte de contrat présent et SSTORE vérifié : même gas/retour/racine non vide, médiane 92,262→43,058 ms par rapport à 98868b93. La première mesure à racine vide et les étapes préliminaires sont conservées comme preuves limitées. Les campagnes CMake actives restent sur 98868b93 ; ce gain local ne ferme pas la charge AMD64, une reconstruction release ou la production. [Sources, preuves et limites](qualification/evm-stack-slots-20261001/README.md).

### 2026-10-01 — Reconstruction CMake 98868b93 : 20/20 ARM64

Campagne ARM64 terminale : 20 tests exactement sélectionnés, tous réussis, zéro ignoré/doublon, exit 0 ; onze empreintes exécutables avant/après identiques. Les six parcours volumineux locaux/libp2p (dont pertes de fragment/ACK) passent avec les huit charges RPC et le socle cœur/RPC/crypto/EEST local. Il ne s’agit pas d’une proposition volumineuse distribuée ni d’une qualification Beacon WAN. AMD64 reste en cours avec au moins un échec de charge. Nouveau candidat 082143a2 : reconstruction CMake ARM64 démarrée dans un répertoire distinct ; préparation par Git AMD64 terminée, démarrage prévu après le handle Docker parent confirmé actif, sans charge concurrente ajoutée. Aucune promotion du code, gate globale ou historique fermé. [Preuves terminales](qualification/evm-boolean-jump-20261001/full-cmake-arm64/README.md).

### Correction du décompte EEST et campagnes CMake — 2026-10-01

Erratum : `matched` inclut `validationMatched`. Le sous-ensemble enregistré comporte **5 387 correspondances totales, dont 4 882 exécutions et 505 validations**, pas 5 387 exécutions supplémentaires. Les textes ci-dessus sont rectifiés ; les journaux bruts et échecs historiques sont conservés. Le rapporteur ajoute `executionMatched`, avec vérificateur indépendant et onze nouveaux contrôles ; deux cas C++ représentatifs et une sélection vide vérifient le comportement. [Décompte et preuves](qualification/eest-counts-20261001/README.md).

Le candidat de pile 082143a2 termine sa reconstruction CMake ARM64 avec 20/20 réussites, zéro ignoré et onze empreintes exécutables inchangées. Le candidat précédent 98868b93 termine AMD64 avec 12/20 réussites et huit échecs de charge RPC ; les campagnes ne sont pas interchangeables. Le nouveau candidat AMD64 est en compilation dans Docker après la terminaison vérifiée de son prédécesseur. [Pile ARM64](qualification/evm-stack-slots-20261001/README.md), [échecs AMD64 conservés](qualification/evm-boolean-jump-20261001/README.md). Aucune intégration des optimisations moteur sur main ni clôture des critères globaux de production.

### Compilation cryptographique GNU — 2026-10-01

Nouvelle couverture du candidat figé 082143a2 : le backend BN128 portable construit avec GNU C++ 13.3.0 (Clang pour les IR LLVM uniquement) passe ses 14 vecteurs à froid dans Docker Linux ARM64, sans test ignoré et avec empreintes inchangées. Cela réduit l’écart de toolchain précédemment explicitement non qualifié ; la variante AMD64 est mise en attente derrière la campagne RPC afin de préserver les mesures. Un nœud complet GCC n’est pas qualifié. [Preuves de compilation](qualification/bn-pairing-portability-20261001/README.md).

### Intégrité des campagnes complètes après échec — 2026-10-01

Les scripts de qualification capturent et comparent désormais les binaires après CTest, contrôlent les sources, et conservent tous les audits même lorsqu’un test échoue. Nouveau vérificateur EEST sur `LastTest.log` complet ; onze nouveaux contrôles (dont mutations de source/binaire et exit CTest 8) passent, 22 avec les contrôles précédents. Aucun moteur modifié ni campagne complète rejouée ; la campagne aacc56da en cours reste figée sur son script d’origine. [Correction et limites](qualification/full-postrun-audit-20261001/README.md).

### CI terminale et candidat d’intégration préparé — 2026-10-01

CI f62236a4 : 257 contrôles opérations et neuf tests C++ sélectionnés passent sans ignoré ; CI 47ba41a1 : 268 contrôles opérations passent sans ignoré. Les jobs non demandés ne sont pas des preuves réseau ou C++. [Archives exactes](qualification/ci-20261001/terminal-f62236a4-and-47ba41a1/README.md).

Branche isolée c10426b2 : onze fichiers moteur/test/CMake identiques au candidat 082143a2, rapporteur EEST corrigé conservé. Régression complète ARM64 préparée conditionnellement à la réussite terminale des 20 tests AMD64 et à la fin contrôlée de l’ancienne campagne 416. Aucun moteur promu sur main ni régression intégrée complète revendiquée. [Préparation et empreintes](qualification/production-evm-integration-20261001/README.md). Tous les critères originaux restent ouverts.

### Régression complète précédente terminée — 2026-10-01

Révision aacc56da Docker ARM64 : **405/416 réussites, onze échecs identifiés, zéro ignoré**, exit CTest 8. Aucun nouvel échec : admission HTTP, huit scénarios RPC de charge, deux anciennes attentes de trame volumineuse. Observation indépendante après terminaison : 196 empreintes exécutables et sources suivies inchangées ; cette observation ne vient pas rétroactivement du script d’origine. EEST : 4 882 exécutions et 505 validations, total 5 387. Journaux bruts conservés sous compression avec empreintes. [Campagne terminale et limites](qualification/full-regression-aacc56da-20261001/README.md). Les réparations/candidats plus récents restent qualifiés séparément, sans effacer cet échec complet ni fermer les gates globaux.

### Candidat moteur portable qualifié et régression intégrée lancée — 2026-10-01

082143a2 termine **20/20 tests AMD64**, zéro ignoré, onze empreintes exécutables inchangées, exit 0 sans OOM. Les huit scénarios RPC de charge passent en Paris/Shanghai avec délais et capacités inchangés. EEST : 4 882 exécutions + 505 validations, total 5 387 sur 317 fichiers. Le même périmètre ARM64 avait déjà passé 20/20. [Preuves et limites](qualification/evm-stack-slots-20261001/README.md). Les 14 vecteurs BN128 GNU/LLVM passent aussi sur AMD64, après la charge RPC ; un nœud GNU complet n’est pas qualifié. [Toolchains](qualification/bn-pairing-portability-20261001/README.md).

Les préconditions exactes sont satisfaites et la reconstruction/régression complète du candidat intégré c10426b2 démarre sur Docker ARM64 et AMD64, sans nouvelle image ni nouveau serveur. Les audits VPS sont coordonnés depuis le Mac car Node n’est pas installé sur l’hôte VPS. [Candidat et provenance](qualification/production-evm-integration-20261001/README.md). Moteur encore sur branche isolée ; aucun résultat complet intégré ni gate global de production déclaré acquis.

### Assemblage depuis le répertoire qualifié — 2026-10-01

Le script de livraison sélectionne désormais le sous-répertoire du build qualifié sans écraser les anciens exécutables ; six arguments conservent le chemin historique. Huit nouveaux contrôles passent, ainsi que deux exports réels Docker sur fichiers factices : trois fichiers réguliers attendus et lien symbolique refusé. Aucun nouveau runtime assemblé ou service déployé. Le README du staking historique est rectifié (plafond 1 000 déjà supprimé du contrat, pagination existante, distinction Beacon). [Preuves et limites](qualification/candidate-build-path-20261001/README.md). Les campagnes intégrées c10426b2 restent figées ; aucune clôture de gate global.

### 2026-10-01 — Préparation WAN du candidat compilé en isolation

Le banc multihost accepte `NANO_TEST_BUILD_SUBDIRECTORY` en mode volume
pour lire Nano, le marqueur et l'auditeur de rotation depuis le build isolé,
y compris pour le backup/audit. Le volume reste en lecture seule ; aucun
binaire historique n'est écrasé. Quatre nouveaux contrôles et les contrôles
d'images passent (8/8 ciblés), la suite opérationnelle locale passe 280/280.
Preuves : `docs/qualification/wan-build-paths-20261001/`. Cela prépare la
qualification réseau du même candidat après les régressions ; aucune campagne
WAN n'est lancée ni déclarée réussie ici, aucun gate de production clos.

### 2026-10-02 — CI opérationnelle terminale ae3d5e06

Le run 36927821703 confirme 280/280 contrôles opérationnels, zéro échec,
annulation ou test ignoré. Core et network sont skipped par le filtre ;
aucune qualification moteur/réseau supplémentaire n'en découle.
[Preuves brutes et empreintes](qualification/ci-20261001/terminal-ae3d5e06/README.md).
Les régressions Docker c10426b2 et les échecs RPC restent distincts et ouverts.

### 2026-10-02 — Échecs ouverts du candidat intégré, campagnes en cours

La campagne ARM64 c10426b2 révèle six échecs de durée RPC et un échec
`consensus_bellatrix_voluntary_exit_pruning` : accès à l'exécution refusé avant
vérification du checkpoint. Le saut d'une barrière de synchronisation du banc
est une hypothèse, pas une cause établie.
[Observation partielle conservée](qualification/production-evm-integration-20261001/in-progress-failures-20261002.json).
Les campagnes ARM64/AMD64 restent actives et figées ; le bilan terminal,
les audits de sources/binaires et les résultats AMD64 restent requis.
La branche `qualification/bellatrix-failure-diagnostics-20261002` prépare la
conservation de la première erreur Bellatrix et son exposition dans le banc :
contrôles syntaxiques ARM64 seulement, aucune exécution qualifiée ni intégration
du moteur dans main. Aucun gate de production, profil réseau ou statut EIP clos.

### 2026-10-02 — Régression intégrée ARM64 terminale : 410/417, sept échecs

Candidat isolé c10426b2 : 417 résultats sur 417 enregistrés, 410 réussites,
sept échecs, zéro ignoré ; CTest exit 8 conservé, durée 6 959,14 s.
197 empreintes exécutables et manifestes de sources inchangés avant/après.
EEST : 4 882 exécutions + 505 validations, 5 387 résultats sur 317 fichiers ;
conformité complète de fork non prouvée. Audit global exit 1 conservé :
les sept tests échoués sont bloquants, les autres audits ont réussi.
[Preuves terminales et empreintes](qualification/production-evm-integration-20261001/full-arm64-terminal/README.md).
Les six échecs de durée RPC et l'élagage des sorties Bellatrix restent à
corriger/qualifier ; le moteur n'est pas promu dans main. Campagne AMD64
encore active ; nouvelles qualifications réseau et exigences de production
inchangées. Les observations partielles précédentes restent historiques.

### 2026-10-02 — Correctif RPC qualifié sur une sélection ARM64 ; cause Bellatrix révélée

La révision isolée a00ff7d0 a exécuté les 16 cas enregistrés : 15 réussites,
un échec, zéro ignoré, CTest exit 8. Les six scénarios de durée RPC auparavant
échoués passent, avec les contrôles RPC réussis/imbriqués, le cœur, la fenêtre
de charge et le rejet des discontinuités d'horloge. Sources et exécutables
inchangés avant/après. Ce résultat ne remplace pas la régression intégrale
c10426b2 ni une qualification AMD64/WAN du correctif.
[Résultats et diagnostic conservés](qualification/bellatrix-pruning-clock-barrier-20261002/README.md).
L'observateur Bellatrix échoue sur un saut simulé de 12 000 ms avec une limite
de 6 000 ms : le scénario omet la synchronisation du slot d'inclusion de la
sortie. Le correctif b0eb5ba1 ajoute cette barrière ; sa validation d'exécution
est en cours, sans augmentation de la tolérance ni changement des règles.
Les matrices EIP et tous les critères de production restent inchangés.
Le moteur et les correctifs demeurent sur leurs branches de qualification.

### 2026-10-02 — Barrière d'horloge Bellatrix qualifiée sur cinq cas ARM64

Le correctif b0eb5ba1, testé dans la source propre 1ac57e25, passe les cinq
scénarios enregistrés : sortie locale, propagation P2P locale, réorganisation,
élagage après finalité et garde d'horloge. CTest exit 0, zéro ignoré, 31,69 s ;
sources et empreintes exécutables inchangées, audits de résultats/inventaire
réussis. La limite d'un slot est conservée ; seule la synchronisation du
scénario à l'inclusion de la sortie est ajoutée.
[Preuves du correctif ARM64](qualification/bellatrix-pruning-clock-barrier-20261002/barrier-arm64/README.md).
L'échec initial reste archivé. Les six cas RPC et ce correctif ont désormais
une réussite ciblée ARM64, mais la qualification AMD64 et la régression du
candidat final cohérent restent ouvertes, ainsi que les exigences WAN et de
production. Aucun moteur/correctif n'est encore promu dans main et aucun
statut global EIP n'est relevé sur la seule base de ces résultats.

### 2026-10-02 — Deux échecs AMD64 supplémentaires restent ouverts

La campagne complète c10426b2 est encore active. Outre les deux cas RPC
soutenus, elle observe un échec consensus_altair_libp2p au payload maximal et
un timeout consensus_bellatrix_partition_simulation à 240,03 s. Cette dernière
trace atteint une finalité commune 9 au slot 88, sans finir les 14 époques
prévues : ni récupération complète qualifiée, ni blocage de protocole démontré.
[Observation partielle et diagnostic](qualification/altair-bound-diagnostics-20261002/README.md).
La branche de diagnostic f92533e9 conserve réponse, tailles, admission,
checkpoint et huit raisons de fin de flux au maximum. Syntaxe ARM64 Release
validée, exécution AMD64 en attente ; aucun seuil relevé ni règle assouplie.
Les 19 cas du correctif RPC/Bellatrix sur 1ac57e25 attendent la fin du conteneur
initial et ses résultats/empreintes avant compilation. Ils ne couvrent pas ces
deux nouveaux échecs. Régression terminale, candidat cohérent, gates WAN et
production globale restent ouverts ; matrices EIP inchangées.

### 2026-10-02 — Manifeste des programmes auxiliaires renforcé

L'ancien manifeste ARM64 c10426b2 vérifie 197 empreintes avant/après, mais
omet nano, nano-altair-probe et nano-finality-crash-fixture fournis en arguments.
Le nouveau collecteur les inclut et permet de déclarer explicitement nano ;
cinq contrôles macOS passent, et l'inspection Docker Linux en lecture seule
retrouve les 197 empreintes inchangées plus ces trois programmes. Les fichiers
de données marqués exécutables sont distingués par leur en-tête.
[Preuves et limites du collecteur](qualification/executable-arguments-manifest-20261002/README.md).
Aucune intégrité avant/après des auxiliaires anciens n'est prouvée rétroactivement.
Les campagnes en cours gardent leurs sources figées ; leur périmètre historique
reste exact. Les prochains runs complets utiliseront le manifeste renforcé.
Il ne découvre pas toutes les bibliothèques ou les programmes dont le chemin
est incorporé dans le code compilé. Gates de runtime/production et matrices EIP
inchangées ; aucune réussite globale déduite de ces contrôles d'outillage.

### 2026-10-02 — CI du collecteur terminale, 285/285 contrôles

Run GitHub 36941560281, source daab685d : opérations 285/285 réussies,
zéro échec/annulation/ignoré, incluant les cinq nouveaux contrôles du manifeste.
Révision de l'artifact et du run concordantes ; rapports bruts et empreintes
[archivés](qualification/ci-20261002/terminal-daab685d/README.md).
Jobs cœur/réseau non demandés par le filtre et ignorés : aucune qualification
du runtime, d'un fork ou de la production déduite. La régression AMD64 c10426b2
poursuit ses cas Capella avec ses sources figées et son périmètre de manifeste
historique ; les quatre échecs connus et les critères originaux restent ouverts.

### 2026-10-02 — Diagnostic Altair AMD64 en attente, moteur c104 conservé

Le contrôle [archivé](qualification/altair-bound-diagnostics-20261002/c104-runtime-control/README.md)
attend la fin de la campagne complète puis des 19 cas correctifs, avec contrôle
des résultats et empreintes. Il ne recompilera que le test instrumenté, lié aux
bibliothèques c104 existantes, volume de build en lecture seule. Révisions du
runtime, du test et du collecteur distinctes ; composition diagnostique, pas
candidate de production. Syntaxe shell vérifiée ; compilation/exécution encore
non réalisées, timeout et assertions d'origine conservés. Aucun échec clôturé,
aucun changement de statut EIP ni des critères de production.

### 2026-10-02 — Profilage du timeout de partition Bellatrix préparé

[Contrôle diagnostique](qualification/bellatrix-partition-profile-20261002/README.md)
lié au runtime c104 d'origine : temps cumulés de l'horloge, des duties et des
admissions/relais, compteurs de messages ; mesures imbriquées non additives.
Syntaxe ARM64 Release avec définitions d'origine vérifiée. Driver VPS observé
en attente du diagnostic Altair ; compilation liée/exécution encore à faire.
112 slots, limite de 240 s et assertions conservés. Bibliothèques/volume en
lecture seule, pas de rebuild du cœur ni nouvelle image. Le timeout historique,
les exigences de finalité/WAN et tous les statuts EIP restent ouverts.

### 2026-10-02 — Cinquième échec AMD64 : activation Capella hors délai

Cas #394 `consensus_capella_deposit_activation` : timeout 360,03 s sous la
limite originale de 360 s. [Traces conservées](qualification/production-evm-integration-20261001/capella-activation-amd64-partial/README.md) :
dépôt consommé (index 1), progression jusqu'au slot 120 et finalité époque 13 ;
assertions de fin d'activation/vote non démontrées. 394/417 résultats observés,
389 réussis et cinq échecs/timeouts. Le cas de reprise #395 reste en cours.
L'ARM64 c104 avait réussi ce cas en 136,363 s ; cause non identifiée, performance
à analyser. Les 19 cas correctifs déjà prévus ne couvrent pas ce nouvel échec.
Aucun délai/critère relâché, aucun échec clôturé ni profil EIP requalifié.

### 2026-10-02 — Sixième échec AMD64 : reprise d'activation Capella

Cas #395 `consensus_capella_deposit_activation_restart` : timeout 360,03 s,
limite originale de 360 s conservée. [Observation complémentaire](qualification/production-evm-integration-20261001/capella-activation-amd64-partial/README.md) :
réouverture « scheduled-pending » observée au slot 104, puis finalité époque 13
au slot 120 ; reprise active/vote et refus du conflit de signature non qualifiés.
ARM64 c104 : 139,204 s réussi, sans cause AMD64 identifiée. Le cas #396
`automatic_registration` a réussi en 29,71 s : 396 résultats, 390 réussis,
six échecs/timeouts. #397 reste en cours. Les contrôles correctifs déjà en attente
ne couvrent pas ces deux timeouts Capella ; aucun statut EIP/production clôturé.

### 2026-10-02 — Septième échec AMD64 : dépôt/enregistrement automatique

Cas #397 `consensus_capella_deposit_auto_registration` : timeout 360,03 s
sous la limite originale de 360 s. [Traces complémentaires](qualification/production-evm-integration-20261001/capella-activation-amd64-partial/README.md) :
index de dépôt 1, slot 120 et finalité époque 13, sans qualification des
assertions terminales d'activation/vote. L'enregistrement seul #396 réussi ne
couvre pas ce scénario complet. ARM64 c104 : 137,396 s réussi ; cause AMD64
non identifiée. 397 résultats observés, 390 réussis et sept échecs/timeouts.
Le cutoff commun des trois cas est une observation, pas une cause démontrée.
Aucune limite relâchée, aucun critère EIP ou production clôturé.

### 2026-10-02 — Huitième échec AMD64 : activation après réindexation

Cas #406 `consensus_capella_reindexed_validator_activation` : timeout 360,02 s
sous la limite originale de 360 s. [Traces complémentaires](qualification/production-evm-integration-20261001/capella-activation-amd64-partial/README.md) :
rejet de l’index périmé observé, activation époque 13, puis un vote du nouveau
validateur au slot 112 et deux au slot 120, finalité époque 13. Ces observations
ne démontrent pas les assertions terminales du scénario. ARM64 c104 : 162,612 s
réussi ; cause AMD64 non identifiée. 406 résultats, 398 réussis et huit
échecs/timeouts. Les diagnostics déjà en attente ne couvrent pas ce cas Capella.
Aucune limite relâchée ; qualifications complètes et production restent ouvertes.

### 2026-10-02 — Neuvième échec AMD64 : sortie après réindexation

Cas #413 `consensus_capella_exit_reindex` : timeout 480,04 s sous la limite
originale de 480 s. [Traces complémentaires](qualification/production-evm-integration-20261001/capella-activation-amd64-partial/README.md) :
activation époque 13, cinq votes du nouveau validateur au slot 144, finalité
époque 16. Ces progrès ne démontrent pas les assertions terminales de sortie
après réindexation. ARM64 c104 : 162,259 s réussi ; cause AMD64 non identifiée.
413 résultats, 404 réussis et neuf échecs/timeouts. Les contrôles correctifs déjà
en attente ne couvrent pas ce cas Capella. Aucun timeout ni critère assoupli ;
aucun statut EIP ou production clôturé.

### 2026-10-02 — Régression intégrée AMD64 terminale : 408/417

[Archive terminale](qualification/production-evm-integration-20261001/full-amd64-terminal/README.md) :
417 résultats complets, 408 réussites, neuf échecs, zéro ignoré ; CTest exit 8,
15 101,53 s. Sources, registre et 197 programmes historiques inchangés.
Audit résultats exit 1 à cause des neuf échecs ; audit EEST réussi :
4 882 exécutions + 505 validations sur 317 fichiers, sans conformité de fork déduite.
Les omissions historiques de trois helpers restent explicites, sans preuve
rétroactive inventée. ARM64 terminal 410/417 reste non réussi également.
Aucun échec effacé, aucun moteur promu, aucune qualification production clôturée.

### 2026-10-02 — Contrôle correctif AMD64 : panne de préparation conservée

[Échec et reprise des pilotes](qualification/corrective-amd64-control-20261002/permission-retry/README.md) :
le dossier de rapports non inscriptible a arrêté le contrôle avant toute
compilation ou test. Essai et erreur conservés, sources inchangées ; les
pilotes suivants se sont arrêtés à leurs barrières de rapports manquants.
Correction limitée aux permissions du nouveau dossier de sortie dédié.
Compilation corrective reprise ; Altair puis Bellatrix restent séquentiels,
sans contournement des audits, changement de runtime ou relaxation des délais.
Aucun résultat correctif encore qualifié ; production toujours ouverte.

### 2026-10-02 — Correctifs ciblés AMD64 : 19/19 réussis

[Contrôle terminal](qualification/corrective-amd64-control-20261002/terminal/README.md) :
source runtime 1ac57e25, 19 tests exacts réussis, zéro ignoré, 135,33 s ;
CTest/conteneur/audit/manifeste post-test exit 0. Sources et sept programmes
inchangés, dont `nano` explicite. Le manifeste renforcé daab685d est distinct
du runtime. Fenêtres RPC Paris/Shanghai et barrière d’horloge des sorties
Bellatrix qualifiées dans ce périmètre ; aucun critère ni délai assoupli.
Les neuf échecs de la campagne complète c104 restent conservés. Altair,
partition Bellatrix et cinq cas Capella exclus de cette sélection restent
ouverts ; pas de promotion moteur ni de qualification production globale.

### 2026-10-02 — Diagnostic Altair AMD64 : refus reproduit, cause encore ouverte

[Diagnostic terminal](qualification/altair-bound-diagnostics-20261002/c104-runtime-control/terminal/README.md) :
un cas original échoué en 2,17 s, limite 120 s inchangée. Composition de diagnostic
c104 + instrumentation du test f92533e9, sans modification du cœur. Payload
8 388 608 octets, frame 16 777 370 octets, réponse `[null]`, highWater inchangé,
checkpoint inchangé et horloge saine. Les traces récentes ne désignent pas encore
le défaut précis du transport ou de l’écriture du pair de qualification.
Sources, programmes et entrées de lien inchangés ; audit garde l’échec.
Aucune correction de protocole ni qualification production déduite de ce seul refus.

### 2026-10-02 — Profil partition Bellatrix : coût des admissions, timeout conservé

[Mesures terminales](qualification/bellatrix-partition-profile-20261002/terminal/README.md) :
timeout 240,03 s sous limite originale 240 s, scénario 112 slots inchangé.
Dernière observation slot 88 : têtes égales, finalité 9, trois époques consécutives ;
assertions terminales non atteintes. Relais 69,806 s, dont admissions bloc/vote
69,726 s incluses (99,885 %) ; reste environ 0,080 s. Les nombreux scans ne
prouvent donc pas un coût dominant. Polls validateurs 128,777 s, horloges 32,729 s.
Temps imbriqués non additionnés deux fois. Sources/programmes/entrées de lien
inchangés ; aucune qualification de candidat ou de production. Priorité :
isoler transition, signature et commit durable sans retirer leurs contrôles.

### 2026-10-02 — Altair : reset distant isolé pendant l’écriture

[Diagnostic du pair](qualification/altair-bound-diagnostics-20261002/probe-transport/README.md) :
cas original encore échoué en 2,11 s, délai 120 s inchangé. Le pair instrumenté
reçoit `Stream: reset by remote peer` en écrivant la trame 16 777 370 octets ;
la lecture de l’ACK n’est pas atteinte. HighWater/checkpoint inchangés, horloge
saine. Le garde distant ou défaut transport précis reste à identifier.
Sources, programmes, entrées de lien cœur/probe inchangés. Essai initial arrêté
avant compilation faute de Python conservé ; contrôle corrigé sans installer
une dépendance. Cœur/daemon read-only ; aucune règle d’admission assouplie.

### 2026-10-02 — Altair AMD64 : succès instrumenté, contrôle original en échec

[Comparaison conservée](qualification/altair-bound-diagnostics-20261002/daemon-comparison/README.md) : traces préadmission/ouverture, 1/1 passe (3,91398 s) ; recompilation sans nouvelles traces, 1/1 échoue (2,19625 s), reset distant sur écriture pleine. Sources/programmes/bibliothèques inchangés avant/après. Effet du diagnostic à isoler ; aucune correction ou conformité supplémentaire revendiquée. Délais, échecs historiques, critères de production et revue indépendante restent conservés.

### 2026-10-02 — Pair de qualification Altair : sondes de statut

[Contrôle AMD64](qualification/altair-bound-diagnostics-20261002/status-peer-control/README.md) : 1/1 passe en 4.36831 s, zéro skip, daemon original inchangé. Le pair de test répond désormais aux sondes périodiques avec son annonce exacte ; la trace précédente localisait le reset à `unknown-peer`. Aucun contrôle d’admission ni délai relâché. Correction du test intégrée ; ancienne campagne et échecs conservés, régression candidat globale et exigences de production toujours ouvertes.

### 2026-10-02 — Profil interne Bellatrix AMD64 démarré

[Préparation reproductible](qualification/bellatrix-inner-profile-20261002/README.md) : diagnostic des copies/contextes/vérifications/sérialisation, scénario original 112 slots et timeout 240 s conservés. Compilation terminée puis CTest actif constaté ; résultat final non acquis. Bibliothèques et objet de test réutilisés, pas de nouvelle image. Aucun critère de production clos ni code de diagnostic promu.

### 2026-10-02 — Profil interne Bellatrix terminal

[Mesures conservées](qualification/bellatrix-inner-profile-20261002/README.md) : timeout original 240,025 s, dernier slot observé 88 ; entrées inchangées et zéro skip. Sur 1 792 publications, 137,117 s incluent 70,948 s de snapshot ; copies du store 1,929 s sur 4 160 appels. Compteurs imbriqués, fenêtres différentes, ne pas sommer. Cible suivante : éviter le reparcours JSON du store sans changer encodage canonique, limites, WAL ou écritures synchrones. Aucune qualification ou promotion runtime ; autres critères et échecs historiques conservés.

### 2026-10-02 — Sérialisation candidate : équivalence et gain partiel

[Contrôles archivés](qualification/bellatrix-snapshot-splice-20261002/README.md) : ARM64, 2 048 comparaisons byte à byte des formats v2-v5, 3 072 rejets de limites et 1 024 annulations ; portée limitée à l’assemblage extrait. AMD64, timeout conservé 240,022 s, slot observé 96 contre 88. Aux mêmes 1 792 appels, encodage 44,416 contre 70,948 s, publication inclusive 109,663 contre 137,117 s. Échec terminal toujours ouvert, pas de promotion runtime ni conformité globale ; autres critères historiques conservés.

### 2026-10-02 — Snapshot : transfert des temporaires, délai encore dépassé

[Contrôle conservé](qualification/bellatrix-snapshot-move-20261002/README.md) : 512 comparaisons ciblées ARM64 passent ; AMD64 expire à 240,025 s, dernier slot 96. Aux mêmes 1 792 appels, encodage 42,678 s et publication inclusive 107,889 s : petit écart descriptif, pas de résolution du cas. Manifestes inchangés, zéro skip. Prochain diagnostic : témoin/commit/lecture authentifiée/Write synchrone séparés, sans retirer de protection. Aucune promotion runtime, autres critères conservés.

### 2026-10-02 — Commit durable mesuré, candidat sérialiseur en qualification

[Profil terminal](qualification/bellatrix-commit-cost-20261002/README.md) : timeout 240,030 s, slot 96. Sur 1 920 commits, 42,966 s incluent 13,194 s de Write synchrone ; témoin externe absent, ne pas généraliser. [Candidat Git 4834a014](qualification/checkpoint-serialization-candidate-20261002/README.md), hors main : test direct du helper ARM64 passe (2 048 comparaisons, 3 076 rejets, 1 024 annulations). Huit cas AMD64 démarrent avec limites originales ; préflight à délai de coalescence erroné arrêté avant test puis corrigé à 60 s et conservé. Aucun runtime main promu ni production qualifiée, autres critères ouverts.

### 2026-10-02 — Candidat sérialisation terminal, cache des octets publiés en compilation

[Parent 4834a014](qualification/checkpoint-serialization-candidate-20261002/README.md) : 8 cas exacts AMD64, 7 passent, partition timeout 240,027 s, dernier slot 96 ; zéro skip et entrées inchangées. [Candidat fa390c3b](qualification/published-snapshot-bytes-candidate-20261002/README.md) : comparaison des octets précédemment publiés, contrôles disque/témoin et repli crypto après réouverture conservés ; nouveau scénario stale-writer. Header modifié, recompilation complète dans Docker existant, sélection de douze tests aux délais originaux ensuite. Aucun résultat nouveau acquis, aucune promotion runtime main ni production qualifiée ; autres critères conservés.

### 2026-10-02 — Cache : échec de compilation du test conservé, reprise test seule

[Candidat](qualification/published-snapshot-bytes-candidate-20261002/README.md) fa390c3b : cœur compilé, nouveau test ne compile pas (champ inexistant), code 2, aucun CTest exécuté. Rapports conservés ; aucun résultat runtime inventé. Correction 090e95df du champ `snapshot` seulement, récupérée par Git ; headers/runtime inchangés, objets du cœur réutilisés. Reprise Docker identifiée en cours avec douze cas et délais originaux ; pas de promotion main ni de clôture de production. Tous les critères et échecs antérieurs demeurent ouverts.

### 2026-10-02 — Cache : qualification AMD64 terminale, partition non résolue

[Rapports conservés](qualification/published-snapshot-bytes-candidate-20261002/README.md) : 090e95df, 12 cas exacts, 10 passent, 2 échouent, zéro skip ; sources/programmes/bibliothèques avant/après identiques. Partition timeout 240,053 s, dernière observation slot 96, finalité commune 10 ; pas de clôture. Nouveau scénario cache arrêté au contrôle des arguments : assertions non exécutées. Correction test seule bad3e0c0 publiée/récupérée par Git, rejeu de ce seul cas en cours avec délai original 90 s. Atomicité/sync/témoin/slashing/sortie/dépôt/reorg/encodage passent dans leurs portées exactes. Aucune promotion runtime main ni qualification globale ; toutes les exigences et anciens échecs conservés.

### 2026-10-02 — Cache publié : nouveau contrôle stale-writer réussi

[Rapport bad3e0c0](qualification/published-snapshot-bytes-candidate-20261002/README.md) : un seul nouveau scénario, 1/1 passe en 0,833 s ; trois retransmissions idempotentes puis changement disque détecté et réacteur fermé. Aucun skip, sorties/entrées vérifiées, pas de rejeu des dix autres réussites 090e95df. Partition demeure en timeout au slot 96, aucune promotion runtime main ni clôture de production. Prochaine correction ciblée : retirer les copies JSON inutiles à la comparaison du cache en conservant tous les champs et gardes de validation. Les exigences et échecs historiques restent ouverts.

### 2026-10-02 — Cache JSON sans copie : gardes passent, partition toujours lente

[Contrôle 2db1840c](qualification/snapshot-cache-row-20261002/README.md), hors main : AMD64 3 cas exacts, 2 passent, partition timeout 240,047 s/dernier slot 96, aucun skip, entrées avant/après inchangées. Égalité complète du cache, mutations, règles/annulation et stale-writer qualifiés dans ces limites. Aucun gain terminal observé, délai non clôturé. Recompilation ARM64 cohérente de la même source figée dans Docker existant et 13 cas en cours ; résultats non acquis. Prochaine analyse : profil CPU isolé, sans retirer validation/WAL/sync. Aucun runtime main promu ni production globale ; tous les critères et échecs conservés.

### 2026-10-02 — ARM64 terminal, fixture corrigée, profil CPU et candidat SHA-256

[ARM64 2db1840c](qualification/snapshot-cache-row-20261002/README.md) : 11/13 passent dont partition 70,708 s ; 2 échecs de copie sur bind mount macOS conservés. Probe reproduit copy refusé sur bind et réussi sur volume Linux ; rejeu des seuls deux cas sur volume existant passe, mêmes sources/programmes/libs/caps/limites. Pas de qualification panne physique ni globale. [Profil VPS](qualification/bellatrix-cpu-profile-20261002/README.md) : 1 420 échantillons sur 30 s, piles de fetch SHA/OpenSSL identifiées, cas complet reste timeout, aucun seuil changé. [Candidat cac8d719](qualification/sha-provider-candidate-20261002/README.md), hors main : méthode EVP SHA256 fetchée une fois, erreurs contrôlées ; nouveaux tests concurrence/indisponibilité passent sur helper ARM64 réel (4 098 comparaisons). Six contrôles AMD64 en compilation/qualification, résultats/gain non acquis. Tous les critères de production et anciens échecs restent ouverts.

### 2026-10-02 — SHA-256 : gate partition AMD64 réussi, suite Capella ouverte

[Candidat cac8d719](qualification/sha-provider-candidate-20261002/README.md), hors main : 6/6 passent, aucun skip, entrées avant/après identiques. Partition 221,908 s <240 s original, slot 112, finalité commune 12 et six avancements consécutifs, assertions finales atteintes. Portée : deux cœurs/64 validateurs, un processus, clock synthétique, sans P2P ; pas de clôture WAN ni réseau public. Racines officielles, concurrence/indisponibilité, chaîne signée et stale-writer passent. Qualification ARM64 intégrée en cours après recompilation complète du cœur (timestamps du worktree renouvelés). Les cinq anciens timeouts Capella sont en qualification sur le même cœur, délais 360/480 s conservés. Aucune promotion runtime main, pas de régression globale nouvelle ; tous les autres critères et anciens échecs restent ouverts.

### 2026-10-02 — SHA-256 intégré qualifié dans six cas sur ARM64 et AMD64

[Rapports cac8d719](qualification/sha-provider-candidate-20261002/README.md) : même manifeste source, 6/6 passent sur chaque architecture, aucun skip, entrées avant/après inchangées. Partition originale : AMD64 221,908 s, ARM64 58,372 s, limites et assertions conservées. Racines officielles/concurrence/indisponibilité/chaîne/cache passent. Aucune égalité du root final entre architectures ni convergence WAN inférée des seules sorties locales ; vraie qualification réseau encore ouverte. Les cinq Capella VPS continuent, régression globale et tous les autres critères conservés. Aucun runtime main promu ni production déclarée prête.

### 2026-10-02 — Candidat de sérialisation Capella, qualification ouverte

[Candidat 3030b799](qualification/capella-serialization-candidate-20261002/README.md), hors runtime main : suppression des copies JSON et du parse/dump du store canonique, comparaison de tous les champs/cache conservée, exception de capacité Capella inchangée. Test direct ARM64 : 3 584 comparaisons d’octets, 3 844 rejets et 1 024 annulations passent ; formats Capella durable v1–v4 / store v1–v3 ajoutés. Dix cas intégrés ARM64 en cours, aucune clôture avant résultats terminaux. Campagne VPS parent cac8d719 indépendante : activation timeout 360,04 s au slot 144/finalité 16, restart encore actif ; source non modifiée pendant ce run. Tous les échecs, délais originaux, régression globale, WAN, bridge/Doppler et revue indépendante restent conservés et ouverts.

### 2026-10-02 — Activation candidat ARM64 et profiling Capella VPS

[Candidat 3030b799](qualification/capella-serialization-candidate-20261002/README.md) : activation intégrée passe en 90,94 s sous le délai original 360 s ; queued BLS changes passe en 1,99 s. Suite de dix cas encore active, résultat global non acquis. [Profil parent VPS](qualification/capella-cpu-profile-20261002/README.md) : 1 403 échantillons CPU sur 30 s, aucune perte, binaire conforme au manifeste pré-campagne. JSON lecture/écriture et piles de publication identifiées ; pas de gain AMD64 inféré ni attribution exclusive. Deux premiers cas parent timeout, auto-registration encore active ; traces intermédiaires explicitement conservées comme non terminales. Délais et exigences originales, anciens échecs, régression globale et gates de production restent ouverts.

### 2026-10-02 — Candidat Capella préparé sur AMD64 avec garde de fin de campagne

[Contrôles 3030b799](qualification/capella-serialization-candidate-20261002/README.md) : 13 726 fichiers sources identiques ARM64/VPS, checkout séparé propre. Contrôleur AMD64 confirmé actif en attente de la fin réelle du parent ; exige ses cinq résultats terminaux et manifestes inchangés avant toute recompilation partagée. Même sélection de dix cas, limites originales, image/volume réutilisés. Cinq cas ARM64 passent déjà (BLS, activation, reprise, auto-registration, reorg dépôts) ; reindexation/sorties encore actives. Qualification AMD64 non acquise, aucune clôture globale/EIP/production ni suppression des échecs.

### 2026-10-02 — Capella candidat ARM64 : dix cas terminaux réussis

[Rapport 3030b799](qualification/capella-serialization-candidate-20261002/README.md) : 10/10, zéro skip, CTest/container/post-manifest 0, sources/programmes/libs inchangés. Activation/reprise/auto-registration : 90,94/92,38/93,15 s ; réindexation activation/sortie : 111,64/110,09 s, limites originales conservées. Assertions finales : trois votes après activation et reprise, conflit refusé avant signer ; index 65 après reorg, neuf votes/finalité 20 et deux owners rouverts ; sortie ancien index refusée, sortie nouvel index incluse et persistée sans altérer l’autre validateur. Portée locale en processus/clock synthétique, pas de WAN ou panne physique. Régression entière ARM64 préparée : recompilation P2P réelle depuis mêmes sources en cours, image/volume réutilisés ; résultat global non acquis. AMD64 nouveau candidat toujours en attente du parent actif. Tous les autres critères/échecs/bridge/revue restent ouverts, aucune promotion runtime main.

### 2026-10-02 — Préparation régression entière et helpers externes ARM64

[Candidat 3030b799](qualification/full-candidate-3030b799-20261002/README.md) : tous targets P2P recompilés depuis mêmes sources ; build complet du cœur encore actif. Registre configuré observé 422 cas, aucun résultat global déduit ; commande peut manquer pendant build et garde finale exige tous exécutables. Observer séparé capture avant CTest les trois arguments exécutables omis du manifeste historique (nano, fixture crash, probe Altair), après compilation vérifiée et sans container test actif ; comparaison après tests encore pending. Contrôles/hashes datés conservés, sources/test limits inchangés, image/volume réutilisés. Parent Capella AMD64 actif, nouveau candidat en attente. Tous critères restants et anciens échecs ouverts.

### 2026-10-02 — Garde permanente des helpers requis dans le manifeste CI

[Contrôles outils](qualification/required-helper-guards-20261002/README.md) : manifeste main refuse désormais nano et fixture crash absents/non exécutables comme arguments, au lieu de les ignorer ; détection des données et garde probe conservées. Sept cas CMake réels passent, zéro skip, dont deux nouveaux couvrant disparition/permissions/remplacement du helper sans mutation du driver. Aucun runtime changé. Checkout actif 3030b799 reste figé et utilise son ancien runner avec observer externe déjà en place ; comparaison helpers après tests pending. Régression entière et AMD64 pas encore terminaux, tous critères de production et anciens échecs ouverts.

### 2026-10-02 — Préparation complète terminée, 422 CTest réellement actifs

[Préflight 3030b799 ARM64](qualification/full-candidate-3030b799-20261002/README.md) : tous targets P2P puis cœur compilés, sources avant/après build identiques. Registre final 422 cas complet, toutes commandes présentes et TIMEOUT explicite ; 200 exécutables principaux/réseau + trois helpers externes mesurés avant tests. Registre/manifestes principaux inchangés entre préparation et lancement. Container de test confirmé actif et premiers cas passent ; résultat terminal et comparaisons après tests non acquis. Ancien registre observé pendant compilation distingué du registre final. Aucun gain WAN, égalité globale de roots, clôture EIP ou production déduit ; AMD64 et autres gates ouverts.

### 2026-10-02 — Parent Capella VPS terminal, candidat corrigé AMD64 actif

[Parent cac8d719](qualification/sha-provider-candidate-20261002/README.md) : cinq timeouts terminaux (quatre à 360 s, sortie à 480 s), zéro skip, exit 8/post-manifest 0 ; sources/programmes/libs avant/après identiques. Rapports négatifs conservés, sampling CPU registration distingué d’un benchmark contrôlé. Contrôleur a vérifié la fin réelle puis démarré le build AMD64 de 3030b799, image/volume existants et checkout séparé ; mêmes dix cas et limites originales, aucun succès nouveau déduit. Régression ARM64 entière encore active, 44 premiers cas passent à l’observation ; résultat global et toutes autres gates encore ouverts.

### 2026-10-02 — Régression complète AMD64 préparée et gardée par Capella

[Contrôleur AMD64 3030b799](qualification/full-candidate-3030b799-20261002/README.md) confirmé actif en attente des dix cas Capella actuels. Exige 10/10 terminaux, zéro skip et manifestes inchangés avant compilation P2P/cœur complète. Audit Node du registre depuis Mac, aucun outil installé sur VPS ; CTest complet refusé sans inventaire complet validé. Manifestes stricts des helpers comme contrôles séparés identifiés, runtime figé ; images/volumes réutilisés. Scripts syntaxiquement vérifiés, exécution complète pas encore acquise. ARM64 continue avec 53 premiers cas réussis à l’observation, pas de résultat global. Tous critères initiaux et échecs restent ouverts.

### 2026-10-02 — Préflight Capella AMD64 identique, tests réellement actifs

[3030b799 AMD64](qualification/capella-serialization-candidate-20261002/README.md) : build terminé, sources identiques ARM64 ; dix noms/modes/délais/labels identiques, seule WORKING_DIRECTORY diffère. Premier queued-BLS passe 9,08 s, activation confirmée active ; avant-tests programmes/registre archivés. Résultat terminal, égalité des roots entre architectures et régression entière AMD64 non acquis ; contrôleur complet attend 10/10 et manifestes inchangés. ARM64 entier toujours actif, aucun autre gate de production fermé ni échec supprimé.

### 2026-10-02 — Première activation Capella AMD64 terminée, campagne ouverte

Candidat 3030b799 : le cas d’activation passe en **356,26 s / 360 s**, avec seulement 3,74 s de marge. Les assertions confirment consommation au slot 80, activation à l’époque 17, trois votes du nouveau validateur, poursuite jusqu’au slot 161 / finalité 18, réouverture des deux propriétaires et rejeu idempotent. [Preuve du cas](qualification/capella-serialization-candidate-20261002/activation-amd64-in-progress/README.md). Ce résultat demeure provisoire au niveau campagne : la clôture des dix cas et les manifestes terminaux restent à vérifier. Temps synthétique, deux états en processus, transport réseau désactivé ; aucune qualification WAN ou panne physique. Le timeout historique du parent est conservé. La régression ARM64 complète est toujours active ; AMD64 reste soumise au passage des dix cas. Tous les autres critères de production, bridge/Doppler et revue indépendante demeurent ouverts.

### 2026-10-02 — Contrôle de preuves de restauration renforcé

[Correctif et preuves](qualification/restore-evidence-guards-20261002/README.md) : le résumé de stabilité conserve désormais le checkpoint Beacon final et le hash d’exécution vérifié ; le contrôleur de restauration refuse compteurs terminaux absents/non entiers, checkpoint manquant ou incohérent, et racine terminale différente à la même époque. Reproduction directe avant correctif conservée ; le runner externe rejetait déjà les compteurs malformés via son reçu d’arrêt, sans comparaison de cette racine. Quatre suites ciblées **60/60, zéro skip**, dont **16 nouveaux cas** ; ce sont des tests de vérificateur, aucune nouvelle qualification WAN ou de production. Les trois avances consécutives et délais restent inchangés. Un reçu terminal plus récent ne prouve pas à lui seul l’ascendance. Campagnes Docker/source gelée 3030b799 inchangées ; historiques, finalité WAN, revue indépendante et lancement restent ouverts.

### 2026-10-02 — Reconnexion : preuves RPC malformées refusées

[Contrôles de reconnexion](qualification/recovery-sample-guards-20261002/README.md) : racines absentes/courtes, époques ou compteurs de pairs malformés et délais invalides sont refusés avant tout crédit ; quantités RPC décimales/hexadécimales canoniques acceptées. **19/19**, zéro skip, dont **13 nouveaux cas**. Aucun changement de runtime ou campagne WAN ; seuils/délais originaux conservés. Écart identifié pour la suite : l’audit natif à six champs rejoue l’historique canonique mais ne reçoit pas encore le checkpoint observé indépendamment pour vérifier son ascendance lorsque le reçu terminal a avancé. Ce critère reste ouvert ; les sources/binaries Docker gelés ne sont pas modifiés.

### 2026-10-02 — Audit natif d’ascendance implémenté sur candidat isolé

[Candidat 736dab46 / preuves](qualification/finalized-ancestry-audit-20261002/README.md), parent 3030b799 : `requiredFinalized` permet de vérifier le checkpoint observé contre la branche finalisée rejouée à la limite de son époque, slots vides inclus. Requêtes historiques à six champs conservées sans prétendre vérifier ce checkpoint externe. ARM64 Docker : **12 cas du helper natif**, puis intégration réelle sur **99 blocs / 97 transactions** : checkpoint valide accepté, tête non finalisée / époque future / racine malformée refusées. Base et driver inchangés avant/après. Tentatives initiales et correction de longueur avant décodage conservées. Compilation directe du seul nouveau module contre les bibliothèques 3030b799 en lecture seule ; ce n’est pas un rebuild complet. Checkpoint choisi dans la fixture pour cette intégration, aucune observation WAN indépendante. Candidat publié séparément ; main runtime inchangé. Branchement du runner WAN, qualification AMD64, régression et tous les critères de production restent ouverts.

### 2026-10-02 — Branchement de l’ascendance à la restauration ; échec de réindexation conservé

[Runner et contrôles](qualification/restore-offline-binding-20261002/README.md) : après arrêt propre du restauré, le checkpoint observé et le token d’arrêt séparés alimentent l’audit natif readonly ; rapport correspondant exactement aux deux exigé avant réussite. **14 nouveaux cas du contrôleur passent**. Candidat natif 40132425 : rapport enrichi vérifié sur un seul rejeu positif réel ARM64 (99 blocs/97 transactions), sans rejouer helper/négatifs inchangés. Pas de campagne WAN : main runtime ancien ne traite pas ce nouveau champ et le runner échoue explicitement jusqu’à qualification/promotion du candidat.

[Échec AMD64 3030b799](qualification/capella-serialization-candidate-20261002/reindex-amd64-in-progress/README.md) : activation réindexée **timeout 360,04 s**, dernier marqueur slot 152/finalité 17/six nouveaux votes. Les activations simples près de 356 s ne ferment pas ce cas. Campagne encore active, manifestes terminaux pendants. Régression complète AMD64 conditionnée aux dix réussites : gate non satisfaisable avec cet échec. Aucun délai augmenté ni source modifiée ; diagnostic de coût après fin de campagne requis. Les critères WAN, architectures, régression, bridge/lancement et revue indépendante restent ouverts.

### 2026-10-02 — Capella AMD64 terminal et candidat de sources cohérent

[Campagne 3030b799 terminale](qualification/capella-serialization-candidate-20261002/terminal-amd64/README.md) : **9/10**, zéro skip, source/binaries/libs inchangés, seul échec activation réindexée **360,04 s**. Sortie réindexée **426,80 s / 480 s** avec mêmes assertions et même racine de finalité ARM64/AMD64 à l’époque 20 ; aucun transport réseau. Le contrôleur de régression complète AMD64 s’arrête à son gate des dix succès. Diagnostic CPU borné actif après fin de campagne, première tentative sans fixtures conservée ; aucune qualification issue d’un run instrumenté.

[Candidat e91d647b](qualification/coherent-candidate-e91d647b-20261002/README.md) réunit runtime 40132425 et contrôleurs main 1623a24f sans modifications de ces composants ; **14 904 empreintes sources identiques** Mac/VPS, checkout distant par Git. Aucune qualification binaire de ce nouvel ensemble, aucune promotion runtime main. Régression complète ARM64 3030b799 toujours active. Coût réindexation, architectures, WAN/reprise, EVM activée, lancement/bridge et revue indépendante restent ouverts.

### 2026-10-02 — Diagnostic CPU réel de la réindexation acquis

[Rapports et erreurs conservés](qualification/capella-reindex-profile-20261002/README.md). Premier diagnostic arrêté faute de fixtures ; seconde tentative monte la source gelée, mais le sélecteur initial cible `timeout`. Le vrai PID C++ 491325 est échantillonné sur **le même run**, sans troisième rejeu ni délai prolongé : **1 419 samples / zéro perte**, binaire SHA attendu. Feuilles : JSON escaped 5,07 %, malloc 5,00 %, keccak 4,58 %, hex 3,10 % ; piles de sérialisation Capella présentes. Aucun pourcentage imbriqué additionné ni gain total déduit. Données perf brutes privées sur VPS. Le run diagnostique atteint 360 s (exit 124), binaire/libs inchangés ; ce n’est pas une qualification. Prochaine correction ciblée : conversions complètes répétées sur hits de mémo, en conservant chaque champ, les validations et l’annulation. Tous les critères de production restent ouverts.

### 2026-10-02 — Encodage SSZ réutilisé après comparaison complète

[Candidat 12ad9eb5 et preuves](qualification/ssz-encoding-reuse-20261002/README.md), parent cohérent e91d647b : évite la reconversion hex des états SSZ inchangés, comparaison exacte de tous les octets et ligne complète conservée, métadonnées courantes et annulation bornée. Docker ARM64 : **773 refus helper**, puis **trois variantes intégrées Capella** passent avec nouvelles mutations SSZ/epoch comparant cache chaud et sans cache. Deux erreurs de compilation/édition de liens conservées ; compilation directe snapshot+audit contre bibliothèques gelées, pas rebuild complet. Sources/programmes/entrées statiques inchangés. Compilation CMake AMD64 du candidat lancée dans un sous-répertoire distinct sur VPS, aucun test AMD64 ni gain de délai démontré. L’échec réindexation 360 s demeure ouvert. Régression ARM64 3030b799 poursuit, dont partition/récupération et pruning Bellatrix passent ; pas de résultat terminal global. Main runtime inchangé ; toutes les gates de production/WAN/bridge/lancement/revue restent ouvertes.

### 2026-10-02 — Manifeste des contrôleurs WAN complété

[Correction et six nouveaux cas](qualification/wan-harness-manifest-20261002/README.md) : liste précédente omettant l’audit de restauration, le contrôleur de stabilité et les dépendances d’alertes remplacée par inventaire récursif des imports statiques relatifs + commandes externes explicites (**31 fichiers**). Six cas Node passent ; après ajout de la racine livraison d’alertes, seul inventaire concerné rejoué et passe. Manifeste de sources seulement, pas attestation des exécutables ni qualification WAN. Contrôle AMD64 12ad9eb5 mis en attente du build réellement actif : exactement réindexation **360 s** + deux helpers **60 s**, sources/programmes/libs et registry/résultats audités, aucun succès revendiqué avant terme. Sources et runtimes des campagnes actives préservés, main runtime inchangé. Tous critères de production et échecs historiques conservés.

### 2026-10-02 — Allocation hex bornée et candidat cohérent suivant

[Candidat 669ee563 et mesures](qualification/hex-allocation-20261002/README.md) : encodage hex canonique avec une allocation de sortie. **1 031 cas différentiels ARM64 et AMD64 passent** ; à 1 MiB, 18 allocations deviennent une, ratio médian du microbenchmark 6,37 ARM64 / 3,79 AMD64, sans extrapolation au nœud. **4 069 checks core ARM64** passent avec types.o nouveau et bibliothèques gelées, programme/entrées statiques inchangés ; lien direct partiel, aucune intégration complète AMD64. **2d2764de** réunit cette correction et les contrôleurs main 6af1db8d, **15 022 sources identiques Mac/VPS**, inventaire réel validé ; ensemble encore non compilé. Campagnes 3030b799 ARM64 et 12ad9eb5 AMD64 continuent sans modification. Échec de réindexation 360 s, régression finale, WAN, EVM activée, lancement/bridge et revue indépendante restent ouverts. Main runtime inchangé.

Compilation CMake du cœur et de sept cibles pertinentes du candidat **2d2764de** démarrée ensuite dans Docker ARM64 (image/volume existants, répertoire distinct, un CPU/2 GiB). Contrôle et limites dans le rapport ci-dessus ; aucun test de cet ensemble compilé encore acquis, pas toutes les cibles de régression. Campagnes gelées préservées.

### 2026-10-02 — Régression ARM64 complète verte et candidat cohérent sélectionné

[Résultats terminaux et limites](qualification/coherent-terminal-arm-20261002/README.md) : **3030b799 ARM64 422/422**, zéro skip, **6 088,58 s**, sources/programmes et trois helpers externes inchangés, audits terminaux passent. EEST consommé **5 387 appariements / 317 fichiers**, aucune conformité intégrale de fork déduite. **2d2764de ARM64 15/15**, zéro skip, **487,22 s**, huit programmes/sept entrées statiques/sources inchangés ; réindexation 102,36 s / 360 s, pas régression complète ni WAN. **12ad9eb5 AMD64 2/3** : timeout réindexation **360,03 s**, slot 152/finalité 17/six nouveaux votes, intégrité conservée. Échec SSZ seul non résolu, nouvelle allocation hex dans 2d2764de encore à qualifier sur ce cas AMD64. Build AMD64 2d2764de actif (répertoire distinct, image/volume existants, deux CPU/3 GiB). Tous critères restants et échecs historiques restent ouverts ; main runtime inchangé.

Les huit cibles AMD64 **2d2764de** terminent ensuite leur compilation ; sélection identique des **quinze cas** confirmée active, succès terminal encore attendu. Une seule régression complète ARM64 du candidat **2d2764de** est lancée après son gate 15/15 : cibles manquantes complétées dans le même build, image/volume réutilisés. Justification : changement global d’encodage depuis 3030b799, dont le résultat 422/422 ne se transpose pas automatiquement. Contrôles et limites dans le rapport ci-dessus ; aucune promotion runtime main.

### 2026-10-02 — Rejeu natif AMD64 préparé sans concurrencer la qualification

[Contrôle et limites](qualification/coherent-offline-amd64-20261002/README.md) : témoin arrêté ARM64 transféré avec hash identique ; audit natif **2d2764de** mis en attente de la campagne AMD64 existante, empreintes base/source/binaire et quatre cas exigés. Aucun résultat acquis ni observation WAN indépendante, aucun journal de signature transféré, aucune reprise des duties prouvée. Activation AMD64 observée **346,72 s / 360 s** ; autres cas et régression ARM64 complète encore actifs. Marge faible et échecs historiques conservés ; aucun critère de production clos ni runtime main promu.

### 2026-10-02 — Chemins WAN cohérents ARM64/AMD64 et checkout distant candidat

[Correction du contrôleur](qualification/coherent-wan-paths-20261002/README.md) : choix séparés des builds ARM64/AMD64, checkout Git distant explicite et routage des appels d’audit/sauvegarde/rotation. **17 contrôles Node Mac passent, dont 7 nouveaux**, erreur initiale null conservée. Enregistrement CTest conditionnel Node ajouté, non exécuté dans les builders actuels sans Node. Aucun runtime ou build actif modifié, aucun résultat WAN acquis ; restauration complète, reprise des duties et tous critères de production restent ouverts.

### 2026-10-02 — Propriétaire Mac natif cohérent en préparation

[Compilation et contrôle](qualification/coherent-wan-native-20261002/README.md) : source 554ebc3a, runtime strictement égal à 2d2764de ; CMake Mac Release isolé actif, huit cibles, P2P existant hashé. Nécessaire au propriétaire macOS natif demandé, les deux autres restent Docker Linux. Huit cas natifs enregistrés et contrôle préparé, **aucun résultat natif acquis**. Campagnes Docker inchangées ; activation/redémarrage AMD64 passent en 346,72/350,86 s sur 360 s, marge faible. Restauration complète, duties, WAN, régression et tous critères de production restent ouverts.

### 2026-10-02 — Build Mac natif terminal, sélection 8/8

[Résultats et portée](qualification/coherent-wan-native-20261002/README.md) : checkout 554ebc3a, runtime égal à 2d2764de, huit programmes compilés ; sources/P2P inchangés. **8/8 CTest natifs, zéro skip, 2,71 s**, registre exact/délais originaux ; **198 fichiers d’entrée** inchangés avant/après. Nouveau périmètre de plateforme native et enregistrement CTest des contrôles WAN ; aucune régression native complète, rotation réelle ou reprise de duties démontrée. Campagnes Docker toujours indépendantes ; tous critères de production ouverts.

### 2026-10-02 — Rejeu canonique signé sur macOS natif 4/4

[Preuves et première erreur conservée](qualification/coherent-wan-native-offline-20261002/README.md) : binaire CMake natif identifié, **99 blocs / 97 transactions**, checkpoint finalisé/engagement attendus acceptés ; tête non finalisée, époque future et racine malformée refusées (**4/4**). Sandbox enfant sans écritures/réseau, base/source/binaire inchangés ; 198 entrées liées identiques au parent qualifié. Première tentative du wrapper refusée avant Nano, aucune qualification déduite. Nouvelle portée OS natif sur fixture arrêtée, pas d’observation WAN indépendante ni reprise des duties ; critères de production et échecs historiques restent ouverts.

### 2026-10-02 — Restauration WAN cohérente mise en attente des gates terminaux

[Contrôleur et erreurs conservés](qualification/coherent-wan-restore-queue-20261002/README.md) : session 78086 observe les campagnes existantes sans les relancer ; gates ARM64 425/425, AMD64 15/15, audit arrêté AMD64 4/4 et natif 8/8/intégrité exigés avant fixture fraîche. Images/volumes réutilisés, architectures séparées, source VPS candidate par Git, aucun runtime main promu. La future campagne privée doit restaurer journaux/witness, reprendre les duties et lier finalité commune observée au checkpoint terminal. **Aucun résultat WAN acquis**, gates encore pendants ; tous critères de production et revue indépendante restent ouverts.

### 2026-10-02 — AMD64 14/15, audit arrêté 4/4 et candidat de dump canonique

[Résultats et candidat f9a3ee1a](qualification/checkpoint-row-dump-20261002/README.md) : parent2d2764de AMD64 **14/15**, zéro skip, intégrité conservée ; réindexation **timeout360,05 s** au même marqueur152/17/six votes. Gate WAN impossible : seul contrôleur en attente arrêté, aucune campagne C++ interrompue ni nœud WAN lancé. Audit arrêté AMD64 corrigé après erreur de sélection hash avant Nano : **4/4**, 99 blocs/97 transactions et checkpoint attendu, base/source/binaire inchangés. Résultat distinct qui n’efface pas le timeout. Nouveau candidat réutilise les dumps après comparaison complète des rows : ARM helper **769 exacts/768 refus/384 annulations**, benchmark assemblage4,52× sans extrapolation ; trois variantes signées et cinq nouvelles catégories de mutation passent via lien partiel. Mémoire du cache accrue, pas build complet/AMD64 du candidat ni gain nœud qualifié. Aucun runtime main promu ; toutes exigences production et revue indépendante ouvertes.

### 2026-10-02 — Compilation cohérente de la correction sur les deux architectures

[Candidat0275b1af / contrôles](qualification/coherent-row-candidate-0275b1af-20261002/README.md) : runtime identique à f9a3ee1a, contrôleurs WAN réunis, **15 313 sources / hash manifeste identique Mac/VPS**. Neuf cibles Release en compilation Docker dans répertoires distincts, images/volume réutilisés. Seize cas pertinents préparés avec limites d’origine et intégrité ; AMD attend son build, ARM attend aussi l’ancienne régression pour éviter la concurrence des scénarios. **Aucun résultat terminal de cet ensemble acquis**, réindexation360 s toujours à fermer. Ancien14/15, porte WAN, mémoire cache, régressions, exigences production/revue indépendante restent ouverts ; runtime main inchangé.

### 2026-10-02 — Compilation ARM64 cohérente terminale

[Build0275b1af terminal](qualification/coherent-row-candidate-0275b1af-20261002/README.md) : neuf programmes, sorties build/container/source-compare0, sources avant/après inchangées. **426 cas enregistrés** (sans Node), seize noms/délais d’origine vérifiés. Aucun cas de ce nouveau build encore exécuté ; ARM attend l’ancienne régression, AMD compile encore. Aucun runtime main promu, aucune porte production fermée.

### 2026-10-02 — compilation cohérente AMD64 terminée, exécution en cours

Candidat isolé **0275b1af**, neuf programmes AMD64 construits, sources exactes identiques à ARM64 ; registre426/sélection16 et délais originaux vérifiés. La sélection AMD64 est active, pas encore terminale. Compilation macOS native du même candidat et contrôles/rejeu signés préparés pour le troisième propriétaire ; aucun nouveau résultat natif ou WAN acquis. Rapports et scripts : [coherent-row-candidate-0275b1af-20261002](qualification/coherent-row-candidate-0275b1af-20261002/README.md). Aucun statut EIP, timeout historique ou critère de production clôturé.

### 2026-10-02 — candidat 0275b1af : périmètre natif qualifié

Neuf programmes macOS construits depuis les mêmes sources Docker : **9/9** contrôles core/helpers/contrôleurs, zéro skip, délais conservés ; **4/4** audits signés en lecture seule (**99 blocs / 97 transactions**, ascendance finalisée requise et trois rejets négatifs). Deux cents entrées binaires existantes et sources inchangées avant/après, y compris après audit. [Preuves et limites](qualification/coherent-row-candidate-0275b1af-20261002/README.md). Cette qualification locale du nouveau candidat ne ferme pas la réindexation AMD360, la régression globale, la restauration des journals/duties ou la finalité WAN ; aucun statut EIP changé.

### 2026-10-02 — régression AMD64 complète conditionnée au candidat 0275b1af

Contrôleur vivant Mac/VPS en attente de la sélection **16/16** avec délais originaux et entrées inchangées ; ensuite build séparé et registre complet **426** exigés, sans reconstruire image ni P2P ou modifier sorties déjà qualifiées. Audits de registre/JUnit/EEST sur Mac puisque builder et VPS sans Node. [Contrôles et scope](qualification/full-coherent-0275b1af-amd64-queue-20261002/README.md). Aucun résultat complet nouveau encore acquis, aucun échec historique/critère de production/EIP clôturé.

### 2026-10-02 — prérequis WAN du candidat contrôlés sans lancement

Nouveau préflight réellement exécuté : sources Mac/VPS, natif9/200 entrées et audit signé natif4 vérifiés ; résultats Docker terminaux et audit AMD4 encore requis, sortie2 attendue, aucun nœud WAN démarré. Régression ARM parent425 explicitement séparée du delta snapshot actuel (deux fichiers runtime modifiés) et de la régression AMD426 attendue. Audit AMD4 préparé sans concurrence avec tests/builds chronométrés. Observation mémoire intermédiaire et activation337,35/360 conservées, sans qualification de charge/mémoire finale. [Contrôles et limites](qualification/coherent-0275b1af-wan-preflight-20261002/README.md). Aucun critère final/EIP ni échec historique clôturé.

### 2026-10-02 — dossier de lancement : inspection Airlock supplémentaire

[Inspection source immuable et critères de raccordement](qualification/doppler-airlock-path-20261002/README.md) ajoutés au cadrage Doppler. Aucune implémentation ou qualification de bridge/vente acquise ; aucun statut EIP ou gate de consensus clôturé. Les campagnes Docker continuent indépendamment.

### 2026-10-02 — lanceur de restauration privée : refus vérifié

Lanceur du candidat0275 préparé avec gate réexécuté, attestation image/binaire future et invocation du harness Capella backup/restore. Exécution actuelle **exit2/preflight-refused** : aucun nouveau nœud WAN ou metadata de provenance Docker écrit, prérequis terminaux encore manquants. [Contrôle et traces](qualification/coherent-0275b1af-wan-preflight-20261002/README.md). Attestation, reprise/finalité WAN, statuts EIP et critères de production restent ouverts.

### 2026-10-02 — réindexation AMD0275 encore en échec, delta DOM isolé

Timeout original **360,04 s** au slot160/finalité18/sept nouveaux votes : régression AMD complète et WAN restent refusés. Sélection encore active, preuves partielles conservées. Candidat a0e18693 : **1153** comparaisons canoniques et trois intégrations signées locales passent par lien partiel ARM ; diagnostic de copie profonde rectifié, aucun gain CPU/mémoire ou qualification globale acquis. Commentaire seul ajusté en418d05cc, branche isolée, aucun runtime promu. [Rapports, patch et limites](qualification/snapshot-dom-lifetime-20261002/README.md). Aucun échec historique, statut EIP ou critère final clôturé.

### 2026-10-02 — sélection AMD0275 terminale, gate complet refusé

**15/16**, zéro skip,1856,66 s ; seule réindexation360,04 en timeout (slot160/finalité18/sept votes), exits399,736/480 passe. Sources/neuf programmes/sept entrées avant/après inchangés, délais originaux exacts. [Sélection terminale](qualification/coherent-row-candidate-0275b1af-20261002/README.md) ; [driver complet exit1](qualification/full-coherent-0275b1af-amd64-queue-20261002/README.md), aucun full AMD démarré. Aucun runtime promu, profil EIP ou gate production clôturé ; poursuite du diagnostic requise.

### 2026-10-02 — SSZ du cache : copie différée, qualification cohérente lancée

[Candidat80373152 et preuves](qualification/snapshot-ssz-deferred-copy-20261002/README.md) : tous les octets SSZ et les autres champs sont comparés avant réutilisation ; placeholder jamais émis, changement matérialisé puis revalidé. ARM partiel :1153 comparaisons canoniques,2560 contrôles différentiels,256 mutations SSZ refusées et trois scénarios locaux signés avec deux SIGKILL chacun passent ; entrées inchangées. Aucun gain de vitesse acquis. Builds Docker cohérents ARM/AMD neufs et sélection16 aux délais originaux mis en attente ; résultats terminaux non acquis. Ancienne queue ARM0275 annulée avant tests, échec AMD15/16 conservé. Runtime main inchangé, aucun statut EIP ni gate production/WAN/lancement clôturé.

### 2026-10-02 — régression ARM2d terminale425/425

[Registre complet et rapports](qualification/full-coherent-2d2764de-arm64-terminal-20261002/README.md) :425/425, zéro skip/échec,5895,09s ; sources et programmes inventoriés inchangés, JUnit exact, audits exits0. EEST5387 (4882 exécution/505 validation),317 fichiers, forkConformanceProven=false. Révision2d antérieure : ne qualifie pas les deltas0275/803, l'AMD ou une reprise WAN. Aucun statut EIP ni gate production/lancement clôturé.

### 2026-10-02 — candidat803 : propriétaire natif et gates préparés

[Contrôleurs et observation effective](qualification/coherent-80373152-release-gates-20261002/README.md) : build natif neuf démarré, qualification9+audits4 en attente ; driver completAMD attend le passage16/16 aux délais originaux avant tout build frais426. Gate WAN exécuté exit2 : sources exactes et parentARM425 vérifiés, six prérequis encore non acquis, aucun nouveau nœud WAN. ARM actuel cinq premiers tests passent (activation79,11/360 ; restart80,34/360), sélection non terminale. Aucun gain causal, runtime promu, EIP ou critère production/lancement clôturé.

### 2026-10-02 — candidat803 : natif9+audit4 terminaux et buildAMD cohérent

[Preuves terminales](qualification/coherent-80373152-release-gates-20261002/README.md) : buildAMD0/neuf cibles/registre426, sélection16 démarrée ; buildnatif0,9/9 zéro skip2,89s et4/4 audits signés99blocs97tx passent. Sources identiques ARM/AMD/natif ; fixture/binaire/200 entrées matérialisées inchangés après tests et audit. Le checkpoint finalisé et l'ascendance sont vérifiés, pas la reprise des duties/WAN. SélectionsDocker et fullAMD non terminaux ; aucun runtime promu, EIP ou critère production/lancement clôturé.

### 2026-10-02 — candidat803 : sélectionARM16 terminale

[Rapports exacts](qualification/coherent-80373152-release-gates-20261002/README.md) :16/16, zéro skip/échec,457,46s ; réindexation97,88/360, sorties95,14/480 passent ; sources/neuf programmes/sept entrées inchangés, registre/délais originaux vérifiés. AMD actif, full et WAN non démarrés. AuditAMD futur lié au binaire complet réel ; enchaînement privé en attente du driverAMD puis audits/gates exacts, sans relance automatique. Aucun runtime promu, statut EIP ou critère production/lancement clôturé.

### 2026-10-02 — reprise privée : intégrité post-exécution préparée

[Lanceur et gate actuels](qualification/coherent-80373152-release-gates-20261002/README.md) : contrôle des sources/200 entrées natives/deux couples de programmesDocker avant-après WAN ajouté, mais non encore exercé sur le réseau. Gate réexécuté :5/8 vérifiés, trois preuvesAMD manquantes, exit2/aucun nœud lancé. TestAMD et queues précis toujours actifs ; aucun runtime ou délai modifié, aucune clôture EIP/production/lancement.

### 2026-10-02 — reprise privée : sources des deux propriétaires liées

[Collecte réelle et contrôles](qualification/coherent-80373152-release-gates-20261002/README.md) :15313 fichiers identiques Mac/VPS, manifestes avant/après des deux sources ajoutés au lanceur préparé ; aucun WAN exercé. AMD restart335,42/360 passe, sélection encore active. Observation partielle : pic499560448octets sous3GiB, zéro OOM/max/throttling ; ni qualification de charge ni gain causal. Tous les critères terminauxAMD/WAN/production et statuts EIP restent ouverts.

### 2026-10-02 — candidat803 : timeout de réindexationAMD persistant

[Échec et contrôleur diagnostic](qualification/coherent-80373152-release-gates-20261002/README.md) :timeout360,04,slot160/finalité18/sept votes. Sélection encore active, pas de16/16 acquis ; full et WAN resteront refusés. Diagnostic unique séparé en attente de la fin :programme exact/archives readonly,scratch neuf,360s,30s49Hz aprèsslot128,PID/executable/hash vérifiés ; profil non acquis et résultat instrumenté non qualifiant. Aucun délai augmenté, échec effacé, runtime promu ou critère EIP/production/lancement clôturé.

### 2026-10-02 — candidat803 : sélectionAMD terminale15/16 et refus effectifs

[Registre et rapports terminaux](qualification/coherent-80373152-release-gates-20261002/README.md) :15/16,zéro skip,1835,73s ; unique réindexation360,04 timeout ; sorties391,395/480 passent. Sources/neuf programmes/sept entrées inchangés et délais exacts. Driverfull exit1/queueWAN exit1,newWanNodesStarted=false. Diagnostic séparé démarré seulement après la sélection, profil non acquis. Aucun runtime promu ou critère EIP/production/lancement clôturé.

### 2026-10-02 — profilCPU803 terminal, correction suivante à établir

[Mesure et scope](qualification/coherent-80373152-release-gates-20261002/README.md) :1420 échantillons/zéro perte/PID564794,30s49Hz après slot128 ; diagnostic124/360s, sources/programme/bibliothèques inchangés, pas de qualification. Allocations,Keccak,BLS/JSON observés ; piles SSZ signing_root et store en chevauchement, sans addition ni gain causal. Examiner les allocations de contexteSHA256 et recalculsJSON ; correction encore non implémentée.15/16 original conservé,full/WAN refusés,aucun runtime/EIP/production/lancement clôturé.

### 2026-10-02 — candidat SHA256 isolé, qualification complète ouverte

[Correctif, sources et mesure](qualification/consensus-sha-context-20261002/README.md) : `f7e46437` dérivé803 réutilise un contexteSHA256 propre à chaque thread ; aucun calcul/signature/gas/délai modifié. Tests directs DockerARM/AMD0 :5634 comparaisons différentielles,513 vecteurs connus,512 mutations de bits,16 threads simultanés/8 remplacements et fournisseur indisponible refusé puis initialisation reprise. Microbenchmark seulement : médiane−14,64% ARM/−11,49% AMD, pas de gain de reprise démontré. BuildCMakeAMD frais lancé avec réindexation360s et deux tests fournisseur30s, processus/conteneur vérifiés actifs ; résultat en attente. Injection des erreurs allocation/finalisation, régression complète/natif/WAN non qualifiés.803AMD15/16/full-WANrefusés conservés,aucun runtime promu,statut EIP/production/lancement clôturé.

### 2026-10-02 — erreurs SHA256 : nouvelle couverture ARM acquise

[Injections bornées](qualification/consensus-sha-context-20261002/fault-paths-arm-terminal/README.md) : compilation directe du runtime figé `f7e46437`, allocation/Init/Update/Final forcés en erreur ; refus, court-circuit, destruction du contexte et388 vecteurs de reprise passent sur DockerARM. Compteurs vérifient la réutilisation après reprise. Cette preuve complète l'absence d'injection du rapport précédent ; ni crash matériel ni qualification complète. ÉquivalentAMD en attente du build/réindexation actuels pour éviter une concurrence de ressources ; auditeur3cas/registres/délais/intégrité préparé, pas encore exercé. Aucun runtime promu, échec effacé, critère EIP/production/lancement clôturé.

### 2026-10-02 — build AMD SHA256 terminal, scénario bloquant démarré

[Build réel et registre](qualification/consensus-sha-context-20261002/amd-build-terminal/README.md) : deux cibles CMake fraîches `f7e46437`, dépendances recompilées, build0 ; registre3 exact/délais360/30/30/sans désactivation vérifiés. Réindexation réellement démarrée PID573453 puis deux testsSHA256 prévus ; résultats/intégrité après exécution encore en attente. Ni build complet du nœud, régression globale, reprise native/WAN ni critère EIP/production/lancement clôturé.803AMD15/16 et anciens échecs conservés.

### 2026-10-02 — f7SHA256 terminal AMD2/3 : réindexation toujours hors délai

[Résultat exact](qualification/consensus-sha-context-20261002/amd-reindex-terminal/README.md) : réindexationTimeout360,04s ; tests fournisseur2/2 passent ; total360,06s,aucun skip,source15313 et deux programmes inchangés,délais360/30/30 préservés,exits8. Dernier progrès slot168/finalité19/huit votes, sans revendication de gain causal ; scénario non terminé. InjectionsAMD post-scénario0/quatre erreurs refusées/388 reprises vérifiées, complétant ARM. Le gain microSHA256 ne clôture pas la reprise. Examiner ensuite la sérialisation répétée des comptes à partir du profilCPU, avec comparaison exhaustive aux seuls caches privés validés et oracle non caché ; correctif suivant non implémenté. Aucun full/WAN actuel lancé,runtime promu,statut EIP/production/lancement clôturé ; anciens échecs conservés.

### 2026-10-02 — candidat comptes0f53 isolé, nouvelle couverture sanitizer

[Correctif et périmètre](qualification/snapshot-account-view-20261002/README.md) : primitiveAccount complète comparée à l'encodage privé antérieurement validé, sans identité de pointeur/racine seule ; nonce,solde,code,tous slots et membres même vides/zéro comparés,annulation maintenue. Autres champs de bloc/reçus/dépôts restent comparés ; toute différence matérialise les deux champs différés avant validation. ARM ASan/UBSan direct1408 oracles/384 lignes/64 annulations passent,syntaxe intégration0 ; aucun ancien core lié. Test intégré14 mutations préparé mais non exercé. BuildAMD frais4cibles/PID577081 réellement actif ; six tests prévus aux délais originaux,registre427 attendu. Ancien f7timeout360,04 conservé ; pas de gain nœud,full/natif/WAN,runtime promu,statut EIP/production/lancement clôturé.

### 2026-10-02 — build0f53 AMD terminal ; transition signée passe, campagne en cours

[Build et observation exacte](qualification/snapshot-account-view-20261002/amd-build-terminal/README.md) :4cibles fraîches/build0/quatre programmes hachés/sources15315 identiques à la version figée ; registre6/délais120/360/30/30/60/60 et absence de règles skip/désactivation vérifiés. Transition signée rapportée Passed28,02s, avec byte-oracle cold/warm et14 mutations ; liaison à l'intégrité finale de campagne encore attendue. Réindexation en cours,résultat non acquis. Auditeur6 exact préparé/syntaxe0 mais pas encore exercé sur résultat terminal. Aucun full/natif/WAN,runtime promu,statut EIP/production/lancement clôturé ; anciens échecs conservés.

### 2026-10-02 — 0f53AMD6/6 terminal ; régressions complètes réelles démarrées

[Audit indépendant terminal](qualification/snapshot-account-view-20261002/amd-six-terminal/README.md) :6/6,zéro skip,382,51s ; réindexation352,815/360 passe et scénario termine,transition signée28,02/120 ; sources15315/quatre programmes inchangés,registre/commandes/délais/JUnit/exits0 vérifiés. Marge7,19s limitée,aucune garantie de charge ni gain causal contrôlé ; anciens timeouts conservés. [Contrôleurs complets](qualification/snapshot-account-view-20261002/full-control/README.md) :nouveaux buildsARM/AMD puis registre427 exigé/testsglobaux une fois,images/volumes existants,sorties qualifiées précédentes intactes ; ARMsession30353/AMDhostPID582547 et conteneurs réellement actifs. Résultats complets/auditsEEST/natif/offline/WAN non acquis,aucun runtime promu/statut EIP/production/lancement clôturé.

### 2026-10-02 — Portabilité native du test des comptes

[Preuve et candidat isolé](qualification/snapshot-account-portability-20261002/README.md) : build natif `0f53b42d` refusé par une surcharge GMP ambiguë dans le nouveau test ; exit 2 et sources inchangées archivés. Descendant `016ab243` corrige une conversion entière exacte dans le seul test, runtime inchangé. Build natif frais lancé, treize cas et quatre audits signés conditionnés à sa réussite ; aucun passage anticipé. Les régressions Docker complètes 427 cas restent sur leur arbre figé `0f53b42d`. Aucun échec historique, critère de production, WAN ou statut EIP supprimé ou clôturé.

### 2026-10-02 — Qualification native du correctif de portabilité

[Résultats terminaux indépendamment audités](qualification/snapshot-account-portability-20261002/README.md) : candidat `016ab243`, onze programmes construits frais ; **13/13 cas natifs** et **4/4 audits signés en lecture seule** passent. 15 315 sources et 205 entrées matérielles inchangées ; rejeu de 99 blocs/97 transactions et refus de tête/époque/racine invalides. Première tentative de sandbox macOS refusée avant Nano conservée, puis relance autorisée des seuls audits avec interdiction de réseau/écriture. Aucun résultat WAN/duties ou régression complète Linux du descendant déduit ; campagnes 427 cas du parent toujours séparées. Tous les critères/EIP non qualifiés restent ouverts.

### 2026-10-02 — Raccordement explicite des preuves au scénario WAN

[Contrôleur exécuté, refus exit 2](qualification/runtime-wan-evidence-gate-20261002/README.md) : delta exact du seul test entre Docker `0f53b42d` et Mac `016ab243`, et audit natif indépendant réexécuté avec succès. Les régressions complètes 427 cas ARM64/AMD64 et l’audit signé du binaire AMD64 exact restent requis ; campagnes actives, résultats globaux non acquis. Aucune nouvelle instance WAN, aucune fausse identité de révision complète ni qualification production déduite. Attestation matérielle des binaires, reprise des duties/finalité commune et tous les autres critères restent ouverts.

### 2026-10-02 — Audit signé du binaire AMD64 complet mis en attente

[Contrôleur réel confirmé vivant](qualification/full-amd-signed-offline-queue-20261002/README.md) : PID VPS 587064, attente du processus Docker complet 582618. Aucun audit ni rebuild lancé ; les 427 résultats, manifestes et hash exact du binaire complet sont requis avant quatre requêtes signées en lecture seule. Régression ARM64/compilation AMD64 existantes continuent, résultats globaux non acquis. Tous les critères de production, WAN, bridge et statuts EIP restent ouverts.

### 2026-10-02 — Lanceur de restauration raccordé aux builds courants

[Lanceur et refus réel](qualification/runtime-wan-evidence-gate-20261002/README.md) : nouveaux chemins complets ARM64/AMD64, source native `016ab243` et Docker `0f53b42d` enregistrées séparément, manifests exacts et 205 inputs natifs exigés. Exécution exit 2 avant création de nœuds : preuves terminales encore manquantes. Voie positive/attestation/restauration WAN non qualifiées ; campagnes existantes continuent. Aucun critère de production ni statut EIP clôturé.

### 2026-10-02 — Audit EEST lié aux fixtures exactes

[Auditeur et contrôles adverses](qualification/runtime-wan-evidence-gate-20261002/README.md) : 317 SHA-256 de fixtures, 5 387 identités exactes et racines/logs/erreurs/gas disponibles liés au corpus, au-delà des compteurs. Rapport historique exercé sans relancer l’EVM, sept altérations refusées ; aucune qualification courante par héritage. Gate des campagnes complètes renforcé, exit 2 faute de preuves terminales actuelles. Campagnes continuent ; toutes les limites de conformité/forks/WAN/production restent ouvertes.

### 2026-10-02 — Deux builds complets réussis, registres ARM64/AMD64 comparés

[Contrôle réel des registres](qualification/full-crossarch-registry-20261002/README.md) : deux builds exit 0, 427 cas distincts avec mêmes commandes/délais originaux, aucune propriété skip/disabled et mêmes 15 315 sources avant. AMD64 entre en régression complète ; ARM64 continue. Résultats terminaux et invariance après tests non acquis ; audit signé VPS en attente et gate WAN fermé. Aucun critère/EIP de production clôturé par ces builds.

### 2026-10-02 — Suite réelle des campagnes vers le scénario de restauration

[Contrôleur vivant PID Mac 60971/session 3141](qualification/current-release-pipeline-20261002/README.md) : observation des deux campagnes existantes, audits terminaux/copies vérifiées puis audit signé VPS et lanceur privé conditionnel. Scripts figés ; staging frais et tous les hashes source/copie/source comparés avant consumption. Aucun nouveau build, campagne ou nœud WAN démarré. Résultats complets et voie positive/restauration/duties/finalité commune non acquis ; tous les critères et statuts restent ouverts.

### 2026-10-02 — Bloc EEST courant ARM64 audité, contrôleur CLI corrigé

[Bloc EEST terminé du candidat `0f53b42d`](qualification/current-arm-eest-partial-20261002/README.md) : 3,45/30 s ; 5 387 variantes exactes/317 fixtures, 4 882 exécution/505 validation, sources actuelles et runner liés au manifeste. Snapshot provisoire de la campagne complète, invariance terminale/AMD64/WAN non acquis. [Contrôleur v2](qualification/current-release-pipeline-v2-20261002/README.md) : corrige le caller CLI Node `/tmp` → chemin canonique après résultat vide réel ; ancien contrôleur seul arrêté exit 143, nouveau PID 61450/session 34463 vivant. Campagnes et audit VPS inchangés, aucun test relancé, aucun critère/EIP global clôturé.


### 2026-10-02 — Régression ARM64 0f53b42d terminale

[Preuves terminales](qualification/full-arm-terminal-0f53b42d-20261002/README.md) : **427/427 réussis, zéro échec/skip, 5652,20 s**, délais originaux. Auditeur séparé terminé exit 0 : 15315 sources, 208 programmes et entrées de liaison inchangés avant/après, registre complet vérifié. Deux audits du journal terminal confirment les 5387 variantes EEST exactes (4882 exécutions/505 validations, 317 fichiers) liées aux fixtures. Ce périmètre ne certifie aucun fork complet ni la production. Campagne AMD64 encore active ; audits signés AMD64 et restauration WAN attendus. Provenance native 016ab243 distincte, runtime main inchangé, échecs historiques et tous autres critères conservés.


### 2026-10-02 — Échec de l’admission libp2p Altair AMD64

[Diagnostic préservé](qualification/amd-altair-ingress-failure-20261002/README.md) : test 287 `consensus_altair_libp2p` échoué en 2,37 s sur le bloc à la taille maximale configurée. Trois programmes inchangés par rapport au manifeste initial ; réponse exacte absente, cause non prouvée. La campagne continue sans relance, mais son gate ne peut plus autoriser la restauration WAN. Candidat séparé e9ae45ee : diagnostic seulement, assertion/limites/délais/runtime inchangés, non exécuté. ARM64 427/427 conservé ; aucun statut EIP global ni critère de production fermé.


### 2026-10-02 — Collecte diagnostique AMD64 en attente vérifiée

[Contrôleur et périmètre](qualification/altair-ingress-diagnostic-queue-20261002/README.md) : candidat e8efb812, diagnostic de test uniquement, mêmes assertions/limites/délais. PID VPS 750324 confirmé vivant, attendant le conteneur full 582618 et ses manifestes terminaux. Une seule compilation directe/exécution ciblée sera réalisée ensuite contre les bibliothèques et programmes réseau existants en lecture seule, sans nouvelle image. Réponse exacte et raisons de fin de flux bornées ajoutées ; aucun diagnostic exécuté ni cause prouvée à ce stade. La restauration reste refusée sur la campagne échouée ; toutes les qualifications globales restent ouvertes.


## Terminal AMD64 et diagnostic ingress — 2 octobre 2026

Révision runtime `0f53b42d` : **426/427**, aucun skip ; `consensus_altair_libp2p` échoue. Audit intégral : refus (exit 1), sources/binaires/entrées de linkage stables. Diagnostic séparé `e8efb812` : compilation réussie, échec reproduit, bloc maximal refusé avant nouvelle admission cœur (`dispatch-unavailable`), cause précise encore ouverte. Aucun profil EIP ni gate WAN/production clos. [Rapports et limites](qualification/amd-terminal-and-ingress-diagnostic-20261002/README.md).


## Cause du refus Altair — 2 octobre 2026

Diagnostic : `peer-missing` après échec du sondage de statut. Le pair de qualification réutilisé était périmé : son binaire ne contenait pas le handler présent dans la source. Recompiler uniquement ce pair depuis `0f53b42d` fait réussir le même cas, daemon/cœur originaux inchangés : bloc maximal 8 Mio, 2 blocs officiels, 9 pairs distincts. Aucun assouplissement runtime. La campagne originale demeure **426/427** ; la cohérence sources/binaires réseau doit être corrigée puis qualifiée sur les deux architectures. [Preuves et portée](qualification/altair-stale-probe-cause-20261002/README.md). Aucun gate EIP/WAN/production global clos.

Le contrôleur `scripts/ci/full-docker.sh` compile désormais tous les binaires réseau depuis le même checkout dans un sous-répertoire neuf de la campagne, puis configure le cœur avec ce daemon. Il refuse un répertoire déjà configuré et conserve le cache CMake réseau. Syntaxe shell vérifiée ; exécution Docker complète de ce contrôleur corrigé encore à qualifier. Les campagnes précédentes restent intactes.


## Campagne réseau cohérente lancée — 2 octobre 2026

Candidat isolé `9e9ebed3` : runtime `0f53b42d`, correction de portabilité native, contrôleur compilant daemon/helpers depuis ce même checkout. Docker ARM (UID 501:20) : réseau compilé, cœur en compilation ; Docker VPS AMD (UID 1000:1000) : réseau en compilation. Images/volumes réutilisés, répertoires neufs ; aucune réussite de régression encore annoncée. Sélection réseau autonome, audits EEST complets, reprise WAN et autres critères restent ouverts. [Contrôleurs, sources figées et observations initiales](qualification/coherent-network-campaign-start-20261002/README.md).


## Réseau natif et complément Docker — 2 octobre 2026

Candidat `9e9ebed3` : **21/21 tests réseau autonomes natifs qualifiés**, 23 hashes d’exécutables stables, sélection nommée conforme et timeouts explicites conservés. L’essai précédent avait réussi ses tests mais son audit BSD était invalide (capture vide) ; il reste refusé et archivé. Le cœur natif existant (13 cas + 4 restaurations signées) a été réaudité, sans rejouer ces tests ; seul le contrôleur CI diffère entre ses sources et le candidat. Deux suppléments Docker de 21 tests attendent les résultats/audits complets des campagnes de 427. Aucun test WAN/finalité distribuée ni gate production global clos. [Preuves, refus et contrôleurs](qualification/coherent-native-network-and-supplement-20261002/README.md).


## Sélection des artefacts WAN corrigée — 2 octobre 2026

Pilote uniquement : sélection explicite du daemon depuis le build de chaque architecture, traduction des argv et du JSON VPS, chemins natifs indépendants et validés avant appels outils. **15 tests de chemins réussis**, dont 4 nouveaux ; 2 exécutions de refus avant Git/SSH/Docker vérifiées. Runtime C++ inchangé. [Preuves et paramètres préparés](qualification/coherent-wan-artifact-selection-20261002/README.md). Campagnes 427/21, audits EEST exacts, rejeu signé AMD4 et attestation des binaires encore requis avant lancement de la restauration WAN ; aucune finalité/duty nouvelle ni production globale qualifiée.


### 2026-10-02 — Contrôle terminal du build réseau cohérent préparé

[Contrôleur et preuves de refus](qualification/coherent-terminal-review-controls-20261002/README.md) : 15 contrôles synthétiques passent (1 positif, 14 refus), avec comparaison des commandes/timeouts des 427 cas, manifests source/programmes, helpers réseau et résultats JUnit. Le contrôle réel refuse la campagne ARM encore active faute de manifeste terminal. Le parcours terminal préparé ajoute les fixtures EEST exactes, les 21 tests réseau et une relecture des SHA du volume en lecture seule. Aucun résultat terminal acquis, aucune couverture runtime ajoutée par ces contrôles, aucun WAN lancé ni runtime promu. Les deux processus de campagne et les deux files réseau ont été confirmés actifs. Tous les critères de production précédents restent ouverts.


### 2026-10-02 — Transport WAN IPv6 avec identité VPS conservée

[Contrôleur SSH/SCP et essais](qualification/coherent-wan-pinned-transport-20261002/README.md) : 12 contrôles passent, deux refus réels de configuration non autorisée (exit 64), lectures SSH et SCP réelles réussies (exit 0). Le seul endpoint autorisé passe en IPv6 en conservant `HostKeyAlias` et la vérification stricte de l’empreinte connue. Aucun logiciel transféré par SCP, aucun nœud WAN lancé, aucun runtime promu. La finalité/restauration WAN, les campagnes terminales et tous les autres critères de production restent ouverts.


### 2026-10-02 — Audit signé AMD64 préparé pour le binaire cohérent

[Contrôleur et requêtes](qualification/coherent-amd-signed-audit-control-20261002/README.md) : attente des propriétaires réels et codes terminaux, revue complète/EEST exact/réseau/SHA, puis quatre requêtes conservées sur la fixture signée 99 blocs/97 transactions. Pas de compilation ni de modification de fixture, limites 180 s inchangées ; ces cas requalifient le nouveau binaire sans ajouter de scénario historique. Aucun résultat signé acquis à ce stade, aucune reprise des duties/finalité WAN prouvée. Les campagnes existantes et tous les gates de production restent ouverts.


### 2026-10-02 — Lanceur de restauration cohérent préparé, exécution refusée avant gates

[Code et inspection en lecture seule](qualification/coherent-wan-launch-control-20261002/README.md) : raccordement au cœur et réseau qualifiés séparément, contrôles terminaux des deux architectures, revue signée indépendante, SHA et sources avant/après, transport IPv6 à empreinte conservée. Lanceur non exécuté/non qualifié : approbation automatique refusée avant gates, aucune commande lancée. Le contrôleur séparé en lecture seule constate que les résultats terminaux sont encore absents. Aucun nœud WAN démarré ni runtime promu. Tous les gates et critères de production restent ouverts.


### 2026-10-02 — Bellatrix : coupe locale et reprise réussies dans la campagne ARM

[Trace CTest complète du cas](qualification/bellatrix-partition-pass-9e9ebed3-20261002/README.md) : **372,25 s / 720 s**, deux cœurs/deux P2P, un hôte, horloge réelle. SIGSTOP des deux P2P pendant 96 s, branches concurrentes exigées, finalité des moitiés isolées inchangée ; convergence des projections Beacon/exécution après SIGCONT en 95 s / fenêtre 240 s. Aucun délai augmenté ou cas relancé. Campagne encore active, intégrité post-campagne à vérifier ; aucune qualification WAN/asymétrique, panne physique ou restauration trois propriétaires. Tous les gates globaux et critères de production restent ouverts.


### 2026-10-02 — Qualification terminale du candidat cohérent Docker ARM64

[Preuves terminales et revue indépendante](qualification/coherent-arm64-terminal-9e9ebed3-20261002/README.md) : **427/427 + 21/21**, aucun skip/échec, délais originaux. Revue exit 0 : 15 315 sources stables, 208 SHA de programmes relus du volume, 317 fichiers / 5 387 variantes EEST exactes. Cette entrée actualise les observations ARM « encore active » précédentes sans les effacer. Régression après recompilation cohérente des helpers ; aucune couverture nouvelle revendiquée. AMD64/audit signé encore actifs, restauration/duties/finalité WAN non prouvés, liaison statique non auditée ici, conformité complète et revue de sécurité indépendante ouvertes. Aucun runtime promu ; tous les critères globaux de production restent ouverts.


### 2026-10-02 — Échec AMD64 WebSocket découvert dans le journal complet

[Trace et diagnostic](qualification/coherent-amd64-websocket-failure-20261002/README.md) : cas 43 échoué, `eth_estimateGas pending` refusé (-32005) après 4 006 ms d’attente, limite originale 4 s. Marqueur finalisé avec 16 clients actifs, mais ce scénario exige zéro refus : qualification AMD64 non acquise. Cause profonde ouverte ; aucun délai/assertion assoupli, aucune relance ni correction runtime. Campagne encore active. Les indications antérieures « sans échec » fondées sur la fin du journal étaient incorrectes. Contrôle WAN maintenu fermé ; preuve ARM64 distincte et tous les critères globaux conservés.


### 2026-10-02 — Mesures internes du cas WebSocket AMD64 préparées

[Contrôleur ciblé et latences client](qualification/coherent-amd64-websocket-timing-control-20261002/README.md) : un rejeu diagnostique attend le propriétaire réel et les preuves terminales de la campagne, conserve programmes/charge/délais et active seulement les traces internes existantes. Aucun résultat acquis ni correction runtime. L’échec d’origine reste conservé ; aucun gate WAN ou critère global de production fermé.


### Nettoyage Docker demandé — 2026-10-02

Caches de build et ressources Docker inutilisées nettoyés sur Mac/VPS ; volumes et conteneurs actifs conservés en attente de clarification sur leur suppression. Rapports des commandes : Mac 20 GB + 6.438 GB, VPS 7.98 GB + 1.264 GB récupérés. Image de compilation ARM précédente absente après nettoyage ; volume `nano-linux-sync-build` conservé. Les preuves ARM archivées restent historiques, mais leur réutilisation opérationnelle nécessite une nouvelle image identifiée. Aucun nouveau build démarré. Campagne VPS toujours active : 388 succès / 389 terminés et échec RPC conservé, 38 cas restants. Production non qualifiée ; tous les critères précédents restent ouverts selon leurs statuts. Preuves : [docker-cleanup-20261002](qualification/docker-cleanup-20261002/README.md).


### 2026-10-02 — Remise à zéro Docker complète autorisée

Après demande explicite de nettoyage de A à Z, conteneurs/images/volumes/caches supprimés sur Mac et VPS : `docker system df` affiche 0B dans les quatre catégories. Campagne VPS interrompue volontairement à 393 cas terminés : 392 succès, échec RPC conservé et 34 cas non terminés. Contrôleurs en attente arrêtés ; diagnostic RPC non exécuté. Rapports hors Docker et preuves Git conservés. Les anciennes images et compilations Docker ne sont plus disponibles pour des contrôles en direct ; reconstruction identifiée requise avant reprise. Aucun nouveau build ni lancement public démarré ; production non qualifiée. [Preuves du reset](qualification/docker-cleanup-20261002/full-reset.json).


### 2026-10-02 — Reconstruction propre et diagnostic RPC ciblé

Après remise à zéro Docker, build des dépendances AMD64 effectivement démarré sur le VPS (PID 1204418, source `9e9ebed3`). [Contrôleur préparé](qualification/clean-rpc-build-control-20261002/README.md) : compilation fraîche de Nano/P2P/test RPC, puis cas Paris WebSocket inchangé, délai original 480 s, traces internes existantes, empreintes sources/programmes avant/après. Syntaxe Python validée ; résultat de compilation/diagnostic non acquis. Échec RPC historique et campagne interrompue conservés ; aucune qualification complète ou production revendiquée.


### 2026-10-02 — Diagnostic RPC propre terminé, échec historique ouvert

Compilation AMD64 Docker neuve de Nano/P2P/banc réseau réussie ; cas Paris WebSocket original instrumenté PASS en 80.91 s. Audit : 15 315 empreintes sources et trois programmes inchangés. Attente WebSocket maximale 3421.725 ms sur limite 4000 ms ; estimation maximale 3426.193 ms. Marge réduite observée, cause de l'échec historique non établie, aucune correction runtime ni régression complète nouvelle. Image/volume neufs conservés pour réutilisation. Les critères WAN, restauration effective des duties, forks et production restent ouverts. [Preuves et limites](qualification/clean-rpc-diagnostic-20261002/README.md).


### 2026-10-02 — Instrumentation des sondes d'estimation préparée

Candidate séparée `3fa8b805` : seules les mesures opt-in des simulations de l'estimateur changent ; recherche, budgets et assertions conservés. [Contrôleur](qualification/rpc-probe-build-control-20261002/README.md) préparé pour réutiliser image/volume AMD64 et compiler les cibles nécessaires, sans reconstruire une image. Pas encore de résultat ; échec historique, régression complète, restauration WAN et production restent ouverts. Aucun changement runtime promu sur main.


### 2026-10-02 — Coût des sondes RPC mesuré, optimisation non qualifiée

Candidate de diagnostic `3fa8b805`, compilation incrémentale Docker AMD64 sans nouvelle image, cas Paris WebSocket original PASS 82.05 s. Les 192 estimations effectuent cinq simulations chacune ; médiane totale 2103.730 ms, simulations individuelles environ 394–416 ms en médiane. Préparation médiane 3.713 ms ; attente WebSocket maximum 3197.998 ms. Source et trois programmes stables audités. Coût des sondes identifié pour ce scénario, cause de l'expiration historique non démontrée ; aucune optimisation promue sur main, régression complète ou production qualifiée. Le volume actuel contient désormais la candidate instrumentée. [Preuves et portée](qualification/rpc-probe-diagnostic-20261002/README.md).


### 2026-10-02 — Optimisation de l'intervalle de gaz préparée

Candidate séparée `d1597d9a` : sonde proche de la borne haute limitée par la tolérance existante 1,5 %, avec succès réellement vérifié et fallback conservé. Nouveaux cas Paris/Shanghai 2/4/8 profondeurs et plafonds serrés, contrôle négatif contre l'algorithme précédent. [Contrôleur Docker incrémental](qualification/rpc-headroom-control-20261002/README.md) syntaxiquement vérifié ; résultats non acquis. Aucune optimisation promue sur main, clôture de l'échec historique, régression complète, WAN ou production revendiquée.


### 2026-10-02 — Optimisation RPC qualifiée sur tranche AMD64

Candidate `d1597d9a` : contrôle négatif de performance rejette l'ancien algorithme ; RPC optimisé PASS 2.90 s / 1053 vérifications, nouvelles profondeurs Paris/Shanghai 2/4/8, gaz minimal indépendant et plafonds serrés. Cas réseau WebSocket Paris original PASS 71.03 s : 192 estimations, quatre simulations chacune contre cinq auparavant, médiane observée 1620.740 ms, file maximum 2595.770 ms (limite 4000 ms conservée). Sources et quatre programmes stables audités, image/volume réutilisés. Qualification bornée instrumentée, candidate non promue sur main ; origine de l'expiration historique, régression complète sans instrumentation, deux architectures, restauration des duties, WAN et production encore ouverts. [Preuves et limites](qualification/rpc-headroom-result-20261002/README.md).


### 2026-10-02 — Régression cohérente complète après correction RPC préparée

Candidate `d1597d9a` : [contrôleur](qualification/rpc-full-regression-control-20261002/README.md) préparé pour réutiliser image/volume VPS, compiler cœur/P2P/helpers dans un nouveau sous-dossier cohérent puis exécuter toute la sélection CTest sans instrumentation. Résultat et sélection terminale non acquis ; supplément réseau, autre architecture, restauration des duties, WAN et critères production toujours ouverts. Pas de nouvelle image VPS ni de promotion runtime sur main.


### 2026-10-02 — Régression complète VPS et dépendances Mac effectivement démarrées

Candidate `d1597d9a` : PID VPS 1440584 et conteneur de préparation vérifiés en vie, compilation fraîche cohérente P2P/helpers dans volume réutilisé ; tests complets sans instrumentation pas encore démarrés. Mac ARM64 : PID Docker 83863 / BuildKit `hgesiw1wk0njw3i54awcf449a` Running, reconstruction d'une image de dépendances après reset, Hunter/cpp-libp2p en préparation. [Receipts de démarrage et logs](qualification/rpc-full-regression-start-20261002/README.md). Aucun résultat complet, autre architecture, restauration des duties, WAN ou production acquis ; aucun runtime promu sur main et échec historique conservé.


### 2026-10-02 — Enchaînement ARM64 vers régression complète préparé

[Contrôleur Mac](qualification/rpc-full-arm64-control-20261002/README.md) syntaxiquement vérifié : attend le PID réel du build de dépendances, refuse sa réutilisation, exige une image ARM64 finale identifiable et les 15 315 sources inchangées, puis compile/qualifie la candidate `d1597d9a` avec le script complet Git sans instrumentation RPC. Une image/un volume requis après reset Mac ; aucun résultat de régression complète ou production encore acquis, tous les autres gates conservés.


### 2026-10-02 — Image ARM64 disponible, compilation cohérente démarrée

Image Mac figée `sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb`, ARM64/Linux ; export/tag/unpack terminés et 15 315 sources inchangées vérifiés. PID dépendances absent ; code de retour direct non capturé, aucune supposition de statut. Contrôleur 84088 et conteneur de préparation ARM64 vérifiés en vie sur la candidate `d1597d9a` : P2P/helpers compilés, cœur en compilation, régression complète non encore acquise. [Preuves](qualification/rpc-arm64-dependency-result-20261002/README.md). Compilation VPS parallèle active ; autres gates de production toujours ouverts.


### 2026-10-02 — Matériel de restauration complet conservé et re-vérifié

Fixture locale hors Docker : 313 fichiers / 121 445 250 octets, empreintes identiques aux manifests arrêtés source/restore historiques ; archive complète vérifiée. Chaîne, témoin cœur, 21 paires journal/témoin et clé P2P présents. Genesis et requête externe séparée retrouvés sur hôte et empreintes originales correspondantes ; anciens chemins/binaire Docker disparus après reset. [Inventaire](qualification/surviving-full-backup-inventory-20261002/README.md). Conservation des bytes prouvée, pas une restauration contre la nouvelle candidate, concordance de tous les engagements indépendants, reprise des duties ou finalité WAN. Tous les gates restants maintenus.


### 2026-10-02 — Contrôle terminal de la candidate RPC préparé

[Contrôle indépendant](qualification/rpc-terminal-review-control-20261002/README.md) adapté à `d1597d9a` avec référence exacte des 15 315 sources et critères inchangés : 427 cas uniques exécutés, aucun échec/skip, délais originaux, sources et programmes stables. 15 contrôles synthétiques passent ; le rapport ARM64 réel incomplet est refusé comme attendu. Aucun résultat terminal, supplément réseau ou lancement WAN encore acquis ; tous les gates et échecs historiques restent ouverts.


### 2026-10-02 — Supplément réseau préparé, garde du chemin corrigé

[Supplément](qualification/rpc-network-supplement-control-20261002/README.md) lié aux images/volumes existants de `d1597d9a`, conditionné à la réussite terminale stricte des 427 cas et à la relecture des programmes. Préflight du rapport réel incomplet refusé avant Docker ; aucun supplément lancé. Wrapper de revue corrigé : chemin de build courant et refus explicite des anciens chemins sous Bash Mac ; 17 contrôles passent. Aucun gate WAN/production fermé, preuves et échecs historiques conservés.


### 2026-10-02 — Engagements indépendants de la sauvegarde retrouvés

[Entrées vérifiées](qualification/surviving-full-backup-pins-20261002/README.md) : reçu d’arrêt original correspondant à la sauvegarde complète ARM64, engagement cœur identique à la requête externe, 21 pins de clés compatibles avec la configuration conservée et les paires journal/témoin. Profil/ancre et limites correspondants ; requête préparée exige le checkpoint finalisé sauvegardé. Aucun pin dérivé de RocksDB, aucun secret publié. Contenu des journaux non encore audité par la nouvelle candidate, duties et finalité WAN non repris ; tous les gates restent ouverts.


### 2026-10-02 — Préparation du propriétaire natif Mac de la candidate RPC

[Contrôleur](qualification/rpc-native-control-20261002/README.md) syntaxiquement vérifié pour `d1597d9a` : build natif neuf, helpers cohérents, dépendances existantes, deux jobs, 14 cas natifs (13 antérieurs + RPC modifié) et 21 cas réseau avec délais d’origine. Manifests sources et entrées de liaison/dépendances disponibles contrôlés ; dyld système non intégralement audité. Aucun résultat acquis ni lancement WAN, gates de production et échecs historiques conservés.


### 2026-10-02 — Nouveau natif qualifié et fixture signée auditée

[Preuves](qualification/rpc-native-result-20261002/README.md) pour `d1597d9a` : 14 cas natifs + 21 réseau réussis sans skip, délais originaux ; relecture indépendante des 15 315 sources et 598 entrées stables. Audit signé en sandbox : 99 blocs / 97 transactions, checkpoint finalisé et engagement cœur externes correspondants ; trois requêtes invalides refusées, 313 fichiers de la fixture inchangés. Pas de régression native complète/dyld intégral, ni audit du contenu des journaux des signataires, reprise des duties, historique complet votes/fork-choice ou WAN. Régressions Docker en cours ; tous les gates et échecs historiques conservés.


### 2026-10-02 — Pilote WAN compatible avec les volumes conservés

[Candidate JS](qualification/rpc-wan-harness-selectors-20261002/README.md) `c7f759bb` basée sur `d1597d9a`, trois fichiers JS seulement : images immuables et volumes existants sélectionnés explicitement par architecture, montage de build traduit et aucun montage de données réécrit. 20 tests passent et trois refus du pilote réel interviennent avant Docker/SSH/SCP. Premier échec de texte d’erreur conservé puis corrigé. Aucun changement C++ ni lancement WAN ; tous les gates terminaux/production et échecs historiques maintenus.


### 2026-10-02 — Audit signé AMD64 préparé pour le nouveau binaire

[Contrôleur terminal](qualification/rpc-amd-signed-audit-control-20261002/README.md) d159 adapté aux image/volume/sous-dossier conservés, quatre demandes et référence de fixture inchangées. Exige contrôleur complet, 427 cas, supplément21 et revue réseau à zéro, puis revues exact EEST/sources/programmes avant audit en lecture seule. Syntaxe vérifiée, aucun audit AMD64 ou WAN lancé ; copie de chaîne non équivalente aux journaux complets pour reprise. Tous les gates de production et échecs historiques maintenus.


### 2026-10-02 — Régression complète ARM64 de la candidate RPC acquise

[Preuves terminales](qualification/rpc-full-arm64-terminal-result-20261002/README.md) `d1597d9a` : 427/427 sans échec/skip, 5 805,22 s, statuts terminaux zéro ; supplément21 réussi. Registre/commandes/délais d’origine préservés, sources 15 315 et programmes 208 relus dans le volume et concordants. Corpus EEST exact sélectionné 317 fichiers / 5 387 variantes vérifié ; pas conformité exhaustive de fork, ni réexécution indépendante des booléens hash/reçus, ni audit intégral des entrées statiques Docker. Sortie/retrait réseau/reprise de données bornée qualifiée sur même hôte, pas WAN. AMD64 encore en cours ; échec historique WebSocket conservé, aucun runtime promu sur main, aucun duty restauré démarré. Tous les autres gates de production demeurent ouverts.


### 2026-10-02 — Rejeu signé Linux ARM64 confirmé

[Audit Docker ARM64](qualification/rpc-arm-signed-result-20261002/README.md) : quatre contrôles réussis sur le binaire des 427 tests, revue terminale préalable et indépendante, 99 blocs / 97 transactions signées, engagements externes concordants et trois rejets attendus. Sources et neuf fichiers de chaîne inchangés, lecture seule sans réseau, clés/journaux non montés. Contrôles existants rejoués sur Linux ARM64 ; pas de nouvelle couverture historique des votes/fork-choice, de reprise des duties ou de WAN. AMD64 en cours ; aucun runtime promu et gates de production maintenus.


### 2026-10-02 — Lanceur de restauration raccordé aux artefacts RPC

[Préparation WAN](qualification/rpc-wan-launch-control-20261002/README.md) : sélection explicite des images/volumes actuels par architecture et pilote JS c7f759bb, gates terminaux renforcés et aucune création de nœud avant les revues. Gate natif indépendant : 14+21+4 réussis, sources 15 315 / entrées 598 / sauvegarde complète 313 fichiers relus. Quatre contrôles sans action externe passent. Campagne AMD64 toujours active ; aucun lancement WAN ni reprise de duties. Le pilote prévoit une nouvelle sauvegarde complète arrêtée, pas la reprise de l’ancienne fixture Mac. Tous les critères de production et échecs historiques conservés.


### 2026-10-02 — Finalité du reçu d’arrêt exigée avant les duties

[Correction du pilote](qualification/rpc-stopped-finality-gate-20261002/README.md) `7434452d` : la requête de restauration exige désormais la finalité externe sauvegardée et vérifie l’audit complet avant redémarrage, au lieu du seul statut canonicalHistoryVerified. 44 contrôles réussis, six nouveaux refus/liaison démontrés par contrôle négatif sur l’ancien pilote ; requête du véritable reçu/configuration liée aux pins déjà qualifiés. Aucun runtime C++, budget ou délai modifié ; pas de reprise WAN acquise. Lanceur adapté au nouveau pilote, gates AMD64 encore ouverts, tous les critères de production conservés.

### 2026-10-04 — Régression Docker AMD64 terminale et audit signé

[Preuves terminales](qualification/rpc-full-amd64-terminal-result-20261004/README.md) `d1597d9a` : 427/427 sans échec/skip, 12 116,38 s ; supplément réseau21 réussi, statuts terminaux et revues zéro. Sources 15 315 et programmes 208 relus et concordants, délais originaux conservés. Corpus EEST sélectionné exact : 317 fichiers / 5 387 variantes ; pas conformité exhaustive de fork ni réexécution indépendante des booléens hash/reçus ou audit intégral des entrées statiques Docker. Audit signé lecture seule : 99 blocs / 97 transactions, pins externes concordants, trois rejets attendus, sources/neuf fichiers inchangés. Pas de reprise des duties, historique complet des votes/fork-choice ni nouveau WAN. Runtime non promu ; échecs historiques conservés. Prochaine étape : restauration complète et finalité commune privée des trois environnements. Tous les critères originaux de production restent ouverts.

### 2026-10-04 — Échec du tunnel privé conservé et transport corrigé

[Preuves et correction](qualification/rpc-wan-tunnel-fix-20261004/README.md) : gates terminaux/relectures réussis puis tentative WAN arrêtée sur le bind IPv4 local de SSH `-6`, avant démarrage des deux pairs Mac. Runtime inchangé, aucun restore/duties/finalité WAN qualifié. Nouveau wrapper `AddressFamily=any` vers l'adresse IPv6 littérale avec même clé d'hôte imposée ; 15 contrôles réussis, six refus démontrés sur l'ancien wrapper et forwarding privé live corrigé. Ancienne tentative conservée, nouveau dossier réservé ; aucune modification C++, de quorum ou de délai. Tous les critères de production demeurent ouverts.

### 2026-10-04 — Restauration complète privée et reprise de finalité qualifiées

[Preuves et limites](qualification/rpc-wan-restore-result-20261004/README.md) `d1597d9a` : nœud VPS complet restauré avec 21 paires journal/témoin et identité P2P, engagement/finalité externes audités avant reprise. Finalité commune 2→3 en 35 624 ms (limite 180 000 ms), stabilité 3→6 en 139 215 ms. VPS restauré sain : 13 propositions / 82 attestations. Audit terminal lecture seule : 61 blocs / une transaction, checkpoint6 et ascendance vérifiés ; 443 fichiers restaurés et source originale inchangés. Pilote initial exit1 sur variable `prepared` hors portée conservé ; audit terminal complété séparément sans rejeu réseau, correction JS 97a6b4de syntaxiquement vérifiée, prochain usage complet à confirmer. Aucun changement C++, délai ou quorum ; runtime non promu. Restauration bornée acquise, pas ancienne fixture historique, charge/partitions/asymétrie/échelle ni votes/fork-choice exhaustifs. Prochaine couverture : majorité connectée pendant isolation VPS, puis critères originaux restants. Production non qualifiée.

### 2026-10-04 — Nouvelle couverture de majorité connectée préparée

[Contrôleur](qualification/rpc-majority-launch-control-20261004/README.md) pour runtime d159 et pilote ba7bac4f : mêmes artefacts immuables/volumes qualifiés, aucun rebuild. Adaptation du pilote limitée aux artefacts conservés et contrôle de montage en lecture seule ; 40 contrôles passent dont cinq nouveaux, quatre contrôles de gates sans action externe. L'île Mac doit démontrer >2/3 du stake SSZ et un lien direct pendant la coupure VPS de 192 s ; minorité bornée aux engagements reçus, reprise monotone 180 s et stabilité trois époques inchangées. Aucun résultat WAN de ce scénario encore acquis ; critères de production et échecs historiques conservés.

### 2026-10-04 — majorité WAN : échec conservé, diagnostic ouvert

[Preuves](qualification/rpc-majority-result-20261004/README.md) runtime d159 / pilote ba7 : stake effectif 43/64, lien direct Mac conservé, VPS isolé 192 s sans pairs. Majorité finalisée 2→4, justification6, tête64 ; minorité reste2. Seuil conservateur5 non dépassé : exit1, hashes runtime/sources inchangés. Reconnexion effectuée en finally, signataires terminaux sains ; reprise/stabilité non qualifiées car arrêt après la gate échouée. Cause non établie : reconstruire participation et inclusion par époque depuis les états persistés avant tout nouveau run ou correction. Aucun seuil/quorum/délai assoupli. Échecs et tous les critères originaux de production conservés. Caches Docker Mac/VPS nettoyés ; images et volumes conservés en attente de confirmation explicite, sauvegardes hors Docker vérifiées. Production non qualifiée.

### 2026-10-04 — votes inclus majorité et portabilité du stockage

[Inspection arrêtée](qualification/rpc-majority-vote-inspection-20261004/README.md) : 52 blocs, checkpoint externe concordant, 331 fichiers inchangés. Époques5/6/7 : 43 signataires BLS distincts sur cible canonique, mais38/35/39 inclus avant la frontière suivante ; autres inclus dans le bloc frontière, après traitement d’époque. Priorité : vérifier la chronologie FFG/inclusion précédente avant correction ; cause unique non établie, échec192s conservé. Inspecteur Docker compilé, lecture SST Mac refusée par RocksDB Linux format7 ; inspecteur natif lié à d159 réussi. Portabilité physique des sauvegardes Mac→Linux non qualifiée et désormais explicitement ouverte. Aucun nouveau réseau ni assouplissement, critères originaux et production non qualifiée conservés.

### 2026-10-04 — correction SST portable intégrée, périmètre borné

[Correction et preuves](qualification/rocksdb-portable-format-20261004/README.md) : nouveau format SST6 explicitement fixé dans ChainStore, minimum lecteur RocksDB8.6, protections de footer/WAL sync conservées. Deux bases neuves avec vrais SST passent Mac→Docker Linux8.9 et sens inverse, checkpoints identiques/refus écritures,20 fichiers inchangés. Reprise WAL native après arrêt brutal passe avec storage.cpp corrigé. Compilation native/ARM64 Docker directe contre bibliothèques d159 gelées, sans rebuild image. Anciennes bases format7 non migrées ; AMD64 et régression du runtime révisé restent à qualifier, d159427 non transféré. Majorité WAN/FFG et toutes les exigences originales, bridge/lancement/revue restent ouvertes. Production non qualifiée.

### 2026-10-04 — FFG persisté concordant, couverture prolongée préparée

[États/contrôle indépendant](qualification/rpc-majority-ffg-inspection-20261004/README.md) : frontières40/48/56/64 conformes au calcul FFG séparé et aux blocs post-frontière persistés, racines comprises ;331 fichiers inchangés. Derniers quorums courants38/35/39, quorums précédents50/43/43 : justification6/finalité4 reproduites, aucune divergence FFG démontrée sur ce périmètre. Échec192s conservé. [Contrôleur prolongé](qualification/rpc-majority-sustained-control-20261004/README.md)384s distinct, seules identité sortie/scénario/durée coupure changées ; seuil minoritaire, reprise180s/stabilité3époques et toutes gates inchangés. Runtime d159 gelé, correction SST main séparée ; résultat réseau non acquis, votes/fork-choice exhaustifs et tous critères originaux ouverts.

### 2026-10-04 — ancienne base SST7 migrée sur copie et relue sous Linux

[Preuve](qualification/rocksdb-format7-migration-20261004/README.md) :46 DB copiées sous verrous exclusifs, WAL sync/flush/compaction format6,331 fichiers originaux inchangés. Linux8.9 relit exactement toutes clés/valeurs chaîne+cœur+22 paires journal/témoin,461 fichiers copiés inchangés. Reactor d159 lié au checkpoint externe retrouve exactement les52 blocs/votes de l’inspection native. Refus SST7 levé sur cette copie, pas migration générale ni duties repris ; utilitaire production/reprise migrée et vérification des engagements signataires restent ouverts. Erreur Python absent/priority nice indisponible conservées, aucun gain causal ni nouveau critère de clôture. Campagne WAN prolongée toujours distincte et active, critères originaux/production non qualifiée conservés.

### 2026-10-04 — majorité connectée384s, reprise et stabilité qualifiées

[Résultat terminal](qualification/rpc-majority-sustained-result-20261004/README.md) d159/ba7, exit0, hashes sources/runtime inchangés.43/64stake Mac directement connectés pendant isolation VPS384s ; finalité majorité6→7 dépasse seuil5 fixé, minorité reste2. Reprise commune9 en76666,176ms /180000ms inchangés. Sauts10→12/12→14 non crédités, fenêtre3avancées14→17 passe (observateur256665ms). Signataires sains393/376/373attestations, conteneurs exit0 sans OOM. Trois processus/deux hôtes/64 validateurs synthétiques, pas réseau massif/public. Ancien192s demeure échoué ; aucune cause/gain causal ou qualification des branches restauration non exercées déduits. Correction SST main séparée ; asymétrie/lifecycle/charge/intégration candidate et tous critères originaux restent ouverts. Production non qualifiée.

### 2026-10-04 — asymétrie prolongée : pilote durci, campagne préparée

[Contrôleur/preuves](qualification/rpc-asymmetric-launch-control-20261004/README.md) : helper utilise horloge monotone et refuse lecture qdisc/RPC terminée hors budget, vérifie réel500ms/zero jitter/corrélation.27 contrôles passent, deux nouveaux cas tardifs refusent ancien helper ; probe isolé VPS confirme réglage0,5s et restauration sans espace réseau hôte touché. Campagne d159/pilote d7c : hold384s + marge progression144s, finalité commune>=2époques sous délai, puis stabilité3époques/santé/hashes inchangés. Aucun résultat réseau encore acquis ; main SST séparé et tous critères originaux ouverts.

### 2026-10-04 — asymétrie500ms qualifiée ; candidat cohérent en compilation Docker

[Résultat terminal](qualification/rpc-asymmetric-sustained-result-20261004/README.md) d159/d7c, exit0/hashes inchangés. Trois processus/deux hôtes/64 validateurs synthétiques : finalité commune2→8 sous netem500ms effectif pendant388606,712ms (hold384s, budget528s),5128paquets/drops0. Délai retiré ; sauts8→10/10→12 non crédités, stabilité3avancées12→15 en222580ms. Signataires355/341/343attestations sains, conteneurs exit0 sans OOM. Cette portée est fixe et privée ; SST6 absent de ces binaires, autres critères originaux conservés.

[Intégration candidate](qualification/production-coherent-build-control-20261004/README.md) efbeb9bc sur branche isolée poussée/fetch Git VPS : runtime/tests/vendor d159 +SST6 +pilote d7c, documents main conservés. Compilation C++/P2P/helpers cohérente démarrée dans Docker ARM64 et AMD64, images/volumes existants, nouveaux dossiers sans écrasement ; régression complète et manifests suivront. Aucun résultat terminal candidat ni promotion main/production acquis. Répétition de fautes/lifecycle/charge/reprise migrée/profil public/bridge/fair launch/revue indépendante et tous critères initiaux restent ouverts.

### 2026-10-04 — compilation ARM64 terminée, régression active et qualification native démarrée

Candidate efbeb9bc : Docker ARM64 atteint compilation100% et lance les427cas enregistrés ; résultat terminal non acquis. AMD64 compile encore. [Contrôle natif](qualification/production-coherent-native-control-20261004/README.md) même source figée : P2P/helpers/configuration compilés, cœur en compilation ;15cas sélectionnés dont stockage readonly/crash-WAL, manifests entrées/sources, audit JUnit prévus. Aucune régression native complète déduite. [Lecture physique SST6](qualification/production-coherent-sst-cross-control-20261004/README.md) ARM64↔AMD64 préparée, refuse gate build absent, copies synthétiques seulement ; aucun résultat encore. Versions antérieures gardent leur portée, candidat non promu et critères initiaux/production ouverts.

### 2026-10-04 — candidat natif/SST6 et rejeu signé qualifiés à portée explicite

[Preuves efbeb9bc](qualification/production-coherent-native-result-20261004/README.md) : compilation native fraîche puis15cas sélectionnés/21réseau, audit indépendant JUnit/registry/délais,603entrées et16494sources rehashées inchangées. Rejeu hors ligne99blocs/97tx, checkpoint externe attendu, refus3requêtes invalides ;313fichiers fixture historique inchangés, sandbox sans réseau/écriture. Pas rejeu votes autonomes/fork-choice/duties ni régression native complète. Nouveaux helpers liés natif↔DockerARM64 écrivent SSTphysique/optionsformat6 ; lecture croisée réussit,20fichiers et programmes inchangés. AMD64 distinct encore en compilation, full427ARM64 en cours. Aucune promotion candidate ni readiness production ; toutes exigences initiales et limites/revue indépendante demeurent ouvertes.

## Avancement Doppler — 4 octobre 2026

L’outil `tools/doppler-launch/preview.mjs` prépare réellement l’appel Airlock
`create` via le SDK officiel 1.0.43, sans wallet ni diffusion. Configuration
explicite Base/USDC, allocations, inflation, gouvernance et migration V2/V4 ;
calldata et hash vérifiables. Cinq tests ciblés couvrent encodage/décodage,
rejets des paramètres et simulation RPC bornée à un bloc en lecture seule.
La configuration synthétique a été rejetée par `eth_call` sur Base : ce résultat
reste un échec, pas une qualification du lancement. Les termes réels, le lien
Base–NANO, le bridge, les retraits, la visibilité T0 et toutes les autres
exigences de production restent ouverts. Les deux campagnes C++ de la candidate
`efbeb9bc` continuent ; ce lot JavaScript ne change pas leurs sources figées.

## Candidate cohérente : stockage croisé et journaux — 4 octobre 2026

Les programmes `efbeb9bc` passent la lecture physique SST format 6 dans les deux
sens Docker ARM64/AMD64. L’audit natif des 22 journaux originaux et migrés passe
avec pins/témoins indépendants et 66 refus de bindings erronés par fixture,
sans signatures ni reprise de duties. Les deux campagnes complètes continuent.
La restauration effective de cette candidate est mise en attente avec refus
automatique tant que leurs résultats terminaux et manifestes ne passent pas.
[Preuves, contrôleur et limites](qualification/coherent-storage-and-restore-20261004/README.md).
Production non qualifiée ; critères originaux intégralement conservés.

## Doppler : cause du rejet corrigée, création simulée — 4 octobre 2026

Le rejet synthétique `0x248d3d6a` correspond à `InvalidGamma` dans les sources
officielles. Le gamma automatique 120 du SDK ne passe pas la normalisation
entiere du constructeur pour 168 époques. L’outil exige maintenant un gamma
explicite et contrôle cette règle et la divisibilité par tick spacing. Avec
20040 dans la fixture, `eth_call` de création sur Base et `eth_estimateGas`
passent (10 912 762 gas), au même bloc de référence ; les adresses token/hook
retournées correspondent aux prédictions SDK. Les erreurs initiales restent
conservées. Cinq tests ciblés couvrent aussi les bindings RPC et les rejets.
Aucune transaction diffusée : achats, ventes, migration, paramètres économiques
réels, actif Base–Nano et tous les critères originaux restent ouverts.
[Preuves](qualification/doppler-unsigned-preview-20261004/).

## Doppler : achat, revente et migration V2 locaux — 4 octobre 2026

Le scénario réutilisable sur fork Anvil privé de Base termine exit 0 : création,
achat USDC/token, revente, deltas de soldes, produit net et migration V2 avec
événement, monnaies, réserves et LP vérifiés. Chain ID 31337, loopback, fonds
fictifs ; aucun déploiement public. Les deux échecs de harness initiaux restent
conservés. Nanobees est le nom de l’actif de lancement confirmé par l’utilisateur ;
les fixtures TEST ne fixent ni ratio Base/natif, ni réserve, ni gouvernance réelle.
Un achat/revente et migration V2 seulement : bridge, fuzzing, graduation V4,
profil réel de Nano et exigences originales restent ouverts.
[Preuves et limites](qualification/doppler-fork-lifecycle-20261004/README.md).

## Programmes de livraison : recompilation indépendante préparée — 4 octobre 2026

Le contrôleur réutilise les images épinglées et attend les campagnes complètes
puis la restauration réussie avant de reconstruire `nano` et `nano-p2p` dans
des répertoires distincts, sans rebuild d'image. Il compare leurs empreintes
aux programmes qualifiés et vérifie les sources/programmes originaux avant
et après. Conservation des logs d'échec et timeout contrôlée ; 16 494 sources
relues conformes. Reproductibilité effective encore **non démontrée** ; scope
limité aux deux programmes, dépendances de l'image/cache réutilisées.
[Contrôleur, contrôles et limites](qualification/reproducible-delivery-control-20261004/README.md).
Tous les critères et échecs précédents restent ouverts selon leur statut.

## Doppler : refus adversariaux et rollback observé — 4 octobre 2026

Le cycle privé Base/Anvil termine exit 0 avec migration prématurée, transfert
non autorisé et double migration réellement rejetés. Erreurs précises et reçus
reverted contrôlés ; état d'enchère, soldes, réserves et LP inchangés avant/après.
Achat/revente/migration V2 passent dans le même cycle. La première tentative
exit 1 (faute de champ du lanceur) reste conservée. Trois cas déterministes,
pas de fuzzing ni qualification du runtime Nano ou du bridge public.
[Sources exécutées, résultats et limites](qualification/doppler-adversarial-lifecycle-20261004/README.md).
Tous les critères originaux restent conservés selon leur statut.

## Stockage : croissance logique mesurée hors ligne — 4 octobre 2026

Le helper C++ autonome passe sous sandbox sans écritures/réseau : 46 bases,
2 686 457 octets logiques identiques original/migré ; 54 états EVM = 60 985
octets, checkpoint vivant = 2 231 548. Taille apparente des fichiers :
51 403 853 → 5 290 934 ; aucune attribution causale ni borne de croissance.
331/461 fichiers et 603 entrées natives qualifiées inchangés. Refus des chemins
vides/symlinks ; erreur initiale du contrôle négatif conservée. Pruning Capella
vivant existant ; rétention des états historiques EVM/archives encore à traiter
avec l'ancre de rejeu et les contrats RPC/audit, sans suppression isolée.
[Outil, mesures et limites](qualification/storage-growth-accounting-20261004/README.md).
Tous les critères originaux restent conservés selon leur statut.

## Candidate cohérente : ARM64 complète terminale — 4 octobre 2026

427/427 réussis, zéro échec/skip, 5 771,83 s ; contrôleur/CTest/audit/manifeste
exit 0. Revue indépendante : 16 494 sources et 208 programmes réels inchangés.
EEST : 5 387 variantes sélectionnées, aucun fork complet certifié. AMD64 actif ;
restauration, reprise de duties et reproductibilité encore attendues.
[Traces et limites](qualification/coherent-arm64-terminal-20261004/README.md).

## Doppler : lectures par hash canonique — 4 octobre 2026

Cinq tests de reorg/bindings passent, puis cycle privé Anvil complet exit 0 avec
six lectures EIP-1898 liées au même hash canonique. Première revente échouée
pour slippage conservée, timestamps du rejeu explicitement déterministes,
sortie minimale inchangée. Assertion RPC seulement, pas preuve cryptographique
Base ni bridge. [Sources, résultats et limites](qualification/doppler-canonical-hash-proof-20261004/README.md).
Tous les critères originaux et échecs restent conservés selon leur statut.

## Pruning historique : draft isolé — 4 octobre 2026

Branche `feature/finalized-state-retention`, révision `4734f285` :
pruning EVM lié à l’ancre Capella et au checkpoint atomique, replay signé
depuis genesis, paramètres désactivés par défaut. Syntaxe Docker ARM64 exit 0 ;
linkage et sept tests ciblés pas encore exécutés. Nouveaux scénarios de rétention
et de panne WAL conservée/perdue ajoutés ; build mis en file après restauration
et reproductibilité pour éviter un chevauchement. Runtime main et candidate
figée inchangés. Corruptions, RPC, AMD64, ressources, sauvegarde et archives
restent ouverts. [Patch, contrôleur, preuves et échecs](qualification/finalized-state-retention-draft-20261004/README.md).
Aucune exigence ni qualification antérieure supprimée ; production non démontrée.

Complément au draft pruning : onze corruptions du marqueur/ancre/checkpoint
et deux résultats RPC (ancien état supprimé/latest exact) sont ajoutés.
Syntaxe Docker exit 0 ; file complétée avant son premier build. Ces nouveaux
scénarios restent non exécutés, sans promotion du runtime ni clôture des critères.

## Assemblage cohérent des deux binaires — 4 octobre 2026

L’outil de packaging sélectionne désormais explicitement le P2P du build isolé,
au lieu de dépendre du P2P racine historique. Dix précontrôles et trois tests du
vrai shell d’export passent ; absence/symlinks refusés, aucun fallback silencieux.
Pas encore d’assemblage Docker réel ni de cycle de promotion de cette candidate.
[Correction et preuves](qualification/coherent-package-paths-20261004/README.md).
Les exigences antérieures restent ouvertes selon leur statut.

## Bridge : observation de finalité source — 4 octobre 2026

Nouveau module en lecture seule `tools/bridge-source/finality.mjs` : reçu/log/bloc
lié à la tête `finalized` de Base, code émetteur lu par hash canonique EIP-1898,
relectures et refus des reorgs/régressions/bindings inattendus. Vingt-quatre tests
avec RPC injecté passent et sont intégrés à la CI d’outillage. Pas de dépôt réel,
preuve cryptographique, journal idempotent, contrats lock/mint ou relayer livrés.
Le résultat interdit tout mint (`eligibleForMint:false`) ; l’assertion RPC seule
ne clôt pas la sécurité du bridge ni les termes Base–Nano.
[Module, tests et limites](qualification/base-source-finality-gate-20261004/README.md).
Toutes les exigences précédentes restent conservées selon leur statut.

## WAN A : preuve de stockage indisponible — 4 octobre 2026

Le volume historique référencé par la forensique A n’est plus disponible.
Une sonde n’a ouvert aucune base : Docker avait créé un nouveau volume vide,
identifié et supprimé après contrôle. Échec de la sonde conservé, aucun audit
canonique revendiqué. Logs/manifeste historiques restent conservés ; la cause
initiale et l’anomalie A restent ouvertes. Autre copie vérifiée ou reproduction
bornée avec les diagnostics actuels nécessaire ; aucune campagne active relancée.
[Constat et périmètre](qualification/wan-a-evidence-availability-20261004/README.md).

## CI distante de l’outillage — 4 octobre 2026

Job GitHub operations à la révision `674a23e0` : terminal success, 375/375 tests
sans échec/skip, logs et artefacts réellement téléchargés et révision vérifiée.
Les jobs core/network de ce run restent actifs à l’observation ; aucune nouvelle
qualification C++/WAN, bridge ou production déduite de ce succès d’outillage.
[Preuves et périmètre](qualification/github-operations-674a23e0-20261004/README.md).
Toutes les exigences antérieures restent conservées selon leur statut.

## CI C++ distante sélectionnée — 4 octobre 2026

Job GitHub core à la révision `674a23e0` : neuf tests attendus, sélection/JUnit
concordants, neuf réussites sans échec/skip, build Release Linux AMD64 et artefacts
récupérés. Révision distincte de la candidate efbeb9bc ; aucun remplacement des
427 tests, de la restauration WAN, du pruning ou de la qualification complète.
Le job réseau reste actif à l’observation.
[Logs et limites](qualification/github-core-674a23e0-20261004/README.md).
Tous les critères antérieurs restent conservés selon leur statut.

## Transport RPC source Base — 4 octobre 2026

Transport de lecture borné livré dans tools/bridge-source/rpc.mjs : délais
par requête, taille reçue, identité JSON-RPC, refus des redirections/retries
et des méthodes d’écriture. Lot ciblé 39/39 (15 nouveaux tests HTTP loopback
+ 24 tests d’observation injectée), exit 0 sans skip.
[Preuves et limites](qualification/base-source-rpc-transport-20261004/README.md).
Aucune authentification cryptographique de finalité, éligibilité au mint,
qualification bridge ou nouvelle conformité EIP/C++ déduite de ce résultat.
Toutes les exigences et tous les échecs antérieurs restent conservés.

## CI : résultat outillage et annulation réseau — 4 octobre 2026

Outillage e87fc7b8 : run 37231284533 terminal success, 390/390 sans échec/skip ;
core/network sautés. [Artefacts](qualification/github-operations-e87fc7b8-20261004/README.md).
Le run réseau 674a23e0 a été annulé par le réglage cancel-in-progress:true
lors du push suivant, build publié interrompu à 73 %, sans qualification
réseau terminale. [Annulation conservée](qualification/github-network-cancelled-674a23e0-20261004/README.md).
Réglage remplacé par false pour préserver les runs actifs, sans supprimer
le groupe de sérialisation. Prochain résultat réseau encore attendu.
Les preuves core/operations antérieures et les critères ouverts restent conservés.

## CI core après correction de concurrence — 4 octobre 2026

Job core du run 37231449042 à 9e87b63b terminal success, neuf noms attendus
avec sélection/JUnit concordants, 9/9 sans échec/skip.
[Artefacts et revue](qualification/github-core-9e87b63b-20261004/README.md).
Runtime inchangé depuis 674a23e0 ; pas d’extension du périmètre ni de
qualification efbeb9bc/pruning/WAN déduite. Réseau encore actif à l’observation.
Tous les critères et échecs historiques restent conservés.

## Stabilité des observations source RPC — 4 octobre 2026

Trois défauts reproduits avec configuration/cache mutables : définition
de l’événement, reçu partagé et checkpoint partagé pouvaient changer pendant
les awaits sans refus. Contrôles avant correction conservés (24 succès,
3 échecs), puis champs vérifiés copiés et 27/27 sans échec/skip, exit 0.
[Preuves](qualification/base-source-observation-snapshot-20261004/README.md).
Pas de preuve cryptographique de finalité, dépôt réel, permission de mint,
bridge qualifié ou changement de candidate C++. Tous les critères et échecs
historiques antérieurs restent conservés.

## Candidate efbeb9bc AMD64 terminale et reprise WAN — 4 octobre 2026

427/427 sans échec/skip, CTest/contrôleur/audit/manifestes exit 0. Revue
indépendante : 16 494 sources et 208 programmes réellement re-hachés,
identiques avant/après ; temps CTest 12 065,68 s. ARM64 déjà 427/427.
[Rapports](qualification/coherent-amd64-terminal-20261004/README.md).
EEST : 5 387 variantes sélectionnées (4 882 exécution, 505 validation),
317 fichiers, aucune conformité complète des forks ni activation déduite.
Réactivation réindexée 351,85/360 s : faible marge de performance ouverte.

Préflight de restauration initial arrêté sur Permission denied du marqueur
(source/binaire inchangés, aucun WAN lancé) ; dépendants ont refusé leur build.
Correction du propriétaire et contrôleurs v2/v3 conservés avec les erreurs.
[Contrôles](qualification/coherent-recovery-queue-correction-20261004/README.md).
Restauration efbeb9bc désormais réellement lancée ; reprise de duties/finalité
commune, reproductibilité et nouveaux scénarios pruning pas encore acquis.
Runtime main non promu ; tous les critères, preuves et échecs antérieurs conservés.

## CI réseau après correction — 4 octobre 2026

Run 37231449042 à 9e87b63b entièrement terminal success. Réseau : pin
cpp-libp2p et 21 noms attendus vérifiés, sélection/JUnit concordants,
21/21 sans échec/skip. [Preuves](qualification/github-network-9e87b63b-20261004/README.md).
Le push d’outillage suivant a attendu sans annuler le build actif. Ancienne
annulation conservée ; pas de preuve WAN ou de capacité réseau déduite.
La restauration efbeb9bc a avancé avec une même racine de finalité de l’époque
2 à 3 sur Mac natif, Docker Mac et Docker VPS en 86,247 s dans le contrôle de reprise,
sous la borne 180 s. Stabilité et audits terminaux encore attendus à cette
observation : pas de clôture générale de restauration/production.
Tous les critères, résultats et échecs antérieurs restent conservés.

## Restauration entière terminale et CI d’outillage — 4 octobre 2026

Candidate efbeb9bc : restauration réelle terminée, contrôleur/revue exit 0.
21/21 journaux de signature ont avancé sans régression ; duties repris
(124 attestations, 19 propositions, 441 messages sync). Finalité commune
2 → 3 en 86,247 s sous la borne 180 s, puis stabilité commune 5 → 8 entre
Mac natif, Docker Mac et Docker VPS. Audit canonique terminal en lecture seule
(78 blocs, 1 transaction), ancrage externe et sauvegarde source inchangée.
[Preuves](qualification/coherent-restore-terminal-20261004/README.md).
Trois nœuds/deux hôtes, 64 clés synthétiques ; aucune coupure physique,
qualification réseau public ou vérification historique complète du fork choice.
L’échec initial de permission reste conservé ; anomalie WAN A encore ouverte.

CI outillage 1a58ae05, run 37233103046 : terminal success, operations 393/393
sans échec/skip. Core/réseau skipped pour cette révision JavaScript.
[Preuves](qualification/github-operations-1a58ae05-20261004/README.md).
Reproductibilité AMD64 encore en cours, pruning non qualifié à cette observation.
Runtime main non promu ; tous les critères, échecs et preuves antérieurs restent
conservés. Bridge, termes économiques et revue indépendante restent ouverts.

## Reproductibilité des livrables terminale — 4 octobre 2026

Candidate efbeb9bc : contrôleur exit 0, ARM64 et AMD64 complets. `nano` et
`nano-p2p` recompilés dans des répertoires neufs sont identiques octet pour
octet aux livrables qualifiés de chaque architecture. Sources et binaires
originaux inchangés ; images/cache épinglés réutilisés, aucune image reconstruite.
[Preuves](qualification/coherent-delivery-reproducibility-20261004/README.md).
Portée : deux exécutables de livraison ; pas de reconstruction hermétique
des dépendances ni de qualification d'image de release finale. Compilation
pruning 4734f285 désormais démarrée, non qualifiée à cette observation.
Tous les critères, résultats et échecs antérieurs restent conservés.

## Pruning isolé ARM64 qualifié localement — 4 octobre 2026

Branche 4734f285 sur base efbeb9bc, désactivée par défaut et non promue.
Build/link, CTest et contrôleur exit 0 : 7/7 sans échec/skip. Sources et
quatre programmes re-hachés indépendamment ; livrables efbeb9bc inchangés.
Deux propriétaires : finalité epoch 7, six mouvements d’ancre, réouverture
et rejeu de 55 états élagués chacun. Fsync/WAL : marqueur, suppression et
checkpoint atomiques avec suffixe conservé/perdu ; onze corruptions refusées.
RPC historique indisponible explicitement, latest exact ; régressions pertinentes.
[Preuves](qualification/finalized-retention-arm64-20261004/README.md).
AMD64 lancé dans l’image existante : [contrôleur](qualification/finalized-retention-amd64-control-20261004/README.md).
Restent sauvegarde/restauration de base élaguée, downgrade, mesures disque,
portabilité terminale et qualification réseau pertinente. Aucun gain SST
physique immédiat ni coupure électrique qualifié. Toutes les preuves, critères
et erreurs historiques sont conservés ; production toujours non qualifiée.

## Nouveau scénario de restauration du stockage élagué — 4 octobre 2026

3358d27b (tests seuls sur runtime pruning 4734f285) : compilation/link/test
ARM64 exit 0. Copie physique hors ligne et hashes exacts, réouverture read-only
sans mutation, reprise des frontières 3 → 5 → 6, sauvegarde source inchangée.
Budget zéro sans suppression et dépassement d’octets refusé atomiquement ;
commit repris après frontière complète, remplacement sous l’ancre refusé.
[Preuves et échec préflight conservé](qualification/pruning-offline-restore-arm64-20261004/README.md).
Seul le nouveau test compilé ; cœur et quatre anciens programmes conservés.
Portée stockage synthétique local, pas de restauration entière du consensus
avec témoins/signers, downgrade, panne électrique ou WAN pruning qualifiés.
Branche de tests publiée, runtime toujours isolé ; exigences et échecs antérieurs
conservés. AMD64 encore en cours à cette observation.

## Pruning AMD64 terminal et restauration Capella locale — 4 octobre 2026

4734f285 AMD64 : build/link, CTest/contrôleur exit 0, 7/7 sans échec/skip
(242,91 s). Revue indépendante de 16 495 sources et quatre programmes réels.
[Preuves AMD64](qualification/finalized-retention-amd64-terminal-20261004/README.md).
Même périmètre que ARM64, sans les nouveaux tests de restauration suivants.

94535766 (tests seuls) : nouveau scénario Capella de copie physique du
propriétaire élagué et 64 journaux, témoins indépendants retenus séparément,
audit genesis sans mutation, conflit refusé avant callback signataire.
Duties repris, 64/64 journaux avancés, finalité commune locale 2 → 7 et
source/sauvegarde inchangées. Premier échec tmpfs 256 Mio conservé ; même
binaire rejoué sur le volume disque existant, exit 0.
[Preuves et échec](qualification/capella-pruned-physical-restore-arm64-20261004/README.md).
Deux propriétaires locaux, pas de WAN ni de coupure électrique qualifiée.
Nouveaux scénarios pas encore AMD64 ; pruning toujours isolé/non promu.
Restent reprise multi-hôte pertinente, downgrade, mesure/gestion des ressources
et critères de production complets. Toutes exigences et erreurs conservées.

## Préparation WAN pruning et nouveaux scénarios AMD64 — 4 octobre 2026

Branche 092a153c : activation explicite de historicalStatePruning sur les trois
nœuds du banc Capella backup/restore. Audits initial/terminal exigent désormais
un rejeu non vide d’états réellement élagués. Trois tests de contrôles et syntaxe
du harness exit 0. [Preuves](qualification/pruning-wan-preparation-20261004/README.md).
Préparation uniquement : aucun nouveau résultat WAN acquis. Contrôleur AMD64
94535766 lancé pour les deux scénarios de restauration seulement, cœur/image
existants conservés. Stockage exit 0, Capella en cours à cette observation.
Ne pas confondre avec le précédent lot 7/7 AMD64. Runtime isolé/non promu ;
exigences, preuves et échecs historiques restent conservés.

## Nouveaux scénarios de restauration pruning AMD64 terminaux — 4 octobre 2026

94535766 : contrôleur, compilations/link et deux nouveaux scénarios exit 0,
délais 120/180 s inchangés. Sources (16 495) et deux programmes re-hachés
indépendamment ; anciens programmes et sources conservés.
[Preuves](qualification/pruning-recovery-amd64-terminal-20261004/README.md).
Copie physique stockage et bornes/refus atomiques qualifiés ; restauration
Capella avec témoins indépendants, 64/64 journaux avancés, conflit refusé avant
signature, finalité commune locale 2 → 7, sauvegarde/source inchangées.
Portée locale AMD64, deux propriétaires et clés synthétiques ; pas de WAN
pruning, downgrade ou panne électrique acquis. Échec tmpfs ARM64 conservé.
Banc WAN 092a153c préparé mais non exécuté ; pruning reste isolé/non promu.
Toutes exigences, preuves et erreurs antérieures demeurent conservées.

## Livrables prunés prêts et campagne WAN active — 4 octobre 2026

092a153c : trois livrables vérifiés avant lancement. Docker ARM64/AMD64 :
entrées nano compilées/linkées contre le cœur 4734f285 qualifié, sources de
runtime identiques, anciens programmes conservés et P2P efbeb9bc exact réutilisé.
Mac natif : nouveau build, scénario de restauration prunée exit 0 ; 64 journaux
avancés, finalité locale 2 → 7. Sources/binaires re-hachés par le préflight.
[Provenance, rapports et contrôle](qualification/pruning-wan-delivery-20261004/README.md).
Aucun rebuild d’image ; pas de preuve de reproductibilité hermétique déduite.
WAN privé Mac natif/Docker Mac/Docker VPS réellement lancé avec pruning explicite
et audits initial/terminal de rejeu requis. Restauration WAN, sortie terminale
et finalité stable encore attendues à cette observation. Aucun critère clos
sur la seule préparation ; runtime main non promu. Toutes exigences et erreurs
historiques conservées, production complète toujours non qualifiée.

## Restauration WAN prunée terminale — 5 octobre 2026

092a153c : contrôleur et revue des preuves exit 0. Trois nœuds réels / deux
hôtes privés ; restauration VPS avec 21 engagements de signature indépendants,
21 journaux avancés, duties repris (122 attestations, 19 propositions, 423 sync).
Finalité commune 2 → 3 en 80 506 ms sous la borne 180 000 ms, puis stable 5 → 8.
Audits lecture seule : 14 puis 59 états prunés rejoués, 77 blocs au terminal ;
source sauvegardée inchangée. [Preuves et limites](qualification/pruned-wan-restore-terminal-20261004/README.md).
Pas de fork choice historique, panne électrique, downgrade ou mesh public acquis.
Délai initial de progression observé, cause non démontrée. Pruning toujours isolé,
runtime main non promu ; toutes exigences et erreurs historiques conservées.
Protection de schéma/downgrade 59fa6530 écrite séparément, pas encore qualifiée.
Production complète et revue de sécurité indépendante toujours ouvertes.

## Protection du schéma pruné — ARM64, 5 octobre 2026

59fa6530 sur branche isolée : schéma nano/2-state-pruned écrit atomiquement
avec checkpoint/marqueur/suppressions. Vrai lecteur efbeb9bc : base non prunée
acceptée, base prunée explicitement refusée sans modification. Nouveau lecteur
accepté, deux désaccords schéma/marqueur refusés. Sept scénarios concernés
terminaux exit 0 : restauration/bornes, corruption, checkpoints/WAL et Capella
avec 64 journaux avancés et finalité locale 2 → 7.
[Preuves ARM64](qualification/pruning-schema-arm64-20261005/README.md).
AMD64, migration des anciennes bases expérimentales prunées nano/1 et gain SST
mesuré restent ouverts. Aucun runtime promu à main ; pas de panne électrique
ou nouveau WAN acquis. Toutes exigences et erreurs historiques conservées.

## Protection du schéma pruné — AMD64 terminal, 5 octobre 2026

59fa6530 : sept scénarios concernés passés aussi dans Docker AMD64, contrôleurs
et revue indépendante exit 0. 16 497 sources et binaires re-hachés, anciens
programmes qualifiés inchangés. Vrai lecteur ancien refusé après contrôle positif
sur base non prunée ; WAL atomique, 64 journaux Capella repris, finalité 2 → 7.
[Preuves AMD64](qualification/pruning-schema-amd64-terminal-20261005/README.md).
Protection de schéma désormais qualifiée ARM64/AMD64 dans ce périmètre. Runtime
main non promu ; migration des anciennes bases expérimentales prunées nano/1,
gain SST mesuré et autres critères production restent ouverts. Pas de panne
électrique ou nouveau WAN déduit ; toutes preuves/erreurs historiques conservées.

## Candidate intégrée prunée préparée — 5 octobre 2026

Branche qualification/production-pruning-20261005, candidate 1d83428e : fusion
sans conflit de main 064aa25d et du runtime qualifié avec garde-fou 59fa6530.
Runtime et tests stockage/Capella vérifiés byte-identiques ; les quatre documents
du goal inchangés à la fusion. Outils combinés : 424/424, code terminal 0.
[Preuves, provenance et échecs de contrôle](qualification/production-pruning-integration-20261005/README.md).
Cette préparation n'est pas une promotion du runtime main ni une nouvelle
qualification WAN. Critères pruning/espace/migration, consensus, exploitation,
revue indépendante et termes financiers restent ouverts ; objectif actif.

## Mesure physique sur copie prunée — AMD64, 5 octobre 2026

Fixture arrêtée 092a153c copiée, original lecture seule/hachages inchangés.
Flush/compaction de la copie : chain 6 761 823 → 793 227 octets apparents ; SST
1 062 871 → 666 214, WAL 5 569 963 → 0. Nœud entier 22 535 188 → 16 566 592.
344 clés/valeurs et toutes catégories logiques identiques. Audit terminal exit 0 :
77 blocs, 59 états prunés rejoués, engagement checkpoint indépendant vérifié.
[Mesures, refus initial et limites](qualification/pruned-copy-compaction-amd64-20261005/README.md).
Gain combiné compaction/WAL sur petite copie : pas de comparaison non-prunée,
politique automatique/en ligne, croissance long terme ou panne électrique acquis.
Migration nano/1 pruné reste ouverte ; aucun runtime main promu. Toutes exigences
et erreurs historiques conservées, objectif production actif.

## Migration des anciennes copies prunées — AMD64, 5 octobre 2026

Helper 23df7f63 : nano/1 pruné → nano/2-state-pruned sur trois copies arrêtées.
Normal, SIGKILL avant écriture, SIGKILL après écriture sync : reprise exacte et
rejeux sans écriture. Mauvais engagement externe refusé, fichiers inchangés ;
seule meta/schema change. Audits avant/après identiques : 77 blocs, 59 états
prunés rejoués, checkpoint indépendant conservé. Ancien lecteur refuse les trois
copies migrées ; originaux et anciens programmes inchangés/re-hachés.
[Preuves, compilation échouée et limites](qualification/pruned-schema-migration-amd64-20261005/README.md).
Contrôleur terminal exit 0. ARM64 du helper, reprise des duties après migration,
maintenance en ligne et panne électrique non acquises ; runtime main non promu.
Toutes exigences et erreurs historiques conservées, objectif production actif.

## Migration ARM64 et reprise des duties locale — 5 octobre 2026

Archive WAN vérifiée e19bafc3, engagement initial correspondant : migration normale
et SIGKILL avant/après écriture passent sur Docker ARM64, rejeux sans écriture ;
audits identiques, 31 blocs/14 états prunés. Source et programmes préservés.
Nouveau scénario 477e5ba4 utilise le helper réel sur une copie legacy contrôlée,
reprend les 64 journaux avec témoins indépendants, refuse le conflit avant signer
et avance la finalité locale 2 → 7 ; contrôleur terminal 0, source/backup inchangés.
[Preuves et préflight refusé conservé](qualification/pruned-migration-arm64-duties-20261005/README.md).
Migration hors ligne du helper qualifiée ARM64/AMD64 ; duties après migration
qualifiés localement ARM64 seulement. Nouveau lot AMD64 actif, terminal attendu.
Pas de nouveau WAN, migration en place ou panne électrique acquis ; toutes
exigences/erreurs conservées, runtime main non promu, objectif production actif.

## Duties après migration AMD64 — terminal réel, 5 octobre 2026

Source 477e5ba4 : même nouveau parcours que ARM64, helper réel et cœur 59fa6530.
64 journaux repris, conflit refusé avant signer, finalité locale 2 → 7, source
et backup inchangés. Conteneur exit 0 via docker wait indépendant enregistré
pendant l'exécution ; hashes des programmes/bibliothèque vérifiés séparément.
[Preuves et timeout du wrapper conservé](qualification/pruned-migration-duties-amd64-terminal-20261005/README.md).
Wrapper compilation+test échoué à 300 s, sans relance ou extension. Qualification
fonctionnelle acquise ; ce budget de contrôle reste échoué. Migration/reprise
locale désormais couvertes ARM64/AMD64 dans cette portée ; aucun nouveau WAN,
migration publique en place, panne électrique ou production globale déduit.
Toutes exigences et erreurs conservées, objectif production actif.

## Sources du cœur qualifié livrées dans main — 5 octobre 2026

Fusion de la candidate source sans conflit : runtime/dépendances/CMake identiques
à 59fa6530, test Capella identique à 477e5ba4, outils JS/CI inchangés depuis le
lot intégré 424/424. Quatre documents originaux byte-identiques avant annexe.
[Provenance et audit de livraison](qualification/runtime-main-delivery-20261005/README.md).
Cette livraison réconcilie enfin les sources main avec le cœur/pruning qualifiés ;
elles n'étaient pas auparavant identiques. Aucun binaire public déployé ou base
active migrée, aucun rebuild d'image. Preuves/erreurs historiques conservées,
périmètres exacts réutilisés, timeout global de contrôle AMD64 toujours échoué.
La production globale, nouvelle preuve WAN après migration, maintenance en place,
critères consensus/exploitation restants, relation Base–Nano/bridge/termes Doppler
et revue indépendante demeurent ouverts. Objectif actif, aucune vente autorisée.


### 2026-10-05 — ARM64 runtime packaging and missing-license fix

Standalone execution base and candidate assembled without recompiling qualified C++ binaries: image `sha256:8a4e25879769a0f8b4b9793280ef11f1ee2a94591a166c84c7bb454f40a6a85d`, 272,168,394 bytes, user 10001:10001. Binary hashes, all eleven notices, shared-library closure and read-only/network-disabled help execution verified. Packaging originally omitted seven licenses; the blocked build is preserved and the exporter now includes them, with complete-manifest regression coverage. Runtime source matches main 4eb31d3a / 59fa6530; cached-object provenance remains explicit. No public node started. AMD64 assembly, hermetic dependency rebuild, outstanding consensus/economic gates and independent security review remain open. Evidence: `docs/qualification/runtime-packaging-arm64-20261005/`. Existing CI 37242291266: operations success; core/network still running at this observation. No existing qualification or historical failure removed.


### 2026-10-05 — Candidate image pair and terminal scoped core CI

AMD64 candidate assembled once on the authorized VPS, controller exit 0: `sha256:47ed3f0093a1708cfad7ccc499c513a9dff21cc58d65447228775705339071ba`, 251,143,515 bytes. ARM64 companion 272,168,394 bytes. Both hashes/user/library closure/eleven notices verified, no C++ rebuild or node start. Immutable paired manifest and explicit compilation provenance retained. CI 37242291266 core terminal success: nine registered tests, zero failures/skips; this does not replace full campaigns. Network job remains pending completion. Source inspection identifies packaged restoration still unsupported by the harness, including audit path/entrypoint and new-copy ownership at UID 10001; fix and actual Nano2 WAN recovery remain required. Evidence: `docs/qualification/runtime-packaging-pair-20261005/`. Hermetic dependency rebuild, all other production gates and independent review remain open; historical failures preserved.


### 2026-10-05 — Packaged Nano2 restoration tools and real offline ownership check

Harness accepts reviewed packaged Capella restoration; installed Nano performs audits, and only the fresh extracted copy changes owner to 10001:10001. Final audit uses direct read-only mounts under that user; source/backup/witness commitments remain separate. Actual AMD64 image archived and audited a stopped Nano2 fixture: 77 blocks, 59 pruned states replayed, checkpoint 827 unchanged; source/restored content manifests and rootless audit JSON match. Original controller exit **123** is preserved (root observer without capabilities could not read peer.key after chown); follow-up full rehash as owner exit **0**, no repeated restore/audit. Initial missing-config mount refusal and unit path failure preserved. Relevant 62-test tools selection passes. No duties or WAN finality advancement claimed, and no image rebuilt for these tools. Evidence: `docs/qualification/packaged-nano2-offline-restore-20261005/`. Native equivalence and actual packaged multi-host restoration/finality remain next, with all other production/security/economic gates open.


### 2026-10-05 — Native Nano2 and packaged source rehash prerequisite

Native storage.cpp rebuilt in an isolated directory from 59fa6530-equivalent runtime; new library/entry/test links reuse explicit 092a153c objects. Member comparison: only storage.cpp.o changed; original binaries/library/object hashes unchanged. Actual local pruned restore terminal 0, all 64 signing journals advance, independent pins/witnesses retained, conflicting proposal rejected before signer, local finality 2→7. Transport=false, power-loss=false; not WAN qualification. Last packaged source rehash call now explicitly selects shell/root tool; actual AMD64 read-only source comparison passed without repeating restore/audit. Evidence: `docs/qualification/nano2-native-restore-20261005/`. Three-node packaged Nano2 restore campaign remains next; all broader production gates/review remain open.


### 2026-10-05 — Existing main CI completed, no restart

GitHub run 37242291266 / source 4eb31d3a terminal **success** for changes, operations, core and network. Network XML: exactly 21 tests, zero failures/skips; core XML: exactly 9 tests, zero failures/skips. Operations log and notices archived. These are selected CI checks, not full EVM/consensus/financial production qualification; subsequent packaging/controller changes retain their separate evidence. Three-node packaged Nano2 restoration remains active and no terminal WAN success is claimed here. Proof: `docs/qualification/main-ci-terminal-20261005/`; prior core proof retained. No repeated CI job or image rebuild. Other requirements and independent security review remain open.


### 2026-10-05 — Packaged Nano2 three-node restoration terminal 0

Exact current Nano2 runtime 59fa6530 (harness 079c7f92) passed original private three-node/two-host restore campaign, controller exit **0** without restart. VPS complete database/peer identity/witnesses and 21 journals restored; independent chain/key pins retained outside archive, all 21 advanced. Common finality 2→3 in **78,597 ms < 180,000 ms**; further shared window **5→8**. VPS fresh duties: 123 attestations, 13 proposals, 441 sync messages, no reported duty failure. Initial audit 32 blocks/15 pruned replays; terminal rootless audit 77 blocks/59 pruned replays, checkpoint 830, independent epoch-8 root/ancestry verified. Original source and offline-copy bytes preserved, 33 harness files and image/native hashes rechecked. This closes the scoped **packaged Nano2 stopped-copy restore + resumed duties + advancing common finality** requirement. Proof: `docs/qualification/packaged-nano2-wan-terminal-20261005/`. It does not close public mesh, physical power loss, historical fork-choice/vote replay, WAN A, all consensus/load/operations/economic gates or independent review. Startup warming measured finalized blocks rather than head; this campaign's latest head advanced. Capella syncing endpoint diagnostic 404 is retained as API coverage gap. All prior failures/evidence remain intact, production objective remains active.


### 2026-10-05 — Finite signed load on current packaged runtime terminal 0

Current runtime 59fa6530/images, harness 5a5ebbe5: private three-node/two-host campaign exited **0**. 96 signed type-2 transactions at bounded requested 1/2/4 per-second ramp; all successful/canonical/finalized complete receipts identical across nodes, nonce/balance/storage match. Workload including collection/finality **276,123 ms < 600,000 ms**; common finality after load **8→11**. Docker exit 0/no OOM, validators healthy; hashes/recorded receipts/harness rechecked. 31 usable resource samples/node, zero unavailable; observed memory native main RSS 302,907,392 B (P2P excluded), ARM64 Docker 350,434,099 B, AMD64 Docker 290,770,125 B. This closes only nominal finite-load inclusion/state-agreement with sampled resources, **pruning disabled**, no capacity/saturation/endurance/storage-growth/public-mesh claim. WAN A cause remains open despite no reproduction here. Proof: `docs/qualification/packaged-nano2-load-terminal-20261005/`. All remaining consensus/pruning-load/operations/financial/security-review requirements remain intact; objective active.


### 2026-10-05 — Capella peer syncing raccordement en cours

Base vérifiée : main ef559d32773cbc5a4f5ac6d03f6c413d73e96e90. Le runtime Capella raccorde désormais `/eth/v1/node/syncing` à un BeaconSyncTracker alimenté par les rapports locaux authentifiés du superviseur P2P, pour `run` et `serve`. Sans échantillon récent et non vide, réponse 503 ; hauteur Beacon et distance décimales, refus des slots futurs et des discontinuités d'horloge. Les indications de pairs ne modifient ni l'horloge ni le checkpoint de consensus. Aucun nouveau profil EVM activé.

Statut : implémenté, qualification en cours ; aucune promotion de candidate ou qualification WAN de cette modification. Docker ARM64 existant, nouveau dossier `/build/capella-sync-arm64-v3-20261005`, contrôleur `/tmp/nano-capella-sync-control-20261005`. Le premier lancement du contrôleur a échoué avant compilation (Python absent de l'image) ; version bash compilée, puis premier scénario terminal 1 : le test synthétique `genesis_time_ms - 1` débordait puisque genesis vaut zéro. Échec conservé dans `/build/capella-sync-arm64-20261005`. Correction du test : recul avant le temps publié strictement positif ; seuls test et main recompilés dans v3, cœur compilé réutilisé. Sortie terminale corrigée et qualification AMD64 encore à vérifier. Images, binaires et preuves antérieurement qualifiés conservés. Revue indépendante et tous autres critères de production restent ouverts.

Suite du lot syncing : v3 terminal 1, après réussite des assertions de synchronisation, sur le redémarrage illégal d’une horloge arrêtée (`Capella clock already started/stopped`). Le test ferme désormais le runtime puis le restaure depuis ses engagements persistés avant de poursuivre les duties. Contrôleur v4 : `/build/capella-sync-arm64-v4-20261005`, manifeste de sources `/tmp/nano-capella-sync-control-20261005/source-manifest-v4.json`. Aucun échec effacé ; qualification finale encore en attente.

Résultat terminal v4 : **0**, Docker ARM64, sans OOM. Preuve `docs/qualification/capella-sync-arm64-20261005` : manifeste inchangé, contrôles de rapport authentifié, observation absente/vide, distance 0/2, type invalide, futur, horloge en recul, annulation et checkpoint non modifié ; restauration effective du runtime puis 64 clés, 16 blocs, engagements/witness et arrêt des workers. Compilation main également réussie. Qualification limitée au runtime direct/RPC local ; HTTP/P2P et AMD64 non qualifiés pour ce nouveau lot, aucune candidate promue. Les échecs des contrôleurs précédents restent conservés.

### 2026-10-05 — Capella syncing HTTP ARM64 terminal

Extension de test uniquement, même cœur compilé précédemment, image et volume réutilisés. Preuve `docs/qualification/capella-sync-http-arm64-20261005` : sortie terminale **0**, manifeste source inchangé, GET Beacon et rapports JSON-RPC effectivement échangés par HTTP ; sans pair 503, mauvais token refusé sans modifier la réponse, slot head décimal sans confusion avec la hauteur d’exécution, distance 0 puis 2 et is_syncing, futur impossible 503 sans divulgation interne. Le scénario poursuit la restauration des témoins/journaux et duties, 64 clés et 16 blocs ; networkTransport=false. Le cœur du lot 2cd9ab9d est simultanément en qualification AMD64, pas encore promu sur origin/main. Aucun nouveau profil EVM, aucune ouverture publique.

### 2026-10-05 — Capella syncing AMD64 direct et ARM64 lifecycle

`2cd9ab9d` récupéré par Git dans un worktree figé du VPS autorisé, même image/volume de compilation. Scénario direct AMD64 terminal **0**, aucun OOM, 64 clés et 16 blocs avec reprise des duties et engagements : `docs/qualification/capella-sync-amd64-20261005`. ARM64 : binaire relinké depuis main et cœur concernés ; `serve` deux démarrages et `run` trois démarrages, reprise witness, configurations invalides refusées, panne du child entraîne arrêt, secrets des validateurs exclus du child ; terminal **0** dans `docs/qualification/capella-sync-lifecycle-arm64-20261005`. Ce lifecycle utilise le child synthétique (realP2pTransport=false) et ne qualifie pas la propagation libp2p. Extension HTTP/lifecycle AMD64 sur source de test 417ed02e en cours ; cœur réutilisé, seul test et main compilés. Origin/main conserve la candidate antérieure jusqu’au résultat.

### 2026-10-05 — Capella syncing lot ARM64/AMD64 terminal et candidates identifiées

HTTP/lifecycle AMD64 sur tests 417ed02e, runtime byte-identique à 2cd9ab9d, terminal **0**, sans OOM : `docs/qualification/capella-sync-http-amd64-20261005`. Les mêmes GET Beacon/rapports JSON-RPC et refus sont vérifiés sur les deux architectures, puis 64 clés/16 blocs, serve deux starts et run trois starts avec reprise witness/child failure. Scope P2P réel non couvert ; ne pas réattribuer les campagnes WAN de la candidate 59fa à ce nouveau lot.

Une assembly par architecture, bases immuables et volumes existants réutilisés, nouveaux nano exacts et ancien nano-p2p inchangé. Images contrôlées en lecture seule, réseau none, UID10001, hashes et 11 notices, résolution dynamique et CLI : `docs/qualification/capella-sync-candidate-pair-20261005/manifest.json`. ARM64 `sha256:03ce073ac1edc7ec5a1d784ad1d67b1ee58d3266c7df65cfcb1915e62c7e47df`, AMD64 `sha256:433ad751a884bb0b7061994f3accb6fede4067c2f2626de861e10b26da297966`. Deux erreurs de wrapper ARM64 conservées (variable zsh réservée après création de l’image ; attente erronée de sortie 0 pour nano-p2p usage, dont le contrat est 1). Vérification corrigée des images réelles terminal 0 sans rebuild. Rebuild hermétique complet, nouvelle finalité multi-hôtes dans ces images et revue indépendante restent non qualifiés ; pas de nœud public ni fonds.

### 2026-10-05 — Participant natif syncing et préparation WAN

Mac natif construit dans un nouveau dossier à partir du core Nano2 précédent, remplacement de trois objets runtime/node/rpc et main ; test frais. HTTP natif terminal **0**, 64 clés et 16 blocs avec reprise witness/duties : `docs/qualification/capella-sync-native-20261005`. Échec de compilation du test conservé : fixture DB::Open incompatible avec RocksDB11 installé sur Mac (Docker utilise8). Détection d’API du fixture comme celle du stockage existant, sans modification du runtime. Micro-test actual DB sur11 et8 : refus absence, écriture sync et réouverture terminal0 ; attente erronée absence de répertoire exit3 conservée, corrigée en absence de CURRENT car LOCK/LOG peuvent être créés lors du refus.

Banc multi-hôtes étendu par option `NANO_TEST_PEER_SYNCING=1` : observation des trois API Beacon avant/après finalité, uint64 décimal exact, distance/is_syncing cohérents, bornage du slot par deux lectures de head, refus flags/overflow ; 503 conservés et deadline fixe60s non étendue. Dix tests JS pertinents passés. Sélection des candidates déjà assemblées dans `docs/qualification/capella-sync-candidate-pair-20261005/selection.json`. Campagne réelle non encore démarrée à cette inscription ; aucune nouvelle image ni modification de consensus, anciennes preuves WAN limitées à leur candidate.

### 2026-10-05 — Candidates syncing : restauration Nano2 et WAN terminal

Preuve `docs/qualification/peer-syncing-wan-terminal-20261005` : terminal **0**, runtime2cd9ab9d/images existantes, banc3a3f3fb2, Mac natif + Docker ARM64 + Docker VPS AMD64, deux hôtes physiques et64 clés synthétiques. Les trois API syncing réellement alimentées par P2P passent au startup et après finalité, distance0, headslots32/32/33 puis80/80/81, sans réponse503 dans ces deux fenêtres. Le VPS est entièrement copié puis restauré, même peerId,21 engagements séparés ; finalité commune2→3 en78792ms<180000, puis fenêtre5→8, racine finale `0xf2fe0096f9e5a8c499a06adfa693a3d23d5a094dfc90be15122cabe12a102deb`. Les21 journaux avancent chacun sequence et digest ; signataire repris121 attestations/17 proposals/423 sync, healthy et zéro erreur. Audit readonly final :78 blocs,60 états prunés rejoués, checkpoint827 et ascendance de la finalité externe vérifiée. Source sauvegardée inchangée,34 fichiers de banc et programmes natifs inchangés, conteneurs arrêtés proprement sans OOM.

Vérificateurs auxiliaires : index Mach-O `__.SYMDEF` change lors du remplacement des objets, RPC recompilé reste byte-identique ; seuls les objets runtime et node changent, tous autres membres conservés. Assertions initiales erronées conservées. Comparaison des journaux corrigée pour la forme réelle du reçu (index authentifié fixe, champ publicKey absent). Aucun re-run de campagne. Qualification limitée à ce réseau privé et cette restauration, pas de panne électrique, validation historique fork-choice/votes complets, capacité publique ou revue indépendante. Les autres critères production/bridge/termes Doppler restent ouverts.

### 2026-10-05 — Maintenance hors ligne portable RocksDB8/11

Les helpers compaction/migration utilisent désormais la détection d’API DB::Open/OpenForReadOnly, compatible avec RocksDB11 natif et8 Docker, sans toucher src/storage ni le runtime. Marqueurs obligatoirement réguliers, non symboliques, au plus96 octets, contenu exact avec newline facultative ; la compaction refusait auparavant les symlinks du répertoire mais pas du marqueur. Preuve `docs/qualification/maintenance-api-portability-20261005` : compilations et opérations natives/Docker ARM64 terminales0, six refus sans modifier les copies par environnement (symlink, surdimensionné, ambigu sur chaque outil), source cold inchangée, migration seulement schema en write sync puis retry0writes ; compaction SHA encadré ordonné inchangé, audits canoniques avant/après sur vraie fixture31blocs/checkpoint293 et14états prunés. Deux erreurs de contrôleur conservées : /tmp alias système refusé ; message spécifique migration attendu incorrectement. Contrôleurs corrigés sur chemins canoniques sans assouplir les helpers. Aucune image reconstruite, aucun in-place, politique automatique, migration publique, panne électrique ou croissance long terme qualifiée ; tous autres critères ouverts conservés.

### 2026-10-05 — Maintenance portable AMD64 terminal

Source outils622983f1, image/volume existants, aucun runtime ou image reconstruit. Preuve `docs/qualification/maintenance-portability-amd64-20261005` : contrôleur corrigé terminal0 sans OOM ; six refus de marqueurs avec copies inchangées, migration schema seulement en write synchrone puis retry0writes, compaction344 entrées/digest logique inchangé. Audits canoniques avant/après identiques :77 blocs,59 états prunés, checkpoint827, ascendance externe époque8 vérifiée ; source cold inchangée. Premier contrôleur terminal1 conservé : mauvais montage du volume originel au lieu de la copie restaurée engagée, refus checkpoint avant mutation. Correction uniquement du montage, mêmes binaires et engagement indépendant inchangé. Portabilité des helpers désormais vérifiée Mac natif RocksDB11 et Docker ARM64/AMD64 RocksDB8 sur leurs fixtures respectives ; pas de qualification automatique/in-place, panne électrique, croissance long terme ou revue indépendante. Tous autres critères restent ouverts.

### 2026-10-05 — Charge bornée avec restauration prunée : banc prêt

Le banc autorise explicitement boundedLoad après backupRestore seulement avec historicalPruning ; conserve Capella/artifacts revus et refus partition/asymétrie/rotation. Le workload96 type2 intervient après restauration complète/reprise de finalité, puis audits terminaux prunés/ascendance externe et journaux conservés. 39 tests concernés (pruning selection, receipts, audit externe, inventaire transitif) passent sans skip ; aucun cœur C++ ou image modifié. Campagne réelle encore à démarrer, aucune réussite charge+pruning revendiquée. Source et anciennes preuves restent conservées, autres critères/revue indépendante ouverts.

### 2026-10-05 — Charge prunée : échec de démarrage horloge Mac

Preuve `docs/qualification/pruned-load-startup-clock-failure-20261005`, banca5d5ce8e/runtime2cd9ab9d/images existantes : terminal1 au délai initial300s, aucun restart, aucune restauration commencée, zéro transaction de charge générée/envoyée. Deux validateurs Mac échouent sur recul local : natif now1791162367881<observed1791162367924 (43ms), Docker12ms, beforeDurable0/forwardExceeded0. Mac Docker exit1/no OOM, VPS exit0/no OOM avec horloge et signer sains, finalité0. Les receipts Mac terminaux manquent car le reactor invalidé refuse current au shutdown. Le mutex protège déjà l’ordre des polls ; hypothèse intermédiaire de course infirmée, cause OS/NTP/suspend non déterminée. Aucun garde-fou assoupli. Prochaine correction : source de temps locale commune, temps écoulé monotone ancré et politique explicite de dérive/restart, qualification des ajustements injectés et de cette campagne réelle. Charge+pruning reste non qualifiée ; toutes autres exigences/revue indépendante restent ouvertes, goal actif.

### 2026-10-05 — Correction source de temps locale en qualification

Ajout LocalMonotonicTime opt-in via clock.maximumWallClockDriftMs : temps system local ancré puis progression steady, ajustements wall bornés ne changent pas les deadlines signées ; dépassement de dérive, recul steady, overflow ou panne source latchent un refus. Le budget est canonique, positif, strictement inférieur au délai attestation et au maximumForwardStepMs. Même callback pour clock, validator, admission et Beacon runtime. Sans option, comportement strict ancien conservé ; aucune activation publique automatique. Contrôle natif primitive : ajustements +/- et frontières exactes, fault persistant, overflow, 800 observations concurrentes passés. Tests runtime ajoutés : budgets invalides refusés avant checkpoint et démarrage du mode monotone. Compilation incrémentale native réussie, scénario runtime en cours ; Docker/AMD64 et campagne réelle prunée pas encore qualifiés. Échec précédent et toutes autres exigences/revue indépendante restent conservés.

### 2026-10-05 — Temps monotone local : trois environnements terminal0

Preuve `docs/qualification/local-monotonic-time-20261005`, C++ab10b8ae : Mac natif et Docker ARM64/AMD64 compilés incrémentalement, runtime/test terminal0, Docker sans OOM ; même core précédent avec objet runtime seul remplacé, aucune image rebuildée/promue. Primitive : ajustements wall +/- bornés et seuils exacts, faults persistants, steady regression, overflow, exception et800 lectures concurrentes. Raccordement runtime :5 budgets invalides refusés avant checkpoint, démarrage opt-in ; puis64 clés/16 blocs, reprises witness/journaux et arrêt workers. Eight checksJS configuration explicite/inventaire transitif passent. Mode optionnel par maximumWallClockDriftMs ; valeur production non choisie, ancien mode strict conservé. Le banc accepte NANO_TEST_WALL_CLOCK_DRIFT_MS ; docs/LOCAL-TIME.md décrit limites cumulatives/restart. Reste à assembler des candidates identifiées de ce nouveau cœur puis qualifier réellement restauration prunée+charge/finalité ; succès local ne remplace pas cette campagne. Échec43/12ms Mac conservé, cause OS non déterminée, autres critères/revue indépendante ouverts.

### 2026-10-05 — Candidates temps monotone assemblées et identifiées

Preuve `docs/qualification/monotonic-clock-candidate-pair-20261005` : runtimeab10b8ae, une assembly ARM64 et AMD64, bases/dépendances/volumes réutilisés sans compilation supplémentaire. Images `sha256:8f635e64f28868385e8ac18a60e32fe0bf7f8826c593f41b9206dffb3145363d` et `sha256:d3626bdfa150a0f0e4909c1a4a769b20df649ef43f3c50846ad0d92c3b6d158d`, hashes/UID10001/11 notices/ldd/CLI contrôlés readonly réseau none. Aucun nouveau succès WAN attribué ; campagne complète restauration prunée+96tx+finalité préparée avec250ms de tolérance explicitement sélectionnée pour la fixture privée uniquement. Les anciennes images/preuves et l’échec horloge restent conservés ; tolérance production et toutes autres exigences/revue indépendante ouvertes.

### 2026-10-05 — Restauration prunée+96tx : refus d’état historique du banc

Preuve `docs/qualification/monotonic-pruned-load-state-refusal-20261005` : runtimeab10b8ae/banc8e555b42, contrôleur terminal1 conservé. Restauration entière VPS31blocs/14états prunés, pins21 indépendants, source/copie inchangées ; finalité commune2→3 en78741ms.96 transactions acceptées/minées46–56, reçus finalisés canoniques complets identiques sur3nodes vérifiés à ancre60. Lecture de l’état de cette vieille ancre ensuite refusée -32000 State unavailable : collecte des reçus a traversé plusieurs époques et état déjà pruné. Pas de réussite workload/états ni fenêtre stable/audit terminal revendiqués. Trois signers/clocks sains à l’arrêt, finalité commune9/root67247f0d…, Docker exit0/no OOM ; recul d’horloge précédent non reproduit dans cette campagne. Correction JS seulement : ancre fraîche commune pour états, hash ancien immutable, erreur exacte disponibilité retryable, autres incohérences fatales, délai600s inchangé/sans resoumission ;44 tests affectés puis9 pour garde-fou renforcé passent. Images/C++ inchangés ; campagne complète corrigée reste à qualifier, autres exigences/revue indépendante ouvertes.

CI du temps monotone : core.sh ajoute explicitement nano-local-monotonic-time-tests à la compilation, consensus_local_monotonic_time à l’inventaire exact attendu et son hash aux artefacts. Prochaine CI sélectionnée devra démontrer10 registrations au lieu9 ; aucun succès de cette nouvelle sélection revendiqué avant sortie terminale.


### 2026-10-05 — Pruned workload: storage DATA collector correction

Preuve `docs/qualification/pruned-load-storage-data-refusal-20261005`, terminal **1** banc2d7ba2ee/runtimeab10. Restauration complète, finalité2→3 en78972ms ;96 transactions acceptées, reçus complets finalisés canoniques identiques indépendamment vérifiés (ancre59). Échec collecteur275845ms : stockage RPC de32 octets DATA traité comme QUANTITY compact, stub `0x2a` erroné. Correction JS uniquement : stockage exactement32 octets, nonce/solde quantities strictes ;12 tests pertinents passés et6 inventaire. Runtime/images inchangés, conteneurs terminaux0 sans OOM, signataires healthy. État finalisé/fenêtre stable/audit final restent non qualifiés ici. Erreur d’archivage chemin Git non normalisé conservée, vérification corrigée sans rejeu réseau. Scénario corrigé à reprendre sans compilation ; production et revue indépendante restent ouvertes.


### 2026-10-05 — CI terminale et nettoyage des caches natifs

`docs/qualification/ci-and-native-cleanup-terminal-20261005` : CI6ccc terminal success,9 tests C++ sélectionnés et21 transport, JUnit sans échec/skip ; CI797b terminal success,455 tests outillage, C++/réseau explicitement skipped. Sélection10 incluant nouvelle horloge : dispatch37254079157 en cours, pas encore qualifié. Ces sous-ensembles ne remplacent ni427 tests ni EEST/forks activés. Nettoyage local demandé :2,862,555,136 octets alloués libérés (caches Anvil et72 objets/dependency obsolètes), binaires/sources/rapports et programmes campagne rehashés inchangés. Docker et bases/preuves intacts ; campagne797b restaurée/prunée en cours, sans rebuild. Autres critères et revue indépendante demeurent ouverts.


### 2026-10-05 — Restauration entière + pruning + charge : terminal qualifié dans son périmètre

Preuve `docs/qualification/monotonic-pruned-load-restore-terminal-20261005`, banc797b00e5/runtimeab10, terminal **0**. Mac natif + Docker ARM64 + VPS Docker AMD64,2 hôtes physiques/64 clés synthétiques, images/programmes réutilisés. Restauration entière et21 pins indépendants, tous journaux avancés ; finalité2→3 en85581ms<180000.96 transactions type2, reçus complets réussis canoniques finalisés identiques ; état récent commun bloc74 nonce97/solde96wei/slot0=42, charge282320ms<600000. Fenêtre stable10→13 en136626ms, racine `0x2ddfc6e2c32eb9c283d4f1ce026b3e496d574b405739c05d05fd0028f5017911`. Audit readonly115 blocs/97 états prunés rejoués/97 transactions, checkpoint1271 et ascendance finalisée externe vérifiés. Source sauvegardée intacte,36 fichiers de banc et programmes natifs inchangés, conteneurs0 sans OOM.35 snapshots ressources, pas une mesure de pic/capacité. Tous échecs antérieurs conservés. Ne qualifie pas automatiquement partitions/rotation/cycle complet/sanctions/grande échelle, historique fork-choice, croissance longue, coupure physique, herméticité, bridge/Doppler ou revue indépendante. Production globale reste ouverte ; nouvelle CI10 encore en cours.


### 2026-10-05 — CI horloge terminale et prérequis de rotation

`docs/qualification/clock-ci-and-rotation-auditors-20261005` : dispatch37254079157/source797b00e5 terminal success,10 C++ sélectionnés dont temps monotone, JUnit zéro échec/skip, transport explicitement skipped (preuve21 antérieure conservée). Auditeur hors ligne compilé seul Mac natif/ARM64/AMD64,6 contrôles agrégat vide passent, source identique ; cœurs/programmes runtimeab10 rehashés inchangés, images/volumes réutilisés sans rebuild d’image. P2P déjà qualifié lié dans le dossier de runtime explicite. Ce sont des prérequis de prochaine campagne WAN rotation/reconnexion, pas sa qualification. Cycle complet/sanctions/échelle/régression complète et tous autres critères/revue indépendante restent ouverts.


### 2026-10-05 — Banc de sortie WAN : artefacts actuels, préparation seulement

`docs/qualification/exit-artifact-preflight-20261005` : ancien banc deux nœuds adapté aux programmes runtimeab10, image AMD64 immuable et volume/sous-dossier explicites readonly. Manifeste obligatoire, révision runtime distincte du Git du banc, SHA256 natifs/VPS et inventaire transitive vérifiés avant/après ; budget horloge privé250ms validé sur fixture exporté.460 tests d’outillage terminal0, zéro échec/skip, dont5 contrôles négatifs d’artefacts. Pas de compilation/runtime modifié, rebuild d’image ou second réseau lancé. Rotation/reconnexion trois nœuds continue séparément ; pas encore de nouveau résultat terminal revendiqué. Sortie/retrait/SIGKILL WAN actuel restent à exécuter ; ancien périmètre64 clés synthétiques, paramètres raccourcis et automaticRecovery de fixture demeure explicite. Tous échecs/critères précédents, cycle complet/sanctions/grande échelle, bridge/Doppler et revue indépendante restent ouverts.


### 2026-10-05 — Rotation + partition : terminal actuel qualifié dans son périmètre

`docs/qualification/monotonic-rotation-terminal-20261005` : banc0950c18e/runtimeab10 terminal0, Mac natif + Docker ARM64 + VPS AMD64,2 hôtes/64 clés publiques synthétiques. Coupure96s traversant frontière128, cutoff136 ; finalité13 conservée pendant isolement, reprise commune18 en140887.790333ms<180000, fenêtre18→21 en143203ms. Trois audits de bases/témoins réouverts et tokens conservés vérifient ascendance finalisée, SSZ/signatures BLS et comité changé ;21 blocs agrégés post-reprise avec positions distantes chacun, même état168/racine finale `0x1bd52fef315cfa2e2a50bb17ffec0b7cb29aabf1c24436b327baa3dded0ce3ce`.5 programmes natifs, deux architectures et36 fichiers de banc inchangés ; conteneurs0 sans OOM, signataires/horloges healthy. Pas de pruning/charge/restauration ni latence asymétrique dans ce lot ; n’étend pas automatiquement les autres périmètres. Campagne sortie/retrait actuelle démarre séparément après terminal, encore ouverte. Grande échelle/cycle complet/sanctions, WAN A, croissance/coupure physique, bridge/Doppler, herméticité et revue indépendante restent ouverts.


### 2026-10-05 — Sortie/reprise/retrait WAN actuel : terminal qualifié dans son périmètre

`docs/qualification/current-exit-withdrawal-terminal-20261005` : banc2c8fc30a/runtimeab10 terminal0, Mac natif + VPS AMD64,2 hôtes/64 clés publiques synthétiques. Exit valide inclus avant finalité, SIGKILL137, survivant demi-stake sans finalité au-delà cutoff, même conteneur/volume/config/identité repris. Finalité commune7, bloc46/hash `0x4902ce289614c6db89db6c90d35d719d41399e2f40385ec0600b8891e9f0f30b` ; historique/paiements identiques,9 paiements dont1 capital, somme33009394462000000000wei égale aux deux soldes finalisés, exit canonique unique. Rejeux C++ readonly natif/VPS terminaux0, tokens d’arrêt indépendants et ascendance du checkpoint observé vérifiés. Échec collecteur1 conservé : exigence de têtes identiques après arrêts successifs invalide pour cette observation ; hauteurs63/64 distinctes, finalité commune prouvée, aucune extension linéaire des têtes prétendue, pas de rejeu réseau/audit. CI37256392186 success460 opérations ; C++/réseau skipped explicitement. Fixture automaticRecovery/paramètres accélérés, deux nœuds, pas de coupure physique/pruning/charge/grande échelle ni cycle admission/activation/sanction WAN complet. Autres écarts et revue indépendante restent ouverts ; production globale non atteinte.


### 2026-10-05 — Sanctions WAN : nouveau helper, admission réelle validée

`docs/qualification/slashing-wan-helper-20261005` : helper C++ séparé, cœur runtimeab10 inchangé. Preuve BLS d’équivocation publique key0, paire inversée, quatre falsifications vérifiées avant export ; sortie existante refusée intacte. Bootstrap IPC privé impose identité chaîne/genesis et décisions exactes rejected/accepted, local_failure n’est jamais un rejet réussi.4 scénarios serveur synthétique passent ; nœud natif réel refuse4 preuves falsifiées et accepte preuve/rejeu inversé, arrêt0 horloge/signataire healthy. Première compilation1 conservée, finale compile/link0 ;5 programmes existants rehashés inchangés. CMake/CI helper ajouté,48 tests scriptsCI passent, nouvelle CI pas encore terminale. Aucune sanction finalisée/relai WAN qualifiée ici, aucun rebuild d’image. Prochain lot : compilation helper Git VPS et scénario WAN borné inclusion/sanction/reprise/idempotence/finalité. Cycle complet admission/activation/sanction, échelle, autres écarts financiers et revue indépendante demeurent ouverts.


### 2026-10-05 — Helper sanction AMD64 terminal, runtime conservé

`docs/qualification/slashing-wan-helper-amd64-20261005` : source Git1bad38f2, compilation/link seuls du helper sur image/volume existants, terminal0 sans OOM,5 programmes existants rehashés inchangés. Même SHA256 source que natif ; preuve JSON complète générée identique (valide/inversée/4 variantes falsifiées). Pas de rebuild d’image ni sanction WAN/finalité/reprise qualifiée par ce lot ; prochaine campagne reste ouverte avec tous les autres critères/revue indépendante.

### 2026-10-05 — Préparation de l’audit de sanction finalisée

Nouvel outil privé `nano-capella-slashing-audit` compilé nativement contre la bibliothèque candidate ab10b8ae, sans reconstruire le nœud ni les images Docker. Trois refus vérifiés : validateur non sanctionné, signature falsifiée, engagement externe incorrect. Première erreur du contrôle du témoin conservée et corrigée : vérifier son intention `next` et le hash du snapshot, et non assimiler son jeton au jeton principal. Preuves : `docs/qualification/slashing-audit-preparation-20261005`. Sanction WAN positive, rejeu après reprise, finalité commune et exécution AMD64 restent ouverts ; aucun critère de production clôturé par cette préparation.

### 2026-10-05 — Échec CI du générateur BLS et correction CMake

Run 37258158600 terminé en échec : `blst.h` absent des includes de la cible privée `nano-capella-slashing-fixture`. Les compilations manuelles précédentes ne prouvaient pas cette cible CMake. Includes explicités pour générateur et auditeur ; nouvelle configuration CMake et compilation des deux unités avec leurs seules options générées réussies. Preuves et échec conservés dans `docs/qualification/slashing-cmake-include-fix-20261005`. CI complète après correction et sanction WAN restent ouvertes.

### 2026-10-05 — Auditeur AMD64 et scénario WAN de sanction/rejeu

Auditeur AMD64 lié contre le cœur ab10b8ae dans le volume existant : sortie 0, sans OOM, six programmes/bibliothèque existants inchangés, aucune image reconstruite. Preuves `docs/qualification/slashing-audit-amd64-preparation-20261005`. Nouveau banc privé `tests/capella/slashing-multihost.mjs` : preuve valide et quatre preuves forgées, sanction canonique, SIGKILL après finalisation, rejeu au redémarrage, avancement commun de deux époques et audits des bases arrêtées avec engagements externes et témoins séparés. Banc préparé ; aucune réussite WAN ni production revendiquée avant sortie terminale et audits.

### 2026-10-05 — Premier essai WAN sanction : échec du collecteur époque zéro

Banc 49f865ad/cœur ab10b8ae : sortie terminale 1, requête erronée du checkpoint finalisé nul d’époque 0 comme bloc Beacon (HTTP404). Nœuds arrêtés, volume et logs conservés. Correction du seul collecteur : sentinel/anchor initial vérifié et aucune tentative de qualification avant une époque positive ; contrôles des finalités ultérieures inchangés. Preuves `docs/qualification/slashing-wan-epoch-zero-failure-20261005`. Sanction et reprise WAN restent non qualifiées.

### 2026-10-05 — Sanction WAN/rejeu observés, audit natif signé réussi

Banc732ba1f3/cœurab10 : deux hôtes/64 clés synthétiques ; finalité commune2→4 en89537ms après SIGKILL137/reprise et rejeu, une inclusion canonique observée par nœud. Résultat original FAILED conservé (référence profile maximumBytes invalide) ; pas de réseau relancé. Requêtes offline corrigées : rejeu canonique standard réussi, premier auditeur échoue sur parent historique non retenu. Auditeur corrigé reconstruit depuis archives signées ; première erreur de wrapper JSON conservée. Audit natif terminal0 : une preuve/une transition slashed, slot3, pénalité immédiate2000000000Gwei, preuve retirée, token externe/témoin et état finalisé vérifiés readonly. Audit AMD64 corrigé encore ouvert. Preuves `docs/qualification/slashing-wan-native-archive-audit-20261005`. CI ajoute compilation auditeur et refus forgé sans création de DB ; bloc exact passé localement, CI complète non revendiquée. Production et autres critères restent ouverts.

### 2026-10-05 — Audits signés de sanction/rejeu Mac–VPS terminaux

`docs/qualification/slashing-wan-stopped-audits-terminal-20261005` : deux audits arrêtés terminal0, même preuve/sanction/solde finalisé. Banc réseau732ba1f3/cœurab10, auditeur corrigé1b4f2b79 ; résultat original FAILED conservé et réseau non relancé. Archives signées rejouées : une preuve canonique, une transition slashed au slot3/root8c1c0b99…, débit immédiat2000000000Gwei et solde finalisé30997712922Gwei identique, preuve retirée, ascendance finalisée externe/token principal/témoin séparé vérifiés readonly. SIGKILL137 après époque2, reprise même conteneur/volume/config/peer, deux bootstraps avec4 refus forgés et preuve/rejeu inversé acceptés ; finalité commune2→4 en89537ms. Programmes runtime inchangés, build du seul auditeur AMD640 sans OOM, aucune image reconstruite. Qualification bornée du validateur existant synthétique0 sur2 hôtes/64 clés, pas cycle admission/activation/sanction complet, échelle ou production. CI37260639656 encore en cours lors capture ; échecb831 conservé. Autres critères/revue indépendante restent ouverts.

### 2026-10-05 — Nouveau fixture dépôt natif réseau et reçu RPC

Option privée `--export-deposit-network` : contrat de dépôt natif lié au profil, fonds synthétiques,64 membres et clé65 en attente (index automatique), transaction type2 signée avec nonce du seed. Exporteur compilé séparément, compile/link/export0 ; six programmes/bibliothèque existants inchangés. Nœud natif réel exécute le dépôt par RPC : hash attendu, reçu réussi/un événement, arrêt0 et horloge/signataires sains,65 clés locales/dont1 awaitingRegistration. Preuves `docs/qualification/native-deposit-network-fixture-20261005`. Ne qualifie pas encore registration consensus, activation/duties, cycle complet ou WAN ; aucune image/fonds réels/déploiement public. CI1b4 encore en cours lors dernier contrôle.


### 2026-10-05 — Exporteur de dépôt : terminal AMD64 et comparaison native

Preuves : `docs/qualification/native-deposit-amd64-terminal-20261005/`. La compilation isolée du test à la révision `d50cb87240f1c6ce46bba513a672547d580d5a4a` est terminée avec code 0, sans OOM ; les sept programmes/bibliothèques protégés de la candidate `ab10b8ae5dd50c7edf6f919ed440aa51a46e5e0e` sont inchangés. L’export AMD64 termine avec code 0 et produit exactement la même transaction signée et les mêmes métadonnées de dépôt que l’export natif Mac. Deux essais d’export échoués (montage des fixtures absent, puis /tmp non inscriptible) sont conservés, suivis de l’export corrigé. Aucun rebuild d’image.

La CI 37260639656 sur `1b4f2b794bf7537203bd72d47421c03fb60aff4b` est terminée avec succès (core et operations) ; network est skipped. Cette CI ne qualifie pas le nouveau dépôt réseau. L’enregistrement, l’activation et les duties WAN du nouveau membre restent ouverts ; production non qualifiée.


### 2026-10-05 — Nouveau membre : campagne privée WAN démarrée

`tests/capella/deposit-multihost.mjs` à `aae450ccdbb2d9beac335c70862232e68836a6c5` utilise la candidate runtime `ab10b8ae5dd50c7edf6f919ed440aa51a46e5e0e`, les binaires natifs Mac et AMD64 VPS existants, 64 membres initiaux et une clé publique synthétique en attente d’index 64. Preuves de démarrage : `docs/qualification/native-deposit-wan-started-20261005/`. Les deux RPC ont observé le dépôt signé réussi (`status=0x1`) au bloc 3, avec un log de dépôt chacun. Ce constat d’exécution ne démontre pas encore l’enregistrement/activation dans le consensus.

Le scénario, borné à 30 minutes, exige une proposition du nouveau membre dans une finalité commune avant SIGKILL, puis une nouvelle proposition postérieure au slot de coupure et une finalité commune avancée après reprise du même conteneur/volume/identité. Session de contrôle 35251, logs `/private/tmp/nano-deposit-wan-control-20261005/driver.log`, données `/private/tmp/nano-deposit-wan-20261005`. Aucun résultat terminal annoncé ici ; l’audit authentifié du registre et des journaux/témoins reste ouvert même si le probe réseau réussit. Aucun fonds réel, exposition publique ou rebuild d’image.


### 2026-10-05 — Auditeur du cycle dépôt/activation : préparation native

`tests/capella/deposit_audit.cpp` et la cible `nano-capella-deposit-audit` reconstruisent les transitions signées depuis le seed authentifié ; ils imposent une transaction de dépôt exacte, un dépôt consommé et une transition de registre uniques, 64→65 membres, la clé et les credentials de retrait attendus, puis deux propositions du nouveau membre de part et d’autre du slot de coupure. L’audit canonique existant, la finalité externe et le token/témoin indépendant du stockage restent exigés. Les journaux/témoins individuels des clés ne sont pas encore qualifiés par cet outil.

Preuves : `docs/qualification/native-deposit-audit-preparation-20261005/`. Compilation/link natifs isolés réussis, programmes runtime protégés inchangés ; un échec de compilation initial sur l’accès au registre map est conservé. Trois cas négatifs (index, clé, montant incorrects) sont rejetés avant ouverture/création de base. La cible est ajoutée à la compilation et à l’inventaire CI. Ni compilation AMD64 de cet auditeur ni audit positif du réseau ne sont annoncés comme acquis ; la campagne WAN continue sur son harness figé `aae450cc`.


2026-10-05, complément auditeur dépôt : le budget d’ascendance signée utilise désormais `maximumBlocks` du request externe (1 à 1 024), distinct des 256 états/blocs du snapshot conservé. La modification concerne uniquement l’outil d’audit ; les limites du runtime restent intactes. Compilation/link natifs de cette version réussis (`attempt4`, `archive-budget-update.json`). La première version AMD64 est terminée avec code 0 et sept artefacts runtime inchangés, mais sa qualification ne s’étend pas à cette modification de budget : compilation AMD64 courante et audit positif restent ouverts.


2026-10-05, complément engagements des signataires : l’auditeur dépôt vérifie maintenant l’inventaire des tokens de clés relevés à l’arrêt, leur identité (dont la clé à index automatique), les checkpoints des journaux et témoins séparés, puis la réouverture du `BeaconSigningJournal` en lecture seule. Il refuse un primary en retard avant toute tentative de réparation et exige l’avancement du watermark de proposition du nouveau membre après la coupure. Compilation/link natifs réussis (`attempt5`), aucun audit positif annoncé avant arrêt des nœuds. La compilation AMD64 précédente avec le budget d’archive séparé est terminée avec code 0 (`amd64-archive-budget-terminal.txt`) ; cette preuve ne qualifie pas encore l’extension des signataires.


2026-10-05, terminal AMD64 de l’auditeur avec engagements des clés : le conteneur `nano-deposit-audit-amd64-v3-20261005` est arrêté avec code 0, sans OOM. Source SHA256 `0f6a9f87c03b22c1f2ecb76dd9f7452025b05f9a57a6a0a50695c507fa58924c`, sept artefacts runtime inchangés ; pins des outils natif/AMD64 dans `docs/qualification/native-deposit-audit-preparation-20261005/tools-current.json`. La compilation des deux outils est acquise ; leur audit positif sur les deux bases arrêtées reste ouvert. Aucun nouvel état du réseau ni de readiness production n’est déduit de cette compilation.


### 2026-10-05 — Dépôt WAN : échec conservé, diagnostic sur bases arrêtées

Campagne `aae450cc`/runtime `ab10b8ae` terminale avec code 1 : après une proposition du nouveau membre 64 au slot 208 finalisée en commun à l’époque 26, SIGKILL 137 et reprise du même conteneur/volume/configuration/identité, la lecture des checkpoints de l’état de tête a régressé. Les processus sont arrêtés ; le conteneur finit avec code 0 sans OOM. Ne pas convertir ce scénario en réussite. Preuves : `docs/qualification/native-deposit-wan-head-state-regression-20261005/`, données et logs complets conservés dans `/private/tmp/nano-deposit-wan-20261005` et le volume VPS identifié.

Les deux audits canoniques read-only terminent avec code 0, vérifient 227 blocs et l’ascendance du checkpoint externe 26 ; les deux états durables arrêtés annoncent le même checkpoint 27/root `0x529881dac167af0aae080581dd0c94a02f704073bc97d8431cfc3ad455f77ba7` et le même head canonique. Cela soutient le diagnostic d’un checkpoint d’état de tête en retard après reorg, mais ne qualifie pas toutes les transitions historiques du fork-choice (`historicalForkChoiceVerified=false`). Les propositions post-reprise du membre 64 et l’audit positif des journaux/témoins individuels restent ouverts.

L’observateur du probe est corrigé pour confirmer les checkpoints avec `/eth/v1/beacon/headers/finalized` signé/canonique, garder seulement des couples époque/root effectivement observés, refuser une régression durable et attendre si l’époque n’est pas encore connue ; aucune époque n’est déduite d’un slot potentiellement manqué. Cinq nouveaux contrôles passent. L’auditeur dépôt extrait les sept champs stricts destinés à l’audit canonique au lieu de lui transmettre ses métadonnées supplémentaires ; l’ancien refus d’interface et le contrôle du checkpoint 26 déjà élagué sont conservés. Compilation native corrigée réussie ; l’AMD64 courant et l’audit positif complet restent ouverts. Les trois refus de métadonnées sont raccordés à la CI ; bloc exact exécuté avec succès. CI 37262765650 sur `90171cec` est verte (core/operations, network skipped), sans extension aux changements ultérieurs.


2026-10-05, contrôle d’interface complémentaire : après fourniture du checkpoint durable 27 encore conservé et du slot réel de coupure, la requête enrichie passe l’audit canonique puis est refusée avec `distinct proposals required` lorsque les deux propositions sont volontairement identiques (`post-fix-semantic-control-v3.*`). Les contrôles précédents — état 26 élagué, puis slot de coupure manquant — restent conservés. Ce refus est attendu et ne qualifie aucune proposition post-reprise. L’observateur utilise aussi l’en-tête finalisé lorsque le checkpoint de tête revient temporairement à zéro après une époque positive.


### 2026-10-05 — Restauration du dépôt : refus de configuration conservé

La tentative `d0823af9` est terminale avec code 1 avant reprise des duties : `Witness recovery cannot use a restart token`. Le runtime a refusé une configuration mélangeant token de redémarrage direct et récupération par témoin. Les originaux arrêtés et leurs engagements restent conservés ; les copies de cette tentative ne sont pas une qualification. Preuves : `docs/qualification/native-deposit-resume-refusal-20261005/`.

Le probe utilise désormais `initial=null`, `restart=null`, l’engagement de cœur externe dans `recoveryCommitment`, et les engagements individuels des clés. L’auditeur charge son ancre externe depuis la requête, indépendamment de la configuration de reprise ; compilation/link natifs v7 réussis, runtime inchangé. La prochaine tentative privée conserve le profil de consensus, avec budget administratif de replay 1024 slots et fenêtre d’horloge correspondante (6144000 ms). Cette fenêtre élargie ne qualifie pas la politique opérationnelle originale de 120000 ms ni une politique de récupération longue de production. Les propositions nouvelles, la finalité commune qui avance et les deux audits positifs restent à démontrer.


2026-10-05, auditeurs de reprise courants : natif v7 et AMD64 v5 compilés/liés avec succès depuis le même SHA256 source `4c4bb8e10b0bc6fc026f0933a9c540f924835a2fd50d07704c84edbf1c68b629`. Conteneur AMD64 arrêté avec code 0, sans OOM, sept artefacts runtime inchangés ; empreintes dans `docs/qualification/native-deposit-audit-tools-v7-v5-20261005/tools.json`. Ces compilations ne prouvent pas encore les audits positifs. La nouvelle campagne de reprise utilise la révision de probe `c95ba023` et conserve le runtime `ab10b8ae` ; aucune réussite terminale de cette campagne n’est revendiquée ici.


### 2026-10-05 — Dépôt et reprise épinglée : terminal positif, deux audits

Probe `c95ba023`, runtime `ab10b8ae`, deux hôtes Mac natif/VPS Docker : restauration intégrale depuis les originaux arrêtés avec engagements externes du cœur et de chaque clé, récupération automatique désactivée. Sortie 0 ; finalité commune époque 27→75/root `0x48cf6d7bbc9b07affee2be3e417ba4d1c7f9ff8f810f7d4a08c85ddbe87b0185`, trois observations stables, nouvelle proposition du membre 64 au slot 594/root `0xcf99798b7462ec4cca264f12cd2cedf74204b3d391f5f2207bb25172e669872b` (ancienne proposition 208, coupure 236). Nœuds arrêtés proprement ; VPS code 0 sans OOM, programmes runtime inchangés.

Auditeurs natif v7/AMD64 v5 : deux sorties 0, rejeu canonique de 368 blocs chacun, une transaction signée de dépôt, une inclusion et une admission 64→65 ; enregistrement slot 48, éligibilité époque 7, activation époque 13. Même solde finalisé du membre 64 : 31988426755 Gwei. Ancienne et nouvelle propositions signées vérifiées dans l’ascendance finalisée. Les tokens externes, primary et témoins indépendants concordent pour le cœur et les 32+33 clés ; inventaire exact 0…64, toutes les dernières cibles d’attestation 76 ou 77 dépassent l’époque de coupure 29. Le journal du membre 64 contient aussi une réservation au slot 616. Audits en lecture seule, sans réparation des bases.

Preuves : `docs/qualification/native-deposit-pinned-resume-terminal-20261005/`. Les originaux froids, la première campagne FAILED et le refus initial de configuration restent conservés ; ne jamais démarrer simultanément un original et sa copie avec les mêmes clés. Portée : deux hôtes, 65 clés synthétiques, profil Beacon/Shanghai, fenêtre administrative de rattrapage élargie 1024 slots/6144000 ms ; pas de nouvelle qualification Docker Mac dans cette campagne. `historicalForkChoiceVerified=false` ; ce rejeu ne prouve pas toutes les décisions historiques de fork-choice. La politique de reprise longue de production, le cycle complet admission/sanction/sortie du nouveau membre, les autres exigences de production et la revue indépendante restent ouverts. Aucun lancement public ni fonds réels.


2026-10-05, CI 37265527065 sur `c95ba02325583d3095cf2a025d3a51b06640a434` terminale avec succès : core et operations réussis, network skipped. Preuve terminale conservée dans le bundle de reprise épinglée. Cette CI complète les contrôles ciblés des outils ; la campagne WAN et les audits ont leurs preuves séparées et ne découlent pas du job network absent.


### 2026-10-05 — Sanction du membre admis : préparation authentifiée

Le générateur privé `slashing_fixture.cpp` accepte désormais `--generate-admitted` pour le membre synthétique 64 depuis la base arrêtée de la reprise qualifiée. Il vérifie ancre externe, rejeu canonique, token externe et témoin indépendant du cœur, registre exact de 65 clés publiques connues, activation et absence de sanction préalable. Aucune écriture dans les bases sources ; profils publics refusés. Le contexte de preuve est celui du checkpoint justifié, conformément à l’admission runtime ; le premier refus `invalid fork slashing context` (outil utilisant à tort la tête) est conservé.

Compilation/link natifs isolés réussis, six artefacts runtime inchangés. Génération positive pour le membre 64 ; quatre cas de signatures/index malformés refusés localement. Token externe falsifié : génération refusée sans fichier de preuve. Le mode historique du membre 0 génère exactement la même preuve et refuse toujours d’écraser un résultat existant. Preuves : `docs/qualification/admitted-validator-slashing-preparation-20261005/`. L’AMD64 courant, l’injection/propagation/inclusion WAN, la pénalité et la sortie/restitution du membre nouvellement admis restent à qualifier. Aucun nouveau résultat de production.


2026-10-05, générateur du membre admis porté en AMD64 : compilation terminale 0 sans OOM, sept artefacts runtime inchangés. Source SHA256 `1c37680ad32713706e07d3d41ae4eb025c40eead1b1fe1375be407ed006d9517`, pins dans `admitted-validator-slashing-preparation-20261005/tools.json`. Génération sur le volume VPS arrêté readonly : preuve exactement égale à celle du Mac ; sortie 0. Le premier refus `create proof output` dû aux permissions du dossier de sortie est conservé ; seul ce dossier a été corrigé dans une nouvelle tentative, sans mutation des bases sources. Portabilité du générateur acquise, sanction réseau et cycle complet toujours ouverts.


### 2026-10-05 — Auditeur de sanction du membre admis

L’auditeur privé de sanction reconnaît le membre 64 : signatures de preuve contrôlées avant ouverture des bases, ancre externe utilisable avec `initial=null`, admission unique vérifiée dans le rejeu canonique, puis preuve et transition `slashed` uniques, identité/solde finalisés et témoin du cœur liés au token externe. Le budget d’archive est séparé des états fork-choice retenus. Compilation/link natifs réussis, runtime inchangé ; premier refus de compilation BLS/span conservé. Sur les bases arrêtées : membre 64 non sanctionné correctement refusé ; preuve forgée refusée sans création de base ; effets historiques du membre 0 inchangés dans l’audit positif. Preuves : `docs/qualification/admitted-validator-slashing-audit-preparation-20261005/`. Audit positif du membre 64 et AMD64 courant restent ouverts.

Précondition de la future panne : `recoveryCommitment` doit correspondre exactement à l’intention du témoin. Un token externe de la restauration précédente devient donc obsolète après avancement. Ne pas réutiliser ce token pour simuler une reprise ni enlever la protection pour obtenir une réussite. Le banc doit capturer et conserver hors du VPS des engagements actualisés du cœur et des signataires à une frontière contrôlée avant SIGKILL ; qualifier séparément cette capture, la récupération et la portée exacte du crash. Aucune qualification de coupure matérielle arbitraire n’est déduite.


### 2026-10-05 — Capture privée des engagements à une frontière contrôlée

Nouvel outil readonly `nano-capella-paused-capture` : vérification de l’ancre externe, du rejeu canonique, des primary/témoins du cœur et de chaque signataire, des minima externes et relecture par de nouvelles ouvertures des bases en fin de capture. Compilation/link natifs v4 réussis, runtime `ab10b8ae` inchangé ; les deux échecs de compilation précédents restent archivés. Sur la source Mac froide arrêtée : capsule exacte du cœur et de 32 clés, trois demandes incohérentes refusées sans capsule. Le bloc CI de refus du bootstrap passe sans création de base ; cinq tests du raccordement des pins passent. Ces contrôles ne qualifient ni toute la CI actuelle ni une capture de nœud actif suspendu.

Le contrôleur doit prouver séparément la suspension de tous les écrivains, puis conserver durablement la capsule hors du VPS avant SIGKILL. L’outil ne déduit aucune santé du runtime ; une borne de séquence supérieure ne prouve pas l’ascendance du digest d’un ancien token. Capture AMD64, suspension effective, crash et reprise avec engagements actualisés restent ouverts. Preuves : `docs/qualification/quiescent-recovery-capture-preparation-20261005/`. Le cycle complet du nouveau validateur et la préparation production demeurent ouverts.

Auditeur de sanction AMD64 courant : contrôle positif historique du membre 0 terminé avec code 0, sortie conservée dans `admitted-validator-slashing-audit-preparation-20261005/amd64-legacy.stdout`. Cela ne qualifie toujours pas une sanction du membre nouvellement admis 64.


2026-10-05, capture AMD64 : compilation/link terminaux 0 sans OOM, même source SHA256 `37777c355732fc2a0dcca781b5e4b682f8a8a0cb5f173d02c75656a7df38092b`, binaire `8d26f5450a8a417a7b1dd62b7b20ccc34205355935e0cd0d603bef430eb436f2`, sept artefacts runtime inchangés. Capture readonly de la base VPS effectivement arrêtée : sortie 0, cœur et 33 engagements de signataires exactement égaux au reçu externe conservé, rejeu canonique et ascendance finalisée contrôlés. Preuves dans `quiescent-recovery-capture-preparation-20261005/amd64-build/` et `vps-cold-controls/`. Sources Mac/VPS froides qualifiées dans ce périmètre ; suspension de nœud actif, conservation durable avant SIGKILL et reprise avec engagements actualisés non encore qualifiées. Aucun nouveau lancement réseau ou public.


2026-10-05, nouveau banc `admitted-slashing-multihost.mjs` préparé : copies froides des deux bases qualifiées, preuve du membre 64, contrôle des refus et du rejeu via admission IPC, observation de la finalité publique signée. Avant SIGKILL : PID/conteneur/exécutable/volume inchangés, tous les threads PID 1 en état T sur deux lectures ; capture readonly vérifiée, capsule enregistrée hors VPS avec fsync, nouveaux pins appliqués sans récupération automatique. Le banc exige ensuite deux époques supplémentaires de finalité commune et conserve les reçus pour les audits arrêtés séparés. Sept contrôles de pins/suspension et syntaxe réussis ; l'exécution WAN et les audits de sanction positifs ne sont pas encore qualifiés. Portée prévue : frontière de publication contrôlée, pas panne physique arbitraire ; deux hôtes, pas nouvelle qualification à trois nœuds.


2026-10-05, première campagne de sanction du membre admis `55762ef6` : terminale 1/FAILED sur HTTP 503 VPS `/eth/v1/beacon/states/head/finality_checkpoints`, avant capture et crash. Bases originales toujours arrêtées ; copies de la tentative conservées, nœuds arrêtés proprement, VPS code 0 sans OOM. Finalité durable restée à l’époque 75 ; ne pas déduire une panne du consensus ni une réussite de sanction. Le corps de la réponse 503 n’était pas conservé : son sous-type et sa cause restent non prouvés. Preuves : `admitted-sanction-wan-first-refusal-20261005/`. Le nouveau banc comprend observation de suspension, capsule externe fsync et reprise strictement épinglée ; ses sept contrôles locaux ne qualifient pas encore l’exécution WAN. Il faut conserver les corps d’erreur et traiter uniquement les refus explicites de capacité/budget de lecture par tentatives bornées, sans ignorer une indisponibilité d’état ou déclarer le RPC qualifié.


2026-10-05, sanction du membre admis, WAN v2 `4d785775` / runtime `ab10b8ae` : terminal 1/FAILED sur délai, preuves `admitted-sanction-wan-liveness-failure-20261005/`. Les deux hôtes ont signé : Mac 600 attestations/73 propositions, VPS 537/74 ; les votes distants restent anciens dans leurs vues et les branches concurrentes ont des racines cibles différentes. Finalité durable inchangée à 75. Vérification BLS des 32 signatures simples de l’époque 170 dans 20 corps de blocs immuables échantillonnés sur la branche Mac : indices synthétiques 0–31 exclusivement ; ce périmètre ne prouve ni absence globale de votes VPS ni quorum de stake. Catalogue/IPC/dépendances de blocs et contention des lectures restent à diagnostiquer, cause non démontrée. Capture avant crash, SIGKILL, sanction finalisée et reprise épinglée non atteints. Les refus HTTP de capacité/budget sont maintenant tracés ; des reprises bornées de lecture ne qualifient pas le RPC. Aucun nouveau critère de production fermé.


2026-10-05, correction du relais d’attestations : reproduction native de la famine du catalogue lorsqu’une livraison répond négativement avant des votes indépendants suivants ; test nouveau FAILED sur ancien code, 45 scénarios PASSED après correction. Le parcours reste borné par le nombre d’entrées examinées et le délai, les refus sont séparés des admissions et ne deviennent jamais des ACK ; toute tentative contenant un refus reste FAILED. Un refus de bloc arrête toujours sa chaîne de dépendances. Preuves `attestation-catalog-rejection-scan-20261005/`. Un retour négatif peut refléter une dépendance refusée : cette reproduction ne démontre pas à elle seule la cause WAN. Compilation Docker ARM64/AMD64 et avancement de finalité après correction encore ouverts ; aucune qualification runtime ab10 héritée automatiquement par ce nouveau relais.


2026-10-05, relais corrigé `8adbabff` : Docker ARM64/Mac et AMD64/VPS terminaux 0, sans OOM ; 45 scénarios du catalogue par architecture et compilation/link P2P réussis dans les images/volumes existants, sources concordantes. Régression native de priorité de tête : 17 scénarios PASSED. P2P natif compilé ; sortie 1 attendue du contrôle d’usage CLI conservée séparément, aucun réseau lancé. Preuves ajoutées à `attestation-catalog-rejection-scan-20261005/`. La candidate devient une composition à identifier explicitement (cœur ab10 + relais nouveau) ; les anciennes preuves de restauration ne changent pas automatiquement de révision. Requalification WAN/finalité et reprise après sanction toujours ouvertes.


2026-10-05, préparation d’une composition explicite pour la requalification WAN : cœur `ab10b8ae`, relais `8adbabff`. Le banc vérifie l’absence de diff src/include/third_party contre le cœur, toutes les sources C++ contre la révision de composition, et les hashes exacts cœur/exporteur de la campagne arrêtée avant copie des données ; il conserve séparément la provenance des outils liés au cœur. Six contrôles de manifestes PASSED, aucun nouveau runtime compilé pour ces modifications JS. Les observations enregistrent maintenant les racines de tête, et le libellé de soumission de preuve est corrigé en VPS. Preuves `admitted-sanction-relay-composition-preflight-20261005/`. Compatibilité des contrôles ne vaut pas nouvelle réussite WAN ni qualification de reprise.


2026-10-05, précontrôle de reprise de la composition : diagnostic temporaire en lecture seule lié au cœur existant, capture terminale 0, digest primaire/témoin et journaux signataires concordants. Horloge durable 1791176963003 ms, observation locale 1791220621160 ms, écart 43658157 ms (~12 h 08), supérieur à la borne administrative 6144000 ms. Aucun démarrage WAN effectué, aucune remise à zéro d’horloge/genesis/journal, aucune mutation du stockage. Preuves `admitted-sanction-relay-composition-preflight-20261005/cold-clock/`. La première poll refuse actuellement l’écart d’arrêt comme un saut d’horloge. Il faut implémenter et qualifier un rattrapage initial explicite et borné par étapes, conservant les refus de régression/sauts en fonctionnement et les limites de rejeu, avant nouvelle requalification WAN. Longue interruption reste un critère de production ouvert.


2026-10-05, rattrapage initial borné implémenté : option explicite clock.startupCatchup {maximumDowntimeMs, maximumSteps}, pas de remise à zéro, chaque publication durable reste dans maximumForwardStepMs et la capacité replaySlots. Vérification des journaux configurés avant rattrapage, threads de signature après rattrapage et poll normale ; protections de régression/sauts en fonctionnement conservées. Native : 9 scénarios RocksDB/witness PASSED (oracle direct, budgets, préfixe durable, annulation, retour/jump, usage unique, défaut inchangé), 4 diagnostics d’horloge PASSED, runtime sans nouvelle option 64 clés/16 blocs/reprise témoins PASSED. Preuves `bounded-startup-catchup-20261005/`. Compilation incrémentale sur copie du cœur ab10, anciens artefacts protégés. Activation de l’option via configuration, Docker ARM64/AMD64, reprise des ~12 heures, migration de snapshot/pins et finalité WAN restent à qualifier ; aucune clôture du critère production longue interruption.


2026-10-05, cœur `6b7a437e` : Docker ARM64/AMD64 terminaux 0 sans OOM ; nano/core/test compilés, 9 scénarios startup + 4 diagnostics horloge + runtime 64 clés/16 blocs par architecture, parents inchangés. Activation réelle de startupCatchup sur copie arrêtée, Docker ARM64 sans réseau/transactions : horloge rattrapée en 8 publications bornées, puis validateur FAILED `Capella slot work budget exceeded`, 0 proposition et 0 attestation ; arrêt propre. Preuves `bounded-startup-catchup-20261005/docker/` et `cold-activation/`. Reprise après ~12 h toujours FAILED : préparer les états de duties/blocs depuis l’ancien head exige encore une continuation bornée sur l’intervalle manqué. Les règles de profil, les journaux et le stockage source restent préservés ; cette réussite d’horloge ne ferme ni reprise globale ni finalité WAN.


2026-10-05, continuation des slots manqués : primitive CapellaSlotPreparation authentifiant le post-état contre son vrai bloc parent, état dérivé privé et étapes bornées avec préfixe conservé après annulation. Docker ARM64 terminal 0 sans OOM ; 11 scénarios PASSED, égalité exacte avec rejeu monolithique sur deux fenêtres historiques, refus de faux parent/état avancé présenté comme bloc signé et budgets invalides. Preuves authenticated-slot-continuation-20261005/. Pas encore raccordée aux duties, construction/admission blocs/votes/contextes cibles ; AMD64, reprise réelle et finalité WAN restent ouverts. L’échec de reprise froide précédent est conservé, aucun critère production fermé.


2026-10-05, continuation authentifiée 0501b7d9 : Docker AMD64/VPS terminal 0 sans OOM, 11 nouveaux scénarios PASSED comme ARM64 ; hashes sources vérifiés sur les deux architectures et parents inchangés. Mise à jour VPS par Git pull, images/volumes réutilisés. Preuves authenticated-slot-continuation-20261005/amd64/. Primitive qualifiée dans ce périmètre ; raccordement runtime et reprise réelle longue interruption restent ouverts.


2026-10-05, contextes issus de la continuation : projections lecture seule du proposant (index/clé/domaine) et du contexte cible d’attestation à frontière exacte, racine du parent authentifiée et overflow refusé. Docker ARM64 : premier test compile FAILED (nom de champ incorrect), diagnostic conservé ; cœur réutilisé après correction, 19 scénarios PASSED et régression runtime 64 clés/16 blocs PASSED, sortie 0 sans OOM. Preuves prepared-duty-contexts-20261005/. Calcul historique du proposant refactorisé sans lever son budget. Raccordement scheduler/reactor/blocs/votes, AMD64 et reprise réelle longue interruption/finalité restent ouverts ; aucune signature nouvelle ni critère production fermé.


2026-10-05, contextes préparés 2102a625 : Docker AMD64/VPS terminal 0 sans OOM, 19 scénarios PASSED et régression runtime existant 64 clés/16 blocs/reprise témoins PASSED comme ARM64. Sources concordantes vérifiées, parents inchangés, VPS reçu par Git pull. Preuves prepared-duty-contexts-20261005/amd64/. Projections pures qualifiées ; raccordement scheduler/reactor/construction-admission blocs-votes, reprise froide et finalité WAN restent ouverts.


2026-10-05, transition de bloc préparée : CapellaBlockPreparation conserve le profil complet et le parent authentifié, prépare jusqu’au slot précédent puis traite normalement le slot final et tout le bloc signé. Docker ARM64 terminal 0 sans OOM : 28 contrôles PASSED, deux blocs BLS avec transactions type 2 après deux fenêtres historiques, dont frontière d’époque ; égalité exacte avec rejeu complet, falsifications re-signées et annulation refusées, anciens budgets inchangés. Preuves prepared-block-transition-20261005/. Construction/vérification pure qualifiée ; AMD64, cache scheduler/reactor, publication Store/fork-choice, reprise froide et finalité WAN restent ouverts. Aucun fonds réel ni signature réseau, aucun critère production fermé.


2026-10-05, transition préparée 3adf73d2 : Docker AMD64/VPS terminal 0 sans OOM, mêmes 28 contrôles PASSED que ARM64 ; sources concordantes et hashes parents préservés, Git pull utilisé. Preuves prepared-block-transition-20261005/amd64/. Périmètre construction/vérification pure fermé ; intégrer maintenant les caches par parent/profil et slot exact au scheduler/workers et les contextes cibles/source d’attestation, sans lever les contrôles token/temps/journal. Reprise froide et finalité WAN restent ouvertes. Tout changement de layout du reactor impose une recompilation complète des unités affectées.


2026-10-05, worker de blocs entrants raccordé : cache unique éphémère par parent/profil, préparation par appel limitée à min(replaySlots, slotsPerEpoch), aucun checkpoint avant validation complète ; contrôle de profil canonique et publication Store/fork-choice normale. Docker ARM64 terminal 0 sans OOM, 42 contrôles PASSED via vrai worker/retries bornés, token/EVM/archives/témoin/réouverture/idempotence concordants ; runtime existant 64 clés/16 blocs PASSED. Changement ABI reactor : 17 unités affectées recompilées, main/test frais ; parents préservés. Preuves prepared-incoming-worker-20261005/. AMD64, retries P2P autonomes, cache duties locales/cibles/source/sync, reprise froide et finalité WAN restent ouverts. Un retour négatif inachevé ne devient jamais un ACK ni une preuve de signature invalide. Autres outils instanciant le reactor à recompiler ; aucune qualification précédente héritée automatiquement.


2026-10-05, worker préparé 37418632 : Docker AMD64/VPS terminal 0 sans OOM, 42 contrôles et runtime existant 64 clés/16 blocs PASSED comme ARM64. Sources SHA256 vérifiées contre le dépôt, 17 unités affectées ABI + main/test recompilés, parents conservés. Preuves prepared-incoming-worker-20261005/amd64/. Admission/publication du worker et reprises manuelles bornées qualifiées dans ce périmètre ; retries P2P automatiques, duties locales/cibles/source/sync, reprise froide et finalité commune WAN restent ouverts. Aucune clôture production ni héritage automatique de qualification des outils anciens.


2026-10-05, duties locales préparées : frames privés head/cible/frontière précédente, profil complet et vrais parents, préchauffage borné avant énumération ou réservation. Raccordement proposant/RANDAO/corps/signature/admission immédiate, source réalisée et cible distincte d’attestation, sync. Docker ARM64 terminal 0 sans OOM : 62 contrôles PASSED (20 nouveaux sur deux variantes), runtime 64 clés/16 blocs, timing 4 cas, santé signataire 2 cas et scheduler 40 blocs/272 votes/1280 sync/finalité 3 PASSED. Deux erreurs de compilation corrigées et conservées ; unités inchangées réutilisées, ABI recompilée. Preuves local-duty-continuation-20261005/. AMD64, votes gossip/différés/inclus, revalidation durable longue interruption, reprise froide et finalité commune WAN restent ouverts. Pas de qualification production ni réseau déduite.


2026-10-05, duties locales 6c97c0b1 : Docker AMD64/VPS terminal 0 sans OOM, sources SHA256 concordantes, 18 unités cœur concernées + main/test recompilés, parents préservés. Mêmes 62 contrôles, runtime 64 clés/16 blocs, timing 4 cas, santé signataire 2 cas et scheduler 40 blocs/272 votes/1280 sync/finalité 3 PASSED comme ARM64. Preuves local-duty-continuation-20261005/amd64/. Git pull utilisé, aucune image reconstruite. Votes après longue interruption, revalidation des archives/contextes de justification, reprise froide réelle et finalité WAN restent ouverts ; ces bancs isolés ne qualifient pas la production.


2026-10-05, continuation des cibles de votes et retarget monotone des duties : frames authentifiés par bloc/profil/cible, cache gossip des deux époques du protocole, progression bornée sans publication prématurée, cible locale partagée avant signature, application différée et restauration indépendante par chunks. Conservation du préfixe lorsque le slot/époque avance sous le même head ; rebind branche/profil. Docker ARM64 final terminal 0 sans OOM, 22 unités cœur concernées puis réparations ciblées, 96 contrôles PASSED, runtime 64 clés/16 blocs et régressions timing/signataire/scheduler PASSED. Échec compilation et régression runtime head→slot futur conservés et corrigés. Preuves target-vote-continuation-20261005/. AMD64, votes inclus/contextes de justification, coût maximal de validation/reprise, activation froide réelle et finalité WAN restent ouverts ; aucune clôture production.


2026-10-05, cibles de votes et retarget b2c952b6 : Docker AMD64/VPS terminal 0 sans OOM, 22 unités concernées + main/test recompilés, hashes sources concordants et parents conservés. Mêmes 96 contrôles, runtime 64 clés/16 blocs, timing 4 cas, santé signataire 2 cas et scheduler 40 blocs/272 votes/1280 sync/finalité 3 PASSED comme ARM64. Preuves target-vote-continuation-20261005/amd64/. Sources reçues par Git pull, image/volume réutilisés. Contextes des votes inclus et des mises à jour de justification, validation/reprise sous bornes de ressources, activation froide réelle et finalité commune WAN restent ouverts. Aucun critère global de production fermé.


2026-10-05, contextes préparés des votes inclus et de justification : API locale typée partageant les frames authentifiées par bloc/profil/cible avec on_block et on_tick ; fallback historique borné conservé. Docker ARM64 initial terminal 0 sans OOM, 106 contrôles et régressions runtime/scheduler PASSED ; extension de tests seulement terminal 0 sans OOM, 116 contrôles dont deux vrais blocs signés BLS contenant une attestation après deux fenêtres historiques. Store exact égal au rejeu complet ; ancien budget 3 refuse encore le rejeu intégral, frames incomplets/profil divergent/signature falsifiée/annulation refusés. Le test on_tick utilise un checkpoint unrealized explicite : il ne prouve pas sa production par quorum FFG. Preuves prepared-ffg-contexts-20261005/. Raccordement des caches Reactor aux nouvelles API, reprise froide réelle et finalité WAN restent ouverts ; AMD64 à qualifier. Aucun critère global production fermé.


2026-10-05, contextes votes inclus/FFG bd0638fd : Docker AMD64/VPS terminal 0 sans OOM, mêmes 116 contrôles que ARM64, runtime 64 clés/16 blocs et scheduler 40 blocs/272 votes/1280 sync/finalité 3 PASSED. Hashes des neuf sources concordants entre architectures et checkout, parents conservés ; image/volume réutilisés, sources transférées par Git pull. Preuves prepared-ffg-contexts-20261005/amd64/. Qualification des nouvelles API locales ; raccordement des caches Reactor, reprise froide réelle et finalité WAN restent ouverts, aucune clôture production.


2026-10-05, Reactor des cibles incluses/FFG : contexte manquant découvert seulement après transition signée complète, frame local par bloc/profil/cible et étapes bornées sans publication prématurée ; on_tick local et simulation du vote différé utilisent les cibles authentifiées par chunks. Nouveau layout : tous les consommateurs transitoires affectés recompilés. Premier ARM64 FAILED (doublon retraité par ancien chemin de vote, budget slots dépassé), diagnostic conservé ; réparation ciblée terminal 0 sans OOM, 130 contrôles et runtime/scheduler PASSED. Worker après deux fenêtres historiques : Store exact égal au rejeu complet, annulation/token/idempotence/archive/témoin réouvert vérifiés. Promotion tick testée sur metadata unrealized explicite, pas une preuve de quorum. Preuves reactor-target-continuation-20261005/. AMD64 et reprise froide réelle en cours de qualification ; reprise automatique P2P et finalité WAN restent ouvertes. Coût total de revalidation et de restauration sous charge à qualifier. Aucun critère global de production fermé.


2026-10-05, activation froide réelle aa9c4a26 sur copie isolée du dépôt arrêté : rattrapage horloge 9 étapes PASSED ; 1 proposition, 4 attestations et 20 messages sync admis, puis validateur unhealthy : « Locally signed Capella proposal refused by admission ». Qualification FAILED malgré sorties contrôleur/conteneur 0 sans OOM. Observation bornée 5 minutes, cadence 100 ms et règles inchangées, 1114 polls de préparation différée, max poll 362 ms ; aucune finalité commune démontrée. Preuves reactor-target-continuation-20261005/cold-activation-failure/. Copie des données et journaux conservée sous /private/tmp/nano-reactor-cold-activation-20261005/data avec nouveau reçu d’arrêt pour toute reprise ultérieure ; original inchangé, aucun peer/flood/fonds. Prochain diagnostic : distinguer invalidité réelle et admission locale différée sur contexte inclus manquant, puis vérifier préchauffage/retry sans resigner ni effacer journaux. Le précédent échec « slot work budget exceeded » demeure historique ; nouvelle cause encore ouverte.


2026-10-05, Reactor cibles/FFG aa9c4a26 : Docker AMD64/VPS terminal 0 sans OOM ; 22 unités transitoires affectées + main/test recompilés, neuf hashes sources concordants avec ARM64 réparé et checkout. 130 contrôles, runtime 64 clés/16 blocs et scheduler 40 blocs/272 votes/1280 sync/finalité 3 PASSED. Preuves reactor-target-continuation-20261005/amd64/. Git pull utilisé, image/volume réutilisés. L’activation froide réelle distincte reste FAILED (refus d’une proposition locale après reprise partielle des duties), comme documenté ci-dessus ; les bancs isolés ne ferment ni cette cause ni la finalité WAN ni la production.


2026-10-05, admission immédiate avec cible prête : nouveau test sur cœur aa9c4a26 inchangé FAILED « ready included target unnecessarily deferred local admission ». Le Reactor réutilisait un contexte prêt puis renvoyait systématiquement false. Correction : réévaluer la transition signée complète dans le même appel après réutilisation authentifiée, reconstruire les vues de pointeurs après croissance du cache ; préparation réellement incomplète toujours bornée/différée sans checkpoint. Docker ARM64 terminal 0 sans OOM, Reactor seul recompilé (layout inchangé), main/test frais ; 134 contrôles et runtime/scheduler PASSED. Preuves ready-target-admission-20261005/. AMD64 et reprise réelle de la copie ayant échoué restent à qualifier ; engagements de signature avancés conservés, automaticRecovery=false. Finalité WAN, coût global des validations et production restent ouverts.


2026-10-05, reprise locale de la copie froide en échec avec 0948b747 : checkpoint et 32 journaux/témoins repris depuis leurs engagements avancés d’arrêt, automaticRecovery=false, cadence et profil inchangés, aucun peer. Observation bornée 2 minutes puis arrêt contrôlé terminal 0 sans OOM : validateur healthy, 9 propositions/82 attestations/348 sync admis, aucune failure. Séquences signataires ≥ reçu précédent, checkpoint avancé, finalité 75/racine inchangées vérifiées contre ce reçu. Preuves ready-target-admission-20261005/cold-resume/. Nouveau reçu public conservé pour tout redémarrage ; données actuelles /private/tmp/nano-ready-target-cold-resume-20261005/data. Qualification locale de reprise après refus ; audit canonique complet du saut historique, finalité commune Mac/Docker/VPS, P2P et ressources restent ouverts. La reprise 14h seule antérieure FAILED demeure dans l’historique ; aucune clôture production.


2026-10-05, admission avec cible prête 0948b747 : Docker AMD64/VPS terminal 0 sans OOM, Reactor seul recompilé contre cœur complet au layout courant, main/test frais, neuf hashes sources égaux à ARM64 et checkout. 134 contrôles, runtime 64 clés/16 blocs et scheduler 40 blocs/272 votes/1280 sync/finalité 3 PASSED. Preuves ready-target-admission-20261005/amd64/. Git pull utilisé, image/volume réutilisés. La reprise locale saine ci-dessus ne ferme pas l’audit canonique complet du saut historique (src/consensus_capella_audit.cpp utilise encore le rejeu complet borné), ni la finalité commune réelle à trois nœuds, ni les ressources ni la production.


2026-10-05, audit canonique par continuation : nouvel audit local prépare le parent signé par chunks puis exécute normalement le bloc entier ; contextes FFG locaux pareil, anciennes API et budgets inchangés. Audit réel avec ancien binaire sur données arrêtées FAILED « Capella slot work budget exceeded », conservé. Docker ARM64 terminal 0 sans OOM, fork_store/audit recompilés (layout inchangé), main/test frais ; 146 contrôles et runtime/scheduler PASSED, dont oracle on_block complet, budget historique toujours refusé, signature BLS invalide/budget zéro/annulation rejetés. Preuves resumed-canonical-audit-20261005/. Capture réelle de la chaîne et des 32 journaux/témoins en cours ; AMD64 à qualifier, aucun critère réseau/production fermé. L’audit ne prétend pas vérifier le fork-choice historique ni les votes standalone.


2026-10-05, audit réel ARM64 du stockage restauré avec nouveau cœur 2795d4b5 : capture readonly terminal 0 sans OOM, outil recompilé contre ce cœur. 378 blocs canoniques/2 transactions rejoués, états et index vérifiés, ancêtre finalisé externe 75/racine confirmés ; checkpoint et témoin primaire concordants, 32 journaux/témoins BLS rouvertures vérifiés, séquences minimales et absence de retraite confirmées. 873 fichiers de données/journaux strictement inchangés par hashes/tailles. Preuves resumed-canonical-audit-20261005/actual-capture/. Fork-choice historique/votes standalone non qualifiés. Export initial de toute la base vers VPS refusé par auto-review (données sensibles/métadonnées de signature non explicitement autorisées) ; aucune donnée transférée. Archive locale revue 14,6 Mo/876 fichiers, clé peer.key et env exclus, transfert pour audit readonly AMD64 en attente d’accord dédié. Tests AMD64 des sources continuent ; finalité commune à trois nœuds et production restent ouvertes.


2026-10-05, audit par continuation 2795d4b5 : Docker AMD64/VPS terminal 0 sans OOM, fork_store/audit seuls recompilés contre cœur au layout courant, main/test frais, dix hashes sources égaux à ARM64 et checkout. 146 contrôles, runtime 64 clés/16 blocs et scheduler 40 blocs/272 votes/1280 sync/finalité 3 PASSED. Preuves resumed-canonical-audit-20261005/amd64/. Sources reçues via Git pull, aucune image reconstruite. Audit réel readonly du stockage/journaux qualifié sur ARM64 comme ci-dessus ; transfert de la copie synthétique pour audit physique AMD64 toujours en attente d’accord dédié. Finalité commune Mac natif/Docker Mac/VPS et autres critères globaux demeurent ouverts.


2026-10-05, audit readonly AMD64 des données déjà présentes sur VPS : outil paused_capture fraîchement compilé contre cœur 2795d4b5, parent binaire/core inchangés, source outil et programme hashés. Capture terminal 0 sans OOM, réseau désactivé, volume et configuration readonly, aucune clé chargée : 369 blocs canoniques/2 transactions, états/index et ancêtre finalisé externe époque 75 vérifiés ; checkpoint 4089/digest 7b96df78 concordant avec témoin, 33 journaux/témoins de signature vérifiés, séquences minimales respectées et aucun signer retraité. Preuves resumed-canonical-audit-20261005/vps-existing-capture/. Première tentative exit 1 conservée : chemin /var/lib/nano sans CURRENT ; correction du chemin existant /var/lib/nano/chain, sans modifier les données. Pas de comparaison SHA de tous les fichiers avant/après dans cette campagne. Aucun export de la copie Mac ; sa portabilité physique ARM64 vers AMD64 reste en attente d'autorisation dédiée. Fork-choice historique/votes standalone, reprise effective des duties et finalité commune à trois nœuds demeurent ouverts ; production non démontrée.


2026-10-05, candidate Mac natif actualisée contre cœur 2795d4b5 : fermeture des dépendances des cinq headers modifiés depuis parent 6b7a437e, 24 unités C++ recompilées (pas de relink avec ancienne disposition Reactor), main/test/paused_capture frais ; hashes des sources et parent archive préservés. Controller terminal 0, contrôles prepared-block 146, configured-runtime 16 blocs/64 clés et scheduler 40 blocs/272 votes/1280 sync/finalité 3 PASSED dans leur périmètre local. Audit readonly natif de la copie Mac restaurée arrêtée : 378 blocs/2 transactions, ancêtre finalisé externe 75, checkpoint 4381 et 32 journaux/témoins vérifiés ; 873 fichiers hashés avant/après inchangés. Preuves resumed-canonical-audit-20261005/native-current/. Aucune nouvelle signature sur cette copie, aucune donnée exportée au VPS ; trois nœuds/finalité WAN, portabilité physique AMD64 et autres critères globaux encore ouverts.


2026-10-05, préparation de reprise réelle à trois nœuds : sources Mac/VPS observées exited 0 et absence de processus natifs ; configurations externement épinglées, répartition complète des 65 signataires sans chevauchement (Mac natif 0–15, Docker Mac 16–31, VPS 32–64), neuf contrôles de préparation PASSED dont retrait, pin manquant, doublon, journal/témoin partagé, profil et finalité contradictoires. Deux copies locales Mac, chacune 872 fichiers SHA identiques à source hors peer.key ; copie VPS sur le VPS uniquement, 812 fichiers SHA identiques, identité P2P exclue. Sources/journaux/témoins préservés, aucune clé chargée. Composition explicite cœur 2795d4b5 + relais 8adbabff, hashes des six programmes natif/ARM64/AMD64 vérifiés sans rebuild. Preuves restored-three-node-prepare-20261005/. Il reste à créer le contrôleur de reprise/observation et lancer effectivement les trois duties ; préparation ne vaut pas finalité réseau. Export physique Mac vers AMD64 encore en attente d'accord, revue indépendante et tous les critères non démontrés restent ouverts.


2026-10-05, première reprise trois nœuds réellement lancée avec contrôleur 20932022 : FAILED après startup, Docker Mac exit 1 sans OOM (« Capella service requires local RPC and exclusive consensus configuration »), RPC configuré à tort 0.0.0.0 dans le contrôleur ; configuration héritée d’audit Mac contenait aussi network.enabled=false. Protection C++ préservée : contrôleur corrigé RPC 127.0.0.1, observations Docker Mac via docker exec curl ; P2P activé avec paramètres/budgets identiques au VPS. Native et VPS arrêtés avec reçus indépendants conservés ; zéro proposition/vote dans cette courte tentative, core checkpoints avancés au démarrage. Pas de relance avec anciens pins : nouveaux audits/pins et préparation de copies actualisées requis avant retry. Refus initial de préflight sur argument de révision erroné également conservé ; aucun signataire lancé lors de ce préflight. Preuves restored-three-node-attempt-20261005/. Finalité commune/reprise des duties et production restent non démontrées.


2026-10-05, requalification post-échec du contrôleur trois nœuds : audits readonly natif (inventaire complet 32, y compris 16 non chargés lors de la tentative) et VPS (33) terminaux 0 ; nouveaux core checkpoints 4382/9a1e8481 et 4102/83f5af33 réouverts avec témoins, états canoniques/ancêtre finalisé externe 75 et journaux vérifiés. Copies Mac suivantes dérivées de l’état natif post-tentative audité, tous fichiers hors peer.key SHA égaux ; VPS conserve son volume post-tentative audité. Aucun retour aux anciens engagements, aucun journal réinitialisé. Contrôleur accepte un bundle de préparation engagé dans le dépôt avec hashes d’input/plan, garde les anciens conteneurs par noms uniques, vérifie l’arrêt natif externe avant la prochaine reprise. Preuves restored-three-node-v2-prepare-20261005/. Cette étape prépare la relance corrigée ; duties/finalité réseau et production restent à démontrer.


2026-10-05, reprise trois nœuds v2 réellement lancée après réaudit : FAILED par contrôleur c76c4dc5, interrogation RPC Docker Mac avant fin de son startup (curl exit 7), alors que processus/P2P se sont initialisés ensuite. Ce refus de connexion était traité à tort comme résultat terminal réseau. Arrêts externes conservés : natif SIGTERM sans reçu complet, Docker Mac exit 137 sans OOM après timeout d’arrêt pendant startup, VPS exit 0 avec un reçu ; santé/signatures des deux Mac non inférées. Copies et journaux post-tentative conservés, nouveaux audits quiescents requis avant retry ; pas de reset ni relance immédiate. Correctif du contrôleur : attendre trois processus vivants et trois vrais événements IPC/P2P ready avant première lecture RPC, deadline fixe 180 s sans remise à zéro ; quatre contrôles délai/terminal/alias PASSED. Preuves restored-three-node-v2-attempt-20261005/. Aucun avancement de finalité commune démontré, production reste ouverte.


2026-10-05, reprise après arrêt v2 non sain : audits readonly séparés des deux Mac (32 journaux chacun) et VPS (33) terminaux 0, témoins et ancêtre finalisé externe 75 vérifiés. Source native reste core 4382 ; Docker Mac core 4400, 379 blocs canoniques et une nouvelle tête signée, huit journaux ont avancé malgré l’arrêt 137 ; les 16 clés du groupe natif sont exactement identiques dans les deux copies. Source Docker Mac post-crash sélectionnée pour les deux nouveaux Mac, 968 fichiers hors peer.key SHA identiques dans chaque copie, engagements de tous les groupes conservés sans reset ; VPS conserve core 4112/volume post-tentative audité. Contrôleur accepte une source arrêtée 137 uniquement comme crash explicitement audité, ne transforme pas sa santé en réussite. Deux refus initiaux d’audit par chemin profile rebased deux fois conservés, corrigés avant lectures de base. Preuves restored-three-node-v3-prepare-20261005/. Préparation de reprise corrigée, aucune finalité commune nouvelle ni production qualifiée.


2026-10-05, reprise restaurée trois nœuds v3 TERMINAL PASSED : contrôleur 57761d82, cœur 2795d4b5 + relais 8adbabff, trois pairs distincts sur deux machines physiques (Mac natif, Docker ARM64 Mac, Docker AMD64 VPS), transport cpp-libp2p privé via SSH, 16/16/33 signataires sans chevauchement. Finalité commune passée de l’époque 75 à 1284/racine ccf88d18, observée sur trois lectures concordantes : un seul avancement distinct, pas trois époques communes successives. Chaque groupe a proposé et attesté ; 65 journaux actifs ont avancé en séquence et digest, trois reçus externes conservés, natif exit 0 et deux Docker exit 0 sans OOM. Six programmes et contrôleurs rehashés identiques après arrêt ; aucune image reconstruite. 45 propositions/354 attestations/1562 sync ; maxPoll VPS 5699 ms pour slot 6 s, périmètre borné sans qualification de charge soutenue. Preuves [restored-three-node-v3-terminal-20261005](qualification/restored-three-node-v3-terminal-20261005/README.md). Audit readonly post-arrêt des bases/témoins à effectuer ; portabilité physique ARM64→AMD64 en attente d’autorisation dédiée. Partitions prolongées/asymétriques, rotations, cycle/sanctions, anomalie WAN A, ressources, exigences opérationnelles et revue indépendante restent ouverts. Profil EVM activé non élargi par cette réussite réseau ; aucun lancement/vente/bridge économique qualifié. Les deux tentatives précédentes restent FAILED. Production non démontrée.


2026-10-05, audits quiescents après reprise v3 : trois captures readonly TERMINAL 0. Mac natif et données Docker Mac examinés avec outil natif actuel 2795d4b5 : chacun 414 blocs canoniques/2 transactions, même tête 64cfb7a2 et finalité externe 1284/ccf88d18 ; checkpoints exacts 4837/917c2bac et 4860/ba23b426 concordants avec reçus d’arrêt. Les 32 journaux/témoins de chaque copie (16 actifs et 16 conservés) sont vérifiés, engagements actifs exactement égaux aux reçus, 1002 et 981 fichiers respectivement SHA inchangés. VPS audité dans Docker AMD64, volume/config/build readonly et réseau désactivé : checkpoint exact 4541/9e3ad987, 33 journaux/témoins et ancêtre finalisé externe 1284 vérifiés, SHA des fichiers avant/après identiques. Aucune base Mac transférée ; seules requête/config publiques synthétiques envoyées. Preuves post-stop-mac/ et post-stop-vps/ dans restored-three-node-v3-terminal-20261005. Ceci ferme l’audit des engagements de cette reprise précise ; fork-choice historique et votes standalone non rejoués, portabilité physique entre architectures et tous autres critères globaux restent ouverts.


2026-10-05, contrôleur de finalité renforcé et préparation v4 : trois checkpoints communs distincts désormais requis, puis trois lectures stables du dernier ; saut de nombreuses époques et répétitions comptent pour un avancement unique, désaccord/pair absent/alias remettent la stabilité à zéro, régression/conflit refusés. Plan de reprise accepte deux captures Mac indépendantes et conserve leurs checkpoints/journaux propres après signatures séparées ; aucun reset/merge. 18 contrôles ciblés PASSED (dont nouveaux gardes), noyau C++ inchangé. Copies v4 Mac séparées SHA égales à leurs sources hors peer.key, VPS réutilise son volume audité arrêté. Preuves restored-three-node-v4-prepare-20261005/. Campagne réelle v4 à lancer ; ceci ne qualifie ni trois avances réelles ni partitions/reconnexion/production. Les anciens PASSED/FAILED conservent leur périmètre initial.


2026-10-05, campagne restaurée v4 f0c4cb03 TERMINAL FAILED : fenêtre fixe 900 s, zéro avancement commun distinct après époque 1284 malgré pairs présents/têtes convergentes. Natif/deux Docker arrêtés exit 0, sans OOM ; trois reçus externes conservés. VPS signataire unhealthy « deposit history contract view failed », seulement 2 propositions/22 attestations ; deux Mac sains avec 304 attestations chacun. Audits post-arrêt des deux Mac readonly terminaux 0, engagements exacts/journaux/témoins/canonique vérifiés et fichiers SHA inchangés ; état canonique époque 1321, flags timely-target précédents 16/16/0 par groupe. Audit readonly post-arrêt VPS encore requis. Preuves restored-three-node-v4-failure-20261005/. Échec conservé, aucune relance avec anciens pins, aucune production qualifiée.

2026-10-05, cause reproduite et correctif local : expiration typée ExpiredCapellaDuty pendant les vues EVM du contrat de dépôt absorbée par la gestion d’erreur EVM, puis transformée en échec contractuel et arrêt du validateur. Test nouveau contre cœur ancien exit 1 « deposit view swallowed typed duty deadline » ; même test contre seule unité consensus_deposit_history.cpp recompilée exit 0, deux interruptions dans count/root, exception exacte et retry sans mutation. Le wrapper de checkpoint préserve maintenant l’exception originale via ExecutionCancelled ; les vrais échecs contractuels restent refusés. Preuves deposit-view-deadline-20261005/. Diagnostic optionnel des participations readonly qualifié sur référence arrêtée, anciennes sorties d’audit identiques ; aucun vote historique rejoué ni santé inférée. Correctif Docker ARM64/AMD64, effet scheduler/réseau et tous critères globaux restent ouverts ; cœur parent et daemons de la campagne non modifiés.


2026-10-05, correctif b5ee00ac qualifié localement sur natif/ARM64/AMD64 : Docker existants terminaux 0 sans OOM, sources Git mêmes cinq SHA, seule unité cœur consensus_deposit_history.cpp recompilée dans une copie du parent 2795d4b5, main/test/capture frais, programmes parents préservés. Régression ancienne bibliothèque exit 1 reproduite puis corrigée exit 0 dans chaque Docker ; configured-runtime 16 blocs/64 clés et scheduler 40 blocs/272 votes/1280 sync/finalité 3 PASSED. Candidate native actualisée mêmes modes PASSED, programmes identifiés. Preuves deposit-view-deadline-20261005/arm64, amd64 et native-candidate. Aucun test réseau ni rotation/cycle complet déduit ; effet du correctif sur VPS sous duties encore à démontrer.

2026-10-05, audit readonly VPS après échec v4 TERMINAL 0 : lecteur AMD64 existant cœur 2795d4b5, volume/config/build readonly, réseau désactivé, aucune signature. 486 blocs/2 transactions, 33 journaux/témoins, checkpoint exact 5549/6ac92944 égal au reçu externe, finalité 1284 et ancêtre externe vérifiés ; 1029 fichiers SHA inchangés avant/après. Preuves restored-three-node-v4-failure-20261005/post-stop-vps/. Aucune base Mac transférée ni santé runtime inférée ; v4 reste FAILED. Tous engagements post-v4 disponibles pour reprise épinglée, aucun reset. Portabilité physique ARM64→AMD64 attend toujours l’accord dédié et critères globaux/revue indépendante demeurent ouverts.


2026-10-05, contrôleur de finalité renforcé et préparation v5 : trois checkpoints communs distincts désormais requis, puis trois lectures stables du dernier ; saut de nombreuses époques et répétitions comptent pour un avancement unique, désaccord/pair absent/alias remettent la stabilité à zéro, régression/conflit refusés. Plan de reprise accepte deux captures Mac indépendantes et conserve leurs checkpoints/journaux propres après signatures séparées ; aucun reset/merge. Qualification existante des 18 contrôles ciblés réutilisée pour le contrôleur JavaScript inchangé depuis f0c4cb03 ; candidate C++ b5ee00ac corrigée et qualifiée localement comme ci-dessus. Copies v5 Mac séparées SHA égales à leurs sources hors peer.key, VPS réutilise son volume audité arrêté. Preuves restored-three-node-v5-prepare-20261005/. Campagne réelle v5 à lancer ; ceci ne qualifie ni trois avances réelles ni partitions/reconnexion/production. Les anciens PASSED/FAILED conservent leur périmètre initial.

## Reprise trois nœuds v5 — 6 octobre 2026

Campagne b8567771/C++ b5ee00ac : sortie terminale 1 sur HTTP 503 Beacon du
VPS, zéro avancée de finalité commune. Corps de refus non conservé par
l'ancien contrôleur : cause précise inconnue. Boucles de validation saines
à l'arrêt, avec propositions/attestations sur chaque nœud ; cela ne qualifie
pas la finalité ni la production. Délais VPS observés > un slot restent ouverts.
Contrôleur corrigé pour conserver le diagnostic et accepter uniquement les
refus temporaires connus de budget de lecture, sans rallonger l'échéance
ni compter de lecture incomplète. Pas de modification du profil EVM/RPC activé.
Tous les échecs et critères précédents restent ouverts dans leur périmètre.
[Preuves v5](qualification/restored-three-node-v5-failure-20261006/README.md).

Audits post-v5 terminés en lecture seule sur les trois magasins : engagements
core et 16/16/33 signataires exacts, fichiers inchangés, ancêtre finalisé 1284
vérifié séparément ; têtes canoniques divergentes. Aucun journal fusionné ou
réinitialisé. Cinq nouveaux tests du contrôleur passent, dont la reprise
d'ascendance après refus partiel. Cela prépare la prochaine campagne ; aucune
finalité commune nouvelle ni disponibilité soutenue n'est qualifiée.


2026-10-06, préparation v6 : engagements post-v5 vérifiés séparément et copies Mac SHA identiques hors identité P2P. Volume VPS arrêté réutilisé après audit indépendant. Journaux non fusionnés ni réinitialisés. Contrôleur 31a66f24 : diagnostics HTTP conservés, refus temporaires connus sans extension de délai et ascendance complète malgré cache partiel (cinq nouveaux tests passent). Binaires C++ et images inchangés depuis v5, noyau b5ee00ac et relais 8adbabff. Cette préparation ne qualifie aucune finalité réelle ni production. Preuves restored-three-node-v6-prepare-20261006/.

## Rattrapage P2P avec profondeur bornée — 6 octobre 2026

Blocage local reproduit dans Docker : tête dépassant le budget de dépendances,
aucun envoi ni catalogue ouvert sur un nouveau pair. Correctif C++ intégré :
seule la limite locale de profondeur autorise le repli vers le catalogue, puis
le balayage de racines suivantes, sans faux ACK ni effacement du refus. Limites
et échéance conservées. 90 scénarios existants et deux nouveaux passent en
transport modélisé ARM64 Docker. AMD64, exécutables P2P et réseau réel de ce
correctif restent à qualifier. v6 continue sur ses binaires d'origine ; aucune
causalité exclusive ni production déduite. Profil EVM inchangé.
[Preuves](qualification/capella-depth-catchup-20261006/README.md).

## Reprise v6 : échéance complète non satisfaite — 6 octobre 2026

Sortie terminale 1 après 900 s, zéro avancée commune au-delà de 1284. Trois
processus arrêtés proprement sans OOM, boucles saines et duties effectifs.
VPS : quinze polls > six secondes, maximum 6697 ms. Aucun HTTP 503 observé
sur cette campagne ; la cause v5 reste inconnue. Audits séparés post-arrêt
réussis, engagements exacts et fichiers inchangés (1260 VPS), têtes divergentes
et ancêtre finalisé vérifié. Journaux non fusionnés/non réinitialisés.
Correctif de profondeur 81e5ab31 absent des binaires v6 : qualification réseau
du correctif encore ouverte, compilations déclenchées seulement après les arrêts.
[Preuves v6](qualification/restored-three-node-v6-terminal-20261006/README.md).

2026-10-06 : relais P2P 81e5ab31 compilé/lié sur Mac natif et Docker ARM64/AMD64,
parents conservés et sources hash vérifiées. AMD64 : 90 scénarios existants +
deux nouveaux passent ; ARM64 : deux nouveaux passent et qualification 90
antérieure réutilisée par égalité des sources. Contrôleurs Docker initiaux
exit 1 conservés : hypothèse incorrecte sur --help, post-vérification négative
CLI attendue et contrôles des compilations/tests exit 0 sans recompilation.
Aucune qualification réseau réelle du nouveau relais ; prochaine reprise avec
les engagements audités post-v6. [Preuves](qualification/depth-p2p-candidate-20261006/README.md).


2026-10-06, préparation v7 : copies Mac séparées et engagements post-v6 audités, volume VPS arrêté réutilisé sans transfert. Relais corrigé 81e5ab31 remplace les trois exécutables P2P, noyau b5ee00ac inchangé ; images réutilisées. Traces P2P existantes activées dans la fixture pour diagnostiquer les refus/ACK/yields, règles et échéance 900 s inchangées. Trois avances communes distinctes restent nécessaires. Préparation non qualifiée réseau/production ; v6 conservé. Preuves restored-three-node-v7-prepare-20261006/.

## Reprise v7 : résultat original — 6 octobre 2026

Résultat FAILED, sortie 1, sans redémarrage du contrôleur. Core b5ee00ac et relais 81e5ab31. Critère de trois avancées distinctes conservé.
mac-native : boucle saine oui, aucune erreur, propositions 33, attestations 296.
mac-docker : boucle saine oui, aucune erreur, propositions 29, attestations 302.
vps-docker : boucle saine non, erreur invalid Capella Store snapshot at line 39, propositions 28, attestations 161.
Engagements avancés : 16/16 natif, 16/16 Docker Mac, 32/33 VPS ; clé en enregistrement automatique inchangée, donc reprise de 65 duties non démontrée.
Audits séparés post-arrêt : engagements exacts de chaque propriétaire, fichiers inchangés. Journaux non fusionnés/non réinitialisés, aucun transfert de base entre hôtes. Reprise réseau et production toujours ouvertes.
[Preuves v7](qualification/restored-three-node-v7-terminal-20261006/README.md).

## Diagnostic de capacité du snapshot v7 — 6 octobre 2026

Lecture Docker AMD64 du VPS arrêté : 256 blocs retenus pour une limite
configurée de 256, 65 messages sur 2048, zéro équivoque sur 1000. Engagement
exact du reçu d’arrêt et témoin vérifiés ; 1393 fichiers inchangés. Cela pointe
vers l’épuisement de rétention pour l’erreur ligne 39 ; le candidat rejeté
en mémoire n’a pas été capturé, sa structure complète reste à reproduire.
Outil diagnostique seul compilé et lié au cœur b5ee00ac inchangé ; premier
échec d’include conservé, copie corrigée sortie 0 sans OOM. Aucune relance
ni signature, aucun rebuild d’image. Gestion de capacité, propagation des votes
et finalité commune restent ouvertes ; augmenter la borne ne les qualifie pas.
[Preuves](qualification/restored-v7-snapshot-capacity-20261006/README.md).

## Reprise bornée des exports P2P — 6 octobre 2026

Cache local authentifié par pair/session implémenté : conserve la préparation
entre échéances, sans transformer un export en ACK. Échec ancien reproduit
(sortie 1), correctif sortie 0 ; 106 cas de transport concernés passent en
Docker ARM64 et AMD64. Relais complets compilés/liés sur les trois plateformes,
ancien cœur b5ee00ac et binaires conservés. Flags/liens CMake réutilisés ;
configuration fraîche non qualifiée. Aucun rebuild d’image, aucune relance
réseau. v7 reste FAILED, capacité du Store et finalité commune toujours ouvertes.
[Preuves et limites](qualification/capella-export-resume-20261006/README.md).

## Refus de capacité avant signature — 6 octobre 2026

Garde de capacité de blocs implémentée dans la proposition et l’admission :
refus avant fournisseur/réservation/publication quand le graphe est plein,
sans augmenter sa limite. Régression avant/après Docker ARM64 et AMD64 :
ancien cœur signe (échec 1 conservé), correctif zéro appel fournisseur, journal
intact, refus sans écriture, horloge active et restauration avec témoin (sortie 0).
Runtime 16 blocs/64 clés et scheduler 40 blocs/272 votes/1280 sync passent
sur les deux architectures. Échec initial de montage des fixtures ARM64 conservé.
Qualification locale seulement ; nouveau cœur natif/runtime non livré, course
signature/admission, autres budgets, libération de capacité et reprise de finalité
restent ouverts. Aucun rebuild d’image ni relance WAN.
[Preuves et périmètre](qualification/capella-block-capacity-pressure-20261006/README.md).

## Signature et admission atomiques du scheduler — 6 octobre 2026

Chemin atomique implémenté : verrou du propriétaire conservé entre signature
et admission durable, sans court-circuiter la validation ni supprimer de journal.
Reproduction du protocole séparé sortie 1 conservée ; admission concurrente
contrôlée, doublon et restauration réussis sur Mac natif et Docker ARM64/AMD64.
Refus à pleine capacité avant fournisseur confirmé sur les trois plateformes.
Scheduler et gardes d’inscription passent sur les deux Docker (40 blocs,
272 votes, 1280 sync). Nouveaux runtime/audits liés et identifiés, pas déployés.
La course est qualifiée dans ce périmètre ; libération de capacité, autres budgets,
I/O à ce point, contention et finalité WAN restent ouverts. Aucun rebuild d’image ;
CMake frais et build intégral reproductible non qualifiés. v7 reste FAILED.
[Preuves et limites](qualification/capella-atomic-proposal-admission-20261006/README.md).

## Éligibilité d’élagage du VPS v7 arrêté — 6 octobre 2026

Diagnostic structural engagé : ancre déjà égale à la finalité epoch 1284,
256/256 blocs tous descendants, zéro bloc supprimable par la frontière actuelle.
SSZ 7 956 128/67 108 864 octets, snapshot 21 914 821/134 217 728 octets ;
la borne en nombre de blocs est atteinte avant ces deux bornes d’octets.
1393 fichiers inchangés ; lecture seule sans suppression, signer ni reprise.
Deux échecs de build diagnostique conservés puis version corrigée sortie 0.
Ce résultat interdit de qualifier un simple prune comme récupération : rétention
sous pression et nouvelle finalité doivent encore être démontrées. Pas d’estimation
de RAM ni de qualification d’un profil agrandi. v7 et production restent ouverts.
[Preuves et limites](qualification/restored-v7-pruning-eligibility-20261006/README.md).


2026-10-06, préparation v8 de rétention explicitement expérimentale : sources post-v7 auditées, engagements exacts, journaux séparés sans remise à zéro. Candidate cœur 05649987 / relais 5902c983 liée sur les trois environnements, images réutilisées. Capacité locale 256→1024 blocs, autres limites en octets et règles inchangées ; ce changement ne clôture ni saturation, ni pruning, ni mémoire, ni production. Échéance originale 900 s et trois avances communes distinctes nécessaires. Préparation seule sans reprise qualifiée. Preuves restored-three-node-v8-retention-prepare-20261006/.

## Reprise v8 expérimentale : résultat original — 6 octobre 2026

Résultat FAILED, sortie 1, sans redémarrage du contrôleur. Core 05649987 et relais 5902c983. Critère de trois avancées distinctes conservé.
mac-native : boucle saine True, erreur None, propositions 37, attestations 297.
mac-docker : boucle saine True, erreur None, propositions 35, attestations 298.
vps-docker : boucle saine True, erreur None, propositions 74, attestations 376.
Audits séparés post-arrêt : engagements exacts de chaque propriétaire, fichiers inchangés. Journaux non fusionnés/non réinitialisés, aucun transfert de base entre hôtes. Reprise réseau et production toujours ouvertes.
[Preuves v8](qualification/restored-three-node-v8-retention-terminal-20261006/README.md).


2026-10-06 — Dispatcher : gel après trois réponses RPC perdues reproduit en HTTP local avec délai réel 5 s et registre réel de tickets. Prototype isolé : 8 scénarios simulés, 5 avec registre réel, 10 contrôles existants et un cas HTTP passent ; anciennes versions échouent sur le gel. Aucun ticket actif retiré sans preuve, annulation puis Forget confirmé, décisions contradictoires restent fermées. Prototype non intégré/non déployé ; cause WAN, régression Docker, liaison P2P et nouvelle finalité commune restent à qualifier. Preuves dispatch-transport-recovery-local-20261006/.


2026-10-06 — Dispatcher intégré : reprise de transport dans la même session après réponses RPC perdues, annulation/réconciliation sans libérer un worker incertain ; contradictions toujours bloquantes. Qualification Docker ARM64 et AMD64 : 10 cas transport, 5 registre réel, 10 régressions et 1 HTTP réel, tous terminaux 0. P2P complet lié sur Mac natif et les deux Docker. Core 05649987 inchangé. Traces de transitions sans secrets. Génération CMake ARM64/enregistrement vérifiés, exécutions via commandes sauvegardées. Reprise avec nouvelle session, cause WAN v8 et finalité commune restent ouvertes ; aucune production déclarée. [Preuves](qualification/dispatch-transport-recovery-integrated-20261006/README.md).


2026-10-06 — Préparation v9 : exacts engagements post-v8, core 05649987 inchangé et relais 76bb2468 qualifié. Sources auditées et copiées localement par propriétaire, volume VPS conservé ; aucun journal réinitialisé/fusionné. Limites réellement utilisées par v8 (1024 blocs) conservées sans hausse. Trois avancées communes distinctes en 900 s restent requises. Préparation seule, finalité et production ouvertes. Preuves restored-three-node-v9-dispatch-prepare-20261006/.


2026-10-06 — Reprise v9 FAILED (session originale 37036, sortie 1), timeout HTTP puis nettoyage SSH IPv6 « No route to host ». Core 05649987 et relais 76bb2468 ; aucune avancée commune depuis 1284. VPS original resté actif, arrêté séparément par IPv4, sortie 0/sans OOM ; résultat original conservé avec deux reçus, troisième arrêt complémentaire documenté. Boucles saines et 65 engagements propriétaires avancés, audits read-only séparés exacts et fichiers inchangés. Traces : 268 envois Mac→VPS acceptés, aucun VPS→Mac observé, aucune transition du dispatcher ; cause WAN/asymétrie et contention/persistance à diagnostiquer. Aucune production déclarée. [Preuves v9](qualification/restored-three-node-v9-dispatch-terminal-20261006/README.md).


2026-10-06 — Export des blocs retenus : blocage derrière l’admission reproduit sur ancien core 05649987. Correctif : vue immuable publiée unique, archive/signature et checkpoint toujours vérifiés. Mac natif et Docker ARM64/AMD64 : tests de concurrence terminaux 0 ; messages signés et export historique Docker qualifiés. Premier export historique arrêté pour fixture de dépôt omise (sortie 1 conservée), ajout du fichier exact puis scénario seul réussi sans recompiler. Binaires nano/capture liés sur les trois environnements, images/archives parentes conservées. Cause WAN v9, reprise avec finalité commune, saturation/pruning et production restent ouverts. [Preuves](qualification/retained-export-contention-20261006/README.md).


2026-10-06 — Préparation v10 : engagements exacts post-v9 (arrêt VPS complémentaire documenté), core bc6d1794 et relais 76bb2468. Copies Mac locales et volume VPS conservés ; aucun journal réinitialisé/fusionné. Export retenu qualifié sans verrou d’admission ; profil/limites et critère de trois avancées communes en 900 s inchangés. SSH explicitement IPv4. Préparation seule, finalité et production ouvertes. Preuves restored-three-node-v10-export-prepare-20261006/.


2026-10-06 — Reprise v10 FAILED : session originale 76765/sortie 1, core bc6d1794, relais 76bb2468 ; délai initial 900 s et trois avancées requises conservés, aucune nouvelle finalité commune depuis 1284. Trois arrêts originaux propres/sans OOM et engagements audités séparément en lecture seule, fichiers inchangés. 221 blocs Mac→VPS et 96 VPS→Mac acceptés ; refus d’attestations et saturation/admissions expirées encore observés. Échantillon : cœur C++ écrit 54 Mo/2 s, sans preuve causale. Prochaines causes à isoler : coût de publication et alternance des préparations de branches. Premier refus préflight de hash avant tout démarrage conservé. Production ouverte. [Preuves v10](qualification/restored-three-node-v10-export-terminal-20261006/README.md).


2026-10-06 — Alternance de blocs frères : défaut de continuation reproduit sur ancien core bc6d1794, sortie 1 sur Mac natif/Docker ARM64/AMD64. Cache de cibles authentifiées partagé et borné : bloc honnête admis en 43 paires (borne 97), oracle pleine durée identique, signature invalide rejetée, duplicata stable, témoin réouvert. Préparation 146 contrôles et wire terminaux 0 ; runtimes nano/capture liés, images réutilisées. Premier montage AMD64 incorrect/sortie 1 conservé. Ni cause v10 ni finalité réseau démontrée ; parents distincts, pression/coût du cache, saturation/pruning et production restent ouverts. [Preuves](qualification/incoming-target-alternation-20261006/README.md).


2026-10-06 — Préparation v11 : engagements exacts post-v10, core 504c9df8 et relais 76bb2468. Fichiers audités inchangés, copies Mac locales et volume VPS conservés ; aucun journal réinitialisé/fusionné. Défaut de continuation de cibles qualifié sur les trois environnements ; profil/limites et critère de trois avancées communes en 900 s inchangés. Préparation seule ; finalité, cause v10 et production ouvertes. Preuves restored-three-node-v11-target-prepare-20261006/.


2026-10-06 — Reprise v11 FAILED : session originale 23882/sortie 1, checkout c193eb4a, core 504c9df8/relais 76bb2468 ; aucune nouvelle finalité commune depuis 1284 ; critère initial de trois avancées en 900 s conservé. Trois arrêts propres et engagements post-v11 audités séparément en lecture seule, fichiers inchangés. Traces/compteurs bruts conservés ; aucun succès réseau extrapolé. Diagnostic natif concurrent documenté ; pas un benchmark CPU isolé. Prototype de cache de votes distinct, non déployé. Cause WAN, saturation/pruning, finalité et production restent ouverts. [Preuves v11](qualification/restored-three-node-v11-target-terminal-20261006/README.md).


2026-10-06 — Contre-exemple natif supplémentaire : deux attestations individuellement valides sur des racines distinctes à la même époque effacent leur préparation dans le core 504c9df8 ; fixture finale exacte, baseline sortie 1. Prototype hors dépôt/runtime : cache primaire par époque conservé, cibles secondaires dans le cache partagé borné, 43 paires pour admettre les deux votes, oracle complet identique, signature invalide rejetée, duplicata stable et témoin rouvert. Première construction de fixture et erreur include conservées distinctement. Prototype non intégré, non déployé, Docker non qualifié ; cause v11 non démontrée et production ouverte. [Preuves](qualification/vote-target-alternation-prototype-20261006/README.md).


2026-10-06 — Cache de cibles de vote intégré : défaut d'alternance de deux votes valides reproduit sur ancien core 504c9df8, sortie 1 avec le même objet test. Entrée primaire conservée et cibles secondaires dans le cache partagé borné, validation maintenue. Mac natif/Docker ARM64/AMD64 : deux votes admis en 43 paires, oracle combiné identique, signature invalide rejetée, duplicata stable, témoin rouvert ; blocs alternants, 146 contrôles et wire terminaux 0. Nano/capture liés, images réutilisées. Pas de finalité réseau ni de cause v11 démontrée ; coût/pression des caches, parents distincts, pruning et production restent ouverts. Reprise uniquement depuis les engagements post-v11. [Preuves](qualification/vote-target-cache-integrated-20261006/README.md).


2026-10-06 — Préparation v12 : engagements post-v11 exacts, core f077205f et relais 76bb2468. Empreintes propriétaires inchangées, copies Mac locales et volume VPS conservés ; journaux non réinitialisés/fusionnés. Cache des cibles de vote qualifié sur les trois environnements ; profil/limites et critère de trois avancées en 900 s inchangés. Préparation seule ; cause v11, finalité et production ouvertes. Preuves restored-three-node-v12-vote-cache-prepare-20261006/.


2026-10-06 — Reprise v12 FAILED : contrôleur 70646 / sortie 1, checkout 4612ddf8, core f077205f / relais 76bb2468. Aucun avancement commun de finalité depuis 1284 dans le délai original de 900 s. Trois propriétaires arrêtés ; audits read-only séparés et engagements exacts conservés. Diagnostic hors ligne borné v11/v12 Mac : 32 votes récents par campagne référencent des blocs absents du destinataire, avant vérification BLS ; cause complète du réseau non prouvée. Priorité au rattrapage des branches/dépendances, sans assouplissement cryptographique. Pas de compilation diagnostique concurrente v12. Tous les critères et échecs historiques restent ouverts/conservés. [Preuves v12](qualification/restored-three-node-v12-vote-cache-terminal-20261006/README.md).


2026-10-06 — Oracle complet sur frontières v12 : les trois blocs manquants à parent connu, après 177/178/179 slots, sont admissibles et reproduisent les en-têtes/hashes d’exécution de la source sur copies en mémoire ; bases Mac arrêtées inchangées. Borne diagnostique 256, limites runtime inchangées. Écart restant : démontrer puis corriger la continuité du rattrapage (parents/cibles, scheduling, délais), sans assimiler cet oracle à une réussite réseau. [Preuves](qualification/v12-frontier-full-oracle-20261006/README.md).


2026-10-06 — Continuations par parent/slot implémentées et qualifiées sur Mac natif, Docker ARM64 et AMD64. Contre-exemple conservé : bloc valide bloqué sous interférence dans l’ancien cœur ; correction conforme à l’oracle, signature invalide rejetée, duplicata stable et témoin réouvert. Scans/reconstructions des consommateurs du type modifié, exécutables cohérents et régressions pertinentes archivés. Erreur initiale native secp256k1 conservée, chemin réparé. Caches bornés en nombre ; pression/mémoire/équité, finalité réseau et tous les autres critères de production restent ouverts. [Preuves](qualification/parent-slot-cache-integrated-20261006/README.md).


2026-10-06 — Préparation v13 : engagements post-v12 exacts, continuation par parent/slot et relais 76bb2468. Copies locales séparées et volume VPS conservé ; journaux non réinitialisés/fusionnés. Même profil, même limite et même critère de finalité en 900 s. Préparation seule, pression/éviction/coût des caches, cause globale WAN, finalité et production ouverts. Preuves restored-three-node-v13-parent-cache-prepare-20261006/.


2026-10-06 — Reprise v13 FAILED : contrôleur 80316 / sortie 1, checkout 24ea13b0, cœur 5787915e et relais 76bb2468 ; 0 avancées communes observées selon le critère original. Trois propriétaires arrêtés et audits read-only des engagements, journaux/témoins et historique canonique ; fichiers inchangés. Pas de compilation concurrente, ni transfert de base inter-hôtes. Périmètre privé exact ; production et revue indépendante restent ouvertes. [Preuves v13](qualification/restored-three-node-v13-parent-cache-terminal-20261006/README.md).


2026-10-06 — Diagnostic Docker ARM sur stockages post-v13 arrêtés : cœur candidat 5787915e, lecteur compilé avec le type actuel, sortie 0 ; tous les fichiers inchangés. Graphes 453/356 sous la limite 1024 ; mêmes trois frontières absentes à parent connu qu’après v12 (10751/10753/10752 depuis 10574). 32 votes récents référencent des blocs absents, avant BLS. Priorité au diagnostic sélection/reprise du relais et admission des frontières sur archives réelles ; cause complète non prouvée, v13 FAILED et production ouverte. [Preuves](qualification/v13-stopped-frontiers-20261006/README.md).


2026-10-06 — Rejeu Docker ARM du relais sur archives post-v13 : catalogue/export et admission C++ réels, transport modélisé, copies chain locales sans clés/journaux, originaux inchangés. Borne 32 : frontière 10753 retentée 21 fois, aucune nouvelle publication. Borne distincte 128 : 10753 admise à sa 23e tentative, 81 publications et 172 racines ACK ; 10751 jamais envoyée, 77 cycles head-admission-unconfirmed arrêtent avant le catalogue. Pas de preuve de blocage infini ni de cause WAN complète. Priorité : dépendances de la tête refusée et accès borné au catalogue sans faux ACK. Échec initial de compilation du diagnostic conservé ; sorties finales 0. Production et revue indépendante ouvertes. [Preuves](qualification/v13-real-archive-relay-replay-20261006/README.md).


2026-10-06 — Repli tête/catalogue corrigé et qualifié dans son périmètre : mêmes six unités sur Mac natif et Docker ARM/AMD, nouveaux binaires P2P identifiés. Catalogue borné accessible après tête non confirmée ; refus de bloc ne masque plus les entrées indépendantes, sans faux ACK ni succès de tête absente. Rejeu sur mêmes archives/cœur/limites : deux frontières admises en 26 cycles et 117 publications, contre une frontière jamais envoyée après 128 cycles avant correction. Originaux inchangés ; transport modélisé, résultat partiel reste FAILED sur autres refus. Finalité WAN/production non démontrées ; prochaine reprise sur engagements/journaux post-v13 exacts. Erreur initiale de collecte conservée et réparée sans relancer les builds/tests. [Preuves](qualification/head-catalog-fallback-integrated-20261006/README.md).


2026-10-06 — Préparation v14 : engagements post-v13 exacts, cœur 5787915e et relais 7248698d avec parcours historique après refus de tête. Copies locales séparées et volume VPS conservé ; journaux non réinitialisés/fusionnés. Même profil, même limite et même critère de finalité en 900 s. Préparation seule, pression/éviction/coût des caches, cause globale WAN, finalité et production ouverts. Preuves restored-three-node-v14-head-fallback-prepare-20261006/.


2026-10-06 — Reprise v14 FAILED : contrôleur 40209 / sortie 1, checkout ec3b6441, cœur 5787915e et relais 7248698d ; 0 avancées communes observées selon le critère original. Frontières manquantes inchangées, graphes sous capacité ; diagnostic avec lecteur existant réutilisé. Trois propriétaires arrêtés et audits read-only des engagements, journaux/témoins et historique canonique ; fichiers inchangés. Pas de compilation concurrente, ni transfert de base inter-hôtes. Périmètre privé exact ; production et revue indépendante restent ouvertes. [Preuves v14](qualification/restored-three-node-v14-head-fallback-terminal-20261006/README.md).


2026-10-06 — Alternance tête/historique qualifiée dans son périmètre : contre-exemple ancien relais conservé (tête non confirmée bloque deux tours historiques), correction et mêmes 19/10/5 scénarios sur Mac natif et Docker ARM/AMD. ACK de tête réel obligatoire, statut absent/tardif et erreurs restent des échecs, quota/annulation partagés ; catégories intercalées et état par session. Trois cibles CMake compilées et CTest ARM 3/3. Nouveaux binaires P2P identifiés, cœur 5787915e inchangé, images/volumes réutilisés. Aucun résultat WAN/finalité nouvelle revendiqué : prochaine reprise post-v14 selon le critère original. Échecs historiques, autres écarts et revue indépendante restent ouverts. [Preuves](qualification/head-history-fairness-integrated-20261006/README.md).


2026-10-06 — Préparation v15 : engagements post-v14 exacts, cœur 5787915e et relais 1427e624 avec alternance tête/historique. Copies locales séparées et volume VPS conservé ; journaux non réinitialisés/fusionnés. Même profil, même limite et même critère de trois finalités nouvelles en 900 s. Préparation seule, finalité/WAN et autres critères de production restent ouverts. Preuves restored-three-node-v15-history-first-prepare-20261006/.


2026-10-06 — Reprise v15 FAILED : contrôleur 29884 / sortie 1, checkout 7c6c1fd3, cœur 5787915e et relais 1427e624 ; 0 avancées communes observées selon le critère original. Frontières et classifications de votes enregistrées ; diagnostic avec lecteur existant réutilisé, sans présumer de leur stabilité ni de la cause WAN. Trois propriétaires arrêtés et audits read-only des engagements, journaux/témoins et historique canonique ; fichiers inchangés. Pas de compilation concurrente, ni transfert de base inter-hôtes. Périmètre privé exact ; production et revue indépendante restent ouvertes. [Preuves v15](qualification/restored-three-node-v15-history-first-terminal-20261006/README.md).


2026-10-06 — Diagnostics de catalogue corrigés : premier refus conservé séparément de la terminaison, parcours complet distinct de libération, compteurs préservés sur délai/annulation du wrapper. 7 nouveaux scénarios et régression pertinente sur Mac natif/ARM64/AMD64 ; CMake SDK/CTest 4/4 sur les deux architectures et relais reconstruits. Échecs initiaux conservés. Core inchangé ; aucune convergence WAN ni production déduite. [Preuves](qualification/catalog-termination-diagnostic-20261006/README.md).


2026-10-06 — Nouveau contre-exemple réel d’admission (0a605778/core 5787915e) : ticket completed/rejected pour le bloc retenu 10751, pourtant admis après 21 continuations explicites supplémentaires. Préparation différée confondue avec rejet dans le vrai worker de reçus ; correction pending/cancellation/budgets/fairness requise. Magasins originaux inchangés, copie locale de chain seulement ; cause WAN globale non prouvée. [Preuves](qualification/block-receipt-deferral-baseline-20261006/README.md).


2026-10-06 — Admission différée corrigée (base bed17913) : préparation incomplète distincte du rejet ; un même travail reprend en FIFO par quanta avec annulation et réservation conservées, sans reçu/ACK prématuré. Bloc préparé signé : 146 vérifications, deux variantes et une seule soumission ; régression ciblée native/ARM64/AMD64 et quatre tests SDK sur les deux architectures. Contre-exemple retenu 10751 : premier reçu accepté en natif/ARM64, aucune resoumission externe (transport/horloge modélisés). Échecs conservés, dont AMD64 RocksDB à nofile=1024 ; reprise des seuls scénarios restants à 65536, sortie 0, sans rebuild du cœur/image. Sources 344 vérifiées ; relais compilés. Finalité commune restaurée, cause WAN globale, charge et production toujours ouvertes ; instrumentation de faute encore activée, release à qualifier séparément. Aucun transfert de base/clé inter-hôtes. [Preuves](qualification/admission-continuations-integrated-20261006/README.md).


2026-10-06 — Préparation v16 : stockages post-v15 vérifiés inchangés et engagements séparés conservés, nouveau cœur/relais 0e2d943a qualifié dans le périmètre admission continue. Copies locales Mac et volume VPS sur son hôte ; aucune base/clé exportée. Critère original de trois finalités communes nouvelles en 900 s inchangé. Préparation seule ; finalité WAN/production restent ouvertes. Preuves restored-three-node-v16-admission-prepare-20261006/.


2026-10-06 — Reprise v16 FAILED : contrôleur 54275 / sortie 1, checkout 6076684d, cœur/relais 0e2d943a ; 0 avancées communes observées selon le critère original. Traces de relais/ressources et hypothèses de parcours enregistrées ; cause WAN globale non prouvée. Trois propriétaires arrêtés et audits read-only des engagements, journaux/témoins et historique canonique ; fichiers inchangés. Pas de compilation concurrente, ni transfert de base inter-hôtes. Périmètre privé exact ; production et revue indépendante restent ouvertes. [Preuves v16](qualification/restored-three-node-v16-admission-terminal-20261006/README.md).


2026-10-06 — Catalogue : contre-exemple de préfixe répété sur base 6076684d, compile/lien 0 et sortie attendue 1. Huit tentatives revisitent trois mêmes racines ; racine valide suivante jamais atteinte après export non confirmé. Transport/core modélisés, défaut de progression confirmé, cause WAN globale non prouvée. Correction de sélection bornée distincte des ACK requise. Audit VPS v16 réalisé depuis configurations/reçu déjà locaux au serveur après refus automatique de l’ancien export ; 1926 fichiers inchangés et engagements exacts vérifiés. [Contre-exemple](qualification/catalog-prefix-starvation-baseline-20261006/README.md).


2026-10-06 — Sélection du catalogue entre tentatives : qualification native, Docker ARM64 et AMD64, sources figées et sorties terminales vérifiées. Seek borné/authentifié dans snapshot ; positions de sélection distinctes des ACK, bouclage/retry et reset session/génération. Neuf nouveaux cas SDK ; suites catalogue/tête/diagnostic/historique et quatre scénarios cœur passent. Le contre-exemple initial et les échecs de lanceurs sont conservés. Aucune image reconstruite, base/configuration/clé transférée. Preuves catalog-selection-integrated-20261006/. Finalité commune post-restauration et causalité WAN restent ouvertes ; release sans instrumentation et production non qualifiées.


2026-10-06 — Préparation v17 : engagements post-v16 et journaux conservés, catalogue à sélection équitable qualifié dans son périmètre local. Trois finalités communes nouvelles en 900 s restent à démontrer ; préparation seule. Production et revue indépendante ouvertes. Preuves restored-three-node-v17-catalog-selection-prepare-20261006/.


2026-10-06 — V17 démarrage FAILED, session 92712/sortie 1 : répertoire VPS local 0700 inaccessible à l’utilisateur du conteneur, avant ouverture de genesis/base. Aucun nœud Mac lancé ni reçu de consensus ; pas un échec du délai de finalité. Volume VPS 1926 fichiers vérifiés inchangés. Correction répertoire 0755/env clés 0600 qualifiée avec image réelle, sans volume de chaîne/signature. Configuration dérivée localement et liée au plan par empreinte ; aucune exportation depuis Mac. Nouvelle tentative requise, finalité et production ouvertes. Preuves restored-three-node-v17-startup-terminal-20261006/.


2026-10-06 — Préparation nouvelle tentative v17 après correction de permissions : copies Mac vérifiées intactes et réutilisées sans copie supplémentaire ; volume VPS inchangé. Aucun signer lancé, binaires C++ identiques qualifiés, préparation VPS locale requise. Même critère de trois finalités nouvelles en 900 s ; finalité et production restent ouvertes. Preuves restored-three-node-v17-retry-prepare-20261006/.


2026-10-06 — Reprise v17 nouvelle tentative FAILED : contrôleur 37147 / sortie 1, checkout 30f144de, cœur/relais d8d5b46c ; 0 avancées communes observées selon le critère original. Traces de relais/ressources et hypothèses de parcours enregistrées ; cause WAN globale non prouvée. Trois propriétaires arrêtés et audits read-only des engagements, journaux/témoins et historique canonique ; fichiers inchangés. Pas de compilation concurrente, ni transfert de base inter-hôtes. Périmètre privé exact ; production et revue indépendante restent ouvertes. [Preuves v17](qualification/restored-three-node-v17-retry-terminal-20261006/README.md).


2026-10-06 — Catalogue : contre-exemple de starvation par rotation du namespace de requête reproduit sur le relais antérieur (sortie attendue 1), correctif intégré qualifié en natif et Docker ARM64/AMD64 : 5 suites affectées, 11/46/19/7/10 scénarios et compilation P2P. Conservation du curseur de sélection pendant la session ; nouveaux snapshots acquis avec le namespace courant et réinitialisation explicite au changement de session. Échec du lanceur natif sans la définition CMake conservé, relance corrigée réussie. Images/volumes réutilisés, cœur et capture inchangés. Cause WAN complète et progression de finalité toujours non qualifiées ; v17 reste FAILED. [Preuves](qualification/catalog-generation-rotation-integrated-20261006/README.md).


2026-10-06 — Préparation v18 : engagements post-v17 et journaux conservés, catalogue à sélection équitable qualifié dans son périmètre local. Trois finalités communes nouvelles en 900 s restent à démontrer ; préparation seule. Production et revue indépendante ouvertes. Preuves restored-three-node-v18-namespace-selection-prepare-20261006/.


2026-10-06 — Reprise v18 namespace de sélection FAILED : contrôleur 53478 / sortie 1, checkout f457bd3b, cœur d8d5b46c / relais 5288e024 ; 0 avancées communes observées selon le critère original. Traces de relais/ressources et hypothèses de parcours enregistrées ; cause WAN globale non prouvée. Trois propriétaires arrêtés et audits read-only des engagements, journaux/témoins et historique canonique ; fichiers inchangés. Pas de compilation concurrente, ni transfert de base inter-hôtes. Périmètre privé exact ; production et revue indépendante restent ouvertes. [Preuves v18](qualification/restored-three-node-v18-terminal-20261006/README.md).


Mesure après arrêt v18 : natif 631 et Docker Mac 556 blocs retenus sur 1024, donc aucune saturation du graphe à cette frontière arrêtée. Le lecteur enrichi conserve exactement les autres champs de capture du lecteur qualifié, sans écriture de données. Cela ne prouve pas les causes de tous les refus pendant la campagne. Les participations récentes des deux états Mac restent limitées à leurs propriétaires ; finalité commune non avancée.


2026-10-06 — Ancre locale signée : contre-exemple de famine reproduit puis correctif intégré. Admission distante obligatoire ; refus explicite permet le parcours borné des parents, résultat ambigu sans retry automatique. Neuf suites SDK et scénario réel --wire qualifiés en natif/ARM64/AMD64, contrôleurs 94615/86191/33176 sortis 0. Images/volumes réutilisés ; cœur inchangé, fixture consommatrice recompilée. Cause WAN globale et finalité commune non prouvées ; échecs historiques et toutes les exigences restantes conservés. [Qualification ciblée](qualification/signed-anchor-integrated-20261006/README.md).


2026-10-06 — Préparation v19 : engagements post-v18 et journaux conservés, ancre signée qualifiée en natif/ARM64/AMD64 dans son périmètre ciblé. Trois finalités communes nouvelles en 900 s restent à démontrer ; préparation seule. Production et revue indépendante ouvertes. Preuves restored-three-node-v19-signed-anchor-prepare-20261006/.


2026-10-06 — Reprise v19 FAILED au démarrage : contrôleur 81355 / sortie 1, checkout fc704b9f, délai IPC VPS de 180 s expiré ; propriétaires Mac non lancés et critère de finalité non exécuté. Arrêt VPS normal et audit read-only des engagements/journaux/historique, 2170 fichiers inchangés ; base et clés non exportées. Cause IPC précise encore ouverte, diagnostic isolé en préparation sans changement des politiques. [Preuves v19](qualification/restored-three-node-v19-startup-terminal-20261006/README.md). Toutes les autres exigences et la revue indépendante restent ouvertes.


2026-10-06 — Diagnostic IPC/RPC intégré et qualifié dans son périmètre : 14 suites affectées et --wire natif/ARM64/AMD64, originaux 43059/8395/34086 sorties 0. Étapes de récupération et causes RPC expurgées ; mêmes règles de fencing/admission/retry. 46 scénarios de récupération, 18 transports loopback et 6 classifications ; cœur réutilisé, fixture recompilée, images inchangées. Cause réelle v19 et finalité commune non prouvées. [Preuves ciblées](qualification/startup-diagnostic-integrated-20261006/README.md). Échecs et critères de production/revue indépendante conservés.


2026-10-06 — Préparation v20 : engagements VPS post-v19 et Mac post-v18 et journaux conservés, diagnostic IPC/RPC qualifié en natif/ARM64/AMD64 dans son périmètre ciblé. Trois finalités communes nouvelles en 900 s restent à démontrer ; préparation seule. Production et revue indépendante ouvertes. Preuves restored-three-node-v20-startup-diagnostic-prepare-20261006/.


2026-10-06 — v20 FAILED : contrôleur original 34949 / sortie 1, checkout e8008698, zéro nouvelle finalité commune en 900 s (3 requises), malgré trois propriétaires restaurés actifs. Trois arrêts normaux ; audits read-only des engagements et témoins comparés aux reçus indépendants, bases inchangées. Le démarrage IPC réussit sans expliquer v19. Les participations Mac restent locales ; les logs ne prouvent pas encore une cause unique. Brouillon non intégré : 48 cas attestation + 10 cas diagnostic RPC passent, sans qualification WAN. [Preuves v20](qualification/restored-three-node-v20-terminal-20261006/README.md). Les autres critères et la revue indépendante restent ouverts.


2026-10-06 — Diagnostic dépendances/ACK des attestations et refus HTTP intégré et qualifié dans son périmètre : natif/ARM64/AMD64, 14 suites affectées et vraie fixture --wire, sources figées vérifiées. 48 cas attestation, 10 classifications/redactions et 18 transports loopback ; aucune modification des politiques ou du quorum. Trace runtime optionnelle bornée à 256 événements d’attestation par nœud. Ne corrige ni ne qualifie la finalité WAN v20 ; cause réelle encore à établir. [Preuves ciblées](qualification/delivery-diagnostic-integrated-20261006/README.md). Les autres critères et la revue indépendante restent ouverts.


2026-10-06 — Préparation v21 : engagements VPS post-v20 et Mac post-v20 et journaux conservés, diagnostic des dépendances de votes et refus HTTP qualifié en natif/ARM64/AMD64 dans son périmètre ciblé. Trois finalités communes nouvelles en 900 s restent à démontrer ; préparation seule. Production et revue indépendante ouvertes. Preuves restored-three-node-v21-delivery-diagnostic-prepare-20261006/.


2026-10-06 — v21 FAILED au démarrage : contrôleur 5353 / sortie 1, checkout 32e75e19, nano_capellaStatus read-error aux étapes status/verify, récupération not-ready puis délai IPC VPS 180 s expiré. Mac non lancés et inchangés (1573/1538 fichiers) ; critère de finalité non exécuté. Arrêt VPS normal, audit read-only des engagements/témoins/historique vérifié sur son hôte. Défaut local reproduit contre cœur inchangé : network_identity et sync_admission_stats bloquent sous admission retenue (deux sorties 1 attendues, annulation sans publication). Correctif et causalité WAN restent ouverts, aucune relance ni limite augmentée. [Preuves v21](qualification/restored-three-node-v21-startup-terminal-20261006/README.md). Toutes les autres exigences et revue indépendante restent ouvertes.


2026-10-06 — Lectures de statut : défaut local identity/sync reproduit contre ancien cœur, correctif qualifié natif/ARM64/AMD64 (78144/46395/11667 sorties 0). Publication authentifiée pour identité et verrou distinct pour compteurs ; aucun quorum, délai ou fencing modifié. ABI cohérente : 136 unités scannées, 18 consommateurs recompilés par architecture, programmes parents conservés. Deux régressions de lecture sous admission retenue passent, ainsi que Beacon/reprise/HTTP/wire ciblés. Échec initial de fixture conservé. [Preuves](qualification/network-status-coherent-20261006/README.md). Effet sur WAN/finalité, release sans instrumentation et autres critères/revue indépendante non qualifiés.


2026-10-06 — Préparation v22 : engagements VPS post-v21 et Mac post-v20, copies Mac v21 non exécutées et journaux conservés, correctif de contention des lectures de statut qualifié en natif/ARM64/AMD64 dans son périmètre ciblé. Trois finalités communes nouvelles en 900 s restent à démontrer ; préparation seule. Production et revue indépendante ouvertes. Preuves restored-three-node-v22-network-status-prepare-20261006/.


2026-10-06 — v22 FAILED : contrôleur original 91325 / sortie 1, checkout 80cd143e, zéro nouvelle finalité commune en 900 s malgré trois propriétaires actifs et rattrapage des hauteurs VPS. Trois arrêts normaux ; audits read-only et engagements/témoins vérifiés, fichiers inchangés (1526/1558/2360). Premier audit natif limité à 1024 blocs échoué et conservé ; requête hors ligne bornée à 32768 réussie, lecteur et limites runtime inchangés. Dépendances de votes bloquées par profondeur dans les traces ; cause WAN complète non prouvée. Brouillon non intégré : contre-exemple ancien code échoué, sept modes natifs revus passent ; runtime/Docker/WAN non qualifiés. [Preuves v22](qualification/restored-three-node-v22-terminal-20261006/README.md). Autres exigences et revue indépendante ouvertes.


2026-10-06 — Préparation bornée des dépendances par pair intégrée : cible/cursor locaux authentifiés, mêmes plafonds et ACK effectifs, remise à zéro aux deux chemins de changement de session. Natif/ARM64/AMD64 : quatorze suites affectées + sept variantes nouvelles, runtime nano-p2p compilé et scénario wire ciblé ; 21 tests Docker passent, sources figées vérifiées et cœur d285db64 réutilisé sans modification. Liaison native initiale erronée exclue, nouvel objet relié et scénario exécuté après terminal original. [Preuves ciblées](qualification/dependency-preparation-integrated-20261006/README.md). Effet sur finalité WAN v22 non qualifié ; autres critères et revue indépendante ouverts.


2026-10-06 — Préparation v23 : états Mac/VPS post-v22 et témoins audités, copies locales sans identité de pair et volume VPS conservé sur son hôte. Cœur inchangé et nouveau relais de préparation bornée qualifié dans son périmètre natif/ARM64/AMD64 ; mêmes limites et trois nouvelles finalités communes en 900 s restent à démontrer. Préparation seule ; production et revue indépendante ouvertes. Preuves restored-three-node-v23-dependency-preparation-prepare-20261006/.


2026-10-06 — v23 FAILED avant lancement : contrôleur 57911 / sortie 1, garde du préparateur VPS limitée à l’ancien nom de lot. Aucun nœud/signataire lancé, zéro reçu ; finalité non testée et copies Mac inchangées (1525/1557). Garde de placement corrigée sans dépendre du nom de lot, digest binaire/canonical hash conservés ; 11 chemins et 3 dérivations auditées qualifiés localement, capsule non auditée refusée. Aucun binaire/image modifié. [Preuves v23](qualification/restored-three-node-v23-preparation-terminal-20261006/README.md). Production/revue indépendante et finalité WAN ouvertes.


2026-10-06 — Préparation v24 : états Mac/VPS post-v22 et témoins audités, copies locales sans identité de pair et volume VPS conservé sur son hôte. Cœur inchangé et nouveau relais de préparation bornée qualifié dans son périmètre natif/ARM64/AMD64 ; mêmes limites et trois nouvelles finalités communes en 900 s restent à démontrer. Préparation seule ; production et revue indépendante ouvertes. Preuves restored-three-node-v24-dependency-preparation-prepare-20261006/.


2026-10-06 — v24 FAILED : contrôleur original 4013 / sortie 1, checkout 49362f29, cœur d285db64 et relais 514ad2b8. Zéro nouvelle finalité commune en 900 s ; trois propriétaires actifs et trois arrêts normaux. Audits read-only : engagements et témoins comparés aux reçus indépendants, hashes inchangés ; budget de rejeu forensic 32768 blocs / 512 Mio, limites runtime inchangées. Régression de tête Docker Mac 18356 → 10760 observée, checkpoint finalisé 1284 conservé ; cause globale et effet WAN du correctif non qualifiés. [Preuves v24](qualification/restored-three-node-v24-terminal-20261006/README.md). Toutes les exigences non démontrées et la revue indépendante demeurent ouvertes.


2026-10-06 — Reprise du parcours de dépendances après délai intégrée : parent local authentifié conservé comme curseur, jamais comme ACK ; effacement avant émission, mêmes limites et fencing. Contre-exemples exports lents/dupliqués : parent 20 tours sans envoi, correctif 15 tours/9 ACK. Natif/ARM64/AMD64 : 23 cas ciblés, runtime et wire liés au cœur d285db64 inchangé, hashes vérifiés. Premier pilote sans retard exclu et conservé. [Preuves](qualification/timeout-preparation-integrated-20261006/README.md). Effet WAN V24 et régression de branche non qualifiés ; autres critères et revue indépendante ouverts.


2026-10-06 — Complément de revue : garde après fermeture du relais avant toute mutation du curseur. Un seul header change ; 12 cas affectés et runtime passent en natif/ARM64/AMD64. Wire précédent réutilisé après scan réel de dépendances et hashes des autres entrées inchangés ; aucun rejeu identique. [Preuves](qualification/timeout-closed-guard-integrated-20261006/README.md). Finalité WAN V24, autres critères et revue indépendante toujours ouverts.


2026-10-06 — Préparation v25 : états Mac/VPS post-v24 et témoins audités, copies locales sans identité de pair et volume VPS conservé sur son hôte. Cœur inchangé et relais de reprise après délai qualifié dans son périmètre natif/ARM64/AMD64 ; mêmes limites, trois nouvelles finalités communes en 900 s à démontrer. Préparation seule, échec V24 et revue indépendante ouverts. Preuves restored-three-node-v25-timeout-preparation-prepare-20261006/.


2026-10-06 — V25 FAILED : original 29725 / sortie 1, checkout e928b177, zéro nouvelle finalité commune en 900 s. Rattrapage partiel de blocs avec ACK réels ; Docker Mac régresse 18793→10936. Trois arrêts normaux, engagements/témoins et hashes vérifiés. Diagnostic authentifié après arrêt : graphes 794/758/627 sur 1024, catalogues individuels sous 64 Kio ; aucune hausse de quota, pas de rejeu canonique identique. Échec initial de liaison du diagnostic conservé et réparé après terminal original avec le même objet. Traces bornées : capacity/queue-expired, suspensions/reprises et erreurs CatalogAcquire ; cause globale non prouvée. [Preuves V25](qualification/restored-three-node-v25-terminal-20261006/README.md). Investigation contention RPC/votes récents ; toutes exigences non démontrées et revue indépendante restent ouvertes.


2026-10-06 — Diagnostic neuf du contrôle catalogue : capture retenue volontairement, génération et release bloqués, puis annulation terminée et état primaire inchangé (original 51762, sortie 0). Premier échec de compilation conservé, pas de modification runtime ni de réseau relancé. Couplage du verrou reproduit ; cause WAN non prouvée. Préparer réservation/capture hors verrou avec fencing et quotas inchangés ; qualifier ensuite contention HTTP et votes. [Preuves](qualification/catalog-control-contention-probe-20261006/README.md).


2026-10-06 — Capture catalogue hors verrou de contrôle : lot intégré natif/ARM64/AMD64, originaux 65381/1980/4618 sorties 0 ; 349 sources figées, deux consommateurs ABI reconstruits, cinq modes concernés réussis. Quatre scénarios neufs (release/stop/expiry/cancel), quotas et courses conservés. Échecs initiaux de montage conservés. Pas de finalité WAN démontrée ; contention de la file HTTP et votes récents encore ouverts avant nouvelle campagne. [Preuves](qualification/catalog-capture-control-integrated-20261006/README.md).


2026-10-06 — Contention de deux travailleurs reproduite (14001/0) : capture retenue + acquéreur en attente font expirer génération/release dans la file. Draft à refus immédiat (48776/0) : contrôles non expirés, réservations drainées et primaire inchangé ; mêmes deux threads et quotas. Comparaison isolée sans HTTP réel, cause WAN et runtime intégré non qualifiés. Prochain lot : code RPC occupé distinct, HTTP réel et qualification des deux architectures. [Preuves](qualification/catalog-worker-starvation-counterfactual-20261006/README.md).


2026-10-06 — Course de génération confirmée (18115/0) : nettoyages étrangers annulent une capture utile. Draft avec réservation suivie (11207/0) : captures bénignes préservées, retraits propres avant/après rotation toujours clôturants, capacité récupérée et primaire inchangé. HTTP réellement activé sur trois architectures ; omission initiale du mode et helper ARM64 sans fixture conservés. Lot final ABI/HTTP/quotas encore à qualifier ; cause et finalité WAN non closes. [Preuves](qualification/catalog-generation-reservation-counterfactual-20261006/README.md).


2026-10-06 — Lot catalogue intégré : refus occupé immédiat (-32005), réservation suivie à travers rotation, retrait propre/arrêt/expiry/cancel conservés. Originaux 57019/81866/79512 sorties 0, sept modes natif/ARM64/AMD64, 350 sources figées et deux consommateurs ABI reconstruits. Scénarios HTTP réel et génération vérifiés explicitement ; modes omis et échecs historiques conservés. Quotas/threads/quorum inchangés. Cause globale et finalité WAN restent ouvertes ; autres tâches RPC/votes récents à qualifier avant clôture production. [Preuves](qualification/catalog-reservation-integrated-20261006/README.md).


2026-10-06 — Préparation v26 : états Mac/VPS post-v25 et témoins audités, copies locales sans identité de pair et volume VPS conservé sur son hôte. Cœur ee5e278b qualifié pour réservation de catalogue/contrôles RPC ; relais ad536163 réutilisé avec preuve de dépendances compilateur inchangées sur trois architectures. Mêmes limites, trois nouvelles finalités communes en 900 s à démontrer. Préparation seule, échec V25 et revue indépendante ouverts. Preuves restored-three-node-v26-catalog-reservation-prepare-20261006/.


2026-10-06 — V26 FAILED : original 95043 / sortie 1, checkout 80292a4d, cœur ee5e278b ; 0 nouvelles finalités communes distinctes, trois requises en 900 s. Trois arrêts normaux, engagements/témoins et hashes vérifiés après arrêt. 2 changements vers des têtes plus anciennes conservés. Diagnostics publics bornés : CatalogAcquire -32005 sur VPS, interprété comme invalid-response par le relais ; causalité non démontrée. Limites inchangées, aucune image reconstruite ni relance. [Preuves V26](qualification/restored-three-node-v26-terminal-20261006/README.md). Tous critères non démontrés et revue indépendante restent ouverts.


2026-10-06 — Contre-exemple HTTP après V26 : le refus explicite de capture -32005 disparaît dans Core ; busy puis disponible échoue sans second acquire. Prototype natif borné : sept scénarios, reprise sur ce seul refus explicite, même identité de requête, pas de rejeu des réponses perdues/inconnues/tardives, release unique et nettoyage du handle tardif. Backend RPC simulé et catalogue vide ; pas de capture moteur ni de finalité WAN qualifiées. Bind initial interdit par isolation conservé, même binaire réparé par accès loopback autorisé. Prototype non intégré, qualification Docker/propagation head-catalogue restantes. [Preuves](qualification/catalog-busy-http-counterfactual-20261006/README.md). Tous écarts non démontrés et revue indépendante restent ouverts.


2026-10-06 — Reprise catalogue intégrée partiellement : natif 25/25 (2780/0), AMD64 25/25 (1514/0), ARM64 24/25 (3458/8), préparation lente échouée. Neuf scénarios HTTP/wrapper passent ; backend simulé, aucune finalité WAN qualifiée. Traces ancien/nouveau à 40 ms non substituables à l’échec ; contre-exemple à 60 ms échoue identiquement (20 tours, 2/9 admissions), quotas inchangés. Limite de progression sous délais à traiter ; source non entièrement qualifiée, pas de relance réseau. [Preuves partielles](qualification/catalog-busy-integrated-partial-20261006/README.md). Échecs historiques et revue indépendante ouverts.


2026-10-06 — Borne du test de préparation corrigée, sans changement runtime : 54 tours dérivés de N=9 et assertion de progrès à chaque tour lent ; anciens échecs à 20 tours conservés. Dix scénarios natif/ARM64/AMD64 passent (6543/10997/56288, sorties 0), 60 ms → 45 tours ; ARM duplicate à 40 ms → 22 tours. Ni accélération ni garantie WAN ; nouvelle inscription CTest et consommateurs core/wire à vérifier avant V27. [Preuves](qualification/catalog-preparation-progress-bound-20261006/README.md). Toutes exigences non démontrées et revue indépendante ouvertes.


2026-10-06 — Fixtures concernées reconstruites avec nouveaux headers réseau et core EE5 inchangé : natif/ARM64/AMD64 --wire passent (51773/43538/58855, sorties 0), 17 messages et deux réouvertures internes ; networkActivated=false/finalizedEpoch=0. Enregistrement CTest du nouveau mode vérifié ; sources/deps/hashes contrôlés, erreurs de driver conservées. [Preuves](qualification/catalog-busy-core-fixture-20261006/README.md). Lot candidat à livrer puis reprise V27 ; aucune finalité WAN ajoutée, critères non démontrés et revue indépendante ouverts.


2026-10-06 — Préparation V27 depuis états post-V26 audités : pins possédés 16/16/33 et historiques 16/16 vérifiés séparément, hashes revérifiés et copies Mac sans peer.key, volume VPS conservé sur son hôte. Core EE5 inchangé ; nouveau relais du lot 2f93dda0 réutilisé avec 35 entrées compilateur identiques par architecture. Trois nouvelles finalités communes en 900 s à démontrer ; mêmes quotas et quorum, aucune image reconstruite. Préparation seule, V26 FAILED et revue indépendante ouverts. Preuves restored-three-node-v27-catalog-busy-retry-prepare-20261006/.


2026-10-06 — V27 FAILED : original 42291 / sortie 1, zéro nouvelle finalité commune en 900 s. Audits après arrêt 34309/8101 sorties 0 : engagements vérifiés, fichiers inchangés. Diagnostic 85421/0 reproduit une acquisition catalogue bloquée derrière le mutex du réacteur malgré annulation ; checkpoint non appelé pendant le verrou. Cause globale WAN non démontrée. Correction non bloquante en cours de qualification, pas de relance identique ni d’image reconstruite. [Preuves](qualification/restored-three-node-v27-terminal-reactor-wait-20261006/README.md). Tous critères non démontrés et revue indépendante restent ouverts.


2026-10-06 — Draft réacteur : acquisition catalogue non bloquante, refus RPC occupé existant, réservation/annulation/retrait conservés. Cinq modes natifs passent ; échec bind sandbox original 42101 conservé, même binaire HTTP loopback 35744/0. Docker ARM64 34853 et AMD64 75818 en cours, pas encore qualifiés. [Preuves natives partielles](qualification/catalog-reactor-failfast-native-partial-20261006/README.md). Pas de campagne WAN relancée ; cause globale et critères de production/restants ouverts.

2026-10-06 — Mise à jour du draft : ARM64 initial 34853/1 arrêté avant scénario pour chemin de fixture incorrect, échec conservé. Même binaire et montage corrigé, original 82245/0 : cinq modes passent et source inchangée. AMD64 75818 encore actif ; pas de qualification trois architectures ni WAN, pas de push du draft.


2026-10-06 — Lot réacteur intégré : capture catalogue non bloquante derrière admission, RPC occupé existant, quotas/quorum/délais inchangés. Huit modes par architecture natif/ARM64/AMD64 passent ; mêmes binaires réutilisés après réparations des drivers, erreurs initiales conservées. Originaux runtime 76431/9495/15849 sorties 0, 1602 sources figées correspondantes et hashes des exécutables relevés. [Preuves](qualification/catalog-reactor-failfast-integrated-20261006/README.md). Pas de WAN relancé ni d’image construite ; V27 FAILED, cause globale/finalité commune et toutes exigences non démontrées/revue indépendante restent ouvertes.


2026-10-06 — Préparation V28 : états post-V27 audités et revérifiés, pins possédés 16/16/33 et historiques conservés séparément, copies Mac sans peer.key et volume VPS réutilisé sur son hôte. Cœur 02ec0ab0 qualifié dans huit modes sur trois architectures ; relais 2f93dda0 réutilisé avec 35 dépendances projet inchangées par architecture. Mêmes quotas et trois nouvelles finalités communes en 900 s à démontrer. Aucun signataire lancé ni image reconstruite. V27 FAILED, cause globale et critères non démontrés/revue indépendante ouverts. Preuves restored-three-node-v28-reactor-failfast-prepare-20261006/.


2026-10-06 — V28 rejet préalable 93900/1 avant démarrage : garde du dossier network inclut le header sessions core modifié. Copies locales inchangées, zéro signataire/nœud/arrêt. Métadonnées rectifiées : alignement relayRevision 02ec0ab0, origine relayBuildRevision 2f93dda0 ; 35 entrées compilateur et hashes des mêmes binaires vérifiés, aucun garde désactivé. [Échec conservé](qualification/restored-three-node-v28-preflight-rejection-20261006/README.md). Première campagne effective et finalité restent à démontrer.


2026-10-06 — [Migration Capella fragmentée intégrée](qualification/capella-fragment-checkpoint-integrated-20261006/README.md) : nouveau scénario valide et trois régressions passent sur Docker ARM64/AMD64. Sources figées 1606 vérifiées, builds incrémentaux (trois unités + fixture), images existantes. Migration explicite, format conservé, récupération primaire précédent via témoin/engagement externe, enfant signé durable, trois refus de corruption. Échecs montage initiaux et limite nofile AMD64 conservés ; seul test de reprise rejoué avec nofile=65536. Flag runtime true non exercé au boot, pas de release propre ou de nœud WAN actualisé. Cache froid, limite1024, collecte des fragments, activation réelle et finalité commune V28 restent ouverts ; aucune qualification EIP globale, bridge/Doppler/production ou revue indépendante ajoutée.


2026-10-06 — [Lecteur de records Capella froids](qualification/capella-cold-record-reader-20261006/README.md) : nouvelle classe locale, références engagées/hash/racines/liaisons Beacon-EVM vérifiés, cache borné en nombre et volume encodé, pins protégés. Fixture valide Docker ARM64/AMD64 : hit sans fetch, pression refusée avant fetch, remplacement LRU, quatre refus et annulation sans mutation ; scénario de migration du lot précédent passe aussi. Deux erreurs initiales de forme de fixture conservées, fixture seule corrigée ; 1608 sources vérifiées, archives/lecteur inchangés pendant réparation. **Non activé dans le graphe runtime**, aucun gain RSS ou finalité qualifié ; rétention complète/limite1024/V28 FAILED restent ouverts. Pas de promotion EIP/production, bridge ou Doppler ; revue indépendante et autres critères inchangés.


2026-10-06 — [Records Capella et leases immuables](qualification/capella-record-leases-cold-graph-20261006/README.md) : graphe froid en fixture, choix de tête/transition signée/snapshot/clock identiques au témoin chaud, mutation candidate préservant snapshots et leases. Six modes Docker ARM64/AMD64 passent, 24 unités core reconstruites, sources figées comparées ; erreurs de préparation et de syntaxe conservées. API head/records modifiée ; builds incrémentaux, outils syntaxe ARM seulement, pas de release propre ni de nœud déployé. Runtime encore chaud, capacité1024/format plat complet/cache concurrent/RSS/finalité WAN V28 FAILED restent ouverts. Aucun critère EIP global, bridge, Doppler, production ou revue indépendante clôturé.


2026-10-06 — [Cache froid concurrent](qualification/capella-cold-cache-concurrency-20261006/README.md) : loads sérialisés, attente annulable, consultation usage en callback, loads réentrants refusés et pins protégés. Nouvelle fixture Docker ARM64/AMD64 : huit lecteurs, un fetch, annulation après attente effective, scénarios graphe/migration associés passent ; core deux unités reconstruites et fixture seule renforcée, hashes inchangés. Sources finales vérifiées, logs initiaux/renforcés séparés, aucune image reconstruite/nœud déployé. Pas de TSAN, runtime froid ou gain RSS qualifié ; rétention JSON/format plat/capacité1024/finalité V28 FAILED et toutes autres exigences non démontrées/revue indépendante restent ouvertes.


2026-10-06 — [Writer de références Capella](qualification/capella-reference-writer-20261006/README.md) : nouvelle API locale, rows traitées une par une sans memo JSON complet, émission des rows chaudes via staging et réutilisation froide après comparaison canonique. Docker ARM64/AMD64 : manifeste chaud/froid identique, expansion canonique identique, annulation source inchangée, format refusé par le restaurateur legacy. Fixture deux records : 91668 octets plats / 4017 manifeste, budget par row44336 ; pas de mesure runtime/RSS. Une unité serializer+fixture reconstruite, API legacy conservée, sources/hashes vérifiés. Restaurateur nouveau format/publication atomique/provider lifetime/runtime froid/capacité1024/finalité V28 FAILED et autres critères/revue indépendante restent ouverts.


2026-10-06 — [Restauration froide directe](qualification/capella-reference-restore-20261006/README.md) : manifeste engagé avant fetch, records validés un par un, graphe de handles froids, registre reconstruit et comparaison canonique sans réexpansion plate. Budget cache total distinct du budget par row. Docker ARM64/AMD64 : graphe/tête identiques, sept refus, annulation avant lecture et pression insuffisante refusée ; writer/graphe/cache/migration associés passent. Helper legacy refactoré : configured-runtime/automatic-recovery concernés passent avec mêmes binaires. Deux unités+fixture reconstruite, sources/hashes vérifiés, aucune image/nœud déployé. Provider lifetime/publication atomique/witness/runtime froid/capacité1024/finalité V28 FAILED et tous autres critères non démontrés/revue indépendante restent ouverts.


2026-10-06 — [Barrière de durée de vie des lecteurs](qualification/capella-checkpoint-provider-lifetime-20261006/README.md) : nouveau provider local, fermeture avant nouvelles lectures, attente du fetch actif et suppression du callback, lecteurs survivants refusés. Docker ARM64/AMD64 : lecture RocksDB réelle/digest vérifié, fermeture concurrente, marqueur de possession libéré, close idempotent et destructor qualifiés dans fixture ; scénarios compact/graphe/migration associés passent. Une nouvelle unité+fixture, sources/hashes vérifiés, pas d’image/nœud déployé. Non raccordé au réacteur ; callbacks non réentrants et I/O bloquante restent contraintes explicites. Ownership réacteur/publication atomique/runtime froid/capacité1024/finalité V28 FAILED et autres exigences/revue indépendante restent ouverts.


2026-10-06 — [Réacteur Capella et publication froide](qualification/capella-cold-reactor-integration-20261006/README.md) : providers PRIMARY/WITNESS possédés et fermés par le réacteur, migration explicite vers un manifeste froid, format conservé, références vérifiées avant batches synchrones. Docker ARM64/AMD64 terminaux 0 : annulation pré-I/O, idempotence, récupération PRIMARY-previous depuis WITNESS, vrai bloc signé, graphe publié froid, refus de lecture différée après fermeture et deux redémarrages ; trois régressions legacy concernées passent, hashes inchangés. Fermeture de 16 unités core dépendantes + fixture, builds incrémentaux seulement. Erreurs initiales de compilation, faux passes du pilote invalidés et teardown de fixture interrompu SIGTERM conservés ; causes corrigées. Journal créé vide et absence de nouveaux crashs injectés : pas de qualification complète anti-double-signature/panne physique. Configuration coldCheckpoint implémentée mais activation boot non qualifiée. Capacité1024/V28 FAILED, finalité WAN, pruning froid/GC/pic mémoire/RSS, autres gates et revue indépendante restent ouverts ; aucune promotion EIP/bridge/Doppler/production, aucun nœud/image déployé.


2026-10-06 — [Runtime froid et pruning compact — qualification partielle](qualification/capella-cold-runtime-20261006/README.md) : budgets coldCheckpoint validés avant toute publication/ouverture witness ; pruning froid compare des manifestes validés sans expansion plate. ARM64 : deux parcours runtime froid terminaux0, chacun32 blocs/64 clés/pruning/reprises ; neuf refus config, reprise automatique engagée et rollback refusé, admission BLS non vide. Fixture alignée sur helper busy de production : cinq refus -32005 suivis de80 attestations livrées en13ms ; mêmes délais/refus stricts. Régression legacy échouée faute de fixture JSON publique dans l’emballage, puis ce seul mode rejoué avec mêmes binaires :72 blocs,6 mouvements anchor,finalizedEpoch7,64 journaux réouverts,hashes inchangés. Échecs initiaux ARM64/AMD64 et diagnostic conservés. AMD64 corrigé EN COURS ; aucune qualification multiarchitecture ni livraison candidate de ce lot tant que sortie terminale/audits manquent. V28 FAILED/capacité1024/finalité WAN/GC/RSS/reprise physique et tous autres gates/revue indépendante restent ouverts ; aucun bridge/Doppler/fork supplémentaire/production qualifié.


2026-10-06 — Runtime froid : cold-automatic AMD64 terminal0,32 blocs/64 clés/pruning/reprise engagée ; cold-bls encore actif, qualification du lot non close. [Prototype isolé idempotence](qualification/capella-duplicate-fastpath-prototype-20261006/README.md) ARM64 : trois mutations locales refusées/réacteur indisponible, annulation et replays sans mutation ; trois paires sur deux records,32 replays96–98ms→3,6–3,7ms. Échec de fixture conservé/corrigé sans rebuild core. Prototype non appliqué/main ni AMD64 qualifié ; aucun gain à grande échelle/critère EIP/WAN/production clôturé.

Lot runtime froid : AMD64-v3 cold-automatic et cold-bls terminaux0 ; contrôleur92481 terminal1 uniquement sur fixture warm absente. Réparation warm seule41583 active, hashes programmes/core/parent vérifiés avant exécution ; audit source1616 fichiers sans écart. [Logs conservés](qualification/capella-cold-runtime-20261006/README.md). Aucun critère WAN, capacité, EVM ou production promu.

Clôture runtime froid ARM64/AMD64 : six scénarios pertinents terminaux0 (deux cold et warm réparé par architecture), intégrité parent/programmes et source vérifiée. Les échecs catalogue/fixture restent archivés. Cela ne qualifie pas WAN, graphe1024, crashs froids, GC/RSS, release propre, nouveaux EIP, bridge/Doppler ou production.

Prototype writer froid : seconde passe de liens via en-têtes après authentification complète initiale ; restauration avec cache1 et sept refus passent sur ARM64 parent runtime. Composition avec duplicate-fastpath en cours ARM64/AMD64, aucune qualification combinée ni intégration du writer revendiquée. [Sources et preuves](qualification/capella-cold-header-links-20261006/README.md).

Composition writer/duplicate ARM64 : contrôles réacteur et trois mutations de stockage terminaux0, puis reference writer/restore terminal0 avec même binaire et hashes inchangés. Le premier pilote sortait avant ce second parcours ; la lacune est conservée et le pilote AMD64 corrigé avant lancement. Qualification AMD64 du parent duplicate34314 encore en compilation de fixture. Aucune qualification complète, intégration du writer ou gate production fermée.

Correctif duplicate-fastpath, parent runtime5b3f5c48 : intégré et qualifié ARM64/AMD64 dans la fixture2 records ;32 replays par série, engagements et archive inchangés, trois altérations locales de stockage refusées puis réacteur fermé. AMD6434314 terminal0, hashes vérifiés, source1616 fichiers sans écart. Le gain paired reste ARM64 seulement. [Preuves et échecs historiques](qualification/capella-duplicate-fastpath-prototype-20261006/README.md). Writer froid combiné : AMD6440272 en cours, code du writer toujours isolé. Aucun critère WAN/grand graphe/release/production fermé.

Writer par en-têtes : prepared-block ARM64/AMD64 terminaux0 et hashes inchangés ; runtime32/cache8 encore actif78239/7934, variante isolée. Préparateur restored-three-node raccorde coldCheckpoint explicite avec budgets locaux et pins conservés,38 tests Node terminaux0 ; cela ne migre aucune base ni ne qualifie le WAN. [Preuves du planner](qualification/restored-plan-cold-options-20261006/README.md).

Clôture writer froid par en-têtes : ARM6478239/AMD647934 runtime32/cache8 terminaux0,hashes vérifiés,CPP/fixtures figés identiques ; évolution JS distincte qualifiée38 tests. Aucun état lourd n’est supprimé de la première validation. Publications complètes/preflight,graphe1024,GC/RSS/reprise physique/WAN et toutes les gates production restent ouverts. Prototype de certificats local séparé, non intégré au runtime.

Prototype validation-memo isolé : ARM6413329 terminal0,oracle canonique/reuse2/refus5 et invalidation profile,hashes parent/programmes inchangés. Aucun raccordement runtime ou contournement du préflight ; AMD64 et grands graphes non qualifiés. [Preuves](qualification/capella-reference-validation-memo-20261006/README.md).

Mémo API isolé ARM64/AMD64 terminal0 (13329/88013),oracle/refus5/profil/hashes qualifiés sur2 records. Raccordement réacteur isolé : nouveau layout,17 unités core,ARM648281/AMD6485922 en cours ; préflight complet conservé et mémo installé seulement après succès durable. [Sources et critères](qualification/capella-reference-memo-runtime-20261006/README.md). Aucun code du mémo intégré au main, aucune gate production fermée.

Raccordement mémo isolé ARM648281 terminal0 :17 core TU,4 modes dont deux runtimes32/cache8 avec reuse de certificats exigé,hachages avant/après OK ; AMD6485922 actif. Cela ne clôt pas préflight complet,graphe1024,SIGKILL/reprise physique,WAN ou production ; code toujours hors main.

Lot isolé cold-memo-sync du6 octobre : ARM6472535 terminal0, quatre erreurs fsync simulées principale/témoin avec suffixe conservé/perdu, oracle durable exact, archives et reprise idempotente. Parent runtime mémo AMD6485922 dernier BLS actif ; nouveau scénario AMD64 non lancé. Pas de code main, coupure physique/SIGKILL, capacité1024 et WAN/production toujours ouverts. Preuves docs/qualification/capella-cold-memo-sync-20261006.

Prototype SIGKILL froid du6 octobre : ARM6486820 terminal0 sur trois frontières WAL/commit, quatre EIO de régression passent car shim modifié ; hashes parent/test/shim inchangés. AMD6464866 actif, aucun code de ce prototype appliqué au main5f2977a6. Ce SIGKILL ne prouve pas panne électrique ni conservation indépendante des engagements réels ; migration interrompue et reprise/WAN toujours ouvertes. Preuves docs/qualification/capella-cold-memo-sigkill-20261006.

Prototype préflight local du6 octobre : ARM6429991 terminal0, quatre scénarios séparés ; dix refus, EIO4/SIGKILL3/guards3 et runtime automatic/BLS32 blocs/cache8 passent. Échec de syntaxe pilote38141/2 conservé, binaires réutilisés sans recompilation ; AMD6479564 actif, aucun code appliqué au main7121cb60. Certificats pour lignes froides identiques seulement, chaque blob encore lu/hashé ; startup et opérations signées restent entièrement vérifiés. Preuves docs/qualification/capella-reference-preflight-reuse-20261006. Grand graphe, WAN et production restent ouverts.


---

**16 octobre 2026 — Bootstrap natif/Linux partagé réellement qualifié à froid ; production non qualifiée.**

Le [lot partagé](qualification/shared-bootstrap-mac-linux-20261015/README.md) livre quatre overlays du launcher existant et un préparateur natif explicite, sans modifier le V7 historique. Le [reçu réel](qualification/shared-bootstrap-mac-linux-20261015/evidence/real-local-bootstrap.json) passe en172742ms : un export/domaine synthétique consommé, import natif65 paires/65 réouvertures,4 signatures BLS de bootstrap vérifiées, import Linux65, copie des indices16–31/16 paires et32 réouvertures C++ à froid. Les imports/copie/audit créent0 nouvelle signature ;51/51 conteneurs absents et6/6 volumes conservés vérifiés. Les anciennes autorités et16 aliases restent stricts, sans rebaser d’inodes natifs en Linux. Les deux imports sont indépendants à partir du même export/plan ; la copie physique reste Linux→Linux, sans transfert de SST native ni qualification cross-OS/AMD. Aucun journal historique manquant du Mac/NAS n’est restauré.

Six syntaxes et les nouveaux cas ciblés passent séparément :2 purs réutilisés sans replay,1 guard/3 refus et2 refus du préparateur. Le premier collecteur reste FAILED sur l’attente de7 SKIP ; les2 cas réellement sélectionnés avaient réussi. V2 du contrôleur est resté HOLD/non exécuté avant la correction de l’ordre des clés. Tous les échecs historiques et la limite du digest brut privé07 non enregistré restent conservés.

**Aucun runtime Nano/P2P, configuration runtime, reprise des duties, coordinateur complet, finalité commune ou cycle complet n’est qualifié par ce PASS.** Reprise/finalité des trois propriétaires, cycle long/inclusion/sanction/retraits, WAN/ressources, autorisation dédiée AMD/VPS encore en attente, release/CI/revue indépendante et Base–Nano/bridge/Doppler restent ouverts. Aucun accès VPS privé, build/pull d’image, fonds ou vente ; goal actif. Aucun EIP/fork/profil ni production promu. Corps historique intégral conservé en préfixe, CSV/RPC inchangés.


---

**16 octobre 2026 — Préflight d’horloge avant chaque premier spawn : tests locaux ciblés réussis, runtime toujours ouvert.**

Le [lot clock](qualification/initial-runtime-clock-20261016/README.md) revérifie les budgets déjà approuvés après préparation et au point de chaque premier démarrage, après les attentes des pairs, du tunnel et des configurations. Le garde d’âge depuis le Genesis est conservé ; la capacité réelle de rattrapage depuis l’horloge durable est contrôlée. Aucun budget, profil, Genesis, P2P, propriétaire de clé ou restore ultérieur n’est modifié.

Le [terminal Root exact](qualification/initial-runtime-clock-20261016/TEST-STATUS.json) clôt quatre syntaxes et quatre nouveaux tests clock déjà réussis, réutilisés sans replay ; seul le guard concerné a été rejoué : un test/trois refus attendus, exit0 en9540ms. Les échecs des collecteurs01 (stabilisation avant Node) et02 (imports publics manquants dans la replica du guard) restent conservés ;03 ajoute les quatre imports exacts puis réussit, sans assouplissement des sources ou autorités.

Aucun C++, Docker, seed/export, runtime ou accès VPS privé dans ce lot de tests. Le préflight utilise l’horloge du Mac et ne qualifie pas l’horloge réelle du VPS, le démarrage C++, les duties, la finalité commune ou le cycle long. Les critères production et autorisations encore ouverts restent ouverts ; aucun EIP, CI, bridge, lancement ou production promu. Les quatre corps documentaires historiques sont conservés intégralement en préfixe, CSV/RPC inchangés.


### 16 octobre 2026 — Construction canonique AMD64 et API ciblée ; production non qualifiée

Le lot [canonique AMD64](qualification/canonical-amd-20261016/README.md) ajoute une preuve sur les sources Git `712b8357d05f09cc28df6e04920c9955e59b7b69` : 143 objets/membres Core, sept exécutables Core et un consommateur réseau, puis 158 fermetures indépendantes M=MD (157 C/C++ et une assembleur), dont 151 Core/consommateurs et sept supplémentaires, plus deux recettes LLVM. L’essai 06 réutilise les sorties construites, compile uniquement une copie assembleur munie d’un vrai MD et exige ses octets identiques à l’original ; l’absence du MD original reste signalée.

Le premier CTest API de cette campagne réussit : cinq groupes, neuf statuts, 21 lectures, 16 refus attendus, deux signatures BLS de fixtures et zéro signature d’audit, sans DB ou runtime de nœud. Une lecture indépendante publie les noms, SHA et tailles des neuf artefacts et retrouve les neuf champs/SHA originaux inchangés. Les échecs 01–05 sont conservés, avec leurs causes observées, diagnostiquées ou inférées explicitement distinguées. Le pic de 72 187 904 octets mesure seulement fermeture/assembleur/test 06 ; ce n’est pas une mesure de la construction complète, d’un nœud ou de sa charge.

Les deux limites `sourceAdmissionDigestPreviouslyRetained:false` et `admissionDescriptorDigestPreviouslyRetainedOffsite:false` restent ouvertes comme limites de provenance historique. Cette voie emploie uniquement les sources publiques et l’image SDK existante ; elle ne lit pas les anciennes métadonnées SDK privées refusées. Les permissions opérationnelles privées restent distinctes. Les corps historiques et la matrice CSV sont conservés sans promotion EIP/fork. L’objectif demeure actif : les nouvelles admissions pour la campagne, la reprise des duties, le cycle complet des validateurs, la finalité commune entre trois propriétaires, la qualification réseau et la production ne sont pas démontrés par ce lot ; aucune CI n’est revendiquée.


### 16 octobre 2026 — Fermeture sélectionnée et deux consommateurs Native ; production non qualifiée

Le [rapport Native](qualification/native-effective-20261016/README.md) garde l’audit global **FAIL** : 14 828/14 835 fichiers réguliers vérifiés, sept fichiers en écart et 14 constats. La fermeture sélectionnée 04 réussit sur Git `def5cda61919bbb40e91744a18661a2ad7a76eba` : 363 sources égales aux blobs, 3 190 fichiers originaux dont 2 535 SDK et 3 761 résolutions vérifiés ; 142 membres/MD hérités et un membre API retenu, cinq nouveaux M, zéro C++/test. Les sept écarts globaux sont hors de ce périmètre, sans effacer l’échec global. Les trois échecs de collecteur et les deux versions source non exécutées restent conservés.

La reproduction prospective distincte de `storage` est byte-identique au membre conservé (625 856 octets, 1 183 dépendances M=MD, trois observations sur 2 648 ms). Sa liaison historique d’alias reste inconnue. Le contrôle exporteur/support (13 matériels, 16 alias, 26 arêtes) et la comparaison de branches CMake ne valent pas fermeture de tous les chargeurs ou recette CMake complète.

Le premier build Native à deux consommateurs reste **FAIL** au CXX, après deux M. Une overlay unique du support de test corrige deux additions GMP/uint64 Apple par conversion décimale exacte (`07993…/24334` vers `66819…/24376`), sans Core/EVM modifié. L’essai 02 réussit : deux compilations/liens et deux vrais M=MD, puis trois CLI ciblées, neuf groupes et 47 refus attendus, sept signatures BLS de fixtures et zéro signature opérationnelle. Politique d’actions, HEAD scellé et soumission via callbacks sont vérifiés ; **ni cycle complet de paiements/retraits, ni retraits uint64 de bout en bout, ni HTTP/exec réel ne sont qualifiés**. Les anciennes preuves ARM/VPS restent liées à l’ancien header.

Les deux nouvelles identités Native portent les identités connues de 22 à 24 ; **les 24 admissions de nouvelle campagne restent toutes à false**. Les identités/matériels connus de P2P et des anciens exporter/importer ne ferment pas leurs dépendances C++/SDK. Recette CMake effective complète, chargeurs, admissions composites, reprise des duties, finalité commune, cycle complet et production restent ouverts. Aucun EIP/fork ou statut CI n’est promu ; les corps historiques et les échecs sont conservés, l’objectif demeure actif.


## 16 octobre 2026 — Native Legacy : deux liens qualifiés, production non qualifiée

Le [lot Native Legacy](qualification/native-legacy-20261016/README.md) publie le terminal réel `197411f7…` : trois M égaux aux MD historiques, une recompilation et un lien du support dont l’objet et la bibliothèque restent identiques octet pour octet, puis deux liens des objets consommateurs conservés contre le Core `7264c212…`. Aucun configure, rebuild du Core ou changement d’archive n’a été exécuté. Deux refus CLI de politique attendus qualifient le chargeur et l’entrée principale des nouveaux exporter/importer Native ; ils ne prouvent ni un export/import réel, ni toutes les résolutions de symboles différées. Aucun test ancien, signature, Docker ou duty n’a été exécuté.

Le préflight conserve les 16 matériaux originaux et l’historique des refus d’orchestration. La nouvelle reproduction identique ne restitue pas la qualification historique inconnue du producteur du support et ne requalifie pas les anciens exécutables. L’échec global des entrées originales et ses sept fichiers hors fermeture sélectionnée, l’alias historique inconnu, la fermeture C++/SDK de P2P, la configuration CMake complète et les 24 admissions de campagne restent ouverts. Les 24 identités connues ne valent pas 24 admissions. Le header `6681926f…`, le CSV EIP et le RPC sont inchangés ; aucun EIP ou fork n’est promu.

La CI de `7c004a00…` est en échec pour facturation/plafond GitHub avant démarrage des jobs concernés : aucun succès CI n’est revendiqué. Le goal reste actif ; vrai transport HTTP/exec, cycle complet, reprise des duties, finalité multi-propriétaire et production restent non qualifiés.


## 16 octobre 2026 — Tool ARM avec header uint64/GMP, production non qualifiée

Le [lot ARM Tool](qualification/arm-tool-u64-20261016/README.md) publie le terminal réel `ecca356b…` et sa jointure `6ae291a5…` : un M indépendant égal au MD, une TU compilée, un lien contre le Core conservé `9ccadf8f…`, deux sélecteurs CLI réussis (six groupes, 21 refus attendus, sept signatures BLS de fixtures, zéro signature opérationnelle). Les neuf feuilles Tool, dont le header `6681926f…`, et 88 autres sources projet atteintes sont liées aux blobs de `a6fdbaed…`. Les 441 fichiers SDK ont de nouvelles gardes FD/path neuf champs/SHA dans le même conteneur ; aucune ancienne autorité n’est rafraîchie et aucune récupération du SDK historique ou du digest brut de l’ancienne admission observe n’est revendiquée.

Conteneur et volume de travail possédés sont absents après nettoyage. Aucun configure, rebuild Core ou build/pull d’image n’a été exécuté. Le refus initial de staging et l’échec de tentative01 avant M/CXX/lien/CLI restent conservés avec le code exécuté. La CI de la base a6 retourne zéro run, sans succès CI revendiqué.

La qualification concerne ce Tool Linux ARM64 et ses deux CLI. Submit ARM, Tool AMD avec ce header, P2P, CMake complet, symboles différés, chemin complet des paiements/retraits uint64, vrai HTTP/exec, cycle des validateurs, 24 admissions de campagne, reprise des duties, finalité multi-propriétaire et production restent ouverts. L’échec global historique des entrées Native (sept fichiers hors fermeture sélectionnée) et l’alias historique inconnu restent ouverts. Aucun EIP/fork n’est promu. Aucune source C++ n’est modifiée par ce paquet ; header66819, CSV et RPC restent inchangés. Goal actif.


## 16 octobre 2026 — consommateurs lifecycle Linux, production non qualifiée

Le [lot Linux consumers](qualification/linux-consumers-20261016/README.md) conserve ARM Submit69ef : huit sources projet correspondent au Git a3, MD/M producteur/ensemble441déps concordent ; zéro compilation, lien, scan ou CLI rejoué. Ce contrôle ne délivre aucune nouvelle qualification SDK/loader et ne récupère ni les neuf champs historiques du programme ni le digest brut de son ancienne admission. Son auditeur était inline Root, sans source autonome exacte conservée.

AMD produit le Tool45090… avec66819 : deux M égaux aux MD conservés, une TU/un lien Tool, zéro compilation/lien Submitba626 réutilisé, trois CLI réussis (neuf groupes,47refus,sept signatures BLS de fixtures,zéro opérationnelle). La postqualification vérifie143enregistrements utilisés, pas143objets Core, et trois nouveaux produits inchangés ;493lignes SDK ont été retenues dans le même conteneur, sans relecture SDK après suppression. Les portées Tool524/Submit442 ne s’ajoutent pas à493. Conteneur possédé absent, zéro configure/Core/image/retestpostqual. Le refus préflight0400 et les deux refus de collecteur (sérialisation, nom de champ) restent conservés ; aucune altération ni nouvelle réussite C++ n’en est déduite. Les quatre lacunes historiques source/admission/recette9/SDK restent false, sans rafraîchissement.

Toujours24identités connues et zéro admission de campagne. L’importeur ARM/Core9a03→9cc, P2P, CMake/loader complets, permissions de nouvelle campagne, fullpayout/retraits uint64, vrai HTTP/exec, lifecycle complet, duties, finalité commune et production restent ouverts, avec l’échec global Native/septfichiers hors fermeture et l’alias inconnu. Aucun EIP/fork/CI promu ; aucune source C++ modifiée, header66819/CSV/RPC et tous les historiques conservés. Goal actif.


## 16 octobre 2026 — Assets de reprise : importeur ARM actuel et inventaire P2P natif

Le lot `docs/qualification/runtime-assets-20261016` ferme la dérive de Core de l’importeur ARM pour sa nouvelle liaison : un `-M` indépendant égal à l’ancien MD, un lien contre Core `9ccadf8f…`, zéro CXX et un refus d’arguments attendu. Le programme `3b635993…` reste byte-identique ; l’ancienne production contre Core `9a03ae14…` n’est pas renommée. Les 44 phases et la postqualification passent, 86 lignes hôte sont conservées et le conteneur/volume ont disparu. Les 444 lignes SDK sont enregistrées dans le même conteneur, sans relecture SDK après suppression. L’import historique 65 n’est pas rejoué. Le refus initial d’admission des nouveaux descripteurs est conservé.

L’inventaire P2P natif trouve 101 archives : 79 séquences ordonnées de membres non-index égales aux archives de build, mais zéro archive complète byte-identique ; le premier refus sur ce critère reste conservé. 127 objets ont leurs MD et recettes présents, avec 123 sources de TUs égales au Git direct et quatre sources Protobuf générées. Aucun `-M`, compilateur, outil SDK, Docker ou réseau n’est exécuté. Les 22 archives transitives restantes contiennent 683 membres, pas 683 TUs ; les dix paquets source en cache n’authentifient pas leurs producteurs. L’objet du consumer Native historique reste absent. La provenance historique, la fermeture transitive et les producteurs Linux ne sont pas qualifiés.

Les 24 identités connues restent à zéro admission de campagne. Les anciens journaux Mac manquants, l’échec global et l’alias historique inconnu, les fermetures CMake/loader complètes, la restauration réelle, les duties à trois propriétaires, la finalité commune et le cycle complet restent ouverts. Aucun EIP, fork, CI ou statut de production n’est promu ; le goal reste actif.


## 16 octobre 2026 — Résolution fonctionnelle du provider Boost P2P natif

La résolution du provider Hunter Boost est qualifiée sur la configuration et la liaison ARM64 de `nano-p2p` : M indépendant égal au MD réel, 1 739 headers Boost et deux archives Hunter Boost 1.87 avec le tag `-a64`, sans provider Boost extérieur. Les refus de tag contradictoire et d’architecture universelle sont attendus ; le retour à ARM64 passe. Les 4 434 fichiers SDK précédemment retenus gardent leurs neuf champs et SHA, sans rafraîchissement d’autorité.

Le premier configure du correctif est conservé comme refus réel sur `Boost::algorithm` ; la correction suivante traite explicitement deux targets header-only. Le premier build source complet reste un échec après 827,363 s sur dérive de métadonnées fraîches, avec octets inchangés et cause physique non établie. Le deuxième build source complet est encore actif : la résolution fonctionnelle ne qualifie pas ce producteur global.

Les 418 avertissements de cible macOS 26.6 contre 26.0 maintiennent la compatibilité OS ouverte. Aucun nœud ou réseau n’a été exécuté par cette qualification, aucun Core ou image Docker n’a été construit. Les 24 identités connues restent à zéro admission de campagne. Les producteurs transitifs, la fermeture complète CMake/loader, la reprise réelle des anciens journaux/duties et la finalité commune restent ouverts. Aucun EIP, fork, CI, cycle complet ou statut de production n’est promu ; le goal reste actif.


## 16 octobre 2026 — Consolidation Native02 et parseur du total des payouts

Pour BoringSSL, 392 sources Release, dont 113 sous `gen/`, sont égales au paquet canonique. Les quatre archives Release/Debug couvrent 784 membres par valeur avec 786 reçus de compilation qualifiés M==MD. Pour 143 membres, plusieurs candidats de la même configuration subsistent. Les 786 chemins `.o` concernés sont absents : aucun objet physique courant ni causalité exacte `ar`/DAG n’est vérifié. Le premier inspecteur refusant `nlink==1` sur des archives à `nlink==2` reste un échec conservé, sans archive modifiée ni build interrompu.

Le garde JS du seul `fullPayoutGwei` total est corrigé dans ce lot, après validation en copie source isolée : pour N payouts positifs, le total BigInt canonique doit satisfaire `N <= totalGwei <= N * (2^64 - 1)`, sans être limité lui-même à uint64. Le vrai test ciblé passe ses deux groupes et dix refus attendus, sans échec ni skip, en 189 ms sous Node v25.9.0. Le premier essai de préparation a refusé avant tout Node sur une assertion de métadonnées ; le chemin divergent n’a pas été retenu. L’ancienne suite runtime n’a pas été rejouée. Aucun C++, header, montant individuel uint64 ou crédit Wei uint256 n’est modifié.

La construction source SDK globale reste en cours, sans terminal de succès acquis. Le correctif JS et son nouveau test sont intégrés indépendamment de ce build C++, qui ne change pas pour ce lot. La qualification du consommateur exige les mêmes sources réseau/tiers/script SDK que `5fc36d9a`, même sur un descendant ne changeant que le harness/documentation. Ces preuves ferment le parseur du total, pas un cycle long, des sorties/paiements réellement exécutés, une reprise ou la finalité. Les autres appels compilateur, recettes d’archivage, DAG transitif, runtime, admissions de campagne, duties et production restent non qualifiés.

[Preuves publiques et périmètre](qualification/native02-source-and-payout-20261016/README.md). Aucun critère de production ni échec antérieur n’est retiré.


## 16 octobre 2026 — SDK source natif terminé et consommateur lié ; production non qualifiée

La construction Native02 à partir des sources termine réellement avec le code 0 après 6 683,292 s, avec zéro construction d’image Docker ou de Core et aucun démarrage de nœud. Le consommateur `nano-p2p` est configuré et lié au nouveau SDK : 101 archives, M indépendant égal au MD réel, 1 739 headers Boost Hunter 1.87 et tag ARM64 `-a64`. Ses sources réseau restent celles de `5fc36d9a…` ; le HEAD `769ab6f3…` conserve les mêmes trois entrées Git réseau/tiers/script SDK. Le refus d’usage sans identité sort avec le code 1 attendu. Le nouvel exécutable est `65d08eee…`, 13 406 616 octets. Le linker ne produit aucun avertissement ; cela ne qualifie pas l’exécution sur d’anciens macOS.

La jointure couvre par valeur les 810 occurrences de membres des archives : 636 ont un seul candidat de compilation qualifié observé, 174 en ont plusieurs. Ce ne sont ni des TUs ni des tests. L’index conserve séparément 2 276 appels M==MD, 74 sorties non nulles, 22 refus sur M indépendant non nul et 371 observations forward ; aucun échec n’est converti en réussite. Les constats de chemins historiques restent distincts : objets 137 présents/741 absents, MD 266 présents/622 absents, sans rafraîchissement d’autorité. Ils ne prouvent ni causalité `ar`, ni tous les générateurs, ni le DAG complet.

Les anciens refus et l’échec global des sept fichiers hors closure restent conservés. La CI du commit `769ab6f3…` échoue pour facturation avant démarrage des jobs, sans tests exécutés ni relance/dépense. Les 24 identités restent sans admission de campagne. Loader complet, anciens OS, ce SDK sur AMD64, runtime réseau, restauration, duties, finalité commune et production restent ouverts ; aucun EIP, fork ou cycle complet n’est nouvellement qualifié. Le goal reste actif.

Preuves publiques et sources des controles : [Native02 SDK](qualification/native02-sdk-terminal-20261016/README.md).


## 16 octobre 2026 — Reçu d’audit à froid historique rapproché

Le reçu autorisé historique confirme un PASS borné : 64 validateurs, 178 blocs rejoués, 12 blocs distants et 16 blocs observés des trois propriétaires, zéro nouvelle signature/runtime. La tentative actuelle transfère les fichiers autorisés et retrouve 13 entrées conformes, zéro divergente/manquante, puis refuse la sortie déjà existante avant tout nouvel audit ; ce n’est pas un échec C++. Le résultat historique compact correspond exactement au reçu VPS retenu (3 579 octets). Le run08 initial reste FAILED ; aucune ancienne autorité neuf-champs/octets n’est rafraîchie. L’état actuel des volumes, la restauration, les duties, la finalité commune et la production ne sont pas qualifiés.

[Recu historique et observation actuelle](qualification/authorized-historical-cold-audit-20261016/README.md).


## 16 octobre 2026 — environnement natif du lanceur : tests ciblés ; production non qualifiée

Le mode fresh utilise un snapshot natif figé, refuse les contrôles loader et les deux variables de child possédées par le superviseur, et conserve les overlays de fixture/soumission attendus. Les environnements Docker/VPS et le C++ sont inchangés. ROOT a passé trois groupes purs, couvrant 29 refus de politique. Ces groupes déjà verts en tentative 01 sont réutilisés sans replay. Les trois échecs de projection 01–03 restent conservés ; aucun n’est transformé en réussite. La tentative 04 passe le guard existant pertinent : un test, trois cas enfants d’autorisation refusée, 6 600 ms, zéro échec/cancel/skip/todo. Aucun Nano, Docker, réseau ou accès à des clés réelles n’est exécuté par ces vérifications.

La garde d’environnement ne démontre ni égalité de toutes les valeurs avec l’ancien consommateur Native02, ni fermeture complète du loader. Les symboles lazy, la compatibilité OS/protocole, les admissions de campagne, le runtime, les duties, la finalité commune et la production restent ouverts. Aucun test C++, rebuild SDK ou nouveau probe de binaire n’est revendiqué. Les critères, échecs et limites historiques restent intégralement conservés ; le goal demeure actif.


## 16 octobre 2026 — sélecteurs des simulations RPC : preuve causale ARM ; production non qualifiée

ROOT a compilé deux TUs seulement, rpc et un test dédié, avec M indépendant égal au MD (529 et 463 dépendances). Le Core copié conserve 142 membres sur 143 ; son nouveau membre rpc égale l’objet compilé. Le test neuf lié à l’ancien Core refuse exactement le premier objet connu après sa baseline chaîne valide (code 1 attendu, quatre checks). Le candidat passe cinq groupes, 120 checks et 58 refus, sans ancienne suite, image reconstruite, nœud ni réseau. La tentative 01 reste refusée avant M/CXX/test : dérive de ctime de trois nouveaux contrôles, octets identiques, cause inconnue. Les anciennes autorités sont intactes ; conteneur absent et volume Linux de sortie conservé.

Cette preuve couvre partiellement eth_call selon EIP-1898 ; estimateGas est une extension Nano explicite. Aucun configure CMake complet n’a été exécuté : CMake global, candidat multiarchitecture/déploiement, état propre des branches non canoniques retenues, autres méthodes EIP-1898, HTTP, AMD64, CI, admissions de campagne, duties, finalité commune et production restent ouverts. Les historiques et limites restent conservés ; le goal demeure actif.

[Sources et preuves RPC](qualification/rpc-eip1898-simulation-20261016/README.md) · [Politique d’environnement natif](qualification/native-runtime-environment-20261016/README.md).


## 16 octobre 2026 — sélecteurs RPC : preuve causale AMD ; production non qualifiée

ROOT a passé sur Docker AMD64 le test dédié déjà qualifié sur ARM : cinq groupes, 120 checks et 58 refus, avec baseline ancien Core refusée exactement comme attendu (code 1, quatre checks). Deux TUs seulement ont été compilés avec M indépendant égal au MD, 527 et 461 dépendances ; le Core copié conserve 142 membres sur 143 et son nouveau membre rpc égale l’objet compilé. Le test prend 129 ms et le contrôle hôte 113 227 ms. Les stdout baseline/candidat sont identiques à ARM. Les 18 fichiers hôte tenus restent inchangés ; conteneur absent, répertoire de sortie conservé sur le VPS.

Les anciennes tables complètes de matériaux/autorités SDK ne sont pas relues ; les reçus de recettes peuvent contenir des noms SDK et les dépendances sélectionnées de l’image actuelle sont réellement relues/admises. Aucune autorité historique n’est récupérée. L’arrêt normal ne qualifie pas le chemin TERM/KILL. Eth_call reste partiellement couvert, estimateGas est une extension Nano ; CMake global, Nano cohérent multiarchitecture, autres méthodes, états de branches non canoniques, HTTP, CI, admissions, runtime, duties, finalité et production restent ouverts. La CI du commit 1f6 échoue pour facturation avant lancement des jobs, sans relance. Tous les historiques sont conservés ; goal actif.

[Preuve AMD](qualification/rpc-eip1898-amd-20261016/README.md) · [Preuve ARM](qualification/rpc-eip1898-simulation-20261016/README.md).


## 16 octobre 2026 — Premier essai lifecycle autorisé : finalité initiale, cycle FAILED

[Sept projections publiques et limites](qualification/fresh-lifecycle-first-attempt-20261016/README.md). Root a reçu le consentement pour un essai synthétique privé borné à sept heures, puis l’a exécuté sur trois runtimes/deux hôtes (source 4984e7e). Les 65 paires bootstrap 16+16+33 passent 130 réouvertures froides ; les runtimes signent et admettent réellement des duties. Finalité commune observée 0 → 2 → 3 : deux avances distinctes puis trois échantillons stables ; trois premiers arrêts sains.

L’essai termine **FAILED, exit1** avant toute soumission du dépôt : comparaison JS d’adresse sensible à la casse malgré identité canonique/montant exacts. Dépôt non soumis ; sortie/slashing et restauration non atteints, aucun paiement du cycle qualifié. Le domaine est consommé, sans reset/rejeu autorisé par ce constat. L’audit terminal refuse les stages incomplets : capture VPS du premier arrêt et second arrêt/capture VPS absents ; quatre captures locales ne remplacent pas ces preuves. Le guard rapporte 29 mesures, aucune SIGINT et le launcher joint, sans preuve de tenue sept heures.

Le correctif JS est ensuite qualifié séparément : ancien 1 PASS / 1 FAIL contre candidat 2 PASS, document exporté accepté inchangé ; suite ciblée 18/18, 0 skip (9 variantes de casse/35 refus). Aucune clé lue, signature ou transaction soumise par ces tests, aucun C++/image reconstruit ni nouvelle campagne. Cette réussite ne requalifie pas l’essai FAILED.

Consentement reçu pour cet essai ne vaut ni réussite du cycle complet ni autorisation supplémentaire déduite. Historiques intégraux conservés : V37/campagne08 et ancien fence ARM restent échoués/ouverts, anciennes identités non récupérées, NAS refusé, Base/Doppler et CI/facturation inchangés. Restauration/duties après reprise, finalité après restauration, cycle long, WAN/charge, bridge/mint, revue indépendante et production restent ouverts. Aucun EIP/fork promu ; goal actif.


## 16 octobre 2026 — sauvegarde physique arrêtée : outil livré, reprise consensus encore ouverte

Un seul CLI `scripts/operations/stopped-backup.mjs` fournit create/verify/restore de
deux racines complètes, preuves externes épinglées, synchronisation des fichiers et
répertoires, publication exclusive et restauration vers un dossier neuf. Les
engagements de consensus ne sont pas déduits des copies. Documentation :
`docs/STOPPED-BACKUP.md` ; preuves : `docs/qualification/stopped-backup-20261016`.

Qualification Root : 26/26 tests Mac et 26/26 Linux ARM sur volume Docker ; le
prédécesseur échoue à la nouvelle régression de mutation d'entrée (0 PASS/1 FAIL).
L'essai Docker sur partage de dossier macOS reste 21/26, 5 FAIL de métadonnées ou
permissions, voie non qualifiée. Aucun ancien champ d'admission n'est rafraîchi.
Deux DB RocksDB synthétiques arrêtées au genesis sont sauvegardées/vérifiées/
restaurées ; deux audits C++ restituent exactement les hashes/racines des originaux.
Sources RO, neuf champs et SHA inchangés ; aucun build C++/image, clé, signature ou
réseau. Le volume neuf de test a été supprimé, aucun volume existant supprimé.

Cette preuve physique ne fait pas de la seconde DB un témoin de consensus, ne
qualifie ni panne électrique, sauvegarde hors hôte, reprise de duties ou V37, et
n'ajoute aucun EIP/fork activé. Le cycle lifecycle complet reste échoué/non qualifié,
comme sa clôture incomplète ; ses journaux et le domaine consommé sont conservés.
CI/facturation, revue indépendante et toutes les autres exigences de production
restent ouvertes. Goal actif ; production non qualifiée.


## 16 octobre 2026 — Correction : captures VPS présentes, absence de copies locales seulement

[Preuve publique et correction ciblée](qualification/fresh-lifecycle-capture-correction-20261016/README.md). Les trois absences du rapport précédent étaient **locales Mac** : première capture VPS, second arrêt VPS et seconde capture VPS. Elles ne prouvaient pas une absence distante. Root a rapproché quatre fichiers distants connus ; le premier arrêt correspond au SHA Mac et les deux captures VPS sont PASS, chacune pour 33 paires et 68 DB en lecture seule. Le second arrêt épingle la première capture et ses témoins externes (première capture : jetons externes false ; seconde : true). Capsules conservées sur VPS, originaux inchangés ; replay historique canonique toujours non qualifié.

Le launcher attend les trois captures avant la clôture ; celle-ci refuse `lifecyclePending` absent après l'échec du dépôt avant soumission, et non l'absence des captures VPS. L'ancien rapport/manifeste restent intacts, cette seule inférence est corrigée. L'inspection ne relit pas les DB, ne signe rien et ne reprend aucun runtime. Essai FAILED, domaine consommé non réemployable ; aucun dépôt/retrait/slashing/paiement, restauration de duties, cycle complet, V37 ou production qualifié. Historiques et autres gates inchangés ; goal actif.

La sauvegarde physique arrêtée est ensuite qualifiée sur les deux racines entières data/witness RO de mac-docker : 369 fichiers (119 767 303 octets), create/verify/restore PASS. Les captures C++ restaurées valident 16 paires/34 DB et les jetons externes du second arrêt/témoins ; Core et états validateurs sont byte-identiques, inventaires historiques originaux respectés. Capture source de 2 667 597 octets byte-identique avant/après ; huit nouveaux contrôles Linux stables sur 2 018 ms, fence neuf champs/SHA PASS. Le premier refus Mac `control-nine-changed` après create reste non qualifié, sans rafraîchir ses anciennes autorités. Backup privé sur le même hôte Docker Mac : aucune reprise de duties, signature, replay historique, sauvegarde hors hôte, panne électrique ou production qualifiée. Aucune nouvelle campagne ; nouvelle tentative en attente d'autorisation.

Le correctif JS du reçu avant clôture passe 13/13 tests (cinq groupes nouveaux, garde des 55 refus conservé). La reprojection hors ligne des trois captures réelles retenues représente 65 paires/136 rôles DB ; aucune nouvelle ouverture des 136 DB. Le test publie avec fsync avant une clôture en échec **injecté** : aucune publication terminale par un nouveau runtime, nouvelle campagne ou réussite lifecycle n'en est déduite.


## 16 octobre 2026 — backup VPS entier et copie lifecycle statx v2, campagne suivante autorisée

[Preuves, sources v1/v2 et refus conservés](qualification/lifecycle-whole-copy-vps-backup-20261016/README.md). Le CLI de backup sauvegarde/vérifie/restaure deux racines VPS arrêtées entières RO : 743 fichiers/124 106 584 octets, captures C++ 33 paires/68 rôles DB avec jetons externes et Core/états byte-identiques. Capture finale source de 2 713 333 octets égale au SHA historique. Les inventaires des racines restent historiques ; les neuf champs des fichiers de preuves privées sont figés dans l'opération avec SHA historiques, sans récupération d'une ancienne table. Backup privé sur le même VPS, aucune garantie hors hôte.

La route lifecycleRestore est distincte. Deux défauts Perl (concaténation et fsync parents) sont corrigés ; parent/mutant 12 PASS/1 FAIL chacun, premier candidat 13 PASS/1 FAIL de projection Root conservé, puis guard seul et fixture exacte PASS, pas unique 14/14 vert. La vraie copie v1 échoue à create ; diagnostic RO : trois observations atteignent 38 170 ms > fenêtre 30 s. V2 statx passe 15/15 sur ARM64 et AMD64, garde historique de 55 refus, 12 refus ABI et contrôles nanosecondes/FD/chemins exacts ; vraie copie VPS v2 PASS : 743 fichiers/70 répertoires, création puis capture C++ 33 paires/68 rôles, vérification et capture source finale identique ; trois observations, temps de stabilité 17 352 ms et étendue 12 198 ms. Limites 175/30 s inchangées, source base c1b + module candidat v2 non encore main lors du contrôle ; C++/CMake inchangés. Aucun ancien échec requalifié, aucune durée standalone inventée.

Un nouveau consentement autorise la tentative privée suivante, sept heures/65 nouvelles identités/33 clés VPS/restaurations/actions contrôlées/deux preuves Mac→VPS, sans fonds/vente/déploiement public. Elle n'est pas commencée, aucun nouveau seed/jeu de clés à ce constat ; ancien domaine/journaux sans reset. Aucun replay historique, duties repris, nouveau cycle, V37, panne électrique, off-host ou production qualifiés. Historiques et CI/facturation conservés ; goal actif.


> **17 octobre 2026 — sortie admise, réorganisation et pruning : causalité puis correction Native.** D07 reproduit réellement la perte 1→0 après pruning réussi sur l’ancien Core, sans changement de tête. D10 réutilise les builds D09, effectue un lien sans nouvelle compilation, puis passe conservation/réouverture/réinclusion et différé sans mutation. D13 passe aussi les deux régressions existantes `--queued-exits` et `--exit-pending-pruning`, sans recompiler le TU D12 ; « lot12 » ne signifiait pas douze tests. Le nouveau Core Native compte 143 objets, dont 142 inchangés. Les échecs préparatoires et D07 causal restent conservés, sans requalification de D09/D11/D12. Linux reste attendu. La tentative03 est autorisée distinctement, mais non activée ; ni campagne, reprise/duties/finalité commune, panne physique ni production ne sont qualifiées. Anciens historiques et statuts EIP conservés. [Preuves et limites](qualification/exit-reorg-prune-20261017/README.md).


> **17 octobre 2026 — suivi Docker ARM du correctif exits/reorg/pruning.** ARM02 passe les deux nouveaux scénarios, conservation avec réouverture/réinclusion et différé sans mutation, après 2 M/2 CXX, remplacement du seul reactor dans 143 objets (142 inchangés), un lien et deux tests 0. Core ARM 36f77c8b…/41045640 B ; originaux hôte inchangés, observations guest stat/SHA distinctes de FD, cleanup 0. Le refus ARM01 avant worker (CID préparé absent, zéro build/test) reste FAILED. Les quatre invocations Native déjà acquises restent inchangées ; les deux régressions existantes ne sont pas exécutées sous Linux par ARM02, et AMD reste attendu. La tentative03 est autorisée mais non activée. Aucune reprise/duty/finalité commune, panne physique ou production qualifiée ; les historiques et statuts EIP sont conservés. [Preuves et limites](qualification/exit-reorg-prune-20261017/README.md).


> **17 octobre 2026 — correctif exits/reorg/pruning confirmé aussi sous Docker AMD64.** AMD01 passe les deux nouveaux cas en 36,090/36,808 s, après 2 M/2 CXX, remplacement du seul reactor dans 143 objets (142 inchangés), un lien et zéro image. Core a3fdf87a…/40811064 B ; originaux hôte inchangés, guest stat/SHA séparés des FD et cleanup réussi. Les deux nouveaux cas sont donc qualifiés dans trois environnements (Native macOS ARM64, Docker Linux ARM64/AMD64) ; les deux anciennes régressions restent qualifiées Native seulement, non exécutées sous Linux. Les exécutables/images runtime restent à actualiser et l’index 24 n’est pas retargeté. CI 39054451069 reste FAILED pour facturation avant étapes. Tentative 03 autorisée mais inactive ; aucune reprise/duty/finalité commune ni production qualifiée. Tous historiques et statuts EIP conservés. [Preuves et limites](qualification/exit-reorg-prune-20261017/README.md).


> **17 octobre 2026 — reliages du Core reactor : trois environnements PASS.** Native 7, ARM 6 et AMD 5 consommateurs totalisent 18 M/18 liens/21 CLI, avec réemploi d’objets authentifiés, zéro CXX/archive modifiée/P2P/ancienne suite. Les 11 ldd Linux et les clôtures publiques passent ; remappages MD et identités historiques non récupérées restent explicitement bornés. Le refus ARM01 avant Docker reste FAILED. Ces contrôles ne démarrent aucun runtime/duty et ne requalifient ni SDK/CMake globaux ni fermeture loader complète. L’index 24/admissions globales et les images candidates restent des lots distincts ; aucune fabrication d’image déduite. Tentative 03 autorisée mais inactive ; reprise/finalité commune et production non qualifiées. Tous anciens corps et statuts EIP conservés. [Reliages et limites](qualification/runtime-reanchor-relinks-20261017/README.md).


### Complément du 17 octobre 2026 — admissions courantes et images des deux architectures

Le [lot des trois admissions de code](qualification/reactor-runtime-code-admissions-20261017/README.md) est terminé : 24 artefacts sélectionnés (9 Native, 8 ARM64, 7 AMD64), audit AMD et clôtures finales positifs, une validation pure des admissions/locateurs. Les anciennes admissions et tous les refus restent conservés ; aucune validation complète de campagne, clé nouvelle, signature, duty ou finalité réseau n’est déduite de ce lot.

Les [images candidates ARM64 et AMD64](qualification/runtime-reanchor-images-20261017/README.md) sont assemblées et vérifiées à partir des exécutables retenus : neuf commandes Docker terminées par architecture, aucune compilation ni téléchargement demandé. La révision binaire reste 1925eee ; le HEAD documentaire d’exécution est 85f6922f. Aucun redémarrage de service ni déploiement public n’a été effectué. Ces images n’impliquent pas que les campagnes SDK utilisent désormais des images runtime à la place de leurs logiciels explicitement montés.

La tentative privée 03 est déjà autorisée distinctement, mais demeure inactive à la clôture de ces deux lots. Restent requis : restauration physique entière avec journaux/témoins, engagements séparés, reprise effective des duties et finalité commune qui avance sur les trois environnements, écarts de consensus/EVM/opérations encore ouverts, bridge et conditions Doppler. La revue de sécurité indépendante reste ouverte ; la production n’est pas qualifiée.


### Complément du 17 octobre 2026 — outils Doppler unsigned

Le [lot d’intégrité des revues Doppler](qualification/doppler-offline-invariants-20261017/README.md) ajoute un lecteur offline lié à une configuration approuvée séparément, le contrôle du checkpoint du fork privé avant impersonation et le minimum réellement reçu après swap. Root a qualifié 33 nouveaux cas API/mocks puis 4 cas de vraie CLI, exit0, sans RPC réel ni nouvelle image. Ces tests Node portent sur les outils de lancement, pas sur la chaîne C++.

Restent non qualifiés par ce lot : nouveau cycle complet Anvil du driver modifié, simulation Base actuelle, relation entre offre préémise Doppler et gateway initialement vide, réserve/ratio/conversion/remboursement/bridge et spécifications économiques. Tous les anciens échecs gardent leur statut, dont le rejet Base ; aucune vente, signature, dépense ou production n’est autorisée par ces tests. La campagne03 déjà active conserve ses sources C++/harnais, binaires, identités et engagements ; aucun redémarrage de chaîne n’est requis pour les seuls outils JavaScript.


### Complément du 17 octobre 2026 — nouveau cycle Doppler privé courant

Le [nouveau contrôle réel du driver Doppler](qualification/doppler-current-private-fork-20261017/README.md) termine exit 0 en 183 879 ms : 19 étapes locales Dynamic V4 → migrateur historique V2, deux swaps dont les sorties reçues atteignent le minimum de 99 %, trois transactions refusées avec état/soldes inchangés et six lectures canoniques `eth_call` dans **Anvil**, pas Nano. Instance privée ancrée au checkpoint Base 52 173 454 ; pas de preuve cryptographique de finalité Base. Les 17 fences hôte représentent 12 anciennes lignes + quatre payloads neufs + un contrôle, sans récupération de 17 admissions historiques. Enfants joints et conteneur absent ; aucune compilation C++, aucun build d’image ni redémarrage de chaîne.

Le refus préparatoire Root `KeyError: absentAfterCleanup` reste conservé, distinct du cycle réussi sans rejeu ; l’ancien rejet synthétique Base InvalidGamma n’est pas requalifié et les 37 tests offline ne sont pas rejoués. Actifs, financement, bénéficiaire et gouvernance restent synthétiques. Aucun fonds/déploiement public, bridge ou relation native, termes réels, T0/indexation ni production qualifiés. La campagne03 garde C++/52 fichiers de harnais, binaires, engagements et HEAD d’exécution `b1f…` ; elle n’est ni retargetée vers `4158…` ni requalifiée par ce lot. Historiques/statuts EIP conservés ; goal actif.


## 17 octobre 2026 — objectif de production réaffirmé par l’utilisateur

Le périmètre ci-dessous est conservé intégralement. Les références `391d434e` et
`efbeb9bc` appartiennent à l’historique demandé ; elles ne remplacent pas la
vérification des sources courantes. Au contrôle précédant cet ajout, `main` est
propre à `e8343be67350f9ab4cbbcf820143cd4313a68ba7`, la révision productrice C++
retenue reste `1925eeeabc9f7d0aef41f2c3fe43c11c31f1f352`, et la campagne03 conserve
son HEAD d’exécution `b1f27bf85e24ba0d9aa04a078c82a21040465202`.

### Cadrage demandé

Livrer Nanobees/Nano C++ comme L1 réellement prête pour la production et préparer son fair launch Doppler sur Base en USDC. Partir de l’état vérifié du dépôt /Users/boris/Dev/blockchain-nano : main 391d434e, candidate C++ figée efbeb9bc. Conserver intégralement docs/PRODUCTION-GOAL.md, docs/VALIDATOR-EIP-ROADMAP.md, docs/EVM-PRODUCTION-EIP-ROADMAP.md et docs/EIP-IMPLEMENTATION-STATUS.md, ainsi que les preuves acquises et tous les échecs historiques. Achever les deux campagnes Docker ARM64/Mac et AMD64/VPS déjà actives sans redémarrage injustifié, vérifier sorties terminales, sources, binaires et audits. Compléter la portabilité physique du stockage et la restauration entière de la candidate avec journaux et témoins anti-double-signature, vérifier les engagements conservés séparément, reprendre effectivement les duties et démontrer une finalité commune qui avance entre Mac natif, Docker Mac et Docker du VPS autorisé ubuntu@145.239.199.166. Fermer les écarts encore ouverts du consensus : partitions/asymétries/reconnexion dans leur périmètre exact, cycle complet des validateurs, rotations, sync, sanctions, anomalie WAN A, charge bornée et ressources. Qualifier le profil EVM/RPC réellement activé, sans revendiquer de forks non activés, avec statuts implémenté/qualifié/partiel/non commencé. Finaliser pruning, CI/builds reproductibles, sauvegardes/restauration, supervision/alertes et mises à jour Docker ; laisser la revue de sécurité indépendante explicitement ouverte tant qu’elle n’est pas réalisée. Travailler par lots fonctionnels complets ; réutiliser les qualifications seulement lorsque révision et périmètre concordent, relancer les tests concernés par les changements ou les écarts, conserver des logs accessibles et réutiliser les images/volumes existants. Livrer les changements revus sur main par Git push et actualiser le VPS par Git pull, sans reconstruire les images pour les seuls documents/outils JavaScript. Terminer la relation vérifiable entre token Base et actif natif Nano : allocations, réserve canonique, ratio, conversion/remboursement et confiance du bridge USDC ; implémenter et qualifier dépôts/retraits, finalité source, reorgs, rejeu, compromission et reprises idempotentes. Préparer les termes réels Doppler : identité, offre, inflation, timing, ticks/prix, frais, gouvernance, seuils, invendus et migration ; utiliser le SDK officiel et l’outil unsigned tools/doppler-launch pour simuler création, achats, ventes et graduation. Conserver le rejet de simulation synthétique sur Base comme échec, pas comme lancement qualifié. Vérifier séparément visibilité à T0 et indexation externe sans présumer de référencement, financement, liquidité, bridge automatique ou retrait des réserves Doppler. Ne pas confondre tests verts, binaire sur VPS, cotation et production prête. Aucun flood continu, fonds réels, vente, déploiement public, nouvelle machine ou dépense sans autorisation dédiée. Continuer de façon persistante ; ne déclarer l’objectif atteint que lorsque tous les critères de production sont démontrés et qu’aucun travail requis ne reste ouvert.

### État et règles de poursuite au présent contrôle

- L’objectif enregistré reste **actif**. L’outil a refusé sa recréation car un
  objectif inachevé existe déjà ; aucune clôture, pause ou réussite fictive n’a
  été utilisée pour contourner ce refus.
- Les lots Docker exits/reorg/pruning et les images retenues ont leurs sorties
  et limites dans les compléments précédents. Ils ne sont pas redémarrés pour
  satisfaire une ancienne formulation de l’objectif.
- La campagne privée03 est effectivement active : session38587 encore vivante,
  sans sortie terminale ni résultat global au contrôle du 17 octobre à19:44UTC.
  Sa première restauration a été suivie d’une finalité commune observée
  `23 → 27 → 28 → 29` sur les trois runtimes, avec trois observations stables.
  Ce résultat intermédiaire ne qualifie ni le cycle complet ni la production.
  Journaux, témoins, identités et autorisation existante restent conservés.
- Le lot RPC suivant doit résoudre ensemble la capture d’état bornée hors des
  longs verrous, l’annulation et le classement des lectures coûteuses, avec les
  quatre lecteurs et `eth_getProof`. Le constat source de coût linéaire reste
  ouvert ; la proposition intermédiaire c6fd n’a été ni compilée ni qualifiée.
  Les compteurs de tests ne constituent pas un objectif artificiel à conserver.
- La revue indépendante, la CI refusée avant exécution pour facturation/plafond,
  les autres écarts de consensus et les conditions économiques/bridge/Doppler
  restent ouverts dans leur périmètre. Le cycle privé Anvil réussi documenté
  précédemment ne clôt pas l’ancien rejet Base ni les conditions d’un lancement.

Les quatre documents et leurs anciens contenus restent intacts comme préfixes.
Aucun statut EIP/fork n’est promu par cette réaffirmation. Toute qualification
ultérieure doit citer ses révisions, ses sorties terminales et son périmètre.


## 17 octobre 2026 — tentative03 terminale FAILED, première admission des sorties corrigée

La [tentative03](qualification/fresh-lifecycle-third-attempt-20261017/README.md)
termine exit1 en4 382 212ms : guard422échantillons, aucun SIGINT, enfant joint.
Première restauration et finalité commune acquises partiellement ; sortie volontaire
acceptée et observation commune jusqu’à87. Le refus ctimeNs survient à la capture
neuve mac-native avant proposer slashing ; acteur inconnu, clôture refusée pour
stages incomplets. Trois captures terminales65paires/136rôles DB sont conservées,
sans signatures ; le fence logiciel terminal et le cycle complet restent ouverts.

La [correction de première admission](qualification/new-output-admission-20261017/README.md)
qualifie causalement l’ancien refus (1FAIL), cinq groupes injectés nouveaux, un
groupe existant d’autorisations et un fichier synthétique réel6,58Mo. Les garde-fous
SHA/identité restent stricts, sans récupération d’une ancienne autorité. Minimum2s
par nouveau save, budgets inchangés ; la tenue d’une campagne entière n’est pas
prouvée. Les refus de préparation/parser et tous les historiques restent intacts.

Aucune relance03, nouveau domaine, signature ou image n’est créée par cette
correction JS ; l’ancien domaine reste consommé. Aucun EIP/fork, bridge/Doppler,
revue indépendante, CI ou production n’est promu. Objectif actif.


## 17 octobre 2026 — garde d’intégrité des sources libp2p qualifiée

Les [preuves ciblées](qualification/libp2p-source-integrity-20261017/README.md)
conservent deux causalités négatives : refus manquant de main sur une source
modifiée, puis de V1 sur un répertoire vide non suivi. Le candidat V2 passe ses
huit groupes sans skip ; blobs, modes, liens symboliques et inventaire ignoré/non suivi sont
contrôlés autour des étapes du build, sans nettoyage et en conservant le premier
échec. Tests synthétiques seulement : aucun SDK ni image construit, aucun clone
externe ni trafic de nœud. Hunter/apt/toolchain, OCI/bit à bit, CI globale et production ne
sont pas qualifiés. La base de revue est main `e42288e6…` ; aucun EIP/fork, duty,
consensus ou campagne n’est promu. Les anciens contenus restent des préfixes
intacts. L’objectif intégral déjà présent reste actif ; sa recréation a été
refusée car un objectif inachevé existe, sans clôture ni réussite fictive.


### 17 octobre 2026 — observations ARM/runtime et vrai HTTP, échecs conservés

Un [lot public distinct](qualification/runtime-main-http-arm-20261017/README.md) documente huit assemblages et neuf arités/aide ARM du candidat c6cdf780, puis douze requêtes du vrai Nano/HTTP sur une DB genesis Shanghai synthétique, deux démarrages/TERM joints exit0 et une réouverture. La continuation hôte reste FAILED : son manifeste source a changé après admission. Un audit Root séparé en lecture seule vérifie 1 256 autres inputs, 33 produits et 537 argv, sans rejeu ; ce n’est pas une requalification du lot global. Le smoke HTTP reste également FAILED pour résolution d’ethers ; une continuation hors ligne vérifie ensuite le même vecteur HTTP, sans nouveau RPC. Les refus préparatoires et Q02 restent conservés. Aucune source C++ candidate n’est importée par cet append, aucun index/admission n’est retargeté. Native/AMD/cohérence globale, duties, finalité et production ne sont pas qualifiés par ce lot.


### 18 octobre 2026 — candidat RPC : Native acquis, préparations AMD distinctes

Le [lot Native public](qualification/rpc-state-proof-native-20261018/README.md) du candidat c6cdf780 atteste quatre tests C++, neuf contrôles d’arité/aide et 24 vecteurs de preuve vérifiés séparément par Node, sans rejeu des huit unités JS. Native04 reste FAILED pour autorité de lien manquante ; sa barrière finale et les deux refus préparatoires Root sont conservés. La continuation réussie ne répète aucun Core `-M`/CXX/ar ni lien de Nano : 10 nouveaux `-M`/CXX et 11 liens achèvent la cohorte de douze programmes. Le Core conserve 143 membres, 44 sélectionnés et 99 hors sélection inchangés ; neuf corps changent sur Native. AMD dispose seulement de la projection source 1 271 fichiers et du raccord de 143 recettes, sans nouvelle compilation. Main fa3 reste distinct du candidat C6 non importé. Les limites O(N), états orphelins, Paris/Shanghai et preuves non authentifiées par consensus demeurent ; aucune image, duties, finalité ou production n’est qualifiée, aucun index/admission retargeté. Les autres FAILED restent historiques ; tentative 03 FAILED consommée, aucune 04 autorisée.


### 18 octobre 2026 — clôtures terminales indépendantes, qualification ciblée de l'outillage

Le [lot public](qualification/terminal-software-fences-20261018/README.md) conserve quatre parses exit0, 14 nouveaux groupes et le seul groupe de garde modifié (trois refus avant seed/output/réseau), sans skip ; les cinq groupes d'admission inchangés ne sont pas rejoués. Les deux jonctions propres conditionnent les captures ; les contrôles sources/binaires, cleanup et clôtures finales sont indépendants, première erreur conservée. V1 refusée en revue source, premier consommateur, KeyError39e4 et projection01 non admise restent historiques ; projection02 distincte sur 50 fichiers publics, clôtures sources/Node inchangées. Aucun C++/SDK/Docker/SSH, duty, signature ou nouvelle finalité ; campagne03 FAILED consommée, aucune04 autorisée. Adapters réels, cycle complet, CI globale, revue indépendante et termes Base–Nano/bridge/Doppler restent ouverts. Aucun EIP/fork/profil ni production promu ; tous les anciens corps et statuts sont conservés.


### 18 octobre 2026 — candidat RPC AMD64 : deadline terminale, produits conservés

Le [lot AMD public](qualification/rpc-state-proof-amd-deadline-20261018/README.md) reste FAILED : dispatcher85431 exit1, budget worker1 800s épuisé au premier CXX proof (0,753s restant). Achevés :152 `-M`,52 CXX (44 Core+8 runtime),1 archive,8 liens/ldd ; zéro test/CLI et aucune vérification indépendante des24 preuves AMD. Archive143 membres,44 sélectionnés,99 hors sélection inchangés ;265 produits vérifiés sur leurs lignes originelles après cleanup, zéro changement. Entrées hôte inchangées, conteneur absent et image finale conformes n'annulent pas l'échec. ParseError préparatoire01, logs et six rapports restent conservés ; aucun C6 importé, runtime/global/production qualifié ni relance automatique. Les preuves antérieures gardent leur portée ; campagne03 FAILED consommée, aucune04 autorisée. CI globale, revue indépendante, consensus/opérations restant et termes Base–Nano/bridge/Doppler demeurent ouverts. Tous anciens corps/statuts EIP restent conservés.


### 18 octobre 2026 — RPC AMD64 : continuation et 24 preuves vérifiées

Le [lot public AMD](qualification/rpc-state-proof-amd-20261018/README.md) achève quatre tests C++, neuf contrôles d’arité/aide sur huit exécutables retenus, puis une vérification Node indépendante des 24 vecteurs effectifs. Les clôtures finales passent : 265 produits originaux et 16 nouveaux produits restent tenus après CLI/cleanup ; la continuation hors ligne conserve six lignes historiques et admet 72 nouvelles entrées aux octets connus, soit 78 fichiers clôturés. L’autorité JSON 866 conserve son contenu déjà publié ; sa propre métadonnée historique absente n’est pas inventée. La continuation ne recompile ni le Core ni les huit assemblages runtime ; les totaux bruts AMD restent 156 M, 56 CXX, une archive et 12 liens/ldd, incluant l’ancien scan interrompu.

Le FAILED initial pour délai, les deux collecteurs échoués et les sources préparatoires refusées sans exécution restent distincts et conservés. Le C++ compilé est C6 `c6cdf780…` ; main `4388f77b…` est la base documentaire auditée, pas un nouveau commit de publication inventé. Les preuves Native et ARM restent acquises dans leur périmètre exact. EIP-1186 est qualifiée localement sur Paris/Shanghai ; O(N), budgets logiques, annulation coopérative, I/O et absence d’états orphelins restent des limites. Les racines MPT sont engagées dans les headers d’exécution ; le vérificateur indépendant n’authentifie pas encore un checkpoint finalisé de consensus. Aucun profil ultérieur, HTTP du vrai main Native/AMD, SDK/CMake/loader globaux, duty, finalité réseau ou production n’est qualifié. Campagne 03 FAILED consommée, aucune 04 autorisée ; CI globale, revue indépendante, opérations/consensus et termes Base–Nano/bridge/Doppler restent ouverts. Tous les corps historiques et statuts EIP sont conservés.


<!-- nano-main-http-persistence-20261018 -->
### 18 octobre 2026 — main HTTP C6 et persistance PoW/Shanghai

Le main publié `bc99b0ec2cf8bc6bcb21a26ec980ef2c4e8573a8` intègre C6 `c6cdf780c68eab8b06306d76257bda9028928ea3`. Le scénario réel `main/serve` PoW synthétique / Shanghai passe en Native Mac (Root 61492, exit 0) et Docker AMD64 VPS (Root 10741, exit 0) : 57 appels HTTP, trois serveurs, deux CLI mine, deux réouvertures et trois TERM/join exit 0. Chaque environnement a cinq preuves HTTP, trois en-têtes et le hash/sender de la transaction existante vérifiés indépendamment ; Root AMD 69605 termine exit 0 et conserve des vecteurs identiques aux octets Native. Fence AMD host 644 entrées et fence de lecture 108 entrées PASS ; conteneur absent et image existante inchangée. Wallets vides, mode dev désactivé et mine RPC refusé.

Les [preuves publiques](qualification/main-http-persistence-20261018/README.md) conservent Native 55054 / oracle de casse et AMD 94771 / borne Docker comme FAILED. La lecture AMD01 Root 88897 passe seulement la collecte, avec `hostPassed=false`. Aucun C++, binaire, image, clé, signature ou devoir créé. Le producteur réseau `1925eeeabc9f7d0aef41f2c3fe43c11c31f1f352` reste séparé. Les racines ne sont pas authentifiées par Beacon ; consensus, finalité multi-nœuds, restauration/duties, critères restants et revue de sécurité indépendante demeurent ouverts. Tentative 03 consommée, aucune 04 ni vente/fonds/bridge public autorisés ; `productionQualified=false`. Livraison documentaire seulement, sans build d'image.


<!-- nano-capella-sync-adverse-local-20261018 -->
### 18 octobre 2026 — sync adverse locale qualifiée sur Docker ARM64 et AMD64

Le nouveau test C++ Capella passe sur Docker ARM64 (Root 51225, exit 0) et AMD64 (Root 18311, exit 0) : deux groupes et trois records par architecture, un seul TU compilé, un lien/contrôle du chargeur et une invocation chacun. Le Core retenu de 143 membres et les 989 sources de production comparées restent inchangés ; aucune image, archive Core ou ancienne suite C++ reconstruite. La nouvelle unité n’a pas été exécutée en Native Mac. Le scénario crée huit signatures de fixtures publiques déterministes, sans clé privée de campagne ni duty. Le service worker réel traite l’orphelin avant son parent puis son retry ; le second groupe vérifie le différé, la réouverture propriétaire/witness/réacteur, la session ancienne inconnue et le retry après accusé perdu. Trois exports et trois soumissions passent par le relais local ; à chaque exécution, engagements canoniques, témoins, archives signées et état d’exécution concordent après réouverture. Ces comparaisons P/W, snapshots et racines sont locales : les records ne prouvent aucune égalité des snapshots entre architectures ni finalité commune.

Six quanta sont observés sur chaque architecture ; pics RSS ARM/AMD : 31 244/31 772 Kio, temps du test : 1 589/5 733 ms. Leurs fences finales de 1 062/1 063 entrées et nettoyages passent ; les quatre nouveaux produits par architecture restent tenus et les conteneurs possédés sont absents. La lecture publique AMD conserve 35 fichiers / 46 057 octets, SSH exit 0, sans rejouer le test. Le garde CI modifié passe séparément 13/13 contrôles, sans skip, configuration CMake du projet ni exécution des 19 anciens tests C++. La sélection quotidienne passe de 19 à 20 noms ; son contrôle lie le nouveau `--self-test` au programme, budget et label exacts. Ni les 20 tests C++ tous exécutés, ni une CI GitHub globale verte ne sont revendiqués.

La préparation 01 (Root 63321) reste FAILED avant publication du contrôle ; ARM02 (Root 7144) reste FAILED sur l’enveloppe du second groupe. V3 corrige seulement l’appel de l’API dans le nouveau TU — chaîne JSON complète au lieu d’objet compact — sans changer le Core ni les assertions/oracles V2. Les anciens reçus ne sont pas réécrits. [Sources, résultats et limites](qualification/capella-sync-adverse-20261018/README.md).

Ce scénario utilise un transport injecté local : il ne qualifie ni le P2P/WAN, les partitions prolongées/asymétriques ou WAN A, ni une progression de finalité, un ancrage indépendant par consensus ou la reprise de duties. Le booléen finalisé inchangé conserve les deux assertions epoch/root et le seal exact ; aucune finalité nouvelle n’est annoncée. Aucune campagne04, production, vente, fonds, bridge ou qualification Doppler ; tous les autres critères et échecs historiques sont conservés. La revue de sécurité indépendante non réalisée reste ouverte. `productionQualified=false`.


<!-- nano-rpc-header-selection-20261018 -->
### 18 octobre 2026 — sélection RPC des headers, réutilisation ARM distincte et qualification AMD

Ce lot fournit les dix blobs C7 de sélection des headers, fondés sur le parent main `801d793f…` / C6. Un index Header dérivé est écrit/supprimé dans les quatre batches atomiques existants. La sélection historique, le fallback legacy borné et les contrôles Beacon passent dans le worker détaché, avant l’état pinné ; les audits globaux des corps/certificats restent inchangés.

ARM02 reste **FAILED dans son ensemble** : 26 sources non consommées changent de ctime, acteur inconnu. Les trois tests mécaniques ont néanmoins passé leurs oracles prospectifs : coût 47 contrôles/23 refus, lecteurs 805/179 et cinq groupes, preuves cinq groupes/63 refus/24 vecteurs. L’audit public distinct passe uniquement la réutilisation du Core ARM `65c91631…` (41 306 456 octets, 143 membres : 44 remplacés/99 inchangés), des 244 produits originaux et des entrées effectivement consommées ; zéro nouvelle compilation, lien ou test dans cet audit. Il ne transforme pas ARM02 en PASS global. Les 24 vecteurs actuels correspondent au contenu canonique déjà vérifié indépendamment, sans nouveau rejeu du vérificateur.

**AMD02 : PASS terminal, dispatcher exit 0 et collecte publique qualifiée.** Le worker a effectué 143 scans M Core et trois scans de tests, 44 compilations Core et trois unités, une mise à jour d’archive, trois liens, trois vérifications du chargeur et trois tests. Les oracles restent 47/23, 805/179 et cinq groupes/63 refus/24 vecteurs. Les 143 membres comprennent 44 remplacés et 99 inchangés ; clôture après nettoyage et image AMD exacte conservées. Les 24 vecteurs égalent le contenu canonique précédemment vérifié, sans nouveau vérificateur indépendant. Native C7 n’est pas qualifié par ce lot. Les signatures Beacon sont douze signatures de fixture publique, aucune duty opérateur. FullBeaconABA, ancrage de finalité indépendant, WAN/finalité commune et production restent ouverts. Main/C6 et le producteur réseau `1925eee…` demeurent distincts. Campagne03 FAILED consommée, aucune04, vente, fonds ou déploiement public autorisés. [Périmètre et preuves](qualification/rpc-header-selection-20261018/README.md). Tous les préfixes, échecs, critères EIP, CI/revue indépendante et termes Base–Nano/bridge/Doppler restent conservés.


## Qualification de cohorte physique C7 — 18 octobre 2026

Le nouvel outil `nano-physical-cohort-portability` produit et inspecte exclusivement deux cohortes publiques synthétiques : huit bases P/W/journal/témoin, cinq archives signées, clôtures froides, registre et opération BLS en attente. Les sources exactes du TU et du helper diagnostique ont été compilées pour Linux ARM64 et AMD64 avec le Core C7 conservé ; les règles de refus du helper restent inchangées. La cible CMake est raccordée par trois commandes revues ; sa génération/construction CMake complète et macOS natif restent à qualifier. Aucun nouveau CTest automatique ni changement du Core de production.

Le volume ARM persiste après remplacement du conteneur puis passe une lecture RO avec engagement externe. Son archive réelle de 5 160 960 octets (69 entrées, huit inventaires) a été copiée par SSH vers le VPS et importée dans de nouveaux fichiers 1000:1000. Le véritable ELF x86-64 AMD `824bd6a9…` (6 408 336 octets) authentifie cette copie : trois records exacts, huit DB, cinq archives, deux passes RO, zéro nouvelle signature, réparation, produce ou compilation ; les 69 identités et octets restent inchangés après nettoyage et sortie indépendante du conteneur à zéro. Les writers ARM et AMD locaux concordent sémantiquement après exclusion uniquement de leurs inventaires physiques. Les booléens `networkTransport:false` des records C++ décrivent leur lecture hors ligne, pas l’absence du transfert SSH externe effectivement réalisé.

[Sources et preuves publiques](qualification/physical-cohort-20261018/README.md). Les échecs ARM01–05, forensic/probe, native-volume01/02 et préparation AMD01 restent des échecs historiques ; aucune réussite bornée ne les requalifie. La copie inverse avec un vrai writer AMD, les copies adverses/antirollback, macOS natif, restauration opérateur mutable, duties et finalité commune avançante restent ouverts. Profil EVM/RPC actif et statuts EIP inchangés ; aucun fork supplémentaire qualifié. CI/facturation, reproductibilité, exploitation, revue de sécurité indépendante et Base–Nano/Doppler restent ouverts. Aucun fonds réel, vente, déploiement public, image reconstruite ou nouvelle campagne privée 04 ; le binaire réseau 1925eee demeure distinct des sources. Production non qualifiée.


## Copie physique inverse et refus adverses C7 — 18 octobre 2026

Le vrai writer AMD conservé a été exporté contre ses 69 identités/SHA originales et son engagement externe, puis son archive de 5 171 200 octets a été transférée vers le Mac. Une première admission des copies SCP a refusé leurs modes 0600 au lieu de 0400, avant Docker/C++ ; ce refus reste conservé. Seules ces copies neuves non encore admises ont été normalisées puis admises prospectivement, sans retransfert ni rafraîchissement d’une ancienne autorité. Import dans un nouveau volume Linux ARM (source 1000:1000, destination 501:20), lecture par l’ELF ARM conservé : PASS, CLI et State.ExitCode indépendamment zéro, huit DB, cinq archives, deux passes RO, RSS 22 660 KiB. Les 12 entrées hôte et 69 entrées invitées restent inchangées après nettoyage ; vues FD9 hôte et invité distinctes. Sept conteneurs retirés, volume conservé. Zéro compilation, produce, nouvelle signature ou image.

Quatre copies adverses AMD passent leurs refus exacts : rôle absent et octet altéré refusés par l’inventaire avant DB ; témoin signataire Beta mélangé à Alpha refusé par l’authentificateur Core du journal ; bit de compression de la signature d’ancre invalidé refusé par la vérification d’archive Core. Chaque CLI et State.ExitCode valent 1 attendu, sans OOM ni signature, avec diagnostic typé au stade check-alpha. Les deux manifestes négatifs sont engagés séparément hors montage et ne modifient que les champs déclarés ; les originaux, les quatre copies et les autorités demeurent intacts après suppression des conteneurs. Ce sont des contre-exemples d’authentification, pas une restauration avec les engagements d’un opérateur ni une preuve de rollback d’un ancien journal.

[Preuves et périmètre](qualification/physical-cohort-20261018/README.md). Les deux sens Linux et ces quatre refus bornés sont acquis pour les sources/binaires épinglés ; aucune portabilité générale revendiquée. Un vrai ancien journal et ses floors récents indépendants, macOS natif, génération/build CMake complet, restauration opérateur/duties/finalité commune restent ouverts. Tous les échecs historiques, critères EIP, CI/reproductibilité, supervision, revue indépendante et Base–Nano/bridge/Doppler sont conservés. Aucun nouveau fork activé, runtime réseau remplacé, fonds, vente, déploiement public ou campagne privée 04. Production non qualifiée ; objectif actif.


<!-- nano-real-physical-journal-rollback-20261018 -->
### 2026-10-18 — refus de vrais anciens journaux, qualification AMD seule

Le nouveau test C++ `b6760c6d…` passe sur AMD64 avec le Core C7 conservé : deux vrais couples primaire/témoin, 7 authentifications Core (4 positives, 3 refus précis), 8 ouvertures RO, une seule signature publique en préparation et zéro pendant les contrôles. Une compilation du TU, 482 dépendances M/MD concordantes, aucun nouveau Core ou image ; état terminal et audit passif indépendants conservés. Parent `14b4117c…` AMD PASS et Native FAILED restent distincts.

Le build CMake natif complet du parent est acquis ; le successeur n'est pas qualifié sur Mac. Ses trois préparations refusent avant compilation/C++ ; la dernière conserve un écart de métadonnées source (15 ctime de fichiers, 28 ancêtres consommés), sans changement de contenu ni inventaire. Les 3 334 fichiers sélectionnés restent exacts. Aucune ancienne autorité n'est rafraîchie. Cohorte native, reproductibilité globale et qualification FD9 de tout le répertoire source restent ouvertes. [Sources, résultats terminaux et échecs](qualification/real-journal-rollback-20261018/README.md).

Ce lot ne qualifie pas le rollback simultané de l'engagement extérieur, la restauration opérateur, les duties, la finalité commune, le WAN ou la production. Aucun statut EIP ni fork réseau n'est promu. Tous les critères, preuves et échecs antérieurs, la campagne privée03 FAILED consommée, la revue indépendante non réalisée et Base–Nano/bridge/Doppler restent conservés et ouverts selon leur périmètre. Objectif ACTIF, production et vente non qualifiées.


## Journaux physiques V3 — résultats distincts, objectif actif

Le lot [V3](qualification/real-journal-rollback-v3-20261018/README.md), préparé depuis main86fa881a et Core C7 source9692, remplace uniquement le TU de test b676 par2e5ef451/28801 ; synchronisations avant premières admissions des nouveaux états, anciens floors/Seals stricts inchangés. AMD64 contrôleur+CLI PASS (1TU/482MMD/7auth dont3refus/8RO/1signature publique) ; Docker ARM64 contrôleur FAILED. Native04produce/checkCLI0 et Native05journalCLI0 sont conservés séparément de leurs contrôleurs globaux FAILED lors des clôtures FD9 de porteurs d’engagement. Cause/acteur inconnus, aucune ancienne autorité rafraîchie ni rétention FD9 historique externe qualifiée. Aucune répétition des appels natifs déjà réussis pour forcer un résultat global. La tentative ARM01 a échoué avant compilation après des observations de sept propriétaires invités passant de 0:0 à 501:20 ; les 17 entrées hôte sont restées strictement intactes, cause et acteur inconnus. ARM02 prépare ses seuls nouveaux montages avant leur première admission, sans relâcher les anciennes autorités.

Tous les critères précédents sont maintenus : restauration opérateur/duties/finalité commune, consensus/réseau/WAN, profil EVM réellement activé, pruning/CI/reproductibilité/exploitation, revue indépendante non réalisée et Base–Nano/bridge/Doppler restent ouverts. Aucun nouveau Core/image, fonds réel, vente ou déploiement public ; publication de sources distincte du runtime réseau1925. Tentative privée03 FAILED consommée, aucune04 autorisée. Objectif ACTIF, production non qualifiée.


## Cohorte macOS → Linux AMD64 et journal ARM03 — qualifications bornées

Le lot [stockage natif](qualification/native-storage-20261018/README.md), parent main `ffe8cd99`, ne modifie aucun Core ni image. Journal V3 Docker ARM03 : contrôleur et audit indépendant PASS, 489 M/MD, un TU/une invocation, sept authentifications dont trois refus, huit RO et une signature publique ; anciens ARM01/02 FAILED conservés. La cohorte publique Native04 a été exportée sans API DB puis réellement transférée/importée sur le VPS : un seul check AMD64 RO, huit DB/cinq archives/deux passes/zéro nouvelle signature, mêmes tokens et racines que le manifeste macOS. CLI et State0, 21 entrées tenues et 69 entrées importées inchangées après nettoyage, audit passif Root indépendant PASS. Le reçu initial de transfert FAILED (tuple stat incluant atime) est conservé ; aucun échec historique réécrit. Native04/05 restent whole FAILED et leurs anciennes autorités ne sont pas rafraîchies.

Portée limitée aux sources/binaires/images et cohorte joints : ni portabilité universelle ou inverse vers macOS, ni restauration opérateur, reprise de duties, finalité commune ou résistance au recul simultané de toutes les autorités externes. Consensus/WAN/partitions/cycle/rotations/sync/sanctions/retraits, charge/ressources, profil EVM activé Paris/Shanghai, pruning/CI/reproductibilité/exploitation et revue indépendante non réalisée restent ouverts selon leurs critères intégraux. Défaut CLI status/audit RW identifié ; correctif/test encore non compilés, non intégrés ici. Base–Nano/bridge USDC/Doppler, réserve/conversion, termes et visibilité T0 restent distincts et ouverts. Sources publiées distinctes du runtime réseau1925 inchangé ; aucun Core/image, fonds, vente, déploiement public ou campagne privée04. Tentative privée03 FAILED consommée. Objectif ACTIF, production non qualifiée.


<!-- nano-cli-readonly-c8-20261018 -->
## C8 — lectures CLI et régressions bornées, objectif actif

Depuis main `467485c596679d80a6e47991053174fde1ac2942`, le lot C8 impose ReadOnly à status/audit et refuse les écritures avec tip, état canonique, compteurs, difficulté, mempool et curseur observés inchangés ; aucune DB ou liaison manquante n’est initialisée. ARM 64770 reste FAILED :36 produits/neuf M/MD/huit liens et chargeurs achevés, zéro RUN, puis anciens host/helper observés 0400 → 0644 à contenu identique. La reprise indépendante 31991 passe huit cas avec ces mêmes produits, sans recompilation : six groupes RO/onze refus/deux barrières d’écriture,18 vraies CLI (8 succès/10 refus/15 gardes),185 checks chain et régressions ciblées. L’audit passif conserve36 produits/19 fichiers du case/96 sources consommées/neuf logiciels et69 entrées publiques ; aucun ancien résultat ni row n’est requalifié. **AMD 9878 : EXIT 0 / PASS : huit cas, 36 produits relus, neuf M/MD, Core143/1/142 ; image existante, donnees synthetiques neuves, aucun ancien volume nomme. Voir la portee et les preuves ci-dessous..** [Portée et preuves](qualification/cli-readonly-20261018/README.md).

Le réemploi143/1/142 compile l’ancien header sémantiquement identique ; ce n’est ni le sélecteur143 M strict, ni un clean build/CMake/CI global ou une preuve de reproductibilité. Paris/Shanghai inchangé, aucun Cancun activé. Production, revue indépendante non réalisée, restauration opérateur/duties/finalité commune, consensus/WAN et autres critères intégraux restent ouverts. Sources et runtime réseau 1925 restent distincts ; privée 03 FAILED consommée, aucune 04. L’hypothèse d’un actif unique représenté sur Base et couvert en NANO reste un brouillon sans offre/ratio décidés ; adaptateur préémission, bridge, USDC/Doppler et visibilité T0 restent ouverts. Aucun fonds, vente, déploiement public ou image reconstruite. Tous les préfixes, preuves et échecs historiques sont conservés ; objectif ACTIF.


<!-- nano-cli-readonly-native-cmake-20261018 -->
## C8 — qualification native CMake et portabilité du helper de tests

Base du C++ de production : `a8eee9d6d3a35b8e14e8a7f7a6cd1b9e5ee2acd7`, inchangée par ce lot. Seul le helper de tests `tests/support/rocksdb.hpp` est adapté aux signatures Open DB** et unique_ptr avec `requires` (`a128ab10… /716`) ; aucun oracle, réglage RocksDB ou règle de consensus n’est modifié.

Native03 : génération CMake et compilation de onze cibles réussies, 143 objets Core neufs. Cinq CTests passent (chain185, block-auth, recovery-audit Paris/Shanghai, Beacon gate), deux échouent (API RO et bootstrap) : le lot entier reste FAILED. Native04 : mêmes binaires, nouveau TMPDIR privé sous `/private/tmp`, zéro build et zéro rejeu des cinq succès ; API6groupes/11refus/2barrières/5RO, bootstrap7records et probe18CLI (8succès/10refus/15fences) passent. Huit familles cumulées sont qualifiées séparément ; aucun PASS unique de sept CTests n’est inventé. Installation CMake : quatre exécutables, onze notices exactes ; `otool -L` sur quatre fichiers et `nano --help` installé sortie0. Pas de nano-p2p natif ni de documentation complète installée dans ce périmètre.

L’audit passif vérifie 161 unités compilées, 489 sources consommées, un fichier généré et 9713 lignes de sources/logiciels/produits conservées. L’absence de depfile assembly BLST et d’identité historique des autres headers externes reste explicite. Le helper passe sur RocksDB11.8.1 par les cas natifs auth/recovery et sur RocksDB8.9.1 par une probe Docker ARM indépendante : 3Open/2succès/1refus, Put1/Get2/cleanup. Le premier cleanup sur bind Mac reste FAILED ; la reprise du même binaire sur tmpfs Linux passe sans compiler ni construire d’image. Native01/02/03 et tous les anciens échecs restent conservés, sans attribution non démontrée des causes.

[Preuves, sorties terminales et limites](qualification/native-c8-cmake-20261018/README.md). Ces faits ne démontrent pas portabilité physique de DB, reproductibilité, CI globale, image de diffusion, restauration opérateur, reprise de duties ou progression de finalité commune. Le runtime réseau1925 n’est pas remplacé. Paris/Shanghai reste le profil activé ; revue de sécurité indépendante et autres critères normatifs restent ouverts. Campagne privée03 FAILED consommée ; aucune nouvelle campagne privée autorisée. L’économie Base–Nano reste un brouillon : offre/ratio, réserve, conversion, bridge USDC, termes Doppler et visibilité T0 ne sont pas décidés/qualifiés. Aucun fonds réel, vente, push d’image ou déploiement public. Objectif ACTIF, production non qualifiée.

## 18 octobre 2026 — candidate Docker complète C8, portée bornée

Deux images locales ARM64/Mac et AMD64/VPS sont assemblées sous la recette27b52941 avec cinq programmes, cinq hashes/ldd et onze notices vérifiés. Trois outils de maintenance par architecture sont liés au Core C8 conservé, sans rebuild du Core. Dix commandes d’arguments réelles passent dans les images (nano help0, quatre refus1 attendus par architecture), sous UID10001, sans réseau, données, clés ni duties ; cinq clôtures possédées sont vérifiées par architecture. Packaging33 et smoke injecté5 passent séparément. Le bug du stdout Docker absent renvoyant LF est corrigé sans reconstruire d’image ; tous les premiers lots FAILED restent conservés, ainsi que l’admission ARM docs ctime et le mauvais cwd Node. [Images, provenance, reçus terminaux et limites](qualification/complete-docker-c8-20261018/README.md).

Cela ferme l’écart de contenu/chargement des cinq programmes Docker dans ce périmètre. Cela ne qualifie ni une reprise opérateur ni une production réseau. Le runtime existant n’est pas remplacé ; Paris/Shanghai reste activé. Restauration complète, duties/finalité commune, autres écarts consensus/stockage, CI/reproductibilité et revue indépendante restent ouverts. Campagne03 FAILED consommée, aucune04 autorisée. Relation Base–Nano, ratio/réserve/conversion, bridge USDC réel et conditions Doppler/T0 restent non décidés/qualifiés. Aucun fonds réel, vente, publication d’image ou déploiement public ; objectif ACTIF.


<!-- nano-cold-c8-canonical-replay-20261018 -->
## C8 — copies complètes à froid et audit canonique sur trois environnements

Depuis main5923aede, seul le lecteur de qualification C++ passe à0e54222c/42508 ; le C++ de production a8eee9d6, les Core C8 et les deux images complètes restent inchangés. Docker ARM64/Mac lit une archive Linux AMD64 ; Docker AMD64/VPS lit l'archive Linux ARM64 ; Mac natif lit l'archive Linux AMD64. Les trois contrôleurs ont une sortie terminale0 et PASS (7862/91770/32580). Chaque appel check unique réalise deux passes RO : huit bases, cinq archives, mêmes engagements primaire/témoin et journaux/floors, ancre signée et rejeu canonique Core réel. Alpha rejoue trois blocs avec ancre, beta quatre ; transactions0, aucune nouvelle signature ou invocation produce. Le checkpoint extérieur antérieur à l'ancre est refusé exactement, sans le transformer en preuve positive de finalité historique. Pics RSS :23760/24884/21408Kio.

Les 69 entrées des copies neuves demeurent exactes à neuf métadonnées et SHA après admission puis fermeture. Docker utilise un seul volume neuf conservé par architecture, UID10001, et neuf conteneurs possédés retirés ; aucune image/Core reconstruite. Native : copie neuve501:20, APIs RO sans montage OS RO revendiqué, onze descripteurs originaux et69 entrées réobservés par l'audit passif Root. Ce cas borne la lecture Linux RocksDB8.9.1→macOS11.8.1 à cette cohorte ; aucun downgrade, compatibilité générale ou provenance complète de la fermeture dyld/SDK n'est déduit. Les refus initiaux de préparation/compilation/lien restent conservés ; Native02 réemploie son objet déjà compilé, sans recompiler pour refaire passer un succès. [Résultats, sources, reçus terminaux et limites](qualification/cold-c8-20261018/README.md).

Le Genesis de cette ancienne fixture ne roundtrip pas via le codec CLI ; le nano audit-capella installé n'est donc pas qualifié par ce lot et le codec n'est pas affaibli. Le succès canonique a externalFinalizedAncestorVerified=false et requiredFinalized=null ; aucune finalité commune avançante, reprise de duties ou restauration opérateur n'est qualifiée. Journaux Mac originaux manquants/NAS refusé, campagne03 FAILED consommée et aucune04 autorisée restent explicites. Le runtime réseau1925 reste inchangé. Tous les critères/préfixes et échecs historiques sont conservés : consensus/WAN/cycle/rotations/sync/sanctions/partitions/charge, pruning/CI/reproductibilité/exploitation et revue indépendante non réalisée restent ouverts selon leurs périmètres. Paris/Shanghai reste activé ; aucun statut EIP/fork n'est promu. Relation Base–Nano, offre/ratio/réserve/conversion, bridge USDC, Doppler/T0 restent un brouillon non adopté ou non qualifié. Aucun fonds réel, vente, dépense ou déploiement public ; objectif ACTIF, production non qualifiée.


<!-- nano-base-native-proposal-20261018 -->
## 18 octobre 2026 — relation Base/Nano, hypothèse de préparation

[Proposition de modèle d’actif](NANOBEES-BASE-ASSET.md) : représentation Base du
NANO natif, ratio 1 pour 1 proposé mais non adopté, préadossement de toute l’offre et
conversion par inventaire à spécifier/implémenter/qualifier. L’utilisateur a dit
ne pas savoir encore quel rôle donner au token Base ; ce n’est pas une décision
économique. Le gateway actuel à offre nulle n’est pas assoupli. Aucun nouveau
statut EIP, succès de bridge, droit aux réserves Doppler ou vente n’est déduit.
Tous les critères et échecs historiques demeurent ; objectif ACTIF, production
non qualifiée. Aucun changement de C++, image, runtime, clés ou duties.


<!-- nano-operator03-c8-offline-20261018 -->
## C8 — données Native réelles de la campagne 03, audit hors ligne

[Preuves et limites](qualification/operator03-c8-offline-20261018/README.md) :
le nano C8 installé passe l'audit canonique de 694 blocs et 2 transactions sur
une copie des 34 rôles DB conservés ; checkpoint indépendant 7 858 et ascendance
finalisée externe époque 87 vérifiés. La capture C8 retrouve les 16 paires de
journaux primaire/témoin et les tokens extérieurs inchangés. Deux self-tests,
trois refus d'arguments attendus et la capture terminent avec les sorties prévues.
Six consommateurs Native sont compilés/liés, sans recompiler Core ; 9 713 entrées
antérieures inchangées. Les 2 391 headers externes seulement observés après
compilation ne prouvent pas leur identité au moment de compiler. Les refus de
copie, dry-run et projection publique restent conservés ; aucun ancien succès
n'est rejoué pour les faire disparaître.

La quiescence des sources n'est pas qualifiée (34 LOCK ouverts en écriture par
un service système), sans cause des anciens échecs démontrée. La clé peer est
exclue : pas de restauration entière, de reprise de duties ou de finalité commune
avançante. Le captureur ne prouve ni signatures des archives witness, ni
ascendance historique des digests, ni rejeu canonique ; ce dernier appartient à
l'audit installé distinct. Fork choice historique et votes autonomes restent
hors preuve. La supervision Source03 reste non intégrée : 16 cas ciblés passent,
mais le parallèle Mac échoue avec SIGABRT Node, cause inconnue.

Campagne03 FAILED et accord consommé ; aucune04 autorisée. Runtime1925 inchangé,
Paris/Shanghai activé, aucun EIP/fork promu. Autres critères intégraux de
consensus, cycle des validateurs, exploitation, pruning, CI/facturation,
reproductibilité et revue indépendante non réalisée restent ouverts selon leur
périmètre. Proposition Base–Nano non adoptée ; bridge USDC, termes Doppler et T0
non qualifiés. Aucun fonds, vente, signature ou déploiement public. Préfixes et
échecs historiques conservés ; objectif ACTIF, production non qualifiée.


<!-- nano-lifecycle-known-new-admission-20261018 -->
## Pilote lifecycle — première admission des fichiers neufs

[Correctif et preuves bornées](qualification/lifecycle-new-file-admission-20261018/README.md) :
les créateurs EXCL JavaScript/Python engagent huit champs et SHA avant trois
captures full9 stables sur au moins deux secondes, sous trente secondes. Le
reader des anciennes entrées reste strict ; les chemins échoués ou déjà admis
ne peuvent pas être réadmis. Le reçu de sortie Python rejoint son admission.
Les pins historiques du driver guard sont conservés ; generation20261018
sélectionne explicitement deux nouveaux helpers, sans retargeter les anciens
plans, admissions ou résultats.

Sur Mac, sept groupes JS passent puis le groupe Python initial refuse le stderr
du launcher Apple. Le seul groupe Python est ensuite qualifié avec un
interpréteur explicite (16 scénarios mockés), sans rejouer les sept corps
inchangés. Le premier agrégat Root reste FAILED pour un compte SKIP attendu
erroné ; sa phase réelle sortie0/1PASS/0FAIL/0SKIP est auditée séparément, sans
rejeu. Six guards et seize cas runtime applicables passent ; deux cas Linux
restent sautés sur Mac. La compatibilité Linux reste ouverte à ce stade. CTest
ajouté sous réseau/Node, mais ni configuration CMake globale ni invocation via
CTest exécutées dans ce lot. Deux préparations refusées et tous les échecs sont
conservés. Aucun C++/Core/image/runtime remplacé, clé ou signature créée.

Cela ne prouve pas la cause de campagne03, sa reprise, le vrai cycle temporel ou
la production. Campagne03 FAILED consommée, aucune04 autorisée ; supervision
Source03 non intégrée, SIGABRT encore inexpliqué. Profil Paris/Shanghai inchangé,
aucun EIP/fork promu. Tous les critères normatifs de consensus, restauration,
duties/finalité commune, exploitation, CI/facturation, reproductibilité et revue
indépendante restent ouverts dans leur périmètre ; économie Base–Nano/bridge
USDC/Doppler/T0 non qualifiés. Aucun fonds, vente ou déploiement public. Préfixes
historiques intégraux conservés ; objectif ACTIF, production non qualifiée.


<!-- nano-lifecycle-known-new-linux-20261018 -->
## Pilote lifecycle — complément Linux sur le VPS

Après Git push/pull d75dd3d7, trois commandes Node sur le VPS terminent0 :
8 groupes NEW-file, 6 driver guards et 18 cas runtime, zéro FAIL/SKIP ; les deux
cas Linux et les 55 refus attendus sont couverts. Python3.12.3/Node18.19.1 déjà
présents, aucune installation. Les 106 entrées sources/logiciels/projection
restent inchangées et tous les groupes possédés sont fermés. Le reçu public
persisté est relu et son SHA confirmé ; aucun C++/Core/image/runtime reconstruit
ou redémarré. [Preuves et portée](qualification/lifecycle-new-file-admission-20261018/LINUX.md).

La qualification Linux de ces helpers complète le bilan Mac fractionné ; aucun
ancien test, refus, manifeste ou agrégat FAILED n'est effacé/requalifié. Cela ne
qualifie ni CMake/CTest exécuté, fermeture complète des runtimes, tenue du vrai
cycle, cohérence entière des opérateurs, ni reprise de duties/finalité commune.
Campagne03 FAILED consommée, aucune04 autorisée ; supervision Source03 non
intégrée, SIGABRT inexpliqué. Profil Paris/Shanghai inchangé, aucun EIP/fork ou
production promu. Tous les autres critères normatifs, CI/facturation, revue
indépendante non réalisée et économie Base–Nano/bridge USDC/Doppler/T0 restent
ouverts dans leur périmètre. Aucun fonds, vente, signature, réseau de chaîne ou
déploiement public. Préfixes historiques conservés ; objectif ACTIF.


<!-- nano-c8-docker-operator-leaves-20261018 -->
## C8 — outils opérateur Docker compilés sur les deux architectures

[Preuves et limites](qualification/operator-c8-docker-leaves-20261018/README.md) :
5 TU/liens ARM64 et 4 AMD64 contre les Core C8 existants terminent avec leurs
28/23 commandes attendues ; 425 entrées sélectionnées tenues par architecture,
9/10 anciens logiciels et les deux produits Core/nano conservés inchangés.
M avant compilation, MD égal après normalisation, sources Git exactes, ELF/ldd
vérifiés. Aucun Core/image reconstruit. Les deux self-tests non signants passent
(25 cas capture ; 3 groupes/26 refus lifecycle submit), ainsi que les refus
sans arguments prévus. Aucun vrai import, dépôt, audit DB ou duty par ce lot.
36 fichiers logiciels neufs sont conservés durablement avec hashes relus.

Les admissions entières des trois opérateurs restent ouvertes : jointure P2P
native, tenue SDK avant compilation des consommateurs natifs concernés, index
24 actifs et nouvelles admissions exactes. Les anciens refus/admissions restent
intacts ; aucune reproduction globale ou cause de campagne03 n'est déduite.
Campagne03 FAILED, accord consommé ; aucune04 autorisée. Runtime1925 et profil
Paris/Shanghai inchangés. Restauration entière, duties/finalité commune et autres
critères normatifs restent ouverts dans leur périmètre, dont supervision,
CI/facturation, reproductibilité et revue indépendante non réalisée. Base–Nano
reste une proposition non adoptée ; bridge USDC/Doppler/T0 non qualifiés. Aucun
fonds, vente, signature, réseau de chaîne ou déploiement public. Objectif ACTIF,
production non qualifiée ; préfixes historiques intégraux conservés.


<!-- nano-c8-native-consumers-p2p-20261018 -->
## C8 — outils natifs et provenance P2P

[Preuves et limites](qualification/operator-c8-native-consumers-20261018/README.md) :
six outils natifs et leur shim liés ; continuation08 conserve07FAILED et réutilise
ses deux compilations réussies. Les headers consommateurs sont tenus avant M/MD,
28 fichiers sont conservés et cinq vérifications passives terminent comme prévu.
La provenance P2P native rejoint81 pins courants sans recompilation/exécution.
Le SDK historique Core/fournisseurs et dyld global ne sont pas qualifiés ; index24
actifs/admissions entières restent ouverts. Copies Docker0400 à compléter par de
nouvelles copies exécutables. Aucun Core/image/runtime remplacé, signature ou
duty. Campagne03 FAILED consommée, aucune04 autorisée ; Paris/Shanghai inchangé,
aucun EIP/fork promu. Autres critères normatifs, revue indépendante non réalisée,
Base–Nano proposé et bridge USDC/Doppler/T0 restent ouverts. Objectif ACTIF,
production non qualifiée ; préfixes et tous les échecs historiques conservés.


<!-- nano-c8-executable-packaging-20261018 -->
## C8 — exécutables Docker et packaging macOS

[Preuves et limites](qualification/operator-c8-executable-packaging-20261018/README.md) :
neuf copies exécutables Docker neuves sont engagées ; les anciens refus ctime et
copies0400 restent conservés. CMake déclare le shim colocalisé et @loader_path ;
sept comparaisons de recettes, deux liens natifs et trois inspections Mach-O
passent, avec53 entrées tenues et groupes fermés. Aucun TU/Core/image reconstruit,
exporter exécuté ou duty repris. Index24/admissions entières, SDK/dyld global et
autres critères normatifs restent ouverts. Campagne03 FAILED consommée, aucune04
autorisée ; runtime1925, Paris/Shanghai et statuts EIP inchangés. Revue indépendante
non réalisée et Base–Nano proposé/bridge USDC/Doppler/T0 restent ouverts. Aucun
fonds, vente ou déploiement public ; préfixes intégraux conservés, objectif ACTIF.


<!-- nano-c8-software-guard-20261018 -->
## C8 — sélection des logiciels et garde du chargement natif

[Preuves et limites](qualification/operator-c8-software-guard-20261018/README.md) :
24 identités logicielles sont jointes aux anciennes lignes strictes (9 natifs,
8 ARM64,7 AMD64), plus le shim auxiliaire. Les deux jointures P2P Docker passent
avec175 entrées tenues chacune. Le contrôle pur des locators confirme24 actifs
et9 montages fichier supplémentaires, sans déplacement ni nouvelle copie.
Le nouveau format C8 vérifie le shim colocalisé et sa relation Mach-O, avant
effets, autour de l’exporter et dans les fences globaux ;13 tests ciblés passent.
Ancien format,24 rôles et13 champs d’admission conservés. Index d’identités établi ;
fermeture composée Core/main et trois admissions entières restent ouvertes,
sans qualification du SDK/dyld global. Aucun build Core/image, exporter réel,
signature, duty ou réseau de chaîne lancé. Campagne03 FAILED consommée, aucune04
autorisée ; runtime1925, Paris/Shanghai et statuts EIP inchangés. Autres critères
normatifs, revue indépendante non réalisée et CI/facturation restent ouverts.
Base–Nano reste proposé, non adopté ; bridge USDC/Doppler/T0 non qualifiés.
Aucun fonds, vente ou déploiement public ; objectif ACTIF, production non qualifiée.


<!-- nano-c8-core-main-join-20261018 -->
## C8 — raccord Core/main terminé sur les trois environnements

[Résultats et portée](qualification/operator-c8-core-main-join-20261018/README.md) : les collectes Native, ARM64 et AMD64 terminent0, avec519/1074/1075 fichiers tenus et clôturés. Les sources projet et recettes présentes se raccordent aux builds qualifiés ; aucun nouveau build, test C++, scan SDK ou image. Native conserve son build CMake143 ; Docker réutilise les preuves143/1/142 et l’ancien header à différence de commentaire fermée. Cela ferme le raccord Core/main sélectionné ; wrapper24/admissions C8 complètes, restauration/duties/finalité commune et tous les critères normatifs restent ouverts. Runtime1925 inchangé, Paris/Shanghai activé,03 FAILED/consommée et aucune04 autorisée. Base–Nano1:1 reste une proposition non adoptée ; réserve/conversion, bridge USDC/Doppler/T0 non qualifiés. CI/reproductibilité/exploitation et revue indépendante non réalisée restent ouvertes. Aucun fonds, vente, signature, nouvelle campagne ni déploiement public ; préfixes et échecs historiques conservés, objectif ACTIF.


<!-- nano-c8-final-software-admissions-20261018 -->
## C8 — assemblage complet des logiciels et admissions opérateur

[Les API réelles acceptent24 actifs et trois admissions](qualification/operator-c8-final-admissions-20261018/README.md) : neuf mounts supplémentaires, wrapper C8 exact, garde Native vérifiée sur exporter/shim,51 entrées clôturées et terminal0. Dix carriers neufs sont admis après quatre probes dont trois stables ; le refus01 avant API, ses changements ctime seuls et les propositions source NO_GO restent conservés. La jointure des sept TU natifs aux observations consommées originales ferme le raccord public manquant, sans nouveau build/SDK/objet. Cela ferme l’assemblage logiciel sélectionné, pas la restauration opérateur ni la progression de finalité commune. Avant04 restent la privatisation du listener Native, une échéance globale incluant clôture et les restaurations Native/Docker Mac en plus du VPS.03 FAILED/consommée, aucune04 autorisée ; aucun seed/clé/duty généré ici. Runtime1925 et Paris/Shanghai inchangés, aucun EIP promu. Tous critères consensus/WAN/partitions/cycle/retraits/sanctions/charge, CI/reproductibilité/exploitation et revue indépendante non réalisée restent ouverts selon leurs portées. Base–Nano1:1 reste proposé ; réserve/conversion, bridge USDC/Doppler/T0 non qualifiés. Aucun fonds, vente, dépense ou déploiement public ; préfixes et échecs historiques conservés, objectif ACTIF.


<!-- nano-c8-loopback-generator-qualification-20261018 -->
## Addendum du 18 octobre 2026 — écoute native et garde du pilote

Le [lot ciblé du générateur](qualification/operator-c8-native-loopback-20261018/README.md) configure le P2P natif frais sur127.0.0.1, les conteneurs sur0.0.0.0 et conserve le RPC local. Le test topologie passe dans AMD03, dont le contrôleur global reste FAILED (1passé/1échoué/70ignorés). Le garde des trois refus d’autorisation passe séparément dans AMD06 (exit0,1passé/5ignorés) sur la candidate Git41865010 ; les échecs de montage/outillage sont conservés. Aucun build d’image, nœud, seed ou signature. Ceci qualifie le générateur et les refus négatifs, pas le WAN réel ni une restauration. L’échéance globale avec clôture et la restauration entière Native/DockerMac restent à intégrer/qualifier ; les deux restaurations VPS existantes ne couvrent pas ces propriétaires. La campagne03 a signé puis échoué : accord consommé, aucune04 déduite. Paris/Shanghai reste activé, sans promotion EIP. La relation token Base–Nano reste à décider (1:1 seulement proposé), Doppler et revue indépendante restent ouverts. Les critères normatifs et tous les échecs historiques sont intégralement conservés.


<!-- nano-c8-owner-restores-global-budget-20261024 -->
## Addendum du 24 octobre 2026 — pilote de restauration et budget global C8

Le [lot C8](qualification/operator-c8-campaign-controls-20261024/README.md) implémente les restaurations entières Native puis DockerMac, conserve les deux restaurations VPS et leurs preuves distinctes, impose les engagements source avant reprise et réserve une clôture dans le budget monotone. Le défaut réel de groupes de processus a été corrigé par spawn direct. La candidate9a3eb5ea a réellement passé53 tests Docker AMD64/Node (exitDocker0, stderr0), dont copies de fichiers synthétiques Linux et processus OS. Les contrôleurs01/02 restent FAILED, respectivement pour défaut réel/timeout et compteur43+10ignorés face à53passés. Les53 noms sont vérifiés séparément contre Git et les clôtures ; aucun test rejoué pour le compteur, aucun ancien échec réécrit. Cela ne qualifie ni stockage RocksDB réel restauré, ni système de fichiers Native, ni duties/finalité commune effective des trois nœuds, ni borne murale globale de campagne. Ces preuves opérateur restent ouvertes. Aucun build d’image, nœud, signature ou nouvelle identité ; campagne03 signée/échouée, accord consommé, aucune04 autorisée. Paris/Shanghai reste activé sans promotion EIP. Consensus/WAN, exploitation, CI (dernier refus vérifié avant toute étape pour facturation/plafond), reproductibilité et revue de sécurité indépendante gardent leurs exigences intégrales. Base–Nano1:1 reste proposé/non adopté ; bridge USDC, Doppler, visibilitéT0 et termes économiques restent ouverts. Aucun fonds réel, vente, déploiement public ou dépense.


<!-- nano-c8-ownership-and-cold-software-20261024 -->
## Addendum du 24 octobre 2026 — suivi opérateur et refus logiciel à froid

Le [lot opérateur](qualification/operator-c8-ownership-cold-audit-20261024/README.md)
publie les intentions d’appartenance avant les commandes externes. La candidate
3f2a4a29 passe cinq groupes ciblés Docker AMD64 (0échec,10ignorés, sortie réelle0),
dont trois nouveaux groupes, le garde des onze refus et l’inventaire transitif.
194 fichiers tenus sont clôturés, le CID de test est absent ; aucun ancien test
rejoué sans lien, aucun build d’image ni nœud de chaîne. Mocks et ordre source ne
prouvent pas le fsync de ces nouveaux reçus, l’identité OS, la quiescence distante,
la borne murale ou les restaurations/duties/finalité des trois propriétaires.

L’audit froid strict est FAILED et conservé : les 24 logiciels plus le shim ont
leurs SHA/tailles connus, mais les 18 fichiers locaux ont un st_dev différent
(16777231→16777229), seul changement des neuf champs. Les sept VPS restent
full9+SHA conformes. Cause précise non démontrée, aucun ancien reçu actualisé ;
une génération distincte de carriers/admissions reste à établir avant campagne.
Docker Mac reste arrêté. Le budget de recette 7h39 et l’exemple8h/10min ne sont
ni une durée adoptée, ni une autorisation, ni une borne garantie de clôture.
03 a signé/échoué, accord consommé ; aucune04 autorisée. Paris/Shanghai demeure
activé sans promotion EIP. Tous critères normatifs consensus/WAN, exploitation,
CI/facturation, reproductibilité et revue indépendante non réalisée restent
ouverts selon leurs portées. Base–Nano1:1 reste proposé/non adopté ; bridge USDC,
Doppler et visibilitéT0 non qualifiés. Aucun fonds, vente ou déploiement public ;
préfixes historiques intégraux conservés, objectif actif et production non qualifiée.


<!-- nano-c8-g2-native-durable-cold-20261024 -->
## Addendum du 24 octobre 2026 — admissions G2 et copie Native durable

Le [lot G2/Native03](qualification/operator-c8-g2-native-cold-20261024/README.md) valide 24 actifs/trois admissions et une copie physique Native arrêtée avec audit canonique indépendant : 373 fichiers, 36 répertoires, 16 paires primaire/témoin et 694 blocs dont l’ancre. Contrôleur réel 0, quatre commandes C++ jointes à 0, vérification externe 0 au moins 287 s après la borne conservatrice de sortie ; neuf champs/SHA intacts, 834 descripteurs fermés. Création directe dans un dossier durable hors Documents, sans rebase ni nouvelle compilation. Documents01 reste FAILED après mutation des dates de 36 répertoires ; les 746 fichiers réguliers restent exacts et la sonde publique reproduit le phénomène, sans identifier l’acteur. Tous refus/NO_GO historiques sont conservés. Cela ferme ce cas Native hors ligne ; portabilité entre architectures, reprise effective des duties/finalité commune, grands historiques et stabilité continue restent hors preuve. Campagne03 signée/échouée, accord consommé ; aucune04 autorisée. Paris/Shanghai seul profil activé, aucun EIP promu. Les critères résiduels consensus/WAN, exploitation, CI facturation/plafond avant étapes, reproductibilité et revue indépendante non réalisée restent ouverts selon leurs portées. Base–Nano1:1 proposé/non adopté ; bridge USDC, Doppler et T0 non qualifiés. Aucun fonds, vente, signature, dépense ou déploiement public ; objectif actif, production non qualifiée.


<!-- nano-c8-runtime-startup-closeout-controls-20261024 -->
## Addendum du 24 octobre 2026 — contrôle de démarrage Docker et clôture C8

Le [lot runtime/supervision](qualification/operator-c8-runtime-controls-20261024/README.md) qualifie la candidate ed651411 dans l’image AMD64 existante : 13 groupes ciblés passent, 0échec,15ignorés, sorties Docker/contrôleur0 et31FD tenus fermés. Le vrai moteur démarre un sleep après create/inspection no/0, refuse always avant start et supprime les deux CID ; Root vérifie séparément leurs absences et celle du lecteur. Aucun Nano, clé, volume ou image créé, aucune reconstruction ; C++ et Paris/Shanghai inchangés, aucun EIP promu. Le pilote conserve le CID et vérifie restart=no ; le superviseur C8 relie budget/réserve/spec/bindings, reconnaît une clôture liée au PID et garde un verdict provisoire jusqu’au terminal externe. Les modèles/IO Linux et ces cas Docker ne prouvent ni9h réelles, fsync Darwin, fermeture des descendants après crash, ni duties/finalité commune ou restauration des trois propriétaires. Les refus source et préparations échouées restent conservés, aucun ancien résultat réécrit. La proposition04 de9h dont45min de clôture n’est pas adoptée ni autorisée ;03 a signé/échoué, accord consommé. Docker Mac reste arrêté, ancienne politique inconnue ; nouvelles liaisons/admissions et précontrôles obligatoires avant domaine frais. Les critères résiduels consensus/WAN, exploitation, CI (dernier refus vérifié avant étapes pour facturation/plafond), reproductibilité et revue indépendante non réalisée restent ouverts selon leurs portées. Base–Nano1:1 proposé/non adopté ; bridge USDC, Doppler/T0 non qualifiés. Aucun fonds, vente, dépense ou déploiement public ; objectif actif, production non qualifiée.


<!-- nano-c8-attempt04-human-consent-20261024 -->
### Accord distinct reçu pour la campagne04

Après les preuves et la proposition précédentes, l’utilisateur a [autorisé directement04](qualification/operator-c8-runtime-controls-20261024/campaign04-authorization-public.json) : neuf heures maximum clôture comprise,65 nouvelles identités de test dont33 clés au VPS, restaurations/duties/retraits/sanction contrôlée et démarrage contrôlé de Docker Desktop, avec arrêt de tout ancien conteneur de test relancé et journaux conservés. Cet accord remplace le statut «04 non autorisée» pour le travail futur ; les anciens constats restent datés et intacts. Aucun domaine04 ni aucune signature n’a encore été créé par ce reçu. Les précontrôles de sources/images/admissions/ressources et l’authentification du profil frais restent obligatoires avant duties.03 demeure FAILED/consommée. Aucun fonds, vente, déploiement public, flood, nouvelle machine ou dépense supplémentaire autorisé ; production non qualifiée, objectif actif.


<!-- nano-c8-campaign04-start-20261024 -->
## 24 octobre 2026 — campagne 04 engagée, résultat complet ouvert

Le [lot de préparation et premier avancement 04](qualification/operator-c8-campaign04-start-20261024/README.md) conserve le refus disque, l’échec initial avant réservation, le vérificateur d’archives 01 NO_GO et la copie de fixtures partielle refusée. Onze anciennes copies de sources sont conservées en archives privées vérifiées sur le VPS ; images, volumes, anciens domaines et journaux restent préservés. Les fixtures manquantes de l’exporteur ont été restaurées aux octets d’origine avec de nouvelles autorités physiques, sans modifier le binaire.

La même campagne 04 se poursuit avec les 65 identités autorisées et le même seed/registre, désormais réservé. Son budget a été réduit à 31 690 586 ms, dont 2 700 000 ms de clôture ; l’échéance initiale reste le 25 octobre à 05:09:03.990 UTC. Trois runtimes ont réellement démarré et partagé deux avancées de finalité (3 puis 4), trois lectures stables et le même bloc d’exécution finalisé. Les restaurations complètes, duties repris, cycle complet des validateurs et terminal externe restent à démontrer ; aucune qualification globale de réseau ou de production n’en découle.

Bindings G2 d234, code JavaScript testé ed651411 et sources de lancement f572653d conservent leurs périmètres distincts ; Paris/Shanghai inchangé, aucun EIP promu. Tous critères normatifs et échecs historiques sont conservés. Partitions/WAN, charge/ressources, grands historiques, exploitation, CI (dernier refus vérifié avant étapes pour facturation/plafond), reproductibilité et revue indépendante non réalisée restent ouverts selon leurs portées. Base–Nano 1:1 proposé/non adopté ; bridge USDC, Doppler et visibilité T0 non qualifiés. Aucun fonds, vente, déploiement public, flood, nouvelle machine ou dépense. Objectif actif.


<!-- nano-c8-campaign04-two-mac-recoveries-20261024 -->
## 24 octobre 2026 — deux reprises Mac observées dans la campagne 04

Le [lot des deux restaurations Mac](qualification/operator-c8-campaign04-mac-recovery-20261024/README.md) lie pour chaque propriétaire 16 paires de journaux/témoins et 34 bases en lecture seule à des engagements externes, avec copies complètes vérifiées avant reprise. La même clé a ensuite proposé un nouveau bloc réellement finalisé : Native slot 90 après tête 79 / journal 60 ; Docker Mac slot 232 après tête 177 / journal 160. Trois nouvelles avancées de finalité commune et le même bloc d’exécution finalisé suivent chaque restauration (époques 8→12 puis 20→24). Les observations Root excluent un simple rejeu d’une proposition présignée plus récente que la tête arrêtée.

Le cycle validateur et les deux restaurations VPS prévues continuent dans le même domaine 04 signé, sans re-export ni reset, sous budget réduit et borne initiale inchangée. Les dernières fences de sources originales, la clôture canonique, les sorties réelles et absences d’acteurs demeurent nécessaires avant qualification complète. Ce lot ne qualifie ni la reprise des anciennes données opérationnelles ni la production globale. C++/images et Paris/Shanghai inchangés ; aucun EIP promu. Tous critères et échecs précédents sont conservés, ainsi que les écarts résiduels consensus/WAN, charge, exploitation/CI/reproductibilité, revue indépendante non réalisée et Base–Nano/USDC/Doppler/T0. Aucun fonds, vente ou déploiement public ; objectif actif.


<!-- nano-c8-campaign04-terminal-failed-20261025 -->
## 25 octobre 2026 — campagne 04 terminée FAILED, reprises partielles conservées

La campagne 04 est terminée **FAILED**, avec sorties effectives du contrôleur et
du wrapper à 1. Ce constat remplace son état « en cours » dans les addenda datés
précédents ; les preuves acquises et tous les échecs restent conservés. Les deux
restaurations entières Mac ont repris une proposition réellement finalisée de
la même clé : Native slot 90 après tête 79/journal 60, Docker Mac slot 232 après
tête 177/journal 160. Le dépôt a enregistré puis activé le validateur d’index 64 ;
le registre compte 65 entrées. La première restauration entière VPS est suivie
de trois avancées de finalité commune (45, 49, 50 depuis 44), puis de trois lectures
stables et du même checkpoint sur les trois propriétaires.

Le critère suivant échoue : aucune nouvelle proposition finalisée de l’index 64
n’est observée dans les 480 000 ms (8 min) prévus après cette restauration VPS.
Son journal de proposition reste au slot 342, tandis que sa cible d’attestation
avance de 46 à 61. Cette activité ne remplace pas la proposition exigée et ne
prouve pas, seule, une attestation canonique finalisée. L’ancien message
`Bounded finalized lifecycle ancestry` regroupe le délai et la borne d’ascendance ;
il ne suffit pas à identifier une panne du consensus ou sa cause. Le cycle
complet, la seconde restauration VPS, les sorties/retraits et la sanction ne
sont donc pas qualifiés.

La clôture conserve 301 étapes, dont deux refusées : `lifecycle-closing` et
`audit-three-owner-restoration`. Les trois captures arrêtées publient 65 paires
primaire/témoin et 136 rôles de bases lus en lecture seule ; elles rapportent
le même checkpoint finalisé, époque 60,
`0xa2f2e649b94ecda019aa8e64ef6726090eeb983e275a7d50e63320da65c2ced7`.
Leur portée reste une capture avec engagements, sans audit terminal complet du
cycle. Les contrôles d’absence Root ne retrouvent aucun acteur/sampler Mac,
aucun conteneur acteur VPS actif ni aucun processus possédé du contrôleur,
du nœud natif ou du tunnel ; les volumes ne sont pas supprimés.

Les correctifs prospectifs du scanner ne changent pas ce verdict historique.
La candidate C++, les images et Paris/Shanghai gardent leurs périmètres ; aucun
EIP n’est promu. Tous les critères résiduels consensus/WAN, exploitation,
CI/reproductibilité et revue de sécurité indépendante non réalisée restent
ouverts. Base–Nano, bridge USDC, Doppler et visibilité T0 ne sont pas qualifiés ;
aucun ratio, fonds réel, vente, déploiement public ou nouvel accord de campagne
n’est créé par ce constat. Objectif actif, **production non qualifiée**.


<!-- nano-closed-inventory-and-finalized-scanner-20261025 -->
## 25 octobre 2026 — lots de conversion fermée et recherche de finalité livrés

Le [lot de conversion V2 fermé](qualification/closed-inventory-v2-20261025/README.md) apporte quatre contrats abstraits, un modèle indépendant et une revue unsigned Doppler. Sept groupes du modèle et six groupes offline passent séparément ;24 assertions Solidity passent sur Anvil privé/Shanghai,65 transactions minées,22 comparaisons comptables et65 débits de portefeuille vérifiés. L’échec initial EIP-170 du contrat de tests à27918octets reste conservé ; deux dispatchers à18919/16613octets gardent les15 corps d’assertions. Les limites170/3860, le compilateur et les quatre contrats V2 restent inchangés. Ce résultat concerne la référence EVM ; l’exécution V2 sur Nano, l’authentification cross-chain/finalité/reorgs, la réserve réelle, les termes économiques et le bridge USDC restent ouverts. Les lots2:3/2:7 sont des unités synthétiques, aucun ratio1:1 adopté. La simulation Base rejetée n’est pas promue ; création/achat/vente/graduation Doppler réels et visibilitéT0 restent non qualifiés.

Le [scanner de finalité](qualification/finalized-scanner-20261025/README.md) apporte dix groupes nouveaux et cinq gardes concernées réussies sur leurs mêmes octets. Le résultat est combiné :13 contrôles passent dans une invocation avec deux erreurs d’import de montage, puis les seules gardes concernées sont réparées/rejouées. Tous échecs de préparation/montage, sorties et skips restent conservés ; aucune invocation échouée n’est rebaptisée PASS. Ancre réelle avant sélection, cache à continuité prouvée, rafraîchissements roulants joints, délais absolus après await, validation fraîche de la clé et floor Mac numérique sont couverts. La génération de pins20261025 est explicite ; les anciennes restent intactes. Aucun nouveau runtime Nano ni domaine signé n’a été lancé. Le floor de journal VPS typé, un vrai resume, une fenêtre de proposition future déclarée et le cycle complet restent ouverts ;04 conserve FAILED et aucun accord05 n’est créé.

C++/CMake/binaires et images restent inchangés, Paris/Shanghai seul profil activé ; aucun EIP n’est promu. Les images existantes sont réutilisées sans build. Tous critères normatifs, preuves et échecs historiques sont conservés. Consensus/WAN, exploitation, CI/reproductibilité et revue indépendante non réalisée restent ouverts selon leurs portées. Aucun fonds réel, vente, déploiement public, flood, machine ou dépense. Objectif actif, production non qualifiée.


<!-- nano-vps-duty-floor-20261025 -->
## 25 octobre 2026 — seuil du journal VPS implémenté, reprise réelle encore ouverte

Le [lot de seuil de proposition VPS](qualification/vps-duty-floor-20261025/README.md) relie désormais la clé au manifeste/registre capturé, les snapshots aux tokens externes, et le seuil àmax(tête arrêtée,slot du journal). Les métadonnées source/copie/stop/manifeste sont vérifiées avant reprise ; des fingerprints full9 sont produits prospectivement dès capture-pin. Sept nouveaux groupes et trois gardes passent dans Docker AMD64 avec Python VPS existant montéRO, UID1000 réel, sorties0 et87FD fermés. Le dispatcher entier est exercé sur fichiers synthétiques ; les refus initiaux de montage restent conservés. Aucune image ni aucun domaine signé lancé. Ni quiescence globale de la base, ni nouvelle reprise réelle, ni qualification des capsules04 ne sont inférées ; une nouvelle admission/campagne autorisée reste nécessaire. La génération de source/pins est explicite, les précédentes et04FAILED restent intactes.

C++ et Paris/Shanghai inchangés, aucun EIP promu. Rejeu V2 sur EVM C++ réelle, cycle validateur complet, budget futur des propositions, autres critères consensus/WAN, exploitation/CI/reproductibilité et revue indépendante non réalisée restent ouverts selon leurs portées. Base–Nano/USDC/Doppler/T0 non qualifiés ; aucun terme économique, fonds réel, vente, déploiement public, flood, machine ou dépense. Tous critères et échecs historiques conservés ; objectif actif, production non qualifiée.


<!-- nano-inventory-v2-native-differential-20261025 -->
### 25 octobre 2026 — InventoryBridge V2 : exécution native limitée, divergence ouverte

Le lot fermé a réussi **24 cas Solidity, 65 transactions et 22 comparaisons d’oracle** dans le moteur C++ Shanghai en processus sur AMD64. Le cœur existant est réutilisé ; seul le programme de test a été compilé. Les sorties effectives Docker/C++ à zéro sont jointes à une adoption indépendante en lecture seule. L’échec initial du post-vérificateur, dû à un mauvais nom de fichier, reste conservé et n’a provoqué aucun rejeu.

Le rejeu Anvil des **mêmes transactions signées** reste **FAILED** : deux racines correspondent, puis la transaction 3 présente une racine d’état différente malgré le même statut, gas, hash, nonce et les mêmes soldes contrôlés. La cause reste indéterminée ; les transactions suivantes ne sont pas qualifiées par ce préfixe. [Preuves, révisions et limites](qualification/inventory-v2-native-raw-differential-20261025/README.md).

Aucune promotion d’EIP, conformité générale EVM/RPC, reprise de consensus ou qualification de production n’en découle. Les historiques d’échec sont conservés ; bridge/finalité source, réserve, allocations, ratio de conversion et termes Doppler restent ouverts, sans adoption implicite de 1:1 ni fonds réels.

Tous les critères normatifs antérieurs restent ouverts selon leur périmètre, dont le cycle complet de consensus, CI/reproductibilité et la revue de sécurité indépendante non réalisée. Objectif actif, production non qualifiée.


<!-- nano-inventory-storage-reference-and-worker-portability-20261025 -->
## 25 octobre 2026 — divergence de référence localisée, sélection VPS corrigée

Le [diagnostic de stockage](qualification/inventory-v2-storage-diagnostic-20261025/README.md) reproduit le seul préfixe de trois transactions concerné, lit14comptes/40slots et vérifie indépendamment les preuves Anvil. Les racines canoniques reconstruites concordent avec Nano. Pour les deux endpoints, ajouter une seule feuille zéro au slot8 reproduit exactement les engagements Anvil1.5.1 ; leurs preuves authentifient cette feuille. Ce résultat explique l'écart de référence observé sans modifier le cœur ni effacer le rejeu initial FAILED ; les62transactions suivantes et l'ancienne différence de324gas restent non qualifiées par ce diagnostic. Aucune conformité EVM/RPC générale ou promotion d'EIP n'est déduite.

Le [correctif de sélection du worker VPS](qualification/vps-duty-floor-portable-20261025/README.md) passe7/7groupes sous Docker Linux, UID/EUID1000 réels,87FD fermés, sorties0. Seul le groupe exigeant ce contexte devient un skip visible ailleurs ; le mode obligatoire refuse un mauvais contexte. Sur Mac, le refus UID501 est vérifié mais la suite reste PARTIAL_UNEXPLAINED, avec timeouts Python et erreur de post-validation conservés. La lane GitHub dédiée sous Node25 et la CI complète restent ouvertes ; le succès Docker Node18 ne les remplace pas.

Les quatre documents normatifs et toutes preuves/échecs antérieurs sont conservés. Aucun runtime de consensus ni nouveau domaine signé n'a été lancé ;04resteFAILED. Cycle validateur complet, budgets futurs/reprise, autres écarts consensus/WAN, exploitation/CI/reproductibilité et revue indépendante non réalisée restent ouverts selon leur périmètre, comme Base–Nano/USDC/Doppler/T0 et les termes économiques (aucun1:1 adopté). Aucun fonds réel, vente, déploiement public, flood, machine ou dépense. Objectif actif, production non qualifiée.


<!-- nano-ci-a80f2617-admission-refused-20261025 -->
Le [run GitHub40146021439](qualification/vps-duty-floor-portable-20261025/retained-public/github-a80f2617-terminal.json), déclenché par le push `a80f2617`, est refusé avant toute étape pour paiement récent échoué ou plafond de dépenses selon GitHub ; worker/operations/changes non démarrés, core/network skipped. Zéro test CI exécuté, aucune relance ni modification de facturation. Les succès Docker restent limités à leurs propres octets et environnements. Tous les critères résiduels, la revue indépendante non réalisée et les échecs historiques sont conservés ; objectif actif et production non qualifiée.

**25 octobre 2026 — complément d'exécution InventoryV2/Geth t8n.** Les 65 enveloppes signées retenues de Native sont comparées à l'outil standalone officiel Geth 1.16.7 sous contexte synthétique AMD64/Shanghai, chainId 31337, legacy, gasPrice 1/baseFee 0. Les 65 post-racines d'état, hash/statut/gaz/nonces et comptes de frais comparés sont exacts (207 551 638 gas), sans rejeu Native ni normalisation de l'allocation de référence. Les 66 commandes terminent exit0/close0 avec groupes joints ; 657 FD du pilote et 25 FD Root sont fermés. Cette preuve reste limitée à cette trace et à ses contrôles ; pré-racine initiale, logs/blooms, RPC/P2P, consensus/reprise et autres forks ne gagnent pas une qualification globale. Le FAILED Anvil et les 324 gas d'écart d'une autre trace restent conservés. Les conditions du bridge, du lancement, de l'audit indépendant et tous les critères non démontrés restent ouverts ; l'objectif de production demeure actif. Voir la [publication InventoryV2/Geth t8n](qualification/inventory-v2-geth-t8n-20261025/README.md) pour les entrées, le terminal et la portée.

**25 octobre 2026 — budget des propositions du cycle validateur.** Le coordinateur distingue F (finalité/opérations) et P (propositions), facture 10F+4P dans le cycle, 2×(R+F+2P) pour les Mac et F initial séparé, et refuse un budget insuffisant avant export et lancement. V1 garde P=F ; v2 exige une fenêtre et un profil déclarés. Les échéances absolues, contrôles après attente et limites d'horizon restent stricts. Le lot Linux initial conserve 57 PASS/1 FAIL sur 58 groupes ; le seul groupe de fixture corrigé passe ensuite avec neuf cas de délai, sept SKIP et aucun rejeu global des 58 groupes. Les modules de production sont identiques entre les deux candidates ; chaque exécution ferme ses 237 FD suivis. La réserve de clôture pour toutes les E/S et la reprise typée restent ouvertes. 04 demeure signée, terminale et FAILED ; aucune campagne 05, qualification BFT/production ou autorisation de lancement n'en découle. L'audit indépendant et les critères restants demeurent ouverts. Voir les [preuves et limites du budget](qualification/lifecycle-proposer-budget-20261025/README.md).

Le 25 octobre, la candidate ae0fee421b9154986a918c2580390388f3686c6d a complété
la comparaison bornée des reçus : 65 transactions Native, 18 contrôles par transaction
et 373 logs identiques à Geth 1.16.7 retenu, avec blooms/encodage/racines singleton.
Premier refus à zéro transaction conservé, Core inchangé et aucun rejeu Geth.
Cette preuve ne clôt ni les autres critères de production ni la revue indépendante.
Voir [reçus et portée](qualification/inventory-v2-receipts-20261025/README.md).

**25 octobre 2026 — racine initiale InventoryV2.** Une seule transition Geth vide retrouve la racine initiale Native `59b1d26c…`, avec les trois comptes de fixture inchangés et zéro gaz. Docker termine exit0 en 1,915 s ; la commande Geth termine exit0/close0, groupe joint, et les 21 FD suivis du pilote et 31 FD Root sont fermés. Aucun rejeu Native ou des 65 transactions, aucune compilation et aucun build d'image. Ce complément ferme uniquement la comparaison de la pré-racine initiale de cette trace synthétique Shanghai. Il ne qualifie pas les blocs minés, le réseau ou le consensus. Les échecs historiques, l'audit indépendant et tous les autres critères de production restent ouverts. Voir les [preuves de racine initiale](qualification/inventory-v2-initial-root-20261025/README.md).

**25 octobre 2026 — CI du lot livre 086c11ed.** GitHub run 40149566316 refuse changes, operations et vps-duty-worker avant toute etape pour echec recent de paiement ou plafond du compte ; core et network sont skipped. Aucun test CI execute, aucune relance manuelle ni depense. La CI complete demeure ouverte ; les preuves Docker locales ne la remplacent pas. Voir le [recu terminal CI](qualification/vps-duty-floor-portable-20261025/retained-public/ci-086c11ed/README.md).


---

### 25 octobre 2026 — annulation Core, clôture I/O et écart historique de gaz

Ce lot conserve les critères et preuves historiques. Sources C++ qualifiées : `e53e06c05f1befa7fa3aeb4970f22987c073d1e3` ; candidate finale de code/outillage : `21a92877e14c37f46d38eda9c8ea62687af801f5` (C++ inchangé depuis e53). Aucune nouvelle campagne réseau, identité de validation, activation de fork, transaction publique ou image Docker n’est créée par cette qualification.

| Sujet | Statut de ce lot | Limite conservée |
|---|---|---|
| Annulation du registre, sorties et sanctions | **Implémenté ; qualifié en composants ARM64 et AMD64** | 28 nouveaux cas, 8 192 lignes synthétiques ; dix invocations par architecture, dont huit résultats conservés et deux Capella complétés. Ce ne sont pas 8 192 nœuds ni une finalité entre trois hôtes. |
| I/O de clôture | **Implémenté ; 36 groupes ciblés qualifiés en deux périmètres** | 19 groupes aux octets inchangés réutilisés et17 scénarios de fixture commune rejoués. Aucune borne physique E/S/OS, clôture complète après SIGINT ou réserve suffisante n’est prouvée. |
| Écart historique de 324 unités de gaz Inventory V2 | **Expliqué hors ligne** | 65 hashes Anvil exacts retrouvés ;27 octets non nuls supplémentaires×12. Aucun rejeu EVM ni correction des frais Nano. Le différentiel historique de racines Anvil reste FAILED. |
| Mémoire du rejeu canonique | **Partiel ; plan Source seulement, nouvelle implémentation non commencée** | La fenêtre de records/SSZ, l’ascendance des dépôts et la RSS doivent encore être traitées/mesurées dans leur périmètre exact. |
| Profil EVM/RPC et EIP | **Statuts antérieurs conservés** | Aucun fork ou EIP n’est promu par des tests de maintenance consensus/outillage ; seules les règles réellement activées font foi. |

Le Core testé est O3. Le driver Capella ARM64 reste O3 ; seul le driver de test AMD64 a été compilé en O1 après son timeout O3 de240s, conservé comme FAILED. Les archives143 membres conservent134 payloads historiques et remplacent exactement neuf objets. La fermeture MMD de18 unités réellement compilées est vérifiée ; celle de tout le Core face aux nouveaux headers, un build propre complet et des binaires de nœud déployés ne sont pas revendiqués.

Les anciens contrôles FAILED restent conservés, dont admissions/refus de métadonnées Mac non attribués, entrées de fixture manquantes et timeout de compilation. La première qualification I/O reste FAILED31PASS/5FAIL/5SKIP ; les17 scénarios terminal ont ensuite tous passé après correction des seules dépendances de fixture. La campagne04 signée reste terminale et FAILED ; aucun domaine/journal/témoin n’est réinitialisé ni accord consommé de nouveau.

Restent ouverts : restauration complète de la candidate et reprise effective des duties avec finalité commune qui avance, cycle complet des validateurs/sanctions, partitions/asymétries/reconnexion et anomalie WAN dans leur périmètre exact, limites de mémoire/reprise/charge, cohérence des binaires et déploiement, CI refusée avant exécution pour compte/plafond et revue de sécurité indépendante. Les termes Base–Nano/Doppler, la réserve/ratio/conversion, le bridge USDC et la visibilité/indexation externe ne sont pas adoptés par ces tests. **La production n’est pas déclarée prête.** L’objectif continue ; aucune nouvelle campagne, vente, fonds réel, ouverture publique ou dépense n’est autorisée par cet addendum.

Preuves : [Core ARM64/AMD64](qualification/consensus-cancellation-20261025/README.md), [clôture I/O](qualification/closeout-io-20261025/README.md), [explication du gaz](qualification/inventory-v2-historical-gas-20261025/README.md), [plan Source du rejeu borné](qualification/canonical-audit-bounded-replay-plan-20261025/PLAN.source.md).
