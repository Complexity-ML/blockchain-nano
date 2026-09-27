# Bellatrix partition : drainage des traces pendant la reprise

## Échec conservé

La campagne de 188 tests sur `af7f99e59504709f8ae3e3b5ed8c4951e46f48d0`
reste en cours. Son scénario `consensus_bellatrix_partition_recovery` échoue
après 516,39 s : aucune finalité commune avançante dans les cinq époques de
reprise autorisées. Les deux nœuds terminent avec finalité 2 et justification 6.
L’extrait original complet est conservé dans `partition-original-failure.log`.
Les binaires du scénario et des nœuds restent identiques au manifeste initial
(`binary-identity.json`). Le correctif source n’est pas dans cette campagne.

## Défaut du banc d’essai

`latest()` drainait les sorties des deux processus. Or le prédicat de reprise
court-circuite avant `latest()` tant que la finalité n’avance pas. Les sorties
ne sont donc plus drainées pendant cette attente. Une trace bloquée peut alors
empêcher la progression nécessaire pour que le prédicat reprenne le drainage.
Le correctif appelle `pump(0)` sur les deux processus avant toute lecture de
finalité. Aucun seuil, délai ou critère de consensus n’est modifié.

## Nouveau contrôle

`tests/test_process_backpressure.cpp` utilise le véritable wrapper `Process`.
Deux enfants écrivent chacun 1 Mio, l’un sur stdout, l’autre sur stderr, puis
créent un marqueur de progression. Sans drainage ils restent vivants mais
n’atteignent pas le marqueur. Avec drainage inconditionnel, ils terminent et
les deux traces sont récupérées intégralement. Le contrôle passe.
Compilation isolée pour ne pas régénérer le build de la campagne active :

```sh
/usr/bin/c++ -std=c++20 -O2 -Iinclude -Ithird_party -isystem /opt/homebrew/include tests/test_process_backpressure.cpp -o /private/tmp/nano-process-backpressure-tests
/private/tmp/nano-process-backpressure-tests
```

CMake enregistre également `process_trace_backpressure` (délai 20 s).

## Limites et suite

Ce contrôle établit le mécanisme de blocage des pipes, pas sa responsabilité
exclusive dans l’échec réseau. Rejouer le scénario réel avec le correctif après
la campagne active ; conserver tous les autres échecs. Ne pas déclarer la
reprise qualifiée avant finalité commune avançante dans le délai inchangé.
Aucune EIP ni qualification production clôturée ; aucun déploiement.

## Périmètre de la régression en cours

Le filtre CTest `^(ethereum_official_|consensus_)` sélectionne **188 des 389**
tests enregistrés avant le correctif. Les **201 autres** ne sont pas exécutés
par cette commande : cela ne signifie ni échec ni absence de preuves historiques.
La liste exhaustive figure dans `campaign-scope.json`. En particulier, plusieurs
primitives nommées `ethereum_consensus_*`, les checkpoints de stockage et les
tests RPC/applicatifs sont hors de ce filtre. Ne pas présenter cette campagne
comme une qualification complète des 389 tests ou de la production.

Le candidat corrigé a été compilé séparément dans
`/private/tmp/nano-partition-drain-candidate`, sans remplacer les binaires de
la campagne. Son rejeu est en attente de la fin du CTest initial, après
vérification des empreintes des binaires et de la présence des 188 résultats.
Le résultat de ce rejeu reste à recueillir.

## Second contre-exemple de la campagne initiale

`consensus_bellatrix_partition_sustained` échoue après 517,35 s sur le même
prédicat de reprise, avant de démarrer le contrôle de stabilité. Les deux
nœuds ont finalité 2 et justification 6 à l’arrêt ; les horloges sont saines.
L’extrait complet est dans `partition-sustained-original-failure.log`. Le
snapshot synthétique de diagnostic a été conservé localement sous
`/var/folders/_s/kmk9hhlj4vs2jgpcvt6px6nr0000gn/T/nano-assertoor-core-failure-cBpcCI/snapshot`.
Ce résultat reste un échec ; le même correctif de drainage doit être validé
aussi par la variante prolongée.

