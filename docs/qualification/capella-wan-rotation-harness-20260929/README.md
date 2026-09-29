# Rotation WAN : instrumentation préparée, campagne non exécutée

Cette livraison modifie les outils de qualification, pas le cœur de consensus
`940736fd2078eb058289434cfefa2c2147df690d`. Elle ne ferme pas le critère rotation
WAN et ne constitue pas une preuve de production.

## Nouveau scénario

`NANO_TEST_WAN_ROTATION=1`, avec `NANO_TEST_CONSENSUS_PROFILE=capella`,
`NANO_TEST_WAN_PARTITION=1` et `NANO_TEST_BUILD_VOLUME=1`, prépare le test sur
les trois instances existantes (Mac natif, Docker ARM64, Docker VPS x86-64).
Le fixture exporté garde 64 clés publiques synthétiques, réparties 22/21/21,
8 slots/époque, 8 époques/période, 6 secondes/slot. La première modification
non triviale du comité est attendue au slot 128.

Le banc attend un head commun entre 120 et 127, vérifie que la déconnexion
précède 128 puis que chacun a franchi 128 lors de la reconnexion. Il conserve
la coupure de 96 s, le délai de reprise de 180 s et les trois avancées communes
du moniteur existant. Une fenêtre manquée échoue, sans report silencieux.

Les blocs signés SSZ sont capturés en continu avant leur élagage : limite de
256 blocs et 24 MiB, trois observations identiques exigées sur toute l'ascendance
retenue. Le cutoff d'inclusion est le head maximum à la reconnexion ; les blocs
antérieurs ne comptent pas comme preuve de reprise. Le collecteur ne vérifie
pas lui-même les engagements cryptographiques et ne suffit pas à qualifier.

Après arrêt, `nano-capella-rotation-audit` doit rouvrir les trois bases avec
leurs témoins de reprise et leurs tokens terminaux exacts, puis vérifier :

- état réellement finalisé, engagement SSZ et comité différent du comité initial ;
- engagement de chaque bloc capturé, signature du proposant, agrégats BLS ;
- au moins trois agrégats finalisés après reprise, avec membres appartenant aux
  autres instances, ownership lié aux configurations de clés de chaque nœud ;
- mêmes état finalisé et comité sur les trois instances, token non modifié.

Les trois audits sont nécessaires avant le verdict global `passed`. Le scope
reste un fixture fixe, sans activation de dépôts, changement de version de fork,
charge applicative ou mesure de capacité de milliers de validateurs.

## Vérifications acquises

- Compilation native de l'auditeur réussie (`native-build.log`).
- 13 tests nouveaux du collecteur réussis (`collector-tests.log`) : désaccord,
  preuves incomplètes, attribution de clés invalide, cycle, budgets, fenêtre
  manquée et timeout sont rejetés.
- Régression CTest du banc réussie (`harness-regression.log`).
- Contrôle négatif réel sur **une copie** de la base et du témoin Mac de la
  campagne `capella-wan-pinned-20260929` : token terminal séquence `0x443`, digest
  `0x200810be38cb64f77f38d216d784d9c6abb6f344a9c38eaee6b3c02a3904633a`.
  La réouverture passe puis l'auditeur rejette l'état finalisé d'époque 11,
  antérieur à la rotation (`pre-rotation-rejected.log`, sortie 1 attendue).
  Les bases originales sont conservées.

## Restant

Compiler le nouvel auditeur dans les volumes Linux existants, puis exécuter le
scénario réel et vérifier ses preuves positives. Les tests du collecteur ne sont
pas des tests réseau, et le contrôle négatif ne valide pas encore le chemin
positif cryptographique du nouvel auditeur. Pas de lancement public ni de fonds.
