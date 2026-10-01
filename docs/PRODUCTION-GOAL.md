# Objectif : Nano C++ de niveau production

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
