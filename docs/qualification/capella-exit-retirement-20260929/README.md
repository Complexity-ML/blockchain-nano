# Conservation et retrait de l'archive de sortie

`consensus_capella_exit_retirement` passe sur Mac en **64,80 s** (CTest total
64,81 s), avec 72 blocs, 524 votes, deux propriétaires de bases indépendantes
et finalité à l'époque 7. Les sources et le binaire sont identifiés dans le
manifeste ; le patch exact est conservé. Il n'y a pas de transport réseau ici.

Le validateur 0 demande sa sortie au slot 9, après une époque de présence
(paramètre propre au test). Le banc lit directement `signedExits` dans chaque
checkpoint durable. Après inclusion non finalisée, les octets signés et leur
identifiant doivent rester présents ; la répétition de la demande ne réécrit
pas le checkpoint. Le banc rouvre les propriétaires et journaux dès ce stade.

Quand l'état finalisé contient l'époque de sortie, l'archive doit disparaître.
Une nouvelle soumission est refusée sans mutation et l'export P2P du message
n'est plus disponible. Ces propriétés sont contrôlées à chaque bloc, après les
réouvertures et après l'élagage automatique : six changements d'ancre, au plus
31 états retenus, 18 à la fin, origine d'admission effectivement supprimée.

Ce résultat ferme le manque du test réseau qui ne lisait que `pending_exits()`.
Il ne prouve pas encore le réancrage d'une sortie jamais incluse, ni une panne
au moment précis du paiement ou de la suppression de l'archive. Le scénario
WAL séparé cible l'admission ; la qualification Mac/VPS reste un autre critère.

```sh
ctest --test-dir build-release -R '^consensus_capella_exit_retirement$' -V
```

Les espaces finaux des journaux texte sont normalisés.
