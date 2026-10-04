# Feuille de route EIP — état réel de Nano

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


### Régression locale et rejeux — 27 septembre 2026

Sur `af7f99e`, 185 des 188 tests sélectionnés passent ; trois échecs du banc
sont conservés. Après correction du drainage des traces et de la base de
résultats Assertoor, leurs trois rejeux passent, dont une stabilité de finalité
commune d’époque 7 à 10 après partition. Les 201 autres tests enregistrés sont
hors de cette campagne ; aucune qualification globale de production.
[Bilan, empreintes et limites](qualification/consensus-regression-20260927/README.md).

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

Dernière revue partielle : 26 septembre 2026 (intégration Bellatrix et reprise).
Source de vérité : code actuel, tests et rapports. Les anciennes notes chronologiques ne remplacent pas cette matrice.

- **Implémenté** : comportement présent dans le périmètre indiqué ; ne signifie
  pas que toute la blockchain est prête pour la production.
- **Partiel** : une couche ou le runner fonctionne, mais activation/intégration
  réseau ou consensus manque encore.
- **Non commencé** : pas d’implémentation spécifique intégrée identifiée dans
  cette revue. Une ligne de documentation ou une bibliothèque ne compte pas.
- **Terminé pour production** : réservé à une preuve de bout en bout sur le
  profil de lancement ; aucune simple liste EIP n’accorde ce statut.

Les campagnes officielles enregistrées couvrent 44 244 cas d’état et 52 870
cas de blocs dans leur périmètre sélectionné. Ces totaux ne constituent pas
une preuve individuelle pour chaque ligne. La traçabilité précise par EIP
reste à compléter. Le genesis réseau autorise actuellement Paris/Shanghai.

