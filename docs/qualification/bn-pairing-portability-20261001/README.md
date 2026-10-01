# BN128 : compatibilité CPU et backend portable

Diagnostic cold-process 0c58afdf : 14 vecteurs réussis sur ARM64, 14 échoués sur AMD64 avec erreur exacte `bn:initPairing`, y compris entrée vide. Ce test ne lie ni EVM ni RocksDB : il isole l'initialisation MCL. La première compilation (55574731) échouait avant tout test sur un header JSON non trouvé ; ces deux tentatives sont conservées.

Le VPS expose un CPU Haswell, BMI2/AVX2 mais aucun ADX. Le backend x86 MCL par défaut compile -mbmi2/-madx et refuse son initialisation sans cette combinaison CPU. Les logs CPU et flags établissent cet écart ; ce n'est pas une reorg ni un défaut de signature utilisateur.

Correction expérimentale 9515c1cb : option MCL_PORTABLE par défaut sous Unix, LLVM bint/base génériques, pas d'objets ADX/AVX512, pas de Xbyak/JIT, MSM désactivé. Le guard CPU demeure pour les builds optimisés ; il est omis uniquement pour ce backend portable. Les contrôles de coordonnées, courbe et sous-groupe de src/bn_pairing.cpp ne sont pas modifiés. Si le compilateur C++ est GCC, Clang compile seulement les IR LLVM. Cette variante GCC reste à qualifier. Les autres toolchains ne sont pas qualifiées.

Les 14 vecteurs cold-process passent sur ARM64 ET sur le même Haswell AMD64 sans ADX, avec le même profil Docker durci (2 CPU/3 GiB/read-only/cap-drop-all/network none). Ce n'est pas encore le rejeu complet de l'EVM ni la preuve de correction des trois fixtures en mismatch. Les hashes incluent le helper, src/bn_pairing.cpp et le corpus.

Candidate intégrée ea9667f6 : qualification cœur/RPC/HTTP/admission/corpus/cold + huit charges lancée sur les deux architectures ; source figée, aucune promotion/déploiement avant résultats. Le diagnostic ajoute un test CTest indépendant avec deadline 30 s pour que cet écart ne soit plus caché par le premier échec du cœur. EIP-197 reste partiel pour la production tant que les binaires complets et le corpus AMD64 ne sont pas qualifiés.
