# Fair launch Base–Nano — proposition, non déployée

Statut : cadrage technique. Aucun contrat de vente ni bridge de lancement n'est
implémenté par ce document. Aucun financement réel n'est ouvert. La cible de
10 000 USDC vient de l'exemple utilisateur ; elle reste un paramètre à confirmer.

## Actif et compatibilité

Nano exécute du bytecode EVM compilé depuis Solidity ; la qualification dépend
du profil de fork et des fonctionnalités effectivement testées. Employer une
cible de compilation commune qualifiée, pas le dernier fork par défaut.
`WrappedNano.sol` crée des WNANO uniquement contre un dépôt de NANO natif.
Un ERC-20 émis sur Base n'est donc pas automatiquement du NANO : une réserve
canonique vérifiable et un droit de remboursement interchaînes sont nécessaires.
La vente doit disposer d'une allocation préfinancée, dont la provenance, le
montant et les allocations conservées par le créateur seront publiés.

## Proposition : une autorité de prix, deux accès

Base hébergerait le registre canonique de vente et la réserve USDC. Nano
hébergerait un accès asynchrone à cette vente. Cette proposition évite deux
courbes indépendantes prétendant avoir instantanément le même état.

Les ordres Nano ne deviennent des achats que lors de leur règlement canonique,
après validation du transfert d'actifs. Un événement de dépôt seul ne suffit
pas. Chaque ordre porte le réseau, les contrats source/destination, un identifiant
de campagne, un nonce, le bénéficiaire, le montant maximal et l'échéance.
Un ordre expiré ou trop cher est rejeté et ouvre un remboursement identifié.
Un message tardif ne doit jamais permettre à la fois remboursement et livraison.
Un délai local seul ne prouve pas qu'un ordre distant n'a pas été exécuté.

Les interfaces doivent afficher « en attente » jusqu'au règlement, sans promettre
un prix distant immédiat. L'arbitrage n'est ni une primitive de consensus ni une
garantie de convergence sans bridge fonctionnel.

## Courbe et réserves

Prototype proposé : prix marginal p(s) = p0 + k*s, s étant l'offre nette vendue.
Acheter q coûte C(s+q)-C(s), avec C(s)=p0*s+k*s²/2. Vendre q rembourse
C(s)-C(s-q). Le code devra utiliser des entiers et un arrondi conservateur
(achats vers le haut, ventes vers le bas), avec unités explicites 18/6 décimales.
Ce choix mathématique n'est pas encore implémenté ou validé.

Les réserves restent dans le contrat, disponibles pour les ventes autorisées.
« Séquestrées » ne signifie pas « aucun remboursement ». Les achats bruts cumulés
ne constituent pas un seuil valable : des aller-retours pourraient le gonfler.
La graduation dépend des réserves nettes comptabilisées, sans compter deux fois
USDC canonique et sa représentation bridgée ni les transferts non sollicités.
Aucun retrait administrateur de cette réserve pendant la vente.

Il reste à fixer p0, k, l'allocation, le seuil, la date limite, la politique de
frais, le sort des invendus et le mécanisme de sortie si le seuil n'est jamais
atteint. Un remboursement au prix initial de chaque achat n'est pas équivalent
à une revente sur courbe, notamment si les jetons sont transférables.

## Graduation

La clôture canonique bloque les nouveaux règlements et émet un identifiant de
graduation unique. Les ordres en vol doivent être réglés ou remboursables avant
d'affecter leurs fonds. La création des pools sur deux chaînes n'est pas atomique :
prévoir des états Prepared, Funded, Activated propres à chaque destination,
des reprises idempotentes et une politique explicite d'échec partiel.
Ne pas annoncer deux ouvertures simultanées garanties.

Une transaction doit déclencher chaque étape ; un contrat ne se réveille pas
seul. Le déclenchement peut être ouvert à tous sans droit de changer les paramètres.
L'allocation de tokens/réserves et les plages de ticks V3 doivent être calculées
pour le prix terminal de la courbe ; « tous les tokens restants » ne garantit
pas cet alignement. Vérifier aussi une pool préexistante ou initialisée à un prix
hostile. Les parts V3 sont des positions NFT : leur verrouillage exige un contrat
et une politique de collecte des frais, pas une simple destruction de LP V2.

## Modules à développer et critères de clôture

### Variante demandée : petite pool et vente parallèle

Une pool initiale avec 100 USDC reste une option à tester, pas un financement
autorisé. Ne pas utiliser son prix spot ni son TWAP comme autorité de tarification
de la vente : la profondeur, la durée d'observation et le coût de manipulation
doivent être évalués. Une moyenne temporelle ne crée pas de liquidité.

Deux marchés ouverts du même actif permettent l'arbitrage, même avec allocations
séparées. Les rachats par la courbe de tokens achetés sur le DEX doivent respecter
l'offre nette vendue et sa réserve ; une allocation externe ne crée aucun droit
supplémentaire sur les USDC de la vente. Tester expressément cet aller-retour,
les flash loans et la manipulation préalable à la graduation.

L'allocation doit distinguer vente, pool initiale et réserve de tokens pour la
graduation. Vendre les 99 % restants ne laisse pas nécessairement de tokens pour
apparier les USDC levés. Le prix d'une position V3 dépend aussi des ticks et de
sa composition : une allocation de 1 % ne fixe pas seule le prix initial.

L'ajout ultérieur de liquidité utilise le gestionnaire de positions, avec bornes
de slippage et échéance. Ce n'est pas un simple transfert de 10 000 USDC à la pool.
Les réserves initiales peuvent avoir changé avec les swaps ; ne pas promettre
10 100 USDC de réserves ni une profondeur garantie après graduation. L'affichage
par un agrégateur externe n'est pas garanti par la seule création de la pool.

Référence : https://developers.uniswap.org/docs/protocols/v3/guides/managing-liquidity/increase-liquidity

### Critères

1. Réserve canonique NANO et représentation Base : preuve d'adossement, émission
   limitée, retrait unique, finalité et gestion d'une finalité contradictoire.
2. Courbe locale : conservation des réserves, limites d'offre, slippage, échéance,
   réentrance, décimales, arrondis, tokens refusant les transferts et faillite de vente.
3. Accès distant : messages forgés, doublons, désordre, reorg, perte de relais,
   expiration concurrente à l'exécution et impossibilité de double remboursement.
4. Graduation V3 : seuil net, saturation d'offre, achats concurrents au seuil,
   transaction échouée puis rejouée, prix initial, pool préexistante et position verrouillée.
5. Banc Base-compatible + Nano réel : mêmes séquences d'ordres, conservation
   globale de l'offre et des fonds ; coupure du bridge au milieu de chaque étape.

CCIP/LayerZero ne sont pas supposés prendre en charge Nano. Un relais simulé peut
servir aux tests mais ne qualifie pas le bridge public. Les premiers actifs du
banc seront synthétiques, sans cotation USD annoncée.

## Sources consultées

- https://docs.chain.link/ccip/tutorials/evm/usdc : transferts avec messages,
  validation de destination et exécution potentiellement désordonnée.
- https://developers.uniswap.org/docs/get-started/concepts/liquidity-providers/concentrated-liquidity :
  plages de prix V3 et liquidité inactive hors plage.
- https://www.circle.com/bridged-usdc : standard USDC bridgé, distinct de la
  qualification du bridge et d'une adoption native par Circle.

La collecte communautaire n'élimine ni les coûts de déploiement et de relais,
ni les risques de contrats, de bridge, de MEV ou de pertes des participants.