## Enregistrement CMake vérifié

Après la fin de la campagne initiale, CMake a été régénéré et seule la nouvelle
cible `nano-process-backpressure-tests` a été construite. Le test CTest
`process_trace_backpressure` passe en 0,73 s. Les journaux de configuration,
compilation et le résultat JUnit sont archivés ici. Les empreintes de `nano`,
`nano-p2p` et du test Bellatrix initial sont toujours identiques au manifeste.

## Rejeu réel corrigé : réussi

Après la campagne initiale, le candidat isolé termine avec code 0. Les binaires
`nano` et `nano-p2p` sont ceux du manifeste initial ; seul le scénario compilé
avec drainage inconditionnel remplace l’ancien harness. Les commandes et son
empreinte sont conservées. Aucun délai ni seuil n’a changé.

Après 96 secondes de coupure demandées, les deux nœuds convergent en **95 s**
vers la finalité d’époque **6**, racine
`0x7935894dfade5ffaa70ee76bcf0323298a25f7c7b1a2d39d7fe14fbee09a652d`.
Le bloc d’exécution finalisé commun est
`0xad343683a4e785259cccedc175d0309094db5b40750899a01619bcfeebcf7283`.
À l’arrêt, les deux vues restent identiques et justifiées à l’époque 7.

Le contre-exemple de partition simple est couvert par ce rejeu local. La
variante prolongée reste à vérifier ; ce succès ne clôture ni le réseau WAN
ni le consensus global ni la production. L’échec initial reste archivé.

## Contrôle de contre-pression renforcé à la revue

Le test ne s’appuie plus sur 100 ms d’attente : chaque enfant remplit son tube
en mode non bloquant jusqu’à `EAGAIN`, écrit un marqueur de tube plein, puis
reprend l’écriture bloquante. Le parent exige les deux marqueurs avant de
drainer. Les deux enfants terminent ensuite et toutes les données sont
récupérées. Le test CTest passe en 0,77 s ; les artefacts `process-eagain-*`
conservent ce contrôle renforcé et l’empreinte du source.

## Variante prolongée corrigée : réussie

Le rejeu complet passe en **565,85 s** avec le délai CTest d’origine de 1080 s.
Après la coupure de 96 s, finalité commune d’époque **7** retrouvée en **144 s** :
racine `0xdcaa49c6eba36a281c6b7b6903690ef1a7aff2da75a0282870cc3c487a9a0d5b`,
bloc d’exécution `0xc85a1530a7e6c1d75c7f85c4c89afa7c1eb4a45f8a0d89cc24f131a456944c9d`.

Le contrôle indépendant observe ensuite trois avancées communes **7 → 8 → 9 → 10**
en 144,552 s. Les deux nœuds terminent sur la même racine finalisée
`0x71dea9f07740e41b4264c5a69b785e3067880cb28717de99ae4aedd8262d6261`, justifiée
à l’époque 11. Les échantillons et résultats sont dans `stability/` ; commande,
empreintes, délais et résultat dans `followup-results.json`.

Les deux échecs de partition de la campagne initiale ont donc chacun un rejeu
corrigé réussi. Leur historique reste conservé. Cela qualifie ces scénarios
locaux avec le drainage réparé, pas le WAN ni une capacité de production globale.

## Intégration dans le build normal

La cible CMake `nano-bellatrix-transition-tests` a été reconstruite après les
rejeux. Son objet compilé est identique octet pour octet à celui du candidat
qualifié. Toutes les sections Mach-O de code/données sont identiques, ainsi
que les commandes de chargement hormis `LC_UUID` ; l’empreinte du fichier
exécutable diffère donc. Comparaison conservée dans `cmake-binary-comparison.json`.
Cela relie le build normal au code testé sans prétendre à une reproductibilité
bit à bit du lien Mach-O ni relancer les mêmes longues campagnes.
