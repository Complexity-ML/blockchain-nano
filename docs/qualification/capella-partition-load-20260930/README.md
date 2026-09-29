# Charge applicative pendant partition — 30 septembre 2026

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

Étape suivante : qualifier le diagnostic natif, construire les mêmes sources
sur les deux Linux avec les images/volumes existants, puis rejouer le scénario
borné pour établir la cause. Ne pas remplacer le résultat initial par ce rejeu.

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
