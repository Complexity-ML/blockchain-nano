# Activation et finalité après réindexation par une réorganisation

## Scénario

Le test reprend intégralement les deux branches AB/BA avec transactions EVM de
dépôt, admission de blocs, votes de sélection, réindexation 64 vers 65 et deux
réouvertures de l'observateur. Il continue ensuite la branche BA avec le
producteur et l'observateur, sans reconstruire leur état depuis une fixture.

Les validateurs déjà actifs produisent les blocs et attestations. La clé A,
configurée par clé publique, est résolue par `CapellaValidatorLoop` à l'index 65.
Le test exige zéro appel de son signataire avant activation. Ses attestations
sont ensuite produites par cette boucle et relayées au producteur via le chemin
normal d'admission SSZ. Les deux têtes doivent atteindre un même checkpoint
finalisé au-delà de l'époque d'activation. Les deux propriétaires sont rouverts
avec les engagements conservés, puis finalité et index sont vérifiés.

## Limites de la preuve

Orchestration locale synchrone, trois bases RocksDB, aucune couche HTTP/P2P/WAN.
Les producteurs initiaux utilisent des journaux isolés pour fabriquer la
concurrence de branches avec des clés de fixture. Les propositions des nouveaux
validateurs sont pilotées par le banc sur le producteur ; les attestations de A
passent par la boucle automatique sur l'observateur. Cette séparation de rôles
n'est pas une preuve d'exploitation de la même clé sur plusieurs opérateurs.

Le redémarrage est ordonné, sans crash pendant commit. La réouverture finale
concerne les propriétaires ; le journal de A reste ouvert et son engagement est
contrôlé inchangé. Pas de charge ni validation de disponibilité sous reorgs
soutenues. Aucun EIP, profil réseau ou objectif production clôturé.

Le budget de blocs du scénario inclut explicitement les slots de continuation
avant pruning ; aucun plafond de production n'est modifié. Les durées et budgets
restent bornés, avec arrêt au slot calculé même si la finalité manque.

## Résultat final

Le nouveau test passe en **175,54 s** : activation époque 13 à l'index 65,
neuf attestations de A admises avec neuf appels au signataire. Aucun appel
avant activation. Les deux nœuds atteignent la finalité commune d'époque 20,
racine `0xf345c36fd1615d05920d8d23bf5fd03c5dd59ecef1777b2ecaf05097fc88f5ec`,
conservée avec le registre réindexé après réouverture des propriétaires.
La continuation s'arrête au slot 177, sans boucle d'attente non bornée.

Le premier build du test référençait `slots` avant sa déclaration ; ordre de
déclaration corrigé, diagnostic conservé. Le test final n'a pas échoué.
Les espaces de fin de ligne des logs sont normalisés. Aucun code de production
ni déploiement VPS modifié ; aucune campagne globale rejouée.
