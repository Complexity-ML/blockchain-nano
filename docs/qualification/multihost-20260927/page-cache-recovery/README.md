# Reprise WAN après cache de pages

Cœur/images `a4bf5e6d3be6e3bb97ed00cafbe4eccf9750f9d3`, banc `398148d`
(inchangé par les commits documentaires suivants). Trois instances, deux
hôtes, 64 clés synthétiques, slots 6 s, epochs 8 slots, aucune transaction
utilisateur. Coupure configurée 96 s après finalité commune 2.

Reconnexion à 1790467046207 ms, reprise commune 8 à 1790467235407 ms :
**189200 ms, échec du seuil 180 s**. Ensuite **8→9→10→11 en 148066 ms**.
Sortie globale 1 conservée. Trois boucles saines ; attestations admises
269/259/252. Audit : 210 observations sans conflit de racine par epoch ni
régression enregistrée. Ne vaut pas preuve cryptographique d’ascendance.

Les états de la branche finalisée native montrent au slot 64 seulement
53,1 % du poids de cible précédente, puis 64,1 % au slot 65 et 78,1 % au
slot 66. La cible courante franchit le quorum au slot 70 ; justification 8
au slot 72. Le cache réduit les requêtes d’inventaire dans le test local,
mais cette campagne ne démontre aucun gain sur le délai global WAN. Les
durées observées incluent l’échantillonnage et ne mesurent pas l’instant
exact de finalisation interne.

Corrélation : 478/520 inclusions reliées à 2658 événements datés ; deux
remises à zéro de curseur exclues comme transferts. Ces correspondances
ne prouvent pas la disponibilité chez le proposant. Les votes et branches
diffèrent de la campagne précédente : pas de comparaison causale entre
deux votes choisis arbitrairement. Revoir le coût de redistribution, les
retours de votes au pair qui les a fournis et l’ordre des files avant un
nouveau changement.

Reproduction, depuis la racine du dépôt, avec ce dossier comme `D` :
```sh
node tests/multihost/inclusion-timing.mjs "$D/native-checkpoint.json.gz" "$D"
node tests/multihost/audit-history.mjs "$D"
python3 docs/qualification/multihost-20260927/ffg-recovery-analysis/analyze.py "$D/native-checkpoint.json.gz"
```
Décodage diagnostic, sans revalidation BLS/SSZ. Les fichiers concernent
uniquement des états synthétiques, sans clés privées. Conteneurs
`nano-qualification-{mac,vps}-muj1ok5x` arrêtés (`exited`), PID natif 31248
absent. Bases conservées. Production, endurance et charge non qualifiées.
