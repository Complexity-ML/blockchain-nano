# Analyse du saut FFG 4 → 6 : checkpoint réel et rejeu natif

L'[échec WAN initial](../capella-wan-asymmetric-20260929/README.md) reste rouge
selon son moniteur originel. Cette analyse explique le saut sans rejouer un
réseau ni modifier le cœur.

Une **copie** de la base native arrêtée est ouverte avec `nano-dump-checkpoint.cpp`,
lié à la bibliothèque native déjà compilée. Le SHA256 du checkpoint est
`d3d27349fe6177a2b04a5f14f67ab8c1be675d1c429064efc4a1b2e324349ecf`,
séquence 740 : correspondance exacte avec le reçu terminal du run. Le checkpoint
comprimé et les sources des outils sont conservés ici. La base originale n'est
pas ouverte en écriture.

`nano-replay-ffg.cpp` vérifie ce digest externe puis recharge le store par le
validateur natif de snapshots (engagements SSZ, configuration et cohérence).
Il sélectionne le parent direct du bloc 72, exige le slot 71, rejoue les slots
avec `prepare_capella_slots` et compare bits, checkpoints justifiés et finalisé
avec le bloc réellement enregistré au slot 72. Résultat : égalité, finalité
4 → 6, justification précédente 6, courante 8, bits 15.

## Lecture indépendante de la règle

La chaîne d'ascendance enregistrée est conservée dans
`nano-ffg-canonical-states.json` ; les autres branches ne sont pas confondues
avec cette chaîne. Au slot 71, époque 8 : total actif 2 048 000 000 000 Gwei,
cible précédente 1 728 000 000 000 (54/64), cible courante 1 504 000 000 000
(47/64). Les deux dépassent deux tiers. Les anciens checkpoints justifiés sont
6 et 6 ; les bits passent de 14 à 15. Les conditions de finalisation à distance
de deux époques désignent alors 6, et non une obligation de finaliser 5 d'abord.
Les valeurs sont celles des états conservés, pas un quorum réinventé.

Référence primaire : [consensus-specs v1.5.0, weigh_justification_and_finalization](https://github.com/ethereum/consensus-specs/blob/v1.5.0/specs/phase0/beacon-chain.md#justification-and-finalization).
La transition native reproduit le checkpoint observé ; cela ne démontre pas
l'ensemble de la conformité consensus ni la cause de chaque vote retardé.

## Correctif du moniteur

Le contrôle strict reste le défaut pour ses anciens appelants. Le mode de
récupération utilisé explicitement par le banc WAN accepte de **chercher** une
nouvelle fenêtre après un saut : il ne compte pas ce saut, efface les avancées
précédentes et exige toujours trois avancées consécutives, liées à l'exécution.
Les historiques de racines/époques restent intacts. Délai absolu, retard maximal,
arrêt des heads et stagnation commune ne sont pas prolongés. Même le checkpoint
de saut doit être lié au bloc d'exécution canonique sur chaque nœud.

19 tests du moniteur passent, dont 6 nouveaux : crédit annulé, sauts répétés
épuisant le délai d'origine, incohérence d'exécution sur le saut, conflit et
régression après remise à zéro. La nouvelle campagne réseau a ses propres
artefacts et ne remplace pas l'échec originel.

## Reproduction du rejeu hors ligne

Décompresser `checkpoint.json.gz` dans un nouveau fichier, compiler
`nano-replay-ffg.cpp` avec les includes et liens de la cible `nano` du build
Release documenté, puis appeler l'outil avec le chemin JSON et le digest externe
préfixé par `0x`. Aucun service réseau ni clé privée réelle n'est nécessaire.
Le compilateur émet seulement les avertissements existants de suffixes GMP.