| EIP | État actuel | Périmètre réel | Code / preuves à consulter | Reste à faire / critère de clôture | Priorité |
| --- | --- | --- | --- | --- | --- |
| [3076](https://eips.ethereum.org/EIPS/eip-3076) | Partiel | Journal v5 et devoirs locaux Bellatrix à plusieurs clés ; redémarrage ordonné et reprise SIGKILL à témoins qualifiés sur 64 clés synthétiques | src/consensus_beacon_signer.cpp ; src/consensus_bellatrix_validator.cpp ; tests/test_bellatrix_transition.cpp ; docs/BEACON-SIGNING-JOURNAL.md | Gestion indépendante des témoins, migration des journaux existants, pannes de stockage et charge ; ne vaut pas qualification de tout le cycle validateur | Validateurs |
| [2](https://eips.ethereum.org/EIPS/eip-2) | Implémenté | Transactions et signatures | src/transaction.cpp ; tests/fixtures/ethereum-official.json | Conserver les régressions du profil activé et relier les cas exacts à chaque EIP | Socle |
| [155](https://eips.ethereum.org/EIPS/eip-155) | Implémenté | Transactions et signatures | src/transaction.cpp ; tests/fixtures/ethereum-official.json | Conserver les régressions du profil activé et relier les cas exacts à chaque EIP | Socle |
| [2718](https://eips.ethereum.org/EIPS/eip-2718) | Implémenté | Transactions et signatures | src/transaction.cpp ; tests/fixtures/ethereum-official.json | Conserver les régressions du profil activé et relier les cas exacts à chaque EIP | Socle |
| [2930](https://eips.ethereum.org/EIPS/eip-2930) | Implémenté | Transactions et signatures | src/transaction.cpp ; tests/fixtures/ethereum-official.json | Conserver les régressions du profil activé et relier les cas exacts à chaque EIP | Socle |
| [1559](https://eips.ethereum.org/EIPS/eip-1559) | Implémenté | Base fee et exécution | src/block.cpp ; src/engine.cpp ; src/evm.cpp | Recommandation RPC historique configurable et partagée avec le portefeuille ; qualifier coût à froid et saturation ; voir docs/RPC-FEE-POLICY.md | Socle |
| [3198](https://eips.ethereum.org/EIPS/eip-3198) | Implémenté | Base fee et exécution | src/block.cpp ; src/engine.cpp ; src/evm.cpp | Recommandation RPC historique configurable et partagée avec le portefeuille ; qualifier coût à froid et saturation ; voir docs/RPC-FEE-POLICY.md | Socle |
| [140](https://eips.ethereum.org/EIPS/eip-140) | Implémenté | Moteur EVM et règles d’état | src/evm.cpp ; src/engine.cpp ; tests/fixtures/ethereum-official.json | Maintenir la qualification et détailler les preuves par EIP ; pas une validation du réseau complet | Socle |
| [150](https://eips.ethereum.org/EIPS/eip-150) | Implémenté | Moteur EVM et règles d’état | src/evm.cpp ; src/engine.cpp ; tests/fixtures/ethereum-official.json | Maintenir la qualification et détailler les preuves par EIP ; pas une validation du réseau complet | Socle |
| [161](https://eips.ethereum.org/EIPS/eip-161) | Implémenté | Moteur EVM et règles d’état | src/evm.cpp ; src/engine.cpp ; tests/fixtures/ethereum-official.json | Maintenir la qualification et détailler les preuves par EIP ; pas une validation du réseau complet | Socle |
| [170](https://eips.ethereum.org/EIPS/eip-170) | Implémenté | Moteur EVM et règles d’état | src/evm.cpp ; src/engine.cpp ; tests/fixtures/ethereum-official.json | Maintenir la qualification et détailler les preuves par EIP ; pas une validation du réseau complet | Socle |
| [211](https://eips.ethereum.org/EIPS/eip-211) | Implémenté | Moteur EVM et règles d’état | src/evm.cpp ; src/engine.cpp ; tests/fixtures/ethereum-official.json | Maintenir la qualification et détailler les preuves par EIP ; pas une validation du réseau complet | Socle |
| [214](https://eips.ethereum.org/EIPS/eip-214) | Implémenté | Moteur EVM et règles d’état | src/evm.cpp ; src/engine.cpp ; tests/fixtures/ethereum-official.json | Maintenir la qualification et détailler les preuves par EIP ; pas une validation du réseau complet | Socle |
| [1014](https://eips.ethereum.org/EIPS/eip-1014) | Implémenté | Moteur EVM et règles d’état | src/evm.cpp ; src/engine.cpp ; tests/fixtures/ethereum-official.json | Maintenir la qualification et détailler les preuves par EIP ; pas une validation du réseau complet | Socle |
| [1344](https://eips.ethereum.org/EIPS/eip-1344) | Implémenté | Moteur EVM et règles d’état | src/evm.cpp ; src/engine.cpp ; tests/fixtures/ethereum-official.json | Maintenir la qualification et détailler les preuves par EIP ; pas une validation du réseau complet | Socle |
| [2200](https://eips.ethereum.org/EIPS/eip-2200) | Implémenté | Moteur EVM et règles d’état | src/evm.cpp ; src/engine.cpp ; tests/fixtures/ethereum-official.json | Maintenir la qualification et détailler les preuves par EIP ; pas une validation du réseau complet | Socle |
| [2929](https://eips.ethereum.org/EIPS/eip-2929) | Implémenté | Moteur EVM et règles d’état | src/evm.cpp ; src/engine.cpp ; tests/fixtures/ethereum-official.json | Maintenir la qualification et détailler les preuves par EIP ; pas une validation du réseau complet | Socle |
| [3529](https://eips.ethereum.org/EIPS/eip-3529) | Implémenté | Moteur EVM et règles d’état | src/evm.cpp ; src/engine.cpp ; tests/fixtures/ethereum-official.json | Maintenir la qualification et détailler les preuves par EIP ; pas une validation du réseau complet | Socle |
| [3855](https://eips.ethereum.org/EIPS/eip-3855) | Implémenté | Profil Shanghai sélectionnable au genesis | src/chain.cpp ; src/evm.cpp ; src/engine.cpp | Maintenir les tests d’activation et de limites | Socle |
| [3860](https://eips.ethereum.org/EIPS/eip-3860) | Implémenté | Profil Shanghai sélectionnable au genesis | src/chain.cpp ; src/evm.cpp ; src/engine.cpp | Maintenir les tests d’activation et de limites | Socle |
| [1153](https://eips.ethereum.org/EIPS/eip-1153) | Partiel pour la production | Moteur Cancun présent ; pas de genesis réseau Cancun | src/evm.cpp ; src/chain.cpp ; docs/EVM-CONFORMANCE.md | Activation réseau cohérente du fork, admission, régressions et reprise | Avant activation Cancun |
| [5656](https://eips.ethereum.org/EIPS/eip-5656) | Partiel pour la production | Moteur Cancun présent ; pas de genesis réseau Cancun | src/evm.cpp ; src/chain.cpp ; docs/EVM-CONFORMANCE.md | Activation réseau cohérente du fork, admission, régressions et reprise | Avant activation Cancun |
| [6780](https://eips.ethereum.org/EIPS/eip-6780) | Partiel pour la production | Moteur Cancun présent ; pas de genesis réseau Cancun | src/evm.cpp ; src/chain.cpp ; docs/EVM-CONFORMANCE.md | Activation réseau cohérente du fork, admission, régressions et reprise | Avant activation Cancun |
| [7702](https://eips.ethereum.org/EIPS/eip-7702) | Partiel pour la production | Autorisations et exécution Prague ; non activé sur réseau | src/authorization.cpp ; src/transaction.cpp ; src/engine.cpp | Mempool/P2P, admission type 4 et activation réseau Prague | Avant activation Prague |
| [4844](https://eips.ethereum.org/EIPS/eip-4844) | Partiel | Blobs/KZG dans le moteur et les runners | src/kzg.cpp ; src/engine.cpp ; tests/test_blobs.cpp | Admission réseau type 3, sidecars, disponibilité, rétention et récupération | Avant service blobs |
| [4895](https://eips.ethereum.org/EIPS/eip-4895) | Partiel | Retraits EVM/Capella, publication atomique et réseau local qualifiés dans des scénarios bornés ; file durable de changements BLS et sélection locale raccordée au constructeur de propositions | src/engine.cpp ; src/consensus_capella_withdrawals.cpp ; src/consensus_capella_bls_change.cpp ; src/consensus_capella_local_body.cpp ; tests/test_capella_transition.cpp | Reprise Linux/VPS, pruning, réorganisations profondes et charge/WAN des changements BLS ; migration réseau et qualification complète du profil avant activation publique | Consensus cible |
| [4788](https://eips.ethereum.org/EIPS/eip-4788) | Partiel | Appel système côté exécution | src/engine.cpp ; tests/test_beacon.cpp | Provisionnement versionné du contrat (code, nonce, adresse) au genesis si fork actif dès le départ, sinon à la transition ; fourniture authentifiée des racines par le nouveau consensus | Consensus cible |
| [6110](https://eips.ethereum.org/EIPS/eip-6110) | Partiel | Requêtes côté exécution ; cycle consensus non terminé | src/requests.cpp ; src/engine.cpp ; tests/test_requests.cpp | Registre BLS, dépôts, sorties, consolidations et application des requêtes dans le consensus | Consensus cible |
| [7685](https://eips.ethereum.org/EIPS/eip-7685) | Partiel | Requêtes côté exécution ; cycle consensus non terminé | src/requests.cpp ; src/engine.cpp ; tests/test_requests.cpp | Qualifier encodage, ordre par type, omission des requêtes vides et requests_hash ; rejeter les engagements altérés ; appliquer dépôts/sorties/consolidations au consensus et vérifier reprise/rejeu | Consensus cible |
| [7002](https://eips.ethereum.org/EIPS/eip-7002) | Partiel | Requêtes côté exécution ; cycle consensus non terminé | src/requests.cpp ; src/engine.cpp ; tests/test_requests.cpp | Registre BLS, dépôts, sorties, consolidations et application des requêtes dans le consensus | Consensus cible |
| [7251](https://eips.ethereum.org/EIPS/eip-7251) | Partiel | Requêtes côté exécution ; cycle consensus non terminé | src/requests.cpp ; src/engine.cpp ; tests/test_requests.cpp | Registre BLS, dépôts, sorties, consolidations et application des requêtes dans le consensus | Consensus cible |
| [7549](https://eips.ethereum.org/EIPS/eip-7549) | Non commencé côté consensus | Pas de traitement spécifique intégré identifié | Consensus actuel : src/consensus_round.cpp ; src/bft_reactor.cpp | Implémenter sur le socle epochs/attestations/domaines ; vecteurs officiels et tests réseau | Consensus cible |
| [7044](https://eips.ethereum.org/EIPS/eip-7044) | Partiel | Domaine Capella durable qualifié dans le traitement des sorties Deneb ; non activé réseau | include/nano/consensus_exit.hpp ; src/consensus_slashing.cpp ; tests/test_deneb_exit_official.cpp ; denebExitQualification | Raccorder aux blocs et au gossip ; activer un profil réseau cohérent et vérifier sa transition | Consensus cible |
| [7045](https://eips.ethereum.org/EIPS/eip-7045) | Non commencé côté consensus | Pas de traitement spécifique intégré identifié | Consensus actuel : src/consensus_round.cpp ; src/bft_reactor.cpp | Implémenter sur le socle epochs/attestations/domaines ; vecteurs officiels et tests réseau | Consensus cible |
| [7917](https://eips.ethereum.org/EIPS/eip-7917) | Non commencé côté consensus | Pas de traitement spécifique intégré identifié | Consensus actuel : src/consensus_round.cpp ; src/bft_reactor.cpp | Implémenter sur le socle epochs/attestations/domaines ; vecteurs officiels et tests réseau | Consensus cible |
| [8045](https://eips.ethereum.org/EIPS/eip-8045) | Non commencé | Review ; dépend de 7917, filtrage des slashés avant sélection | [Source épinglée et critères](qualification/validator-eip-sources-20260927/README.md) | Prévision stockée complète et stable après slashing ; activation expérimentale explicite | Extension demandée |
| [8068](https://eips.ethereum.org/EIPS/eip-8068) | Non commencé | Draft ; dépend de 7251, hystérésis et seuil temporaire | [Source épinglée et critères](qualification/validator-eip-sources-20260927/README.md) | Migration SSZ, retraits, arrondis et séquences de neutralité qualifiés dans un profil expérimental | Extension demandée |
| [8363](https://eips.ethereum.org/EIPS/eip-8363) | Non commencé | Draft ; destruction de récompenses selon le stake, dépendances fonctionnelles au consensus | [Source épinglée et critères](qualification/validator-eip-sources-20260927/README.md) | Déductions, inactivity leak, transition et comptabilité qualifiées ; paramètres Nano explicites | Extension demandée |
| [7805](https://eips.ethereum.org/EIPS/eip-7805) | Non commencé | Propositions demandées ; aucune implémentation spécifique identifiée | docs/VALIDATOR-EIP-ROADMAP.md | Épingler version et dépendances, implémenter et tester dans un profil expérimental distinct | Extension demandée |
| [7732](https://eips.ethereum.org/EIPS/eip-7732) | Non commencé | Propositions demandées ; aucune implémentation spécifique identifiée | docs/VALIDATOR-EIP-ROADMAP.md | Épingler version et dépendances, implémenter et tester dans un profil expérimental distinct | Extension demandée |
| [8141](https://eips.ethereum.org/EIPS/eip-8141) | Non commencé | Propositions demandées ; aucune implémentation spécifique identifiée | docs/VALIDATOR-EIP-ROADMAP.md | Épingler version et dépendances, implémenter et tester dans un profil expérimental distinct | Extension demandée |
| [7594](https://eips.ethereum.org/EIPS/eip-7594) | Non commencé | KZG existant ne constitue pas PeerDAS | src/kzg.cpp ; network/main.cpp | Custody, colonnes, preuves de cellules, échantillonnage et disponibilité réseau | Avant service PeerDAS |
| [7516](https://eips.ethereum.org/EIPS/eip-7516) | Partiel pour la production | BLOBBASEFEE implémenté dans le moteur ; lit la base fee des blobs du bloc | src/evm.cpp ; tests/test_blobs.cpp | Qualifier opcode 0x4a, coût 2 gas, valeur issue du bloc et rejet avant Cancun ; admission, RPC et rejeu multi-nœuds après activation | Avant activation Cancun |
| [2537](https://eips.ethereum.org/EIPS/eip-2537) | Partiel pour la production | Sept précompiles BLS12-381 implémentés | src/bls.cpp ; src/precompiles.cpp ; tests/test_bls.cpp | Qualifier les sept adresses, encodages, contrôles de sous-groupes applicables et gas/erreurs ; activation Prague et rejeu réseau ; distinct du BLS de consensus | Avant activation Prague |
| [2935](https://eips.ethereum.org/EIPS/eip-2935) | Partiel pour la production | Appel système d’historique implémenté dans le moteur | src/engine.cpp ; tests/test_requests.cpp | Provisionner code/nonce/adresse au genesis ou à la transition ; tester appel système avant transactions, fenêtre 8191, limites, reprise et rejeu réseau ; BLOCKHASH inchangé | Avant activation Prague |
| [7623](https://eips.ethereum.org/EIPS/eip-7623) | Partiel pour la production | Plancher de gas calldata dans le moteur | src/engine.cpp ; docs/EVM-CONFORMANCE.md | Activation Prague et admission/estimation cohérentes du plancher | Avant activation Prague |
| [7691](https://eips.ethereum.org/EIPS/eip-7691) | Partiel | Paramètres blobs Prague dans le moteur | src/engine.cpp | Intégration exécution/consensus et disponibilité réseau | Avant blobs Prague |
| [7840](https://eips.ethereum.org/EIPS/eip-7840) | À implémenter / qualifier | Configuration du calendrier des blobs non établie dans le genesis réseau | src/chain.cpp ; src/engine.cpp | Configuration versionnée et validation du calendrier ; statut Informational distinct du consensus | Configuration forks |
| [7642](https://eips.ethereum.org/EIPS/eip-7642) | Applicabilité à documenter | Nano utilise cpp-libp2p, pas le protocole Ethereum eth/69 | network/main.cpp | Nécessaire seulement si compatibilité filaire eth/69 revendiquée ; ne pas confondre avec EVM | Périmètre réseau |

## Hors numéros d’EIP : fondations nécessaires

| Composant | État | Reste à faire |
| --- | --- | --- |
| Admission au-delà de 1 000 | Implémentée et testée localement à 1 001 par transactions signées, RPC et réouverture | Lecture du staking et transport encore bornés ; ne pas promettre capacité réseau illimitée |
| Votes individuels à grande échelle | 100 000 et 1 million validés localement, certificats complets | Intégrer comités/agrégats et qualifier le réseau ; pas assimiler à autant de nœuds |
| Permutation des comités Ethereum | Primitives qualifiées localement jusqu’à 1 million ; affectations de slots intégrées aux devoirs Bellatrix et sélection groupée testée sur 64 clés | Qualifier le registre ouvert, les opérations et le réseau à grande échelle ; aucune équivalence avec 1 million de nœuds |
| BLS de consensus / agrégation | Partiel : signatures BLS des devoirs et blocs Bellatrix intégrées ; primitives et scénarios à clés synthétiques qualifiés | Cycle du registre/PoP, agrégats de réseau et changements de validateurs à qualifier de bout en bout |
| Domaines et racines SSZ de signature | Intégrés aux signatures Bellatrix locales avec identité/registre authentifiés ; tests de refus et de reprise existants | Qualifier les transitions de forks et l’activation publique ; les fixtures ne prouvent pas ces opérations réseau |
| Casper FFG / LMD-GHOST | Partiel : transitions et choix de branche Bellatrix implémentés ; finalité locale et scénario distribué ciblé enregistrés | Reprise après partition de 96 s réussie sur deux nœuds / un hôte ; partitions prolongées, autres répartitions du stake et exploitation distribuée à qualifier |
| Registre beacon / RANDAO / slashing | Partiel : RANDAO, journal anti-slashing, preuves de sanctions et sorties volontaires signées intégrées aux propositions/archives/P2P Bellatrix ; historiques de dépôts EVM par branche intégrés au Store/checkpoint Bellatrix ; admission et activation avec attestation qualifiées localement sous profil natif explicite ; clés locales à index automatique résolues par clé publique, attente et attestations après dépôt qualifiées ; entrée distante, branches concurrentes et retraits encore ouverts | Cycle ouvert entrées/sorties/sanctions, disponibilité des opérations et reprise après panne à terminer et qualifier |
| ERC-20/721/1155 | Contrats et scénarios ciblés existants | Étendre les scénarios ; pas de revendication « tout OpenZeppelin testé » |
| ERC-1271/165/2612 et EIP-712 | Audit de couverture nécessaire | Répertorier contrats et cas exacts ; éviter d’assimiler compilation et comportement |

## Mise à jour obligatoire après chaque travail

Pour changer un état : indiquer le changement, le profil de fork, les tests
positifs/négatifs, le rapport de campagne et les limites restantes. Une EIP
présente dans un runner reste partielle tant que le réseau requis ne l’admet
pas. Les dépendances des forks complets doivent être conservées ; cette matrice
est une feuille de route priorisée, pas une sélection arbitraire de règles.

Sources et priorités : [validateurs](VALIDATOR-EIP-ROADMAP.md) et
[EVM/production](EVM-PRODUCTION-EIP-ROADMAP.md). Les propositions Draft/Review
restent expérimentales. Les paramètres économiques Nano doivent être documentés
séparément des valeurs Ethereum. Les gates de durabilité, réseau, exploitation
et sécurité restent applicables en plus de cette feuille de route.

Preuves locales : `ethereumCommitteePrimitiveQualification` et
`validatorOpenAdmissionQualification` dans `tests/fixtures/ethereum-official.json`,
avec vérificateurs et contrôles négatifs. Elles ne valident pas le nouveau
consensus complet ni un réseau d’un million de nœuds.

## Clarifications RPC et sérialisation — revue du 24 septembre 2026

- `maxFeePerGas` et `maxPriorityFeePerGas` sont déjà traités dans les
  simulations (`src/rpc.cpp`, `tests/test_rpc.cpp`). Le pourboire recommandé
  fixe est un écart distinct, pas une absence de ces champs.
- `eth_createAccessList` : aucune implémentation identifiée ; à ajouter et
  comparer à un client de référence, notamment pour les accès froids et les
  listes fournies en entrée.
- Les transactions de types 3 et 4 gardent leurs enveloppes typées **RLP**
  selon EIP-4844/EIP-7702. **SSZ** concerne les objets de consensus et les
  sidecars correspondants : ne pas convertir leurs transactions en SSZ.
- EIP-2935 ne remplace pas la sémantique de l’opcode BLOCKHASH.
- EIP-7685 figurait déjà comme partielle ; pas de nouvelle ligne en doublon.
- Le profil complet Prague se contrôle aussi contre la meta-EIP 7600 ; la
  liste de quelques briques n’est pas une preuve de conformité complète.

Preuve `consensusBlsVerifyQualification` : primitive individuelle BLS, distincte
des précompiles EIP-2537. Aucun certificat agrégé ni poids de vote autorisé
par ce seul résultat.

Les critères de clôture ci-dessus sont des travaux à vérifier, pas des tests
déclarés réussis par cette revue documentaire. Références normatives :
[EIP-7516](https://eips.ethereum.org/EIPS/eip-7516),
[EIP-2537](https://eips.ethereum.org/EIPS/eip-2537),
[EIP-2935](https://eips.ethereum.org/EIPS/eip-2935),
[EIP-7685](https://eips.ethereum.org/EIPS/eip-7685) et
[profil Pectra](https://eips.ethereum.org/EIPS/eip-7600).

Preuve `consensusSigningQualification` : racines SSZ et construction des domaines,
avec corpus officiel épinglé et contrôle négatif du vérificateur. Cette primitive
ne constitue pas encore une protection des votes du réseau par ces domaines.

Campagne `hundredThousandVotesQualification` : 100 000 votes individuels,
certificat complet, doublons et quatre entrées invalides ; 148,42 s au total,
332 742 656 octets de RSS maximal et 55 800 000 octets de messages cumulés.
Mesure locale unique, sans promesse de débit ni qualification multi-nœuds.

Campagne `millionVotesQualification` : 1 000 000 de votes individuels, certificat
complet, doublons, équivoque et changement de registre vérifiés. Total :
1 482,79 s ; RSS maximal : 3 170 107 392 octets ; messages : 558 000 000 octets.
Réception avec contrôle du quorum : 267,78 s ; doublons : 264,30 s.
Ces coûts ne conviennent pas à une participation individuelle de tout ce
registre à chaque bloc rapide : comités et agrégats restent à intégrer.
Aucun million de nœuds ni million d’inscriptions au staking n’a été testé.

Preuve `consensusBlsAggregateQualification` : FastAggregateVerify, 12 vecteurs
officiels et 33 mutations rejetées ; exige des preuves de possession vérifiées
par le registre. Ne valide pas les poids, les quorums ou les signataires autorisés.

Campagne `realNetworkScaleQualification` : 4 puis 16 vrais nœuds locaux
(cœur C++, RocksDB et relais libp2p distincts), 4 blocs certifiés et 1 transaction,
avec un signataire de fixture ; accord exact des hash, racines et reçus.
Le scénario séparé de 4 validateurs BFT actifs Shanghai passe également.
10 000 nœuds non exécutés : ce Mac dispose de 24 Gio et limite globale de
6 000 processus, contre au moins 20 000 nécessaires avec cœur et relais.
L’utilisateur a retenu une petite échelle réelle sur ce Mac ; aucune preuve
de réseau 10 000 nœuds, multi-hôte ou d’endurance n’en découle.

Preuve `consensusDepositQualification` : racines SSZ DepositMessage (5 vecteurs
Ethereum), 3 signatures synthétiques et 24 mutations rejetées. La signature
prouve la possession de la clé ; elle ne prouve ni le financement ni l’ordre
d’un dépôt. Admission au registre et cycle EIP-6110 restent partiels.

Preuve `stakingQueryRulesQualification` : budgets des lectures de staking
configurables dans le genesis signé, engagés dans son hash ; réouverture
avec règles différentes refusée. Trois suites passent, puis un cas renforcé
vérifie l’épuisement du budget global sans publication partielle du registre.
Les valeurs historiques sont conservées si la configuration est absente.
Ce changement ne supprime pas les coûts de lecture du registre entier ni
les copies d’état par appel : pagination/incréments restent ouverts.

Preuve `stakingPaginationQualification` : interface paginée du contrat et
synchronisation intégrée au moteur sous `stakingQueryRules.version=2`.
Équivalence exacte avec l’interface historique pour 5 tailles de page ;
6 pages invalides/doublons et 4 configurations invalides rejetées.
1 001 inscriptions signées, certificats BFT via RPC et réouverture auditée
passent sur Paris et Shanghai. Anciennes interfaces conservées par défaut.
La pagination est implémentée ; le parcours incrémental du registre, les
copies d’état et la qualification de grands réseaux restent ouverts.

Qualification locale complémentaire (24 septembre 2026) : les appels EVM
statiques lisent désormais l'état sans le copier ni créer de snapshots de comptes.
`readonlyEvmQualification` conserve 18 artefacts vérifiés pour 9 tests Release,
dont 20 cas statiques, annulation, rollback du parent et régressions staking/RPC.
Cela réduit le coût des getters paginés mais ne clôture ni EIP-214 sur le corpus
officiel complet après modification, ni le passage réseau à 100 000 validateurs.

Rafraîchissement EVM : `readonlyEvmCorpusQualification` vérifie 5 387/5 387 cas
sur les 317 fichiers de fixtures d'état actuellement versionnés, dont 505 rejets.
Identités et empreintes contrôlées ; zéro mismatch/unsupported/error. Ce périmètre
ne remplace pas les corpus historiques plus larges ni les scénarios de blocs et
réseau. Aucun changement de statut de production n'en découle.

Consensus : `ConsensusCommittee` relie désormais PoP, appartenance unique,
agrégats BLS et poids sans débordement. Trois vérifications positives et vingt
rejets adversariaux sont archivés dans `committeeParticipationQualification`.
L'état autorisant ces membres/poids/domaines, le traitement des époques et
l'intégration réseau restent requis ; aucun statut d'EIP complet n'est ajouté.

Attestations : la racine SSZ typée (slot, index, bloc, checkpoints source/cible)
est vérifiée contre cinq vecteurs officiels et reliée au test d'agrégat du comité.
`consensusAttestationQualification` conserve les preuves ; fenêtre d'inclusion,
règles de fork et finalité restent ouvertes, notamment pour 7045/7549.

Slashing : le prédicat de double vote / vote englobant d'attestations est testé
sur 24 cas locaux (`attestationConflictQualification`). Il ne sanctionne pas
encore les validateurs : validation complète des preuves et mutation persistante
du registre restent ouvertes.

Slashing authentifié : `verify_attester_slashing` vérifie maintenant les deux
signatures et l'intersection des signataires avec cohérence des clés historiques
(`authenticatedSlashingQualification` : 3 preuves positives, 10 rejets).
L'application des pénalités et son intégration persistante restent ouvertes.


Transition de sanctions : `slashingTransitionQualification` archive 26 scénarios
locaux de comptabilité et d'éligibilité Phase0, plus les régressions des preuves
BLS et comités. Les sorties différées, récompenses et rejets atomiques sont testés
en mémoire. Ce résultat ne clôture aucune EIP : persistance par bloc, cycle
d'époques et raccordement réseau restent ouverts. Voir la feuille de route
validateurs pour le profil exact et les limites.


Reprise du registre : `slashingRecoveryQualification` ajoute un snapshot local
strict et une réouverture RocksDB après sortie abrupte du processus écrivain.
11 contrôles et 19 rejets de snapshots passent ; le rejeu ne double pas la
sanction. Cette qualification du stockage n'active pas le registre dans le BFT
et ne prouve pas l'engagement du consensus dans les blocs ni la reprise après
coupure électrique. Les EIP gardent leur statut actuel.


Slashing par époque : `slashingEpochQualification` ajoute pénalité corrélée,
rotation du bucket et contrôle de séquence, avec 11 contrôles et 8 rejets.
Les quatre suites passent. Le format snapshot v2 inclut les paramètres ajoutés
et rejette v1. Le cycle complet des époques et le consensus réseau restent
ouverts ; aucun statut d'EIP n'est promu sur cette sous-transition.


Preuve officielle de slashing : `officialSlashingQualification` compare sans
écart 320 soldes (90 modifiés) dans les cinq fixtures Phase0 minimal du composant
`process_slashings`, épinglées à une révision Ethereum. Le calcul pur est testé
sur leurs états isolés ; la transition complète conserve ses contrôles de
cohérence. Cinq suites passent. Ce périmètre ne clôture ni le cycle complet des
époques ni une EIP de consensus ; l'intégration réseau reste ouverte.


RANDAO : `randaoQualification` vérifie les révélations BLS liées à l'époque,
au fork et au genesis, les rejets de slots rejoués, la rotation et les graines
retardées. 13 contrôles et 19 rejets locaux passent avec les régressions BLS/SSZ.
L'éligibilité du proposant, la persistance et le raccordement au consensus actif
restent requis ; aucune activation réseau n'est revendiquée.


RANDAO officiel : `officialRandaoQualification` valide 3 révélations BLS et
192 mixes à partir de trois scénarios de blocs Phase0 minimal épinglés, avec
15 rejets locaux. Les indices des proposants viennent des fixtures : sélection,
validation complète des blocs et intégration réseau ne sont pas qualifiées.
Trois suites passent et les contrôles négatifs sont archivés.


Proposants : `proposerQualification` recalcule les 3 indices officiels du corpus
RANDAO et teste 98 comparaisons pondérées, 10 rejets et l'annulation. Les accès
scalaires au shuffle concordent avec 240 permutations officielles. Les mesures
100 000/1 000 000 concernent des registres locaux à poids maximaux égaux, sans
réseau. Profil Phase0 seulement ; Electra/EIP-8045 et activation restent ouverts.


Jonction proposant/RANDAO : `selectedRandaoQualification` relie la sélection,
l'éligibilité et la clé du registre à la révélation BLS. Trois contrôles positifs,
12 rejets locaux et 6 rejets ajoutés aux 3 projections officielles passent dans
quatre suites. Le registre authentifié, le bloc complet, le commit durable et
le raccordement au réacteur restent ouverts ; aucun statut d'EIP n'est promu.

Signature des blocs : `beaconHeaderQualification` vérifie cinq racines SSZ
BeaconBlockHeader et trois signatures de blocs officiels, avec 25 mutations de
racines et 33 rejets de signatures/contextes. Cinq suites passent. La racine du
corps est fournie par la fixture officielle, sans calcul complet du corps ni
transition d'état ; intégration au réacteur et commit atomique restent ouverts.
Les statuts réseau/EIP ne changent pas.

Corps des blocs : `beaconBodyQualification` calcule le corps SSZ Phase0 complet
sur cinq fixtures officielles couvrant toutes ses opérations. Six suites
passent, ainsi qu'une campagne ciblée ASan/UBSan. Les trois signatures de blocs
utilisent maintenant ce corps recalculé, levant la dépendance à une racine de
fixture de la campagne précédente. La validité des opérations, la transition
complète, les schémas des forks suivants et l'intégration réseau restent ouverts.
Aucun statut d'EIP ni de production n'est promu par ce seul calcul SSZ.

Authentification commune : `blockAuthenticationQualification` relie corps SSZ,
signature, parent/slot attendus, proposant et RANDAO dans une préparation sans
mutation canonique. Trois blocs officiels, 54 rejets, 404 troncatures, 21
annulations, quatre contrôles locaux et sept erreurs de configuration sont
qualifiés dans six suites. Le candidat n'est pas un post-état validé ; opérations,
transition complète, commit durable et activation restent ouverts.

Sanction des proposants : `proposerSlashingQualification` couvre les quinze
scénarios officiels Phase0 (3 succès, 12 rejets, dont un registre amont incohérent).
La transition économique partagée avec les attestations reproduit les registres
et 192 soldes attendus. Rejeu, snapshot et débordement sont contrôlés ; sept
suites passent. Le traitement dans les blocs et l'activation réseau restent
ouverts ; ce profil Phase0 ne promeut aucun EIP Electra ni statut production.

Traitement des sanctions dans le bloc : `blockOperationsQualification` relie
l'authentification signée au RANDAO et aux proposer slashings ordonnés. Trois
blocs officiels reproduisent les projections attendues, deux blocs authentifiés
à preuves répétées sont rejetés sans mutation. Six suites passent, avec échec
tardif et annulation. Les autres opérations restent explicitement non traitées ;
état complet, commit et activation réseau ne sont pas qualifiés.

Cycle de sortie : `voluntaryExitQualification` couvre les dix cas officiels
Phase0, avec 320 enregistrements conformes, quatre rejeux et des rejets de
signature/domaine/débordement sans mutation. La file de sortie est mutualisée
avec les sanctions ; sept suites passent. Le raccordement aux blocs/réseau,
les paiements et les règles modernes EIP-7044/7002 restent ouverts.

EIP-7044 : `denebExitQualification` qualifie le domaine Capella figé et les
transitions de sortie sur seize cas officiels Deneb, avec 448 enregistrements
conformes et deux différences explicites par rapport à Phase0. Six suites
passent. Statut désormais partiel : bloc/gossip, activation et profil réseau
complet restent requis ; aucune activation Deneb n'est annoncée.

Admission des validateurs : `depositTransitionQualification` valide les dix-sept
scénarios officiels de dépôts Phase0, avec preuve Merkle ordonnée, PoP des nouvelles
clés, recharges et 966 enregistrements conformes. Les credentials/éligibilité
sont conservés dans le registre v3 ; le snapshot de dépôt conserve root/count/cursor.
Onze suites passent. La racine financée doit encore être fournie par le consensus,
l'activation et le traitement complet des blocs restent ouverts. Ce travail ne
vaut pas implémentation d'EIP-6110 ni ouverture effective du registre réseau.

Intégration du registre dans les blocs : `registryBlockOperationsQualification`
compose sanctions de proposants, dépôts et sorties Phase0 sur état privé. Huit
suites passent ; six scénarios officiels sont exécutés (trois valides/trois invalides),
deux séquences traversant une époque restent différées et non comptées comme succès.
Attestations non prises en charge signalées distinctement des blocs invalides.
Ce progrès ne clôt ni EIP-6110 ni EIP-7044 réseau : Eth1, époques, état complet et
commit/réacteur restent à intégrer. Les statuts EIP de la matrice restent inchangés.

Cycle de vie Phase0 : `registryEpochQualification` ajoute la programmation des
activations après finalité et des éjections, avec file partagée et churn paramétré.
Les quinze cas officiels de registry_updates sont couverts (14 conformes, 1 rejet
attendu), ainsi que deux registres synthétiques locaux de 100 000/1 000 000 entrées.
Neuf suites passent. Cette sous-transition ne clôt pas le traitement d'époque,
la finalité ou les EIP de staking modernes ; aucun statut réseau n'est promu.

Finalité de composant : `phase0FfgQualification` calcule justification/finalisation
Phase0 depuis comités, bitlists et soldes de l'état authentifié. Dix scénarios
officiels et sept suites conformes ; seuil exact de deux tiers testé au-delà de
uint64, doublons/sanctionnés/cibles incorrectes contrôlés. L'admission des
attestations, l'ordre d'époque complet, le choix de branche et le commit/réseau
restent ouverts ; aucun EIP réseau ni objectif de production n'est déclaré clos.

Admission Phase0 : `attestationInclusionQualification` ajoute la validation des
attestations (comité, fenêtre, source, BLS/fork/genesis), les métadonnées d'inclusion
et leur composition atomique dans les blocs signés avant dépôts/sorties. Les 41
cas officiels donnent 17 états conformes/24 rejets attendus ; sept suites passent.
Deux séquences officielles multi-époques supplémentaires sont différées. Les
sanctions d'attestateurs, opérations d'époque complètes et commit/réacteur réseau
restent ouverts ; EIP-7045/7549 et forks modernes ne sont pas promus par ce travail.

Sanctions d'attestateurs : `attesterSlashingQualification` authentifie les preuves
indexées et les intègre avant attestations/dépôts/sorties. Trente cas officiels
isolés, quatre blocs officiels et onze suites sont qualifiés ; un état synthétique
à chronologie incohérente concorde sur le composant mais reste rejeté par le chemin
contrôlé des blocs. Les cinq types d'opérations Phase0 sont composables ; Eth1,
époques, state_root, commit et réseau restent ouverts. Aucun EIP moderne n'est clos.

Soldes effectifs du socle Phase0 : `effectiveBalanceQualification` qualifie le
composant d'hystérésis sur l'unique scénario officiel (64 entrées, 8 changements)
et les frontières/annulations locales. Détail dans `VALIDATOR-EIP-ROADMAP.md`.
Le traitement complet des époques et son intégration réseau restent ouverts ;
cette preuve ne clôt ni EIP-7251 ni la production.

Fin d'époque du socle Phase0 : `epochMaintenanceQualification` couvre les resets
Eth1/sanctions/RANDAO, les HistoricalBatch SSZ et la rotation des attestations,
avec six scénarios officiels. Voir `VALIDATOR-EIP-ROADMAP.md` pour les frontières
et critères restants. Ces composants ne clôturent pas le traitement complet des
époques, EIP-6110 ou l'intégration réseau.

Récompenses Phase0 : `rewardsQualification` qualifie les 19 scénarios officiels
sur le calcul isolé après FFG (1 216 entrées comparées), y compris inactivité,
doublons et genesis. Les contrôles d'annulation et de débordement sont détaillés
dans `VALIDATOR-EIP-ROADMAP.md`. Ce socle ne clôt pas les récompenses modernes,
EIP-8068/8363 ou l'intégration du consensus au réseau.

Époque Phase0 composée : `orderedEpochQualification` trace l'ordre des dix étapes,
la cohérence des paramètres et le marqueur de retraitement. Six projections de
fixtures de slots concordent ; un scénario d'historiques reste différé. Cette
preuve ne couvre ni `process_slot`, ni la racine BeaconState, ni la persistance
ou le consensus réseau. Voir `VALIDATOR-EIP-ROADMAP.md` avant toute conclusion de
conformité complète ou d'activation d'un EIP.

Racine SSZ Phase0 : `beaconStateRootQualification` couvre les 123 vecteurs
BeaconState officiels du preset minimal et les 21 champs. Le hash complet
n'est pas encore raccordé au traitement des slots ou à la validation des blocs ;
les invariants d'admission du consensus restent distincts du schéma SSZ. Aucun
EIP/fork moderne ni état réseau n'est déclaré complet par cette preuve.

Slots Phase0 : `fullSlotsQualification` compare désormais les états SSZ complets
sur les sept scénarios officiels (100 slots), incluant `historical_accumulator`.
La nouvelle preuve dépasse les projections de `orderedEpochQualification` sans
les réécrire. Les blocs signés complets, la persistance/reprise et le consensus
réseau restent ouverts ; aucun fork post-Phase0 n'est activé par cette étape.

Blocs signés Phase0 : `fullBlocksQualification` couvre les 45 scénarios officiels
`sanity/blocks` minimal (29 états complets conformes, 16 rejets), en composant
slots/époques, votes Eth1, opérations et vérification du state_root. Les anciennes
séquences différées sont couvertes ici sans réécriture des campagnes historiques.
Le stockage durable, l'admission des états, le fork choice et l'intégration au
réseau restent ouverts. Il ne s'agit pas d'une activation des forks modernes.

Persistance Phase0 : `slotSnapshotQualification` trace une enveloppe SSZ/profil/
marqueur canonique et une reprise après `_exit` post-commit RocksDB. La poursuite
de la séquence reproduit l'état officiel, avec 25 enveloppes invalides rejetées.
Cette preuve locale ne clôt pas les fautes pendant écriture, la liaison EVM /
consensus, les snapshots réseau ou le commit certifié du nouveau consensus.

Fautes de persistance : `snapshotSyncQualification` couvre une erreur fsync et
trois dispositions de suffixe WAL (conservé/perdu/tronqué), avec reprise cohérente
SSZ/marqueur et continuation officielle. Portée locale macOS, stockage d'exécution
synthétique de support ; pas de preuve de coupure physique, de Linux ou de liaison
EVM/Beacon active. Les critères réseau/durabilité restants demeurent ouverts.

Choix de branche Phase0 : calcul de tête sur snapshot authentifié ajouté,
avec tests locaux de pondération/bonus/viabilité et cent arbres générés. Voir
`docs/VALIDATOR-EIP-ROADMAP.md`, section du 25 septembre sur le choix de tête.
Les handlers d'événements, leur authentification, la persistance du Store et
l'intégration réseau demeurent ouverts ; aucun EIP/fork réseau n'est activé
par cette primitive.

Admission fork choice : contrôle temporel/cible, comité dérivé, BLS et derniers
messages implémentés sur contexte cible authentifié fourni par l'appelant.
Tests locaux : 11 rejets, 70 annulations et effet sur la tête ; aucune activation
réseau ni qualification du Store complet. La dérivation SSZ du contexte, les
autres événements et la persistance restent à réaliser.

Équivoques du fork choice : double authentification BLS et intersection des
signataires implémentées ; les tests locaux vérifient l'exclusion du poids et
le changement de tête. La persistance de cet ensemble et le traitement réseau
restent ouverts. Aucune pénalité financière hors transition de bloc n'est ajoutée.

Contextes du fork choice : extraction depuis le post-state SSZ lié à l'en-tête
authentifié et avancement des slots sautés ajoutés. Qualification locale sur
29 séquences de blocs officielles (24 contextes avancés), neuf rejets et sept
annulations. Ce n'est ni une admission d'ancre non fiable ni une activation réseau.

Horloge du fork choice : expiration du bonus et promotion des checkpoints/du
registre ajoutées, avec mille comparaisons locales de rattrapage. Source d'horloge,
événements blocs, calcul des checkpoints non réalisés et persistance restent à
raccorder ; aucune activation réseau supplémentaire.

Store de fork choice en mémoire : admission de blocs reliée à la transition SSZ,
au bonus et aux checkpoints FFG. Sous-ensemble genesis : 15 séquences officielles
valides et 12 invalides, 16 blocs acceptés ; 18 fixtures intermédiaires explicitement
exclues de ce test de Store. Replay officiel fork_choice, durabilité, import
opérationnel d'ancre et réseau encore ouverts.

Adaptateurs Store : votes gossip, preuves et horloge utilisent désormais les
contextes SSZ des blocs conservés. Cinq suites passent. Écart de qualification
confirmé : les fixtures fork_choice du commit épinglé commencent avec Altair ;
leur replay fidèle nécessite les transitions/structures Altair, encore absentes
de ce pipeline Phase0. Ne pas assimiler les tests locaux au corpus officiel.

Prérequis Altair : schéma BeaconState à 24 champs et hash_tree_root implémentés,
123 vecteurs officiels minimaux concordants. Le support reste structurel :
transitions Altair et replay fork_choice non implémentés dans ce pipeline.
Aucun fork réseau activé par cet ajout.

Altair : opération isolée de mise à jour des scores d'inactivité implémentée,
21 post-states officiels concordants. Les fixtures synthétiques ne qualifient
pas l'admission d'état ; composition d'epoch et exécution réseau encore ouvertes.

Altair : récompenses/pénalités isolées ajoutées, 15 post-states officiels
concordants et saturation par catégorie vérifiée. Ne clôt ni l'époque complète,
ni l'admission des états, ni la qualification réseau.

Altair : justification/finalisation isolée ajoutée, dix post-states officiels
concordants et seuils pondérés testés. Les indicateurs de participation restent
soumis à authentification en amont ; ce résultat ne prouve pas la finalité réseau.

Altair : sélection/rotation des sync committees ajoutée, cinq post-states officiels
concordants. Répétitions de membres conservées ; budget local d'échantillonnage
explicite. Le traitement SyncAggregate des blocs et la composition d'epoch restent
ouverts, sans activation réseau.

Altair : rotation de participation isolée implémentée, douze post-states officiels
concordants. Opération non idempotente : application unique et persistance doivent
être assurées par la composition d'epoch, encore ouverte.

Altair : époque complète ordonnée et process_slots ajoutés, sept séquences
minimales officielles (100 slots) concordantes. Marqueur anti-rejeu et échec tardif
sans publication partielle vérifiés. Transition de blocs Altair, replay fork_choice,
durabilité et réseau encore ouverts.

Altair : opération SyncAggregate implémentée et qualifiée sur les 24 cas officiels
minimal (16 états exacts, huit signatures rejetées), avec tests de corruption,
autre genesis, débordement et annulation. Cette étape complète la vérification
isolée des sync aggregates, pas la transition complète des blocs ni le consensus
réseau. L'admission doit authentifier le comité, sa clé agrégée et les PoPs.
Voir la campagne `altair-sync-aggregate-20260925` et la feuille de route validateurs.

Altair : attestations et récompenses de proposant implémentées, 41 opérations
officielles minimal qualifiées (17 états exacts, 24 rejets). Les répétitions ne
paient pas deux fois les mêmes indicateurs de participation. Contrôles BLS,
fenêtre d'inclusion et comités sont dérivés de l'état admis. La transition complète
des blocs reste ouverte ; aucune activation réseau. Campagne
`altair-attestation-20260925`, détails dans la feuille de route validateurs.

Altair : dépôts hérités de Phase0 adaptés à l'état Altair, avec initialisation
à zéro des nouvelles listes de participation/inactivité. Les 19 opérations
minimal officielles passent (17 états exacts, deux rejets), ainsi que rejeu,
preuves modifiées, débordements et annulations. Cela ne constitue pas EIP-6110
ni l'authentification réseau de la source des dépôts. Campagne
`altair-deposit-20260925`, détails dans la feuille de route validateurs.

Altair : sanctions de proposants et d'attestateurs adaptées à l'état SSZ complet,
avec paramètres économiques Altair. Sur 45 fixtures officielles : 11 transitions
strictes exactes, 33 rejets et un calcul synthétique exact dont l'état incohérent
reste rejeté par l'admission stricte. Rejeu, mutations, débordements et annulations
sont testés. Aucun passage du consensus actif à Altair ; composition complète
et fork_choice restent ouverts. Campagne `altair-slashing-20260925`.

Altair : transition complète des blocs signés composée avec hash du corps SSZ,
slots/époques, signatures/RANDAO, votes Eth1 et toutes les opérations jusqu'au
SyncAggregate et à la racine finale. Les 52 séquences officielles minimal passent
(37 post-states exacts, 15 rejets ; 105 blocs acceptés avec racine vérifiée).
Cette composition ne vaut ni fork_choice Altair, ni admission/migration de fork,
ni persistance/raccordement au réseau actif. Campagne
`altair-full-blocks-20260925`, détails dans la feuille de route validateurs.

Fork choice Altair : Store événementiel en mémoire ajouté, dix scénarios officiels
get_head qualifiés (109 blocs, 104 ticks, 19 votes, une preuve séparée, 1081 champs
vérifiés). L'écart initial venait des attestations embarquées omises par le replay ;
la réception les traite après validation du bloc. Les attentes restent inchangées,
et l'oracle Ethereum officiel confirme zéro écart. Les profils de proposer boost
restent explicites. Autres groupes fork_choice, persistance et réseau encore
ouverts ; campagne `altair-fork-head-fixed-20260925` et feuille de route validateurs.

Fork choice Altair : extension aux 23 scénarios officiels on_block et huit reorg,
sans changement des attentes ni du moteur. Avec get_head, les trois suites passent
(41 scénarios, 10040 champs ; quatre blocs invalides attendus rejetés).
Campagne `altair-fork-block-reorg-20260925`. Le Store reste en mémoire ;
withholding/ex_ante/get_proposer_head, persistance/reprise et raccordement EVM/P2P
restent ouverts. Aucun EIP supplémentaire n'est déclaré intégralement activé.

Fork choice Altair : les cinq scénarios minimal withholding/ex_ante passent
(77 événements blocs, 870 champs), avec confirmation indépendante par l'oracle
Ethereum officiel, BLS actif, zéro écart. Campagne
`altair-fork-withholding-ex-ante-20260925`. Les deux scénarios ex_ante mainnet
ne sont pas couverts ; get_proposer_head, stockage durable et intégration réseau
restent ouverts. Aucun changement du profil réseau actif ou du statut des EIP.

Altair : optimisation optionnelle get_proposer_head implémentée avec règles
explicites et poids authentifiés. Deux scénarios officiels (312 champs) confirmés
par l'oracle Ethereum ; dix gardes locales. Les six groupes fork_choice minimal
passent ensemble (48 scénarios, 11222 champs). Campagne
`altair-fork-proposer-20260925`. Cela ne clôt pas les tests mainnet, l'admission
de snapshots, la persistance/reprise ni le raccordement EVM/P2P ; aucun nouveau
profil de consensus n'est activé.

Altair : enveloppe locale versionnée de sauvegarde d'état SSZ et marqueur d'époque,
liée au profil complet et à une racine authentifiée extérieure. Reprise après
_exit suivant un commit RocksDB, continuation identique au post-state officiel,
12 contrôles et 105 rejets de snapshots ; régressions Phase0/blocs Altair vertes.
Campagne `altair-snapshot-recovery-20260925`. Ne sauvegarde pas encore le Store
fork_choice complet ; coupure électrique, liaison EVM/consensus et réseau restent
non qualifiés. Aucun statut d'EIP ou profil réseau activé n'est promu.

Altair : sauvegarde canonique du Store complet et reconstruction du registre
justifié, avec digest externe et budgets explicites. Les 18 scénarios get_head/reorg
passent après 35 commits RocksDB, arrêts _exit et réouvertures ; tête, poids et
continuation conservés. Un écart de registre lors d'un checkpoint avançant d'époque
sans changer de racine a été corrigé, échec conservé. Huit suites vertes ; campagne
`altair-store-snapshot-20260925`. Interruptions pendant commit, anti-rollback,
ancrage réseau, élagage et intégration EVM/P2P restent ouverts. Pas d'activation.

Persistance du Store : commit ChainStore dédié, séquence + digest, remplacement
atomique synchronisé et rejet des écrivains périmés. Erreur d'écriture : handle
bloqué jusqu'à réouverture. Tests de concurrence, six SIGKILL, restauration d'une
ancienne base détectée avec engagement extérieur, EIO de synchronisation macOS
avec suffixe WAL conservé/perdu et poursuite après réouverture. Les 35 reprises
Altair utilisent désormais cette API sans blocs d'exécution artificiels.
Campagne `fork-checkpoint-commit-20260925` ; intégration au réacteur/EVM, Linux,
ancrage opérationnel et perte électrique restent non qualifiés.

Altair : coordinateur événementiel durable ajouté. Tick/bloc/vote/slashing sont
préparés puis committés avant publication ; conflit/erreur disque bloque l'instance,
annulation/refus laisse l'état inchangé. Get_head/reorg : 787 commits (18 init),
17 reprises, 3721 champs officiels conformes ; gardes d'annulation/publication et
fsync EIO macOS vérifiées. Campagne `altair-durable-reactor-20260925`.
Snapshots complets par événement : débit non qualifié. Aucun raccordement P2P,
activation réseau ou lien atomique EVM n'est déduit de cette intégration locale.

Altair : décodeurs SSZ bornés de votes et preuves raccordés aux entrées du
coordinateur durable. 71 encodages officiels pré-Electra, 768 entrées mal formées
ou hors limites rejetées ; cinq suites vertes, reprises événementielles conservées.
Campagne `consensus-message-ssz-20260925`. La validation de signature/consensus
reste distincte du codec. Transport P2P, cadrage/décompression/quotas et schéma
Electra EIP-7549 restent à intégrer ; aucune activation réseau.

Transport Altair : adaptateur d'enveloppe Nano JSON/hex préparé, identité/profil
et tailles vérifiés, clés dupliquées rejetées, budget partagé InboundBudget et
framing JsonFrame réutilisés. Test local : deux blocs officiels committés avant
acceptation, 14 rejets de messages, 24 refus par quota ; trois suites vertes.
Campagne `altair-wire-admission-20260925`. Pas encore de flux libp2p réel ni de
handler enregistré ; négociation/runtime/files et essais distribués restent ouverts.

Altair : adaptateur asynchrone de streams ajouté (lecture fragmentée bornée,
PeerId du transport, réservation maintenue jusqu'à fin réelle du worker, annulation
et réponse unique). Dix cas contrôlés ; neuf suites réseau et deux régressions
libp2p/BFT locales vertes. Campagne `altair-async-stream-20260925`.
Ces connexions réelles concernent le transport existant : Altair attend encore
handler/négociation, runtime et file worker bornée. Aucun nouveau profil actif.

Complément transport Altair (25 septembre 2026) : file worker séquentielle réelle
avec budgets cumulés de messages/payloads, annulation coopérative et drainage.
Dix suites réseau passent ; douze cas de stream incluent le vrai worker derrière
un stream contrôlé. La file n'est pas encore instanciée dans le daemon ni reliée
au coordinateur durable. Cela ne change aucun statut d'activation EIP/consensus.
Voir `docs/VALIDATOR-EIP-ROADMAP.md`, section « File worker Altair bornée ».

Complément service Altair (25 septembre 2026) : la file worker est raccordée au
coordinateur durable pour blocs/votes/slashings, avec annulation aux checkpoints
et décision après commit. Les 18 scénarios officiels minimal head/reorg passent
par ce chemin (3 721 checks, 17 reprises). Ce service cœur n'est pas encore exposé
au daemon séparé ; aucun réseau Altair ni nouveau profil EIP n'est activé.
Détails et limites dans « Service worker–consensus durable » de la feuille
validateurs ; preuves `build-cmake/qualification/altair-durable-worker-20260925/`.

Complément IPC Altair (25 septembre 2026) : entrée `nano_altairReceive` désactivée
par défaut, authentifiée par le jeton réseau et limitée à une écoute loopback
lorsqu'un service est injecté. Le trajet HTTP–worker–RocksDB est testé sur deux
blocs officiels ; RPC/HTTP existants passent. L'instanciation dans le runtime,
l'appel du daemon et la récupération des acquittements perdus restent ouverts.
Aucune activation EIP/consensus ; détails dans la section « Entrée RPC privée
Altair » de `docs/VALIDATOR-EIP-ROADMAP.md`.

Complément configuration Altair (25 septembre 2026) : le cœur peut recharger
explicitement le profil complet déjà sérialisé dans les snapshots (91 quantités,
sans défaut implicite), avec contrôle de taille/digest avant parsing. Deux blocs
officiels et la reprise des snapshots passent, avec 502 profils invalides refusés.
La composition au démarrage avec une ancre authentifiée reste ouverte ; ce
chargeur n'active aucun fork ni EIP. Voir « Chargement explicite du profil Altair »
dans `docs/VALIDATOR-EIP-ROADMAP.md`.

Complément bootstrap Altair (25 septembre 2026) : import local borné des fichiers
état/bloc avec engagements externes de racines et domaine de fork. Deux ancres
officielles (dont une au slot 17), onze refus et quatre annulations passent avant
publication durable explicite. Ce chargeur ne fournit pas la politique de confiance,
de fraîcheur ou de finalité ; son intégration à `run_node` reste ouverte. Aucun
fork activé. Détails dans « Import local d'une ancre vérifiée » de la feuille
validateurs.

Complément runtime Altair (25 septembre 2026) : `nano run` peut maintenant
composer explicitement profil/ancre/Store/service RPC, avec initialisation et
reprise strictement séparées. Le démarrage géré est testé avec les vrais processus
Nano et nano-p2p, mais le protocole Altair n'est pas encore appelé par le daemon,
son horloge n'est pas pilotée et ses décisions ne sont pas liées à l'EVM. Aucun
statut EIP de production n'est clos par cette intégration. Voir
`docs/ALTAIR-RUNTIME.md` et la section « Composition dans nano run » de la feuille
validateurs.

Complément horloge Altair (25 septembre 2026) : runtime à heure locale configurable,
ticks périodiques et avant traitement, déduplication des instants, blocage sur
recul/saut excessif. Statut courant accessible par RPC privé authentifié. Cinq
suites passent, dont huit gardes temporelles déterministes. Les écritures restent
des snapshots complets ; ni la tenue en charge ni la synchronisation d'horloges
multi-hôtes ne sont qualifiées. Aucun protocole P2P Altair ni liaison EVM activés.
Voir `docs/ALTAIR-RUNTIME.md` et la feuille validateurs.

Complément IPC Altair (25 septembre 2026) : tickets bornés par session, retry sans
redispatch, consultation/annulation/retrait authentifiés et protection contre un
Submit retardé après annulation. La perte d'une réponse HTTP après admission est
testée avec un seul commit du bloc officiel. Les tickets ne persistent pas après
redémarrage ; leur rapprochement avec l'état durable et leur usage par le daemon
restent ouverts. Aucun changement d'activation EIP/réseau. Voir la section tickets
de `docs/ALTAIR-RUNTIME.md`.

Complément client IPC Altair (25 septembre 2026) : dispatcher asynchrone borné
Submit/Job/Cancel/Forget, avec récupération de réponses perdues et conservation
de l'admission quand le résultat est incertain. Onze suites réseau et deux tests
HTTP d'intégration passent. `Core::call` est exercé avec un commit unique des blocs
officiels ; l'instanciation dans le daemon et la négociation P2P restent ouvertes.
Aucun fork ni statut production activé ; détails dans `docs/ALTAIR-RUNTIME.md`.

Complément identité Altair (25 septembre 2026) : le statut RPC privé expose une
empreinte du profil complet, du choix de branche, de l'ancre et de la chaîne,
ainsi que les limites wire. Treize scénarios d'identité et quatre suites ciblées
passent ; la négociation par le daemon reste à intégrer. Aucun état EIP ni fork
activé ne change. Voir `docs/ALTAIR-RUNTIME.md` et la feuille validateurs.

Complément reprise IPC Altair (25 septembre 2026) : rotation atomique des sessions
à tickets pour refuser les anciennes admissions retardées, sans effacer les
workers encore actifs ni réécrire les décisions commitées. Cinq suites ciblées
passent, dont 64 courses admission/rotation. L'orchestration du daemon reste à
raccorder ; aucun statut EIP, fork ou production n'est promu par ce test.
Voir `docs/ALTAIR-RUNTIME.md` pour le périmètre et les limites de récupération.

Complément intégration daemon Altair (25 septembre 2026) : récupération IPC
réellement appelée par `nano-p2p` avant construction de son client à tickets,
avec contrôle du profil, rotation confirmée et attente d'un inventaire vide.
Douze suites réseau et cinq suites cœur/intégration passent, dont seize scénarios
de récupération contrôlée et le démarrage/reprise des processus réels. Le handler
Altair P2P, la propagation et la liaison atomique EVM restent ouverts. Aucun fork
ni statut EIP de conformité n'est promu. Voir `docs/ALTAIR-RUNTIME.md`.

Complément admission réseau Altair (25 septembre 2026) : handler libp2p optionnel
avec handshake de profil complet, passage par les tickets et conservation de la
capacité jusqu'au résultat durable. Treize suites réseau et six suites cœur passent ;
le trajet réel reçoit deux blocs officiels et le payload maximal de 8 Mio. L'IPC
compact corrige le dépassement dû à l'hex sans réduire le profil ni élargir HTTP.
L'horloge de qualification est injectée ; ni propagation automatique, ni finalité
distribuée, ni liaison EVM ne sont closes. Aucun statut EIP/fork de production
n'est promu. Voir `docs/ALTAIR-RUNTIME.md` et la feuille validateurs.

Complément persistance Altair (25 septembre 2026) : blocs signés conservés dans
le même checkpoint que l'état, lecture RPC privée paginée, vérification des octets
et signatures à la reprise. Huit suites passent, incluant parcours officiels
head/reorg, corruption d'archives, erreur WAL injectée, annulation avant commit et
lecture des anciens checkpoints sans invention de signatures. Aucune conformité
EIP/fork n'est promue : diffusion automatique, pruning, stockage incrémental et
liaison consensus–EVM restent ouverts. Voir `docs/ALTAIR-RUNTIME.md`.


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


### Bridge : compromission de quorum (scénario Ronin)

La clôture de production inclut désormais explicitement la matrice
`docs/BRIDGE-SECURITY-REQUIREMENTS.md` : quorum et clés administratives compromis,
révocation, preuve de finalité, rejouabilité et réconciliation des réserves.
Tous ces essais de bridge restent non commencés. Le consensus et les EIP ne
constituent pas une preuve de sécurité des retraits inter-chaînes.


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


### Protection des signatures Beacon — 25 septembre 2026

`BeaconSigningJournal` réserve durablement avant le callback de signature BLS.
Propositions monotones, sources non décroissantes et cibles croissantes protègent
contre les doubles signatures/votes qui s'entourent, avec répétition exacte du
dernier message autorisée. Base dédiée par clé, engagement externe à la reprise,
refus d'une instance périmée et vérification de la signature retournée.
La campagne `build-cmake/qualification/beacon-signing-journal-20260925/` passe le
journal (18 refus avant signature, concurrence, faute de synchronisation et crash)
et les deux suites cryptographiques officielles. Aucun devoir de validateur
n'est activé. L'import/export EIP-3076 est ajouté à la matrice comme non commencé.


### EIP-3076 : export et arrêt durable (25 septembre 2026)

`retire_and_export_interchange()` produit le format v5 après synchronisation du
retrait permanent du journal source. La campagne
`build-cmake/qualification/beacon-signing-export-20260925/journal-tests.log`
valide les réservations exportées, l'idempotence, les refus après réouverture
et l'échec de synchronisation injecté. Le journal actif v1 reste lisible.
L'import/fusion et les vecteurs officiels inter-clients restent à qualifier.
L'arrêt de tous les utilisateurs externes de la clé est une précondition ; aucune
protection contre une clé volée ni activation réseau n'est revendiquée.


### EIP-3076 : import et corpus inter-clients (25 septembre 2026)

La campagne `build-cmake/qualification/beacon-signing-import-20260925/final-tests.log`
passe les deux suites locales. L'import par clé fusionne les maxima sans les
réduire, conserve les racines inconnues, refuse les documents invalides sans
mutation partielle et ne réactive pas une base retirée. Les snapshots v1/v2 sont
conservés ; v3 représente les protections sans racine connue.

Le corpus épinglé `bb7900688eecd287d8723154faac6addb5e63336` contient 38 cas :
65 imports par clé, 146 tentatives de réservation, 211 rechargements. Quatre cas
avec données invalides sont rejetés à l'import selon l'option upstream ; deux
répétitions exactes sont acceptées au-delà de la stratégie minimale stricte, sans
mutation. Le runner contrôle les racines officielles via le vrai journal, sans
signer. Voir `docs/BEACON-SIGNING-JOURNAL.md` pour l'oracle et la reproduction.
Le chemin du corpus externe est requis explicitement ; son absence n'est pas
une qualification. Le parcours multi-clés et les tâches de validateur restent
ouverts, tout comme les critères globaux de production.

Le verrou de signature est désormais partagé par les journaux associés au même
`ChainStore`, y compris pendant les callbacks. Le test natif bloque un backend,
demande un export depuis un second objet et vérifie l'attente puis les refus des
deux objets. Cela ferme la course lecture/signature entre instances locales ;
les copies indépendantes de clé/base restent hors de cette protection.


### EIP-3076 : parcours CLI hors ligne (25 septembre 2026)

`nano beacon-journal init/status/import/export` exige une identité explicite et,
pour toute base existante, un engagement externe exact. Les résultats sont écrits
dans un dossier neuf hors de la base, avec permissions privées et synchronisation.
L'import/export ne charge aucune clé privée et n'active aucune tâche de validateur.

La campagne `build-cmake/qualification/beacon-journal-cli-20260925/tests.log`
couvre le parcours avec le vrai binaire, la migration entre deux bases, les refus
d'écrasement, de base manquante/verrouillée, d'identité/engagement incorrects et
les protections après retrait/import. Les deux pannes d'artefact injectées sur
macOS vérifient l'échec explicite sans reprise de signature. Le journal et le
corpus officiel font partie des régressions. Voir `docs/BEACON-JOURNAL-CLI.md`.

La base et les reçus externes ne sont pas une transaction atomique commune.
Une panne entre leur écriture peut demander une récupération explicite ; aucun
contournement automatique du contrôle d'engagement n'est ajouté. La coordination
multi-clés, cette récupération et les tâches réelles restent ouvertes.


### Tâches locales d'attestation Altair (25 septembre 2026)

`prepare_altair_attestation_duty` dérive le vote depuis le choix de branche,
l'état avancé, le registre et le RANDAO authentifiés. Le coordinateur durable
relie cette préparation au journal anti-slashing avec identité complète,
engagement courant et horloge locale contrôlés. Il n'accepte aucun champ de vote
fourni par le demandeur et n'active pas de boucle de validation dans `nano run`.

La campagne `build-cmake/qualification/altair-attester-duty-20260925/tests.log`
passe quatre suites ciblées : publication du coordinateur, journal, CLI et
nouvelle tâche. Le scénario composé sur ancre officielle signe une attestation
BLS ensuite admise et archivée par le chemin normal ; il vérifie 14 refus avant
fournisseur, deux variantes d'inéligibilité, une expiration pendant signature
et le refus d'un coordinateur périmé. La proposition de blocs, les autres tâches,
la planification/diffusion, la gestion des clés et l'exécution atomique restent
ouvertes. Périmètre et sources : `docs/ALTAIR-ATTESTER-DUTIES.md`.

La préparation sur un bloc déjà au créneau cible a été corrigée après un test
positif supplémentaire (`current-slot-failure.log` puis `duty-final.log`). Le
refus d'une seconde tête est maintenant vérifié comme refus anti-slashing exact.
`p2p-regression.log` confirme aussi la propagation existante de votes officiels
entre deux démons réels ; cette preuve ne vaut pas planification/diffusion
automatique de la nouvelle tâche locale.


### 2026-09-25 — borne temporelle de sortie des attestations locales

Le contrôle de tâche Altair refuse maintenant un recul d'horloge entre lectures
pendant la même tentative, y compris dans le même slot, et revérifie délai et
annulation après la vérification BLS. Les échecs après réservation ne rendent
pas la signature et ne suppriment pas la protection anti-slashing. Quatre suites
passent dans `build-cmake/qualification/altair-duty-time-20260925/tests.log`, avec
reproduction avant correction conservée dans `before-fix.log` et contrôles de
refus conflictuel après réouverture. Voir [le périmètre des tâches](ALTAIR-ATTESTER-DUTIES.md).
Ce jalon ne clôt aucune EIP ni l'activation réseau : horloge opérationnelle,
planification, propositions, diffusion et binding EVM restent ouverts.

### 2026-09-25 — construction locale des blocs Altair

`prepare_altair_unsigned_block` partage les contrôles de transition avec le
chemin reçu et calcule la racine d'état avant signature. L'admission reçue conserve
ses vérifications BLS et de racine, sans option de contournement publique.
Le corpus officiel fournit 105 reconstructions exactes, 12 refus de candidats
invalides et trois enveloppes incorrectes dont seul le corps est reconstructible.
[Construction et limites](ALTAIR-BLOCK-CONSTRUCTION.md) ; preuves dans
`build-cmake/qualification/altair-unsigned-block-20260925/final-tests.log`.
Ce jalon prépare les tâches de proposition ; il ne clôt pas le consensus autonome,
le binding EVM, la finalité distribuée ou une EIP de fork ultérieur.


### 2026-09-25 — signature des propositions liée au Store

Le coordinateur durable construit maintenant une proposition sur la tête choisie,
contrôle créneau, clé/domaine et journal, puis réserve avant signature BLS. Le
résultat reste soumis à l'admission normale. Qualification composée : bloc
identique au vecteur officiel, admission par un second coordinateur, archive et
restauration, onze refus avant fournisseur et expiration finale conservant la
réservation. Voir [construction et signature](ALTAIR-BLOCK-CONSTRUCTION.md) et
`build-cmake/qualification/altair-proposer-signing-20260925/final-tests.log`.
RANDAO fourni par le corps de test, opérations en attente, automatisation,
diffusion persistante et binding EVM restent ouverts ; aucune activation réseau
ni clôture d'EIP supplémentaire n'est revendiquée.


### 2026-09-25 — RANDAO local du proposant

`sign_randao` dérive le proposant depuis le parent authentifié avancé au slot,
lie clé/époque/fork/genesis et signe uniquement le domaine RANDAO. Le journal
sérialise avec son retrait et vérifie son checkpoint sans modifier les réservations
anti-slashing de blocs/votes. La révélation officielle est reproduite puis utilisée
dans le bloc signé/admis. Sept refus avant fournisseur, signature invalide,
expiration et passage synthétique à l'époque 1 sont couverts par
`build-cmake/qualification/altair-local-randao-20260925/final-tests.log`.
Voir [construction et RANDAO](ALTAIR-BLOCK-CONSTRUCTION.md). Restent la sélection
d'opérations, la planification, la diffusion persistante et la liaison EVM ; ce
jalon n'active aucun réseau et ne clôt pas le consensus de production.

### 2026-09-25 — finalité obtenue avec des blocs et votes générés localement

La qualification `tests/test_altair_local_finality.cpp` enchaîne sélection du
proposant, RANDAO local, construction/signature de blocs, tâches d'attestation,
admission et transitions de finalité. Trois scénarios de 33 blocs / 64 clés de
fixture exigent : finalité avec participation complète, absence de finalité sans
votes ou avec 21 clés votantes. Les bases du coordinateur et des 64 journaux sont
fermées/rouvertes au milieu de chaque scénario. Preuves :
`build-cmake/qualification/altair-local-finality-20260925/final-tests.log`.
[Portée et limites](ALTAIR-LOCAL-FINALITY.md) : test local piloté, sans SyncCommittee
participant, sans réseau distribué ni binding EVM. Le critère de finalité/reprise
réseau du goal reste ouvert ; cette campagne qualifie l'assemblage local.

### 2026-09-25 — rattrapage P2P d'une chaîne produite localement

`consensus_altair_generated_chain_libp2p` connecte deux vrais démons libp2p aux
coordinateurs source/récepteur et exige la réception de 33 blocs générés par les
64 clés de fixture. Vérifications : octets archivés, état SSZ de tête, justification,
checkpoint finalisé et restauration de celui-ci depuis le disque destinataire.
Preuve : `build-cmake/qualification/altair-generated-p2p-20260925/final-tests.log`.
Voir [périmètre réseau](ALTAIR-LOCAL-FINALITY.md). Ce test qualifie un rattrapage
réseau à horloge pilotée ; la finalisation collaborative, les partitions, la
planification autonome et le binding EVM restent ouverts.

### 2026-09-25 — rattrapage après perte d'ACK et panne du démon P2P

Le scénario `consensus_altair_generated_chain_libp2p_restart` exige la perte d'un
ACK observée, un récepteur tué par SIGKILL après persistance d'un préfixe et une
reconnexion des deux démons avec leurs identités conservées. Le rejeu doit préserver
ce préfixe et rejoindre exactement les 33 blocs et la finalité de la source.
Preuve : `build-cmake/qualification/altair-generated-p2p-restart-20260925/tests.log` ;
[périmètre précis](ALTAIR-LOCAL-FINALITY.md). Le cœur reste vivant pendant la panne
P2P. Les partitions de consensus, la redécouverte automatique, le crash du cœur
et la liaison à l'exécution ne sont pas clos par ce test.

### 2026-09-25 — refus d'une réécriture finalisée malgré des signatures valides

`consensus_altair_adversarial_finality` construit deux transitions complètes signées
en contournant volontairement le journal local des clés de fixture. La branche
qui évite le checkpoint finalisé est acceptée par un Store témoin au genesis,
mais refusée par le Store finalisé, avant et après fermeture/réouverture de sa base.
Un bloc concurrent descendant du checkpoint finalisé reste admissible. Preuve :
`build-cmake/qualification/altair-adversarial-finality-20260925/tests.log` ;
[périmètre et limites](ALTAIR-LOCAL-FINALITY.md). Les checkpoints hostiles au bootstrap,
les attaques de quorum, les partitions et les sanctions économiques restent hors
de cette qualification locale.


### 2026-09-25 — preuve de double proposition incluse et sanction persistante

Le scénario adversarial inclut maintenant les deux headers concurrents dans un
bloc produit/signé/admis normalement. Il compare toutes les balances avec une
transition témoin sans preuve, vérifie pénalité/récompense configurées, sortie,
échéance de retrait et vecteur de slashings, puis rouvre la base. Signature de
preuve altérée, doublon et rejeu ultérieur sont refusés avant signature sans
mutation ; un bloc normal est ensuite admis avec état attendu et finalité intacte.
Preuve : `build-cmake/qualification/altair-generated-slashing-20260925/final-tests.log` ;
[limites](ALTAIR-LOCAL-FINALITY.md). Sanctions corrélées, retrait final, preuves
acheminées automatiquement et profil Electra restent hors de ce scénario Altair.

### 2026-09-25 — socle Bellatrix pour le payload d'exécution

Le conteneur `ExecutionPayload` dispose d'un codec SSZ canonique et d'une racine
SSZ qualifiés sur huit vecteurs officiels épinglés : octets/racines exacts,
offsets et budgets contrôlés, uint256 conservé et annulation. Sources, limites et
critères suivants : [payload Bellatrix](BELLATRIX-EXECUTION-PAYLOAD.md). Preuves dans
`build-cmake/qualification/bellatrix-execution-payload-20260925/`.
État : composant de format implémenté, intégration consensus–EVM non commencée
pour ce schéma. Aucune validation d'exécution, commit atomique, activation réseau
Bellatrix ou conformité de fork complète n'est revendiquée.

### 2026-09-25 — candidat Bellatrix vérifié par l'EVM Paris

L'adaptateur `prepare_bellatrix_execution` lie le payload au contexte fourni
(parent, numéro, timestamp, RANDAO), reconstruit son hash RLP et rejoue les
transactions signées sur une copie de l'état parent authentifié. Les engagements
d'exécution faux sont refusés, y compris avec un hash annoncé cohérent. Les
annulations EVM sont propagées sans publication d'état partiel. Quatre
transactions synthétiques (types 0/1/2, stockage, REVERT), 16 payloads invalides,
41 points d'annulation et les huit vecteurs SSZ officiels passent les deux tests
ciblés. [Portée et preuves](BELLATRIX-EXECUTION-PAYLOAD.md).

État : intégration consensus–EVM commencée au niveau du candidat isolé.
L'appel depuis une transition Beacon Bellatrix authentifiée, la persistance
atomique des deux états et leur reprise restent à implémenter/qualifier.
Aucune activation réseau, clôture de fork ou aptitude production n'en découle.

### 2026-09-25 — liaison du payload à l'état Beacon Bellatrix

Les schémas SSZ du header d'exécution, de l'état Beacon (25 champs) et du corps
(10 champs) sont qualifiés par 24 vecteurs officiels épinglés. L'opération
`prepare_bellatrix_payload_state` authentifie la racine de départ, lie le parent
d'exécution au header mémorisé et dérive timestamp/RANDAO de l'état, puis prépare
les deux résultats sans mutation des parents. Les erreurs de disponibilité ou
de format de l'historique `BLOCKHASH` sont fatales, même si leurs effets EVM
seraient compatibles avec les engagements annoncés. Sept tests ciblés passent,
dont les régressions de schémas Phase0/Altair et des blocs Altair complets.
[Preuves, sources et limites](BELLATRIX-EXECUTION-PAYLOAD.md).

État : opération isolée implémentée/testée, transition signée Bellatrix complète
et durabilité conjointe toujours non terminées. Les essais de liaison utilisent
un état synthétique de format ; ils ne ferment aucun EIP ni le profil réseau.

### 2026-09-25 — slots/epochs Bellatrix et racines historiques

`prepare_bellatrix_slots` et `prepare_bellatrix_epoch` avancent désormais un état
déjà Bellatrix avec budgets locaux et marqueur de traitement. Les racines de
slots utilisent les 25 champs, en-tête d'exécution inclus ; les opérations
d'epoch communes sont réutilisées avec paramètres Bellatrix sans substituer une
racine Altair. Sept cas officiels, 100 slots et les sept comparaisons de parcours
groupé/fractionné produisent les post-états exacts. Des essais supplémentaires
couvrent annulation, double epoch, budgets et conservation du header lors d'un
rollover historique. [Sources, preuves et limites](BELLATRIX-SLOTS.md).

État : avancement slots/epochs implémenté et qualifié sur ce corpus ; traitement
signé du bloc Bellatrix, admission dans le choix de branche et persistance
Beacon–EVM conjointe toujours à terminer. Aucune aptitude production déduite.

### 2026-09-25 — bloc Bellatrix signé avec transactions EVM

`prepare_bellatrix_state_transition` raccorde maintenant slots/epochs, signature
BLS et proposant, exécution EVM avant la mise à jour du mix RANDAO, opérations de
bloc et racine finale. Les opérations communes réutilisent le parcours Altair
avec une projection privée réversible ; les engagements restent Bellatrix.
La construction locale est distincte de l'admission et ne signe pas de bloc.

Une chaîne synthétique de neuf blocs signés parmi 64 clés de fixture traverse
une epoch, exécute neuf transactions et inclut une double proposition sanctionnée
avec comparaison de toutes les balances. Sept blocs invalides, sept points
d'annulation et trois budgets sont couverts. Les six suites ciblées passent.
[Sources, preuves et limites](BELLATRIX-TRANSITION.md).

État : transition post-Merge complète implémentée en mémoire, qualification
encore partielle et non différentielle pour Bellatrix. Choix de branche,
signature locale durable et persistance/reprise Beacon–EVM ne sont pas encore
raccordés à ce parcours. Aucun EIP ou profil réseau n'est clôturé par ce test.

### 2026-09-25 — Store Bellatrix lié aux branches EVM

Le parcours signé Bellatrix alimente désormais un Store privé qui conserve
ensemble les états Beacon/EVM et les reçus par branche. La qualification locale
couvre dix blocs conservés, deux bascules de tête pilotées par six votes BLS,
les lectures du hash parent et le refus des liaisons d'exécution incohérentes.
Voir [BELLATRIX-FORK-STORE.md](BELLATRIX-FORK-STORE.md) pour la portée et les
limites : pas de commit atomique RocksDB, de reprise ni d'activation réseau
Bellatrix à ce stade. Cette étape ne clôture aucune qualification de production.

### 2026-09-25 — Primitive de commit atomique consensus/exécution

`commit_execution_checkpoint` regroupe snapshot, branche EVM, états, reçus et
index dans un batch RocksDB synchronisé, avec CAS du jeton et de la tête. Il
supporte les réorganisations, y compris vers une branche plus courte. Tests
locaux de concurrence, SIGKILL, refus de candidats incohérents et erreur de sync
avec conservation/perte du suffixe WAL : voir
[EXECUTION-CHECKPOINT.md](EXECUTION-CHECKPOINT.md). La sérialisation authentifiée
et la reprise du Store Bellatrix ainsi que son reactor réseau restent ouvertes ;
la primitive opaque ne clôture pas à elle seule la persistance du consensus.

### 2026-09-25 — Snapshot et reprise des branches Bellatrix/EVM

Le Store Bellatrix dispose d'une enveloppe canonique liée au profil attendu et
à un digest indépendant. La qualification compose cette sauvegarde avec le commit
atomique RocksDB : trois réouvertures, dont deux changements de branche, retrouvent
la même tête Beacon/EVM, le même état et les mêmes reçus. Six corruptions/écarts
sont refusés. Voir [BELLATRIX-STORE-RECOVERY.md](BELLATRIX-STORE-RECOVERY.md).
Ce sont des checkpoints locaux authentifiés, pas une synchronisation depuis un
pair. Le coordinateur durable Bellatrix, ses archives signées et le réseau restent
à raccorder ; la production et la finalité distribuée ne sont pas déclarées closes.

### 2026-09-25 — Coordinateur durable Bellatrix

Les événements blocs/votes/horloge/slashing sont raccordés au checkpoint atomique
via `BellatrixDurableReactor`. Qualification locale : équivalence au Store pur,
deux bascules de tête, redémarrage avec neuf blocs signés et six votes archivés,
annulations avant écriture, auteur obsolète et erreur de sync sur macOS.
Voir [BELLATRIX-DURABLE-REACTOR.md](BELLATRIX-DURABLE-REACTOR.md). La signature locale,
le transport Bellatrix et la qualification distribuée restent ouverts. Aucun
fork réseau n'est activé et la production n'est pas déclarée qualifiée.

### 2026-09-25 — Signature locale Bellatrix liée au Store

Le coordinateur dérive les duties, construit/exécute les propositions et utilise
le journal anti-slashing durable avant signature. Qualification locale : huit
propositions/reveals, une attestation et cinq refus, dont une double proposition
après réouverture du journal et une expiration pendant la signature. Voir
[BELLATRIX-LOCAL-SIGNING.md](BELLATRIX-LOCAL-SIGNING.md). La boucle autonome et le
transport réseau Bellatrix restent à raccorder ; pas d'activation de production.

### 2026-09-25 — Lot réseau Bellatrix en cours, non qualifié

Le code relie maintenant `run_node`, le runtime Bellatrix, les méthodes privées
`nano_bellatrix*` et le daemon libp2p sous `/nano/bellatrix/1.0.0`. Les blocs/votes
archivés passent par l'inventaire, l'export borné et les tickets d'admission durable.
Chaque enveloppe porte l'identité complète du profil ; les enveloppes Altair à
cinq champs ne sont pas acceptées en Bellatrix. Les imports/synchronisations de
blocs legacy sont refusés dans ce mode ; le gossip des transactions est conservé.

Le mode genesis `beacon` exige Paris. Le runtime partage la base canonique et
exige soit un snapshot initial local authentifié, soit un jeton exact de reprise,
sans repli automatique. Le RPC ne démarre qu'après vérification du checkpoint.
L'horloge locale invalide le coordinateur en cas de discontinuité ou erreur locale.
Tête, état EVM, mempool et événements sont publiés après le commit atomique ; la
préparation de la mempool conserve les métadonnées déjà authentifiées et traite
les transactions communes aux branches avant réadmission des orphelines.

Compilation réussie de `nano`, `nano-p2p`, `nano-beacon-execution-gate-tests`,
`nano-txpool-canonical-tests`, `nano-bellatrix-wire-identity-tests` et
`nano-bellatrix-transition-tests`. Journaux locaux :
`build-cmake/qualification/bellatrix-network-integration-20260925/`.
La première compilation a révélé des comparaisons string/JSON corrigées ; un
appel a ensuite demandé une nouvelle cible avant régénération de CMake, puis la
régénération et la compilation de cette cible ont réussi.

La campagne regroupée a exécuté les nouveaux scénarios. Les nouveaux fichiers
couvrent les contrôles d'accès avant vérification du checkpoint, l'isolation des
mutations legacy, les enveloppes de réseau/IPC et la mempool lors des réorganisations.
Le scénario local de boucle validateur passe après les corrections décrites
ci-dessous. La finalité distribuée et la reprise réseau restent à terminer. La compilation
ne clôture aucun EIP ni critère de production. Aucun push ou déploiement de ce lot.

La boucle validateur locale est maintenant écrite et raccordée à la configuration
optionnelle `bellatrix.validator`. Elle charge les clés BLS depuis les variables
d'environnement nommées (exclues de l'environnement du daemon P2P), vérifie leur
liaison au registre authentifié et utilise un journal distinct par clé. Elle
construit le payload depuis la mempool et l'état de la branche, réserve de la
place pour les attestations, signe via le journal puis repasse par l'admission
normale. Les réservations de signature déjà durables sont respectées après reprise.
Le chemin Beacon de préparation EVM n'ajoute plus la récompense legacy de minage.

Nouveau scénario écrit : `consensus_bellatrix_validator_loop` (64 clés locales,
horloge synthétique, transaction de la mempool, finalité puis réouverture de la
base). **Il passe en 80,26 s et ne représente pas un réseau distribué.**
Résultat : 31 propositions, 200 attestations admises, époque 2 finalisée et
époque 3 justifiée, transaction exécutée, état/racine/finalité retrouvés après
réouverture. Les scénarios de mempool, de contrôle de démarrage, d’identité des
enveloppes et de transitions signées passent également dans leur périmètre.
Le constructeur utilise pour l'instant une participation sync vide et les votes
archivés ; il refuse une proposition nécessitant des dépôts obligatoires qu'aucun
fournisseur local ne lui a donnés. La collecte des opérations, l'agrégation et la
qualification de performance restent ouvertes. La validation complète de chaque
vote candidat est une base de correction, pas une preuve de débit en production.

La première exécution de la boucle a révélé deux écarts corrigés : abstention
lorsqu'un checkpoint d'époque précède l'ancre disponible, puis conservation
durable des votes du slot courant sans effet prématuré sur le choix de branche.
Le tick du slot suivant applique ces votes ; la reprise vérifie aussi les votes
en attente. Le scénario a été prolongé à quatre époques pour respecter le délai
FFG de démarrage, sans réduire les critères : époque finalisée >= 2 et justifiée >= 3.

Le nouveau scénario `consensus_bellatrix_validator_network` utilise deux daemons
libp2p et deux cœurs HTTP/base séparés dans un seul processus de test. Sa première
exécution a synchronisé les blocs et l'état mais a échoué sur la participation :
les retours de messages identiques modifiaient inutilement le jeton durable et
perturbaient les tâches locales. Les doublons exacts vérifient désormais le
checkpoint existant sans recommit. Le scénario réseau passe après cette correction en 112,33 s : même état EVM,
reçu de transaction et époque finalisée 2 sur le nœud observateur. Ce scénario concentre les clés
sur un seul hôte validateur, avec un second nœud observateur ; il ne qualifie pas
la répartition du stake entre machines ni les partitions réseau.

Le contrôle de concurrence a été adapté pour modifier effectivement l'horloge :
un appel identique n'écrit plus de checkpoint. L'ancien scénario supposait à
tort que deux opérations sans changement de contenu créaient un auteur obsolète.
Les contrôles conservent les refus d'auteur obsolète, les annulations avant commit
et l'erreur de synchronisation disque ; la dernière exécution passe en 4,07 s.
Le journal `cas-final.log` conserve ce résultat.

Le scénario ajouté `consensus_bellatrix_runtime_restart` passe en **8,98 s**.
Il appelle le véritable chargeur `start_bellatrix_runtime` avec profil/snapshot
sur disque, 64 clés BLS synthétiques chargées par variables d’environnement et
64 journaux séparés. Il produit un bloc, arrête les services, retient explicitement
les engagements du coordinateur et des journaux, rouvre la base puis produit le
bloc suivant. Un nouvel appel au même slot ne répète pas les signatures réservées.
Il refuse un digest de profil altéré, une capacité de transport insuffisante,
deux sources de confiance simultanées, une clé incompatible avec son index et
un engagement de reprise obsolète. Une régression de l’horloge ferme l’accès à
l’exécution. `BellatrixValidatorLoop::restart_commitments()` permet la capture
explicite des engagements des journaux ; ce n’est pas une sauvegarde externe
automatique ni une garantie contre le rollback de ces engagements.
Preuve locale : `runtime-restart.log` dans le répertoire de qualification du lot.
Ce scénario reste dans un processus avec horloge synthétique : le lancement par
CLI, les cœurs indépendants, le stake réparti et les partitions restent à qualifier.

Les trois contrôles de non-régression du transport partagé passent en 3,06 s :
`consensus_altair_wire_admission`, `consensus_altair_core_client` et
`consensus_altair_libp2p`. Ils ont été reconstruits après les changements aux
schémas d’enveloppes, au dispatch IPC et à la compatibilité des pairs. Le dernier
utilise un vrai daemon et neuf identités de pairs distinctes ; ce résultat couvre
l’admission réseau, pas la finalité distribuée. Journal :
`shared-transport-regression.log`. Les scénarios réseau Bellatrix déjà passés
n’ont pas été rejoués pour cette seule extension du test de configuration.

### 2026-09-25 — Lancement CLI Bellatrix et reprise des processus

`consensus_bellatrix_managed_runtime` lance deux vrais binaires `nano run`,
chacun avec son daemon libp2p, sa base et son serveur HTTP. Le Genesis Beacon
est chargé par le parseur réel ; le snapshot de consensus synthétique correspond
à ce Genesis. Les 64 clés de test sont chargées via l’environnement du seul
nœud validateur. L’autre cœur est observateur. L’horloge est celle du système.
Le scénario attend la synchronisation au bloc 3, arrête le validateur,
reprend avec les engagements capturés à l’arrêt et vérifie la réception du bloc
suivant sur l’observateur resté actif. La première exécution passe en 18,21 s
(blocs 3 puis 4). Le test du chargeur/reprise passe aussi en 6,46 s après
raccordement au même arrêt explicite.

L’arrêt expose `bellatrix-runtime-stopped` avec le jeton du coordinateur et ceux
des journaux des clés. Le chemin `run_node` termine le daemon et les workers HTTP
avant la capture ; `stop_bellatrix_runtime` termine les devoirs locaux, l’horloge
et les workers consensus avant de lire ces engagements. Aucun secret n’est
inclus. Ces métadonnées doivent être retenues séparément et reportées dans la
configuration de reprise ; leur émission dans un journal ne constitue pas une
sauvegarde externe automatique. Un arrêt brutal et la perte du dernier engagement
ne sont pas résolus par une confiance automatique dans le disque.

Périmètre prouvé : processus indépendants, démarrage CLI, production signée,
synchronisation P2P et reprise propre d’un validateur. Restent ouverts : stake
réparti entre signataires indépendants, finalité distribuée, partition/rattrapage,
arrêt brutal et procédures de conservation durable des engagements. Aucun de
ces résultats ne clôture le lancement public ni les autres EIP de la matrice.

Après correction de l’ordre de terminaison HTTP, le scénario concerné est rejoué
et passe en **17,97 s**, avec synchronisation aux blocs 3 puis 4 après reprise.
Journal complet : `managed-drain-qualification.log`. Cette réexécution couvre
la modification de l’arrêt ; les suites EVM et de finalité locale inchangées
n’ont pas été relancées. Aucun push ni déploiement de ce lot à ce stade.

### 2026-09-25 — Qualification des signataires répartis

Nouveau scénario `consensus_bellatrix_distributed_finality` : deux cœurs `nano run`
et deux daemons libp2p sur un même Mac, chacun avec 32 des 64 clés synthétiques.
Les environnements enfants ne reçoivent que leurs clés respectives. Le scénario
redémarre un des validateurs avec ses engagements exacts, puis attend quatre
époques avec horloge réelle. Le critère de passage exige le même checkpoint
finalisé (racine et époque), d’époque >= 2, sur les deux nœuds ; une tête commune
ou un bloc produit ne suffit pas. Les engagements d’arrêt exposent maintenant
également le checkpoint finalisé et l’époque justifiée pour cette vérification.

Le budget propre à ce test est explicite : 128 blocs, 2 048 votes archivés,
64 Mio d’états cumulés, enveloppe de 128 Mio. Ce budget ne modifie pas les règles
du registre ni les paramètres d’un réseau déployé. Il ne corrige pas l’absence
d’élagage des états/blocs du Store, qui reste un écart de fonctionnement durable.
Ce scénario ne couvre ni plusieurs machines physiques ni les partitions.

**Résultat : passage en 183,45 s.** Les deux nœuds finalisent l’époque 2 avec la
même racine `0xc90b8d700df145836b8a3d43af2c85fd536c4a9271a548da0f1a89509c07799f`
et justifient l’époque 3. Le scénario atteint le slot 32 (hauteur d’exécution 31,
un slot sans bloc) après redémarrage d’un validateur à la hauteur 3. Cette preuve
couvre une finalité commune avec signataires répartis entre deux processus,
chacun détenant la moitié des clés, sur **une seule machine physique**.
Journal complet : `distributed-qualification.log` dans le répertoire du lot.
Les partitions, la panne brutale, le rattrapage prolongé, l’élagage et la mise
à l’échelle ne sont pas clos par ce résultat. Le RPC refuse encore explicitement
les tags `safe`/`finalized` ; leur raccordement au checkpoint Beacon reste ouvert.
Aucun push, changement du VPS ou lancement public n’est effectué ici.

### 2026-09-25 — Raccordement RPC du checkpoint finalisé Beacon

Le tag `finalized` est raccordé au bloc d’exécution du checkpoint finalisé
Beacon. Il est publié sous le verrou de chaîne après le même commit atomique
que la tête/l’état, vérifié contre l’ancêtre canonique ou le suffixe validé,
et ne peut reculer ou changer de hash à hauteur égale. Après réouverture,
le coordinateur revalide le snapshot et le bloc canonique avant de rendre
ce tag disponible. Le tag est refusé sans consensus Beacon authentifié ;
il ne constitue jamais un alias de `latest`.

Référence : [paramètre de bloc JSON-RPC Ethereum](https://ethereum.org/en/developers/docs/apis/json-rpc/#the-block-parameter).
Méthodes raccordées par le résolveur commun : lecture de blocs/transactions,
solde, nonce, code, stockage, historique des frais et simulations. `safe` reste
refusé, et le parseur spécifique des filtres/logs ne prend pas encore en charge
`finalized` : aucune conformité RPC complète n’est déclarée ici.
Le scénario de finalité locale comporte désormais une transaction supplémentaire
après le checkpoint : le stockage finalisé doit rester à 2 tandis que la tête
courante vaut 3. Les réponses sont comparées à l’état d’exécution associé au
checkpoint du Store, puis revérifiées après réouverture.

**Qualification passée : 3/3 en 90,11 s.** Contrôle d’accès : 0,56 s ;
configuration/reprise : 9,11 s ; boucle avec finalité et lectures RPC : 80,44 s.
Le scénario compare huit familles de réponses au checkpoint puis après
réouverture, vérifie le stockage finalisé à 2 contre 3 à la tête et l’absence
de mutation canonique par `eth_call`. L’époque 2 reste finalisée, l’époque 3
justifiée. Preuve : `finalized-rpc-qualification.log`. Les scénarios P2P longs,
non modifiés par ce raccordement, n’ont pas été relancés. Les binaires de test
ont été liés au code modifié ; aucun binaire n’a été déployé sur le VPS.

### 2026-09-25 — Coupure du transport entre deux moitiés du stake

Scénario ajouté : `consensus_bellatrix_partition_recovery`. Il démarre les deux
cœurs et daemons réels, répartit les 64 clés en deux ensembles de 32, effectue
une reprise propre puis attend quatre époques. Après établissement d’une finalité
commune, il suspend uniquement les deux daemons enfants par SIGSTOP pendant
deux époques (96 s avec ce preset). Les cœurs/signataires restent actifs.
Il exige que les deux branches avancent et divergent, sans changement du bloc
finalisé. Après SIGCONT, il exige un nouveau bloc finalisé commun plus récent
et la convergence des têtes. Les daemons sont repris même si une assertion échoue.
La coupure est une suspension du composant transport, pas une simulation de
latence/perte sélective de paquets ni de plusieurs machines physiques.

La durée totale est bornée par CTest à 720 s. Aucun trafic de flood ni fonds
réels. `partition-run-inputs.json` conserve les empreintes des binaires et des
sources modifiées utilisées, avec le commit de base. Le résultat doit être lu
avec le journal complet, sans assimiler le démarrage du scénario à son succès.

**Première exécution : échec en 519,26 s.** La finalité reste inchangée pendant
les 96 s d’isolement et les têtes reconvergent ensuite, mais aucun checkpoint
plus récent n’est finalisé dans les cinq époques de récupération. Le journal
`partition-qualification.log` et `partition-run-inputs.json` conservent cet échec.
Il ne faut pas le résumer comme une réussite de la reprise après partition.

L’inspection du relais révèle un blocage de parcours sur un vote explicitement
refusé et une cadence d’un seul élément par seconde, alternant blocs/votes.
Correction en cours de qualification : les votes refusés sont différés jusqu’à
la fin d’un parcours, sans bloquer les suivants ni être considérés comme livrés.
Un ACK absent/ambigu ne fait toujours pas avancer le curseur ; l’ordre des
blocs reste inchangé. Le relais utilise un timer indépendant, 100 ms par défaut,
réglable par `NANO_CONSENSUS_RELAY_INTERVAL_MS` (25..60000 ms), avec toujours une
seule tentative en vol et les mêmes budgets d’admission entrants. Cette cadence
est une politique locale, pas une règle de stake. Le nouveau scénario de transport
`consensus_relay_rejected_vote_progress` vérifie les refus, la reprise différée,
les réponses perdues et la conservation de l’ordre des blocs ; il passe.
Le lien causal avec l’échec de finalité doit encore être confirmé par le scénario
complet, dont les critères restent inchangés.

**Seconde exécution : non qualifiée.** Le contrôle Altair/libp2p passe en 2,30 s,
mais le scénario de partition échoue en 228,84 s sur
`isolated half-stake advanced finality`, avant la récupération réseau. Le journal
complet `partition-recheck.log` contient les traces de relais ;
`partition-recheck-inputs.json` identifie les sources et binaires de ce passage.
Le test local de progression du relais passe en 0,33 s. Cela ne suffit pas à
valider la correction de la récupération après partition.

La trace du second échec ne donne pas les hauteurs et racines des deux réponses
ayant déclenché cette assertion. Il faut capturer les checkpoints au changement
et leur lien au préfixe signé avant coupure pour distinguer une finalisation
retardée utilisant des votes antérieurs d’une finalisation illégitime pendant
l’isolement. Aucune violation du quorum n’est affirmée sur la seule assertion,
et aucune réussite de reprise n’est revendiquée. Les critères de sécurité ne
sont pas retirés. Aucun push ou déploiement de ce correctif non qualifié.

### Diagnostic FFG retardé — 25 septembre 2026

Le test `ethereum_official_altair_ffg` couvre désormais trois assertions supplémentaires : un quorum de l’époque précédente peut finaliser un checkpoint plus ancien sans votes de l’époque courante ; la racine finalisée reste celle du checkpoint ; sans ce quorum précédent, cette finalisation ne se produit pas. Les dix fixtures officielles et ces nouveaux cas passent (0,31 s). Source : [consensus-specs v1.5.0, justification et finalisation](https://github.com/ethereum/consensus-specs/blob/v1.5.0/specs/phase0/beacon-chain.md#justification-and-finalization).

Ce contre-exemple précise la portée d’une assertion de partition ; il ne démontre pas la cause du précédent échec réseau et ne clôt pas la reprise après partition. Le scénario réseau diagnostique conserve son assertion stricte pendant son exécution. Preuves et empreintes : `build-cmake/qualification/bellatrix-network-integration-20260925/{nano-delayed-ffg-test.log,delayed-ffg-inputs.json}`.

### Partition diagnostique : reprise non acquise — 25 septembre 2026

`consensus_bellatrix_partition_recovery` échoue en **518,79 s** après deux époques de coupure puis cinq époques de reconnexion. Pendant la coupure, les branches progressent sans changer la finalité ; le scénario ne rencontre donc pas cette fois l’assertion ambiguë de finalisation retardée. La finalité RPC reste au bloc `0x1`. À l’arrêt ordonné, les deux nœuds exposent la même finalité Beacon d’époque `0x0`, une justification d’époque `0x2`, `clockHealthy=true` et `validator.healthy=true`. Les compteurs locaux sont respectivement 39/35 propositions et 165/171 attestations admises (le compteur source repart après son redémarrage initial).

Cela écarte un arrêt déclaré de l’ordonnanceur ou de l’horloge dans cette exécution. Cela ne prouve ni la livraison utile de chaque vote, ni son inclusion dans la branche canonique : la prochaine investigation porte sur ces deux étapes et la participation par époque. La correction du relais ne clôt donc pas la reprise. Aucun assouplissement d’assertion, push ou déploiement n’a été réalisé. Preuves : `build-cmake/qualification/bellatrix-network-integration-20260925/partition-diagnostic.log` et `partition-diagnostic-inputs.json`.

### Conservation des preuves de partition — 25 septembre 2026

Le scénario sait désormais conserver ses bases **après arrêt des deux processus**, uniquement sur échec et avec `NANO_TEST_PARTITION_EVIDENCE` explicite ; il refuse d’écraser un dossier précédent. Le nouvel outil natif `nano-bellatrix-checkpoint-export` lit une copie de base et vérifie le checkpoint contre le jeton enregistré dans la sortie d’arrêt indépendante avant export. Il ne lance ni réseau ni signataire. Compilation réussie ; validation de l’export sur la prochaine base conservée encore en attente.

Une exécution diagnostique est en cours avec cette conservation, sans modification supplémentaire des règles de consensus ni de l’assertion de partition. Ses entrées sont empreintées dans `build-cmake/qualification/bellatrix-network-integration-20260925/partition-retained-inputs.json`. Aucun verdict nouveau de reprise n’est acquis à ce stade.

### Participation après partition et coût des clés sans devoir — 25 septembre 2026

L’exécution avec conservation échoue en **521,87 s**. Les bases arrêtées sont conservées (42 Mio). L’export natif vérifie avec succès les deux checkpoints contre les engagements d’arrêt ; les rapports dérivés des SSZ indiquent que le maximum de participation cible précédente dans les états retenus n’atteint que 20,3 %, 20,3 %, 9,4 % et 12,5 % aux époques 7 à 10, sur les deux nœuds. Ces maxima sur les branches retenues ne sont pas des métriques de réseau distribué ni une validation complète du parseur diagnostique.

L’inspection du chemin local montre une relecture du checkpoint complet (4 690 018 octets en fin de ce scénario) pour chaque clé interrogée par `sign_randao` et `sign_attestation`, y compris celles sans devoir. La vérification disque est déplacée après la sélection du devoir et reste obligatoire **avant toute signature éligible**. Cela élimine des relectures inutiles ; son effet sur la reprise de finalité reste à mesurer. Une qualification groupée de la chaîne signée et de la boucle de validation est lancée après cette modification.

Preuves : `partition-retained-db/`, `nano-{source,observer}-checkpoint.json`, `nano-{source,observer}-participation.txt` et le script d’analyse conservé, sous `build-cmake/qualification/bellatrix-network-integration-20260925/`.

### Sélection groupée des devoirs locaux — 25 septembre 2026

La seule suppression des lectures disque pour les clés sans devoir préserve les deux tests ciblés (84,17 s cumulées), mais ne réduit pas sensiblement la durée de la boucle locale (80,41 s contre 80,44 s précédemment). Elle ne peut donc pas être présentée comme la cause résolue du blocage.

Le lot ajoute une sélection groupée du proposant et du comité d’attestateurs à partir de l’état authentifié. L’ordonnanceur évite d’interroger les fonctions de signature pour toutes les clés sans devoir ; les fonctions de signature revalident encore identité, slot, engagement disque et journal avant utilisation du signataire. La préparation du comité partage désormais les traversées d’état et le calcul des affectations. Les nouveaux contrôles comparent le proposant à la sélection de référence, vérifient les huit affectations du profil minimal, leur bit unique, l’exclusion d’un équivoque et le refus d’un slot périmé.

Compilation réussie ; qualification groupée runtime/redémarrage, boucle locale et chaîne signée en cours. Ni la reprise réseau ni la capacité à 100 000 validateurs ne sont démontrées par cette modification. Entrées : `batch-duties-inputs.json` dans le dossier de qualification Bellatrix.

La sélection groupée passe ses trois tests : redémarrage 8,66 s, boucle locale 75,01 s et chaîne signée 3,72 s. Ce gain reste modeste et ne prouve pas encore la reprise réseau.

### Attestations identiques déjà incluses — 25 septembre 2026

Les blocs conservés montrent des inclusions répétées des mêmes attestations. Le constructeur local filtre maintenant les empreintes des attestations identiques déjà incluses dans les **ancêtres récents du parent choisi**, sans consulter les branches concurrentes pour décider une exclusion. Les messages restent archivés pour le relais. Les nouveaux votes gardent la validation complète habituelle. La boucle locale vérifie désormais l’absence de doublons identiques dans sa chaîne et la conservation d’au moins 128 attestations utiles ; finalité, transactions, réouverture et garde-fous de signature restent dans la qualification groupée lancée. Résultat encore en attente.

La qualification groupée après déduplication passe **3/3** : runtime/redémarrage 8,69 s, boucle locale **28,04 s**, chaîne signée 3,75 s (40,49 s cumulées). La boucle locale était à 80,41 s avant sélection groupée/déduplication ; ces mesures sur le même Mac indiquent une réduction d’environ 65 % de sa durée, sans constituer un benchmark réseau. Le nouveau contrôle d’unicité et de conservation des votes utiles passe, ainsi que la finalité et la réouverture.

La partition est relancée avec ce lot modifié et conservation des bases en cas d’échec (`partition-optimized-db`, manifeste `partition-optimized-inputs.json`). Sa réussite n’est pas acquise au lancement.

### Reprise : arrêt des propositions et erreurs masquées — 26 septembre 2026

La partition après optimisation échoue en **518,11 s**. Les deux ordonnanceurs restent déclarés sains (203 et 195 attestations admises), finalité d’époque 0 et justification d’époque 2. Les bases concordent : horloge au slot 88, dernier bloc au slot 79. Les gains locaux n’ont donc pas résolu la reprise réseau.

L’inspecteur hors ligne reconstruit l’état depuis l’engagement d’arrêt vérifié. Au slot 88 il trouve le proposant 36, huit attestateurs et un corps de bloc valide de 3 395 octets. Le diagnostic de corps reconnaît uniquement les clés publiques des fixtures synthétiques connues, prépare leur révélation RANDAO et ne publie ni bloc ni transaction. Il opère sur la copie arrêtée, sans horloge active ni réseau. Cela écarte une impossibilité intrinsèque de construire le prochain bloc depuis cet état ; cela ne reproduit pas les courses concurrentes.

Le traitement d’erreur local avait un défaut : n’importe quelle `invalid_argument` pouvait être ignorée si le jeton avait changé entre-temps. La reprise est maintenant limitée à une exception typée `StaleBellatrixDuty`, émise explicitement lors d’un conflit de jeton. Les autres erreurs arrêtent l’ordonnanceur et restent visibles dans `failure` ; `staleDutyRetries` compte les conflits réellement classifiés. La lecture initiale du réacteur est elle aussi couverte par ce traitement. Nouveau contrôle de classification et qualification groupée en cours ; aucun nouveau résultat réseau revendiqué.

La qualification du traitement typé passe **3/3** : runtime/redémarrage 8,82 s, boucle locale 27,78 s, chaîne signée 3,77 s (40,37 s cumulées). La boucle locale termine sans erreur ni reprise de conflit dans ce scénario synthétique. Les conflits réels du réseau et la cause de l’arrêt des propositions restent à reproduire ; aucune nouvelle partition n’est lancée à ce stade. Les bases conservées servent au prochain diagnostic de reprise. Preuves : `nano-duty-race-qualification.log` et `duty-race-inputs.json`.

### Reproduction courte et proposition cohérente — 26 septembre 2026

L’inspection des journaux à partir de leurs engagements d’arrêt ne montre aucune proposition réservée au-delà du slot 79. Sur une **copie distincte**, l’outil natif reconnaissant les clés de fixture synthétiques prépare, signe puis admet le bloc du slot 88 : tête 79 → 88, sans réseau ni fonds réels (`nano-fixture-replay.json`).

La reprise de 24 secondes avec horloge et ordonnanceur actifs, sans P2P, reproduit ensuite le blocage : zéro proposition, neuf attestations, sept reprises pour conflit de version, tête toujours 79 (`nano-fixture-resume.json`). Les bases historiques sont conservées ; chaque essai utilise une autre copie.

Le chemin local de proposition regroupe maintenant RANDAO, construction et signature sous un même verrou du réacteur. Les wrappers unitaires conservent leur verrouillage ; les helpers privés sont utilisés uniquement sous ce verrou. L’horloge ne peut plus changer le jeton entre ces étapes. Délais réels, identité, journaux anti-slashing, validation complète et admission normale après signature restent appliqués. La reprise courte modifiée admet une proposition et treize attestations, avec trois conflits de version, tête 79 → 91 et ordonnanceur/horloge sains (`nano-fixture-atomic.json`). Ces deux observations bornées ne sont pas un benchmark ni une preuve de finalité réseau.

Qualification groupée chaîne signée, boucle locale, redémarrage et scénario à deux processus gérés lancée ; résultat en attente. Entrées empreintées : `atomic-proposal-inputs.json`.

La qualification de la proposition cohérente passe **4/4** en 49,19 s : processus réseau gérés/redémarrage, runtime local, boucle à 64 clés et chaîne signée. La boucle locale prend 26,78 s ; les scénarios conservent les contrôles de finalité, signature, annulation et réouverture dans leurs périmètres respectifs. La matrice principale a été actualisée pour distinguer intégration Bellatrix existante et qualification réseau encore partielle.

Le scénario complet de partition est lancé avec ce lot (`partition-atomic-inputs.json`, conservation prévue dans `partition-atomic-db`). Aucun verdict de reprise ni statut production n’est acquis au lancement.

### Départ à finalité 2, puis conflits locaux persistants — 26 septembre 2026

La partition avec proposition cohérente échoue en **517,25 s**. Avant isolement, la finalité atteint cette fois le bloc 16 / époque 2 ; elle reste inchangée pendant la coupure et la reprise. À l’arrêt : mêmes 77 blocs retenus, dernier slot 83, 218/217 attestations admises, aucune erreur déclarée, mais **206/203 reprises pour conflit de version**. Le résultat ne clôt pas la finalité après reconnexion.

Les opérations locales de devoirs dérivent désormais leurs données depuis l’état courant sous le verrou qui protège l’opération. Les appels locaux ne transportent plus un jeton capturé entre deux prises de verrou, périmé par les publications d’horloge. Les API à engagement explicite continuent à refuser les anciens jetons ; les vérifications disque, registre, clé, slot, signature et journal sont conservées. Les sélecteurs de tâches fournissent seulement des indications : les méthodes de signature redérivent et vérifient les données sous verrou.

Sur une nouvelle copie de la même base de référence, la reprise locale de 24 secondes admet **2 propositions et 14 attestations, zéro conflit de version**, tête 79 → 91, horloge et ordonnanceur sains (`nano-fixture-current.json`). Cette observation n’est pas un résultat réseau. Contrôles supplémentaires : équivalence du comité local au comité du checkpoint, refus d’un slot futur et impossibilité pour une clé non affectée d’atteindre le signataire. La qualification groupée correspondante est en cours. Le README décrit désormais ce profil Bellatrix et ses limites.

La qualification des devoirs dérivés sous verrou passe **4/4** en 49,34 s : chaîne signée 3,48 s, boucle locale 26,88 s, runtime/redémarrage et processus réseau gérés 18,97 s cumulées. Les nouveaux contrôles de clé non affectée et de slot futur passent. Le scénario complet de partition est lancé sur ce lot (`partition-current-inputs.json`, conservation prévue `partition-current-db`) ; son résultat reste en attente.

### Reprise réseau après correction des devoirs locaux — 26 septembre 2026

Le scénario `consensus_bellatrix_partition_recovery` passe en **420,77 s** sur les binaires du manifeste `partition-current-inputs.json`, dont les neuf empreintes ont été revérifiées après le test. Preuve conservée : `build-cmake/qualification/bellatrix-network-integration-20260925/nano-partition-current.log`. Aucun nouveau lancement n’a été nécessaire pour recueillir ce résultat.

Périmètre effectivement exercé : deux processus cœur et deux démons P2P, 64 clés synthétiques réparties entre deux validateurs, horloge réelle, un seul Mac. Le scénario comprend un redémarrage ordonné avec engagements externes, quatre époques initiales, 96 secondes de suspension des communications avec branches concurrentes actives, puis reconnexion. La finalité initiale (bloc 16 / époque 2) reste inchangée pendant la coupure. Après reconnexion, les deux nœuds retrouvent une tête commune et une finalité strictement plus récente ; à l’arrêt, même racine finalisée `0x9732ff022994f1c76ab927942248bc9fa6699db30a0dfbef5dc3baef08c531c5`, époque 7, justification 8. Les assertions de sûreté et de reprise n’ont pas été affaiblies.

Les échecs précédents restent conservés ci-dessus. Cette réussite clôt ce scénario borné de reprise ; elle ne clôt ni le registre ouvert, ni la reprise après crash sans engagement d’arrêt, ni les partitions prolongées, ni la qualification sur plusieurs hôtes. Le journal de réussite n’exporte pas les compteurs finaux de l’ordonnanceur : aucune affirmation de zéro conflit de version réseau n’en est déduite.

Preuve synthétique versionnée (événements observés et empreintes, sans clés privées) : [`qualification/bellatrix-partition-20260926.json`](qualification/bellatrix-partition-20260926.json). Le journal brut reste dans le dossier local de qualification indiqué ci-dessus.

### Vérification groupée des composants partagés — 26 septembre 2026

La revue du lot examine aussi le rattachement du runtime au nœud, le filtrage des secrets transmis au démon, l’arrêt avant capture des engagements, les tags RPC de finalité, la publication du mempool et le relais partagé Altair/Bellatrix. La vérification groupée de fin de lot passe **10/10 en 5,10 s** : historique/capacité/publication canonique du mempool, identité Bellatrix, verrou d’exécution, progression après vote refusé, reçus IPC, client cœur, transport libp2p Altair et journal anti-slashing. Ce contrôle cible les composants partagés modifiés ; les scénarios locaux et la partition réussis ne sont pas relancés.

Preuve versionnée : [`qualification/bellatrix-shared-20260926.json`](qualification/bellatrix-shared-20260926.json), empreintes des exécutables et du journal. Journal brut local : `build-cmake/qualification/bellatrix-network-integration-20260925/nano-bellatrix-lot-shared.log`. Ces résultats ne clôturent pas les critères de production encore ouverts.

### Réservations Beacon avec témoin durable — 26 septembre 2026

Une option C++ de `BeaconSigningJournal` écrit un témoin indépendant avant le principal. À la reprise, elle termine seulement la réservation exacte autorisée par le témoin ; elle refuse une base trop ancienne ou une identité différente. La politique de signature et le mode historique à engagement exact sont conservés. Le témoin doit être protégé de la restauration conjointe avec le principal ; ce n’est pas une détection de rollback de sa propre racine de confiance.

Qualification **2/2 en 2,30 s** : journal existant modifié et nouveau scénario `consensus_beacon_witness_recovery`. Nouveau périmètre : `SIGKILL` après réservation avant signature, réouverture sans reçu d’arrêt, restauration du seul principal à son prédécesseur puis achèvement exact, refus de rollback plus ancien, instance périmée, témoin absent et mauvaise identité, erreurs WAL injectées sur le témoin puis sur le principal empêchant toute signature. La perte de la dernière écriture principale est modélisée par une copie arrêtée ; ce n’est pas un essai de coupure électrique physique.

Preuve versionnée : [`qualification/beacon-witness-20260926.json`](qualification/beacon-witness-20260926.json). Journaux locaux : `build-cmake/qualification/beacon-witness-20260926/`. Raccordement au runtime, migration des journaux existants et reprise du checkpoint consensus/exécution restent ouverts ; aucun redémarrage complet après crash ni statut production n’est revendiqué.

### Raccordement des témoins au runtime Bellatrix — 26 septembre 2026

Le runtime charge désormais un témoin optionnel pour chaque clé et un témoin distinct pour le checkpoint couplé consensus/exécution. La publication du coordinateur synchronise l’intention complète avant le commit atomique principal. Au redémarrage, le snapshot est vérifié (profil, état, archives signées, digest) et seule l’écriture de son prédécesseur exact peut être achevée. Une version plus ancienne est refusée avant exposition de l’exécution. Le mode antérieur à engagement explicite demeure disponible.

Le nouveau scénario `consensus_bellatrix_witness_runtime` passe en **12,28 s** : runtime à 64 clés, reprise sans injecter les engagements d’arrêt, conservation tête/état, absence de signature répétée, reprise des propositions, récupération d’une publication du coordinateur depuis son prédécesseur et refus d’un rollback plus ancien avec exécution fermée. Le scénario existant `consensus_bellatrix_runtime_restart` passe en **6,16 s**. Un premier échec provenait du partage accidentel du témoin entre le cas de configuration invalide et le cas valide ; les dossiers ont été isolés, sans affaiblir le refus d’écrasement. Les deux journaux sont conservés.

Preuve : [`qualification/bellatrix-witness-runtime-20260926.json`](qualification/bellatrix-witness-runtime-20260926.json). Il reste à qualifier un arrêt brutal du processus réseau complet, la perte d’écriture lors d’une publication changeant effectivement la tête EVM et le coût du double stockage sous charge. La récupération actuelle ne protège pas la restauration conjointe des témoins et des bases principales.

### Crash du processus réseau et perte de publication EVM — 26 septembre 2026

Le nouveau scénario `consensus_bellatrix_crash_runtime` passe en **19,14 s** : deux vrais cœurs `nano run`, deux démons libp2p, 64 clés sur le validateur source, un observateur, un seul hôte. Après synchronisation au bloc 3, `SIGKILL` tue le cœur source sans événement `bellatrix-runtime-stopped`. Le redémarrage utilise uniquement les témoins configurés, sans injecter de reçus d’arrêt ; le bloc 3 reste canonique et le bloc 4 atteint l’observateur. Ce scénario ne comporte pas d’assertion de finalité distribuée (un seul nœud détient les clés).

Le scénario natif de récupération est complété par la restauration du principal à une base contenant seulement le genesis, en conservant le témoin de sa première publication réelle. Il reconstruit le bloc et l’état EVM attendus, puis vérifie encore la récupération du prédécesseur d’une publication d’horloge et le refus d’un rollback plus ancien. Cette version passe en **12,20 s** ; les anciennes preuves sont conservées.

Preuves versionnées : [`qualification/bellatrix-crash-20260926.json`](qualification/bellatrix-crash-20260926.json) et [`qualification/bellatrix-execution-recovery-20260926.json`](qualification/bellatrix-execution-recovery-20260926.json). Journaux bruts dans `build-cmake/qualification/beacon-witness-20260926/`. Les limites restantes sont l’indépendance opérationnelle des témoins, leur propre rollback, la migration des journaux existants, les pannes physiques et le coût des snapshots doublés sous charge. Aucun déploiement VPS ni fonds réel.

### Preuve de double vote jusqu’à la sanction locale — 26 septembre 2026

Le réacteur conserve désormais les preuves BLS d’attester slashing admises, dans un format durable v4. Les formats v1/v2/v3 restent lisibles et leur encodage reste inchangé en l’absence de preuves. La restauration revérifie SSZ, empreintes et signatures. La conservation utilise le budget explicite `equivocations` et le budget d’enveloppe ; les doublons exacts n’avancent pas le checkpoint.

Le constructeur local réserve de la capacité pour ces opérations et les insère avant les attestations, avec validation complète de chaque candidat. Il ignore les preuves dont tous les signataires communs sont déjà sanctionnés dans l’état parent ; les preuves restent conservées pour les branches concurrentes. Les transitions économiques existantes appliquent pénalité et sortie. Référence : [traitement officiel des attester slashings](https://ethereum.github.io/consensus-specs/specs/phase0/beacon-chain/#attester-slashings), avec les paramètres Altair/Bellatrix du profil actif.

Le nouveau scénario `consensus_bellatrix_local_slashing` passe en **6,62 s** : preuve altérée refusée sans publication, preuve réelle conservée, doublon sans nouvelle version, redémarrage puis inclusion par le proposant configuré, indicateur `slashed`, sortie programmée et solde débité. Il utilise une signature malveillante synthétique produite hors du journal, pour vérifier le traitement d’un validateur qui contourne volontairement sa protection locale.

L’encodeur ajouté reproduit exactement **30 preuves officielles SSZ** ; la suite étendue passe en **0,30 s**, avec refus des bornes invalides et propagation de l’annulation. Preuve : [`qualification/bellatrix-local-slashing-20260926.json`](qualification/bellatrix-local-slashing-20260926.json). Reste à raccorder l’inventaire/relais autonome de ces preuves, leur élagage après finalité, les sorties volontaires et les dépôts. Ce résultat local ne clôt pas le registre ouvert ni le cycle réseau des validateurs.

### Inventaire et relais des preuves Bellatrix — 26 septembre 2026

Les méthodes privées `nano_bellatrixSlashingInventory` et `nano_bellatrixSlashing` exportent les preuves archivées sous l’identité complète du réseau. Elles exigent le jeton du superviseur et passent dans la file RPC bornée. Le démon Bellatrix alterne désormais blocs, attestations et preuves avec un seul relais en vol. Le mode Altair conserve son alternance précédente. Un refus explicite de preuve permet de visiter les preuves suivantes puis de réessayer ; une réponse absente n’avance jamais le curseur. Un changement de session réinitialise aussi le curseur des preuves.

Qualification des nouveaux contrôles **2/2 en 5,85 s** : inventaire sans autorisation refusé, export SSZ identique, conservation/reprise/inclusion locale ; relais indépendant du transport avec refus, preuve suivante et réponse perdue. `nano-p2p` compile. Preuve : [`qualification/bellatrix-slashing-relay-20260926.json`](qualification/bellatrix-slashing-relay-20260926.json). La propagation effective d’une preuve entre pairs réels et la régression de débit liée à l’alternance sur trois types ne sont pas encore qualifiées ; aucune réussite de réseau distribuée n’est déduite de cette compilation.

### 2026-09-26 — Preuve de slashing relayée entre deux démons

`consensus_bellatrix_slashing_network` passe en **5,99 s**. Deux cœurs dans le processus de test et deux vrais démons `nano-p2p`, sur un Mac et avec horloge synthétique : le destinataire reçoit et archive les octets signés avant tout bloc contenant la preuve, exclut le signataire du choix de branche, puis accepte le bloc qui applique le slashing. Le validateur synthétique 35 passe de 32 000 000 000 à 31 000 000 000 unités de solde consensus, avec sortie à l'epoch 5 ; les états EVM concordent.

Preuve et empreintes : `docs/qualification/bellatrix-slashing-network-20260926.json`. Ce scénario clôt uniquement le raccordement effectif du relais de preuves. La tenue sous charge et la reprise après partition avec l'ordonnancement désormais partagé entre blocs, votes et preuves restent à qualifier ; l'archive de preuves ne dispose pas encore d'élagage après finalité. Aucune conclusion de préparation générale à la production.

### 2026-09-26 — Saturation par preuves redondantes

La revue du lot slashing a identifié qu'un signataire pouvait fabriquer plusieurs paires de votes contradictoires pour le même fautif et occuper l'archive. L'admission conserve désormais une nouvelle preuve seulement si l'intersection de ses signataires couvre au moins un fautif non couvert par les preuves archivées. Les exclusions de choix de branche ne remplacent pas ce contrôle : elles peuvent exister sans preuve en attente pour le constructeur de blocs.

Deux nouveaux cas de `consensus_bellatrix_local_slashing` passent (**5,53 s**, avec le scénario de sanction/reprise existant) : autre preuve signée du même fautif et paire signée sans fautif commun. Ni archive ni checkpoint ne croissent dans ces cas. Rapport : `docs/qualification/bellatrix-slashing-dedup-20260926.json`. Le coût de vérification BLS reste présent ; ce résultat ne qualifie pas une résistance générale au spam. L'élagage des preuves après sanction finalisée reste ouvert.

### 2026-09-26 — Reprise avec relais blocs/votes/preuves

Le scénario `consensus_bellatrix_partition_recovery` passe en **517,21 s** avec l'ordonnancement du relais modifié : deux cœurs indépendants, deux démons P2P, 64 clés synthétiques réparties entre deux validateurs sur un Mac. Coupure de 96 s, branches concurrentes, finalité inchangée pendant la coupure, puis tête commune et finalité commune progressant à l'epoch 8 (justification 9). Rapport : `docs/qualification/bellatrix-slashing-partition-20260926.json`.

Cette exécution a commencé avant le correctif de déduplication des preuves : ses empreintes d'entrée sont conservées séparément ; le correctif a ses nouveaux cas ciblés propres. Les 517,21 s dépassent le passage historique de 420,77 s, sans mesure permettant d'attribuer l'écart au seul relais. Ce résultat démontre la reprise bornée avec le nouvel ordonnancement, pas la tenue sous charge ni une disponibilité distribuée en production.

### 2026-09-26 — Élagage des preuves après sanction finalisée

Le réacteur conserve les preuves tant qu'au moins un fautif commun n'est pas sanctionné dans l'état finalisé authentifié. À l'avancement de la finalité, il retire atomiquement les preuves devenues inutiles ; les rejeux n'en recréent pas. La récupération restaure son snapshot exact sans y appliquer un nouvel élagage.

Nouveau scénario `consensus_bellatrix_slashing_pruning` : **40,72 s**, deux cœurs dans un processus, deux démons P2P, horloge synthétique. Il vérifie la conservation avant finalité, l'élagage après, le rejeu sans écriture, une nouvelle preuve reçue après disparition de l'ancien curseur de catalogue et la réouverture du stockage. Rapport : `docs/qualification/bellatrix-slashing-pruning-20260926.json`. Cela ferme le manque d'élagage des preuves signalé dans les notes précédentes ; l'historique complet des blocs/états et la charge prolongée restent hors de ce résultat.

### 2026-09-26 — Sorties volontaires Bellatrix, raccordement local en cours

Admission des sorties signées SSZ de 112 octets, contrôle BLS et ancienneté, déduplication par validateur, archive durable v5 et inclusion par le constructeur local ajoutées. L'admission seule ne modifie pas le registre ; la transition complète du bloc programme la sortie. Le scénario nouveau `consensus_bellatrix_voluntary_exit` passe en **5,50 s** : signature invalide, demande future, ancienneté insuffisante, doublon sans publication et sortie sans slashing. Délai synthétique d'ancienneté : une epoch, explicitement limité au test ; sortie à l'epoch 6, retrait autorisable au plus tôt à 262.

Rapport : `docs/qualification/bellatrix-voluntary-exit-core-20260926.json`. Ce lot reste en cours : transport P2P, reprise des sorties en attente et authentification de leur contexte lors des changements de branche ne sont pas qualifiés. Le versement des retraits n'est pas implémenté par ce parcours Bellatrix ; ne pas assimiler cette étape aux EIP-4895, 7002 ou 7044 activées sur le réseau.

### 2026-09-26 — Sorties volontaires : transport et restauration du contexte

L'archive de sorties conserve désormais la racine du bloc d'admission avec les octets SSZ signés. La restauration vérifie le registre et le domaine de signature à ce bloc authentifié, indépendamment de la tête courante. Inventaire/export RPC authentifiés, transport Bellatrix borné à 112 octets et relais P2P des sorties ajoutés.

`consensus_bellatrix_voluntary_exit_network` passe en **9,54 s** : deux cœurs dans le processus du scénario, deux démons P2P, un hôte, ancienneté synthétique d'une epoch. Réception avant inclusion, calendrier de sortie identique après le bloc, puis réouverture conservant archive, contexte et tête ; doublon sans nouvelle publication. Un premier échec de 28,79 s a révélé une catégorie omise dans le dispatch démon-vers-cœur ; correction et échec sont conservés dans `docs/qualification/bellatrix-voluntary-exit-network-20260926.json`.

Restent avant clôture de ce lot : branches concurrentes, élagage à la finalité, cas négatifs du transport et qualification des composants partagés affectés. Le relais alterne désormais blocs/votes/preuves/sorties ; les preuves de partition précédentes concernent l'ordonnancement antérieur. Pas de versement de retrait ni d'activation d'un fork ultérieur dans ce résultat.

### 2026-09-26 — Sorties volontaires : cycle de demande qualifié et livraison

Le lot couvre admission signée, stockage du contexte d'admission authentifié, transport, inclusion, conservation en cas de réorganisation, puis élagage après programmation finalisée de la sortie. Il ne verse pas de retrait et ne constitue pas le cycle complet des dépôts/activations/retraits du registre ouvert.

Nouveaux résultats :
- `consensus_bellatrix_voluntary_exit_pruning` : **44,13 s**, conservation jusqu'à finalité, élagage, rejeu sans publication, nouvelle sortie relayée après changement de catalogue et réouverture.
- `bellatrix_wire_identity` enrichi : **0,33 s**, sorties de taille exactement 112 octets, rejet des tailles 0/111/113, de l'identité étrangère et du protocole legacy ; enveloppes hex et IPC compact.
- `consensus_bellatrix_voluntary_exit_reorg` : **10,81 s**, deux blocs valides signés par un proposant synthétique équivoque ; après expiration du proposer boost, choix de la branche sans sortie, annulation du calendrier, conservation de la demande et réinclusion ; pair synchronisé et stockage rouvert.
- Régression groupée des composants affectés : **5/5 en 18,68 s** (relais, reçus, client IPC, reprise témoin et slashing local).

Rapports : `docs/qualification/bellatrix-exit-lifecycle-20260926.json`, `bellatrix-exit-reorg-20260926.json` et `bellatrix-exit-shared-20260926.json`. Profils synthétiques sur un seul Mac, ancienneté minimale d'une epoch dans ces scénarios seulement. La réorganisation conserve les mêmes clés de registre : les allocations concurrentes de nouveaux validateurs par dépôts ne sont pas couvertes. La charge et la reprise après partition avec l'ordonnancement à quatre catégories restent à qualifier ; les résultats antérieurs de partition restent ceux de leurs empreintes enregistrées.

### 2026-09-26 — Source des dépôts : replay EVM et identité du contrat

`replay_execution_deposits` rejoue un bloc contre l'état parent vérifié, exige une empreinte de bloc issue d'une autorité indépendante du paquet reçu et une empreinte de code du contrat provenant de la configuration de confiance. Il retourne un état candidat, les reçus et les dépôts ordonnés, sans modifier l'état fourni. Les indices doivent être consécutifs et l'extraction respecte un budget et l'annulation. La fonction utilise actuellement l'adresse du contrat Ethereum ; elle ne doit pas être interprétée comme un client léger authentifiant une autre chaîne.

Nouvelle qualification `consensus_execution_deposit_source` : **35 cas officiels, 680 dépôts, 13 cas sans dépôt, 0,80 s**. Les transactions sont exécutées par l'EVM C++ ; le résultat est comparé au post-état officiel. Pour chaque cas, les vues Solidity `get_deposit_count()` et `get_deposit_root()` concordent avec les données extraites et l'arbre SSZ reconstruit indépendamment ; les montants correspondent à l'augmentation du solde du contrat. Les refus couvrent mauvaises empreintes bloc/code, état parent incohérent, engagement de reçus altéré, indice incorrect, capacité épuisée et annulation. Les cas officiels incluent appels directs/imbriqués, revert, out-of-gas et appels sans dépôt effectif.

Référence : [contrat de dépôt Phase0](https://ethereum.github.io/consensus-specs/specs/phase0/deposit-contract/). Preuve : [`qualification/execution-deposit-source-20260926.json`](qualification/execution-deposit-source-20260926.json). **Statut réseau inchangé : raccordement partiel.** Ce helper n'est pas encore appelé par le runtime Bellatrix. Restent l'historique de dépôts lié aux branches, les preuves Merkle, le vote Eth1 et la consommation par le registre avec PoP, puis activation et restitution. Le profil Prague des fixtures n'active pas Prague dans le réseau Bellatrix ; aucun versement ou dépôt réel n'est effectué.


### 2026-09-26 — Historique de dépôts par branche et preuves vers le registre

`ExecutionDepositHistory` ajoute un arbre immuable de profondeur 32, des preuves Phase0 avec mélange du compteur et une ancre de bloc d'exécution. L'ajout partage les sous-arbres inchangés et copie le chemin de la feuille ; une branche n'avance jamais le compteur d'une autre. Les vues exécutées du contrat (`get_deposit_count`, `get_deposit_root`) doivent correspondre à l'historique avant et après ajout. Les snapshots se reconstruisent seulement contre un état EVM et un bloc authentifiés indépendamment, avec identité de code et capacité explicite ; les données du snapshot ne sont pas leur propre preuve de confiance.

La campagne finale passe **2/2 en 1,13 s**. Nouvelle couverture : toutes les **680 preuves des 35 cas officiels** vérifiées, puis un parcours Paris utilisant de vraies transactions signées et le contrat Solidity : dépôt financé avec PoP BLS vers `process_phase0_deposit`, complément conservant les coordonnées de retrait, nouvelle clé sans PoP ignorée avec consommation d'indice, branches concurrentes et preuves croisées refusées, rejeu sans double crédit, conservation des arbres précédents, bloc sans dépôt et restauration JSON. Modification/réordonnancement des feuilles, ancre étrangère et dépassement de capacité sont refusés.

Preuve : [`qualification/execution-deposit-history-20260926.json`](qualification/execution-deposit-history-20260926.json). Les rapports précédents restent inchangés. **Le raccordement runtime Bellatrix, les votes Eth1, la persistance atomique avec le checkpoint réseau, l'activation et le versement des retraits restent ouverts.** La restauration qualifiée ici est celle du snapshot de l'historique, pas une reprise après crash RocksDB. Ce lot ne clôt pas EIP-6110 réseau et ne mesure pas encore 100 000 ou 1 000 000 validateurs.


### 2026-09-26 — Dépôts raccordés au Store Bellatrix et au checkpoint EVM

Le profil Bellatrix peut désormais inclure l'empreinte du code du contrat de dépôt. Cette extension participe à l'identité du réseau ; le profil et les snapshots historiques restent encodés à l'identique quand elle est absente. Chaque bloc accepté porte son historique immuable de dépôts, construit à partir des reçus déjà vérifiés par la transition complète, sans second replay des transactions. L'ancre exige un historique cohérent avec le contrat (vide par défaut, historique authentifié fourni explicitement pour une ancre plus récente).

L'historique figure dans le snapshot existant du Store, publié par le coordinateur couplé consensus/exécution. La restauration vérifie les données contre l'état EVM et les vues du contrat. **Campagne finale 4/4 en 5,79 s** : nouveau scénario `consensus_bellatrix_deposit_store` (0,80 s), régression de la chaîne signée sans dépôt, sources officielles et historiques/proofs. Le nouveau scénario exécute deux dépôts, accepte une branche concurrente sans le second dépôt, contrôle les compteurs propres aux branches, refuse une altération du snapshot malgré son checksum recalculé et le chargement sous profil historique, puis ferme et rouvre réellement `Blockchain` et le réacteur avec le même checkpoint. Un échec initial de nettoyage provenait de l'ordre de destruction des objets du test ; corrigé, conservé dans les preuves.

Rapport : [`qualification/bellatrix-deposit-store-20260926.json`](qualification/bellatrix-deposit-store-20260926.json). **Restent la sélection/vote Eth1, la fourniture des dépôts obligatoires au proposant local et l'admission/activation automatique du registre.** La réouverture testée est ordonnée : aucune nouvelle preuve de SIGKILL avec dépôts, de réseau multi-hôte ou de charge n'est revendiquée. Les snapshots complets et les historiques répétés dans les enregistrements persistent comme coût de passage à l'échelle. Aucun déploiement ni fonds réel.


### 2026-09-26 — Vote natif et admission automatique par le proposant Bellatrix

L'extension de profil explicite `nano-native-eth1-v1` active la sélection d'une source d'exécution stable pour chaque période de vote : dernier ancêtre précédant le début de période, avec repli sur l'ancre authentifiée. Le nœud refuse tout vote natif ne correspondant pas à l'historique de dépôts d'un ancêtre de son propre parent, avant signature locale et lors de l'admission du bloc. **Cette politique Nano ne reproduit pas la sélection temporelle des blocs Eth1 externes d'Ethereum et ne constitue pas EIP-6110.** Les profils sans dépôts et ceux de suivi seul gardent leurs identités précédentes.

Le constructeur anticipe exactement la majorité stricte du traitement Eth1, réserve le budget des opérations obligatoires et inclut les preuves Merkle ordonnées avant les sorties volontaires. La transition complète conserve le contrôle de possession BLS avant admission d'une nouvelle clé. Aucun crédit de stake n'est effectué par un simple message de pair.

La nouvelle qualification `consensus_bellatrix_native_deposit_admission` passe en **12,42 s** : transaction signée déposée dans la mempool, exécution du contrat, vote puis admission du 65e validateur au slot 12 (période synthétique de huit slots, cinq votes nécessaires), éligibilité à l'activation après traitement d'époque et restauration du registre/curseur après fermeture/réouverture de la base. Racine de dépôt fabriquée refusée avant signature et à réception d'un bloc signé ; preuve obligatoire falsifiée refusée sans appel au signataire ; rejeu du bloc d'admission sans nouvelle publication. Campagne finale **4/4 en 22,04 s**, incluant les propositions historiques, sorties volontaires et stockage des dépôts.

Rapport : [`qualification/bellatrix-native-deposit-20260926.json`](qualification/bellatrix-native-deposit-20260926.json). Restent l'activation effective avec participation de la nouvelle clé, les allocations de registre concurrentes et sorties en attente, le réseau multi-pairs sous ce profil, les pannes et les retraits. Le validateur nouveau n'est pas actif en fin de scénario ; ni EIP-6110 réseau, ni préparation globale à la production ne sont déclarés terminés.


### 2026-09-26 — Activation du validateur déposé et première attestation

`consensus_bellatrix_deposit_activation` prolonge le parcours de dépôt jusqu'à la participation réelle, avec les délais d'activation du profil conservés. Le 65e validateur est éligible après dépôt, reçoit une activation à l'**époque 8**, puis signe une attestation admise et présente dans les messages du choix de branche. Aucun appel au signataire de cette clé n'a lieu avant son activation. La finalité progresse ensuite à l'**époque 7**, après l'entrée en activité. La fermeture et réouverture du nœud conservent le registre, le curseur de dépôt et l'état EVM.

Résultat : **réussi en 139,72 s**, une signature de la nouvelle clé. Les traitements d'activation existants n'ont pas nécessité de modification pour ce scénario. Une observation ponctuelle du processus à 136 s indique 402 080 Kio de RSS et 98,1 % CPU ; ce n'est ni un pic ni un benchmark de capacité. Rapport : [`qualification/bellatrix-deposit-activation-20260926.json`](qualification/bellatrix-deposit-activation-20260926.json).

Limites : un Mac, horloge synthétique, période de vote Eth1 d'une époque, clés du futur validateur préconfigurées directement dans le scheduler. L'entrée via configuration CLI depuis une autre machine et la proposition de bloc par la nouvelle clé restent à qualifier. Les budgets de rétention du test sont augmentés pour ce parcours borné ; cela ne résout pas l'élagage du Store. Restent notamment les allocations concurrentes de nouveaux indices avec sorties en attente, les retraits, le réseau et la charge de production.


### 2026-09-26 — Identité BLS des sorties archivées

Les demandes en attente sont maintenant identifiées par **(indice, clé publique BLS)**. La clé vient du contexte d'admission vérifié ; elle est reconstruite à la restauration, sans accepter un champ de clé supplémentaire depuis un pair ou le snapshot. Déduplication, unicité à la restauration et sélection du proposant utilisent cette identité. Une demande liée à une autre clé est ignorée avant consommation du budget d'essais de proposition. L'élagage conserve les demandes avant finalité ; une sortie correspondante finalisée ou l'attribution finalisée du même indice à une autre clé rend l'ancienne demande obsolète.

La lecture du traitement d'activation confirme que l'éligibilité doit être finalisée avant programmation de l'activation. **Une réorganisation ordinaire changeant la clé d'un validateur déjà actif n'a donc pas été établie comme attaque exploitable.** Le changement explicite l'identité au lieu de dépendre uniquement de cet invariant. Le nouveau contre-exemple utilise deux signatures BLS valides pour des clés différentes au même indice dans des contextes synthétiques ; il ne prétend pas créer deux histoires finalisées admissibles.

Qualification **5/5 en 69,73 s** : nouveau contrôle d'identité, sortie volontaire existante avec cache de clé vérifié, réorganisation/réinclusion, élagage après finalité et parcours de dépôt enrichi d'un refus de sortie signé par une clé non encore activée. Les scénarios réseau existants gardent les mêmes clés entre branches. Rapport : [`qualification/bellatrix-exit-identity-20260926.json`](qualification/bellatrix-exit-identity-20260926.json). Aucun changement du format réseau des sorties, aucun versement de retrait et aucune clôture de préparation générale à la production.


## 2026-09-26 — libsecp256k1 et candidats de retraits Capella

`src/crypto.cpp` délègue désormais dérivation de clé publique, signature RFC6979
low-s et récupération ECDSA à `libsecp256k1` v0.7.1 (source officielle fournie,
SHA-256 épinglé dans `dependencies.lock.json`, licence MIT conservée). Le contexte
est randomisé avant son partage en lecture. Le calcul de signature sur les secrets
avec GMP/OpenSSL est retiré. OpenSSL demeure nécessaire pour TLS, aléa et keystores ;
BLS conserve `blst`. CMake lie la dépendance statiquement ; le Makefile la construit
aussi. Le Dockerfile inclut déjà ces sources via `COPY . .` et sa configuration
CMake ; aucune bibliothèque dynamique supplémentaire n'est nécessaire.

Qualification : 18 vecteurs indépendants ethers 6.16.0, y compris les condensés
supérieurs ou égaux à l'ordre de la courbe ; récupération high-s préservée pour
ECRECOVER, entrées invalides et huit utilisateurs concurrents du contexte.
Les suites core (dont les précompiles et signatures EIP-712), chain, RPC,
authorization, transactions Ethereum et retraits passent : **8/8 en 9,82 s** avec
les deux nouvelles suites. Aucun fork ni statut global de conformité n'est promu.
Docker/VPS non reconstruits ou redéployés sur ce lot. Signature GPG du tag amont
non vérifiée indépendamment ; le hash de l'archive téléchargée est consigné.

Pour EIP-4895/Capella, `consensus_capella_withdrawals` calcule la liste autorisée
(retraits complets/partiels, ordre, adresses, montants, curseurs), débite un candidat
de registre et vérifie le crédit Shanghai dans un candidat EVM. Liste altérée,
double débit, dépassement de curseur, annulation et échec tardif de racine sont
couverts sans mutation des états parents. **Partiel** : la transition SSZ Capella,
l'admission réseau authentifiée, la persistance atomique des curseurs et le passage
du profil Bellatrix/Paris au profil Capella/Shanghai restent à intégrer. Ce module
ne suffit pas à activer les retraits du réseau ; aucun vecteur officiel de consensus
Capella n'est revendiqué dans cette campagne.

Preuves : [rapport](qualification/secp256k1-capella-20260926.json),
`tests/test_secp256k1.cpp`, `tests/fixtures/secp256k1-ethers.json`,
`tests/test_capella_withdrawals.cpp`.
Sources : https://github.com/bitcoin-core/secp256k1/tree/v0.7.1 et
https://ethereum.github.io/consensus-specs/specs/capella/beacon-chain/.


## 2026-09-26 — campagne officielle EEST consume direct

L'outil officiel EEST pilote désormais les runners C++ Nano via un consumer dédié,
sans se présenter comme Geth et sans utiliser un autre moteur. Corpus stable
v5.4.0 épinglé, archive vérifiée contre le digest GitHub publié. Résultats :

| Sélection | Réussis | Échecs / non pris en charge |
| --- | ---: | ---: |
| StateTest Paris | 1 564 | 0 |
| StateTest Shanghai | 1 745 | 0 |
| StateTest Cancun | 16 847 | 0 |
| BlockchainTest Paris | 1 624 | 0 |

Les deux campagnes vont au terme de leur sélection, sans atteindre les seuils
maxfail. Durées pytest : 431,29 s pour les états (deux workers), 171,57 s pour les
blocs (un worker). Quatre contrôles du consumer exigent le rejet d'une racine
fausse, d'un cas absent et d'un fork non implémenté, ainsi que le succès du témoin.
Les verdicts individuels complets sont conservés dans les rapports JSON compressés.

Ce résultat qualifie l'exécution sur ces fixtures, pas l'ensemble du corpus, tous
les forks, toutes les règles d'en-tête ni le consensus réseau. Aucun statut
« terminé pour production » n'en découle. Sur le périmètre observé, aucune
nouvelle divergence EVM n'impose un changement de moteur ; la priorité reste
l'intégration Capella/retraits et du consensus, la rétention du stockage et la
qualification distribuée. Les campagnes des autres profils restent ouvertes.

[Protocole reproductible](EEST.md) ;
[rapport et empreintes](qualification/eest-v5.4.0-20260926.json).


### Limite opérationnelle transverse — allocation RocksDB (2026-09-26)

Le constructeur conserve WAL et `sync=true`, mais désactive la préallocation
`allow_fallocate`. Un comparatif de 66 petites bases reproduit 5,15 Go alloués
avec les défauts contre 2,97 Mo sans préallocation ; les 198 valeurs des trois
variantes survivent à la réouverture. Les tests du journal de signature et des
témoins de récupération passent. Voir
[rapport](qualification/rocksdb-allocation-20260926/README.md).

Cette optimisation n'est ni un pruning de l'état EVM, ni une nouvelle preuve de
conformité EIP. La reprise RPC (~3 s, avec attente volontaire du test) doit rester
distincte de la finalité après crash. Les règles de rétention et l'endurance
restent des critères de clôture séparés.

### Observation Beacon native (2026-09-26, partielle)

Trois lectures HTTP `states/head/{root,fork,finality_checkpoints}` sont raccordées
au runtime Bellatrix avec snapshots validés, file de calcul bornée et accès
loopback. Nouveau test HTTP natif passé, redémarrage runtime passé ; voir
[preuves](qualification/beacon-reads-20260926/README.md) et
[périmètre restant Assertoor](ASSERTOOR-INTEGRATION.md).
Aucune EIP supplémentaire qualifiée par ces lectures ; Assertoor n'a pas encore
été exécuté et les routes genesis/spec/syncing/headers/blocks/events restent ouvertes.

- Extension d'observation du 2026-09-26 : en-têtes Beacon signés par tête/racine/slot
  raccordés aux archives natives. Tests de corruption BLS/SSZ et de cohérence après
  deux changements de tête passés ; [preuves](qualification/beacon-headers-20260926/README.md).
  Ce résultat ne qualifie aucune EIP supplémentaire et ne clôt pas Assertoor.

- Observation Beacon, suite du 2026-09-26 : genesis/version et paramètres issus
  du profil authentifié servis en HTTP natif et testés. La route spec reste
  partielle ; aucun historique Ethereum inventé. Le SDK exact d'Assertoor exige
  NodeSyncing dès son initialisation : télémétrie native encore à raccorder.
  [Preuves et prérequis](qualification/beacon-metadata-20260926/README.md).
  Aucun changement d'état de qualification des EIP ni activation réseau.

- Observation Beacon, suite du 2026-09-26 : NodeSyncing raccordé aux slots annoncés
  par les pairs Bellatrix compatibles via nano-p2p et rapport interne authentifié.
  Tests HTTP (retard, absence, expiration, annonce future) et deux processus avec
  redémarrage passés. [Preuves](qualification/beacon-sync-20260926/README.md).
  Ce statut d'observation n'est ni une preuve de finalité ni une qualification
  Assertoor ; aucune EIP supplémentaire déclarée conforme.

- Observation Beacon, suite du 2026-09-26 : blocs signés Bellatrix exposés en
  JSON/SSZ. SDK exact d'Assertoor exécuté contre Nano ; octets réencodés identiques.
  Huit corps officiels avec opérations non vides passent aussi le round-trip
  externe. [Preuves et limites](qualification/beacon-blocks-20260926/README.md).
  Cela qualifie ces formats, pas une nouvelle EIP ni la suite Assertoor complète.

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

## Historique : échec initial de finalité avec Assertoor — 2026-09-26

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

- 2026-09-26 — Qualification de reprise Bellatrix : le nouveau témoin natif
  à livraison directe passe (six avancées 6→12, 122,04 s), mais le scénario
  P2P de partition prolongée reste en échec. Cette différence ne change aucun
  statut EIP en conformité complète ; elle localise la prochaine investigation
  dans l’intégration temps réel et le rattrapage des messages.
  [Résultats et périmètres](qualification/recovery-sustained-20260926/README.md).

### Reprise réseau : état courant après corrections — 2026-09-26

Les échecs ci-dessus restent conservés comme historique. Le relais saute désormais
les votes déjà acquittés avec un budget borné ; le moniteur vérifie la cohérence
des lectures et lie la finalité Beacon réalisée à l’exécution finalisée.

| Scénario | Preuve actuelle | Limite |
| --- | --- | --- |
| Partition P2P de 96 s | Réussite 517,99 s ; reprise 95 s puis trois avancées communes 6→7→8→9 | Deux cœurs, deux P2P, 64 clés synthétiques, un Mac |
| SIGKILL puis redémarrage | Réussite 325,58 s ; trois avancées communes 2→3→4→5 pendant 144,793 s | Arrêt de processus, pas panne électrique |

[Partition](qualification/recovery-sustained-20260926/recovery-bound/README.md) et
[crash](qualification/recovery-sustained-20260926/crash-current/README.md) :
logs, observations et empreintes conservés. Ces résultats ferment ces scénarios
locaux bornés ; aucune EIP supplémentaire ne devient conforme par déduction.
La rétention des états historiques, l’endurance, le multi-hôte et le grand
registre restent ouverts. Le profil réseau reste Bellatrix ; les profils EVM
plus récents du banc ne constituent pas leur activation réseau.

- 2026-09-26 — Primitive d’archive atomique de checkpoint implémentée et testée :
  deux cibles passent en 1,49 s, avec réorganisation, SIGKILL et suffixe WAL
  conservé/perdu. Raccordement des nouveaux blocs signés Bellatrix réalisé :
  neuf archives identiques avant/après réouverture dans le scénario signé.
  **Migration des anciens snapshots, lecture P2P depuis cette archive et
  élagage non réalisés.**
  [Preuves](qualification/checkpoint-archive-20260926/README.md).

## Qualification multi-hôte — 26 septembre 2026

Cœur b7e2748 : Mac natif, Docker ARM64 Mac et Docker AMD64 VPS, deux hôtes
physiques et 64 clés synthétiques 22/21/21. Une fenêtre instrumentée réussit
la finalité commune 2 → 3 → 4 → 5 en 199002 ms. La campagne précédente
échoue au critère de continuité avec 2 → 3 → 4 → 6 ; accord final conservé.
Le retard intermittent d’inclusion reste ouvert. Aucune qualification
d’endurance ou de production : voir [preuves multi-hôte](qualification/multihost-20260926/README.md).

- 2026-09-26, révision 42a3b13 : finalité 2→3→4→5 en 135047 ms,
  mais validateur Docker Mac arrêté par expiration de proposition. Le nœud
  continuait à suivre la chaîne : **qualification globale en échec** malgré
  le code 0 de l'ancien lanceur. Un contrôle des trois statuts validateurs
  rejette maintenant cette campagne et accepte le témoin précédent.
  [Preuves et prochain correctif](qualification/multihost-20260926/with-batch/README.md).

- 2026-09-26 — Dimensionnement de position Uniswap V3 : simulateur entier Python
  comparé au mint des bytecodes officiels sur Nano Paris, deux ordres des actifs.
  Montants, liquidité et tick vérifiés ; aucune qualification du bridge ou de la
  vente publique. [Preuves](qualification/graduation-v3-20260926/README.md).

- 2026-09-26 — Position V3 full range : mint à 100 unités synthétiques puis
  achat/revente du montant reçu sur Nano Paris. Position active conservée,
  soldes et variation de prix vérifiés ; aucun actif réel ni mesure de capacité
  réseau. [Résultats](qualification/graduation-v3-20260926/full-range/README.md).

## Qualification multi-hôte après correction des propositions expirées

Révision b1ccbac : finalité commune 2→3→4→5 en 145763 ms, trois
validateurs sains et actifs (136/133/128 attestations locales admises).
Mac natif + Docker ARM64 + VPS AMD64, deux machines, 64 clés synthétiques.
La campagne précédente avec validateur arrêté reste classée en échec.
Les partitions WAN, l’endurance et le grand registre restent ouverts ; aucun
statut EIP ou production global ne découle de cette fenêtre. [Preuves](qualification/multihost-20260926/with-expiry/README.md).

- Partition WAN du 26 septembre, cœur b1ccbac : **échec de reprise** dans les
  180 s après reconnexion d'une coupure de 96 s. Trois validateurs actifs,
  198/187/185 attestations admises, finalité restée à 2 ; justification 3/3/9
  à l'arrêt. Retard de convergence à diagnostiquer, pas une panne du fournisseur
  de signature observée. [Preuves](qualification/multihost-20260926/wan-partition/README.md).

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

### Reprise WAN horodatée et correction du parcours d’inventaire

La campagne `80493b7`, banc `398148d`, reproduit le dépassement du délai :
**187542 ms contre 180 s**, puis finalité commune **8→11 en 141066 ms**.
Trois boucles saines ; l’audit de 209 observations ne relève ni conflit par
epoch ni régression. Les traces corrèlent plusieurs votes inclus tard à
17,6–18,2 s entre relais successifs, avec limites d’horloge documentées.
[Campagne et périmètre](qualification/multihost-20260927/timed-recovery/README.md).

La correction `a4bf5e6` conserve une page bornée par pair après accusé et
réduit de 32 à 1 les requêtes d’inventaire pour 32 votes inconnus dans le
nouveau test. Exports et accusés individuels restent obligatoires ; perte
d’accusé, payload indisponible et catalogue renouvelé sont couverts.
L’ancien code échoue au contrôle négatif ; le daemon corrigé compile.
[Preuves locales](qualification/relay-page-cache-20260927/README.md).
Effet WAN encore à mesurer : aucun EIP ni critère de production clôturé.

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
