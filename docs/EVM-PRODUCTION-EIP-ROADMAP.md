# EIP pour l’EVM et la production Nano

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


Sélection priorisée à partir des spécifications officielles, consultées le
24 septembre 2026. Cette liste complète le goal ; elle ne prétend ni épuiser
toutes les EIP d’un fork ni prouver leur implémentation actuelle dans Nano.

Une EVM compatible active un profil cohérent avec toutes ses dépendances,
pas une sélection libre d’opcodes. Les règles historiques encore pertinentes
font partie du socle ; les changements PoW historiques ne sont pas imposés
au consensus PoS de Nano. Pour chaque ligne : auditer l’existant, corriger les
écarts et rattacher des preuves plutôt que réimplémenter ce qui fonctionne.

## Socle transactions, sécurité et frais

| EIP | Rôle / validation |
| --- | --- |
| [2](https://eips.ethereum.org/EIPS/eip-2) | Signatures canoniques et échecs de création ; volet exécution applicable |
| [155](https://eips.ethereum.org/EIPS/eip-155) | Protection des transactions legacy par chain ID |
| [1344](https://eips.ethereum.org/EIPS/eip-1344) | CHAINID accessible aux contrats |
| [2718](https://eips.ethereum.org/EIPS/eip-2718) | Enveloppes de transactions typées |
| [2930](https://eips.ethereum.org/EIPS/eip-2930) | Listes d’accès |
| [1559](https://eips.ethereum.org/EIPS/eip-1559) | Base fee dynamique, brûlage et frais prioritaires |
| [3198](https://eips.ethereum.org/EIPS/eip-3198) | BASEFEE accessible aux contrats |

## Exécution déterministe et maîtrise du coût

| EIP | Rôle / validation |
| --- | --- |
| [140](https://eips.ethereum.org/EIPS/eip-140) | REVERT et annulation des effets |
| [150](https://eips.ethereum.org/EIPS/eip-150) | Tarification des opérations et transfert borné de gas aux appels |
| [161](https://eips.ethereum.org/EIPS/eip-161) | Traitement des comptes vides |
| [170](https://eips.ethereum.org/EIPS/eip-170) | Limite de taille du code déployé |
| [211](https://eips.ethereum.org/EIPS/eip-211) | Données de retour des appels |
| [214](https://eips.ethereum.org/EIPS/eip-214) | STATICCALL et interdiction des mutations |
| [1014](https://eips.ethereum.org/EIPS/eip-1014) | CREATE2 et déploiements déterministes |
| [2200](https://eips.ethereum.org/EIPS/eip-2200) | Comptabilité SSTORE |
| [2929](https://eips.ethereum.org/EIPS/eip-2929) | Accès chauds/froids |
| [3529](https://eips.ethereum.org/EIPS/eip-3529) | Réduction des remboursements de gas |
| [3860](https://eips.ethereum.org/EIPS/eip-3860) | Taille et coût de l’initcode |

## Compatibilité Solidity moderne et forks

| EIP | Rôle / validation |
| --- | --- |
| [3855](https://eips.ethereum.org/EIPS/eip-3855) | PUSH0, Shanghai |
| [1153](https://eips.ethereum.org/EIPS/eip-1153) | Stockage transitoire, Cancun |
| [5656](https://eips.ethereum.org/EIPS/eip-5656) | MCOPY, Cancun |
| [6780](https://eips.ethereum.org/EIPS/eip-6780) | Sémantique moderne de SELFDESTRUCT, Cancun |
| [7702](https://eips.ethereum.org/EIPS/eip-7702) | Délégation de code des comptes, Prague ; autorisations et rejeu à qualifier |

## Blobs et rollups : périmètre réseau complet

| EIP | Rôle / validation |
| --- | --- |
| [4844](https://eips.ethereum.org/EIPS/eip-4844) | Transactions blobs, KZG, disponibilité et rétention |
| [7594](https://eips.ethereum.org/EIPS/eip-7594) | PeerDAS : échantillonnage, custody et reconstruction |

## Applications : standards ERC distincts du moteur EVM

Qualifier ERC-20, ERC-721, ERC-1155 et ERC-1271 par contrats Solidity,
ainsi que signatures EIP-712, permis ERC-2612 et détection ERC-165.
Sources : https://ercs.ethereum.org/ et https://eips.ethereum.org/EIPS/eip-712.
Ces standards ne sont pas des opcodes à ajouter au moteur. Tests de contrats,
callbacks malveillants, permissions, signatures, rejeu et compatibilité RPC
sont nécessaires. Aucun standard ERC ne certifie la viabilité économique.

## Critères de production qui ne sont pas des EIP

Finalité et reprise, partitions, limites de ressources, durabilité, sécurité
RPC/P2P, builds reproductibles, gestion des clés et paramètres de lancement
restent nécessaires. EIP-1559 ne garantit ni prix constant ni absence de spam.
EIP-4844/PeerDAS sont pertinents pour les rollups mais ne sont pas obligatoires
pour toute L1 EVM et ne garantissent aucune baisse de prix chiffrée.

La compatibilité doit séparer : implémentation dans les runners, admission
réseau, activation du fork et validation de bout en bout. Les extensions
Draft/Review déjà demandées restent suivies dans VALIDATOR-EIP-ROADMAP.md
et ne deviennent pas des prérequis universels à toute blockchain de production.

## Compléments de couverture Cancun / Prague

Suivre explicitement EIP-7516, 2537, 2935, 7623, 7691 et la configuration
7840. Évaluer 7642 seulement pour une éventuelle compatibilité eth/69.
Référence de composition : https://eips.ethereum.org/EIPS/eip-7600.
Leur état réel est consigné dans EIP-IMPLEMENTATION-STATUS.md ; plusieurs
sont déjà présents dans le moteur sans activation réseau. Ajouter comme
critères le provisionnement versionné des contrats système et la couverture
RPC eth_createAccessList ; préserver le traitement existant des frais dynamiques.

### 2026-09-27 — Reprise de fermeture des catalogues Capella

Fermeture idempotente authentifiée et une seule reprise sur issue RPC inconnue :
19 scénarios de relais, registre réel avec quotas/expiration, et RPC HTTP local
passent. Aucun changement des règles EIP ou du consensus. L'ouverture dont la
réponse est perdue reste **partielle** : quota conservé jusqu'au TTL ; clôture
requise par identifiant de requête et tests de réordonnancement/annulation.
Pas de qualification production ni de déploiement VPS pour cette tranche.
[Preuves et limites](qualification/capella-catalog-close-20260927/README.md).

### 2026-09-27 — Ouverture de catalogue adressable et annulation préalable

Le démon Capella utilise maintenant un identifiant de requête et une génération
serveur pour libérer une ouverture même sans son accusé. Annulation avant
ouverture, doublons, conflit de contenu, expiration, changement de génération
et redémarrage sont qualifiés localement. Historique des annulations borné par
rotation ; les handles vivants restent consultables et libérables.
20 scénarios de relais, registre réel, RPC et rattrapage à deux processus passent.
L'ancienne ouverture sans identifiant reste disponible et dépend toujours de son
handle/TTL ; le démon ne l'utilise plus. Serveur inaccessible : nettoyage au TTL.
Les effets de rotation sous forte concurrence restent à mesurer. Aucun nouvel
EIP déclaré conforme par cette modification de transport.
[Protocole, preuves et limites](qualification/capella-catalog-acquire-20260927/README.md).

Régression distribuée supplémentaire : deux nœuds locaux × 32 validateurs,
époque 2 finalisée sur une racine commune, trois observations stables.
2 863 relais réussis, 8 échecs fermés ; aucune qualification WAN supplémentaire.

### 2026-09-27 — Concurrence des catalogues : quotas validés, équité ouverte

Nouveau test natif : 32 threads × 32 vagues, budgets séparés de handles/mémoire,
32 ouvertures identiques simultanées et 128 courses ouverture/annulation.
Quotas, absence de résurrection, récupération sans TTL et checkpoint inchangé
validés en 5,92 s. Deux threads sur 32 n'obtiennent aucun accès pendant les vagues
avec capacité 2 : **équité sous contention non démontrée**. Tous récupèrent
ensuite un accès en exécution sérialisée. Ce test du registre ne qualifie pas
l'ordonnancement tournant des pairs du démon, ni un réseau de 32 nœuds.
Aucune nouvelle conformité EIP ni disponibilité production déclarée.
[Mesures, preuve et prochain critère](qualification/capella-catalog-concurrency-20260927/README.md).

### 2026-09-27 — Nettoyage Capella après disparition d'un pair

Défaut de raccordement corrigé : le filtre du démon refusait la nouvelle
libération de catalogue après invalidation du pair. Le filtre partagé autorise
maintenant le nettoyage authentifié, tout en bloquant les nouveaux travaux.
Régression avec filtre/registre/RPC réels : ouverture allouée, pair invalidé,
réponse perdue, libération et quota immédiatement réutilisable. Test HTTP réussi
en 22,78 s ; rattrapage réel à deux processus et reprise après perte d'ACK réussis.
L'équité entre plusieurs pairs concurrents reste à qualifier. Aucun EIP ni
critère de production globale clôturé par cette correction de transport.
[Preuves et limites](qualification/capella-peer-cleanup-20260927/README.md).

### 2026-09-27 — Six nœuds Capella, concurrence et pair non réactif

Qualification du démon réel : un source et cinq followers indépendants via
libp2p. Après un premier relais, le démon d'un follower est suspendu ; pendant
40,10 s, les quatre autres avancent et reçoivent des tours pour les trois
catégories. Tentatives terminées : [5,98,96,96,99], pair suspendu en premier.
Après reprise, les six têtes et racines d'état correspondent. Arrêt propre.
Ce résultat borne un cas d'équité réseau locale ; ni équité générale, ni finalité
avec stake distribué, ni qualification WAN. Le premier essai moins strict est
conservé séparément. Aucun changement du moteur ni nouveau statut EIP.
[Preuves et périmètre](qualification/capella-competing-peers-20260927/README.md).

### 2026-09-27 — Sélection locale des changements BLS de retrait

Nouvelle API du constructeur Capella : candidats signés, budget d'essais,
réservation de place avant transactions, validation complète par ajout. Cas
signature invalide/valide/doublon, limite d'essais, annulation, égalité SSZ avec
construction indépendante, signature et publication testés. Le retrait vers la
nouvelle adresse survient lors d'un balayage ultérieur, jamais dans le bloc du
changement ; deux retraits observés, test final 5,47 s.
**Partiel** : file durable, RPC/gossip et raccordement à `propose_current` restent
à faire ; le scheduler ne fournit pas encore ces opérations. Aucune activation
publique ni clôture complète EIP-4895.
[Code, référence Capella et preuves](qualification/capella-local-bls-changes-20260927/README.md).

### 2026-09-27 — File durable BLS et propositions Capella

La file des changements de credentials BLS est persistée avec le checkpoint,
authentifiée à la reprise, bornée par la configuration et consultée par les
constructeurs ordinaires et `propose_current`. Doublons idempotents, cible
concurrente refusée, admission sans mutation des credentials ; suppression après
finalisation. Qualification : 3 tests de transition/sélection/file passent ;
le nouveau parcours `propose_current` avec deux bases et mempools indépendants
produit 40 blocs et 272 votes, finalise l'époque 3, reprend les deux bases et
64 journaux de signature, verse le retrait et retire la file finalisée (34,56 s).
**Partiel** : entrée RPC/P2P et relais, réorganisations de cette file, pruning de
son contexte d'admission, saturation et crash pendant admission restent à
qualifier. Le test est local, sans transport ni activation publique ; il ne
clôture pas EIP-4895 ou l'objectif production.
[Code et preuves](qualification/capella-queued-bls-changes-20260927/README.md).

### 2026-09-27 — Admission BLS Capella par message et RPC privé

Message `bls_to_execution_change` de 172 octets, activé uniquement sous Capella,
acheminé par la file bornée vers la validation cryptographique et le checkpoint.
Tests : admission native et régression wire (2/2), dispatcher IPC (9 scénarios),
RPC HTTP local réel avec refus du mauvais jeton et admission durable autorisée
(17,17 s). Le daemon compile avec le nouveau type. **Partiel** : export/catalogue
et relais automatique restent à implémenter ; trajet libp2p réel, réorganisations
et pannes de cette file restent à qualifier. Aucun déploiement public.
[Preuves et portée exacte](qualification/capella-bls-admission-20260927/README.md).

### 2026-09-27 — Catalogue BLS et relais libp2p réel

Inventaire trié des changements applicables, catalogue stable et export signé
bornés ; rotation de quatre catégories par pair et contrôle du hash avant relais.
Tests : catalogue natif (7,14 s), 22 scénarios de relais (0,35 s), puis deux vrais
nœuds/daemons libp2p locaux. Une demande déposée uniquement sur le nœud sans
validateur atteint le producteur, est incluse et permet un retrait constaté sur
les deux nœuds. Rattrapage d'un état neuf sous la même identité et récupération
d'une réponse perdue passent aussi. **Partiel** : finalité avec stake distribué,
WAN, réorganisations de la file, saturation, pruning et panne pendant admission
restent hors de cette preuve. Aucun déploiement public.
[Rapport et artefacts](qualification/capella-bls-relay-20260927/README.md).

### 2026-09-27 — Réorganisation et capacité de la file BLS

Nouveau scénario natif (5,90 s) : deux changements de branche par votes signés,
restauration du changement BLS abandonné dans les candidats/catalogue et dans une
proposition valide, deux reconstructions depuis checkpoint, conservation de
l'archive non finalisée. Une file configurée à une entrée refuse une seconde
demande valide sans mutation du checkpoint et conserve l'idempotence du doublon.
Aucun correctif de production nécessaire pour ces cas. **Partiel** : réorganisations
profondes, pruning du contexte, admissions concurrentes et pannes d'écriture
restent à qualifier ; pas de nouvelle preuve réseau ni paiement après réinclusion.
[Scénario, limites et preuves](qualification/capella-bls-reorg-20260927/README.md).

### 2026-09-27 — Erreur de synchronisation pendant admission BLS

Nouveau test macOS (6,21 s), quatre cas : erreur EIO sur WAL principal ou témoin,
avec conservation ou suppression contrôlée du suffixe écrit. Fermeture du réacteur
et des lectures owner/RPC, reprise cohérente, état de consensus inchangé et
resoumission idempotente vérifiés. La demande revient via le témoin lorsqu'il la
conserve ; si le témoin perd sa nouvelle écriture avant publication principale,
la demande doit rester absente jusqu'à resoumission. Aucun correctif du moteur.
**Partiel** : ce n'est pas une coupure physique ni une qualification Linux/VPS ;
pruning, réorganisations profondes et charge restent ouverts.
[Preuves, essai initial et portée](qualification/capella-bls-sync-20260927/README.md).

### 2026-09-27 — Primitive d’élagage Capella et identité réseau stable

Séparation de l’ancre réseau initiale et de l’ancre de stockage, snapshot v2 pour
les états élagués et conservation du format v1 existant. Le candidat pur réduit
41 états de blocs à 18 (1 893 314 → 873 602 octets sérialisés), sans changer la
finalité, les watermarks de votes, l’identité réseau après réouverture ni le
résultat du bloc signé suivant. Test final : 32,88 s ; régression de transition
signée également passée lors du premier essai.
**Partiel** : pas d’élagage automatique en fonctionnement ni de récupération
d’espace RocksDB démontrée. Restent la publication durable, la réconciliation
des archives et opérations en attente, la conservation du contexte des dépôts
natifs, puis les tests prolongés et de panne pendant l’élagage. Aucun EIP clos.
[Code, empreintes et résultats](qualification/capella-pruning-primitive-20260927/README.md).

### 2026-09-27 — Publication durable de l’élagage Capella

API locale `prune_finalized` raccordée au checkpoint du réacteur/owner. Avant
écriture, le candidat doit correspondre exactement à la transformation qualifiée,
puis son enveloppe complète doit passer la validation de reprise (votes, preuves,
changements BLS). Test étendu passé en 34,25 s : annulation sans mutation,
écriture unique, doublon sans écriture, fermeture/réouverture réelle de RocksDB,
votes et identité conservés, même bloc et racine d’état au bloc suivant.
**Partiel** : scheduling automatique, contexte des dépôts natifs, scénarios avec
changements BLS/preuves en attente et pannes pendant l’élagage restent ouverts.
Pas de récupération d’espace disque ni de production démontrée par ce seul test.
[Preuves et portée](qualification/capella-durable-pruning-20260927/README.md).

### 2026-09-27 — Demande BLS en attente lors de l’élagage

Nouveau scénario passé en 34,06 s : le nœud suiveur conserve une demande signée
pendant 40 blocs ; son état d’admission est ensuite supprimé par l’élagage.
Après fermeture/réouverture de RocksDB, les octets signés sont identiques et la
racine d’admission désigne l’état finalisé conservé. Le doublon ne réécrit rien.
Le bloc suivant applique les credentials attendus et donne les mêmes états sur
les stores élagué et non élagué. L’archive non finalisée reste présente.
Aucun correctif du moteur requis. **Partiel** : paiement ultérieur, réorganisation
après élagage, preuves de slashing, dépôts natifs, pannes et déclenchement
automatique restent à qualifier dans ce contexte.
[Scénario et preuves](qualification/capella-pruning-bls-20260927/README.md).

### 2026-09-27 — Préfixes authentifiés d’historique des dépôts

Nouvelle primitive C++ reconstruisant un ancien préfixe de l’arbre des dépôts,
avec partage des sous-arbres et au plus 32 nouveaux nœuds. La racine et le compte
sont vérifiés ; le tuple historique hash d’exécution/compte/racine doit être
préalablement authentifié par l’appelant. Test passé en 0,39 s : quatre préfixes,
six preuves identiques, rejet d’une autre branche et de commitments altérés,
annulation sans mutation. Aucun nouveau profil réseau activé.
**Partiel** : l’élagage natif reste refusé. Il faut conserver les références
d’ancêtres et de périodes, puis raccorder ces préfixes à la sélection des votes
et aux preuves de dépôts obligatoires sans modifier les règles d’admission.
[Code, justification et preuves](qualification/deposit-history-prefix-20260927/README.md).

### 2026-09-27 — Références des dépôts antérieurs à l’ancre Capella

Élagage natif raccordé à la conservation des en-têtes et engagements de dépôts
historiques, sans leurs états EVM/Beacon complets. Lookup des anciens votes,
sélection de période et preuves obligatoires utilisent ces références et les
préfixes authentifiés. Format local v3 ajouté ; v1/v2 conservés.
Tests passés : dix votes de période comparés, preuve obligatoire identique après
deux élagages, rejet d’ancêtres altérés/manquants et d’une branche étrangère ;
régression Capella BLS/élagage également verte (34,02 s).
**Partiel** : le test natif utilise une ascendance Beacon synthétique sur des
blocs d’exécution réellement exécutés. La transition Capella native complète,
la reprise v3, les nouveaux dépôts après élagage et les pannes restent à qualifier.
Les métadonnées d’ancêtres croissent encore avec l’historique ; aucun stockage
indéfiniment borné ni déclenchement automatique n’est démontré.
[Preuves et portée exacte](qualification/capella-native-deposit-pins-20260927/README.md).

### 2026-09-27 — Reprise native Capella du checkpoint v3

Nouveau scénario natif passé en 41,85 s : contrat de dépôt présent au genesis,
40 blocs signés, 272 votes, finalité époque 3, élagage puis vraie réouverture
RocksDB/owner. Le checkpoint passe de 2 528 208 à 1 191 779 octets (41 → 18 états).
Trois snapshots altérés sont refusés malgré recalcul du digest externe.
Un nouveau dépôt signé après élagage produit le même historique et le même état
sur les stores élagué/non élagué, puis survit à une seconde réouverture.
**Partiel** : le registre initial contient 64 validateurs synthétiques ; le test
n’attend pas la consommation du nouveau dépôt par une période de vote complète.
Activation/top-up, pannes pendant élagage, preuves de slashing, scheduling et
croissance prolongée restent ouverts. Aucun espace disque récupéré mesuré.
[Scénario, empreintes et résultat](qualification/capella-native-v3-restart-20260927/README.md).

### 2026-09-27 — Élagage automatique Capella

Le runtime active désormais l’élagage aux ticks ; option locale booléenne
`automaticPruning` (défaut true). Le réacteur attend le contexte d’époque requis
et diffère si un vote signé référence un bloc qui serait supprimé. La publication
utilise la validation complète de reprise et le checkpoint durable existants.
Test natif passé en 67,87 s : 72 blocs, 528 votes, finalité époque 7, six mouvements
d’ancre, maximum 31 états retenus puis 18 finaux ; réouverture des deux owners.
La régression de configuration/runtime passe aussi en 21,75 s.
**Partiel** : ce parcours ne qualifie pas encore le report sur vote de branche
concurrente, les preuves de slashing ou les pannes entre tick et élagage. Les
55 références de dépôts conservées, l’historique EVM et les archives croissent
encore ; aucune borne globale du disque ni qualification WAN/production.
[Implémentation et mesures](qualification/capella-automatic-pruning-20260927/README.md).

### 2026-09-27 — Erreurs de synchronisation pendant l’élagage natif

Nouveau test macOS passé en 67,06 s, quatre cas : erreur EIO du WAL principal
ou témoin, dernière écriture conservée ou supprimée dans une base jetable.
Réacteur/owner/RPC indisponibles après l’erreur ; reprise au checkpoint élagué
si le témoin le conserve, sinon ancien checkpoint jusqu’à la nouvelle tentative.
Votes signés et état EVM préservés, nouvelle tentative idempotente, réouverture
et lecture RPC après reprise réussies. Aucun correctif du moteur nécessaire.
**Partiel** : pas de coupure physique, de qualification Linux/VPS ni d’injection
entre le commit du tick et l’élagage automatique ; archives BLS/slashing en attente
et branches concurrentes restent à qualifier dans ce scénario.
[Quatre cas et preuves](qualification/capella-pruning-sync-20260927/README.md).

### 2026-09-27 — Preuve de slashing après suppression de son ancien état

Nouveau test natif passé en 41,98 s : preuve BLS de double vote conservée malgré
la suppression de l’ancien état référencé, checkpoint v3 et réouverture RocksDB.
L’index équivoque et les octets signés restent présents ; resoumission inversée
sans nouvelle écriture. Le bloc suivant applique le flag `slashed` et produit
les mêmes états Beacon/EVM sur les stores élagué et non élagué. La preuve reste
archivée tant que son inclusion n’est pas finalisée. Aucun correctif moteur requis.
**Partiel** : preuve livrée localement avec clés synthétiques ; pas de qualification
de partition, réorganisation de l’inclusion ou retrait final de l’archive.
[Scénario et preuves](qualification/capella-pruning-slashing-20260927/README.md).

### 2026-09-27 — Retrait des preuves de slashing après finalisation

Publication corrigée : une preuve est retirée seulement si tous les validateurs
qu’elle incrimine sont marqués slashed dans l’état finalisé. Une inclusion sur
la tête non finalisée ne suffit pas ; les indices équivoques restent conservés.
Nouveau test passé en 69,18 s : 72 blocs, 519 votes, finalité époque 7, six élagages,
conservation avant finalité puis retrait, resoumission sans recréer l’archive et
réouverture des deux owners. Régression de preuve non finalisée également verte.
**Partiel** : un seul validateur fautif testé ; preuves multi-validateur, sanctions
partiellement finalisées, réorganisation et panne au retrait restent à qualifier.
Aucun proposer refusé observé dans ce run ; pas de qualification de ce cas.
[Implémentation et preuves](qualification/capella-slashing-retirement-20260927/README.md).

### 2026-09-27 — Consommation d’un dépôt natif après élagage

Nouveau scénario `consensus_capella_deposit_consumption` : transaction de dépôt
sur le contrat natif après élagage, réouverture, puis votes natifs jusqu’à la
majorité de la période suivante. Deux owners doivent rester identiques ; le
curseur ne passe de 0 à 1 qu’une fois. Le solde du validateur existant est vérifié
exactement, dépôt et pénalités du comité de synchronisation inclus ; le rejeu ne
change pas le checkpoint, puis les deux bases sont rouvertes.
L’assertion initiale ignorait les pénalités de participation sync vide : échec
conservé, correction du test sans changement des règles de consensus.
**Partiel** : top-up d’un validateur existant, pas admission/activation d’une
nouvelle clé, ni EIP-6110 Ethereum, bridge, WAN ou panne au commit du dépôt.
Résultat : passé en 87,06 s ; consommation au slot 80, poursuite jusqu’au slot 89,
pénalité exacte de 11 180 gwei, solde attendu/observé de 33 007 719 982 gwei.
Réouverture des deux bases et rejeu idempotent vérifiés.
[Scénario et preuves](qualification/capella-deposit-consumption-20260927/README.md).

### 2026-09-27 — Admission et activation d’une nouvelle clé native

Nouveau scénario `consensus_capella_deposit_activation` : dépôt signé au seuil
de solde effectif pour une clé absente du genesis, après élagage et réouverture.
Le registre doit passer de 64 à 65 entrées seulement lors de la consommation du
dépôt ; identité, credentials et solde initial sont vérifiés. La clé en attente
ne doit pas signer ; son époque programmée doit rester stable, puis ses votes
doivent être acceptés sur les deux owners et son activation survivre au restart.
Passé en 159,63 s : dépôt consommé au slot 80, activation époque 17, trois
votes de la nouvelle clé acceptés, finalité époque 18 au slot 161. Réouverture
des deux bases vérifiée ; régression top-up passée (85,92 s). Aucun correctif
du runtime requis. L’époque exacte n’est pas comparée à un client de référence.
**Partiel** : scénario local à clés synthétiques ; pas churn massif, signature
de dépôt invalide, partition, reorg ni panne au commit d’activation. Ne clôture
pas EIP-6110 ni l’ensemble du consensus de production.
[Scénario et preuves](qualification/capella-deposit-activation-20260927/README.md).

### 2026-09-27 — Dépôt natif à preuve de possession invalide

Nouveau scénario : dépôt payé et preuve Merkle cohérente pour une nouvelle clé,
mais signature BLS produite par une autre clé. Le curseur doit avancer sans
admettre le validateur. Les deux reactors doivent refuser ses demandes de
signature avant tout appel au signataire, sans changement de checkpoint ;
rejeu, égalité Beacon/EVM et absence d’admission après réouverture sont vérifiés.
Passé en 87,21 s : consommation au slot 80, poursuite jusqu’au slot 89, registre
à 64 conservé et finalité époque 9. Runner officiel passé en 0,38 s ; résultats
et empreintes archivés. Aucun correctif runtime requis.
**Partiel** : une signature par mauvaise clé, pas toutes les variantes malformées,
reorg, injection de votes hostiles ou panne au commit. Aucun changement des règles
runtime ; EIP-6110 et la préparation production globale restent ouverts.
[Scénario et preuves](qualification/capella-invalid-deposit-signature-20260927/README.md).

### 2026-09-27 — Agrégats sync dans le proposant Capella natif

`submit_sync_aggregate` vérifie un agrégat complet contre l’état authentifié,
le slot courant et la tête sélectionnée. Un seul agrégat éphémère est retenu,
avec préférence pour davantage de positions participantes. L’assemblage local et
`propose_current` l’incluent uniquement pour le même slot/parent, puis la transition
complète revalide le bloc. Signature seule et application des récompenses sont
séparées pour ne pas appliquer prématurément la comptabilité avant les retraits.
Nouveau scénario alternant pleine participation et agrégat vide, avec corruption,
slot futur, checkpoint périmé, rejeu et annulation. Fixtures officielles sync
également contrôlées avec un cas de soldes saturés. Résultat : 35,37 s,
40 blocs dont 20 pleinement participatifs, 272 votes, finalité époque 3 et
deux réouvertures ; 16 fixtures officielles valides et 8 invalides conformes.
**Partiel** : collecte/signature des messages individuels, sous-réseaux,
agrégation de contributions partielles, transport RPC/P2P et raccordement au
scheduler restent à implémenter ; invalidation par reorg à qualifier séparément.
[Code et preuves](qualification/capella-local-sync-20260927/README.md).

### 2026-09-27 — Fusion des participations sync disjointes

Le cache d’agrégats du proposant fusionne désormais deux participations non vides
et disjointes, vérifiées pour le même slot/parent. Les signatures G2 sont sommées,
les bits réunis et l’agrégat résultant revalidé avant publication. Les chevauchements
ne sont jamais sommés ; un sous-ensemble ne peut gonfler la participation.
La primitive BLS rejette encodages invalides, mauvais sous-groupes et points à
l’infini, y compris une somme qui s’annule. Test local renforcé avec deux moitiés,
ordre alterné, doublons et sous-ensembles ; comparaison à une signature complète
construite indépendamment. Passé en 36,01 s : 40 blocs, 20 agrégats fusionnés,
272 votes, finalité époque 3 et deux réouvertures. Runner BLS : 12 cas officiels
conformes, mutations existantes et nouvelles assertions de somme validées.
**Partiel** : cache toujours borné à un agrégat, pas fusion optimale de recouvrements,
ni collecte/signature individuelle, P2P ou scheduler. Les bascules de branche et
redémarrages entre contributions restent à qualifier.
[Implémentation et preuves](qualification/capella-sync-merge-20260927/README.md).

### 2026-09-27 — Admission individuelle des messages sync

Le reactor accepte maintenant un message `(slot, racine Beacon, index, signature)`
pour le slot de proposition suivant. Il vérifie l’état/branche, l’appartenance au
comité et la signature BLS dans le domaine approprié. Seules les positions non
encore couvertes sont ajoutées, avec multiplicité des clés répétées ; le rejeu
n’augmente pas le poids. Aucune écriture durable lors de la collecte.
Scénario renforcé : complétion d’une moitié agrégée par messages individuels,
comparaison à l’agrégat complet indépendant et rejets adversariaux. Passé en
38,33 s : 40 blocs dont 10 complétés par messages individuels, 272 votes,
finalité époque 3 et deux réouvertures. Empreintes et logs conservés.
**Partiel** : API locale uniquement ; format réseau, P2P, signature et scheduler
restent ouverts. Dérivation du contexte répétée par message : coût à mesurer et
optimiser avant toute revendication à 100k/1M validateurs.
[Implémentation et preuves](qualification/capella-sync-message-20260927/README.md).

### 2026-09-27 — Encodage et admission réseau des messages sync

Codec SSZ strict de 144 octets ajouté avec vecteur connu d’ordre/endianness.
Le type `sync_committee_message`, activé seulement pour Capella, traverse les
enveloppes hex/base64, le dispatcher privé et le worker borné jusqu’au reactor.
L’admission SSZ choisit son checkpoint sous verrou ; tailles et identité réseau
sont contrôlées avant traitement. Scénario local renforcé avec reçus asynchrones
réels du service et alternance hex/base64, plus régression dispatcher/stream.
Passé : 38,76 s pour 40 blocs/272 votes, finalité époque 3 et deux réouvertures ;
dispatcher/stream verts en 0,52 s. Binaire nano-p2p recompilé, empreintes vérifiées.
**Partiel** : pas encore de diffusion sortante ni d’échange libp2p réel de messages
sync ; l’adaptateur RPC est exercé directement, pas via HTTP. Signature locale,
scheduler, export éphémère et qualification WAN restent ouverts.
[Implémentation et preuves](qualification/capella-sync-wire-20260927/README.md).


### Diffusion sortante sync Capella — 27 septembre 2026

Les messages sync individuels validés sont exportables via catalogue figé et relais
P2P privé, avec un cache mémoire borné au comité et au contexte slot/branche.
Le daemon traite ce cinquième type ; le relais vérifie le hash du contenu.
Qualification : 40 blocs / 272 votes / finalité 3 / deux réouvertures (41,36 s),
export vers un second réacteur, doublons, budgets et invalidation après changement
de tête ; trois tests réseau passent. **Partiel** : aucune nouvelle preuve de
transport libp2p réel ni de livraison avant échéance. Signature automatique,
ordonnancement, sous-réseaux et performances restent ouverts ; aucune clôture EIP
ou production. [Preuves](qualification/capella-sync-export-20260927/README.md).


### Calendrier de précollecte sync — 27 septembre 2026

Correction d’un écart découvert en préparant la signature automatique : les
messages sont désormais admis et exportables pendant leur slot de signature,
puis pendant le slot d’inclusion. Deux contextes bornés conservent séparément
l’agrégat courant et le suivant. Test nouveau de diffusion anticipée vers un second
réacteur, franchissement du slot et inclusion exacte ; coexistence des deux
contextes vérifiée. 40 blocs / 272 votes / finalité 3 / deux réouvertures, 44,95 s.
**Partiel** : signature automatique, calendrier réel, rotation de comité et livraison
libp2p dans les délais restent ouverts. Cette correction remplace la rétention à
un seul contexte de la tranche précédente, sans clôturer une EIP ni la production.
[Spécification et preuves](qualification/capella-sync-precollection-20260927/README.md).


### Signature sync locale et planificateur — 27 septembre 2026

`sync_current` authentifie comité/branche/domaine et journal de clé, contrôle la
signature du fournisseur et l’échéance, puis repasse par l’admission normale.
Le planificateur effectue ce devoir après les attestations et expose le compteur
`syncMessagesAdmitted`. Les messages déjà collectés ne sont pas resignés.
**1280 messages sync**, 40 blocs, 272 attestations et finalité 3 dans le scénario
planifié, avec redémarrage et 64 journaux réouverts (38,21 s). Comparaison à des
signatures indépendantes, mauvaise clé, annulation, signature invalide, dépassement
de slot et seuil du tiers de slot qualifiés séparément (48,46 s).
État **partiel réseau/consensus** : clocks synthétiques, livraison libp2p/WAN dans
les délais, rotation de comité et performance restent à qualifier. Aucune clôture
EIP ou production. [Preuves](qualification/capella-sync-local-signing-20260927/README.md).


### Sync entre vrais pairs et lecture après élagage — 27 septembre 2026

Deux nœuds libp2p locaux (32 clés distinctes chacun) atteignent une finalité commune
d’époque 2. **8 blocs finalisés** contiennent chacun 6–10 positions sync distantes,
avec égalité des blocs observés sur les deux pairs et vérification BLS indépendante.
Le premier run échouait sur une lecture Beacon ; correction de l’exclusion
systématique de l’ancrage dans l’archive signée, sans fabriquer de signature.
Régression d’élagage passée (67,76 s), puis confirmation réseau réussie.
**Partiel** : seulement 21–27/32 positions incluses, 365 tentatives de relais sync
incomplètes contre 11 complètes (pas un taux de perte par message). Priorité à
l’efficacité de livraison ; WAN, adversité, rotation et production restent ouverts.
[Preuves, échec conservé et limites](qualification/capella-sync-distributed-20260927/README.md).


### Relais sync : suppression des échos — 27 septembre 2026

Les admissions acceptées alimentent maintenant le bon cache de possession du pair
pour les attestations et messages sync Capella. Les catalogues consultent ce cache
avant l’export ; les limites de débit, délais et règles de consensus restent inchangés.
Quatre tests réseau passent (33 cas pour le catalogue), dont rejet, changement de
session, pagination et budgets. Sur deux vrais pairs locaux : finalité commune 2,
**32/32 positions sync dans les 8 blocs finalisés contrôlés**, contre 21–30 avant.
Exports blocs 5981→81, attestations 19863→237, sync 3736→1092 sur ces deux runs.
**Partiel** : 119 livraisons sync sans accusé confirmé subsistent ; ces nombres ne
sont pas des taux de perte ni une garantie générale. Attribution côté réception,
rotation, WAN et adversité restent ouverts. [Preuves et limites](qualification/capella-sync-noecho-20260927/README.md).


### Attribution des interruptions réseau — 27 septembre 2026

Instrumentation optionnelle de la réception : fin unique, cause, type si décodé,
état du worker ; erreurs du diagnostic sans effet sur l’admission. Sept nouveaux
cas dans la suite stream ; quatre CTests passent. Deux pairs locaux : finalité
commune 2 et 32/32 positions sync sur huit blocs finalisés vérifiés. Le récepteur
observe **169 refus du budget avant décodage**, sans erreur locale du cœur ni
échec d’écriture d’ACK. L’émetteur observe 132 livraisons sync non confirmées.
**Partiel** : pression d’admission prouvée, correspondance individuelle et cause
fine du budget non établies. Prochaine étape : attribution débit/concurrence puis
ordonnancement sortant borné ; aucune hausse des limites. Rotation, WAN et
production restent ouverts. [Preuves](qualification/capella-receive-attribution-20260927/README.md).


### Régulation sortante Capella — 27 septembre 2026

Les messages consensus utilisent le sous-budget bulk (4/s par pair, rafale 8),
plus restrictif que le budget général. Les limites réceptrices restent inchangées.
Les catalogues partagent maintenant un rythme sortant de 4/s avec rafale bornée
à 4 ; envoi différé gardé par la session et la durée de vie du catalogue. Les refus
récepteurs exposent leur cause précise. Cinq suites passent, dont 1000 envois
simulés contre le vrai budget. Une première variante trop espacée réduisait la
participation (28/32 minimum) : preuve conservée. Variante finale, deux pairs
locaux : finalité commune 2, 32/32 dans huit blocs finalisés contrôlés, aucun refus
de budget, une livraison sync non confirmée (132 auparavant), 18 refus explicites.
**Partiel** : ces derniers refus restent à attribuer ; pas de garantie statistique,
WAN, rotation ou grande population. [Preuves et limites](qualification/capella-send-pacing-20260927/README.md).


### Causes des refus sync du cœur — 27 septembre 2026

Compteurs bornés, saturants et éphémères ajoutés au statut RPC authentifié et au
reçu d’arrêt ; aucune modification des décisions ni des checkpoints. Le scénario
local (45,95 s) vérifie les causes et distingue annulation/checkpoint périmé d’un
refus de message. Deux pairs réels locaux : finalité commune 2, 32/32 positions
dans huit blocs finalisés. Les 16 refus sync du run correspondent à **14 racines
inattendues, un slot expiré et un slot déjà atteint par la tête** ; aucun échec au
contrôle BLS observé. Cela ne valide pas les signatures rejetées avant ce contrôle.
Zéro refus de budget ; deux livraisons sans accusé confirmé restent ouvertes.
**Partiel** : attribution agrégée, pas causalité réseau par message ; ne pas relâcher
ces contrôles. Prochaine qualification : rotation des comités et réseau asymétrique.
[Preuves](qualification/capella-sync-admission-causes-20260927/README.md).


### Rotation sync native avec reprise — 27 septembre 2026

Nouveau scénario du planificateur sur le preset minimal inchangé : **216 blocs,
trois frontières (64/128/192), deux changements de composition et 215 agrégats BLS
vérifiés à 32/32 positions**. Redémarrage propriétaire/réacteur et 64 journaux au
slot 127, puis inclusion complète au slot 128. Finalité époque 25 ; élagage actif,
maximum 31 états de blocs retenus. Deux CTests passent (209,76 s), dont le scénario
court existant. L’échec initial, dû à l’hypothèse erronée que les deux comités
initiaux de la fixture différaient, est conservé ; le test étendu exige bien deux
changements réels. **Partiel** : horloge simulée, un processus natif, pas de transport
réseau ni de crash physique. Rotation distribuée, asymétrie et partitions restent
à qualifier. [Preuves et référence](qualification/capella-sync-rotation-20260927/README.md).


### Rotation sync distribuée — 27 septembre 2026

Deux nœuds libp2p réels avec 32 clés distinctes chacun, slots de 6 s et preset
inchangés : **finalité commune époque 17 après changement effectif du comité**.
Les deux bases réouvertes via leurs témoins de reprise contiennent le même état
finalisé au slot 136 ; son engagement SSZ et son nouveau comité sont vérifiés.
Huit blocs finalisés (129–136) portent des contributions distantes BLS vérifiées,
avec **28–32 positions sur 32**, et non 100 % partout. Zéro refus de budget observé ;
80 refus sync du cœur (65 racines, 15 slots déjà atteints), deux livraisons sans ACK
confirmé et neuf exports indisponibles restent tracés. **Partiel** : deux pairs
sur un hôte, réseau stable ; asymétrie, partitions à la rotation, reprise après
crash et grande population restent ouverts. [Preuves](qualification/capella-distributed-sync-rotation-20260927/README.md).


### Reprise sync après coupure du transport — 27 septembre 2026

Le nouveau mode `--sync-transport-outage-cli` passe sur deux pairs libp2p réels
(32 clés distinctes chacun). Après 96 s de SIGSTOP des seuls démons P2P,
les branches divergent sans finaliser au-delà de la borne pré-coupure, puis
retrouvent une finalité commune époque 4. Quatre blocs finalisés aux slots 29–32
contiennent des signatures distantes BLS vérifiées pour des slots supérieurs
aux têtes observées à la reconnexion (pas une preuve horaire d’émission) ;
le slot 28 n'en contient aucune. Participation observée : 15–31/32 positions,
pas une reprise complète instantanée. Zéro refus de budget observé ; 29 refus
sync du cœur (19 racines, 10 inclusions déjà atteintes) et un ACK sync non confirmé
restent tracés. **Partiel** : coupure de processus transport sur un seul hôte,
sans rotation pendant la partition ni crash disque. Aucun nouveau statut de
conformité EVM/EIP ni validation de production. [Preuves](qualification/capella-sync-outage-recovery-20260927/README.md).


### Rotation sync pendant une coupure — 27 septembre 2026

Le mode `--sync-rotation-outage-cli` passe : deux pairs, 32 clés distinctes chacun,
coupure des démons P2P pendant 96 s commencée avant le slot128, franchissement
de la rotation sur deux branches isolées, puis finalité commune époque18.
Les deux bases réouvertes donnent le même état SSZ engagé au slot144 et un comité
explicitement différent de l'initial. Cinq blocs finalisés (140–144) ont leurs
32 positions BLS vérifiées, dont12 ou20 distantes ; le slot139 n'a que12 positions
et aucune distante. Reprise progressive. Zéro refus de budget observé, mais
67 refus sync (56 racines/11 inclusions atteintes), un échec d'écriture ACK,
deux livraisons non confirmées et six exports indisponibles restent archivés.
**Partiel** : outage de processus sur un hôte, sans qualification WAN, pertes
asymétriques, crash sous charge ou grande population. Aucun nouveau statut EVM
ou production. [Preuves](qualification/capella-sync-rotation-outage-20260927/README.md).


### Contre-exemple actif : retard asymétrique — 27 septembre 2026

**ÉCHEC** sur l'image du commit `c369693` : retard netem de500ms sur la seule
sortie d'un pair, deux démons actifs/32clés chacun. Finalité commune2 avant
perturbation, puis3, sans atteindre4 dans la fenêtre96+144s. Têtes divergentes,
processus/horloges sains ; aucune finalité contradictoire observée. Ce résultat
réouvre la qualification de progression sous latence, malgré les reprises de
partition réussies. Les catalogues d'attestations prennent jusqu'à27s près de
l'arrêt ; cause précise à établir, sans relâcher les règles ni les échéances.
La reprise sans retard n'a pas été qualifiée après cet échec. Ressources dédiées
nettoyées et absence vérifiée. [Preuves](qualification/capella-asymmetric-delay-20260927/README.md).


### Essai rejeté : livraison de deux votes simultanés — 27 septembre 2026

Le candidat passe quatre suites ciblées, mais échoue au même scénario500ms :
finalité commune3 sans atteindre4. Il introduit aussi des refus de concurrence
bulk absents du témoin. Les modifications de production sont retirées ; leur
patch, les nouveaux tests et toutes les traces sont conservés pour reproduction.
Le blocage reste actif ; prochaine analyse : ordonnancement des catalogues et
livraison des votes récents/dépendances, sans relâcher les limites.
[Expérience rejetée](qualification/capella-vote-pipeline-20260927/README.md).


### Diagnostic mesuré du retard — 27 septembre 2026

Le scénario500ms échoue encore à la finalité3, avec instrumentation seule.
Pour les attestations, la médiane d'envoi passe de213–219ms à2893–3019ms :
environ1,4–1,5s d'ouverture du flux, puis1,5s après écriture (écriture médiane0ms).
Les traces distinguent admission/refus/issue inconnue et conservent les échecs.
Le temps post-écriture inclut transport et travail distant, pas seulement RTT.
Prochaine correction : amortir négociation et accusés par réutilisation ou lots
bornés, en préservant les décisions d'admission et les budgets. **Blocage actif**,
aucune nouvelle conformité EVM ou disponibilité de production.
[Mesures et limites](qualification/capella-send-timing-20260927/README.md).


### Primitive de réception réutilisable — 27 septembre 2026

Session séquentielle bornée à 16 messages, avec budget, validation du domaine et
accusé distincts par message. Annulation à la frontière ACK corrigée ; **11 cas
nouveaux**, 30 au total dans `altair_stream`, et `timed_reply` passent. Binaire
P2P compilé. **Non raccordée au réseau** : négociation, émetteur et invalidation
par génération restent à intégrer. Le contre-exemple de finalité à 500ms reste
ouvert ; aucun statut EIP ou production clôturé. [Preuves et limites](qualification/consensus-stream-session-20260927/README.md).


### Finalité sous retard asymétrique : scénario passé — 27 septembre 2026

Le transport Capella réutilise des flux négociés séparément, bornés à 16 messages,
avec un accusé et une admission par message. Huit nouveaux cas émetteur portent
la suite de flux à 38 ; compatibilité/quota/relais vérifiés. **Le même scénario
Docker 500ms passe désormais** : finalité commune 2 → 4 pendant 147,985s de retard,
puis 5 après retrait (43,826s pour la qualification de reprise). Médiane des
attestations sous retard ≈1,5s, contre ≈3s précédemment ; aucun refus de budget
entrant journalisé. Refus d'admission, un échec d'ACK et deux envois d'attestation
non confirmés restent dans les preuves. Ce contre-exemple local est couvert ;
WAN géographique, nombreux pairs, adversaires, charge et production globale
restent ouverts. Aucun nouveau statut EIP clôturé. [Code, mesures et limites](qualification/capella-stream-reuse-20260927/README.md).


### Crash complet, reprise avec témoin et ancre après élagage — 27 septembre 2026

Deux courses de délais sont reproduites puis corrigées : une expiration JSON déjà
en file fermait un flux terminé ; un ancien délai d'inactivité pouvait fermer le
flux réutilisé. La concurrence de neuf pairs synthétiques respecte les huit
permis bulk globaux et conserve la capacité de contrôle (pas un réseau de neuf
nœuds qualifié). 40 cas de flux et cinq cas JSON passent.

Le nouveau SIGKILL Docker révèle d'abord une configuration de reprise absente
(refus explicite du checkpoint), puis une finalité bloquée à 2 malgré le témoin.
Le relais conservait l'ancre du démarrage après élagage. Il reconnaît désormais
l'ancre actuelle renvoyée par le cœur authentifié pour terminer une recherche
d'ancêtres, sans inventer d'ACK du pair. Six cas nouveaux portent la livraison
ancestrale à 19 scénarios ; les descendants exigent toujours leur admission réelle.

**Rejeu passé** : deux nœuds, 32 clés synthétiques chacun ; SIGKILL de 96,801s,
identité/volume/checkpoint conservés, puis finalité commune 7 en 141,703s depuis
la configuration de reprise (139,655s après disponibilité vérifiée). Les trois
campagnes et les tests négatifs sont conservés. La configuration passe
explicitement de l'amorçage à la reprise par témoin ; le redémarrage autonome
sans changement de configuration, les pannes physiques, le WAN, les nombreux
pairs et la production globale restent ouverts. Aucun nouveau statut EIP clos.
[Correctifs, échecs, réussite et limites](qualification/reusable-stream-deadlines-20260927/README.md).

### Reprise automatique Capella explicite — 27 septembre 2026

Option `automaticRecovery: true` : conserver le snapshot initial dans la
configuration, initialiser seulement si les deux stores sont vierges, sinon
exiger la reprise vérifiée par témoin. Le témoin absent avec checkpoint primaire
est refusé ; aucune réinitialisation silencieuse. Deux scénarios runtime passent
(ancien mode explicite et nouveau mode automatique, 64 clés synthétiques).
Le crash Docker sans modification de configuration reste à qualifier ; la perte
ou le rollback conjoint des deux stores exige une frontière de confiance externe.
Aucun EIP clôturé, aucune qualification globale de production.
[Preuves et limites](qualification/capella-automatic-recovery-20260927/README.md).

### Crash Docker avec configuration inchangée — 27 septembre 2026

**PASS** : deux nœuds × 32 clés synthétiques, SIGKILL complet pendant 96,757 s,
reprise automatique avec le même fichier (SHA256 contrôlé), la même identité
P2P et le checkpoint conservé. Finalité commune d'époque 7 après 143,110 s
depuis la préparation du redémarrage ; disponibilité vérifiée après 2,022 s.
Ce cas clôt la limite de modification manuelle de configuration du scénario
précédent. WAN, pertes physiques, rollback indépendant, longues pannes, charge
et production globale restent ouverts. Aucun EIP clôturé ni VPS modifié.
[Preuves](qualification/capella-automatic-docker-recovery-20260927/README.md).
[Procédure de reprise](CAPELLA-RECOVERY.md).

### Coût des retraits sur grands registres — 27 septembre 2026

Six nouveaux cas Capella passent : 100 000 et 1 000 000 entrées synthétiques,
retraits denses/rares/absents, bouclage du curseur, montants/adresses/ordre,
conservation de tous les validateurs et refus de listes altérées. À un million,
préparation du candidat en 13 788–32 442 µs ; pic RSS processus 394 772 480 octets.
Sélection bornée à 16 384 entrées, mais copie du registre O(N) toujours présente.
Mesure locale unique hors SSZ/BLS/disque/réseau, pas une capacité de nœud actif.
Aucun EIP ni objectif production clôturé ; mesurer ensuite la projection SSZ
authentifiée et le runtime complet.
[Code, mesures et limites](qualification/capella-withdrawal-scale-20260927/README.md).

### Authentification SSZ à un million : coût limitant mesuré — 27 septembre 2026

Deux nouveaux cas passent avec comparaison de tous les octets et refus d'une
racine incorrecte. États synthétiques de 100 000 / 1 000 000 entrées, autres
vecteurs au preset minimal. Préparation complète des retraits SSZ : 422 669 /
4 258 573 µs ; calcul initial de racine : 215 134 / 2 068 384 µs. À un million,
état de 139 010 821 octets et RSS maximal du processus 875 020 288 octets.
La mesure inclut authentification/projection/copie/recalcul de racine, hors
BLS, reste de transition, disque et réseau. Elle démontre un coût significatif
que le benchmark registre seul masquait ; aucune capacité runtime clôturée.
Priorité : profiler et réduire les calculs complets répétés sans supprimer
l'authentification, puis mesurer le nœud complet. Aucun EIP clôturé.
[Preuves](qualification/capella-withdrawal-ssz-scale-20260927/README.md).

### Arbre SSZ Validator de taille fixe — 27 septembre 2026

Optimisation C++ : les huit champs du validateur sont hachés sur la pile, sans
vecteurs temporaires ni recalcul des sous-arbres nuls inutiles. Vérifications de
format, annulation, authentification et racine de sortie conservées. Six suites
passent ; 17 nouvelles mutations de champs. Racines avant/après identiques à
100 000 et un million d'entrées. À un million, préparation SSZ mesurée de
4 468 134 à 3 029 580 µs (~32 % de temps en moins, comparaison unique).
Le coût complet du runtime reste ouvert ; aucune conformité EIP supplémentaire.
[Preuves et comparaison](qualification/ssz-validator-fixed-tree-20260927/README.md).

### Retraits SSZ sans projection complète du registre — 27 septembre 2026

Le chemin SSZ authentifie la racine et les cardinalités, puis sélectionne les
retraits directement dans une fenêtre bornée. Règles de sélection partagées avec
le registre classique ; comparaison exacte de la liste et recalcul final maintenus.
Cinq suites passent, avec deux nouvelles annulations sans mutation. À un million,
RSS maximal du benchmark de 854 589 440 à 639 205 376 octets ; temps mesuré de
3 029 580 à 2 911 148 µs. Racines identiques. Les autres projections et le coût
runtime complet restent ouverts ; aucune clôture EIP ou production.
[Preuves](qualification/ssz-withdrawal-direct-sweep-20260927/README.md).

### Réutilisation SSZ confinée à une transformation — 27 septembre 2026

Entrée copiée et intégralement authentifiée avant transformation ; racines des
champs variables réutilisées uniquement après égalité exacte des octets et
longueurs, au sein du même appel. Champs changés et offsets revérifiés, aucun
cache externe fourni ou persistant. Sept suites passent, dont nouveaux contrôles
sur les huit états Capella officiels (modifications de champs, entrée non
authentifiée, sortie malformée, isolation et abandon). À un million, préparation
de 2 911 148 à 1 525 469 µs ; RSS 639 090 688 octets, racines identiques.
Mesures locales uniques hors runtime complet ; aucune clôture EIP/production.
[Preuves et frontière de confiance](qualification/ssz-scoped-field-reuse-20260927/README.md).

## 2026-09-27 — Régression Bellatrix : drainage des traces de reprise

La régression sur `af7f99e` expose un échec de finalité après partition
(516,39 s). Le prédicat ne drainait plus les sorties tant que la finalité
n’avançait pas : le correctif draine les deux processus inconditionnellement.
Un nouveau contrôle à deux enfants reproduit la contre-pression stdout/stderr
et vérifie la récupération intégrale après drainage. Le scénario réseau avec
ce correctif reste à rejouer ; la campagne initiale continue sur ses binaires
inchangés. Aucun délai ni critère abaissé, aucune EIP clôturée.
[Échec, contrôle et limites](qualification/bellatrix-partition-trace-drain-20260927/README.md).

## 2026-09-27 — Assertoor : défaut de base de résultats reproduit

La régression `consensus_bellatrix_assertoor_finality` échoue après 296,02 s
malgré une finalité commune d’époque 4 à l’arrêt. Le journal Assertoor signale
une table `task_logs` absente. Un reproducer à deux connexions avec son pilote
exact reproduit l’erreur du DSN mémoire ; un fichier temporaire partagé entre
connexions passe. Le probe configure désormais une base de résultats sur disque
propre à chaque run. Le rejeu réel reste à effectuer ; aucune règle de consensus
ni limite temporelle modifiée, aucun EIP clôturé.
[Échec, reproducer et correction](qualification/assertoor-result-database-20260927/README.md).

## 2026-09-27 — Bilan initial de la régression consensus

Sur les binaires `af7f99e`, **185/188 tests sélectionnés passent, 3 échouent,
aucun n’est ignoré**. Les deux partitions Bellatrix échouent avant la reprise
de finalité ; Assertoor finalité échoue avec une table de résultats absente.
Les correctifs de banc d’essai sont préparés séparément et attendent leurs
rejeux réels. La reprise après crash avec stabilité prolongée passe (325,44 s).
Les 201 autres tests CTest sont hors de cette campagne ; aucun statut EIP ou
production n’est clôturé. [Bilan, empreintes et résultats complets](qualification/consensus-regression-20260927/README.md).

## 2026-09-27 — Rejeu de partition Bellatrix après correction du banc

Le scénario simple corrigé passe : coupure demandée de 96 s, finalité commune
d’époque 6 retrouvée en 95 s, mêmes racines Beacon et bloc d’exécution sur les
deux nœuds. Les binaires Nano/P2P sont inchangés ; seul le drainage des sorties
du scénario est corrigé. Les seuils et délais restent identiques. La variante
prolongée et le rejeu Assertoor sont encore attendus ; aucune clôture EIP ou
production. [Rejeu et contre-exemples conservés](qualification/bellatrix-partition-trace-drain-20260927/README.md).

## 2026-09-27 — Rejeu Assertoor après correction de sa base

Le contrôle réel de finalité passe en 198,32 s : quatre tâches Assertoor
réussies, finalité commune d’époque 2 et même bloc d’exécution sur les deux
nœuds, vérifiés indépendamment du cache Assertoor. Plus d’erreur de table SQL
absente dans ce rejeu. Les binaires Nano/P2P et les délais restent inchangés.
Ce résultat local couvre ce défaut du banc ; il ne clôture aucune EIP ni la
production. [Échec initial et rejeu complet](qualification/assertoor-result-database-20260927/README.md).

## 2026-09-27 — Trois échecs initiaux couverts par leurs rejeux

La partition prolongée corrigée passe (565,85 s) : reprise commune d’époque 7
en 144 s après reconnexion, puis trois avancées communes jusqu’à l’époque 10
en 144,552 s. Avec la partition simple et Assertoor déjà rejoués, les trois
échecs initiaux ont chacun une preuve corrigée réussie. La campagne initiale
reste 185/188 ; ne pas présenter l’ensemble comme un unique run 188/188. Aucun
code de consensus ni seuil assoupli, aucune nouvelle clôture EIP/production.
[Campagne et rejeux séparés](qualification/consensus-regression-20260927/README.md).

## 2026-09-27 — Recommandation des frais RPC historique

Le pourboire constant de 1 gwei est remplacé par une politique configurable :
échantillons de frais effectifs récents, exclusion du producteur et cache par
hash de tête. Le portefeuille partage cette recommandation ; les frais explicites
restent inchangés et les valeurs implicites sont bornées à uint256. Quatre tests
passent (8,65 s), dont le nouveau `rpc_fee_policy` ; les trois autres couvrent
les parcours RPC/CLI existants. Aucun changement de formule consensus EIP-1559.
Coût à froid, saturation et concurrence pendant une réorganisation restent à
qualifier ; aucun statut global de production clôturé.
[Politique](RPC-FEE-POLICY.md) et [preuves](qualification/rpc-fee-policy-20260927/README.md).

## 2026-09-27 — Charge RPC et correction de l’exclusion du producteur

Le nouveau scénario détecte un défaut masqué par l’ancien test : adresse avec
checksum comparée à une adresse normalisée, donc producteur non exclu. Les deux
adresses sont désormais normalisées ; l’ancien test utilise un pourboire du
producteur inférieur aux frais externes. Cinq tests ciblés passent ; le scénario
final couvre aussi 20 blocs / 28 560 transactions. Calcul sans cache de
recommandation autour de 652 ms, sous mutex RPC ; 1 000 lectures en cache en
5,385 ms au total. Mesures locales, caches disque non purgés, sans HTTP/P2P.
Coût répété à chaque tête, fenêtre maximale, équité et saturation restent ouverts.
[Échec initial, correction et mesures](qualification/rpc-fee-load-20260927/README.md).

## 2026-09-27 — Réutilisation bornée des échantillons de frais RPC

Les pourboires sont maintenant réutilisés par hash de bloc, dans la seule fenêtre
courante ; rechargement de politique, réorganisation et expiration de fenêtre
restent contrôlés. Cinq tests affectés passent, avec nouveaux cas de branche
remplacée et expiration. Après un nouveau bloc de 1 428 transactions, mesure
locale de 33,518 ms contre 671,225 ms pour un estimateur neuf sur la même tête.
Le premier calcul, les branches non cachées, la fenêtre maximale et l’équité
sous mutex RPC restent ouverts. Aucun changement de consensus EIP-1559.
[Implémentation, mesures et limites](qualification/rpc-fee-incremental-20260927/README.md).

## 2026-09-27 — Reprise ordonnée pendant l’activation d’une nouvelle clé

Nouveau scénario `consensus_capella_deposit_activation_restart` passé en 159,53 s.
Dépôt consommé au slot 80 ; owner et journal rouverts au slot 104 pendant l’attente,
puis second owner et journal après le premier vote au slot 142. Le vote
contradictoire est rejeté avant le signataire, sans mutation du journal ; trois
votes légitimes sont admis et la finalité atteint l’époque 18. Registre à 65,
activation époque 17 et curseur unique conservés après réouverture finale.
L’échec initial de banc est conservé, aucun correctif runtime. **Partiel** :
réouvertures ordonnées, pas crash au commit, perte de témoin, WAN ou churn massif.
Ne clôture ni EIP-6110 ni le cycle validateur de production.
[Preuves et limites](qualification/capella-activation-restart-20260927/README.md).

## 2026-09-27 — Clé configurée avant inscription, résolution automatique

Le runtime accepte désormais `index: null` : clé publique dérivée du secret,
index recherché dans la tête authentifiée à chaque poll, attente sans devoir
si absente et garde d’activation inchangée. Journal, identité de chaîne et
identité du devoir restent vérifiés avant signature. Les index explicites
restent stricts ; reçus et traces identifient les clés automatiques.
Quatre tests passent : deux nouveaux (chargement/reprise avec clé absente et
dépôt jusqu’aux trois attestations automatiques) plus runtime et finalité de la
boucle existante. Dépôt slot 80, activation époque 17, finalité époque 18.
**Partiel** : projection du registre à grande échelle, reorg concurrent, crash
au commit et WAN restent ouverts. Aucun statut EIP-6110/production clôturé.
[Configuration](CAPELLA-VALIDATOR-REGISTRATION.md) et [preuves](qualification/capella-automatic-registration-20260927/README.md).

## 2026-09-27 — Recherche des clés automatiques sans projection complète

Le reactor Capella authentifie le SSZ de la tête puis ne conserve que les clés
locales demandées. Nouveau test à 100 000/1 000 000 enregistrements synthétiques :
146 ms/1,43 s avec authentification, trois correspondances exactes et une absence.
Racine incorrecte, doublon, largeur, annulation et cardinalité incohérente refusés.
La régression du runtime à 65 clés configurées passe ; 2/2 tests en 29,37 s.
Le coût linéaire sous verrou à chaque poll, les changements de branche concurrents
et la charge distribuée restent ouverts. Aucun million de nœuds ni BLS mesuré,
aucune clôture EIP ou production. [Preuves](qualification/capella-registration-ssz-scale-20260927/README.md).

## 2026-09-27 — Cache des index lié à la tête authentifiée

Cache privé à une entrée, indexé par racine SSZ et ensemble exact de clés locales,
sous verrou du reactor. Pas de snapshot conservé ; une absence n'est mémorisée
que pour la racine courante. Nouvelle couverture du scénario de réorganisation :
trois recalculs de racine, sept hits, deux bascules de branche, annulation et
changement des clés contrôlés. Runtime et dépôt/activation passent aussi :
**3/3 en 163,77 s**, trois attestations de la nouvelle clé après activation.
Le coût froid à chaque nouvel état, les indices divergents après dépôts concurrents
et la charge distribuée restent ouverts. Aucun EIP/production clôturé.
[Preuves et limites](qualification/capella-registration-cache-20260927/README.md).

## 2026-09-27 — Index automatique revérifié sous verrou de signature

Les propositions, attestations et messages sync revérifient la paire clé/index
sur la tête admise avant le signataire. Un index périmé devient un devoir à
retenter (`StaleCapellaDuty`), sans masquer une mauvaise association au journal.
Nouveau test : trois refus d'index et trois refus de configuration, zéro appel
au signataire et checkpoint/journal inchangés, puis 40 blocs et finalité époque 3
avec les clés automatiques. Régressions index fixe et dépôt/activation passent :
**3/3 en 160,16 s**. Deux dépôts concurrents réattribuant un index et disponibilité
WAN restent ouverts ; le test injecte directement un index périmé, pas un thread
concurrent. Aucun EIP/production clôturé.
[Preuves](qualification/capella-registration-signing-guard-20260927/README.md).

## 2026-09-27 — Deux branches financées, ordres de dépôts inversés

Nouveau cas dans `consensus_execution_deposit_history` : mêmes clés A/B déposées
par transactions exécutées dans les ordres AB/BA, index de A 0/1 ; quatre preuves
étrangères et quatre rejeux refusés sans mutation. Les deux restaurations
reconstruisent leurs registres. Le sélecteur natif distingue les deux ascendances
à cardinalité égale et vérifie quatre opérations obligatoires. Test passé en
0,39 s. Enveloppes Beacon synthétiques pour la sélection ; bascule réelle du
reactor, fork choice et signatures après réindexation restent ouverts.
Aucun changement de production, aucun EIP clôturé.
[Preuves et limites](qualification/competing-deposit-order-20260927/README.md).

## 2026-09-27 — Reactor réindexé par de vrais blocs à dépôts concurrents

Nouveau scénario local avec trois propriétaires RocksDB : branches AB/BA
signées et admises jusqu'au slot 49, dépôts consommés, registre de 66 membres.
Les votes de comité sélectionnent chaque branche ; la clé A passe de 64 à 65,
avec état EVM/historique cohérents et deux réouvertures vérifiées. L'ancien
index est refusé avant signature sans mutation ; le nouveau reste inactif.
Test passé en 22,09 s. Échec initial du budget de 64 blocs du banc conservé,
budget du scénario calculé à 98 sans changement de consensus. Activation après
reorg, finalité, threads concurrents et WAN restent ouverts. Aucun EIP clôturé.
[Preuves](qualification/capella-competing-deposit-reorg-20260927/README.md).

## 2026-09-27 — Activation et finalité après la réindexation 64 vers 65

Extension intégrée de la concurrence de dépôts : après sélection de BA, la
boucle automatique retrouve A à l'index 65, attend l'activation époque 13 puis
produit neuf attestations admises. Aucun appel au signataire avant activation.
Le producteur et l'observateur finalisent le même checkpoint d'époque 20 ; les
deux réouvertures conservent finalité et index. Test passé en 175,54 s, jusqu'au
slot 177. Trois bases locales, appels orchestrés sans transport WAN, journal A
resté ouvert, pas de crash au commit ni reorgs soutenues. Aucun EIP/profil ou
objectif production clôturé. [Preuves](qualification/capella-reindexed-activation-20260927/README.md).

## 2026-09-27 — Panne WAL au vote de réorganisation de dépôts

Injection macOS fsync/F_FULLFSYNC EIO dans un sous-processus au vote qui fait
passer A de 64 à 65. Cinq accès refusés après erreur. WAL conservé : état complet
65 et archive du vote ; suffixe perdu : état complet 64 sans archive, puis rejeu
vers 65. Racines EVM/têtes et enveloppes durables exactes contrôlées. Test passé
en 28,61 s. Échec initial du banc (snapshot simple versus enveloppe de votes)
conservé et corrigé. Pas de panne physique, Linux/VPS, témoins indépendants ou
WAN ; aucun EIP/production clôturé. [Preuves](qualification/capella-deposit-reorg-sync-20260927/README.md).

## 2026-09-27 — Injection WAL portée et qualifiée sous Linux ARM64

Interposition fsync/fdatasync réservée aux tests Linux, contrôlée sur fichier
ciblé, fichier distinct et après désactivation. Réutilisation de l'image de
dépendances dans Docker sans réseau, 2 CPU/4 Go. Le scénario de réorganisation
avec WAL conservé/perdu passe en 24,63 s : fermeture après EIO, état ancien ou
nouveau cohérent, archives et rejeu vers l'index 65 vérifiés. Aucun code du nœud
modifié, ni nouvelle image/VPS. Linux ARM64 local seulement : x86-64/VPS,
WAN et autres pannes restent ouverts. [Preuves](qualification/linux-deposit-reorg-sync-20260927/README.md).

## 2026-09-27 — Panne WAL qualifiée sur VPS Linux x86-64

Au commit `c379e7d`, le scénario de réorganisation de dépôts passe sur le VPS
Ubuntu 24.04/Docker amd64 en **99,87 s** : cinq accès refusés après EIO,
ancien état/index 64 si suffixe perdu, nouvel état/index 65 si WAL conservé,
puis rejeu cohérent vers 65. Sources propres, empreintes et journaux archivés.
Test local isolé sur le VPS, pas WAN ni panne physique ; autres points de panne,
charge, témoins indépendants et qualification globale restent ouverts.
[Preuves et limites](qualification/vps-deposit-reorg-sync-20260927/README.md).

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
