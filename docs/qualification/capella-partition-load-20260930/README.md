# Charge applicative pendant partition — 30 septembre 2026

## Charge pendant partition WAN qualifiée — 30 septembre 2026

La tentative C sur `22b4b65` réussit : 96 transactions admises pendant une coupure
réelle de 97,882 s, finalité commune reprise en 136,834 s ; reçus finalisés et
états identiques vérifiés en 350,273 s, puis époques communes 9 à 12 en 124,230 s.
Les trois validateurs terminent sains, les deux conteneurs sortent à zéro sans OOM.
Les 33 mesures par nœud sont complètes. Preuves et vérification hors ligne :
`attempt-c/verified-result.json` et `attempt-c/manifest.json`.
Cette charge est bornée (SSTORE constant), sur trois instances et deux machines ;
aucune capacité maximale ni endurance prolongée n'est démontrée.
L'arrêt du réacteur de la tentative A reste inexpliqué ; la production globale reste ouverte.

Le nonce final vaut 97, le solde de la cible 96 wei et son stockage 42 sur
les trois nœuds. L'ancre commune des reçus est le bloc 50. Les maxima observés
sont 405,58 MiB / 68,6 % CPU pour le processus natif, 369,9 MiB / 59,9 % pour
Docker Mac et 406,9 MiB / 100,48 % pour le VPS. Ce sont des échantillons,
non des maxima continus ; le RSS natif exclut son enfant P2P. BlockIO mesure
des écritures cumulées, pas l'occupation disque. Un premier appel au résumeur
hors ligne a reçu un chemin de fichier au lieu du dossier attendu (ENOTDIR) ;
l'appel corrigé a produit le rapport, sans relancer la campagne.

## Tentative B : reprise du consensus, blocage des transactions

Le rejeu sur `406227f` (`attempt-b/`) échoue également, pour une raison différente
et observable. Les 96 transactions ont toutes été soumises pendant une coupure
de 97,607 s. La finalité commune reprend à l'époque 7 en 129,867 s après
reconnexion, mais aucun des 96 reçus n'apparaît. Une capture RPC sur les trois
nœuds montre 96 transactions `pending`, le nonce canonique inchangé à 1 et des
têtes de chaîne aux hauteurs 99/100/100. Le lot est déclaré en échec à 617,977 s
(limite de 600 s vérifiée après une collecte). Les trois validateurs restent sains
à l'arrêt, les deux conteneurs sortent à 0 sans OOM. Cela ne transforme pas
l'échec applicatif en réussite.

La sélection Capella prenait les 16 premiers candidats classés par frais et
ordre d'arrivée, avant la résolution des dépendances de nonce. Un préfixe de
transactions futures pouvait ainsi exclure le nonce initial après réorganisation
ou réception hors ordre. Le test ciblé reproduit ce contre-exemple avec 96
transactions, dont le nonce 0 reçu en dernier.

Le correctif ajoute une sélection fondée sur le nonce canonique de chaque compte :
seule sa première transaction exécutable concurrence les autres comptes par frais,
puis les nonces contigus deviennent éligibles. Le budget reste à 16, les trous de
nonce et les transactions dépassant le gaz disponible ne sont pas contournés.
L'exécution EVM continue de valider les transactions sélectionnées.

Qualification native (`nonce-ready-native/`) : test du mempool réussi en 0,36 s,
avec réintégration d'orphelines, trous de nonce, priorité entre comptes, gaz et
annulation sans mutation. Le test Capella signé reçoit 32 transactions dans
l'ordre inverse, les inclut par lots de 16 et vérifie leur finalisation : réussite
en 34,44 s, avec reprise et journaux de signature. Un premier échec de ce test
(35,07 s) est conservé : son assertion finale supposait encore une transaction
par bloc. Le comptage est maintenant fondé sur les transactions réellement admises.

Qualification Linux du cœur `22b4b65` réussie (`nonce-ready-linux/`) :
mempool et scénario Capella passent sur ARM64 (0,01 / 25,57 s) et VPS x86-64
(0,03 / 97,33 s). Les conteneurs sortent à zéro sans OOM ; journaux, états,
révision et empreintes des binaires sont archivés. La tentative WAN C est réussie (voir ci-dessus).

