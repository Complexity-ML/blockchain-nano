# Diagnostic FFG de la reprise WAN

Analyse hors ligne du checkpoint RocksDB natif arrêté de la campagne
`sustained-recovery`, cœur `c41e7b4`. Le dump a été obtenu avec
`rocksdb::DB::OpenForReadOnly` et la clé `fork-checkpoint/v1/payload`.
Archive synthétique sans clés privées. Aucun nouveau réseau lancé.

Reproduction :
```sh
python3 docs/qualification/multihost-20260927/ffg-recovery-analysis/analyze.py docs/qualification/multihost-20260927/ffg-recovery-analysis/native-checkpoint.json.gz
```

Le script lit les offsets Bellatrix utilisés par
`src/consensus_bellatrix_fork_state.cpp`, les limites enregistrées dans le
checkpoint et les soldes effectifs non slashés actifs. Le profil de cette
campagne utilise 8 slots par epoch. Les lignes retenues pour le diagnostic
sont les ancêtres par liens parent du checkpoint finalisé 11.

| Slot | Poids cible précédente | Poids cible courante | Justifié | Finalisé |
|---|---:|---:|---:|---:|
| 63 | 34,4 % | 57,8 % | 3 | 2 |
| 64 | 60,9 % | 0 % | 3 | 2 |
| 66 | 76,6 % | 10,9 % | 3 | 2 |
| 70 | 85,9 % | 75 % | 3 | 2 |
| 72 | 100 % | 0 % | 8 | 2 |
| 80 | 100 % | 0 % | 9 | 8 |

Au slot 64, la cible précédente reste sous les deux tiers. Elle dépasse ce
seuil au slot 66 ; la cible courante le dépasse au slot 70. Les transitions
d’époque produisent la justification 8 au slot 72 puis la finalisation 8
au slot 80. Cela explique la marche temporelle visible dans cette branche :
réduire seulement un timeout d’observation ne corrige pas le rattrapage.

Prochaine investigation : distinguer votes arrivés tard et votes disponibles
mais inclus tard avant la frontière 64, puis examiner la sélection des
attestations par le proposant. Les compteurs de latest messages seuls ne
prouvent pas leur inclusion dans l’état ni leur poids de cible valide.

Limites : décodage diagnostic, pas revalidation des signatures, racines SSZ
ou transitions ; liens parent non revérifiés cryptographiquement. Un seul
stockage natif terminal, pas une chronologie d’arrivée des votes ni une preuve
causale sur tous les nœuds. Le délai initial de 180 secondes reste échoué.
