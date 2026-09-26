# Ordonnancement du relais Bellatrix — 2026-09-26

## État avant correction

- Sous Assertoor : finalité à l'époque 0, tâche expirée après 240 s. Preuves dans
  `../assertoor-finality-20260926/`.
- Témoin natif sans Assertoor : échec après 253,70 s, faute de têtes communes
  dans le délai. Les reçus d'arrêt montrent pourtant une même finalité époque 2
  et une justification époque 3. Voir `baseline-failed.log`.

Ces deux observations excluent une explication limitée au décodage d'Assertoor.
Elles ne constituent pas une mesure statistique de performance : un seul Mac,
horloge réelle, essais successifs et compilation locale pendant le témoin.

## Défaut de capacité et correction

Le relais émet un message par tentative et alternait quatre catégories : blocs,
votes, preuves de slashing, sorties. Chaque tentative attendait un nouveau tick
de 100 ms, même pour une catégorie vide. Les votes étaient donc plafonnés à
2,5 tentatives/s avant le coût des appels et des acquittements.

Les catalogues de votes sont réinitialisés lors de l'élagage. Réémettre deux
époques de 64 votes en 48 secondes demande déjà 2,67 votes/s, sans les reprises
ou délais. Il s'agit d'une insuffisance de capacité identifiable dans le code,
pas d'une preuve que toute absence de finalité vient uniquement de cette cause.

`poll_altair_archives` peut désormais enchaîner les catégories vides via la
boucle asynchrone, avec un budget de quatre tentatives. Après une livraison ou
un échec, il attend le prochain tick. Un seul relais est en vol ; rotation des
pairs et catégories, curseurs, acquittements, reprises et admission cryptographique
restent identiques. Cette modification s'applique au mode Bellatrix uniquement.
Aucun délai de finalité ni règle de quorum n'est assoupli.

Le budget de quatre est une borne locale de travail sur les quatre catégories,
pas une limite du nombre de validateurs. Cette correction ne qualifie pas la
capacité pour 100 000 ou 1 000 000 validateurs, ni une saturation de toutes les
catégories à la fois.

SHA256 du nano-p2p avant correction :
`2bf2052be54786f935eb77e2476078781df456576bf6495a5841afa5372a9477`.
SHA256 après correction :
`d5acd439b54ab209aa47f909459fbe881e98f22d9f9e1a942069adc025e47a38`.

Commande de vérification :

```sh
cmake --build build-network --target nano-p2p -j4
ctest --test-dir build-cmake \
  -R '^consensus_bellatrix_(assertoor_finality|voluntary_exit_network|slashing_network)$' \
  --output-on-failure
```

## Verdict du premier correctif

**La finalité n'est pas résolue.** Le nouvel ordonnancement passe les tests de
sortie volontaire (10,30 s) et slashing réseau (5,33 s), mais la campagne
Assertoor finalité échoue encore après 269,82 s : finalisée 0, justifiée 2.
Preuves : `after-tests.log` et fichiers `after-*`.

Le changement reste un correctif de capacité partiellement qualifié, pas une
preuve de résolution du défaut de convergence. Pas de baisse de seuil, de
prolongation de la tâche de finalité ni de déploiement de ce résultat.

Le harnais a ensuite été complété : en cas d'échec, la sonde capture les corps de
blocs de tête et checkpoints avant l'arrêt ; le C++ conserve les bases des deux
nœuds après leur arrêt propre dans un dossier temporaire distinct. Ces bases
sont celles des fixtures à clés publiques synthétiques, jamais celles du VPS.
Cette instrumentation est compilée mais son chemin d'échec n'a pas encore été
rejoué. Les limites d'attente du harnais incluent les diagnostics ; le délai de
240 s de la tâche Assertoor demeure inchangé.
