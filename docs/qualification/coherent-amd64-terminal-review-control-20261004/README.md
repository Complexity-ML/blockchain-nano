# Revue indépendante terminale AMD64 — contrôleur préparé

Adaptation du vérificateur ARM64 déjà exécuté, pour les sources/rapports/volume
exacts du VPS efbeb9bc. Aucun test, compilation d’image, signature ou service.
Exiger d’abord les sorties terminales contrôleur/CTest/audits/manifeste à zéro,
427 noms uniques correspondant au registre, aucune erreur/skip. Relire les
16 494 fichiers effectivement suivis par Git et tous les programmes du manifeste
(208 attendus selon la campagne), comparer avant/après. Vérifier le volume existe
avant une sonde Docker ; aucune création implicite d’un volume absent.
Recalcul des empreintes sur volume RO, ressources limitées ; contrôle des résultats
et du décompte EEST à partir du log terminal entier. Nouvelle sortie exclusive.

Exécution sur le VPS **après fin des tests et du scénario WAN**, pour ne pas
perturber ses mesures temporelles :

```sh
python3 -B /home/ubuntu/blockchain-nano-git/docs/qualification/coherent-amd64-terminal-review-control-20261004/review.py /home/ubuntu/nano-coherent-amd64-terminal-review-20261004
```

Le script est préparé et sa syntaxe Python vérifiée, **pas encore exécuté**.
Une réussite ne clôt que la campagne complète de cette candidate/architecture.
Restent restauration/reprise de duties, reproductibilité, nouveau pruning,
profil de production activé, bridge, termes de lancement et revue indépendante.
