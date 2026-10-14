# Compilation ciblée de la maintenance — contrôleur ROOT proposé

Un seul contrôleur, deux architectures Docker existantes. Il adapte les gardes qualifiés cd1b (owner/runtime) et la provenance 1da7 (correction pending) ; il n’exécute leur main ni leurs anciens tests. L’auteur n’a lancé aucun Docker/compiler/test/SSH. ROOT fournit toutes les SHA externes, fige le contrôleur puis le déclenche lui-même.

Le candidat base est b6b7 : les mêmes API/ABI owner/input/runtime et la fixture pending corrigée déjà qualifiée. Le CLI V2 4611/1b034 et le harness V2 5c83/f7c74/f398 ajoutent trois sources tools/tests/support uniquement. La sourceview neuve compte1320 fichiers :1317 anciens, seulement la fixture pending déjà qualifiée actualisée, plus trois nouveaux fichiers ; le CMake ajoute uniquement les trois cibles/install du tool. Tous les anciens compiler argv, flags et link.txt des cibles parent doivent rester byte-exacts. Aucun nouveau cœur/header ABI, aucune compilation des142 core units, aucun ar/ranlib de construction.

Les parents sont exactement run-02 : archive142/objets/conso/stat/MD réels + rawmember proof, SDK before/after. La correction pending fournit le vrai CPP/ELF/proof/sdk et un archive byte-identique ; ce résultat n’est pas transposé à un nouveau ELF. Les142 membres sont relus réellement avec ar-t et ar-p/hash/longueur sur la copie byte-exacte ; les142 objets du parent et toutes les sources sont protégés avant/après. Les MD ancestraux restent identifiés comme ancestraux, jamais reconstruits/qualifiés comme une compilation récente.

Source/build/controls neufs, source et controls RO dans Docker ; seul build RW. Images littérales e30 ARM64 et fd9 AMD64, --pull=never/network none/cap-drop ALL/no-new-privileges/2CPU/4GiB/pids256 et UID/GID du parent. Configure reproduit les anciens argv ; trois vrais -M avant CPP puis exactégalité avec trois actual.MD. SDK union ancien + nouveaux headers/liens + résolution réelle libdl.so/libdl.a avant CPP et hashes avant/après. Les sept libraries et KZG généré sont liés aux pins historiques disponibles et les copies à leurs readbacks. Pas d’install/update/imagebuild ni montage SDK hôte.

Le contrôleur compile seulement tool/test/shim (180s par objet), lie seulement leurs trois sorties (120s chacun), et produit terminal-build.json status BUILD_PASS_MAINTENANCE_TEST_NOT_EXECUTED. Durée extérieure1200s, configure120/probes240/SDK120/arproof120/compile600/link400. Un échec ne redémarre rien ; logs/commandes/exit/partial receipts restent accessibles, cleanup ne supprime que le conteneur possédé par son UUID/label vérifié. Les ctime Darwin sont diagnostiques ; SHA et dev/inode/UID/GID/mode/bytes/mtime restent stricts, tous les ctime sont enregistrés honnêtement. La plateforme AMD conserve aussi ctime strict.

Arguments obligatoires :

```
python3 build-maintenance-root-only.py --controller-sha CONTROLLER_SHA \
 --guard PATH_TO_cd1b --guard-sha cd1b0ce8abd1a69ec6c6dcc0b3c3ac79415ff00a0676187c48309bd5943a9ae2 \
 --repair-guard PATH_TO_1da7 --repair-guard-sha 1da7fbfb6e5651ba6b6f8ac61215426ca75abcd0bc2ff26e0d187faf523c69b4 \
 --parent-plan ACTUAL_PLAN_49ad_OR_c45b --parent-plan-sha FULL_PLAN_SHA \
 --parent-terminal ACTUAL_run02_TERMINAL_b9bd_OR_62e --parent-terminal-sha FULL_TERMINAL_SHA \
 --pending-terminal ACTUAL_1da7_BUILD_TERMINAL --pending-terminal-sha FULL_SHA \
 --candidate CURRENT_b6b7_MANIFEST --candidate-sha b6b7c4504ab02bc20c83f90688f9038ff9d118b616ed41f2c7db4eff59e53ff8 \
 --cli CLI_V2_MANIFEST --cli-sha 4611d22aa891e9b6368bc31cd6f7432635fc1dc0cb1ec4cf295a354e3a2c192d \
 --harness HARNESS_V2_MANIFEST --harness-sha 5c83d5e31803ddbdb6ea3026ec99f7ed460cf1c6db6e7f61f6c9bd77e1a6deba \
 --output NEW_ABSOLUTE_NAMESPACE
```

Paths sont ceux de l’hôte appelant et des manifestes publiquement transplantés par ROOT ; pas de request/capsule/DB privée. Le dossier output et toutes ses sousdirs sont neufs et disjoints des parents/control/source-proposals. Le module de garde est chargé depuis exactement les bytes externes pinés ; aucun module privé/project site package.

La seule future qualification sera le nouveau CTest maintenance, dispatché séparément après un vrai build PASS. Aucun test startup/pending ancien ni nœud/duty/clock/réseau n’est exécuté par ce contrôleur. La reprise WAL interrompue et la production demeurent ouvertes.
