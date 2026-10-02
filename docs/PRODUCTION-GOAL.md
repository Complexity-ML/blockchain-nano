# Objectif : Nano C++ de niveau production

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
