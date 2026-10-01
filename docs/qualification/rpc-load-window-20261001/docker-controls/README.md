# Contrôles de compilation Docker ARM64 et AMD64

Source : b3b2245131a1a2f81d70866e1cb217321ca9c690. Builders existants :
ARM64 sha256:6116669430b02b7e8012af8c8bd515a5a7dcd1dd2034649f9b45ae5b9e70a3a4 ;
AMD64 sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c.

Les deux tests RPC corrigés passent `clang++ -fsyntax-only` sur les deux
architectures avec les dépendances existantes. Le helper compile et ses onze
contrôles passent avec GCC, optimisation O3 et NDEBUG. Ce sont des contrôles de
compilation et de politique de durée ; les tests complets ne sont pas liés ni
exécutés ici et aucune qualification réseau n'est acquise.

Le premier contrôle ARM64 a omis le define CMake NANO_TEST_SOURCE : diagnostic
conservé. L'invocation corrigée passe les deux syntax checks et compile le helper,
mais son exécution termine 126 sur /tmp non exécutable. Le helper est ensuite
compilé/exécuté dans un montage explicitement exec (exit 0), sans répéter les
syntax checks. AMD64 utilise directement les flags et le montage corrigés,
exit 0. Sources read-only, network none, aucun volume de build monté ; quota
0.25 CPU et 1 Go (256 Mo pour le helper ARM64 seul), cap-drop ALL.

La régression complète c10426b2 reste indépendante. Les sources runtime sont
identiques entre c10426b2 et b3b22451 ; cela ne prouve pas l'identité de nouveaux
binaires ni le succès des six scénarios corrigés. Les observations des deux
échecs OOG locaux d'origine sont également conservées sans modification.