**Le correctif de sélection est qualifié par la tentative C dans ce périmètre. L'arrêt du réacteur en tentative A reste une anomalie distincte, non expliquée
et non reproduite en B. La production globale demeure ouverte.**

## Tentative A : arrêt du réacteur

**Échec conservé, qualification ouverte.** La campagne `attempt-a` a accepté
64 transactions sur les 96 prévues avant de s'arrêter. Le nœud Docker ARM64 du
Mac est sorti à 1, sans OOM, avec :

```
Nano: Capella reactor unavailable; reopen required
```

La coupure prévue pour 96 s a été interrompue à 49,570 s après les erreurs des
deux branches d'observation/injection. La reconnexion Docker a bien été effectuée
avant l'arrêt des autres instances. Leurs validateurs étaient sains ; Docker Mac
n'a pas produit de reçu terminal de validation. Aucun succès de finalité après
charge n'est revendiqué, et les transactions non soumises n'ont pas été rejouées.

Le VPS est le relais commun de cette topologie : sa déconnexion laisse les trois
nœuds avec zéro pair. Cette campagne n'est donc pas une partition entre un groupe
de deux Mac connectés et un VPS isolé. Les checkpoints observés pendant la coupure
restent à l'époque 2, même racine. Les bases de l'échec sont conservées.

## Diagnostic et suite

Le cœur testé était `940736fd2078eb058289434cfefa2c2147df690d`, identique au test
de charge sans panne réussi. Son réacteur ne conservait pas l'exception initiale
ayant déclenché sa fermeture. Une faute d'horloge, de publication ou de stockage
ne peut donc pas être distinguée à partir du seul message final. Le logger JS
ne détaillait pas non plus les exceptions imbriquées dans `AggregateError`.

Le correctif de diagnostic conserve la première exception du réacteur et ajoute
sa cause aux erreurs suivantes d'indisponibilité. Il maintient la fermeture des
accès et n'assouplit aucun quorum, délai ou règle de consensus. Les erreurs
individuelles de l'observateur et de l'injecteur sont désormais archivées avant
la restauration du réseau. **Ce correctif ne résout pas encore la cause de l'arrêt.**

Le test d'horloge existant est étendu pour vérifier que la cause initiale survit
à des lectures refusées et à une nouvelle invalidation. Le build natif passe ;
le scénario d’horloge passe en 33,57 s et la régression de panne d’écriture du
réacteur en 3,93 s (`diagnostic-native/`). Les 23 tests JS d'orchestration passent,
dont quatre nouveaux cas de concurrence/restauration. Un vérificateur des preuves
de la campagne ajoute 15 contrôles contre les soumissions hors coupure, preuves
manquantes, état divergent, reprise tardive et identités de validateurs dupliquées.

Les mêmes tests passent sur ARM64 (2,96 s et 24,46 s) et VPS x86-64
(12,61 s et 94,80 s), à la révision `406227f` (`diagnostic-linux/`). Les deux
conteneurs de build sortent à 0 sans OOM. Le rejeu borné est lancé dans
`/tmp/nano-partition-load-20260930-b` ; son résultat est détaillé ci-dessus et ne
remplace pas l’échec initial. La cause de l'arrêt en A reste non établie.

## Preuves

`attempt-a/` contient les logs, les 64 acceptations, les transactions synthétiques
préparées, les états Docker finaux, les 12 échantillons de ressources par instance
et leurs empreintes. Les mesures précédant l'arrêt ne représentent pas un pic
exact. Les erreurs « warming » du démarrage sont également conservées.

Commande d'origine :

```sh
NANO_TEST_CONSENSUS_PROFILE=capella NANO_TEST_BUILD_VOLUME=1 \
NANO_TEST_BOUNDED_LOAD=1 NANO_TEST_WAN_PARTITION=1 \
node tests/multihost/run.mjs ubuntu@145.239.199.166 \
  940736fd2078eb058289434cfefa2c2147df690d /tmp/nano-partition-load-20260930-a
```

Le prochain rejeu devra utiliser une nouvelle sortie et la révision des binaires
diagnostiques, sans modifier les fichiers ni volumes de cette tentative.

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
Preuves : [forensique A](attempt-a-forensics/manifest.json).
L'arrêt A reste non expliqué ; aucun résultat de production n'est déduit de cette recherche.
