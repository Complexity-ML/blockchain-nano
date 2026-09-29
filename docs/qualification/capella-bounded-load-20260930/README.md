# Charge applicative bornée sur Mac et VPS — 30 septembre 2026

Statut : campagne réussie dans le périmètre ci-dessous, code de sortie 0.
Répertoire de travail : `/tmp/nano-bounded-load-20260930-a`.

Les 96 transactions sont finalisées sur les trois nœuds, avec reçus identiques,
au bloc commun 55 (`0x8e8e9946e870fef6b57ad68457355ec575436362ee435233585d993249bc095d`).
Nonce, solde destinataire et stockage sont vérifiés par les assertions du runner ;
les réponses individuelles de ces trois lectures d'état ne sont pas archivées.
Le contrôle complet du lot a pris 290,781 s après le début de soumission, dont
plusieurs collectes séquentielles de reçus : ce n'est pas une mesure précise de
latence de consensus. La stabilité post-charge couvre les époques 8 à 11 en
131,089 s. Les trois validateurs sont sains à l'arrêt (269/257/258 attestations
admises) ; les deux conteneurs sortent à 0 sans OOM.

Soumissions observées : 96 en 57,688 s entre première et dernière acceptations.
Sur chacun des trois paliers, les 31 intervalles donnent respectivement
0,989 / 1,938 / 3,670 acceptations/s. Il s'agit des cadences de l'injecteur,
pas du débit maximal de Nano.

31 échantillons de ressources par nœud, aucun manquant, intervalle maximal
20,804 s, début 1 ms après l'événement de démarrage des trois instances :

| Instance | Mémoire maximale observée | CPU maximal observé |
| --- | ---: | ---: |
| Mac natif (processus principal) | 272,5625 MiB | 45,1 % (`ps`) |
| Docker Mac | 306,1 MiB | 45,21 % |
| Docker VPS | 271 MiB | 102,8 % |

Le CPU Docker utilise 100 % pour environ un cœur ; les méthodes natives et
Docker diffèrent. Les 2,28 Go de BlockIO écrits par conteneur sont cumulatifs,
pas une mesure d'occupation disque. Les démarrages Docker antérieurs au processus
natif ne sont pas couverts par ces échantillons.

Le nouveau mode `NANO_TEST_BOUNDED_LOAD=1` du runner multihôte soumet exactement
96 transactions type 2, signées hors ligne avec la clé publique de fixture 777,
sur la chaîne isolée 1337. Trois paliers de 32 transactions visent respectivement
1, 2 et 4 soumissions/s ; les temps réels sont enregistrés. Chaque appel transfère
1 wei et exécute le contrat existant `SSTORE(0,42)`. Les écritures répétées ne
constituent pas un test de croissance du stockage, de contrats DeFi complexes ou
de débit maximal. Aucun fonds réel, aucune partition injectée dans cette campagne.

Critères : identité du genesis et du contrat, nonce initial sans autre émetteur
concurrent, financement synthétique suffisant, hash signé conservé par le RPC,
96 reçus réussis et identiques sur les trois nœuds dans des blocs canoniques
couverts par un même bloc finalisé, nonce final, solde destinataire augmenté de
96 wei et stockage final égal à 42. Une fenêtre bornée de 600 secondes commence
avant la première soumission. Le contrôle existant de stabilité de la finalité
s'exécute ensuite, ainsi que le contrôle des validateurs à l'arrêt.

Le cœur reste à `940736fd2078eb058289434cfefa2c2147df690d`, identique aux campagnes
précédentes ; les sources et binaires sont identifiés dans `run.jsonl`.
Les instances Docker réutilisent images et volumes de compilation existants.
La collecte des ressources commence au démarrage des trois instances ; elle
mesure des échantillons, pas les pics entre deux lectures. Le RSS natif exclut
le processus P2P enfant ; le compteur Docker BlockIO n'est pas l'espace disque.

Contrôles locaux ajoutés : 15 cas du vérificateur de reçus passent. Le résumé
des ressources a révélé un rejet des lignes `ps` brutes terminées par un saut de
ligne ; correction du parseur et 9 tests réussis, dont le refus de deux processus
dans une même ligne d'observation. Cela ne modifie aucun code du nœud.

La campagne ne clôt ni les limites restantes des sanctions, ni le profil EVM
complet, ni la qualification de version candidate, ni la revue indépendante,
ni les prérequis du bridge et du lancement Base–Nano.

## Preuves et reproduction

- `run.jsonl` : empreintes des binaires et des modules réellement chargés,
  chronologie, arrêt et verdict global.
- `load-transactions.json` et `load.jsonl.gz` : transactions synthétiques signées,
  96 acceptations, quatre collectes de reçus brutes et verdict du lot.
- `observations/` : contrôle de finalité post-charge et résultat.
- `resources.jsonl`, `resources-summary.json`, `*-state.json` : ressources et
  états Docker finaux. `verified-summary.json` : périmètre et cadences calculées.
- `manifest.json` : SHA-256 des preuves. Le code initial testé est une extension
  du commit `c11965c` ; les modules du runner sont identifiés par leurs hashes.

```sh
NANO_TEST_CONSENSUS_PROFILE=capella NANO_TEST_BUILD_VOLUME=1 \
NANO_TEST_BOUNDED_LOAD=1 node tests/multihost/run.mjs \
  ubuntu@145.239.199.166 940736fd2078eb058289434cfefa2c2147df690d \
  /tmp/nano-bounded-load-NEW
```

Le test utilise le binaire Foundry `cast` local pour signer hors ligne. Les clés
sont uniquement celles des fixtures publiques. Les conteneurs sont arrêtés et
leurs volumes conservés ; ne pas relancer cette commande pour simplement relire
les preuves. Les 15 tests du vérificateur et les 9 tests du résumé ressources
se lancent avec `node --test tests/multihost/{load-receipts,resource-summary}.test.mjs`.
