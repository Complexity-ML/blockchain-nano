# EIP et consensus : périmètre demandé pour Nano

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


### Partition Docker Capella qualifiée — 27 septembre 2026

Deux daemons actifs, 32 validateurs chacun : après 96 secondes de coupure réseau demandées, finalité commune d’époque 7 retrouvée en 142,223 secondes. Le défaut de démarrage avec superviseur Linux PID 1 est corrigé. [Preuves, échecs initiaux et limites](qualification/capella-docker-partition-20260927/README.md). Une seule campagne locale ; WAN, partitions asymétriques, adversaires, charge et production complète restent ouverts.

Qualification complémentaire : [panne P2P avec validateurs Capella actifs](qualification/capella-transport-outage-20260927/README.md). Après 96 secondes de suspension des seuls daemons, deux branches distinctes convergent vers une finalité commune d’époque 4. Test local passé ; la partition de paquets avec daemons actifs est qualifiée séparément ci-dessus. La production complète reste ouverte.

### Correction du transport privé — 27 septembre 2026

Le client RPC réutilise désormais ses connexions HTTP sans rejouer les requêtes ambiguës. **Deux runs locaux avec 32 validateurs par nœud atteignent une finalité commune d’époque 2**, dont une confirmation sur binaire propre. 17 tests réseau passent, six scénarios HTTP sont qualifiés et la reprise avec accusé perdu passe. [Preuves et limites](qualification/capella-core-keepalive-20260927/README.md). Les contre-exemples ci-dessous sont historiques ; les partitions asymétriques, le WAN et la qualification de production restent ouverts.

Diagnostic antérieur à la correction du transport, conservé comme contre-exemple : [retards d’admission des votes Capella](qualification/capella-vote-propagation-20260927/README.md). Le même test distribué échoue encore ; des votes distants sont admis jusqu’à 12 slots après leur émission observée. Les préparations différées tracées réussissent. La cause du retard reste à isoler ; aucune clôture de la finalité ni de la production.

### Contre-exemple historique avant correction : finalité Capella répartie — 27 septembre 2026

**ÉCHEC** du test à deux vrais pairs libp2p, chacun avec 32 clés distinctes :
aucun checkpoint finalisé commun d’epoch ≥2 en 210 secondes. Finalité bloquée
à zéro, justification finale 1 contre 2 ; têtes de vote divergentes dès le slot 19.
Les 64 derniers votes observés diffèrent entre nœuds malgré des relais réussis.
[Contre-exemple et traces](qualification/capella-distributed-finality-20260927/README.md).
Cause non établie : priorité au suivi des votes acquittés jusqu’à leur application
différée/choix de branche. Le succès du rattrapage simple ne clôture pas ce blocage.

Liste ajoutée au goal à la demande de l'utilisateur le 24 septembre 2026.
Les statuts Ethereum ci-dessous sont ceux consultés sur eips.ethereum.org à
cette date. Ils ne signifient ni implémentation ni activation dans Nano.
Une proposition non finalisée reste dans le périmètre de travail, avec version
épinglée, tests dédiés et activation distincte du profil stable.

