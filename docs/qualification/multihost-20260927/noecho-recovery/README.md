# Reprise WAN après suppression du renvoi au pair émetteur

Cœur/images `c2ec4d7936304ce868f2699d2932eadc2363e9e3`, banc et empreintes
consignés dans `run.jsonl`. Trois instances (Mac natif, Docker Mac arm64,
Docker VPS amd64), deux hôtes, 64 validateurs synthétiques, slots de 6 s,
époques de 8 slots. Aucune transaction utilisateur ni fonds réels.

Coupure configurée de 96 s après finalité commune 2. Reconnexion à
1790468518099 ms ; nouvelle finalité commune 7 à 1790468659101 ms :
**141002 ms, réussite du seuil inchangé de 180 s**. Puis **7→8→9→10 en
144025 ms**, résultat global 0. Trois boucles saines ; attestations admises
243/233/227. Audit de 183 observations, cinq époques : aucun conflit de
racine par époque ni régression enregistrée.

C'est une première réussite de ce scénario WAN avec cette correction,
après les campagnes 187542 ms et 189200 ms conservées. Elle ne constitue
pas une mesure statistique du gain causal ni une garantie de reprise sous
charge. Les durées comprennent l'échantillonnage des observateurs.

Corrélation diagnostique : 420/458 inclusions de la branche finalisée
native reliées à 1838 événements datés ; huit remises à zéro de curseur
exclues des transferts. Un achèvement de relais ne prouve pas la date de
disponibilité chez le proposant. Les mesures de décalage des horloges
figurent dans le journal. Pas de revalidation cryptographique BLS/SSZ
par les analyseurs hors ligne.

Après terminaison du contrôleur (code 0), les conteneurs
`nano-qualification-{mac,vps}-muj2k4yo` sont `exited`. Aucun processus ne
correspond au chemin de campagne. Les bases synthétiques sont conservées.
Ce résultat ne clôture ni la qualification de production, ni l'endurance,
ni les autres topologies, ni la charge, ni le passage à grande échelle.

Reproduction des analyses depuis la racine du dépôt, avec ce dossier comme D :
```sh
node tests/multihost/audit-history.mjs "$D"
node tests/multihost/inclusion-timing.mjs "$D/native-checkpoint.json.gz" "$D"
node tests/multihost/relay-summary.mjs "$D"
```
