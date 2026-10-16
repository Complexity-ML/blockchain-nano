# Test RPC courant : Native, Linux ARM64 et AMD64

La TU courante `tests/test_rpc_simulation_block_id.cpp` (`3f3dc2bc…`, 12665 octets), adaptée aux deux API RocksDB `DB::Open`, est maintenant passée dans les trois environnements, sur deux architectures CPU. La preuve Native antérieure est réutilisée sans rejouer son test. Les deux nouveaux essais Linux passent chacun cinq groupes, 120 checks et 58 refus attendus via le seul sélecteur `--eip1898-self-test`.

| Nouvelle qualification | M indépendante / CXX / lien / ldd / test | Dépendances M = MD | Test |
| --- | --- | --- | --- |
| Linux ARM64, tentative02 | 1 / 1 / 1 / 1 / 1 | 463, égalité littérale et normalisée | 52 ms, PASS |
| Linux AMD64, tentative01 | 1 / 1 / 1 / 1 / 1 | 461, égalité littérale et normalisée | 127 ms, PASS |

Les Core ARM `e2127db4…` /41053616 et AMD `e49483a5…` /40819792 restent inchangés, avec leurs 143 membres déjà qualifiés. Aucun Core, image, ancienne variante RPC ou suite historique n’a été reconstruit/rejoué. Les DB créées par ces tests sont exclusivement synthétiques dans un `/tmp` neuf et vide après le test ; les conteneurs possédés sont absents. Les sources projet rejoignent les sept arbres qualifiés de base `bd11bd9f…`, sous HEAD Root `87a32094…`.

Root conserve le refus générique de démarrage ARM01 (130 ms, sortie vide, avant admission de la sortie et avant M/CXX) ; les vingt entrées anciennes passaient. Un probe ultérieur du montage a observé UID0:GID0/mode0755 ; il peut lui-même avoir déclenché une copie initiale. Le lien avec cette préparation est donc inféré, et le point précis du refus01 n’a pas été instrumenté. ARM02 valide avant le worker un montage sans copie, UID501:GID20/mode0700. La comparaison JSON du pin Core a passé et n’est pas retenue comme cause. Ce changement hôte n’est pas une correction EVM. Le probe réel de la garde ressources modifiée passe avec trois échantillons de démarrage sur Mac, VM Docker et hôte VPS. Son reçu indique `probeOnly:true`, `launcherStarted:false` et `sigintSent:false` : aucun coordinateur, chemin d’arrêt ou suivi en campagne n’est qualifié. L’inspection ultérieure du volume ARM01 avec montage sans copie observe zéro fichier et UID0:GID0/mode0755, sans ouvrir de fichier de données ; elle ne prouve pas à elle seule la cause précise du refus initial.

Le manifeste source `d30e…` et les trois admissions de code ne sont pas réécrits par ce lot. Cette qualification ferme l’écart de variante du test Linux ; elle ne qualifie ni HTTP, un configure CMake global, tout le SDK/loader, ni une campagne fraîche. Les anciennes preuves et les refus historiques restent conservés, notamment la facturation CI avant exécution des jobs. Ce lot n’autorise ni nouvelle campagne, ni nouveau seed ou transfert de clé ; reprise des devoirs et finalité commune restent non exécutées, et la production reste non qualifiée.

La [preuve Native déjà publiée](../complete-nano-cross-arch-20261016/README.md) conserve son périmètre historique. Les deux résultats Linux sont de nouvelles preuves en processus, pas une extrapolation du résultat Native.

Références publiques lues (SHA256 / octets) :

- ARM02 : `d6307fda9c2de9abcc0dcb44dc2152a1d8d04e129a95f3c88e4d2b88326129e0` /3796.
- AMD01 dispatch : `ba2a9176354c21a03c3836a2c293614c1588f56ba502b645ec619eedb481470b` /5508.
- Native antérieur : `723ce846ba02385aa9e9f3bce77865189a08e14dfa15d62a8f88a581e72e382b` /2724.
- Garde ressources, probe : `3d1c46002ac9f6ad565120496b5ab937456650074c25092f54a12b439b8fa01a` /321.
- Garde ressources, trois échantillons : `2d0b83e07220f986711df9c1b99fd9b80071ffd7184180f6eb935337a555cce9` /2094.
- Inspection ultérieure ARM01 : `efe6bd272933210475daf421e84af537cc5f9ae4b536a0ed1cad35e23f713fbd` /63.

Docker documente le montage sans population initiale via [volume-nocopy](https://docs.docker.com/engine/storage/volumes/). Cette règle motive le montage corrigé ; elle ne prouve pas le point exact du refus historique ARM01. Les sources sous source/ sont des captures Root des contrôleurs exécutés, dépendant de leurs reçus privés locaux ; les statuts source-only historiques des propositions ne sont pas réécrits. Les résultats acquis sont portés par les reçus evidence/.