| EIP | Objet | Statut officiel vérifié |
| --- | --- | --- |
| [7251](https://eips.ethereum.org/EIPS/eip-7251) | Solde effectif accru et consolidation des validateurs | Final |
| [7002](https://eips.ethereum.org/EIPS/eip-7002) | Sorties et retraits déclenchés depuis la couche d'exécution | Final |
| [7685](https://eips.ethereum.org/EIPS/eip-7685) | Transport des requêtes exécution vers consensus ; dépendance de 7002/7251 | À suivre dans la spécification épinglée |
| [7917](https://eips.ethereum.org/EIPS/eip-7917) | Prévision déterministe des proposeurs ; dépendance de 8045 | À suivre dans la spécification épinglée |
| [8045](https://eips.ethereum.org/EIPS/eip-8045) | Exclusion des validateurs sanctionnés de la sélection des proposeurs | Review |
| [8068](https://eips.ethereum.org/EIPS/eip-8068) | Neutralité du solde effectif entre capitalisation et prélèvement des gains | Draft |
| [8363](https://eips.ethereum.org/EIPS/eip-8363) | Destruction progressive d'une part des récompenses selon le staking | Draft |
| [7805](https://eips.ethereum.org/EIPS/eip-7805) | FOCIL, listes d'inclusion imposées par le choix de branche | Draft |
| [7594](https://eips.ethereum.org/EIPS/eip-7594) | PeerDAS, disponibilité des blobs par échantillonnage ; dépend de 4844 | Final |
| [7732](https://eips.ethereum.org/EIPS/eip-7732) | ePBS, séparation native proposeur/constructeur | Review |
| [8141](https://eips.ethereum.org/EIPS/eip-8141) | Frame Transaction, évolution du format d'exécution/abstraction de compte | À suivre dans la spécification épinglée |

Le post-quantique est une exigence de recherche et de migration à préciser :
le texte fourni ne donne aucun numéro d'EIP déterminé. Ne pas inventer une
spécification, une date d'activation ou une protection déjà obtenue.

## Prérequis de consensus, au-delà des EIP

Les comités, leur rotation, SSZ, les domaines BLS, RANDAO, le registre,
Casper FFG/LMD-GHOST et le cycle de sanctions/sorties nécessitent une référence
versionnée aux [consensus-specs](https://github.com/ethereum/consensus-specs).
Les EIP seules ne constituent pas une implémentation de consensus.
La transposition des unités ETH en NANO et des paramètres économiques doit
être explicite, déterministe et versionnée, sans conversion implicite.

## Preuves requises avant clôture

Pour chaque élément : dépendances, version amont, différences Nano,
implémentation, vecteurs positifs/négatifs, intégration réseau, activation et
migration documentées. Un test EVM de requête n'établit pas que le registre
consensus applique cette requête. Un statut Final n'établit pas à lui seul la
sécurité du portage. Les versions expérimentales ne doivent pas être présentées
comme un profil Ethereum finalisé ou une production qualifiée.

## Compléments recherchés pour les validateurs

Priorité : compléter le socle de consensus et son cycle de vie avant les
extensions économiques expérimentales. Sources officielles consultées le
24 septembre 2026.

| EIP | Apport et travail à qualifier |
| --- | --- |
| [6110](https://eips.ethereum.org/EIPS/eip-6110) | Dépôts de validateurs fournis par la couche d'exécution ; admission et ordre des requêtes |
| [7549](https://eips.ethereum.org/EIPS/eip-7549) | Agrégation des attestations entre comités ; absence de double comptage et membership |
| [7044](https://eips.ethereum.org/EIPS/eip-7044) | Pérennité des sorties volontaires signées ; domaines de signature et rejeu |
| [7045](https://eips.ethereum.org/EIPS/eip-7045) | Fenêtre d'inclusion des attestations ; slots limites et attestations tardives |
| [4895](https://eips.ethereum.org/EIPS/eip-4895) | Retraits consensus vers exécution ; comptabilité et traitement unique |
| [4788](https://eips.ethereum.org/EIPS/eip-4788) | Racines de blocs beacon accessibles depuis l'EVM ; cohérence EL/CL |

La consolidation 7251 ne réduit pas nécessairement le nombre de machines :
un même opérateur peut déjà héberger plusieurs clés de validation par nœud.
La liste production/exécution est distincte : [EVM-PRODUCTION-EIP-ROADMAP.md](EVM-PRODUCTION-EIP-ROADMAP.md).

## Vérification des participations d'un comité — 24 septembre 2026

`ConsensusCommittee` relie les primitives de dépôt/PoP, domaines de signature
et agrégats BLS. Son constructeur copie un ensemble de membres, rejette indices
et clés dupliqués, poids nuls et preuves de possession invalides. `verify`
exige des indices triés, uniques et présents dans cet ensemble, vérifie la
signature sur la racine avec son domaine fixé puis retourne le poids signé et
le poids total en précision arbitraire. Aucun seuil de finalité n'est déduit.
Référence conceptuelle : [indexed attestations](https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#is_valid_indexed_attestation).

La source d'autorité demeure une précondition : le registre, l'éligibilité,
les poids effectifs, le domaine et le comité du slot doivent provenir de l'état
consensus authentifié. Une preuve de dépôt ne prouve pas les fonds et la racine
d'objet doit engager toutes les données du vote. Cette brique n'est pas encore
appelée par le réacteur BFT et ne remplace pas le cycle d'époques ou la finalité.
Les PoP sont actuellement vérifiées à la construction du snapshot ; l'admission
persistante devra éviter ce travail à chaque comité sans perdre sa garantie.

`committeeParticipationQualification` archive trois vérifications valides,
vingt rejets adversariaux, un total de poids supérieur à uint64 et l'indépendance
du snapshot vis-à-vis des mutations du tableau d'origine. Il s'agit de tests
locaux construits, pas d'une qualification Ethereum complète ou multi-hôte.

## Données d'attestation SSZ — 24 septembre 2026

`ConsensusAttestationData` engage le slot, l'index du comité, la racine du bloc,
les époques et racines des checkpoints source/cible. Sa racine SSZ correspond
aux cinq vecteurs `AttestationData/ssz_random/case_0..4` de la révision
`bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908` de consensus-spec-tests.
Les 35 mutations de champs changent les racines ; le test de comité signe
maintenant cette racine typée et rejette également les sept substitutions.
La nouvelle campagne `consensusAttestationQualification` conserve 27 rejets
BLS/admission en complément des vecteurs SSZ. La campagne précédente à 20
rejets est conservée sans modification.

Le calcul SSZ n'impose pas la fenêtre de slots, la justification/ascendance des
checkpoints ni la règle d'index spécifique au fork (notamment EIP-7549).
Ces contrôles et l'application à la finalité restent à intégrer avec l'état
consensus authentifié ; l'ancien réacteur BFT n'est pas changé par cette brique.

## Contradiction d'attestations — 24 septembre 2026

`is_slashable_attestation_data` reprend le prédicat Ethereum : données distinctes
avec la même époque cible, ou première attestation entourant strictement la
seconde (`source1 < source2` et `target2 < target1`). Le second cas est orienté ;
le traitement des preuves doit respecter cet ordre. Une répétition identique
ne constitue pas un double vote. 24 cas locaux, dont les bornes uint64, passent
avec les régressions SSZ/comité dans `attestationConflictQualification`.
Référence : [is_slashable_attestation_data](https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#is_slashable_attestation_data).

Cela n'applique aucune pénalité : les deux signatures, l'intersection des indices,
l'éligibilité à la sanction, sa comptabilité et sa persistance restent requis.
Les cas de test sont construits localement ; aucune conformité au corpus complet
d'opérations de slashing n'est revendiquée.

## Preuves de slashing authentifiées — 24 septembre 2026

`verify_attester_slashing` relie le prédicat de contradiction aux deux signatures
BLS, avec les snapshots/domaines historiques fournis par l'appelant. Les indices
sont vérifiés, leur intersection est calculée et chaque index commun doit garder
la même clé publique entre les deux contextes. Une intersection vide, une clé
réaffectée, une signature invalide ou des votes non contradictoires sont rejetés.
Trois preuves positives et dix rejets adversariaux passent, avec la régression
comité, dans `authenticatedSlashingQualification`.

L'appelant doit encore fournir les contextes issus d'un état consensus authentifié.
Le résultat identifie des signataires contradictoires ; il ne prouve pas leur
éligibilité actuelle à une sanction et ne modifie aucun solde. Le traitement
exactement une fois, le retrait différé, les règles économiques, la persistance
atomique et l'intégration au réacteur demeurent requis.

## Transition de slashing Phase0 — 24 septembre 2026

`process_phase0_attester_slashing` consomme la preuve authentifiée et un registre
courant fourni par l'appelant. Il vérifie la continuité des clés, filtre les
validateurs non encore sanctionnés entre activation et retrait, programme leur
sortie selon la file et le churn, prolonge le délai de retrait, enregistre le
solde effectif dans le bucket de slashing, puis applique pénalité et récompenses.
Les paramètres économiques sont explicites ; aucun profil réseau n'est activé.
La transition utilise les règles
[Phase0](https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#slash_validator),
pas les règles économiques Electra/Pectra.

Les modifications sont préparées dans une copie puis échangées sans exception.
Preuve invalide, rejeu sans nouveau signataire sanctionnable, état incohérent ou
dépassement uint64 ne doivent pas laisser de modifications partielles. Les
indices de sortie sont regroupés une fois pour éviter de rescanner le registre
pour chaque sanction ; la validation et la copie restent proportionnelles au
registre (avec tri des clés), sans qualification à un million pour ce chemin.

26 scénarios locaux et les régressions preuve/comité passent dans
`slashingTransitionQualification`. Les tests couvrent les limites d'éligibilité,
les récompenses au validateur sanctionné lui-même, plusieurs sorties, les
paramètres alternatifs et le rollback après une première modification préparée.
Ce ne sont pas des vecteurs officiels complets de transition BeaconState.

Restent requis : registre financé et contextes historiques authentifiés,
persistance atomique avec le bloc, rotation des buckets à chaque époque,
pénalités corrélées différées, cycle complet entrées/sorties, profil de fork
choisi et intégration au réacteur. La transition est actuellement en mémoire ;
elle n'est pas raccordée à `NanoStaking.sol` ni au BFT actif.

## Snapshot et reprise du slashing — 24 septembre 2026

Le format local `nano-phase0-slashing-v1` conserve les règles Phase0 attendues,
l'époque, les clés/indices, soldes, dates de sortie/retrait, marqueurs de sanction
et buckets de slashing. Le décodeur vérifie les quantités hexadécimales canoniques
uint64, l'ordre strict des indices, l'unicité des clés, les types et les dates.
Les règles de configuration doivent correspondre exactement à celles fournies
par l'appelant. Ce format JSON local n'est pas une racine SSZ de BeaconState.

`slashingRecoveryQualification` couvre 11 contrôles de roundtrip/reprise et
19 snapshots invalides. Un processus enfant écrit genesis puis un bloc et son
registre sanctionné avec `ChainStore::commit`, puis appelle `_exit(0)` sans fermer
RocksDB. Le parent récupère le tip, l'état EVM et les snapshots historiques,
vérifie les sanctions/récompenses et rejoue la preuve sans double prélèvement.
Un bloc rejeté ne crée pas de snapshot et ce résultat subsiste après réouverture.
L'identité de chaîne incompatible est également rejetée par le stockage.

Le commit utilise le WriteBatch synchrone existant, inchangé. Le test est un
harness de stockage avec des blocs construits localement, sans validation du
consensus PoW/BFT ; il n'active pas le registre dans `Blockchain`. Il ne couvre
ni interruption au milieu d'une écriture, ni panne électrique, ni engagement
cryptographique du registre dans l'en-tête, ni rejeu réseau de ses transitions.
Ces points restent requis avant de revendiquer la persistance du nouveau
consensus actif. Les trois suites preuve/transition/reprise passent.

## Comptabilité du slashing par époque — 24 septembre 2026

`advance_phase0_slashing_epoch` applique la pénalité corrélée différée puis
remet à zéro le bucket de l'époque suivante et avance ce registre d'une seule
époque. L'époque attendue doit correspondre exactement à l'état fourni : le
rejeu et les sauts sont rejetés sans mutation. Les agrégats sont calculés en
entiers GMP ; l'ordre des divisions entières et le plancher du total actif
suivent [process_slashings](https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#slashings).
La pénalité précède l'effacement du bucket, conformément au traitement
[process_slashings_reset](https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#slashings-balances-updates).

Le snapshot local est désormais **`nano-phase0-slashing-v2`** : il ajoute aux
règles l'incrément du solde effectif et le multiplicateur proportionnel. La v1
historique est rejetée, sans migration implicite. Aucun registre de ce format
n'est activé sur le réseau ou dans l'image VPS ; les preuves v1 précédentes
restent des archives de leur révision. Les paramètres ne sont pas des défauts
réseau et doivent provenir de la configuration authentifiée.

`slashingEpochQualification` archive 11 contrôles et 8 rejets atomiques :
parcours depuis une preuve signée jusqu'à la pénalité différée, restauration
snapshot sans répétition, arrondis, montant nul/plafonné, absence de stake actif,
sommes dépassant uint64 et configurations invalides. Les quatre suites
preuve/transition/reprise/époque passent. Il s'agit de cas locaux, pas d'une
qualification complète des vecteurs officiels BeaconState.

Cette fonction traite seulement le registre de slashing. Elle ne remplace pas
le traitement complet des époques : récompenses, hystérésis des soldes effectifs,
entrées/sorties complètes, RANDAO, justification/finalité restent requis.
L'appelant doit ordonner ces étapes selon le fork et persister leur résultat
avec le bloc ; l'horloge de slots et le réacteur actif ne l'appellent pas encore.

## Vecteurs officiels de pénalités Phase0 — 24 septembre 2026

Les cinq cas du groupe `minimal/phase0/epoch_processing/slashings/pyspec_tests`
de [consensus-spec-tests](https://github.com/ethereum/consensus-spec-tests/tree/bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908/tests/minimal/phase0/epoch_processing/slashings/pyspec_tests)
sont importés avec fichiers Snappy originaux, SSZ bruts, licence et empreintes.
Le lecteur C++ du schéma minimal compare les 320 soldes attendus, dont 90
modifiés, et tous les champs de la projection du registre : aucun écart.
Les paramètres minimal proviennent de fichiers consensus-specs eux-mêmes
épinglés et hachés dans le manifeste local.

Le premier essai via la transition complète rejetait ces fixtures : certains
pré-états ont une date de retrait finie et une date de sortie FAR_FUTURE_EPOCH.
Ces états isolent le composant `process_slashings`. Le calcul pur est maintenant
exposé par `phase0_slashing_penalties`, utilisé aussi par la transition validée.
Les attentes officielles ne sont pas réécrites et les contrôles de cohérence
du registre restent dans `advance_phase0_slashing_epoch`. Les pré/post-états
complets d'époque (`pre_epoch`, `post_epoch`) ne sont pas inclus dans cette preuve.

`officialSlashingQualification` conserve le premier rejet, les cinq suites
passantes et les contrôles négatifs : solde attendu altéré avec empreinte mise
à jour, puis rapport avec un solde comparé manquant. Le premier est rejeté par
le binaire C++, le second par le vérificateur de rapport. Le lecteur SSZ est
spécifique au test ; il ne constitue pas le décodeur réseau d'un BeaconState.
La qualification ne couvre pas encore le consensus actif ni un cycle d'époque
complet. Le registre ouvert, RANDAO, finalité et activation restent à intégrer.

## RANDAO authentifié et graines — 24 septembre 2026

`ConsensusRandaoState` conserve l'époque, le ring de mixes et le dernier slot
ayant apporté une révélation. `process_consensus_randao` vérifie une signature
BLS sur la racine SSZ de l'époque, avec DOMAIN_RANDAO et le fork/genesis fournis,
puis applique SHA-256 de la révélation par XOR au mix courant. Les slots répétés,
antérieurs ou hors époque sont rejetés sans mutation. Une signature d'époque
peut être réutilisée à un autre slot si son proposant est autorisé : il ne faut
pas dédupliquer les signatures indépendamment des blocs. Référence :
[process_randao](https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#randao).

La rotation copie le mix courant vers l'époque suivante, avec contrôle de
séquence. `consensus_randao_seed` dérive les graines des devoirs actuels/suivants
à partir du mix retardé, du type de devoir et de l'époque encodée little-endian,
selon [get_seed](https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#get_seed).
Des devoirs historiques demandent leur snapshot historique ; cette API refuse
d'utiliser silencieusement un ring devenu trop récent. Les longueurs de ring,
slots par époque et lookahead doivent provenir du profil authentifié.

`randaoQualification` archive 13 contrôles et 19 rejets, dont quatre valeurs de
graine SHA-256 calculées indépendamment, les bornes uint64, la rotation,
l'isolation des devoirs immédiats et une permutation utilisant la graine obtenue.
Les régressions BLS (29 vecteurs officiels) et racines de signature passent.
Les scénarios RANDAO sont locaux, pas le corpus officiel d'opérations RANDAO.

L'appelant doit encore authentifier le proposant sélectionné et son éligibilité,
le fork et le genesis, valider le bloc, puis persister mix et watermark avec
l'état consensus. Ces fonctions ne sont pas appelées par le réacteur actif et
ne modifient pas `PREVRANDAO` sur le réseau. La persistance du ring, l'horloge des
slots, le registre ouvert et le rattachement aux comités/finalité restent ouverts.

## RANDAO des blocs officiels Phase0 — 24 septembre 2026

`officialRandaoQualification` utilise les scénarios `empty_block_transition`,
`empty_epoch_transition` et `skipped_slots` du corpus
[consensus-spec-tests](https://github.com/ethereum/consensus-spec-tests/tree/bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908/tests/minimal/phase0/sanity/blocks/pyspec_tests)
minimal Phase0. Les trois révélations BLS passent ; les 192 mixes finaux,
l'époque et le dernier slot correspondent aux états post officiels. Quinze
mutations locales de signature, fork/genesis, clé ou répétition de slot sont
rejetées. Les fichiers Snappy originaux et SSZ bruts sont hachés et conservés.
Le lecteur SSZ minimal de test est partagé avec la qualification du slashing.

Les indices des proposants sont lus dans les blocs officiels et leurs clés
dans le pré-état. Leur sélection n'est pas recalculée. Le test n'exécute ni les
autres opérations des blocs, ni leur signature/en-tête, ni la racine complète
du BeaconState. Les cas officiels sont positifs ; les rejets malveillants sont
locaux. La preuve ne doit pas être présentée comme conformité complète des
blocs ou des époques. Les trois suites RANDAO local/officiel et slashing officiel
passent. Un mix post attendu altéré avec empreinte mise à jour est rejeté par
le binaire ; un rapport annonçant 191 mixes au lieu de 192 est aussi rejeté.
La sélection des proposants, la persistance et le consensus actif restent ouverts.

## Sélection pondérée du proposant Phase0 — 24 septembre 2026

`phase0_proposer_index` filtre les validateurs actifs à l'époque demandée,
dérive la graine du slot depuis le RANDAO et applique l'échantillonnage pondéré
Phase0. `shuffled_validator_index` calcule un index sans construire une permutation
complète à chaque essai. La comparaison des poids utilise un intermédiaire
128 bits ; les paramètres du profil sont explicites. Référence :
[compute_proposer_index](https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#compute_proposer_index).

`proposerQualification` recalcule les trois indices des proposants officiels
utilisés par la qualification RANDAO, au lieu de les accepter comme donnée.
Il vérifie 98 comparaisons locales pondérées, 10 rejets, l'annulation et la
correspondance scalaire avec les 240 permutations officielles. Un index de
proposant falsifié dans une fixture est rejeté même après mise à jour de son
empreinte. Les trois suites passent.

La sélection locale sur registres de 100 000 et 1 000 000 entrées, toutes au poids
maximal, a pris respectivement environ 0,53 ms et 3,92 ms sur ce Mac. La mesure
inclut le filtrage du registre, exclut sa construction, la communication réseau,
la validation des signatures et les commits disque. Ce n'est ni un million de
nœuds, ni une preuve de latence sous poids hétérogènes/charge. Le parcours initial
reste O(N) et alloue une liste des actifs ; un registre d'époque authentifié
mis en cache reste à intégrer. Le callback d'annulation interrompt le travail,
mais une interruption ne doit pas devenir un verdict de bloc invalide.

Ce profil Phase0 ne retire pas silencieusement les validateurs slashed du tirage :
la validation du bloc doit encore rejeter un proposant sanctionné selon ce fork.
Les changements Electra et EIP-8045 ne sont pas revendiqués. Le registre fourni
doit être financé et authentifié ; persistance, horloge et réacteur actif ne
sont pas encore raccordés à cette sélection.

## Contrôle commun proposant/RANDAO — 24 septembre 2026

`process_phase0_selected_randao` relie le registre de validateurs, la sélection
pondérée et la révélation BLS. Le registre et le RANDAO doivent désigner la même
époque. La clé publique n'est pas fournie séparément : elle est résolue depuis
l'index sélectionné dans le registre. Le candidat doit être actif et non slashed,
son index doit correspondre au tirage, et la révélation doit passer la vérification
BLS. Le slot rejoué, l'absence de validateur et les révélations malformées sont
rejetés avant mutation. Un proposant sanctionné n'entraîne pas un nouveau tirage.

`selectedRandaoQualification` couvre trois contrôles positifs et douze rejets
locaux. Le cas déterminant utilise une signature d'époque valide d'un autre
validateur inscrit : la primitive BLS/RANDAO seule l'accepte avec sa clé, tandis
que le contrôle commun la refuse pour ce slot. Les erreurs et l'annulation
laissent le mix intact. Les trois projections officielles passent par ce même
contrôle, avec six rejets supplémentaires (mauvais index et proposant slashed).
Les quatre suites passent ; 192 mixes officiels restent identiques.

Pour la fixture qui franchit une époque, les champs du registre utilisés par
le proposant sont vérifiés identiques entre pré/post-états ; le test aligne
ensuite l'époque de la projection. Il ne prétend pas exécuter le cycle complet
de cette époque. Le registre financé/authentifié, l'en-tête, la signature de
bloc, le corps et les engagements d'état restent à valider, puis à committer
avec le mix. Le réacteur actif n'appelle pas encore ce contrôle. Cette jonction
ne vaut donc ni qualification d'un bloc complet, ni activation réseau.

## Engagement SSZ et signature du bloc — 24 septembre 2026

`consensus_beacon_header_root` calcule la racine SSZ du conteneur BeaconBlockHeader
(slot, index du proposant, parent, état, corps), distinct de l'en-tête RLP
exécution de Nano. `verify_consensus_beacon_header` vérifie la signature BLS
avec DOMAIN_BEACON_PROPOSER, version de fork et racine genesis fournis.
La clé doit provenir d'un registre authentifié ; cette primitive ne valide pas
à elle seule le devoir du proposant, le parent ou la transition d'état.

`beaconHeaderQualification` conserve cinq racines officielles et 25 mutations
de champs, trois signatures de blocs officiels et 33 rejets (champs, fork,
genesis, clé, signature altérée/tronquée et substitution par une révélation
RANDAO). Les cinq suites liées passent. Le rapport falsifié et une racine
attendue volontairement incorrecte sont rejetés. Les fixtures proviennent de
`ethereum/consensus-spec-tests` à la révision
`bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908` ; les cinq cas statiques sont
`mainnet/phase0/ssz_static/BeaconBlockHeader/ssz_random/case_0..4`.
Les valeurs importées gardent les chemins et SHA-256 des YAML sources ; la
licence MIT amont est conservée dans `tests/fixtures/consensus-randao-official/LICENSE`.

Limite explicite : pour les trois signatures, la racine du corps provient du
latest_block_header du post-état officiel d'un scénario à un seul bloc. Le test
ne recalcule pas le corps SSZ complet. Il prend la racine d'état dans le message
signé, sans prouver cette transition. Calcul du corps, vérification du parent,
transition complète, jonction atomique avec RANDAO et intégration au réacteur
restent nécessaires. Aucune activation ni qualification complète d'EIP.
Référence : https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#verify_block_signature

## Calcul complet du corps SSZ Phase0 — 24 septembre 2026

`phase0_beacon_body_root` décode strictement le corps SSZ et calcule son
engagement : révélation RANDAO, Eth1Data, graffiti, proposer/attester slashings,
attestations avec bitlists, dépôts avec preuves et sorties signées. Offsets
canoniques, tailles fixes, bornes de listes et délimiteurs de bitlist sont
contrôlés. Les bornes sont fournies par le preset appelant ; elles ne limitent
pas la population totale des validateurs. Le merkle padding n'alloue pas la
capacité maximale configurée. Le transport devra toujours borner les messages
avant ce calcul ; ce composant ne remplace pas les quotas réseau.

`beaconBodyQualification` conserve cinq racines de corps officiels aléatoires
(20 proposer slashings, 6 attester slashings, 24 attestations, 14 dépôts,
21 sorties), 25 mutations de champs fixes, 320 mutations d'octets, 261 rejets de
structures invalides et 13 cas aux limites. Six suites passent. Le décodeur et
le hachage SSZ nouvellement ajoutés sont aussi compilés avec AddressSanitizer
et UndefinedBehaviorSanitizer : même campagne, aucune erreur détectée. Cela
ne constitue pas une qualification de l'ensemble du binaire sous sanitizers.

Les trois signatures de blocs du corpus RANDAO utilisent désormais une racine
de corps calculée depuis les octets reçus, puis comparée au post-état officiel.
Cela lève la limite de la campagne historique `beaconHeaderQualification`,
conservée sans modification. Les cinq fixtures sont épinglées à la même révision
`bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908` du corpus officiel.

Périmètre : schéma Phase0 uniquement, pas les corps Altair/Bellatrix et suivants.
Une opération SSZ bien formée n'est pas nécessairement autorisée : signatures
internes, appartenance, preuves de dépôt, compteurs, transition d'état, parent,
jonction atomique avec RANDAO et activation du réacteur restent à vérifier.
Référence : https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#beaconblockbody

## Préparation authentifiée d'un bloc Phase0 — 24 septembre 2026

`authenticate_phase0_block` relie l'enveloppe SSZ signée, le corps recalculé,
le parent/slot attendus, la signature du proposant issu du registre, son devoir
pondéré et sa révélation RANDAO. Une limite d'octets fournie par la configuration
est vérifiée avant le calcul du corps. Les entrées sont constantes ; un succès
retourne `AuthenticatedPhase0Block` avec un mix candidat, jamais un commit.
Les données malformées ou non autorisées retournent `nullopt`. Les erreurs de
configuration et l'annulation restent des exceptions distinctes.

`blockAuthenticationQualification` : trois blocs officiels épinglés passent,
54 cas invalides et 404 enveloppes tronquées sont rejetés. Quatre contrôles
locaux couvrent le succès, un bloc réellement signé par un autre validateur,
un bloc réellement signé avec révélation invalide et le dépassement de taille.
Sept erreurs de configuration sont détectées. Les 21 points d'annulation
observés sont chacun déclenchés, jusqu'après le calcul du candidat, sans changer
le registre ou le RANDAO source. Six suites passent, dont `block_auth` existante.

Dans les fixtures officielles, le parent attendu vient du tableau block_roots
du post-état après traitement des slots ; ce test ne reconstruit pas encore ce
traitement. Pour le saut d'époque, les champs du registre nécessaires au tirage
sont vérifiés inchangés et la rotation RANDAO est effectuée séparément.
Le contexte, le registre financé, le fork et les limites doivent être fiables.

Ce résultat est une authentification provisoire, pas un bloc valide : signatures
et sémantique des opérations, état résultant, transition de slots/époques,
persistance atomique et raccordement au réacteur restent à intégrer. Le schéma
est Phase0 uniquement. Aucune activation réseau ni finalité nouvelle.

## Double proposition signée et sanction — 24 septembre 2026

`verify_proposer_slashing` authentifie deux en-têtes différents, signés avec la
même clé de registre pour le même slot et le même index. Le fork historique
pour l'époque de la preuve et le genesis sont fournis par le contexte fiable.
`process_phase0_proposer_slashing` vérifie l'éligibilité actuelle et applique
la même transition économique atomique que les preuves d'attestation : pénalité,
file de sortie, retrait différé, bucket et récompenses. Le code économique est
mutualisé ; les régressions d'attestation, reprise et pénalité différée passent.

`proposerSlashingQualification` couvre les quinze cas officiels Phase0 minimal,
révision `bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908` : trois succès et douze
rejets. Pour les succès, la projection complète des registres/buckets et 192
soldes concordent avec les post-états officiels. Le proposant sanctionné peut
être lui-même bénéficiaire de la récompense, conformément à la fixture.
Une paire d'en-têtes d'un slot futur peut être sanctionnable : le test officiel
le demande ; cette vérification n'ajoute pas d'interdiction temporelle arbitraire.

Un des douze rejets concerne un pré-état volontairement incohérent : retrait à
l'époque zéro avec sortie à FAR_FUTURE. Le premier passage du runner échouait
sur `invalid slashing registry` ; le contrôle de production n'a pas été affaibli.
Le test reconnaît explicitement ce cas amont et le compte séparément comme
rejet de registre, pas comme échec cryptographique.

Six rejeux (immédiats et après aller-retour du snapshot) sont refusés sans
nouveau prélèvement ; douze mutations locales de domaine/preuve sont refusées ;
trois débordements de bucket laissent le registre intact. Sept suites passent.
Le snapshot testé ici est un aller-retour du codec ; la reprise RocksDB existante
est une régression séparée. Le traitement des opérations dans le bloc, la
validation de l'état complet et l'activation réseau restent ouverts. Économie
Phase0 uniquement, pas conformité Electra/EIP-7251.
Référence : https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#proposer-slashings

## Sanctions raccordées au bloc signé — 24 septembre 2026

`prepare_phase0_proposer_operations` authentifie le bloc et traite ses proposer
slashings, dans leur ordre, sur des candidats privés de registre/RANDAO. Le
bénéficiaire des récompenses vient du proposant authentifié. Le fork d'état
(previous/current/epoch) sélectionne le domaine des preuves historiques ; le
contexte de signature du bloc doit concorder. Le résultat expose explicitement
les nombres d'attester slashings, attestations, dépôts et sorties non traités.
Il ne doit jamais être publié comme état canonique à ce stade.

`blockOperationsQualification` couvre quatre succès locaux, sept rejets,
douze annulations et un débordement après une première sanction valide.
Un second slashing invalide ou répété invalide le candidat entier, sans publier
la première sanction ni le mix. Le test inclut une ancienne version de fork,
la sanction du proposant du bloc lui-même et une sortie non validée signalée
par le résultat. Les entrées restent inchangées même après succès.

Cinq scénarios officiels de blocs Phase0 minimal épinglés complètent ces tests :
trois succès (cinq sanctions, 192 soldes et RANDAO conformes aux post-états) et
deux rejets de preuves répétées. Ces deux blocs passent d'abord l'authentification
SSZ/BLS et échouent bien au traitement des opérations. Les métadonnées confirment
un seul bloc par scénario. Six suites passent ; les contrôles négatifs du rapport
et du post-état attendu sont archivés.

Limites : ce composant ne traite pas le vote Eth1, les autres opérations,
slots/époques ni la racine d'état complète. Les parents des cas valides sont
comparés aux block_roots officiels ; les cas invalides fournissent le parent du
bloc comme contexte pour isoler le rejet tardif. Le commit durable, les autres
forks et le réacteur restent ouverts. Les copies/scans du registre par preuve
ne constituent pas une qualification de débit à un million de validateurs.

## Sorties volontaires Phase0 — 24 septembre 2026

`process_phase0_voluntary_exit` vérifie le message SSZ signé (époque/index),
DOMAIN_VOLUNTARY_EXIT, la clé du registre, l'activité, l'ancienneté minimale,
l'époque demandée et l'absence de sortie antérieure. Le fork du domaine doit
correspondre à l'époque signée et provenir du contexte fiable. La file de sortie
est partagée avec les sanctions : capacité par époque calculée depuis la
population active, délai d'activation de sortie et retrait différé. La transition
est préparée sur copie puis publiée en mémoire, sans débit ni paiement.

`voluntaryExitQualification` conserve les dix cas officiels épinglés Phase0
minimal : quatre succès, six rejets. Les 320 enregistrements de validateurs et
les buckets concordent avec les post-états officiels ; les soldes restent
inchangés. Les cas valides couvrent la sortie simple, une sortie ultérieure,
le churn minimum et le churn calculé selon le nombre d'actifs. Quatre rejeux,
24 mauvaises signatures/domaines/requêtes et quatre débordements d'époque
sont refusés sans mutation. Sept suites passent, y compris les sanctions et
leurs traitements dans des blocs officiels, après mutualisation de la file.

Ce composant ne réalise ni versement des fonds ni transition complète de bloc.
Intégration aux opérations signées, epoch processing et commit réseau restent
à terminer. Il n'implémente pas encore EIP-7044 (domaine Capella durable) ou
EIP-7002 (demande par clé de retrait) et ne change pas leur statut dans la matrice.
Référence : https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#voluntary-exits

## EIP-7044 : domaine Capella durable — 24 septembre 2026

`process_consensus_voluntary_exit` sélectionne explicitement le profil depuis
un contexte d'état/configuration fiable. Phase0 garde le domaine correspondant
à l'époque signée. Deneb fixe la version à `capella_version` configurée, sans
utiliser les versions previous/current pour signer les sorties. Genesis reste
dans le domaine. Aucun changement du moteur EVM n'est requis par cet EIP.

`denebExitQualification` couvre les seize scénarios officiels Deneb minimal
épinglés : six succès, dix rejets et 448 enregistrements conformes aux post-états.
Deux cas officiels distinguent les profils : une signature Capella reste valide
là où l'ancienne règle sélectionnerait Deneb, tandis qu'une signature Deneb
acceptée par l'ancienne règle doit être refusée. Six changements locaux des deux
versions de fork conservent la validité du domaine Capella. Ils ne simulent pas
une transition complète de fork. Six rejeux, douze mauvais domaines/genesis et
douze contextes inconnus/futurs sont refusés sans mutation. Six suites passent,
y compris les dix fixtures Phase0 via le même sélecteur de profil.

La matrice passe EIP-7044 de non commencé à partiel. Il reste à raccorder ce
sélecteur au traitement des blocs et au gossip, puis à qualifier l'activation
réseau. Le lecteur SSZ Deneb ajouté est uniquement un outil de test des champs
consommés, pas un client Deneb complet. La file actuelle ne prétend pas appliquer
les changements économiques Electra. Sur deux branches partageant genesis et
version Capella, la réutilisation d'une sortie signée reste possible par
conception de l'EIP ; ce n'est pas un transfert des fonds.
Source : https://eips.ethereum.org/EIPS/eip-7044

## Admission par dépôt prouvé — 24 septembre 2026

`process_phase0_deposit` vérifie la racine SSZ DepositData, les 33 niveaux de
preuve Merkle et la position attendue avant admission. La racine/count Eth1
et le registre doivent être authentifiés par l'appelant. Une fausse preuve
est rejetée avant parcours ou copie du registre. Une nouvelle clé exige une
preuve de possession BLS dans le domaine genesis ; un dépôt valide ajoute une
entrée inactive avec solde effectif arrondi/plafonné selon les règles explicites.
La limite de registre vient du preset SSZ, pas d'un plafond de 1 000 validateurs.

Une recharge d'une clé déjà admise modifie seulement son solde : ni les nouveaux
credentials ni une signature de recharge invalide ne remplacent l'identité
existante. Pour une nouvelle clé à signature invalide mais preuve de dépôt
valide, la position est consommée sans créer d'entrée, conformément à Phase0.
Les erreurs de preuve ne consomment rien ; les débordements ne publient aucune
modification. Une admission ne déclenche pas immédiatement l'activation.

`depositTransitionQualification` couvre les dix-sept scénarios officiels épinglés :
six admissions, cinq recharges, quatre signatures ignorées et deux preuves
rejetées. Les 966 enregistrements comparés incluent credentials et époque
d'éligibilité ; root/count/cursor concordent aussi. Quinze snapshots et rejets
de position déjà consommée, 45 mutations de branches/données, onze débordements
et douze snapshots invalides sont vérifiés. Onze suites de régression passent.

Le registre conserve désormais withdrawal_credentials et activation_eligibility_epoch.
Son snapshot local passe à `nano-phase0-slashing-v3` ; v1/v2 sont rejetés, sans
migration automatique ni modification des archives historiques. Les appels à
ce codec ne sont pas raccordés au réacteur actif. `nano-phase0-deposit-v1`
conserve ensemble registre, racine, nombre et compteur de dépôts, avec les règles
liées au snapshot. La régression de reprise RocksDB du registre v3 passe ; les
nouveaux snapshots de dépôt sont qualifiés par aller-retour du codec, pas encore
par une campagne de panne au milieu d'un bloc avec dépôt.

Restent : authentification/évolution de la racine Eth1, traitement des dépôts dans
les blocs, activation/éjection par époque, état complet et commit réseau. Le
mécanisme est Phase0 ; EIP-6110 et les files de dépôts Electra ne sont pas promus.
Source : https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#deposits

## Intégration des opérations du registre dans les blocs — 25 septembre 2026

`prepare_phase0_registry_operations` compose l'authentification du bloc/RANDAO,
les sanctions de proposants, les dépôts ordonnés puis les sorties volontaires.
Un candidat privé contient le registre, la racine/count/cursor des dépôts et le
RANDAO préparé. Aucun état d'entrée n'est modifié, y compris lors d'un échec tardif,
d'un débordement ou d'une annulation. Le nombre de dépôts obligatoire est vérifié ;
les paramètres de la file de sortie doivent être communs aux sanctions et sorties.
Les domaines de sortie Phase0 dépendent de l'époque signée, non du bloc courant.

Le résultat distingue candidat, bloc invalide et opérations non prises en charge.
La présence de sanctions d'attestateurs ou d'attestations empêche de produire un
candidat : leurs effets ne peuvent pas être sautés avant dépôts/sorties. Ce statut
ne signifie pas que le bloc est invalide selon Ethereum.

`registryBlockOperationsQualification` : huit suites passent. Les tests composés
locaux couvrent trois cas positifs, dix rejets, trois combinaisons non prises en
charge, dix-neuf annulations, trois configurations invalides et un débordement
atomique. Un bloc combine sanction, recharge, admission et deux sorties : la file
partagée respecte le churn et l'admission reste inactive. Un dépôt prouvé à PoP
invalide consomme sa position sans admettre de clé et permet les sorties suivantes.

Six scénarios de blocs officiels sont exécutés : trois valides (193 entrées du
registre comparées, RANDAO et compteur de dépôt conformes), trois invalides qui
passent l'authentification puis échouent aux opérations. Deux séquences officielles
supplémentaires sont conservées mais différées explicitement car leur second bloc
traverse une époque : `voluntary_exit`, `multiple_different_validator_exits_same_block`.
Elles ne comptent pas comme réussies.

Limites : racine/count après vote Eth1 fournis par l'appelant, votes Eth1 non traités,
attestations et sanctions d'attestateurs non intégrées, transition d'époque et
state_root complets absents, pas de commit durable du bloc ni raccordement au
réacteur actif. Le pipeline est Phase0, pas Deneb/Electra ; EIP-7044 reste qualifié
séparément. Les copies et parcours par opération ne qualifient pas le débit d'un
registre à un million de validateurs. Clôture : compléter ces étapes, débloquer les
deux séquences d'époque et qualifier le commit/rejeu réseau de l'état complet.
Source : https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#operations

## Registre à la frontière d'époque — 25 septembre 2026

`prepare_phase0_registry_updates` implémente le composant Phase0 de mise à jour
du registre : éligibilité à l'époque suivante pour un solde effectif maximal,
éjection des validateurs actifs au seuil configuré, activation différée des
entrées dont l'éligibilité est finalisée, tri par époque d'éligibilité puis index
et churn dérivé des paramètres et du nombre de validateurs actifs. Le tri partiel
sélectionne les premiers candidats sans plafond arbitraire de taille du registre.
La file de sortie est la même que pour les sanctions et sorties volontaires.
Le composant ne modifie ni les fonds ni le solde effectif.

`registryEpochQualification` : tous les quinze cas officiels Phase0 sont couverts,
quatorze états conformes (1 152 entrées comparées) et un débordement attendu rejeté.
Dix-neuf éligibilités, dix-neuf activations et trente-sept éjections programmées,
quatorze snapshots conformes, soixante-huit annulations sans mutation de la source
et dix rejets locaux de configurations/états/débordements. Neuf suites passent.

Mesure locale bornée du composant sur ce Mac, build Release : 100 000 entrées en
17 078 µs, 1 000 000 en 120 190 µs ; pics RSS du processus respectivement 56 033 280
et 491 880 448 octets. Entrées synthétiques avec clés uniques supposées authentifiées,
moitié actives/moitié en attente ; limites de churn et ordre contrôlés sur toutes
les entrées. Le temps mesure la transition, pas la construction/vérification du
jeu de données. RSS = maximum du processus, pas allocation incrémentale. Une seule
mesure par taille, sans validation BLS ni réseau : zéro nœud distribué, aucun débit
réseau ou capacité de consensus à un million de validateurs n'est démontré.

Le résultat reste un candidat privé à intégrer exactement une fois dans la
transition d'époque complète. L'époque finalisée doit être authentifiée par le
consensus ; elle n'est pas calculée ici. Ni incrément d'époque, marqueur de rejeu,
récompenses/pénalités complètes, mise à jour des soldes effectifs, state_root ni
commit durable ne sont fournis par cette fonction. Les deux séquences multi-époques
précédemment différées restent ouvertes. Profil Phase0 : pas de consolidation
EIP-7251, de churn en solde Electra ni d'activation sur le réseau Nano.
Source : https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#registry-updates

## Justification et finalisation Phase0 — 25 septembre 2026

`prepare_phase0_justification_finalization` calcule les checkpoints justifiés et
finalisés et les quatre bits de justification depuis l'état authentifié. Il
reconstruit les comités des époques précédente/courante depuis le registre actif,
les mixes RANDAO et les paramètres de shuffle ; les bitlists désignent les
participants dans ces comités. Les cibles ne correspondant pas à l'historique sont
ignorées, les participants répétés réunis sans compter deux fois leur poids et
les validateurs sanctionnés exclus du poids attestant. Le dénominateur utilise
les validateurs actifs de l'époque courante, y compris les sanctionnés, comme
Phase0 ; les attestations de l'époque précédente peuvent inclure des sortants.
Les poids utilisent des entiers larges et le seuil exact 3 × participation ≥
2 × solde actif. Les quatre règles de finalisation et les deux époques initiales
sont traitées ; aucun candidat ne modifie ses snapshots d'entrée.

`phase0FfgQualification` couvre les dix scénarios officiels du corpus épinglé :
173 attestations en attente, checkpoints et bits conformes dans tous les cas,
y compris soutien insuffisant, mauvaise cible et validateurs sortants. Contrôles
supplémentaires : dix duplications, dix exclusions des sanctionnés, dix mutations
de cibles, quatre contrôles de seuil/genesis (dont sommes au-delà de uint64),
dix-sept contextes invalides et 226 annulations sans mutation. Sept suites passent
avec les composants de shuffle, RANDAO, sélection du proposant et registre d'époque.

Frontière de confiance : les attestations en attente proviennent de l'état déjà
authentifié ; elles ne portent plus de signatures et ne sont pas une entrée réseau.
La validation BLS, source et inclusion des attestations dans les blocs reste à
raccorder avant ce composant. Finalité réseau, LMD-GHOST, transition d'époque
complète, state_root, garde de rejeu et commit durable ne sont pas démontrés.
Le composant ne remplace pas le BFT actif. Les parcours/shuffles n'ont pas fait
l'objet d'une mesure de finalité à un million de validateurs dans cette campagne.
Clôture : intégrer l'admission authentifiée, l'ordre d'époque et la persistance,
puis confronter des séquences complètes et des partitions/reprises réseau.
Source : https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#justification-and-finalization

## Admission des attestations et composition des blocs — 25 septembre 2026

`process_phase0_attestation` vérifie époque/slot, délai d'inclusion Phase0,
checkpoint source, index/bitlist du comité, présence de participants et signature
BLS agrégée dans le domaine du fork de l'époque cible et du genesis. Les clés
viennent du registre authentifié, dont financement et PoP (y compris genesis)
doivent avoir été établis lors de l'admission. `phase0_attestation_duties` est
partagé avec FFG : registre contigu, clés uniques, comités dérivés de RANDAO.
L'attestation n'apporte ni sa liste de validateurs ni leurs poids.

Seule une admission réussie ajoute à la liste précédente/courante ; le délai et
l'index du proposant sélectionné sont conservés pour les récompenses d'époque.
Les participants sanctionnés peuvent signer une inclusion valide, mais FFG exclut
leur poids. Une mauvaise cible/tête reste admissible si signée : c'est le filtrage
FFG qui écarte les cibles non concordantes. Les doublons sont admissibles dans la
liste, sans compter deux fois leur poids dans FFG. Limite de liste issue du preset.

`prepare_phase0_registry_operations` accepte maintenant un `Phase0AttestationStage` :
après authentification/RANDAO et sanctions des proposants, les attestations sont
vérifiées sur des candidats privés avant dépôts et sorties. Une signature invalide
ou une sortie invalide tardive ne publie ni attestation ni modification du registre.
Sans ce contexte, les attestations restent explicitement non prises en charge ;
les sanctions d'attestateurs restent non prises en charge dans tous les cas.

`attestationInclusionQualification` : 41 scénarios officiels, 17 états conformes et
24 rejets attendus ; 85 mutations rejetées, 17 répétitions d'inclusion conformes,
17 limites de listes, 70 annulations et 8 contextes invalides. Un bloc signé local
combine sanction/attestation/dépôts/sorties, avec deux échecs, 29 annulations et
deux configurations invalides (y compris bloc vide). Sept suites passent, dont
les dix cas officiels FFG après partage du code de comités et ajout des métadonnées.

Les séquences officielles `attestation` et `duplicate_attestation_same_block`
(slots 9 et 17) sont conservées entièrement mais explicitement différées : elles
traversent une époque et ne comptent pas comme blocs conformes. Restent les
sanctions d'attestateurs, votes Eth1, récompenses/autres opérations d'époque,
state_root et commit/reprise du bloc complet, puis raccordement au réacteur.
La reconstruction des comités/proposants par attestation n'est pas encore optimisée
par un cache d'époque ni qualifiée à un million de validateurs. Profil Phase0,
aucune activation Deneb/Electra ou finalité réseau n'est annoncée.
Source : https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#attestations

## Sanctions d'attestateurs et cinq types d'opérations — 25 septembre 2026

`process_phase0_registry_attester_slashing` authentifie les deux attestations
indexées depuis les clés du registre financé/PoP préalablement validé. Les indices
sont non vides, triés, uniques, présents et bornés par le preset SSZ ; chaque
signature utilise le fork de son époque cible. Les fenêtres d'inclusion et
l'appartenance au comité courant ne sont pas des conditions de cette preuve de
faute. Les preuves anciennes/futures restent traitées selon Phase0. Seuls les
signataires communs encore sanctionnables sont punis, dans l'ordre des indices,
avec file de sortie, pénalité, récompenses et registre des sanctions partagés.
Annulation, débordement et erreur ne publient aucune sanction partielle.

Une distinction explicite conserve la validation de l'état :
`phase0_attester_slashing_component` calcule la transition isolée sans valider la
chronologie globale. Le scénario officiel valide `already_exited_long_ago`
utilise des retraits à l'époque 2 et des sorties aux époques 5/6. Son résultat
isolé concorde, tandis que le chemin contrôlé le rejette. Le traitement des blocs
appelle exclusivement le chemin contrôlé ; aucune option de relâchement n'y est
exposée. Les contrôles de chronologie des snapshots/autres sanctions sont conservés.

`attesterSlashingQualification` : trente cas officiels isolés (neuf états conformes,
21 preuves invalides), 576 entrées comparées et 37 validateurs sanctionnés. Huit
cas valides concordent aussi sur le chemin contrôlé ; le neuvième est le cas
synthétique rejeté décrit ci-dessus. Seize rejets de rejeu avant/après snapshot,
48 mutations, huit débordements et vingt annulations sont vérifiés.

Dans les blocs, les sanctions d'attestateurs sont exécutées après les sanctions
de proposants et avant les attestations/dépôts/sorties. Quatre blocs officiels
sont qualifiés : trois projections conformes (192 entrées, cinq preuves), un
doublon rejeté après authentification. Les recouvrements partiels ne débitent pas
deux fois les mêmes participants. Un bloc local compose les cinq types d'opérations,
avec deux rejets, 36 annulations et un débordement tardif sans mutation des sources.
Onze suites de régression passent. Les compteurs de l'ancien scénario local de
registre deviennent 11 rejets/2 non-pris-en-charge : les preuves indexées vides
sont désormais rejetées ; seules les attestations sans contexte demeurent non
prises en charge. Les anciennes campagnes restent archivées sans réécriture.

Cela complète les types d'opérations Phase0 dans le pipeline partiel, pas la
validation du bloc entier : votes Eth1, slots/époques complètes, state_root,
commit/reprise durable et réacteur réseau restent ouverts. Les séquences
multi-époques précédemment différées ne sont pas promues. Aucun fork moderne ni
EIP-7549/Electra n'est déclaré activé par cette étape.
Source : https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#attester-slashings

### Soldes effectifs Phase0 — composant d'époque qualifié

`prepare_phase0_effective_balances` applique les seuils d'hystérésis du preset
épinglé, l'arrondi par incrément et le maximum effectif Phase0. Les comparaisons
par différence évitent les débordements des additions solde/seuil. Les règles
sont explicites ; ce maximum individuel n'est pas un plafond de validateurs.
Le candidat conserve les soldes réels, le registre et ses métadonnées ; une
annulation ou une configuration invalide ne modifie pas la source.

`effectiveBalanceQualification` : le scénario officiel complet compare 64
validateurs (8 changements), avec 8 cas locaux de seuils/cap/solde maximal,
66 annulations et 8 rejets. Quatre suites passent, dont les régressions du registre,
de FFG et des opérations de blocs. Les preuves incluent une altération de l'état
attendu rejetée malgré un hash de fixture recalculé.

Ce composant n'avance pas l'époque : l'orchestration des récompenses, des resets,
des historiques et de la rotation des attestations reste à intégrer, puis à
qualifier avec state_root, persistance et réseau. Aucun EIP-7251/Electra ni
consensus actif n'est déclaré complet.
Source épinglée : https://github.com/ethereum/consensus-specs/blob/fcdcf42024f33c498bff74936eabea2d96049fc2/specs/phase0/beacon-chain.md#effective-balances-updates

### Fin d'époque Phase0 — historiques et rotation

`consensus_epoch_maintenance` couvre cinq composants : reset périodique des votes
Eth1, remise à zéro de la prochaine case de sanctions, copie du mix RANDAO vers
la prochaine case, accumulation de `HistoricalBatch` SSZ (racines des blocs et
états), et rotation des attestations courantes vers les précédentes. Les entrées
sont des états authentifiés ; ces fonctions ne valident pas des votes réseau.
Les paramètres viennent du preset. Les vecteurs d'historiques utilisent sa taille
puissance de deux ; la limite de liste est vérifiée avant tout ajout.

`epochMaintenanceQualification` couvre les six scénarios officiels des cinq
composants, sept contrôles de frontières/retour circulaire, douze rejets et 128
annulations pendant la construction des historiques. Cinq suites passent avec
les régressions RANDAO, sanctions différées, FFG et soldes effectifs. Les cinq
altérations des états attendus (hashs recalculés) sont rejetées par les tests.

Les candidats conservent le slot et l'époque : l'orchestrateur doit appliquer
chaque étape une seule fois, après les calculs qui consomment les anciens votes,
sanctions et soldes effectifs, puis avancer le slot. Aucun marqueur de rejeu ni
commit durable n'est créé ici. Les récompenses, l'orchestration complète, le
hash de BeaconState et l'intégration au réseau restent ouverts. Le reset Eth1
ne remplace pas l'authentification des dépôts ou le traitement des votes Eth1.
Cette étape ne constitue pas EIP-6110, Capella/Electra, ni la finalité réseau.
Source épinglée : https://github.com/ethereum/consensus-specs/blob/fcdcf42024f33c498bff74936eabea2d96049fc2/specs/phase0/beacon-chain.md#epoch-processing

### Récompenses et pénalités Phase0

`prepare_phase0_rewards` reconstruit les comités de l'époque précédente, compte
une seule fois chaque participant non sanctionné pour source/cible/tête, et
choisit la première attestation au délai d'inclusion minimal pour rémunérer le
proposant. Le calcul utilise le solde actif de l'époque courante et la finalité
après FFG. Les pénalités d'inactivité suivent le seuil strict du preset. Les
produits/sommes intermédiaires et la racine carrée entière utilisent GMP ; les
montants finaux et additions de soldes sont contrôlés en uint64, les débits
saturent à zéro. Le candidat privé ne change aucun solde source en cas d'échec.

`rewardsQualification` : les 19 scénarios officiels concordent (1 216 entrées,
946 soldes changés, quatre scénarios de fuite d'inactivité, deux au genesis).
Dix-neuf contrôles de duplication, 275 annulations, onze entrées/configurations
rejetées, un débordement tardif atomique et 64 débits saturés sont vérifiés.
Six suites de régression passent ; une altération de solde attendu est rejetée
malgré le recalcul de son hash de fixture.

Les attestations de l'état doivent avoir été authentifiées à l'inclusion ; ce
calcul n'est pas une API de réception de votes réseau. Il ne modifie ni époque,
ni poids effectif, ni registre d'activation. L'orchestration doit l'appeler une
seule fois, après FFG et avant les mises à jour du registre et des poids. La
transition complète, son state_root, sa persistance et l'intégration réseau
restent ouvertes. Ces règles Phase0 ne sont pas les récompenses Altair/Electra,
ni les propositions EIP-8068/8363.
Source épinglée : https://github.com/ethereum/consensus-specs/blob/fcdcf42024f33c498bff74936eabea2d96049fc2/specs/phase0/beacon-chain.md#rewards-and-penalties-1

### Composition ordonnée de l'époque Phase0

`prepare_phase0_epoch` compose FFG, récompenses, registre, sanctions corrélées,
reset Eth1, soldes effectifs, resets sanctions/RANDAO, historiques et rotation
des attestations. Les paramètres communs sont contrôlés entre composants. Un
slot de fin d'époque et un état authentifié après `process_slot` sont requis.
Le candidat privé conserve slot/époque et renseigne `last_processed_epoch` : le
retraitement est rejeté. Ce marqueur doit être persisté avec l'état complet ;
son absence est réservée à l'import d'un état déjà authentifié.

`orderedEpochQualification` : six **projections** des scénarios officiels de slots
comparent registre, RANDAO, checkpoints, attestations et votes Eth1 (384 entrées,
cinq transitions d'époque). Les entrées `slots.yaml` sont conservées et hachées.
Les racines mises en cache à chaque slot ne sont pas calculées dans ces projections.
Le septième scénario `historical_accumulator` est différé, pas réussi. Aucune
racine de l'état attendu n'est utilisée comme entrée du calcul.

Les contrôles locaux vérifient cinq rejets de retraitement, l'ordre sanction /
poids effectif / reset, une composition FFG-récompenses-rotation avec votes,
594 annulations et treize rejets, dont un dépassement de capacité tardif de
l'historique sans mutation partielle. Sept suites passent. Un solde attendu
altéré est rejeté même avec un hash recalculé ; le vérificateur refuse de masquer
le scénario différé.

L'ordre complet des composants est implémenté, mais **la transition de slots et
la racine BeaconState ne sont pas qualifiées**. Restent `process_slot`, le hash
SSZ complet, la reprise durable du marqueur, les séquences signées multi-époques
et le raccordement au consensus réseau. Les anciennes limites des composants
isolés restent dans leurs campagnes historiques ; aucune activation moderne ou
qualification de production n'est déduite de cette composition.
Source : https://github.com/ethereum/consensus-specs/blob/fcdcf42024f33c498bff74936eabea2d96049fc2/specs/phase0/beacon-chain.md#epoch-processing

### Racine SSZ du BeaconState Phase0

`phase0_beacon_state_root` calcule `hash_tree_root` sur les 21 champs de l'état
SSZ complet : parties fixes, offsets/listes, validateurs et soldes, historiques,
RANDAO, sanctions, attestations en attente et checkpoints. Les limites du preset
font partie du calcul ; les branches nulles sont complétées sans allouer la
capacité maximale des listes (notamment le registre à 2^40). Les encodages non
canoniques des offsets, booléens et bitlists sont rejetés.

`beaconStateRootQualification` : les 123 vecteurs officiels du preset minimal
sont conformes, dans les sept groupes `ssz_lengthy`, `ssz_max`, `ssz_nil`,
`ssz_one`, `ssz_random`, `ssz_random_chaos`, `ssz_zero` (1 194 781 octets décodés).
Vingt et une mutations vérifient la contribution des champs ; 33 encodages ou
configurations invalides et 585 annulations sont vérifiés. Trois suites passent,
avec les régressions d'époque/historiques. Une racine attendue altérée est rejetée
même avec son hash de fichier recalculé.

La validité SSZ est distincte de la validité consensus : les vecteurs aléatoires
peuvent avoir des dates incohérentes ou des listes validateurs/soldes de tailles
différentes. Cette fonction valide leur schéma et leur racine ; l'admission de
l'état doit vérifier leurs invariants, clés et origine. Aucun état aléatoire
n'est admis par cette qualification dans le registre ou le réseau.

Le hash complet est désormais disponible mais reste à relier à `process_slot`,
au codec d'état modifiable et à la vérification des blocs/époques. La séquence
`historical_accumulator` précédemment différée n'est pas encore qualifiée.
Le preset mainnet, les états post-Phase0, la persistance et le réacteur réseau
ne sont pas couverts par cette campagne.
Source : https://github.com/ethereum/consensus-specs/blob/fcdcf42024f33c498bff74936eabea2d96049fc2/specs/phase0/beacon-chain.md#beacon-state

### Transitions de slots avec état SSZ complet

`prepare_phase0_slots` relie désormais le hash BeaconState au cache des racines
par slot, au remplissage de la racine de l'en-tête précédent et à la composition
des époques. Le codec natif reconstruit les champs consommés par l'époque et
réencode ses listes/offsets, en conservant les autres champs. Il contrôle la
cohérence des longueurs validateurs/soldes. L'entrée reste un état authentifié,
pas un mécanisme d'admission d'un genesis ou snapshot arbitraire.

`fullSlotsQualification` : les sept scénarios officiels du preset minimal,
100 slots, concordent **octet par octet sur les états complets** (107 223 octets
comparés), racines comprises. `historical_accumulator`, précédemment différé,
est maintenant qualifié dans cette nouvelle campagne. Les anciennes preuves de
projection restent inchangées. Sept contrôles comparent exécution groupée et
slot par slot avec le même marqueur ; sept rejets de retraitement de cible,
sept points d'annulation, dix entrées invalides et une rotation de records
non vides sont vérifiés. Six suites passent. Une racine historique attendue
altérée est détectée malgré un hash de fixture recalculé.

La limite de travail `maximum_slots` est un budget explicite du demandeur, pas
une limite du protocole : le rattrapage peut se faire par lots, avec les mêmes
états finaux. Les octets et `last_processed_epoch` restent des candidats privés
à persister atomiquement. Le marqueur n'appartient pas au schéma SSZ Ethereum.

Cette qualification ne couvre pas encore les blocs signés complets, les votes
Eth1/dépôts composés avec eux, l'authentification des snapshots, leur commit et
reprise durable ou le réseau actif. Les historiques sont recalculés depuis les
états complets ; aucune performance de nœud à un million de validateurs n'est
déduite de ces sept scénarios. Les profils post-Phase0 restent distincts.
Source : https://github.com/ethereum/consensus-specs/blob/fcdcf42024f33c498bff74936eabea2d96049fc2/specs/phase0/beacon-chain.md#beacon-chain-state-transition-function

### Transition complète des blocs signés Phase0

`prepare_phase0_state_transition` relie slots/époques, parent dérivé de l'état,
proposant/signature, RANDAO, votes Eth1, les cinq types d'opérations et la
vérification finale du `state_root` annoncé. Il n'existe pas d'option pour sauter
la vérification de racine. Les votes Eth1 comparent les trois champs et exigent
une majorité stricte sur la période entière ; les opérations consomment ensuite
le nombre de dépôts déterminé par cette décision. Les données de parent et
registre ne viennent pas du bloc à valider. L'authentification est actuellement
répétée par le composant d'opérations ; aucune mesure de débit réseau n'est
inférée de cette implémentation.

`fullBlocksQualification` : tous les 45 scénarios officiels `sanity/blocks`
Phase0 minimal sont importés, avec leurs blocs et métadonnées. Les 29 séquences
valides concordent octet par octet sur les états complets (530 412 octets) ; les
16 invalides sont rejetées. Les séquences exécutent 97 transitions acceptées,
avec attestations, dépôts, sorties, sanctions, votes Eth1, sauts d'époques et
historiques. Les séquences multi-époques auparavant différées sont couvertes dans
cette nouvelle campagne ; leurs anciennes preuves restent inchangées.

Le bloc officiel à racine erronée est rejeté malgré une authentification de
signature/RANDAO réussie. Sept points d'annulation, huit mutations/troncatures,
quatre erreurs de configuration/budget et une altération d'état attendu avec
hash recalculé complètent les contrôles. Cinq suites de régression passent.

L'état de départ doit être authentifié. Le résultat demeure un candidat privé :
ni commit durable atomique octets/marqueur, ni récupération après panne, ni
fork choice ou finalité réseau ne sont fournis par cette API. Les 45 scénarios
ne prouvent pas à eux seuls toute la conformité Phase0, les profils modernes,
l'interopérabilité de réseau ou une capacité en charge. Cette étape ne bascule
pas le consensus actif et ne clôt aucun objectif de production.
Source : https://github.com/ethereum/consensus-specs/blob/fcdcf42024f33c498bff74936eabea2d96049fc2/specs/phase0/beacon-chain.md#beacon-chain-state-transition-function

### Enveloppe de persistance SSZ et reprise locale

`phase0_slot_snapshot` / `phase0_slot_from_snapshot` définissent l'enveloppe
canonique `nano-phase0-slot-v1`, contenant SSZ, racine, profil complet de règles
et marqueur d'époque. Après l'époque zéro, le marqueur doit être présent et
correspondre à l'époque précédente. Aucun historique n'est inventé à la lecture.
La restauration exige une racine attendue obtenue d'un checkpoint authentifié,
et une limite d'allocation SSZ ; l'appelant doit aussi borner le JSON avant de
le parser. Le profil inclut les limites opérationnelles : leur modification
nécessitera une migration explicite de l'enveloppe.

`slotSnapshotQualification` vérifie douze propriétés et rejette 25 enveloppes
altérées/non canoniques, dont profils divergents, marqueurs absents, état modifié,
racine recalculée par l'altération, dépassement de taille et mauvais checkpoint.
Un processus enfant enregistre le checkpoint et le premier bloc d'une séquence
Ethereum, puis appelle `_exit` après les commits synchrones RocksDB. Après
réouverture, SSZ et marqueur concordent ; le bloc suivant traverse une époque
et reproduit exactement l'état officiel. Le retraitement des blocs est rejeté.
Une seconde réouverture conserve cet état et l'absence d'un commit rejeté.
Quatre suites passent avec les régressions de slots/blocs et de sanctions.

**Portée exacte** : le test utilise l'emplacement consensus du lot atomique
existant `ChainStore`, associé à des blocs d'exécution synthétiques de support.
Il ne prouve pas encore la liaison sémantique entre état Beacon et état EVM,
ni un commit certifié du nouveau consensus. La racine de référence provient des
blocs officiels déjà authentifiés, pas de l'enveloppe stockée. Le checkpoint
initial du test est au slot 8 ; son marqueur est explicitement importé depuis
cette référence de confiance. L'enveloppe ne constitue pas un protocole
d'authentification de snapshots reçus du réseau.

L'arrêt testé survient **après** le commit, sans destructeur RocksDB. Il qualifie
cette reprise WAL locale, pas une coupure physique ni un arrêt au milieu de
l'écriture. Les fautes pendant le commit, le raccordement au fork choice et au
réacteur et les migrations de profils restent ouverts. Aucun état réel du VPS
ni fonds n'a été modifié.

### Erreur de synchronisation WAL et récupération des snapshots

`snapshotSyncQualification` utilise l'interposition macOS de test existante,
absente du binaire Nano, pour faire échouer `fsync` sur le WAL temporaire choisi.
L'erreur est observée après son append et avant le succès du commit ; les lectures
du magasin encore ouvert restent au commit précédent. Le processus sort sans
fermer RocksDB. Le parent teste trois dispositions du seul suffixe non synchronisé :
conservé, entièrement retiré, ou tronqué à sa moitié. Les tailles observées sont
consignées et vérifiées. Les octets plus anciens ne sont pas modifiés.

Dans le cas conservé, la réouverture retrouve le nouveau commit entier ; dans
les deux cas de perte, elle retrouve l'ancien entier. Bloc, état d'exécution de
support, SSZ et marqueur restent cohérents. La reprise effectue les transitions
manquantes une seule fois, reproduit l'état officiel et le conserve après une
seconde réouverture. Cinq suites passent, incluant les régressions existantes
Shanghai d'erreur de synchronisation et de nouvelle tentative.

Cette preuve couvre une erreur de synchronisation et trois dispositions de fin
de WAL sur ce Mac. Elle ne simule pas toutes les positions de déchirure, les
pertes du contrôleur/disque, ni une coupure physique. Le retrait/tronquage du
suffixe intervient uniquement après sortie du processus dans ses répertoires
de test. Comme dans `slotSnapshotQualification`, les blocs d'exécution restent
des supports synthétiques : la liaison sémantique EVM/Beacon et le commit
certifié du nouveau consensus réseau ne sont pas qualifiés. Le test est limité
à macOS et n'est pas une qualification Linux du VPS.

### Choix de tête Phase0 sur snapshot authentifié — 25 septembre 2026

`phase0_fork_choice_head` implémente le calcul LMD-GHOST sur une vue fournie
par l'appelant : poids des derniers votes du registre au checkpoint justifié,
exclusion des membres inactifs/slashés/équivoques, propagation aux ancêtres,
bonus temporaire du proposant, filtrage des feuilles par justification/finalité,
et départage lexicographique des racines. Les membres ajoutés après le checkpoint
ne contribuent pas. Le dénominateur du bonus conserve les membres actifs slashés
et le plancher d'un incrément. La racine nulle désactive le bonus.

Référence : [Phase0 fork-choice au commit fcdcf420](https://github.com/ethereum/consensus-specs/blob/fcdcf42024f33c498bff74936eabea2d96049fc2/specs/phase0/fork-choice.md).
Les sommes utilisent des entiers larges et le parcours est itératif. La vue doit
être authentifiée : cette API ne valide ni signatures ni transitions de blocs,
ne traite pas `on_attestation`, `on_block`, `on_tick` ou les preuves d'équivoque,
et ne persiste pas le Store. Les checkpoints réalisés/non réalisés, le registre
historique et l'éligibilité temporelle du bonus restent à fournir par ces handlers.

`consensus_phase0_fork_choice` couvre égalités, descendance, exclusions, registre
post-checkpoint, bonus et plancher, dépassement uint64, sources réalisées/non
réalisées, limite des deux epochs, parents invalides, annulation et une branche
locale de 20 000 blocs. Cent arbres générés déterministes comparent 10 000 poids
avec une définition directe par recherche d'ancêtres, puis la tête résultante.
Ce sont des tests locaux, pas des vecteurs officiels de fork choice ni un réseau
à 20 000 nœuds. Les suites de blocs officiels et de récupération de snapshots
passent aussi après recompilation. Journaux et empreintes locaux :
`build-cmake/qualification/fork-choice-snapshot-20260925/`.

Clôture encore requise : importer/rejouer les événements officiels, implémenter
les handlers et leur état temporel, authentifier les snapshots d'entrée, lier
Store, exécution et commit certifié, puis qualifier partitions/reprises sur le
réseau actif. Ce composant ne constitue pas une activation du nouveau consensus.

### Admission des votes de fork choice — 25 septembre 2026

`process_phase0_fork_vote` ajoute les vérifications `validate_on_attestation`,
l'authentification BLS et la mise à jour des derniers messages. Les comités et
clés sont dérivés du registre/RANDAO au checkpoint cible fourni par l'appelant.
La cible doit être un ancêtre connu à la frontière d'epoch du bloc voté ; le vote
ne peut agir qu'à partir du slot suivant. Le gossip est limité à l'epoch courante
ou précédente. L'exception pour les votes inclus dans un bloc nécessite un bloc
déjà authentifié par l'appelant. Un doublon ou conflit dans la même epoch conserve
le premier message ; seuls des votes d'epoch strictement supérieure le remplacent.
Les indices déjà marqués équivoques ne sont pas mis à jour. La signature est
vérifiée même si aucun message ne doit changer. L'admission ne marque pas à elle
seule une équivoque et n'exécute pas de sanction.

Référence identique au commit Phase0 indiqué ci-dessus, sections `on_attestation`
et `update_latest_messages`. Les règles d'inclusion d'une attestation dans un bloc
ne sont pas substituées aux règles de fork choice : notamment, cette admission
ne compare pas la source au checkpoint justifié courant du Store.

Le test local `consensus_phase0_fork_vote` utilise des clés BLS générées, vérifie
le changement de tête après admission, les répétitions/conflits, exclusions,
remplacement par epoch, 11 rejets et 70 points d'annulation sans mutation publiée.
Les suites de choix de tête et d'inclusion des attestations officielles passent
également. Journaux : `build-cmake/qualification/fork-vote-local-20260925/`.
Ce test n'est pas un replay des événements officiels de fork choice.

Il reste à dériver/authentifier le contexte cible depuis le SSZ stocké (avec les
slots sautés), traiter les preuves d'équivoque, les événements blocs et horloge,
les files de votes différés et le Store persistant, puis raccorder le réseau.
L'appelant doit garantir les preuves de possession des clés du registre, la liaison
entre contexte et racine cible, et l'accès exclusif à la vue. Aucun vote réseau
brut ne doit pouvoir fournir lui-même ce contexte ou le drapeau de bloc vérifié.

### Preuves d'équivoque dans le choix de branche — 25 septembre 2026

`process_phase0_fork_slashing` implémente la mutation de l'ensemble des
équivoques décrite par `on_attester_slashing` dans la référence Phase0 épinglée.
Les deux attestations indexées doivent être contradictoires (double vote ou
surround dans le sens prescrit), leurs indices non vides, triés, uniques et dans
la limite configurée. Les deux signatures agrégées sont vérifiées avec le domaine
de chaque epoch cible et les clés du **block state justifié**, fourni authentifié
par l'appelant. Ce contexte diffère du checkpoint state avancé utilisé pour les
votes. Seule l'intersection des signataires rejoint l'ensemble des équivoques.
Une preuve valide sans intersection ou déjà traitée est sans effet. Le registre,
les soldes et les derniers messages ne sont pas mutés : les pénalités financières
restent du ressort des opérations de bloc. L'ensemble doit être conservé au moins
depuis le dernier checkpoint finalisé, y compris pendant la synchronisation.

Les tests locaux vérifient deux signatures BLS, intersection partielle de groupes
agrégés, absence d'intersection, double vote/surround, ancien domaine de fork,
doublons, indices invalides, borne configurée et annulation à sept points. Une
preuve acceptée retire le poids du signataire et change la tête attendue. Deux
suites locales passent. Artefacts :
`build-cmake/qualification/fork-slashing-local-20260925/`.

Cette étape ne qualifie pas la réception P2P, le replay officiel d'événements,
la dérivation du block state justifié, ni la conservation durable de l'ensemble.
Elle ne déclenche aucune activation réseau du nouveau consensus.

### Contextes de fork choice dérivés du SSZ — 25 septembre 2026

`prepare_phase0_fork_state_contexts` relie le post-state immédiat d'un bloc
**déjà authentifié** à son en-tête : racine SSZ annoncée, slot, dernier en-tête
avec state_root nul, racine du bloc du checkpoint et marqueur d'epoch. Il extrait
le contexte du block state pour les preuves d'équivoque et avance les slots
sautés jusqu'au début de l'epoch cible pour le contexte des votes. Registre,
RANDAO, fork et racine de genesis proviennent du SSZ ; les deux registres ne sont
pas confondus. Un budget de slots limite le travail demandé par l'appelant.

`consensus_phase0_fork_state` rejoue les 29 séquences valides du corpus officiel
de blocs Phase0 et compare leurs états finaux aux fichiers officiels. Il dérive
ensuite les contextes, dont 24 nécessitent des slots sautés. Les projections sont
comparées au décodeur indépendant des tests ; l'avancement est comparé à
`prepare_phase0_slots`, déjà qualifié séparément par les vecteurs officiels.
Neuf rejets (racines, marqueur, en-tête, overflow, budget) et sept annulations
sont vérifiés. Les quatre suites blocs/slots officiels, votes et contextes passent.
Artefacts locaux : `build-cmake/qualification/fork-state-context-20260925/`.

Cette API ne peut pas transformer un en-tête arbitraire en ancre de confiance :
l'appelant doit avoir validé le bloc et son ascendance, ou disposer d'une ancre
initiale authentifiée. Les preuves de possession du registre initial restent
requises. La récupération du Store, l'horloge, l'admission des blocs dans le
choix de branche et son raccordement au consensus réseau restent ouverts.

### Horloge du choix de branche — 25 septembre 2026

`prepare_phase0_fork_tick` fournit une transition privée horloge/vue : expiration
du bonus au premier nouveau slot, mise à jour de l'epoch courante et promotion
des checkpoints non réalisés à la traversée d'une frontière d'epoch. La promotion
de justification nécessite le contexte authentifié du checkpoint et remplace
également le registre utilisé pour les poids. Les checkpoints de même epoch ne
sont pas remplacés. L'ensemble des équivoques et les derniers votes sont conservés.

Référence : `on_tick`, `on_tick_per_slot` et `update_checkpoints` dans la
spécification Phase0 épinglée ci-dessus. Comme aucun événement ne s'intercale
pendant l'appel, les checkpoints non réalisés sont constants : les frontières
suivantes n'ont aucun effet supplémentaire. Le rattrapage utilise donc un calcul
constant en nombre de slots manqués (la copie de la vue reste proportionnelle à
sa taille). Un temps local antérieur à celui du Store est rejeté par l'API.

`consensus_phase0_fork_clock` compare mille rattrapages déterministes à une
référence locale slot par slot, vérifie le seuil exact, le cas sans changement de
slot, le rejeu temporel, le remplacement du registre, huit rejets, deux points
d'annulation et un rattrapage jusqu'à UINT64_MAX. Les quatre suites du fork choice
passent après recompilation. Artefacts locaux :
`build-cmake/qualification/fork-clock-local-20260925/`.

Restent à fournir la source d'horloge opérationnelle, le calcul/authentification
des checkpoints non réalisés lors des blocs, la publication durable conjointe de
l'horloge et de la vue, puis le replay officiel et l'intégration réseau. Cette
transition ne fait pas tourner un nœud et ne qualifie pas sa synchronisation.

### Admission des blocs dans le Store en mémoire — 25 septembre 2026

`Phase0ForkStore` rassemble la vue/horloge, les blocs et leurs états SSZ,
les indicateurs de ponctualité et les justifications non réalisées.
`prepare_phase0_fork_block` relie le traitement `on_block` Phase0 : parent connu,
slot non futur, descendance du checkpoint finalisé, transition complète et
state_root, tête antérieure, insertion privée, bonus du premier bloc ponctuel
avec même racine dépendante du mélange, puis checkpoints réalisés/non réalisés.
Le calcul FFG anticipé est dérivé du SSZ sans mutation du post-state. Un bloc
reçu dans une epoch ultérieure applique également ses checkpoints anticipés.
Une promotion de justification dérive son registre depuis le bloc du checkpoint.
Le candidat entier doit être publié/commité ensemble par l'appelant.

La qualification locale utilise les 27 fixtures officielles de blocs qui démarrent
au slot zéro : 15 séquences valides, 12 invalides, 16 blocs acceptés. L'horloge est
avancée avant chaque admission ; les états finaux valides égalent les post-states
officiels. Les 18 fixtures démarrant sur un état intermédiaire sont exclues de ce
test de Store, sans être retirées de la qualification complète de transition de
blocs. Sont aussi testés les doublons, le bloc futur, le parent inconnu, la
signature corrompue, le seuil exact de ponctualité et sept points d'annulation.
Les six suites de régression passent. Artefacts :
`build-cmake/qualification/fork-store-blocks-20260925/`.

Il ne s'agit pas encore d'un replay des fixtures officielles **fork_choice** :
les arbres concurrents, changements de justification/finalité au cours d'un tel
replay et les arrivées adverses restent à qualifier. Le Store est copié en mémoire
et ne démontre ni une consommation mémoire bornée en fonctionnement prolongé,
ni la durabilité. Restent l'import d'ancre opérationnel, les adaptateurs votes/
équivoques/horloge sur ce Store, son format durable et sa reprise, puis le
raccordement EVM/P2P. Aucune activation réseau ou qualification production.

### Événements liés au Store et périmètre officiel — 25 septembre 2026

Les adaptateurs `prepare_phase0_store_vote`, `prepare_phase0_store_slashing` et
`prepare_phase0_store_tick` dérivent leurs contextes depuis les blocs SSZ du
Store. L'API de vote gossip n'expose ni contexte fourni par l'émetteur ni drapeau
permettant de contourner la fenêtre temporelle. Les preuves utilisent le contexte
du block state justifié sans avancer ses slots. L'horloge récupère le checkpoint
nécessaire à la promotion et avance son état privé si des slots ont été sautés.
Les opérations retournent des candidats complets ; leur commit reste à raccorder.

Cinq suites passent. Les séquences genesis passent désormais par l'adaptateur
horloge. Un genesis synthétique avec clés BLS générées vérifie vote accepté,
signature invalide, vote prématuré, preuve contradictoire, conservation des
équivoques après vote répété/changement de slot. Une promotion sur métadonnées de
checkpoint **synthétiques** vérifie l'extraction automatique du registre ; elle
ne prétend pas que des slots vides produisent une justification.

La consultation du dépôt épinglé `consensus-spec-tests` a confirmé l'absence du
répertoire `fork_choice` sous [minimal/phase0](https://github.com/ethereum/consensus-spec-tests/tree/bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908/tests/minimal/phase0)
et mainnet/phase0. Le corpus existe sous
[minimal/altair/fork_choice](https://github.com/ethereum/consensus-spec-tests/tree/bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908/tests/minimal/altair/fork_choice).
Il ne faut pas décoder ces états comme du Phase0 ou retirer leurs champs pour
annoncer une conformité. Le replay fidèle de ce corpus ajoute un prérequis ouvert :
SSZ et transitions Altair (participation, inactivity scores et sync committees),
ou des vecteurs Phase0 générés avec une provenance et un périmètre explicitement
distincts. Aucun de ces replays n'est actuellement qualifié.

Artefacts locaux, listings consultés et journaux :
`build-cmake/qualification/fork-store-adapters-20260925/`.
Durabilité, admission des ancres, mémoire bornée, ordonnancement des votes différés
et raccordement EVM/P2P restent ouverts.

### État SSZ Altair — 25 septembre 2026

`altair_beacon_state_root` implémente le schéma complet à 24 champs : les deux
listes de participation remplacent les attestations pendantes Phase0 ; scores
d'inactivité, comité de synchronisation courant et suivant sont ajoutés. Chaque
clé de 48 octets est hachée comme un vecteur SSZ, puis les clés et la clé agrégée
forment le conteneur SyncCommittee. Le format Altair est sélectionné explicitement.
Le code partage la mécanique Merkle avec Phase0 sans réinterpréter ses états.

Référence : [Altair beacon-chain au commit épinglé](https://github.com/ethereum/consensus-specs/blob/fcdcf42024f33c498bff74936eabea2d96049fc2/specs/altair/beacon-chain.md).
Tous les 123 vecteurs BeaconState du preset minimal épinglé correspondent aux
racines officielles (sept groupes, 1 401 532 octets SSZ décodés). Sept mutations
des nouveaux champs affectent la racine, 25 encodages/configurations invalides
sont rejetés, sept points d'annulation sont vérifiés. Les 123 racines Phase0 et
les transitions complètes de blocs Phase0 restent conformes à leurs tests.

Corpus : `tests/fixtures/consensus-altair-state-roots-official/`, manifeste,
Snappy originaux, SSZ décodés, racines et preset avec empreintes conservés.
Journaux : `build-cmake/qualification/altair-state-root-20260925/`.
Cette qualification est **SSZ seulement** : les octets de participation et les
clés sont des valeurs de schéma, pas une preuve de validité de consensus/BLS.
Les transitions Altair, le calcul des récompenses/participation/inactivité,
les sync committees et le replay officiel fork_choice restent ouverts.

### Scores d'inactivité Altair — 25 septembre 2026

`prepare_altair_inactivity_updates` implémente l'opération isolée
`process_inactivity_updates` sur les octets SSZ : epoch genesis inchangée,
éligibilité de l'epoch précédente, participation cible à temps des membres
non slashés, incrément du biais en cas d'absence, décrément saturé et récupération
hors fuite d'inactivité. L'addition uint64 est contrôlée avant récupération.
Les paramètres proviennent du preset/configuration, sans valeur réseau par défaut.
Seuls les scores changent dans le candidat ; aucune epoch/slot n'est avancée.

Les 21 états post officiels de l'opération correspondent intégralement, y compris
les octets hors scores. Trois suites passent (inactivité, SSZ Altair, récompenses
Phase0). Six entrées/configurations invalides, dont overflow de score et compte
incohérent, et sept points d'annulation sont vérifiés. Corpus et empreintes :
`tests/fixtures/consensus-altair-inactivity-official/`. Journaux :
`build-cmake/qualification/altair-inactivity-20260925/`.

Deux fixtures (`randomized_state`, `randomized_state_leaking`) contiennent cinq
validateurs dont l'activation est UINT64_MAX mais la sortie finie. Comme dans la
spécification de cette opération, le calcul applique les prédicats d'éligibilité
sans revalider tout le cycle de vie. Ces fixtures **ne prouvent pas l'admission**
de tels états. Les contrôles d'admission/authentification Altair restent à
implémenter avant tout raccordement réseau. La composition d'epoch, les autres
récompenses, les sync committees et le replay fork_choice demeurent ouverts.

### Récompenses et pénalités Altair — 25 septembre 2026

`prepare_altair_rewards` implémente `process_rewards_and_penalties` sur SSZ :
récompense de base par incrément, participation source/cible/tête pondérée,
absence de récompense de participation pendant une fuite d'inactivité et pénalité
d'inactivité basée sur le score. Le dénominateur utilise les soldes effectifs
actifs de l'epoch courante ; les participants éligibles sont ceux de l'epoch
précédente, hors slashés pour les crédits. Les calculs intermédiaires utilisent
des entiers larges, les additions de soldes sont contrôlées en uint64.

Les catégories sont appliquées dans l'ordre source, cible, tête, inactivité :
chaque débit est saturé à zéro **avant** les catégories suivantes. Ce n'est pas
une unique soustraction de la somme des deltas. Le test d'un solde nul qui reçoit
ensuite une récompense de tête vérifie ce point. L'appel doit suivre la mise à
jour d'inactivité dans une future composition d'epoch ; il ne la refait pas.

Les 15 post-states officiels complets correspondent. Six entrées/configurations
invalides (dont overflow de crédit), sept points d'annulation et l'ordre de
saturation sont vérifiés. Deux suites passent : récompenses et inactivité Altair.
Corpus : `tests/fixtures/consensus-altair-rewards-official/`.
Journaux : `build-cmake/qualification/altair-rewards-20260925/`.

La fonction reste une opération isolée, sans admission d'état, protection contre
le double traitement, commit ni réseau. Les pondérations 14/26/14 et le
dénominateur 64 sont les constantes du profil Altair, fournis explicitement dans
les règles. La composition d'epoch, les autres opérations Altair et le replay
fork_choice demeurent ouverts.

### Justification et finalisation Altair — 25 septembre 2026

`prepare_altair_justification_finalization` calcule les poids cibles à partir
des indicateurs de participation et du registre SSZ. Les participants doivent
être actifs dans l'epoch concernée et non slashés. Le dénominateur retient tous
les membres actifs de l'epoch courante. Les deux premières epochs restent
inchangées ; ensuite sont appliqués le décalage des bits, les seuils inclusifs
de deux tiers et les quatre règles de finalisation. Les racines sont extraites
de l'historique au slot du checkpoint, avec contrôle de la fenêtre disponible.
Les comparaisons de poids utilisent des entiers larges.

Les dix post-states officiels complets correspondent. Trois tests locaux
vérifient le seuil exact, un poids synthétique à un wei en dessous et l'exclusion
d'un participant slashé. Le cas à un wei est un test arithmétique synthétique,
pas une preuve d'admission d'un solde effectif hors incrément. Quatre rejets et
sept points d'annulation sont vérifiés. Trois suites passent : finalité,
récompenses et inactivité Altair.

Corpus : `tests/fixtures/consensus-altair-ffg-official/`.
Journaux : `build-cmake/qualification/altair-ffg-20260925/`.
La signature et l'authenticité des indicateurs doivent être établies en amont.
Cette opération ne qualifie pas la finalité réseau : composition d'epoch,
comités de synchronisation, transitions de blocs et replay fork_choice Altair
restent à compléter avant le raccordement au réseau.

### Rotation des sync committees Altair — 25 septembre 2026

`prepare_altair_sync_updates` réalise la rotation à la frontière configurée :
l'ancien comité suivant devient courant, puis le nouveau suivant est sélectionné
par mélange RANDAO/domaine sync et échantillonnage pondéré par solde effectif.
Les indices répétés sont conservés. Les clés sélectionnées sont agrégées en G1
avec vérification d'encodage/sous-groupe ; cela ne remplace pas les preuves de
possession et l'admission du registre en amont. Le SSZ hors comités reste inchangé.

La taille du comité, la période, le lookahead et les tours de mélange viennent du
preset. Le nombre maximal d'essais est un budget local fourni par l'appelant :
son dépassement lève une erreur, sans repondération ni résultat partiel. Il n'est
pas un plafond de validateurs. Le parcours du registre reste proportionnel à son
nombre de membres ; aucun résultat de débit distribué n'est déduit de ce test.

Les cinq post-states officiels correspondent : quatre rotations et un cas hors
frontière. Un cas local avec un seul membre actif conserve ses 32 occurrences
pour le preset minimal. Sept rejets, sept points d'annulation, clés invalides et
agrégat vide sont vérifiés. Trois suites passent : rotation, BLS et FFG Altair.
Corpus : `tests/fixtures/consensus-altair-sync-official/`.
Journaux : `build-cmake/qualification/altair-sync-20260925/`.

Cette étape ne traite pas encore `SyncAggregate` dans les blocs. Rotation de
participation, composition d'epoch, transition Altair complète, replay officiel
fork_choice et réseau restent à compléter.

### Rotation de participation Altair — 25 septembre 2026

`prepare_altair_participation_updates` copie les indicateurs courants vers la
participation précédente puis remet les indicateurs courants à zéro, après
validation SSZ et cohérence des tailles avec le registre. L'état source reste
inchangé ; seul le candidat contient la rotation.

Les douze post-states officiels correspondent intégralement, couvrant listes
nulles, remplies, aléatoires et registres plus grands. Trois rejets et sept points
d'annulation sont vérifiés ; les suites de récompenses et finalité passent aussi.
Un test confirme explicitement que la rotation répétée n'est pas idempotente :
le second appel effacerait la participation précédente. La composition d'epoch
et son marqueur devront garantir l'application unique avant raccordement réseau.

Corpus : `tests/fixtures/consensus-altair-participation-official/`.
Journaux : `build-cmake/qualification/altair-participation-20260925/`.
Cette opération ne ferme ni l'assemblage de l'epoch ni les contrôles d'admission,
la transition de bloc Altair, le replay fork_choice ou la durabilité.

### Époque et slots Altair composés — 25 septembre 2026

`prepare_altair_epoch` applique les douze opérations dans l'ordre prescrit :
FFG, inactivité, récompenses, registre, pénalités corrélées, Eth1, soldes effectifs,
slashings, RANDAO, historique, participation et sync committees. Les opérations
communes réutilisent les composants existants avec les paramètres Altair, dont
le multiplicateur de slashing. La reconstruction SSZ conserve les listes et les
offsets canoniques après remise à zéro des votes Eth1 ou extension de l'historique.
Le slot reste inchangé et le candidat porte le marqueur de l'epoch traitée.

`prepare_altair_slots` calcule les racines de l'état/en-tête pour chaque slot,
met à jour l'historique, appelle l'epoch à la frontière, puis incrémente le slot.
L'appel direct de l'epoch exige que les racines du slot aient déjà été mises en
cache, comme dans la spécification. L'état initial reste soumis à admission
externe ; octets et marqueur doivent être publiés et persistés ensemble.

Les sept séquences officielles Altair de slots correspondent intégralement,
soit 100 slots. Les sept comparaisons entre traitement groupé et slot par slot
reproduisent aussi les marqueurs. Sont vérifiés : rejeu d'epoch, mauvaise epoch,
appel hors frontière, règles incohérentes, budget de slots et sept annulations.
Un épuisement du budget de sélection au dernier composant sync ne publie aucun
changement ; la nouvelle tentative réussit. Sept suites de régression passent.

Corpus : `tests/fixtures/consensus-altair-slots-official/`.
Journaux : `build-cmake/qualification/altair-full-slots-20260925/`.
Cette qualification couvre les séquences minimal fournies, pas l'ensemble des
combinaisons adverses d'epoch ou un réseau distribué. Le traitement des blocs
signés Altair (notamment SyncAggregate), le replay officiel fork_choice et la
persistance/reprise du nouveau Store restent ouverts. Aucune activation réseau.

### SyncAggregate Altair — 25 septembre 2026

`prepare_altair_sync_aggregate` vérifie le bitvector et la signature BLS sur la
racine du bloc au slot précédent, avec le domaine sync committee et la version
de fork correspondant à ce slot. Le cas sans participant exige la signature
infinie canonique. Les clés répétées gardent leur multiplicité. Le proposant est
dérivé du registre et du RANDAO ; récompenses et débits saturés sont appliqués
dans l'ordre des positions, y compris lorsque le proposant est aussi membre.
Les additions vérifient le débordement et seul un candidat SSZ privé est retourné.

Les 24 cas officiels minimal du commit épinglé passent : 16 post-states identiques
octet pour octet et huit signatures invalides rejetées. Ils couvrent notamment
la participation vide/partielle/complète, les membres sortis ou retirables, le
proposant dans le comité et les signatures de mauvais domaine/bloc/comité.
Quatre mutations locales vérifient longueur, corruption et autre genesis ; cinq
contextes invalides ou débordements et sept annulations sont aussi rejetés.

L'opération suppose un état admis : clés avec preuve de possession, comité
et sa clé agrégée cohérents et authentifiés. Le calcul direct sur les participants
est équivalent au raccourci par clé agrégée de la spécification sous cette
précondition. Cette opération isolée n'authentifie pas le bloc enveloppant et
n'empêche pas sa réapplication : le pipeline complet doit assurer l'unicité.
Transition complète de bloc Altair, replay fork_choice, persistance et réseau
restent à terminer. Aucun changement du consensus actif.

Corpus : `tests/fixtures/consensus-altair-sync-aggregate-official/`.
Preuves : `build-cmake/qualification/altair-sync-aggregate-20260925/`.

### Attestations Altair — 25 septembre 2026

`prepare_altair_attestation` applique les indicateurs de participation et les
récompenses de proposant Altair. Le comité, le proposant et le domaine de
signature sont dérivés du registre, du RANDAO et du fork de l'état. Les contrôles
communs Phase0 (source, fenêtre d'inclusion, comité et signature BLS) sont
réutilisés sur un contexte privé ; aucune liste d'attestations en attente ni
limite Phase0 n'est ajoutée à l'état Altair.

Les indicateurs source/target/head dépendent des racines historiques et du délai
d'inclusion. Seuls les indicateurs nouvellement acquis contribuent à la récompense
avec les poids Altair 14/26/14, 8/64. Une inclusion répétée reste valide sans
paiement supplémentaire. Les états candidats restent privés jusqu'au succès,
avec vérification des cardinalités et du débordement de solde.

Les 41 opérations officielles minimal passent : 17 post-states identiques en
SSZ et 24 attestations invalides rejetées. Les 17 répétitions sont sans effet ;
85 mutations supplémentaires (signature, source, longueur de bits, participation
vide, autre genesis), cinq contextes invalides/débordements et sept annulations
sont vérifiés. Les clés doivent avoir établi leurs PoPs lors de l'admission de
l'état ; cette opération ne qualifie pas la transition complète du bloc.

Corpus : `tests/fixtures/consensus-altair-attestation-official/`.
Preuves : `build-cmake/qualification/altair-attestation-20260925/`.
Restent notamment les différences de dépôts/slashings et leur composition dans
le bloc Altair, puis le replay fork_choice et l'intégration réseau persistante.

### Dépôts Altair — 25 septembre 2026

`prepare_altair_deposit` réutilise les preuves Merkle, l'ordre des dépôts et les
signatures de possession du composant existant, puis reconstruit l'état SSZ Altair.
Les nouveaux validateurs reçoivent leurs deux indicateurs de participation et
leur score d'inactivité à zéro. Un complément de dépôt conserve ces champs et
les credentials existants. Une signature invalide d'une nouvelle clé consomme
l'index prouvé sans ajouter de validateur, conformément à la spécification.
Les sept listes variables sont réencodées avec leurs offsets canoniques ; les
cardinalités et la limite de représentation SSZ sont contrôlées.

Les 19 cas officiels minimal passent : 17 états complets identiques octet pour
octet (sept admissions, cinq compléments, cinq signatures ignorées) et deux
opérations invalides rejetées. Les 17 positions consommées ne sont pas rejouables ;
51 mutations de preuves/montants, cinq débordements et sept annulations sont
rejetés. Les racines SSZ des résultats restent calculables avec le schéma Altair.

Corpus : `tests/fixtures/consensus-altair-deposit-official/`.
Preuves : `build-cmake/qualification/altair-deposit-20260925/`.
L'Eth1 deposit root/count doit être authentifié par le traitement du bloc. Cette
opération privée ne prouve ni ce lien à la couche d'exécution, ni la persistance,
ni la finalité réseau. Slashings Altair et composition complète restent ouverts.

### Slashings Altair — 25 septembre 2026

`prepare_altair_proposer_slashing` et `prepare_altair_attester_slashing` dérivent
le proposant depuis l'état et vérifient les preuves avec les domaines de fork
correspondant à chaque signature. Le moteur commun de sorties et de sanctions
est réutilisé avec les paramètres Altair : pénalité minimale du preset Altair,
récompense du proposant 8/64 (exactement division par huit), et multiplicateur
de corrélation Altair. Les sorties, soldes et accumulateurs de slashings sont
reportés dans un candidat SSZ privé ; les autres champs restent inchangés.

Les 45 cas officiels minimal sont couverts : trois états de proposer slashing
et huit états de attester slashing correspondent intégralement au post-state ;
33 opérations invalides sont rejetées. Le neuvième cas valide d'attester slashing,
`already_exited_long_ago`, utilise une chronologie de registre synthétique
incohérente : son post-state SSZ est reproduit par le composant isolé, mais le
chemin strict rejette cet état. Ce cas n'est pas compté comme transition stricte
réussie. Le cas invalide `invalid_proposer_is_withdrawn` est lui aussi rejeté
pour incohérence du registre. Aucun contrôle d'admission n'est affaibli.

Onze rejeux ne peuvent appliquer une seconde sanction ; 22 mutations de preuves
ou genesis, onze débordements et quatorze annulations sont vérifiés. Les clés du
registre restent soumises à établissement préalable des PoPs. Cette étape ne
constitue ni une transition complète de bloc ni une activation du consensus.

Corpus : `tests/fixtures/consensus-altair-{proposer,attester}-slashing-official/`.
Preuves : `build-cmake/qualification/altair-slashing-20260925/`.
La composition des blocs Altair et leur replay fork_choice restent ouverts.

### Transition complète de blocs Altair — 25 septembre 2026

`prepare_altair_state_transition` compose maintenant les slots/époques Altair,
l'authentification du bloc et de son proposant, le parent, le RANDAO, les votes
Eth1, les sanctions de proposants puis d'attestateurs, les attestations, les
dépôts, les sorties et le SyncAggregate. Le corps SSZ Altair inclut le neuvième
champ SyncAggregate dans son hash_tree_root ; offsets et tailles des opérations
sont validés avant décodage. La racine d'état finale annoncée est obligatoire.
Aucun mode de saut des signatures ou du contrôle de racine n'est exposé.

L'état et le marqueur d'époque restent privés jusqu'à la réussite complète.
Les votes Eth1 sont comptés contre une majorité stricte de la période entière,
et le nombre de dépôts doit être exactement celui exigé par le curseur et le
compte authentifié. Les paramètres partagés des époques, sanctions, sorties,
récompenses, registres et comités sont contrôlés pour éviter les profils mixtes.

Les 52 séquences officielles minimal passent : 37 états finaux reproduits
intégralement, 15 séquences invalides rejetées, 105 blocs intermédiaires acceptés
avec contrôle de racine, 772164 octets de post-states comparés. Sont inclus les
opérations mixtes aléatoires, chevauchements de sanctions, auto-sanction du
proposant, doublons, votes Eth1, transitions d'époque, inactivité, historique et
sync committees vides/partiels/complets. Onze mutations/troncatures, cinq erreurs
de règles/budget et sept annulations sont testées. Un bloc à racine finale
incorrecte mais à signature d'enveloppe valide est rejeté ; un rejeu au même
slot l'est également.

Corpus : `tests/fixtures/consensus-altair-full-blocks-official/`.
Preuves : `build-cmake/qualification/altair-full-blocks-20260925/`.
Ce jalon compose les blocs Altair sur un état initial admis, sans implémenter
l'admission de snapshots, la migration Phase0→Altair, le fork_choice Altair,
la persistance du nouveau Store ou son raccordement EVM/P2P. Aucun changement
du consensus actif ni qualification de production distribuée.

### Fork choice Altair — get_head qualifié, 25 septembre 2026

Adaptateurs Altair ajoutés pour Store en mémoire, ancre de confiance externe,
contextes de votes/sanctions, horloge et réception de blocs complets. La réception
utilise la transition Altair et dérive les contextes depuis les états liés aux
blocs. Le proposant, les signataires et leurs poids ne viennent pas de messages
non authentifiés. L'ancre doit être approuvée extérieurement ; le constructeur
contrôle son lien bloc/état mais n'établit ni sa confiance ni les PoPs du registre.

Deux politiques de proposer boost sont explicites : la release v1.6.0-beta.0
sans filtre de dépendance du shuffling, et la référence plus récente avec ce
filtre (politique par défaut). Les dix scénarios officiels get_head sont rejoués
avec la politique historique et sa configuration temporelle exacte. Il ne s'agit
pas d'un changement du consensus actif de Nano.

**Échec initial conservé et résolu.** À l'événement 135 de
`voting_source_within_two_epoch`, le replay sélectionnait le bloc 41 au lieu du
bloc 40. Il omettait les attestations embarquées, alors qu'un événement bloc du
format officiel implique aussi leur réception. Ce n'était pas une erreur dans
les attentes de la fixture. `prepare_altair_received_block` compose désormais
on_block puis les attestations et preuves de double vote incluses. L'exception
de fenêtre temporelle des votes n'est accessible qu'après validation du bloc,
jamais via un paramètre fourni par un émetteur gossip. Un vote embarqué dont le
contexte ou l'ascendance ne permet pas l'admission au fork choice n'invalide pas
le bloc valide ; aucune file de récupération/différé n'est encore fournie.

Les dix scénarios passent : 109 événements blocs, 104 ticks, 19 votes séparés,
une preuve de double vote séparée et 1081 champs vérifiés (tête, temps,
checkpoints, bonus). Le code Python officiel, compilé dans un environnement
externe temporaire depuis le commit de release avec BLS actif, rejoue lui aussi
ces dix scénarios sans aucun écart. Les attentes publiées ne sont pas modifiées.
Un vote ancien reçu seul est rejeté ; le même vote embarqué dans un bloc validé
reste pris en compte. Signature de bloc altérée et sept annulations ne publient
aucune modification du Store d'entrée.

Échec historique : `build-cmake/qualification/altair-fork-head-20260925/failing-LastTest.log`.
Preuves corrigées : `build-cmake/qualification/altair-fork-head-fixed-20260925/`.
Restent les autres groupes officiels (on_block, reorg, withholding, ex_ante,
get_proposer_head), l'admission de snapshots, la persistance/reprise, la gestion
des ressources et le raccordement réseau. Aucun statut production n'en découle.

### Fork choice Altair — on_block et reorg qualifiés, 25 septembre 2026

Le runner événementiel commun rejoue désormais les 23 scénarios officiels
minimal `on_block` et les huit scénarios `reorg`, au même commit de corpus
bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908 et avec le profil historique
v1.6.0-beta.0. Aucun changement des attentes ni du moteur n'a été nécessaire.
Les fichiers originaux YAML/Snappy, leurs décodages et leurs empreintes sont
conservés ; les références partagées du groupe get_head sont aussi vérifiées.

Résultats : on_block traite 629 événements blocs et 638 ticks, vérifie 6319
champs et rejette les quatre événements invalides attendus (parent incorrect,
bloc futur, bloc antérieur à la finalité, branche incompatible avec le checkpoint
finalisé malgré des slots sautés). Reorg traite 257 événements blocs, 271 ticks
et huit votes séparés, vérifie 2640 champs, sans rejet attendu. Ces cas couvrent
les justifications retardées, les votes FFG suffisants/insuffisants et les branches
concurrentes. Les dix scénarios get_head restent verts avec leurs sept contrôles
d'annulation et gardes de réception ; ces gardes locales ne sont pas recomptées
pour chacun des deux nouveaux groupes.

CTest : `ethereum_official_altair_fork_head`,
`ethereum_official_altair_fork_block`, `ethereum_official_altair_fork_reorg` :
3/3 suites, 41 scénarios, 10040 champs vérifiés au total.
Preuves : `build-cmake/qualification/altair-fork-block-reorg-20260925/`
(journal CTest, compilation, résultats structurés et empreintes des sources).
Cette campagne native n'inclut pas un nouveau rejeu Python indépendant des deux
nouveaux groupes ; l'oracle antérieur concernait uniquement get_head.

Restent withholding, ex_ante et get_proposer_head, puis admission de snapshots,
persistance/reprise atomique, bornage des ressources et raccordement EVM/P2P.
Les scénarios utilisent le preset minimal et un Store en mémoire ; ils ne sont
ni un réseau distribué, ni une activation de consensus, ni une preuve de production.

### Fork choice Altair — withholding et ex_ante minimal, 25 septembre 2026

Les deux scénarios minimal de rétention de blocs (`withholding_attack` et
`withholding_attack_unviable_honest_chain`) et les trois scénarios minimal
ex_ante (vanilla et sandwich avec/sans attestation honnête) passent dans le
runner événementiel commun, sans modification du moteur ni des attentes.
Même corpus épinglé bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908 et profil
historique v1.6.0-beta.0 ; YAML/Snappy originaux conservés et empreintes vérifiées.

Withholding : 66 événements blocs, 86 ticks et 760 champs vérifiés.
Ex_ante : 11 événements blocs, 11 ticks, deux votes séparés et 110 champs
vérifiés. Aucun rejet attendu. Le replay indépendant avec l'implémentation
Ethereum officielle de release et BLS actif confirme les cinq scénarios et
870 champs sans écart. Les deux scénarios ex_ante réservés au preset mainnet
(`attestations_is_greater_than_proposer_boost_with_boost` et
`sandwich_with_boost_not_sufficient`) ne sont pas inclus dans cette campagne.

CTest : `ethereum_official_altair_fork_withholding` et
`ethereum_official_altair_fork_ex_ante`, 2/2 suites.
Preuves : `build-cmake/qualification/altair-fork-withholding-ex-ante-20260925/`
(journal natif, rapports indépendants, empreintes et scripts oracle temporaires).
Ces replays ne sont pas des attaques distribuées ni des mesures de charge.
Get_proposer_head reste à implémenter/qualifier ; snapshots, persistance/reprise,
ressources et raccordement au réseau restent ouverts. Aucune activation réseau.

### Altair — choix de branche du proposant, 25 septembre 2026

`altair_proposer_head` implémente l'optimisation optionnelle get_proposer_head
sur le Store authentifié. Les seuils de poids, délai et distance à la finalité
sont fournis explicitement. Les poids sont dérivés du registre justifié et des
votes admis, avec exclusion des votes sanctionnés/équivoques par le moteur
commun. Le solde actif utilisé pour les seuils inclut les validateurs sanctionnés,
comme la référence. Les calculs de poids utilisent des entiers sans débordement.

Choisir le parent exige simultanément : tête tardive, slot hors frontière
d'époque, checkpoints FFG compétitifs, finalité assez récente, proposant à
l'heure, réorganisation d'un seul slot, poids de tête strictement inférieur au
seuil et poids du parent strictement supérieur au sien. Un boost encore actif
sur la tête est une erreur de contexte ; le délai limite est inclusif.
La fonction ne modifie pas le Store, ne sélectionne aucun validateur et ne signe
aucun bloc. Elle suppose un état authentifié, sans admission de snapshot réseau.

Les deux scénarios officiels minimal get_proposer_head passent (312 champs),
confirmés indépendamment par le code Ethereum de release avec BLS actif.
Dix gardes natives supplémentaires couvrent délai inclusif/dépassé, tête ponctuelle,
boost résiduel, seuil de tête nul, seuil de parent UINT64_MAX, checkpoints FFG
incompatibles, plusieurs slots, frontière d'époque et annulation.
Les six groupes fork_choice minimal passent : 6/6 suites, 48 scénarios,
11222 champs officiels. Cela ne couvre pas les deux scénarios ex_ante mainnet.
Preuves : `build-cmake/qualification/altair-fork-proposer-20260925/`.

La qualification des groupes minimal ne termine pas l'intégration : admission
d'ancre/snapshots, persistance et reprise atomique, bornage et élagage du Store,
liaison EVM/P2P et finalité distribuée restent ouverts. Aucune activation réseau
ni changement du statut production.

### Altair — sauvegarde locale et reprise de transition, 25 septembre 2026

`altair_slot_snapshot` / `altair_slot_from_snapshot` ajoutent une enveloppe
canonique versionnée pour l'état SSZ vérifié et son marqueur de dernière époque.
Le profil contient tous les paramètres de transition Altair (époques, récompenses,
comités, sanctions, limites SSZ/corps, dépôts, sorties, attestations et sync).
La lecture exige une racine fournie par un checkpoint authentifié extérieur à
l'enveloppe. Modifier l'état et son hash interne ne suffit pas à être accepté.
La taille SSZ est bornée avant décodage ; le lecteur JSON doit aussi borner son
entrée. Ce format local n'est pas une admission de snapshots fournis par des pairs.

Le test utilise deux blocs successifs de la séquence officielle Altair attestation.
Après le premier bloc, un processus enfant écrit dans ChainStore/RocksDB avec
le commit synchronisé existant puis appelle _exit sans destructeurs. Le parent
rouvre la base, récupère les mêmes octets et marqueur, rejette le rejeu du bloc,
puis applique le suivant : résultat identique à l'exécution sans interruption
et au post-state officiel. Une seconde réouverture conserve ce résultat. Un
commit d'exécution invalide n'enregistre ni bloc ni snapshot ; une liaison à une
autre chaîne est rejetée.

Douze contrôles de reprise et 105 rejets de snapshots (chaque paramètre numérique
du profil, format, champs, marqueur, corruption, racine étrangère, limites) passent.
Les tests Phase0 existants et les 52 séquences de blocs Altair restent verts :
3/3 suites. Preuves : `build-cmake/qualification/altair-snapshot-recovery-20260925/`.

Limites : les blocs d'exécution de ce test sont synthétiques et servent uniquement
au transport atomique déjà disponible dans ChainStore. La liaison sémantique
EVM/consensus n'est pas qualifiée. L'arrêt survient après le commit réussi,
pas pendant une coupure électrique. Le Store de choix de branche (votes,
équivocations, branches, horloge, timeliness et checkpoints non réalisés) n'est
pas encore sauvegardé/restauré par cette enveloppe. Sa reprise complète reste
le prochain chantier ; aucune activation réseau ni validation production.

### Altair — sauvegarde du Store et reprise événementielle, 25 septembre 2026

`altair_store_snapshot` / `altair_store_from_snapshot` encodent désormais toutes
les branches/états, votes les plus récents, équivocations, horloge, checkpoints
réalisés/non réalisés, timeliness et proposer boost. Le registre pondéré du
checkpoint justifié est reconstruit depuis les états SSZ authentifiés ; les poids
ne sont pas acceptés comme une seconde copie arbitraire. L'enveloppe canonique
lie les règles de transition et le profil de boost/délais. Les tailles JSON,
SSZ individuelle/cumulée et nombres d'entrées sont bornés par des budgets locaux
explicites, sans nouveau plafond de validateurs dans les règles du réseau.

La lecture exige un digest externe de confiance, vérifié avant parsing JSON.
Un digest fourni par le même pair que le snapshot ne constitue pas cette confiance.
L'appelant doit conserver atomiquement cet engagement, sa séquence et sa liaison
à la chaîne. L'API n'assure pas seule l'anti-rollback du disque ni l'admission de
snapshots réseau ; ses limites d'allocation ne remplacent pas l'élagage du Store.

Échec initial conservé : filtered_block_tree avançait l'époque du checkpoint
justifié tout en conservant la racine de l'ancre après des slots vides. Le premier
restaurateur reprenait à tort le registre de l'époque initiale. Il reconstruit
maintenant l'état au début de la nouvelle époque. Aucune attente officielle ni
règle de vote n'a été modifiée pour corriger cet écart.

Les dix scénarios get_head et huit reorg sauvegardent leur Store à mi-parcours
puis à la fin (un scénario très court ne permet que la fin) : 35 écritures synchronisées dans ChainStore/RocksDB, suivies de
_exit du processus écrivain, réouverture et restauration. Le digest attendu est
conservé séparément par le parent. Tête, poids et branches viables sont identiques,
et les événements suivants continuent à satisfaire les attentes officielles.
Douze rejets au total couvrent corruption, champ ajouté, limites JSON/blocs/SSZ
et profil changé ; l'annulation est vérifiée dans chaque groupe. Les six suites
fork_choice ordinaires restent vertes : 8/8 suites au total.
Preuves : `build-cmake/qualification/altair-store-snapshot-20260925/`, échec initial
inclus. Tests : `consensus_altair_store_snapshot_head` et
`consensus_altair_store_snapshot_reorg`.

Les blocs d'exécution du transport de test restent synthétiques. Ce résultat
qualifie le format et la reprise après commit réussi, pas la liaison sémantique
EVM/consensus, les interruptions pendant commit, la panne électrique, la politique
d'ancrage en production, l'élagage, ou le raccordement au réacteur/P2P actif.

### Commit séquencé du Store — concurrence et erreurs disque, 25 septembre 2026

ChainStore dispose d'un emplacement dédié `fork-checkpoint/v1`, indépendant des
blocs d'exécution. `commit_fork_checkpoint` remplace atomiquement contenu et token
(séquence uint64 + SHA-256) dans un WriteBatch synchronisé. Il exige l'identité
chainId/genesis déjà liée et le token exact de la version précédente ; deux
écrivains calculant depuis la même version ne peuvent pas tous deux réussir.
Seule la version courante est conservée, sans accumulation de snapshots complets.
Le transport reste opaque : l'appelant doit préparer/valider le candidat de
consensus avant de le soumettre. Ce commit n'est pas encore atomique avec l'EVM.

Une erreur d'écriture rend l'instance de checkpoint inutilisable : lectures et
nouveaux commits de checkpoint échouent jusqu'à fermeture et réouverture.
L'appelant doit alors inspecter le résultat récupéré avant toute signature ou
publication. Cette règle traite l'issue incertaine d'une synchronisation échouée.
La lecture accepte aussi un engagement exact conservé extérieurement : une copie
ancienne de la base est rejetée si cet engagement correspond à la version récente.
Sans cet engagement extérieur, la restauration de tout le disque reste indétectable.
Ce mécanisme ne remplace pas une politique d'ancrage et de protection des clés.

Qualification : un seul gagnant pour deux écrivains concurrents, rejets des
versions périmées/digests faux/identités étrangères/tailles interdites ; six SIGKILL
pendant une campagne bornée de remplacements de 4 Mio, avec récupération d'un
checkpoint complet et conservation des séquences acquittées. Cela ne prouve pas
que chaque signal tombe à l'intérieur de l'appel d'écriture. Une injection macOS
fsync/F_FULLFSYNC EIO cible en plus exactement le WAL du commit : suffixe conservé
puis suffixe non synchronisé retiré dans une base temporaire. Les deux résultats
récupèrent respectivement le nouveau ou l'ancien couple contenu/token, puis
permettent un nouveau commit après réouverture. Le handle incertain est bien bloqué.

Les 35 reprises des 18 scénarios Altair get_head/reorg utilisent cette API dédiée,
sans blocs d'exécution synthétiques. Huit suites ciblées couvrent aussi les reprises
d'état Phase0/Altair et les injections disque historiques Paris/Shanghai.
Campagne : `build-cmake/qualification/fork-checkpoint-commit-20260925/`.
Tests ajoutés : `consensus_fork_checkpoint_commit` et, sur macOS,
`consensus_fork_checkpoint_sync_failure`. Restent les interruptions/erreurs Linux,
la coupure électrique physique, l'intégration événementielle au réacteur,
la liaison atomique EVM, l'ancrage et l'élagage. Aucun profil réseau activé.

### Coordinateur événementiel Altair durable, 25 septembre 2026

`AltairDurableReactor` relie tick, réception de bloc (messages embarqués inclus),
vote gossip et preuve de slashing au checkpoint séquencé. Chaque événement est
préparé sur un candidat privé, sérialisé, puis écrit avec CAS synchronisé avant
publication d'un pointeur d'état immuable et de son token. La publication après
succès n'appelle aucun callback et utilise des déplacements vérifiés noexcept.
Les méthodes sont sérialisées ; les callbacks d'annulation ne doivent pas entrer
à nouveau dans le coordinateur. Un bloc/vote/preuve refusé ne crée pas de commit.

Un conflit de version ou une erreur de commit rend le coordinateur indisponible,
y compris pour lire l'état courant. Les pointeurs précédemment publiés restent
immuables et inchangés. Une annulation pendant préparation/sérialisation ne modifie
ni mémoire publiée ni disque et n'interdit pas une nouvelle tentative. Le démarrage
exige soit un Store initial déjà authentifié pour une base vierge, soit un token
exact conservé extérieurement pour restaurer le checkpoint. L'identité de chaîne
et le profil sont vérifiés par les composants de stockage/restauration.

Les 18 scénarios officiels get_head/reorg passent via le coordinateur : 787 commits
synchronisés, dont 18 initialisations ; 17 réouvertures à mi-parcours, un scénario
ne contenant pas assez d'événements pour cette coupure. Les 3721 champs officiels
restent conformes après reprise. Une suite ciblée vérifie le bloc invalide,
cinq positions d'annulation, la conservation des anciens pointeurs, le conflit
avec un écrivain concurrent et, sur macOS, fsync EIO avant publication. Après
résolution de l'issue disque et réouverture, le traitement reprend. Quatre suites
passent, dont la régression get_head sans mode durable.
Preuves : `build-cmake/qualification/altair-durable-reactor-20260925/`.

Ce coordinateur est une brique intégrée de consensus/stockage, pas encore un service
P2P ni le remplacement du BFT actif. Il écrit un snapshot complet par événement :
base de correction fonctionnelle, sans qualification de débit ou d'amplification
d'écriture. Les commits incrémentaux, élagage, transport/admission, lien atomique
avec l'EVM, politique d'ancrage/signature et qualification réseau restent ouverts.

### Entrée SSZ des votes et preuves — Altair, 25 septembre 2026

Les décodeurs `decode_consensus_attestation_ssz` et
`decode_consensus_attester_slashing_ssz` sont désormais dans le code de production.
Ils traitent le schéma d'attestation antérieur à Electra, avec limite de comité
fournie par le profil. Offsets canoniques, longueurs fixes/variables, terminaison
du bitlist et nombre d'indices/bits sont contrôlés avant allocation de listes.
L'allocation dépend du message réel, jamais de la taille maximale configurée.
Un message mal formé/hors limite retourne nullopt ; configuration nulle et
annulation restent des erreurs distinctes. Les indices dupliqués/non triés,
signatures et devoirs appartiennent à la validation sémantique ultérieure.

`AltairDurableReactor::vote_ssz` et `slashing_ssz` composent ce décodage avec
l'admission cryptographique et le commit durable. Aucun champ fourni par le pair
ne permet de contourner la fenêtre gossip ou de se déclarer inclus dans un bloc.
Les messages mal formés ne modifient pas le token durable. Les replays officiels
get_head/reorg utilisent maintenant ces entrées en mode durable ; leur décodeur
historique de support n'est plus la porte d'entrée des messages séparés.

Qualification : 41 encodages d'attestations et 30 encodages de preuves du corpus
Altair officiel sont décodés avec conservation de tous les champs. Cela inclut
les opérations sémantiquement invalides du corpus : validité SSZ ne signifie pas
validité de signature ou de consensus. 768 troncatures, offsets altérés,
terminaisons et dépassements sont rejetés ; erreurs de configuration et annulations
propagent correctement. Cinq suites passent, incluant les 18 scénarios durables,
787 commits, 17 reprises et les gardes de publication/erreur disque.
Campagne : `build-cmake/qualification/consensus-message-ssz-20260925/`.

Reste l'intégration du transport P2P : cadrage, décompression bornée, négociation
de profil, files/budgets d'admission, récupération des dépendances, service réseau
et qualification distribuée. Le codec n'implémente pas le nouveau schéma Electra
(EIP-7549) et ne constitue aucune activation de fork ni de réseau.

### Enveloppe d'admission Altair préparée, 25 septembre 2026

L'examen de `network/main.cpp` confirme que le daemon actuel échange avec le BFT
actif via RPC. Il n'a pas encore de service d'événements Altair. L'adaptateur
`network/altair_message.hpp` prépare cette frontière sans enregistrer de nouveau
handler dans le daemon ni mélanger les profils actifs.

L'enveloppe JSON/hex non compressée contient exactement chainId, genesisHash,
forkDigest, kind et ssz. C'est un format Nano, pas le framing gossipsub Ethereum.
Le profil local fournit les identités et tailles maximales par type. Identités
incompatibles, champs supplémentaires, clés dupliquées, types inconnus (dont tick),
hex non canonique et dépassements sont rejetés avant décodage binaire. La limite
de frame passe par JsonFrame avant parsing ; un lecteur de flux devra appliquer
la même limite dès la réception fragmentée, pas seulement sur le buffer complet.
Le digest de fork est un filtre de compatibilité, pas une preuve d'authenticité.

L'admission reçoit une référence au budget partagé du daemon et utilise ses
leases bulk jusqu'à la fin du handler. Un message invalide consomme aussi son
budget ; une autre instance d'adaptateur ne crée pas une nouvelle réserve.
L'identité de pair doit venir du stream libp2p authentifié, jamais du JSON.
Le handler est synchrone et doit terminer son commit avant de répondre ; les
erreurs locales de consensus/disque/annulation remontent sans être converties
en verdict d'invalidité du pair. L'adaptateur ne doit pas être ré-entrant.

Test local vers le coordinateur durable : deux blocs du scénario officiel
chain_no_attestations committés avant acceptation, tête conforme aux attentes,
14 messages rejetés, 24 refus par quota sur une rafale bornée de 32 tentatives,
isolation des parts de pairs, partage du budget entre instances et libération
de lease après exception. Le framing est vérifié avec fragments de 1, 7 octets
et frame entière. Trois suites passent (wire, codec SSZ, publication durable).
Preuves : `build-cmake/qualification/altair-wire-admission-20260925/`.

Ce test n'ouvre aucun réseau libp2p. Restent l'enregistrement/negociation du
protocole, le service RPC/runtime Altair explicite, le travail hors boucle I/O
avec files bornées, la propagation/récupération des dépendances et la qualification
multi-nœuds. Pas de Snappy ni de fallback vers un autre profil acceptés ici.
Aucune activation réseau ou conformité wire Ethereum n'est revendiquée.

### Adaptateur asynchrone Stream libp2p, 25 septembre 2026

`AltairStreamRequest` raccorde l'enveloppe d'admission à l'interface Stream de
cpp-libp2p. Le PeerId vient du stream, la lease du budget partagé est acquise
avant lecture, et la limite de frame est appliquée à chaque fragment avant ajout
au buffer. JsonRead expose une lecture brute bornée pour préserver les clés JSON
dupliquées jusqu'au validateur d'enveloppe ; les interfaces JSON existantes restent
disponibles. La poursuite des lectures passe par l'exécuteur I/O, sans récursion
synchrone sur les fragments. Les erreurs du callback applicatif ne sont plus
avalées comme des erreurs de framing.

Le dispatcher externe reçoit message, drapeau d'annulation et callback de fin.
Il doit fournir une vraie file bornée et retourner false/lever seulement si aucun
travail n'a démarré. Il signale accepted uniquement après validation et commit
durable. Les callbacks de fin peuvent venir d'un worker ; leur traitement est
reposté sur l'exécuteur I/O et reste unique. Expiration ou arrêt explicite ferme
le stream et signale l'annulation. Si le worker tourne encore, la lease reste
occupée jusqu'à sa fin effective : les délais ne permettent pas de multiplier
les travaux cachés. Une fin tardive ne répond pas au stream expiré. Une erreur
locale ne produit aucun acquittement d'acceptation.

Dix cas sur streams contrôlés implémentant l'interface libp2p couvrent fragments,
fin depuis un thread worker, double callback, expiration pendant travail, arrêt
explicite, pair sans identité, lecture bloquée, dépassement, JSON invalide/dupliqué,
file refusée et erreur locale. Les neuf suites du module réseau passent. Les deux
régressions network/bft_network avec vrais processus et connexions libp2p locaux
passent aussi : elles qualifient le lecteur partagé et le transport existant,
**pas un réseau Altair actif**. Le test local enveloppe/coordinateur reste vert.
Preuves : `build-cmake/qualification/altair-async-stream-20260925/`.

Restent l'enregistrement/négociation du protocole Altair dans le daemon, le
service runtime/RPC explicite, la file worker bornée réelle et son arrêt coordonné,
la récupération des dépendances et les essais distribués Altair. L'adaptateur
n'est pas encore enregistré dans `network/main.cpp`. Pas d'activation réseau.

### File worker Altair bornée, 25 septembre 2026

`network/altair_worker.hpp` fournit désormais une file réelle, à un thread,
compatible avec le dispatcher de `AltairStreamRequest`. L'ordre FIFO sérialise
les traitements de consensus ; le nombre de travaux et la capacité allouée des
payloads SSZ sont bornés en comptant les travaux en attente **et** celui en cours.
Ces paramètres sont une politique locale de ressources, pas une limite du
registre des validateurs. Les callbacks et autres métadonnées sont bornés en
nombre, pas mesurés dans le budget des payloads.

`stop()` refuse toute nouvelle admission et signale l'annulation au travail actif
et aux travaux en attente. Ces derniers sont terminés sans entrer dans le handler.
Le handler actif doit coopérer aux checkpoints d'annulation et finir tout commit
avant son retour ; une décision déjà commitée n'est pas réécrite en rejet à cause
d'une annulation tardive. Les exceptions du handler produisent une erreur locale.
La callback de fin est appelée une fois après le traitement ; elle ne doit pas
lever d'exception. `join()` appartient au thread propriétaire, hors boucle I/O,
après `stop()`. L'exécuteur I/O reste vivant jusqu'au drainage des fins postées.
Un handler bloqué qui ignore l'annulation peut encore retarder l'arrêt : pas de
prétention de préemption forcée ou de délai d'arrêt garanti.

Les dix suites du module réseau passent, dont `altair_worker` (budgets cumulés,
capacité mémoire excédentaire, FIFO, exception, annulation et refus après arrêt).
`altair_stream` passe maintenant douze cas : deux supplémentaires utilisent le
vrai worker et vérifient la rétention de l'admission jusqu'à sa fin, la réponse
normale et l'absence de réponse après annulation. Les streams restent contrôlés,
sans connexions entre pairs. Preuves :
`build-cmake/qualification/altair-bounded-worker-20260925/`.

La file est disponible mais **pas instanciée dans le daemon**. Restent son
raccordement au service de consensus durable, l'enregistrement/négociation du
protocole, l'arrêt coordonné du runtime, la liaison atomique EVM et les essais
Altair distribués. Aucune activation réseau ni qualification production.

### Service worker–consensus durable, 25 septembre 2026

`network/altair_consensus_service.hpp` raccorde la file séquentielle au
`AltairDurableReactor` : blocs, attestations et preuves de slashing SSZ passent
par les validateurs et checkpoints d'annulation du cœur, puis par son commit
synchrone avant la callback d'acceptation. Le contrat de dispatch partagé
`altair_work.hpp` rend cette composition indépendante de libp2p : le service
est destiné au processus cœur, le daemon réseau restant séparé. Une exception
locale devient `local_failure`, sans acquittement positif. Une annulation après
commit ne réécrit pas la décision publiée. L'horloge reste contrôlée localement ;
il n'existe aucun type de message distant `tick`.

Qualification locale : les 18 scénarios officiels minimal `get_head` et `reorg`
passent avec `NANO_TEST_DURABLE_WORKER=1` : 366 blocs, 27 attestations, une preuve
de slashing, 375 ticks locaux et 3 721 checks. Les 787 commits incluent les
18 initialisations ; 17 reprises arrêtent/joinent le service avant fermeture de
la base, rouvrent le checkpoint exact puis poursuivent les événements. Aucun
résultat attendu du corpus n'a été changé. Le test d'admission wire passe aussi
par ce service et vérifie qu'un coordinateur indisponible après conflit de
checkpoint ne renvoie pas d'acceptation, puis qu'une instance restaurée traite
à nouveau les messages. Les dix suites du module réseau passent toujours.
Preuves : `build-cmake/qualification/altair-durable-worker-20260925/`.

Cette qualification utilise un vrai thread et RocksDB, mais pas une liaison
entre processus ni un réseau distribué Altair. Restent le point d'entrée privé
côté cœur, le transfert depuis le daemon, la négociation du protocole, le pilotage
de l'horloge et l'arrêt du runtime complet, puis la liaison EVM et les essais
multi-nœuds. Aucun changement d'activation réseau ni de statut production.

### Entrée RPC privée Altair, 25 septembre 2026

`nano_altairReceive(token, encodedEnvelope)` est maintenant traité par `Rpc`
uniquement lorsqu'un `AltairRpcService` a été explicitement injecté à sa
construction. Sans service, la méthode renvoie `-32601`. Avec service, le jeton
réseau est obligatoire (`-32001` sinon), `HttpServer` refuse une adresse d'écoute
non loopback et les requêtes navigateur avec Origin sont refusées. Ce contrôle
ne remplace pas la sécurité du compte hôte ni une authentification de snapshot.

Le cœur valide à nouveau l'enveloppe brute (profil, champs, doublons et tailles)
avant la file de consensus. La méthode compte dans le budget partagé des tâches
RPC coûteuses, pour HTTP et WebSocket, y compris dans les batches/notifications.
Elle libère le verrou RPC avant d'attendre le worker. Le worker Altair garde son
propre budget jusqu'à la fin réelle du travail. La réponse `accepted` suit le
commit durable ; les erreurs locales sont distinctes des rejets de consensus.
Saturation/arrêt renvoient `-32005`, erreur locale `-32603`, enveloppe invalide
`-32602`. Une échéance dépassée renvoie `-32002` et signale l'annulation au worker.
Une échéance ou une déconnexion n'est **pas une preuve d'absence de commit** ;
la stratégie de récupération après perte d'acquittement reste à raccorder au
transport du daemon. Aucun tick ne peut être transmis par cette méthode.

Les suites `rpc`, `http`, `consensus_altair_wire_admission` et
`consensus_altair_private_http` passent. Le nouveau cas utilise de vraies
connexions HTTP loopback, un worker réel et RocksDB ; deux blocs officiels sont
persistés, et le choix de tête correspond au corpus. Il teste le mauvais jeton,
la désactivation par défaut, le bind public refusé, les enveloppes incohérentes
ou dupliquées, le coordinateur indisponible et le service arrêté ; les RPC
ordinaires restent disponibles. Client et serveur sont dans le même processus
pour cette qualification : ce n'est pas encore un test daemon/cœur séparés.
Preuves : `build-cmake/qualification/altair-private-http-20260925/`.

Restent la construction du service depuis une configuration/ancre authentifiée
au démarrage de Nano, son appel depuis `nano-p2p`, la négociation de profil et
la gestion des réponses perdues, puis le runtime multi-nœuds et la liaison EVM.
La configuration courante n'injecte aucun service : pas d'activation implicite.

### Chargement explicite du profil Altair, 25 septembre 2026

Le runtime ne pouvait pas encore reconstruire les règles complètes sans les
helpers de fixtures. Le cœur expose maintenant `altair_transition_profile` et
`altair_transition_from_profile` : les 91 paramètres numériques et la version
initiale de fork sont lus sans défaut implicite, depuis le même format déjà lié
aux snapshots. Les tableaux ont une longueur exacte, chaque quantité est une
chaîne hexadécimale canonique, et toute conversion vérifie la largeur réelle du
champ cible (notamment les compteurs de rounds). Champs inconnus/manquants et
versions de fork non canoniques sont refusés. Les contrôles existants de cohérence
entre règles de bloc et d'epoch sont réutilisés ; cela ne prétend pas certifier
toutes les combinaisons économiques ou tous les paramètres opérationnels.

`load_altair_transition_profile` borne les octets, compare leur SHA-256 à un
digest fourni indépendamment, puis parse le JSON en refusant les clés dupliquées.
Le digest attendu doit provenir d'une configuration de confiance : le calculer
à partir du fichier reçu ne l'authentifierait pas. La sélection d'un profil
réseau et l'authentification de l'ancre restent des responsabilités distinctes.

`consensus_altair_explicit_profile` passe avec 502 refus de configurations
malformées/incohérentes et deux blocs officiels qui reproduisent exactement
l'état final attendu avec les règles rechargées. Les octets des snapshots restent
identiques. `consensus_altair_snapshot_recovery` passe également (105 snapshots
rejetés, reprise après sortie abrupte et résultat officiel identique).
Preuves : `build-cmake/qualification/altair-explicit-profile-20260925/`.

Il reste à composer ce chargeur avec les fichiers d'ancre vérifiés et les limites
du Store dans le démarrage de Nano, à gérer la reprise, puis à raccorder le daemon.
Aucun preset minimal n'est installé comme profil de production ; aucun consensus
réseau ni paramètre de genesis existant n'est activé/modifié.

### Import local d'une ancre vérifiée, 25 septembre 2026

`load_altair_anchor` compose maintenant les règles explicites avec deux fichiers
locaux bornés : état SSZ et bloc Beacon non signé. Les engagements attendus
(racines du bloc et de l'état, genesis_validators_root et version du fork) sont
fournis séparément par l'appelant. Le chargeur vérifie le format SSZ, le domaine
de fork, la liaison bloc/état et ces racines avant de rendre un Store candidat.
Il reconstitue le marqueur d'epoch à partir du slot authentifié et conserve
l'horloge du slot d'ancrage. Il ne touche pas au stockage.

La lecture refuse les fichiers absents/non réguliers et les tailles excessives ;
la taille est aussi surveillée pendant la lecture. Les checkpoints d'annulation
couvrent lecture, validation et retour du candidat. Les contrôles de cohérence
existants des règles de bloc/epoch sont réutilisés, avec validation explicite des
paramètres de timing et du profil de proposer boost.

`consensus_altair_trusted_anchor` passe : ancre de départ et ancre officielle au
slot 17 donnent exactement les Stores attendus ; onze altérations de racine,
domaine, données, taille, chemin ou règles sont refusées. Quatre annulations à
différentes étapes ne créent aucun checkpoint. Le candidat accepté peut ensuite
être publié par le coordinateur durable au numéro de séquence 1.
Preuves : `build-cmake/qualification/altair-trusted-anchor-20260925/`.

Les engagements doivent provenir d'une source indépendante de confiance. Cette
fonction ne choisit pas le checkpoint, ne prouve ni sa fraîcheur ni sa finalité
et ne revalide pas les preuves de possession historiques. L'admission de snapshots
de pairs, la politique de bootstrap/reprise et la composition dans `run_node`
restent à terminer. Aucune activation réseau ou liaison EVM n'est revendiquée.

### Composition dans `nano run`, 25 septembre 2026

`start_altair_runtime` compose profil, ancre, Store durable et service RPC avec
une configuration exacte, sans defaults de consensus. Les identités de chaîne
proviennent du genesis d'exécution ; le fork digest dérive de l'ancre. Les budgets
worker/transport sont vérifiés pour pouvoir transporter les messages du profil.
Les handles du service et du réacteur retiennent un propriétaire commun, qui
draine le worker avant de fermer la base.

`nano run` peut injecter ce runtime via le champ `altair`, uniquement en RPC
loopback, sans signataire BFT historique configuré et avec une base distincte.
Le mode initial refuse tout checkpoint déjà présent. La reprise exige un token
exact et vérifie aussi profil et ancre, sans retour implicite à un Store neuf.
Le journal `altair-runtime-ready` expose le token initial et précise explicitement
l'absence de protocole réseau Altair actif et de liaison EVM.

Les tests couvrent les configurations refusées, le maintien des durées de vie par
le handle RPC, l'initialisation et la reprise aux séquences 1 puis 2, et le refus
d'un token devenu ancien. Le cas géré lance les vrais exécutables `nano` et
`nano-p2p`, vérifie le RPC authentifié, arrête puis relance avec le token attendu.
Ce sont des processus locaux, pas un réseau de validateurs Altair.

Configuration et limites : `docs/ALTAIR-RUNTIME.md`. Restent le pilotage local de
l'horloge, l'appel/négociation depuis le daemon, le suivi des engagements après
commit, la récupération des acquittements perdus et la liaison EVM. Aucun fichier
de déploiement n'active ce runtime par défaut. Preuves :
`build-cmake/qualification/altair-managed-runtime-20260925/`.

### Horloge locale et statut du checkpoint, 25 septembre 2026

`AltairLocalClock` est désormais composable dans le runtime via une section
`clock` explicite. Activée, elle échantillonne l'heure système UTC, effectue les
ticks périodiques sans accumuler de timers et actualise l'heure avant le traitement
des messages. Un instant inchangé ne crée pas de checkpoint supplémentaire. Les
reculs, sauts au-delà de la borne locale ou erreurs de traitement verrouillent
l'horloge en erreur jusqu'au redémarrage ; les messages suivants ne sont plus
traités. Une annulation de requête seule ne verrouille pas l'horloge.

Le statut privé `nano_altairStatus(token)` expose séquence, digest, heure du Store,
identité du profil wire et état de santé de l'horloge. Il est désactivé sans service,
authentifié comme la réception et exécuté hors verrou RPC dans la file bornée.
Cela permet de lire un engagement après commit, sans prétendre conserver cet
engagement sur un support indépendant ni détecter seul un rollback du disque.

Les cinq suites RPC, HTTP, HTTP Altair privé, runtime et runtime géré passent.
Huit gardes d'horloge utilisent une source temporelle déterministe injectée :
déduplication, annulation, progression, recul, erreur persistante, thread périodique,
actualisation avant message et grand saut refusé sans nouveau commit. Le runtime
géré conserve une horloge désactivée pour son replay déterministe ; son test ne
qualifie donc pas la dérive des horloges de plusieurs hôtes. Le contrôle d'accès au
statut est aussi testé sur HTTP loopback.

Preuves : `build-cmake/qualification/altair-local-clock-20260925/`. Chaque tick
réel persiste encore le Store complet : cadence, amplification d'écriture et
performances sous charge restent ouverts. Les messages P2P Altair, les pertes
d'acquittement et la liaison EVM restent à raccorder ; aucune activation réseau.

### Tickets bornés et perte de réponse HTTP, 25 septembre 2026

Le service Altair expose un ledger de tickets par session de processus, borné par
le nombre de jobs configuré. Une reprise du même numéro/payload retrouve la
réponse sans redispatch. Un numéro réutilisé avec un autre contenu est refusé.
Les résultats restent présents jusqu'à retrait explicite et aucun travail actif
n'est évincé. Un watermark interdit la réadmission des numéros retirés/anciens.
L'annulation d'un numéro pas encore admis le réserve aussi contre un Submit HTTP
retardé. Pour les travaux actifs, la décision de fin reste distincte de la demande
d'annulation, et un commit déjà fait n'est jamais converti en rejet.

Les endpoints privés Submit/Job/Cancel/Forget/Jobs complètent Status. Le callback
de fin publie atomiquement le résultat, y compris avec une fin synchrone ou un
callback doublé. Un résultat local_failure n'est pas présenté comme un rejet
consensus. Le ledger ne prétend pas persister les reçus : une nouvelle session
après redémarrage implique un résultat inconnu à rapprocher du Store durable.

Les suites RPC, HTTP, admission wire, HTTP Altair privé et ledger passent. Le test
HTTP envoie un bloc, vérifie son admission depuis une autre connexion, ferme la
première sans lire sa réponse, puis resoumet le même ticket et récupère son
résultat. Le contrôle de séquence du checkpoint prouve un seul commit de ce bloc.
Les contrôles d'accès des cinq nouveaux endpoints sont vérifiés. Le test natif
couvre saturation, refus d'effacement actif, retry, conflit de contenu, annulation,
callback doublé, nouvelle session et admission retardée après Cancel.

Preuves : `build-cmake/qualification/altair-receipt-recovery-20260925/`.
Le daemon n'appelle pas encore ce protocole IPC. La récupération après changement
de session, l'admission P2P complète et la liaison EVM restent ouvertes.

### Client IPC avec tickets côté daemon, 25 septembre 2026

`network/altair_core_dispatch.hpp` fournit maintenant un dispatcher asynchrone
compatible avec le Stream Altair et `Core::call`. Les messages admis localement
restent bornés, les admissions sont ordonnées et une réponse Submit perdue est
résolue par recherche du même ticket. Une réponse Forget perdue ne perd pas la
décision déjà confirmée. Les doubles callbacks sont ignorés. Une réponse de
mauvaise séquence ou une indisponibilité dépassant le budget de tentatives bloque
le client en conservant ses admissions ; aucune fausse fin de worker n'est émise.

Les onze suites du module réseau passent. Les huit scénarios du dispatcher
couvrent pertes Submit/Forget, ticket inconnu puis retry identique, annulation avec
admission incertaine, saturation, cœur indisponible, réponse incohérente et arrêt
avec requête locale en attente. Le test `consensus_altair_core_client` utilise le
vrai client HTTP `Core::call` contre le serveur et le service durable : deux blocs
officiels sont commités une fois chacun, malgré une réponse d'admission supprimée
à la frontière du callback client. Le test distinct HTTP privé ferme réellement
une connexion sans lire sa réponse et reste vert.

Preuves : `build-cmake/qualification/altair-core-dispatch-20260925/`.
Ce composant n'est pas encore instancié par le daemon. Restent découverte et
vérification du profil, récupération des tickets initiaux, handler P2P et essais
entre plusieurs nœuds. Aucun protocole Altair réseau n'est activé par cette étape.

### Identité sémantique du runtime, 25 septembre 2026

Le statut privé expose une empreinte SHA-256 de l'identité de chaîne, du profil
complet de transition, du choix de branche et des engagements d'ancre, ainsi que
les limites wire du cœur. Douze modifications de règles/identité/ancre changent
l'empreinte ; un treizième scénario vérifie sa stabilité avec un autre chemin,
d'autres budgets de worker et un fichier de profil reformatté. Le redémarrage
avec un checkpoint plus récent conserve aussi l'empreinte.

Quatre suites passent : runtime, HTTP privé, client IPC réel et démarrage/reprise
des processus Nano/P2P. Preuves :
`build-cmake/qualification/altair-consensus-identity-20260925/`.
La découverte par le daemon, la négociation P2P, la récupération initiale des
tickets et l'intégration du handler restent ouvertes. Aucun fork ni réseau public
activé ; aucune clôture de conformité EIP revendiquée.

### Clôture des sessions IPC avant reprise, 25 septembre 2026

Le RPC privé `nano_altairRotateSession` change la session attendue sous le même
verrou que Submit. Les anciennes requêtes retardées sont refusées ; les tickets
actifs restent visibles, annulables et non effaçables avant leur fin. Les décisions
commitées et le watermark sont conservés. Une répétition avec l'ancienne session
échoue sans autre rotation.

Cinq suites passent (ledger, HTTP privé, client IPC, runtime et processus gérés),
avec 64 courses Submit/rotation et des requêtes d'ancienne session sur HTTP réel.
Preuves : `build-cmake/qualification/altair-session-fence-20260925/`.
Cela prépare la récupération du daemon ; ni son orchestration, ni un réseau Altair
actif ne sont démontrés. La clôture porte sur les tickets, pas sur l'ancien appel
synchrone Receive. Le propriétaire unique et le passage exclusif de l'admission
réseau par les tickets restent des exigences d'intégration.

### Récupération IPC raccordée au daemon, 25 septembre 2026

`network/main.cpp` découvre désormais le service Altair du cœur et exécute
`AltairCoreRecovery` avant de construire son client à tickets. Cette récupération
vérifie chaîne/profil/limites, confirme une rotation de session, attend et retire
les anciens tickets, puis revérifie identité et session. Un résultat prêt exige
un inventaire vide. Une rotation dont la réponse est perdue doit être confirmée
à nouveau après lecture du statut. Les erreurs sont bornées par une échéance
globale et un budget de transport ; aucune expiration ne vaut fin de worker.

Les douze suites réseau passent, avec seize scénarios de récupération contrôlée.
Quatre suites Altair passent sur le cœur : runtime, HTTP privé, client IPC et
processus gérés. Le client réel récupère un ticket antérieur malgré une réponse
de rotation supprimée à sa callback ; Nano et son vrai daemon annoncent
`altair-ipc-ready` au démarrage et après reprise du checkpoint. Le test réseau
historique passe également avec les nouveaux binaires.

Preuves : `build-cmake/qualification/altair-daemon-recovery-20260925/`.
Cette préparation IPC n'enregistre pas le handler libp2p Altair, ne signe pas de
blocs et ne lie pas le consensus à l'EVM. Le propriétaire unique du daemon, la
supervision après échec et la configuration opérateur des budgets de récupération
restent à finaliser avant activation. Le chemin synchrone Receive reste hors de
la clôture des tickets ; aucune admission réseau ne doit l'utiliser.

### Admission Altair sur libp2p, 25 septembre 2026

Le mode explicite `altair.networkAdmission: true` exige l'horloge locale, ferme
l'ancien chemin synchrone Receive et permet au daemon d'enregistrer le handler
`/nano/altair/1.0.0` après récupération IPC. Le pair doit avoir effectué le handshake
sur sa véritable identité libp2p et annoncer le même profil complet. Les messages
passent ensuite par le worker et le Store durable ; le profil annoncé ne remplace
aucune validation SSZ/BLS. Le budget partagé n'est acquis qu'une fois et n'est
libéré qu'après fin effective du traitement.

Treize suites réseau et six suites cœur/intégration passent. Le nouveau trajet
réel Noise/libp2p → daemon → HTTP → réacteur transporte le payload maximal de
8 Mio (invalide, rejeté par le cœur), importe deux blocs officiels et contrôle leurs
checkpoints/heads. Neuf identités de pairs distinctes couvrent refus sans handshake,
absence/différence de profil, enveloppes invalides, rejets du cœur et panne locale.
L'horloge est celle des fixtures ; aucune finalité distribuée n'est revendiquée.

Le profil de 8 Mio révélait un dépassement du JSON-RPC de 16 Mio avec l'encodage
hex. L'IPC du dispatcher utilise désormais une enveloppe objet base64 canonique,
sans modifier le format réseau, le profil ni la limite HTTP. Un retry hex/base64
du même payload conserve le même ticket. La capacité totale est vérifiée avant
ouverture de la base ; les profils plus grands nécessitant des fragments restent
refusés explicitement et à implémenter.

Preuves : `build-cmake/qualification/altair-libp2p-ingress-20260925/`, y compris
l'échec initial et les réussites après correction. Aucun déploiement n'active le
mode par défaut. Restent propagation sortante, récupération des dépendances,
duties/signatures de validateurs, supervision multi-processus et liaison atomique
avec les blocs EVM, puis qualification distribuée des pannes/finalité/charge.

### Archive signée atomique pour la propagation, 25 septembre 2026

Le réacteur persiste désormais les octets SSZ signés des nouveaux blocs dans le
même commit que le Store. Les archives immuables ne sont publiées qu'après succès
de l'écriture synchronisée. Le format durable v2 enveloppe le Store et les blocs ;
les checkpoints v1 sans octets signés restent lisibles sans données inventées.
Une racine déjà connue ne remplace pas une signature archivée et ne remplit pas
une lacune historique par le raccourci des doublons.

Deux RPC privés, `nano_altairInventory` et `nano_altairBlock`, fournissent une
pagination bornée et les payloads compacts à partir de la même vue publiée.
Le test libp2p exporte les deux blocs réellement reçus et compare leurs octets.
La restauration vérifie digest, SSZ, en-tête, racine et signature ; cinq archives
altérées sont refusées même après recalcul du digest externe dans le test.

Huit suites passent. Les parcours officiels head/reorg couvrent 18 scénarios,
366 admissions de blocs et 17 reprises. Le test d'archive ajoute annulation juste
avant commit, erreur de synchronisation WAL injectée, absence de publication sur
échec, reprise sans séparation état/archive, réouverture et lecture v1. Ce n'est
pas une qualification de coupure électrique sur stockage de production.

Preuves : `build-cmake/qualification/altair-signed-archive-20260925/`.
La diffusion automatique reste à raccorder. Les archives font encore partie des
snapshots complets : stockage incrémental, pruning, réhydratation authentifiée des
anciens blocs manquants et débit durable restent ouverts, ainsi que la liaison EVM.


### Propagation Altair automatique qualifiée localement — 25 septembre 2026

Le relais de blocs utilise l'archive signée durable et des curseurs par pair,
avancés uniquement après acquittement positif. Catalogue stable sur tick/ajout,
réinitialisation lors d'un changement de session ou d'époque du catalogue.
La campagne `build-cmake/qualification/altair-block-propagation-20260925/`
passe 14 tests réseau et 3 tests cœur, dont deux vrais daemons/deux cœurs HTTP,
perte d'acquittement et redémarrage du daemon source avec deux blocs officiels.
Voir `docs/ALTAIR-RUNTIME.md` pour le périmètre exact et les limites.
Aucun statut EIP ou profil réseau n'est promu : finalité distribuée, tâches et
signatures de validateurs, gossip des votes, réconciliation après rejet permanent
et liaison atomique EVM/consensus restent à terminer.


### Rattrapage après panne du cœur Altair — 25 septembre 2026

`consensus_altair_restart_libp2p` passe avec un vrai processus cœur récepteur,
deux daemons libp2p, deux `SIGKILL` suivis de réouvertures de RocksDB et un
émetteur laissé actif. Le récepteur retrouve son engagement séquence/digest,
change de session de gossip, récupère le bloc officiel manqué et conserve les
SSZ ainsi que la tête attendue après la seconde réouverture. Preuve locale :
`build-cmake/qualification/altair-core-crash-catchup-20260925/verified.log`.
Le test utilise une horloge de fixture et un engagement externe connu ; il
n'interrompt pas un commit en cours et ne simule pas de coupure électrique.
Aucun EIP, profil réseau ou statut de production n'est promu par cet essai.


### Attestations signées durables — 25 septembre 2026

Le réacteur archive les SSZ canoniques des attestations qui modifient les
derniers votes, atomiquement avec l'état de consensus (`nano-altair-durable-v3`).
Les doublons n'amplifient pas l'archive ; la fenêtre courant/précédent borne sa
rétention, avec limites de ressources explicites et restauration revalidant
les signatures. Les formats v1/v2 restent acceptés sans backfill implicite.
Qualification : `build-cmake/qualification/altair-signed-votes-20260925/` ; voir
`docs/ALTAIR-RUNTIME.md` pour les tests et limites. Le raccordement de ces votes
à la diffusion P2P reste ouvert, ainsi que les devoirs/signatures des validateurs,
la finalité distribuée et la liaison atomique EVM/consensus. Aucun statut EIP
ni profil réseau n'est promu.


### Diffusion P2P des attestations qualifiée localement — 25 septembre 2026

L'archive signée des votes est exposée par deux lectures privées authentifiées
et diffusée avec des curseurs distincts de ceux des blocs. Alternance par pair,
ACK obligatoire, reprise de catalogue après expiration et contrôle de profil.
La campagne `build-cmake/qualification/altair-vote-propagation-20260925/` passe
14 tests réseau et 3 tests P2P. Le scénario officiel à branche courte plus lourde
vérifie qu'une attestation reçue via deux vrais daemons change la tête distante ;
la perte d'ACK et l'expiration sont également vérifiées.
Pas de génération autonome de votes, de finalité multi-epoch distribuée ni de
preuve de débit à grande échelle. Les tâches/signatures des validateurs,
slashings P2P, réconciliation et liaison atomique EVM restent ouverts. Aucun
profil réseau ni statut EIP n'est promu par ces tests.


### Journal anti-slashing avant émission de signatures — 25 septembre 2026

Le nouveau composant `BeaconSigningJournal` est distinct du journal BFT
hauteur/tour. Il conserve atomiquement les dernières réservations par clé avant
signature BLS ; les conflicts, retours arrière et instances périmées sont refusés.
La qualification couvre 18 refus avant callback, signatures concurrentes,
réouverture, panne de synchronisation et crash avant retour de signature.
Voir `docs/BEACON-SIGNING-JOURNAL.md` et la campagne
`build-cmake/qualification/beacon-signing-journal-20260925/`.

Reste à raccorder les tâches authentifiées du validateur et la gestion des clés.
Ajouter l'import/export EIP-3076 v5 à la clôture de production : racines absentes,
fusion d'historiques et protection des zones sans historique. Ce format n'est
pas encore implémenté. Le journal local ne protège pas une clé utilisée ailleurs
ou un quorum de bridge compromis ; aucune finalité réseau n'est activée.

## Qualification multi-hôte après correction des propositions expirées

Révision b1ccbac : finalité commune 2→3→4→5 en 145763 ms, trois
validateurs sains et actifs (136/133/128 attestations locales admises).
Mac natif + Docker ARM64 + VPS AMD64, deux machines, 64 clés synthétiques.
La campagne précédente avec validateur arrêté reste classée en échec.
Les partitions WAN, l’endurance et le grand registre restent ouverts ; aucun
statut EIP ou production global ne découle de cette fenêtre. [Preuves](qualification/multihost-20260926/with-expiry/README.md).

### Correctifs locaux et nouvelle qualification WAN échouée

- `9e1b904` : expiration normale RANDAO/attestation reprise au slot suivant ;
  engagements durables conservés, erreurs fournisseur fatales. Scénarios locaux
  passés ; [preuves et limites](qualification/multihost-20260926/duty-expiry/README.md).
- `5da8484` : lecture par page de l'inventaire des votes déjà acquittés. Le
  scénario de 70 votes connus puis un vote nouveau passe de 71 à 3 lectures
  du cœur, avec le même budget de parcours et un seul nouvel envoi par tentative.
  [Preuves locales](qualification/multihost-20260926/relay-pages/README.md).

La campagne `5da8484` conserve ces seuils et **échoue** : après la coupure de
96 s, aucune nouvelle finalité commune sous 180 s. Les trois boucles restent
saines ; justification 7/7/7, finalité 2. Le VPS dépasse une fois la durée
du slot (6360 ms). L’asymétrie de visibilité des votes demeure.
[Preuves complètes](qualification/multihost-20260926/wan-pages/README.md).
Ces modifications ne changent pas les règles de fork ni le statut d'une EIP.

### Réception idempotente des votes — qualification locale

Les retransmissions SSZ identiques déjà authentifiées évitent désormais la
revalidation et la sérialisation globale. Signatures altérées, annulation et
réouverture contrôlées ; [preuves locales](qualification/vote-retry-20260927/README.md).
La reprise WAN reste non qualifiée : cette optimisation ne clôt aucun EIP.

La campagne WAN `b2e4ebe` **échoue** également : finalité restée à 2 sous
180 s après reconnexion, trois boucles saines, aucun cycle au-delà du slot.
Au slot 64, visibilité des votes précédents 22/21/64 : le relais sortant et
l’ordre de rattrapage restent à diagnostiquer.
[Preuves](qualification/multihost-20260927/vote-retry/README.md).

### Relais sortant par pair — qualification locale

Une tentative par pair, quatre au total par défaut, configurables via
`NANO_CONSENSUS_RELAY_CONCURRENCY`. Le test de pair lent, saturation et réponse
tardive passe ; le binaire P2P compile. [Preuves](qualification/peer-relay-20260927/README.md).
Effet sur la reprise WAN non encore qualifié ; aucun statut EIP clôturé.

La campagne `c41e7b4` échoue au délai de 180 s, mais une reprise tardive est
observée : finalité 8 commune dans les statuts terminaux et les échantillons
du slot 80 (~184 s après reconnexion). Au slot 72, les trois nœuds connaissent
64 votes précédents. Trois boucles saines. La fenêtre de progression durable
n’a pas été atteinte ; aucun reclassement en réussite.
[Preuves](qualification/multihost-20260927/peer-relay/README.md).

### Reprise WAN suivie de finalité soutenue — périmètre borné

Cœur `c41e7b4`, banc `14f1f15` : coupure 96 s, reprise commune observée
à 186922 ms (**échec du seuil initial 180 s**), puis **8→9→10→11 en 143118 ms**.
Trois boucles saines, checkpoints liés à l’exécution et audit de 207 observations
sans conflit ni régression. Sortie globale 1 conservée. La fenêtre après reprise
est qualifiée dans ce périmètre, pas le délai ni la production globale.
[Preuves](qualification/multihost-20260927/sustained-recovery/README.md).

### Diagnostic des transitions FFG après reprise WAN

Le checkpoint natif de la campagne ci-dessus montre 60,9 % de poids de cible
précédente au slot 64, puis quorum au slot 66 ; justification 8 au slot 72
et finalisation 8 au slot 80. Investigation suivante : arrivée et inclusion
des attestations avant la frontière d’époque. Aucun délai assoupli.
[Analyse reproductible et limites](qualification/multihost-20260927/ffg-recovery-analysis/README.md).

### Reprise WAN horodatée — redistribution à investiguer

Campagne `80493b7` : seuil 180 s encore échoué (187542 ms), puis trois avancées
8→11 en 141066 ms. Six votes inclus tard montrent 17,6–18,2 s entre fin du
relais natif→VPS et fin du premier relais VPS enregistré. Les mesures d’horloge
ne suffisent pas à expliquer cet écart ; analyser les files et le parcours
d’inventaire, sans assimiler cet écart à une latence réseau pure.
[Preuves et limites](qualification/multihost-20260927/timed-recovery/README.md).

### Relais : réutilisation des pages inconnues

Correction du gaspillage d’inventaire : 32 votes inconnus utilisent une seule
page RPC tout en conservant 32 exports et accusés indépendants. Contrôle
négatif sur l’ancien code, perte d’accusé et invalidation après export
couverts. Compilation du daemon réussie ; effet WAN encore non mesuré.
[Preuves](qualification/relay-page-cache-20260927/README.md).

### Cache des pages : résultat WAN

La campagne `a4bf5e6` échoue encore au seuil de reprise (189200 ms contre
180 s), puis progresse de 8 à 11 en 148066 ms. Au slot 64, seulement 53,1 %
du poids de cible précédente est inclus ; quorum au slot 66. Le gain local
de requêtes ne démontre pas un gain de finalité WAN. Trois boucles saines,
preuves et échec conservés ; aucun statut EIP clôturé.
[Campagne](qualification/multihost-20260927/page-cache-recovery/README.md).

### Possession des votes par le pair émetteur

Après admission durable d’un vote reçu, le relais retient sa possession
par la session émettrice et évite de le lui renvoyer. Rejets, erreurs,
refus de file, autres types et autres sessions sont isolés dans les nouveaux
contrôles. Le relais injecté envoie toujours au pair indépendant.
[Preuves locales](qualification/vote-sender-knowledge-20260927/README.md).
Compilation réussie ; aucun gain WAN ni critère production encore revendiqué.

### Reprise WAN qualifiée avec suppression des renvois

La campagne `c2ec4d7` respecte pour la première fois ce scénario WAN :
reprise commune en **141002 ms** après reconnexion (seuil inchangé 180 s),
puis **7→8→9→10 en 144025 ms**. Trois instances, deux hôtes, 64 validateurs
synthétiques, coupure 96 s ; trois boucles saines, sortie globale 0.
183 observations sans conflit de racine par époque ni régression.
[Preuves](qualification/multihost-20260927/noecho-recovery/README.md).
Les échecs précédents sont conservés. Une campagne ne prouve pas le gain
causal statistique, l'endurance, la charge ou les autres topologies ; aucun
EIP supplémentaire ni la production globale n'est déclaré achevé.

### Capella : changements des clés de retrait qualifiés

`consensus_capella_bls_change` implémente l'opération signée et sa transition
sur état SSZ authentifié : domaine genesis, autorité de la clé de retrait,
mutation atomique des credentials, rejet des doublons et annulation.
14 cas officiels épinglés (7 valides avec état complet identique, 7 invalides),
63 rejets locaux, 7 rejeux et 14 annulations. Contrôle négatif de signature
effectivement détecté ; régression Capella ciblée 3/3.
[Preuves](qualification/capella-bls-change-20260927/README.md).
L'ordre des opérations du bloc, l'activation, le réacteur et le transport
restent à raccorder. EIP-4895 demeure partiel, sans activation réseau.

### Capella : payload SSZ et retraits officiels

Le codec Capella complet est ajouté : offsets canoniques, budgets,
transactions et retraits séparés, racine SSZ du payload et de son header.
30 vecteurs officiels concordent ; 271 rejets locaux, 30 mutations de
retrait et 60 annulations sont vérifiés. Le même décodeur alimente les
54 cas officiels de retraits : 35 états SSZ complets identiques et 19 refus.
[Preuves](qualification/capella-payload-20260927/README.md).
Régression ciblée 5/5. Il reste à composer la transition payload/état EVM,
les slots/epochs, le bloc signé et sa publication durable. Aucun changement
du profil réseau Bellatrix ni clôture EIP-4895.

### Capella : couplage des retraits et de l'exécution Shanghai

Le payload reçu est maintenant relié à un état SSZ authentifié : parent
EVM, timestamp, RANDAO, liste de retraits attendue, débit consensus et
exécution Shanghai sont vérifiés avant retour d'un candidat privé unique.
Le header actualisé conserve les historiques et ajuste leur offset.
[Preuves](qualification/capella-paired-execution-20260927/README.md) :
PUSH0 exécuté, quatre retraits, sept refus dont un paiement excessif
EVM-valide, trois annulations et régression ciblée 6/6.
Le bloc signé complet, slots/epochs Capella, l'activation et le commit
durable restent ouverts ; aucun changement de profil réseau ni clôture EIP.

### Capella : racine du corps de bloc

`capella_beacon_body_root` engage le payload Shanghai et les changements
BLS signés, avec limites et offsets canoniques. 30 racines officielles,
330 refus locaux, 177 mutations et 60 annulations sont qualifiés ; les
cinq suites structurelles ciblées passent.
[Preuves](qualification/capella-body-20260927/README.md).
Il ne s'agit pas encore de la transition signée complète : slots/epochs,
composition, activation et persistance restent requis. Réseau inchangé.

### Capella : slots et époques

Les candidats Capella calculent les racines du bon fork, composent les
opérations d'époque et ajoutent les résumés historiques sans prolonger
l'ancienne liste. Sept scénarios officiels, 100 slots et états SSZ complets
concordants ; rejeu découpé, rollback tardif et annulations qualifiés.
Headers et curseurs non nuls sont conservés. Régression Altair/Bellatrix/
Capella 3/3. [Preuves](qualification/capella-slots-20260927/README.md).
La transition signée complète, l'activation et la publication durable restent
à raccorder. EIP-4895 et le réseau demeurent partiels.

### Capella : composition du bloc signé

La construction et l'admission Capella composent désormais les composants
qualifiés : slots/epochs, signature/proposant, retraits/Shanghai, opérations,
changements BLS, sync aggregate et racine postérieure. Neuf blocs synthétiques
signés franchissent une époque, changent une clé et versent un retrait au
balayage suivant ; 20 refus et 18 annulations conservent les parents.
[Preuves](qualification/capella-signed-transition-20260927/README.md).
Régression des transitions Altair/Bellatrix/Capella 3/3. Ce scénario ne vaut
pas corpus complet Capella. Activation, Store/réacteur, réseau et commit
durable restent ouverts ; aucun EIP ni objectif production n'est clôturé.

### Capella : migration de fork programmée

Migration Bellatrix→Capella authentifiée et calendrier explicite, y compris
frontière sans bloc et entrée au slot exact. Quatorze états officiels SSZ
complets concordent ; dix refus et trois annulations contrôlent les candidats.
[Preuves](qualification/capella-fork-20260927/README.md).
Reste le premier payload Shanghai sur parent Paris, puis la composition
Store/réacteur, l'activation et le commit durable. Le profil réseau ne change
pas ; EIP-4895 et l'objectif production restent partiels.

### Capella : premier payload Shanghai sur parent Paris

Le pont EL autorise maintenant la transition Paris→Shanghai sous contexte
Capella authentifié, tout en rejetant Shanghai→Paris et les profils ultérieurs.
Un état Bellatrix synthétique est migré puis reçoit quatre retraits et une
transaction PUSH0/SSTORE. Refus, annulations et régression 4/4 qualifiés.
[Preuves](qualification/capella-first-payload-20260927/README.md).
Restent le calendrier composé avec l'admission signée, Store/réacteur, commit
durable et qualification réseau. Aucun profil actif ni statut EIP clos ne change.

### Capella : calendrier composé avec les blocs signés

Construction et admission depuis Bellatrix composent désormais migration
programmée et transition Capella complète. Deux nouvelles chaînes de neuf
blocs couvrent le slot exact et trois slots sans bloc à la frontière, avec
parent EL Paris, signatures, retraits et PUSH0/SSTORE. 24 refus et 18 annulations
par nouvelle chaîne ; régression Altair/Bellatrix/Capella/migration 4/4.
[Preuves](qualification/capella-scheduled-block-20260927/README.md).
Restent Store/réacteur, profil durable, commit atomique et qualification réseau.
Les profils actifs et les statuts EIP partiels restent inchangés.

### Shanghai : intégrité et reprise du checkpoint d'exécution

Le commit atomique contrôle maintenant le corps des retraits contre sa racine
et refuse les retraits sans engagement. Qualification Shanghai : réorganisation,
CAS, archives, quatre SIGKILL/réouvertures et faute sync WAL conservée/perdue ;
montants restaurés contrôlés. Régression stockage Paris/Shanghai 4/4.
[Preuves](qualification/shanghai-checkpoint-20260927/README.md).
Le snapshot testé est synthétique : Store Capella, profil durable et réacteur
restent à raccorder. Aucun statut EIP clos ni profil réseau modifié.

### Capella : contextes du choix de branche

Helpers Capella pour registre lié au bloc, checkpoints réalisés/unrealized,
état cible et identité du proposant. Vingt-sept blocs signés contrôlés, six
contextes cibles, refus de headers non liés et de proposants sanctionnés ;
régression ciblée 3/3.
[Preuves](qualification/capella-fork-context-20260927/README.md).
Le Store complet, son snapshot durable et le réacteur réseau restent ouverts.

### Capella : Store de branches en mémoire

Store Capella avec états consensus/EVM couplés, admission signée, horloge et
sélection de tête. Trois chaînes de neuf blocs et trois branches concurrentes
qualifiées, avec refus de signatures/retraits altérés et annulations ; régression
3/3. [Preuves](qualification/capella-store-20260927/README.md).
Qualification spécifique votes/slashing/dépôts, snapshot, conservation des
branches à travers le fork, persistance et réacteur restent ouverts. Le Store
testé démarre sur une ancre Capella authentifiée ; réseau inchangé.

### Capella : snapshot local et restauration

Format local et profil canonique Capella, digest vérifié avant parsing,
registre justifié reconstruit depuis SSZ. Trois chaînes de neuf blocs avec
aller-retour de branches et cache qualifiés ; commit atomique du vrai snapshot
et réouverture RocksDB avec tête/racine EVM concordantes. Régression 3/3.
[Preuves](qualification/capella-snapshot-20260927/README.md).
Restent crash du vrai Store, votes/finalité, branches Bellatrix à travers le
fork et intégration réseau. Checkpoint local authentifié uniquement.

### Capella : crash et reprise du vrai Store

Six SIGKILL/réouvertures sur trois scénarios signés, en alternant les vrais
snapshots d'une chaîne courte et longue avec retrait. Cohérence du token,
du Store restauré, des états EVM et des index canoniques vérifiée ; aucun
commit acquitté perdu dans cette campagne.
[Preuves](qualification/capella-store-crash-20260927/README.md).
Crash de processus uniquement, pas panne électrique. Votes/finalité réseau,
Store vivant traversant le fork et réacteur restent à qualifier/raccorder.

### Capella : votes et réorganisations avec états EVM distincts

Dix-huit votes BLS valides et dix-huit signatures altérées testés, six bascules
de tête/EVM entre deux branches réellement différentes. Trois preuves de
double vote marquent les équivoques ; snapshots avec votes et exclusions
restaurés exactement.
[Preuves](qualification/capella-votes-20260927/README.md).
Qualification d'admission locale et de LMD-GHOST, pas de finalité FFG
multi-époque, de pénalité économique ni de réseau Capella.

### Capella : finalité FFG locale sur plusieurs époques

Deux nouvelles chaînes de 33 blocs avec votes inclus : participation complète,
256 attestations et finalité époque 2 ; contrôle à 50 %, 128 attestations et
aucune progression de justification/finalité. Restauration et crashs conservés ;
régression 3/3. [Preuves](qualification/capella-finality-20260927/README.md).
Preuve locale au Store, pas de réseau multi-hôte. Réacteur, activation du Store
à travers les forks et qualification distribuée restent ouverts.

### Capella : coordinateur de publication durable

Blocs, votes, équivoques et ticks peuvent maintenant publier le Store et le
suffixe EVM dans un commit atomique CAS. Comparaison après événements,
doublons sans réécriture, annulations et redémarrages qualifiés sur cinq
chaînes ; trois suites réussies.
[Preuves](qualification/capella-reactor-20260927/README.md).
Les erreurs disque injectées sont désormais qualifiées localement (voir ci-dessous).
Restent les archives/notifications,
la signature locale, le propriétaire Blockchain et le réseau, ainsi que le
Store vivant traversant les forks. Aucun profil réseau activé.

### Capella : erreur WAL du coordinateur et reprise

Injection EIO pendant la publication d'un bloc signé : cinq accès bloqués après
écriture incertaine. Réouverture avec suffixe conservé ou perdu, snapshots et
index canoniques vérifiés, rejeu identique dans les deux cas. Quatre suites
passent ; le contrôle supplémentaire des index perdus est également vert.
[Preuves et limites](qualification/capella-reactor-sync-20260927/README.md).
Qualification macOS locale seulement ; authentification du token de reprise,
raccordement réseau et migration du Store vivant restent à clôturer.

### Capella : archives atomiques des blocs signés

Le coordinateur conserve désormais le bloc signé exact avec son Store et son
suffixe EVM ; lecture vérifiée SSZ/BLS, doublons sans remplacement. Qualification
locale : 91 archives retrouvées après reprise, branches alternatives conservées,
WAL conservé/perdu cohérent et neuf archives altérées refusées.
[Preuves](qualification/capella-signed-archive-20260927/README.md).
Votes/sorties/preuves de slashing archivés, rétention, notifications, signature
locale, raccordement Blockchain/RPC/P2P et Store inter-forks restent ouverts.
Les checkpoints historiques ne sont pas rétroalimentés en blocs signés.

### Capella : votes gossip durables et application différée

Enveloppe locale versionnée : votes signés et Store engagés atomiquement,
validation SSZ/BLS à la reprise. Les votes du slot courant sont conservés avant
acquittement, appliqués au slot suivant ; doublons sans réécriture et rétention
sur epoch courante/précédente. Dix-huit votes immédiats et douze différés
retrouvés après réouverture ; signatures altérées et entrées dupliquées refusées.
[Preuves](qualification/capella-durable-votes-20260927/README.md).
Restent notamment archives sorties/slashings, file des parents inconnus,
signature locale, notifications, intégration réseau et Store vivant inter-forks.
Aucune activation publique ni conformité réseau complète revendiquée.

### Capella : preuves de double vote durables

Enveloppe locale v2 avec preuves SSZ signées et exclusion fork-choice engagées
atomiquement. Reprise authentifiée, variantes d'un même fautif sans croissance
de l'archive : trois preuves restaurées, dix-huit répétitions sans réécriture,
neuf checkpoints altérés refusés. Quatre suites réussies.
[Preuves](qualification/capella-durable-slashings-20260927/README.md).
Ce n'est pas une sanction financière automatique. Restent inclusion locale dans
les blocs, retrait des preuves consommées finalisées, sorties, signature locale,
notifications, réseau et migration du Store inter-forks.

### Capella : devoirs et signature locale d'attestations

Comités, checkpoint source/cible et domaine dérivés du Store authentifié ;
signature protégée par le journal anti-slashing existant et contrôle d'horloge.
Douze attestations locales comparées, reprise du journal et conflits refusés ;
journaux d'une autre chaîne et résultats expirés rejetés.
[Preuves et correction d'une attente de test](qualification/capella-local-attestation-20260927/README.md).
Restent RANDAO/propositions locales, ordonnanceur, diffusion réseau, exploitation
des journaux et migration inter-forks. Aucune activation réseau Capella.

### Capella : RANDAO et propositions locales protégées

Proposant/domaine dérivés du Store ; corps exécuté avant réservation du header
dans le journal anti-slashing. Quatre suites réussies, 88 révélations RANDAO et
88 blocs signés localement comparés puis reçus sur cinq chaînes. Doubles
propositions après réouverture, corps invalides et signatures expirées refusés.
[Preuves](qualification/capella-local-proposal-20260927/README.md).
Restent assemblage automatique des corps, ordonnanceur, diffusion réseau,
qualification dépôts natifs et première proposition lors de la migration du
Store vivant. Les descendants d'une ancre Capella sont seuls qualifiés ici.

### Capella : assemblage local du corps de bloc

Retraits calculés depuis SSZ authentifié, payload Shanghai exécuté, transactions
sélectionnées par nonce avec budgets explicites ; inclusion bornée des preuves
et votes archivés. Vingt-neuf corps assemblés puis admis sur cinq scénarios,
reprise des cinq blocs supplémentaires et trois fautifs marqués slashed.
Sept suites réussies, dont les contrôles de retraits directement affectés.
[Preuves](qualification/capella-local-body-20260927/README.md).
La finalité produite entièrement par l'assembleur est qualifiée ci-dessous.
Le raccordement mempool propriétaire est qualifié ci-dessous. Restent sorties/changements BLS, contributions sync,
ordonnanceur et réseau.

### Capella : finalité issue de la boucle locale complète

Deux Stores durables indépendants et production entièrement locale : 40 blocs,
272 votes, epoch 3 finalisée. Contrôle avec moitié des identités participantes :
40 blocs, 133 votes, aucune finalité. Stores et journaux réouverts au slot 24,
production poursuivie jusqu'au slot 41 sans divergence.
[Preuves et périmètre](qualification/capella-assembled-finality-20260927/README.md).
Il s'agit de deux coordinateurs dans un processus, sans transport P2P : aucune
qualification distribuée ni activation publique n'est déduite de ces résultats.

### Capella : raccordement au propriétaire d’exécution

Le coordinateur durable publie désormais état, tête, finalité, mempool et événements
via le commit Beacon atomique du propriétaire `Blockchain`. Assemblage depuis
la mempool ; lectures fermées avant attachement et après invalidation/détachement.
Deux propriétaires indépendants : 40 blocs, 272 votes, époque 3 finalisée,
réouverture au slot 24 et contrôles directs des RPC latest/finalized/nonce.
[Preuves et limites](qualification/capella-execution-owner-20260927/README.md).
Le Genesis Shanghai est construit dans le test ; le démarrage public reste Paris.
Restent migration du Store vivant, ordonnanceur/P2P/services Beacon, réorganisations
et pannes d’écriture avec propriétaire, sorties/changements BLS et contributions sync.

### Capella : réorganisations et panne WAL avec propriétaire

Deux nouveaux scénarios qualifient le raccordement `Blockchain` : bascule vers
une branche vide puis retour, réinsertion/retrait de la transaction dans la mempool,
index et reçu RPC cohérents ; erreur fsync injectée, huit accès directs bloqués
et RPC fermé, reprise après conservation ou perte du suffixe WAL puis rejeu.
Quatre suites réussies en 28,78 s, dont deux contrôles existants affectés.
[Preuves](qualification/capella-owner-resilience-20260927/README.md).
Périmètre local synthétique, sans panne électrique physique ni réseau. Le test
fournit le jeton de reprise correspondant à l’issue connue ; l’authentification
indépendante de ce jeton reste requise pour le runtime de production.
Restent migration inter-forks du Store vivant, ordonnanceur/transport Capella,
files de sorties/changements BLS, contributions sync et qualification dépôts natifs.

### Capella : reprise à partir d’un témoin séparé

Le coordinateur peut écrire un journal témoin avant chaque commit principal :
jetons précédent/suivant, snapshot et bloc signé nouvellement archivé. La reprise
accepte exclusivement l’état précédent ou suivant attesté par ce témoin et termine
la publication précédente si nécessaire, archive signée comprise.
Trois cas qualifiés : erreur fsync principale avec suffixe conservé ou perdu,
et erreur fsync du témoin avec intention conservée et principal inchangé.
Témoin vide, signature d’archive corrompue et rollback trop ancien refusés.
Cinq suites réussies en 62,04 s, dont finalité/reprise et réorganisations raccordées.
[Preuves](qualification/capella-witness-recovery-20260927/README.md).
Le témoin reste une source de confiance locale indépendante à provisionner ; deux
répertoires sur le même disque ne protègent pas d’un rollback ou d’une perte commune.
Perte du suffixe du témoin lui-même non qualifiée ici. Raccordement au runtime,
migration du Store vivant, ordonnanceur et P2P Capella restent ouverts.

### Capella : horloge et ordonnanceur des validateurs locaux

Ajout de `CapellaLocalClock` et `CapellaValidatorLoop` : sélection des devoirs depuis
le Store, proposition depuis la mempool, attestation et admission normale des résultats.
Préparation/signature de proposition sérialisées sous le verrou du coordinateur.
Horloge indépendante, regroupement des polls par phase et fermeture de l’exécution
sur discontinuité ; timings d’epoch vérifiés contre le profil de consensus.
Le nouveau scénario produit 40 blocs et 272 votes, finalise l’epoch 3 et reprend
au slot 24 avec témoin et 64 journaux réouverts. Aucun doublon de signature lors
des polls répétés ; annulation et arrêt du worker contrôlés.
[Preuves et limites](qualification/capella-local-scheduler-20260927/README.md).
Il s’agit de l’ordonnanceur local : démarrage CLI/configuration, chargement des clés,
service RPC/Beacon, transport P2P et migration du Store inter-forks restent à raccorder.

### Capella : démarrage local par configuration

`start_capella_runtime` charge profil/snapshot authentifiés, limites, horloge,
clés BLS vérifiées contre le registre, journaux et témoins de reprise ; arrêt propre
avec export des engagements. Seize blocs produits via mempool après chargement de
64 clés synthétiques, avec arrêt/réouverture intermédiaire depuis les témoins du
consensus et des signataires. Quatre configurations invalides refusées.
[Preuves](qualification/capella-configured-runtime-20260927/README.md), nouveau test
réussi en 21,43 s. Aucun changement Bellatrix ni répétition des suites inchangées.
Cette factory C++ ne constitue pas encore le branchement CLI : dispatch CLI,
services RPC/Beacon, transport P2P et migration inter-forks restent ouverts.

### Capella : lectures Beacon authentifiées

Projection des états root/fork/finalité, genesis et paramètres ; en-têtes et blocs
signés JSON/SSZ par tête, finalité, slot ou racine, avec retraits et changements BLS.
Lecture Store/archive sous le même verrou ; blocs non archivés explicitement absents.
Quatorze lots de contrôles sur neuf blocs et six votes/deux réorganisations ;
indicateurs canonical/finalized des branches vérifiés. Handler du runtime vérifié
sur seize blocs/reprise et en-tête finalisé contrôlé après la boucle de quarante blocs.
[Preuves et corrections des essais initiaux](qualification/capella-beacon-reads-20260927/README.md).
Cinq suites distinctes réussies, dont les régressions Beacon Bellatrix.
La lecture Capella est raccordée au runtime C++ ; serveur HTTP/dispatch CLI,
worker d’admission privé, observations de pairs, P2P et migration inter-forks restent ouverts.

### Capella : admission bornée des messages

Service local pour blocs signés, attestations et preuves de slashing, avec identité
liée à la chaîne, au genesis, à l’ancre et au profil complet. File ordonnée bornée
en tâches et octets, annulation avant commit, arrêt avant export des engagements.
Capacités validées avant démarrage des signataires. Dix-sept messages exercés,
deux réorganisations, enveloppes étrangères et saturations rejetées ; runtime
qualifié sur seize blocs avec reprise et cinq configurations invalides refusées.
[Preuves et périmètre](qualification/capella-wire-admission-20260927/README.md).
Deux suites initiales réussies ; seule la suite runtime modifiée a été rejouée.
Il reste à raccorder reçus RPC privés, inventaire/export/synchronisation, serveur
HTTP/CLI et libp2p avec budgets par pair authentifié, puis migration inter-forks.
Ce worker C++ ne constitue pas une qualification réseau ni une activation publique.

### Capella : reçus d’admission privés

Le service expose soumission, consultation, annulation, retrait et rotation de
session sur la même file bornée. Six contrôles ajoutés au test runtime : résultat
commis, répétition identique, conflit de contenu, rejeu retiré, annulation anticipée
et ancienne session. Test réussi en 21,55 s avec seize blocs et reprise.
[Preuves](qualification/capella-admission-receipts-20260927/README.md).
Les reçus sont locaux au processus : perte de session signifie résultat inconnu.
Les points d’entrée C++ restent à exposer par RPC privé authentifié ; inventaire,
export, synchronisation et transport libp2p restent ouverts.

### Capella : export cohérent des archives

Export des blocs signés, votes et preuves de slashing avec identité réseau et
checkpoint lus sous un même verrou ; contrôle facultatif du checkpoint attendu,
limite SSZ et annulation. L’ancre de confiance reste sans signature fabriquée.
Seize messages exportés puis décodés à l’identique après deux réorganisations ;
checkpoint périmé et budget insuffisant refusés. Test enrichi réussi en 5,38 s.
[Preuves](qualification/capella-archive-export-20260927/README.md).
Cette primitive C++ ne clôt pas la synchronisation : pagination d’inventaire,
transport, authentification RPC et budgets réseau restent à raccorder.

### Capella : inventaire paginé lié au checkpoint

Parcours des racines ordonnées avec curseur, nombre d’entrées et taille JSON bornés.
Continuation uniquement avec checkpoint inchangé ; curseurs inconnus refusés.
Seize messages retrouvés sans doublons puis exportés après deux réorganisations,
par pages de deux entrées maximum et 512 octets ; test enrichi réussi en 5,28 s.
[Preuves](qualification/capella-inventory-20260927/README.md).
Les blocs listés sont des candidats connus, dont l’export vérifie la disponibilité
signée. L’ordre par racine n’est pas l’ordre des parents. La pagination sous
activité nécessite encore une stratégie de catalogue stable ; RPC/P2P et
ordonnancement des dépendances restent ouverts avant synchronisation complète.

### Capella : catalogue immuable pendant l’activité

Capture bornée des seules racines avec checkpoint et type de message ; pagination
sans retenir les états EVM ni les payloads. Un catalogue de quatre blocs capturé
au slot 5 reste identique après production supplémentaire et deux réorganisations.
Budget insuffisant et curseur inconnu refusés ; test enrichi réussi en 5,30 s.
[Preuves](qualification/capella-stable-inventory-20260927/README.md).
La pagination locale ne dépend plus des ticks suivants. Restent quotas/expiration
des handles RPC, ordonnancement des parents et transport authentifié. Le catalogue
ne garantit pas la rétention future des payloads après élagage.

### Capella : quotas des sessions d’inventaire

Registre de catalogues avec handles aléatoires, propriétaire, expiration sans
prolongation à la lecture, plafond de handles et budget agrégé des racines.
Lecture/fermeture étrangères refusées ; saturation, libération et expiration
qualifiées dans le test enrichi (5,35 s).
[Preuves](qualification/capella-catalog-sessions-20260927/README.md).
Le futur adaptateur doit fournir le propriétaire authentifié et les budgets
de requête. Raccordement runtime/RPC et synchronisation distribuée restent ouverts.

### Capella : cycle de vie des catalogues dans le runtime

Configuration optionnelle `catalogs` (handles, rootBytes, ttlMs), validée avant
les signataires ; propriétaire partagé avec le coordinateur. L’arrêt invalide
les catalogues et interdit leur réouverture. Runtime : seize blocs avec reprise,
six configurations invalides refusées et trois contrôles de catalogue réussis.
Régression du registre également verte : deux suites en 26,52 s.
[Preuves](qualification/capella-runtime-catalogs-20260927/README.md).
Les services sont raccordés au runtime C++, pas encore aux méthodes HTTP privées
ni au dispatch CLI/P2P. Ces raccordements restent des critères ouverts.

### Capella : méthodes RPC privées raccordées

Injection explicite du runtime dans `Rpc`, exclusive des services Altair/Bellatrix.
Méthodes de statut, reçus, export et catalogues protégées par le jeton privé ;
écoute locale obligatoire via la politique existante du serveur HTTP. Les porteurs
du jeton forment un principal administratif commun, pas des identités P2P distinctes.
Scénario runtime enrichi vert (21,43 s), régression RPC générale verte (6,27 s).
[Preuves et signatures des méthodes](qualification/capella-private-rpc-20260927/README.md).
Qualification limitée au répartiteur `Rpc::handle` : sockets HTTP, dispatch CLI et
synchronisation entre pairs restent à qualifier/raccorder avant activation.

### Capella : qualification HTTP privée

Nouveau scénario `consensus_capella_private_http` : serveur réel sur port loopback
éphémère et client persistant. Écoute publique refusée, mauvais jeton rejeté et
requête d’origine navigateur interdite même avec jeton correct. Soumission/reçu,
export et cycle des catalogues passent par HTTP ; scénario réussi en 17,24 s.
[Preuves](qualification/capella-private-http-20260927/README.md).
Il reste le dispatch CLI, la synchronisation et ses dépendances, puis le transport
libp2p. Ce test local ne prouve pas une qualification distribuée ou sous surcharge.

### Capella : genesis Beacon/Shanghai depuis JSON

Le chargeur accepte maintenant Shanghai explicitement pour Beacon, avec domaine
Capella distinct et engagement de retraits vides. Retraits non vides ou racine
incohérente refusés ; règles Paris inchangées. Les barrières avant vérification du
checkpoint sont testées à l’ouverture et à la réouverture pour les deux profils.
Tests `beacon_execution_gate` et `chain` verts en 4,28 s.
[Preuves](qualification/capella-json-genesis-20260927/README.md).
Ce changement est un prérequis du démarrage CLI : sélection du runtime dans
`run_node`, transport P2P et migration d’une chaîne existante restent ouverts.

### Capella : démarrage du binaire en mode serve

`nano serve` charge le runtime depuis la section `capella` du fichier de service,
exige un jeton privé hexadécimal de 32 octets et l’écoute loopback, puis raccorde
RPC et lectures Beacon. Arrêt HTTP avant export des engagements du runtime.
Nouveau test : deux démarrages réels avec reprise témoin et lectures HTTP, refus
sans jeton et avec écoute publique (1,43 s). Régression CLI existante verte (0,89 s).
[Configuration et preuves](qualification/capella-cli-serve-20260927/README.md).
Ce scénario CLI ne produit pas de blocs et n’utilise pas de clés validateur.
Le mode `run` avec P2P, la synchronisation et la migration inter-forks restent ouverts.

### Capella : rattrapage piloté par inventaire/export

L’export de bloc fournit slot et parent sous le verrou du checkpoint. Nouveau
scénario : second coordinateur et stockage indépendants depuis l’ancre initiale,
inventaire paginé puis seize messages admis normalement. Tête, état EVM et preuves
retenues convergent après les deux réorganisations de la source (test 5,76 s).
[Preuves](qualification/capella-inventory-catchup-20260927/README.md).
Le checkpoint finalisé reste à l’epoch zéro dans ce scénario ; ce n’est pas une
preuve de progression de finalité. La boucle de récupération est un banc de test
local, pas encore le synchroniseur réseau de production.

### Capella : ordonnancement structurel du rattrapage

Primitive C++ bornée : contrôle SSZ, recalcul des racines, refus des doublons et
parents absents, ordre parents avant enfants. Les métadonnées JSON ne déterminent
plus seules l’ordre. Validation BLS/EVM toujours effectuée à l’admission normale.
Quatre contrôles négatifs ajoutés ; rattrapage de seize messages toujours convergent
(test 5,80 s). [Preuves](qualification/capella-sync-order-20260927/README.md).
La recherche réseau des parents manquants et la reprise sur pages élaguées restent
à implémenter ; ce composant n’est pas le synchroniseur réseau complet.

### Capella : récupération bornée des parents

Collecteur itératif depuis les têtes demandées jusqu’aux racines locales connues,
dépendances tirées du SSZ, ancêtres dédupliqués et budgets cumulés. Deux extrémités
de branches suffisent à récupérer neuf blocs en neuf appels, puis l’admission
normale converge avec votes/slashing. Indisponibilité, limite et annulation testées
(5,73 s). [Preuves](qualification/capella-sync-dependencies-20260927/README.md).
Le callback de récupération n’est pas encore un transport libp2p ; sélection des
pairs, délais réseau et reprise après élagage restent à raccorder.

### Capella : récupération de session IPC privée

Le statut publie le profil Capella, les capacités réelles du worker et des messages,
la liaison d’exécution et la session. La machine commune de récupération IPC est
exercée via les RPC Capella : rotation, maintien de highWater=3 et vidage des anciens
jobs. Test runtime passé en 21,57 s ; admission réseau explicitement désactivée.
[Preuves](qualification/capella-ipc-recovery-20260927/README.md).
Le démon libp2p, le relais de catalogues et le démarrage réseau géré restent à
raccorder ; cette preuve locale ne clôture pas la qualification réseau Capella.

### Capella : cycle de vie du nœud géré

`nano run` sélectionne le runtime Capella et ses handlers privés, retire les clés
validateur de l’environnement enfant, arrête les workers et publie le reçu de
reprise. Test du vrai binaire avec enfant de contrôle : trois démarrages, reprise
avec témoins de chaîne/signature, refus de profils concurrents/RPC public et arrêt
sur panne enfant (1,45 s). CLI serve et RPC existantes également vertes.
[Preuves](qualification/capella-managed-node-20260927/README.md).
L’enfant du test n’est pas libp2p : adaptation du démon Capella et relais des
archives/catalogues encore ouverts. Aucun déploiement distribué Capella qualifié.

### Capella : raccordement IPC du véritable démon libp2p

Le démon reconnaît Capella, route les RPC de récupération et conserve l’identité
complète. Les chemins historiques d’import/annonce de blocs sont exclus de ce
profil ; le relais d’archives ancien ne lui est pas appliqué. Deux démarrages réels
`nano` + `nano-p2p` en loopback passent avec reprise persistante et session IPC prête.
[Preuves](qualification/capella-daemon-ipc-20260927/README.md).
Admission Capella toujours désactivée : aucun échange de blocs ni progression de
finalité distribuée prouvés. Relais de catalogues, recherche de parents via pairs
et critères d’activation restent ouverts.

### Capella : relais borné des catalogues

Relais asynchrone de catalogue stable, export unitaire, progression sur accusé
positif, clôture et traitement des réponses tardives. Budgets de pages/entrées et
délai global ; quinze scénarios adverses/succès passent (0,32 s). Intégration via
les vrais RPC et worker : réadmission des archives déjà connues sans changement
du checkpoint, catalogues fermés (21,87 s).
[Preuves](qualification/capella-catalog-relay-20260927/README.md).
L’ordre des racines ne garantit pas l’ordre des parents. Raccordement aux pairs,
collecte des dépendances et reprise après expiration restent requis avant activation.

### Capella : livraison des ancêtres avant les descendants

Préparation bornée depuis l’archive locale jusqu’à l’ancre ou un ancêtre confirmé
par le même pair/session ; livraison séquentielle et cache alimenté uniquement
sur accusé positif. Treize scénarios passent (0,31 s). Le test à deux réacteurs
demande les descendants d’abord : neuf exports/livraisons sans doublons, puis
votes/slashing, avec convergence tête/état/preuves (5,73 s).
[Preuves](qualification/capella-block-delivery-20260927/README.md).
Callbacks encore locaux ; cycle de vie des sessions pairs et raccordement au
démon restent ouverts. Finalité à l’epoch zéro dans cette preuve de rattrapage.

### Capella : rattrapage entre deux vrais pairs libp2p

Le démon relie les catalogues, la livraison des ancêtres et les accusés issus du
worker de consensus. Activation explicite et budgets configurés ; changements de
session/epoch pair invalident les caches et les réponses d’anciennes tentatives.
Deux vrais nœuds en loopback convergent après arrivée tardive, puis après remise
à l’état initial du récepteur conservant son identité P2P et perte volontaire
d’un accusé. Hash de bloc et racine d’état communs sont enregistrés.
[Preuves](qualification/capella-two-peer-network-20260927/README.md).
Validateurs concentrés sur l’émetteur : finalité multi-validateur distribuée,
partitions, WAN, charge durable et longues reprises après élagage restent ouverts.

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
