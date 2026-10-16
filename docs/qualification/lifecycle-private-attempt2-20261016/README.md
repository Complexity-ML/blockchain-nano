# Campagne privée 02 : progrès observés, résultat global FAILED

**Campagne privée autorisée, terminée sans réussite globale.** Le [reçu terminal Root](terminal-root-observation.json), daté du 16 octobre 2026 à 19:34:37 UTC, conserve l’essai privé autorisé de 65 identités de test sur trois runtimes/deux hôtes, révision `e05ea5b9ff5bc8282a9fc120c079bdbddc29f7e6`. Le launcher est joint, exit1 ; le guard n’a pas envoyé SIGINT et compte 381 échantillons. Aucun temps de tenue de sept heures n’est déduit de ce nombre.

| Observation conservée | Portée |
|---|---|
| Finalité commune initiale 0 → 2 → 3 | Progression initiale observée. |
| Activation à l’epoch17 ; finalité commune 3 → 15 → 16 → 17 | Activation observée, sans clôture du cycle complet. |
| Après première restauration : finalité commune 20 → 24 → 25 → 26 | Avance après restauration observée ; copie source de 746 fichiers/47 240 521 octets. |
| Dernier arrêt VPS : epoch79, 33 clés locales, 220 propositions/1 909 attestations/7 342 messages sync admis | Constat du VPS seul, pas une finalité commune à epoch79. |

**L’essai reste FAILED.** La défaillance primaire est un `FileNotFoundError` au contrôle `voluntary_exit` : `config/genesis-state.ssz` manque lors de `lifecycleOwnerProjection` en mode stage. Le constat source transmis par Root ajoute que le transport des six fichiers omet aussi `genesis-validators.ssz` ; ce second manque n’est pas un nouvel échec runtime observé dans le reçu.

La clôture refuse ensuite « All three terminal signer receipts required » : deux reçus terminaux sur trois. La lecture source signalée par Root l’explique par le retrait du VPS de `started` avant l’étape qui échoue. Cette absence comptable n’est pas une preuve de processus VPS encore actif : à l’observation terminale, zéro conteneur de campagne tourne sur Mac Docker et VPS Docker, et les processus Native/tunnel sont absents. Le défaut de clôture reste conservé ; aucun PASS global n’en est reconstruit.

Les quatre rapports d’étapes enregistrés sont référencés ci-dessous ; leurs chemins complets et SHA256 figurent dans le reçu Root. Ils n’ont pas été ouverts pour rédiger cette note.

| Rapport enregistré | Octets |
|---|---:|
| `initial-finality.json` | 1 685 |
| `lifecycle-activation-finality.json` | 1 838 |
| `lifecycle-after-first-restore-finality.json` | 1 841 |
| `lifecycle-voluntary-exit-2-runtime-stop-public.json` | 6 577 |

Le [correctif du transport et des engagements](../lifecycle-control-stage-20261016/README.md) passe séparément 25 tests Docker ciblés et est intégré. Les observations partielles et tous les échecs restent intacts ; aucune restauration/runtime-recovery globale, sortie/retrait/slashing complet ou cycle lifecycle n’est qualifié. Aucun domaine/journal n’est réinitialisé ; aucune clé ou capsule n’est exportée pour ce diagnostic. La campagne n’est pas relancée par cette note.

Production non qualifiée, goal actif. V37, CI/facturation, tenue sous charge, qualification WAN complète et gates externes demeurent ouverts ; aucun EIP/fork, bridge ou propriété économique n’est promu. Le rapport conserve la distinction entre observation runtime et qualification unitaire ultérieure. Aucun nouvel essai n’est lancé.
