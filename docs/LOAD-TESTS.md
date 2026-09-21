# Tests de charge locaux

Le programme C++ `nano-load-tests` ouvre uniquement un serveur HTTP sur loopback,
avec des bases RocksDB temporaires supprimées à la sortie. Il pré-signe les
transactions pour que le temps de signature du générateur ne soit pas compté
comme latence du serveur.

```
ctest --test-dir build-cmake --output-on-failure -R '^load$'
build-cmake/nano-load-tests 8 50
```

Arguments : clients concurrents (1 à 32), transactions valides par client (1 à 90).
Chaque client soumet une transaction EIP-1559 valide, une transaction signée pour
un autre chainId et une lecture RPC. Le cas CTest utilise 4 clients × 25, soit
300 requêtes HTTP ; 8 × 50 produit 1 200 requêtes, dont 400 transactions valides.
Une erreur réseau n’est pas comptée comme un rejet valide : le test exige
explicitement le refus du mauvais chainId.

Après admission, la file est vidée en blocs signés, rejoués sur un second moteur.
Le test compare racines et reçus, vérifie le montant total reçu, audite les deux
historiques et rouvre la base. Il refuse les pertes, duplications, files bloquées
et divergences. Les délais client et CTest bornent les attentes.

Les métriques JSON mesurent le débit des requêtes mixtes, les latences p50/p95
**d’admission** des transactions et le temps de production/rejeu après admission.
Elles ne sont pas un débit de transactions finalisées par consensus BFT.

Limites : test court sur une seule machine, transferts simples, pas de charge
P2P distribuée, pas de finalité BFT activée, pas de mesure de saturation CPU/RAM,
pas de panne disque ni de partition. La production a lieu après la phase HTTP ;
ce test ne mesure donc pas encore admission et production simultanées. Un résultat
réussi ne constitue pas une autorisation de lancement public.

## Flood continu pendant la production

Un troisième argument optionnel active des workers supplémentaires d’attaque
pour une durée bornée de 0 à 60 secondes :

```
build-cmake/nano-load-tests 8 50 20
```

Huit clients légitimes sont alors accompagnés de huit workers répétant des
transactions signées d’une autre chaîne et des lectures saines. L’attaque se
poursuit pendant le drainage, le rejeu et l’audit (contrairement au mode par défaut
décrit plus haut). Les réponses doivent contenir un rejet explicite ; timeout et
erreur transport font échouer l’essai. Les compteurs de refus et de lectures
saines sont exportés. Le test exige une file vidée et une racine intacte après le
flood. Il ne simule pas encore une saturation de bande passante, des connexions
lentes, ni un flot de transactions valides au-delà de la capacité de la mempool.

### Premier essai continu : non validé

L’exécution locale `8 50 20` a échoué côté générateur avec
`All RPC nodes unavailable: RPC transport: Can't assign requested address`.
Le RpcClient ouvre une nouvelle connexion par requête ; l’erreur est compatible
avec l’épuisement des ports éphémères côté client. Cet essai ne démontre ni un
plantage du serveur ni sa résistance à vingt secondes de flood. Le générateur
doit réutiliser ses connexions avant une nouvelle campagne soutenue ; ne pas
compter cette erreur comme un rejet applicatif correct ou diminuer silencieusement
la charge pour déclarer la même campagne réussie.

### Connexions réutilisées

Le serveur HTTP accepte maintenant le keep-alive demandé par le client et remet
à zéro le parser et la réponse entre requêtes. Une session est renouvelée après
1 000 requêtes ; les limites existantes (128 connexions, corps 16 MiB, en-têtes
16 KiB et délai d’opération de 15 secondes) restent appliquées. Les erreurs de
lecture ne réutilisent pas la connexion.

Le générateur local possède une connexion par worker, avec échéance de lecture/
écriture de dix secondes et aucune relance automatique d’une requête perdue.
Il reconnecte seulement après une fermeture annoncée par le serveur. Le test
HTTP effectue 1 005 requêtes, puis une erreur RPC suivie d’une lecture valide.
Cette modification répond au défaut de génération observé précédemment ; elle
ne constitue pas une limite de débit ni une protection suffisante contre toutes
les formes de déni de service.

Résultat local observé après cette correction (`8 50 20`) : 23 040 rejets
explicites par les workers d’attaque et 23 040 lectures saines, en plus des 400
transactions valides, 400 rejets et 400 lectures de la charge de base. Admission
p95 : 10,447333 ms. Production puis rejeu : 1,886598 s. Audit, file vidée et
réouverture vérifiés. Ces mesures sont propres à cette exécution locale ; aucune
limite CPU/RAM ni garantie de débit de finalité distribuée n’en est déduite.
