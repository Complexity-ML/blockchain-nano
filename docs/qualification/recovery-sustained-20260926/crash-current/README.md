# Reprise SIGKILL et finalité continue — 2026-09-26

`consensus_bellatrix_crash_sustained` réussit en 325,58 s avec les mêmes binaires que la qualification de partition `recovery-bound`.

Deux cœurs natifs et deux processus P2P sur un Mac, 64 clés synthétiques réparties 32/32. Le scénario tue un validateur par SIGKILL sans reçu d’arrêt propre, le redémarre puis observe trois avancées consécutives de finalité commune (2→3→4→5) pendant 144 793 ms. Les checkpoints Beacon sont liés aux blocs d’exécution finalisés sur les deux nœuds. Les deux nœuds terminent au même checkpoint finalisé 5, justifié 6.

Les fichiers `network.log`, `samples.jsonl`, `result.json` et `manifest.json` conservent les preuves et empreintes. Les empreintes ont été revérifiées à l’archivage. Le dépôt était modifié ; HEAD seul ne représente pas le code testé.

Ce test prouve ce scénario local borné. SIGKILL ne simule pas une panne électrique ni une perte de cache disque. Il ne qualifie ni plusieurs hôtes, ni une endurance longue, ni le pruning ou la croissance indéfinie de l’archive.
