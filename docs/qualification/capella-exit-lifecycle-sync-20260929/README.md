# Reprise WAL aux transitions de sortie — 29 septembre 2026

**16/16 cas sur Mac**, test `consensus_capella_exit_lifecycle_sync` réussi
(130,29 secondes). Le code du nœud est inchangé ; les modifications portent
sur le banc C++ et son enregistrement CTest. Les sources et le binaire du banc
sont identifiés dans le manifeste.

## Transitions effectivement atteintes

| Transition | Opération du réacteur | Résultat contrôlé |
| --- | --- | --- |
| Inclusion de sortie | Bloc | Époque de sortie attribuée, archive conservée |
| Retrait de l’archive | Tick de finalité | Checkpoint finalisé avancé, archive supprimée |
| Paiement du capital | Bloc | Premier et unique versement du capital |
| Finalisation du paiement | Tick de finalité | Capital présent dans l’état finalisé |

Chaque transition est exécutée avec une erreur fsync sur le WAL principal ou
sur celui du témoin, puis avec le suffixe conservé ou tronqué : quatre cas par
transition. Le processus termine par `_exit` après l’erreur, sans nettoyage C++.
Ce n’est pas une coupure électrique ni une simulation de tous les secteurs
disques endommagés. Le SIGKILL réel avant finalité est qualifié séparément dans
le scénario Mac/VPS.

Les bases sources sont fermées avant copie : aucun fichier RocksDB actif
n’est copié. Une exécution de référence indépendante calcule les états complets
avant/après. Après panne, le banc vérifie :

- checkpoint principal entièrement ancien ou nouveau, jamais un mélange ;
- RPC et état du propriétaire indisponibles pendant l’incertitude ;
- reprise déterministe par le témoin, ou maintien ancien si sa réservation
  n’a pas survécu ;
- registre, archive, tête, racine EVM et bloc finalisé cohérents ;
- somme exacte des retraits égale aux soldes courant et finalisé ;
- au plus un paiement du capital ;
- rejeu aboutissant exactement au résultat de référence, doublon sans nouveau
  checkpoint et résultat stable après une seconde réouverture.

Le banc refuse de passer si l’une des quatre transitions n’a pas été atteinte.
Il poursuit le réseau local jusqu’à dix époques avec des délais de sortie
raccourcis pour le test. Les témoins sont des bases séparées sur le même Mac,
pas des domaines de panne physiques indépendants.

## Échecs du banc conservés

1. Le contrôleur RPC était construit après l’injection et rencontrait déjà
   l’état fermé. Il est désormais construit avant l’erreur pour tester son
   refus de servir ensuite.
2. L’observation limitée aux blocs atteignait inclusion et paiement, mais
   pas les deux transitions de finalité. Le test a refusé de conclure.
3. Une tentative de prédiction par vote immédiat ignorait les attestations
   différées du slot courant. L’injection est maintenant située au tick où
   elles sont traitées et où les checkpoints de finalité avancent.

La campagne Linux/VPS passe sur `2d7c6da` : 16/16 cas, 447,33 s ;
avec les régressions admission WAL et transport signé, 3/3 tests en 476,02 s.
[Logs, JUnit, cas et provenance](vps/manifest.json). Le conteneur termine
avec code 0, sans OOM. La régression
ciblée du même binaire passe : 6/6 en 121,81 secondes (admission WAL, sorties
locales, file durable/reorg, transport signé, retrait d’archive et élagage
d’une sortie en attente).
La clôture globale du jalon exige encore son audit de couverture.

```sh
ctest --test-dir build-release -R '^consensus_capella_exit_lifecycle_sync$' -V
```

Les espaces en fin de ligne des rapports texte sont normalisés ; le patch
compressé conserve le diff exact. Les erreurs initiales restent des résultats
échoués, même après correction du banc.
