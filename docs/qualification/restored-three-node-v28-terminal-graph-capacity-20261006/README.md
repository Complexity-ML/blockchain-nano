# V28 : finalité figée et capacité du graphe atteinte

Campagne privée main `ee08b6ce`, cœur C++ `02ec0ab0`, relais compilé `2f93dda0` réutilisé par équivalence de ses 35 entrées de compilation. Le contrôleur original 7884 termine avec sortie 1 : aucune nouvelle finalité commune pendant les 900 secondes ; trois checkpoints distincts étaient requis. Baseline époque 1284, racine `0xccf88d18b2b98689ce47814d966ab42d10898eddd83a31bb2e3873ac5b2a2c48`. Trois reçus d’arrêt, exit 0 et absence d’OOM des conteneurs. La préflight initiale 93900 rejetée avant démarrage demeure dans son archive séparée. Le champ originalSession du terminal est une annotation du contrôleur après constat de sa sortie.

Les audits originaux 10017 et 73931 passent : 1728 fichiers natifs, 1759 Docker Mac, 2716 VPS inchangés, engagements de reprise et témoins de signature comparés séparément. Chaque Mac possède 16 clés actives et conserve aussi 16 journaux historiques ; le VPS possède 33 clés. Les 65 validateurs synthétiques appartiennent à trois nœuds sur deux hôtes physiques. Les configurations, requêtes privées et bases ne sont pas incluses dans cette archive. Le VPS a construit ses requêtes localement.

## Mesure et mécanisme

Les lecteurs diagnostiques réutilisent le cœur qualifié et ajoutent uniquement une projection publique du graphe restauré. Les fichiers source arrêtés sont inchangés avant/après. La compilation native initiale avec chemin relatif cassé est conservée dans cette archive ; sa réparation pointe vers le header figé. Aucun changement de binaire de production, aucune nouvelle image ni campagne relancée.

| Nœud | Blocs / limite 1024 | Slot ancre | Blocs avant/à finalité | Snapshot (octets) | SSZ états (octets) |
| --- | ---: | ---: | ---: | ---: | ---: |
| Mac natif | 1024 | 10271 | 1 | 90042255 | 34572752 |
| Docker Mac | 1024 | 10271 | 1 | 89807770 | 34455440 |
| Docker VPS | 627 | 10271 | 1 | 54161501 | 20485656 |

`blocks` est le nombre total de blocs du graphe, **pas** le nombre de blocs du seul rejeu canonique. L’admission de nouveaux blocs retourne false lorsque `state_->blocks.size() >= limits_.blocks` (`src/consensus_capella_reactor.cpp`). Les propositions locales retournent aussi nullopt sous cette condition (`src/consensus_capella_signing.cpp`). Les doublons déjà conservés restent traités séparément. Les deux Mac sont saturés ; le VPS conserve 627 blocs et ne déclenche pas ce refus de capacité. La saturation constitue donc une cause suffisante de refus des nouveaux blocs/propositions sur les deux Mac, sans établir toute la chronologie ni une cause unique de WAN A.

L’ancre conservée est déjà celle de la finalité, avec un seul bloc avant/à sa frontière. `prune_if_ready_locked` retourne quand l’ancre n’a pas changé ; appeler le même élagage ne libère pas les descendants non finalisés. Le pruning historique de l’exécution ne remplace pas la rétention du graphe consensus. Augmenter le plafond retarde seulement cette saturation. Supprimer arbitrairement une branche non finalisée peut changer LMD-GHOST et perdre du contexte de validation.

## Correctif requis, encore ouvert

Séparer la rétention persistante authentifiée du graphe et des états complets en mémoire ; permettre une récupération bornée du contexte d’une branche froide sans perdre votes, watermarks, checkpoints, archives, engagements de signature ou identité réseau. La capacité de mémoire doit produire une pression temporaire récupérable, pas un verrou définitif empêchant la nouvelle finalité. La nouvelle représentation et sa migration doivent rester vérifiables après panne.

Qualification à ajouter : saturation sur plusieurs branches valides, progression sans nouvelle finalité préalable, rechargement d’une branche froide, fork choice identique, refus d’une archive altérée, interruption avant/après publication et restauration des mêmes engagements ; puis une seule nouvelle campagne de finalité commune après correction pertinente. Aucune de ces réussites n’est revendiquée ici.

La fenêtre RPC VPS filtrée est une observation partielle, pas le total de campagne. L’échantillon Docker CPU/mémoire/BlockIO est instantané/cumulatif et ne prouve pas une charge soutenue, une taille physique de stockage ni une cause des écritures. Production, partitions restantes, cycles/rotations des validateurs, bridge, termes Doppler et revue indépendante restent ouverts.
