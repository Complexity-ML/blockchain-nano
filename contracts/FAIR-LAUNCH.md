# Fair launch Doppler / Base–Nano — préparé, non déployé

Statut : cadrage technique. Aucun contrat de vente ni bridge de lancement n'est
implémenté par ce document. Aucun financement réel n'est ouvert. La cible de
10 000 USDC vient de l'exemple utilisateur ; elle reste un paramètre à confirmer.

## Choix utilisateur courant — Doppler retenu le 1 octobre 2026

Plateforme retenue explicitement par l’utilisateur : https://www.doppler.lol/.
Le lancement est préparé sur Base, avec USDC comme numéraire souhaité, sans
petite pool initiale financée par 100 USDC. Le parcours Dynamic a été exploré ;
aucune configuration finale ni transaction de création n'est approuvée ici.

Employer le protocole et l’application Doppler pour la découverte du prix et
la migration de liquidité, plutôt que développer une courbe de vente maison.
Leur site et leur documentation officiels confirment Base et une interface/SDK.
Le mode, les contrats effectivement utilisés, le numéraire USDC disponible,
les paramètres économiques, frais et droits de migration devront être vérifiés
sur la configuration finale. Le site décrit des migrations vers Uniswap v2/v4 :
la graduation V3 de l’ancien cadrage n’est donc pas une contrainte retenue.

L’intégration à Doppler ne transforme pas le token Base en NANO natif et ne
constitue pas un bridge. Sa relation avec l’actif Nano reste à définir et tester.
La visibilité du marché Doppler et l’indexation externe à T0 sont deux critères
séparés : vérifier les intégrations effectivement disponibles, sans garantir
DexScreener par le seul déploiement. Aucune vente ni dépense ouverte ici.

Sources officielles consultées le 2026-10-01 :
- https://www.doppler.lol/
- https://docs.doppler.lol/

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

## Réserves et ancien prototype de courbe

Ancien prototype maison, écarté au profit du protocole Doppler : prix marginal p(s) = p0 + k*s, s étant l'offre nette vendue.
Acheter q coûte C(s+q)-C(s), avec C(s)=p0*s+k*s²/2. Vendre q rembourse
C(s)-C(s-q). Le code devra utiliser des entiers et un arrondi conservateur
(achats vers le haut, ventes vers le bas), avec unités explicites 18/6 décimales.
Ce prototype n’est pas implémenté et ne décrit pas l’algorithme de Doppler ; ne pas en faire la spécification du lancement retenu.

Les réserves restent dans le contrat, disponibles pour les ventes autorisées.
« Séquestrées » ne signifie pas « aucun remboursement ». Les achats bruts cumulés
ne constituent pas un seuil valable : des aller-retours pourraient le gonfler.
La graduation dépend des réserves nettes comptabilisées, sans compter deux fois
USDC canonique et sa représentation bridgée ni les transferts non sollicités.
Aucun retrait administrateur de cette réserve pendant la vente.

Il reste à fixer les paramètres de l’enchère Doppler, l’allocation, le seuil, la date limite, la politique de
frais, le sort des invendus et le mécanisme de sortie si le seuil n'est jamais
atteint. Un remboursement au prix initial de chaque achat n'est pas équivalent
à une revente sur courbe, notamment si les jetons sont transférables.

## Graduation

La destination DEX et les droits sur les réserves suivent le migrateur Doppler
retenu et ses contrats vérifiés. Les contraintes V3 ci-dessous ne s’appliquent
que si une position V3 est effectivement prévue ; aucun retrait libre d’USDC
pour financer Nano ne doit être présumé à partir du terme « migration ».


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

## Tableau de bord économique à implémenter

Aucune UI n'est encore livrée par ce cadrage. Elle affichera les données suivantes,
sans promesse de performance et sans jauge destinée à provoquer une urgence d'achat.

| Champ | Sens et provenance exigés |
| --- | --- |
| Prix marginal de la courbe | Calcul depuis l'état canonique au bloc identifié ; distinct du prix moyen d'un ordre |
| Devis d'achat ou de revente | Montant net, frais, minimum reçu/maximal payé, échéance et hypothèse d'état ; estimation avant confirmation |
| Réserves | Solde brut du contrat, réserve comptabilisée pour la courbe, obligations de remboursement et fonds en transit séparés |
| Offre | Allocation de vente, offre nette vendue, inventaire restant, allocation réservée à la graduation et adossement du token Base |
| Historique | Achats/ventes exécutés avec montants et frais, réseau, hash de transaction, bloc et statut de finalité |
| Phase | Vente ouverte, clôture, règlement des ordres, graduation partielle/complète ou remboursement selon les droits du contrat |
| Après graduation | Adresse pool, token0/token1, frais, ticks, prix courant, liquidité active, état hors plage, NFT et politique de verrouillage |

Le prix théorique et les transactions réalisées sont des séries distinctes. Avant
le premier échange, afficher « aucun échange exécuté » ; ne pas fabriquer des
bougies de marché à partir des devis ou de la formule. Afficher l'unité USDC,
sans assimiler mécaniquement un token bridgé à des dollars disponibles.

Chaque vue porte réseau/chainId, adresses vérifiées, numéro et hash du bloc lu,
heure de mise à jour et niveau de finalité. Les lectures d'une même chaîne doivent
viser le même bloc ; Base et Nano ont des références distinctes. Une réorganisation
invalide les événements concernés et reconstruit les agrégats, sans les compter deux
fois. Dédupliquer par réseau/contrat/transaction/index de log et suivre leur bloc.
Données indisponibles ou périmées : afficher l'état explicitement, pas zéro.

Un montant envoyé à un relais n'est pas encore une réserve disponible. Une copie
bridgée et son collatéral canonique ne sont jamais additionnés comme deux apports.
Les fonds remboursables ne sont pas présentés comme librement injectables dans V3.

La revente sur courbe, le remboursement d'un ordre non exécuté et l'échange sur DEX
sont trois opérations distinctes. Après graduation, la réserve devenue liquidité
ne garantit plus un remboursement au prix d'achat. La clôture et les délais du
bridge doivent être visibles avant signature, selon les règles effectivement codées.
L'interface seule ne crée aucun droit de retrait.

Critères UI : vérifier les totaux contre l'état on-chain au bloc retenu ; simuler
événements dupliqués/désordonnés, reorg, RPC indisponible, devis expiré, ordre distant
en attente et graduation partielle. Les boutons suivent les droits des contrats,
et aucune donnée de démonstration ne doit être affichée comme un marché réel.

## Modules à développer et critères de clôture

### Historique écarté : petite pool et vente parallèle

Cette variante a été écartée par l’utilisateur. Les points suivants documentent
les contraintes étudiées, sans autorisation de financement. Ne pas utiliser son prix spot ni son TWAP comme autorité de tarification
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
2. Configuration/protocole Doppler : conservation des réserves, limites d'offre, slippage, échéance,
   réentrance, décimales, arrondis, tokens refusant les transferts et faillite de vente.
3. Accès distant : messages forgés, doublons, désordre, reorg, perte de relais,
   expiration concurrente à l'exécution et impossibilité de double remboursement.
4. Graduation selon le migrateur Doppler choisi : seuil net, saturation d'offre, achats concurrents au seuil,
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

## Visibilité dès T0 — critère de sélection

L'utilisateur demande une cotation directement visible à l'ouverture de la vente.
Vérifier l'indexation pré-graduation du protocole retenu ; un graphique propriétaire
ne suffit pas à démontrer une disponibilité sur DexScreener. L'indexation externe
étant asynchrone, mesurer son délai et ne pas promettre une simultanéité absolue.
Séparer le prix initial achetable, la première transaction et les bougies indexées.

Avant de sélectionner un launchpad : vérifier sur une vente réelle comparable
réseau, factory, token, phase active et visibilité dans l'agrégateur ; examiner
l'actif de paiement, les frais, l'offre créée ou importable, l'adossement NANO,
les règles de retrait, le seuil et les destinations de la liquidité. Aucun
protocole tiers n'est choisi sur la seule présence d'une API.

Premier examen documentaire (26 septembre 2026, non qualification) :

| Candidat | Fait documenté | Écart ou preuve manquante pour Nano |
| --- | --- | --- |
| Flaunch | API Base/Base Sepolia ; lancement à prix fixe ; paramètre de capitalisation initiale exprimé en USDC | Ce paramètre n'est pas une réserve USDC ni un seuil de collecte ; compatibilité token adossé, courbe choisie, migration Nano et indexation T0 à prouver |
| Virtuals | VIRTUAL sert d'actif de référence/liquidité de son écosystème d'agents | Ce n'est pas automatiquement une vente USDC de NANO ; usage, token importable, migration et indexation T0 à prouver |
| Contrat Nano propre | Architecture décrite dans ce fichier, non implémentée | Intégration de l'indexeur à obtenir et vérifier ; reproduire des noms d'événements ne suffit pas |

Sources primaires consultées :
- https://docs.flaunch.gg/references/api
- https://docs.flaunch.gg/
- https://whitepaper.virtuals.io/
- https://docs.dexscreener.com/token-listing
- https://docs.dexscreener.com/api/reference

Aucun contact n'a été envoyé et aucun lancement n'a été effectué.

### Écarts confirmés lors de l'examen documentaire

Flaunch décrit un lancement à prix fixe et en ETH ; sa FAQ interdit les ventes
pendant le fair launch. Son API crée un nouveau memecoin. Ce parcours documenté
ne remplace donc pas tel quel la courbe USDC avec rachats décrite ici ; il faudrait
revoir ces choix et prouver l'adossement avant toute adoption. Aucune preuve
consultée n'établit à elle seule son indexation DexScreener à l'ouverture.
Sources : https://docs.flaunch.gg/protocol/frequently-asked-questions et
https://docs.flaunch.gg/references/api.

Lors de cette comparaison initiale, Doppler était un candidat technique : la
documentation décrivait des modules de création de tokens, des courbes
configurables et des déploiements sur Base/Base Sepolia. Le choix utilisateur du
1 octobre 2026, consigné en tête de document, retient désormais Doppler. Restent à vérifier
l'indexation avant migration, l'utilisation de l'actif USDC retenu, l'import ou
l'adossement de l'offre NANO, les modules canoniques autorisés et la destination
des liquidités. Sources : https://docs.doppler.lol/explainer et
https://docs.doppler.lol/reference/contract-addresses.

Critère non négociable avant lancement : preuve d'indexation du parcours exact,
pas seulement d'autres tokens du même écosystème après leur graduation. Le choix
du prestataire, les fonds réels et le déploiement restent non effectués.

### Parcours Dynamic examiné avec l'utilisateur

Le formulaire copié par l'utilisateur le 26 septembre 2026 expose Base et
permet de remplacer le numéraire ETH par **USDC**. C'est une observation de
l'interface, pas une vérification des contrats ou de l'adresse USDC émise par
Circle. Nom proposé : Nanobees ; symbole proposé : NBE, non confirmé.
Couverture générée ; aucune signature ni transaction de création effectuée par
l'assistant. La date du 10 octobre à 18 h Paris a uniquement été proposée comme
valeur provisoire pour parcourir les étapes ; elle n'est pas une échéance de
production validée.

Le SDK officiel consulté est `@whetstone-research/doppler-sdk` (exemples EVM
avec import `/evm`), et non `@dopplerprotocol`. Les exemples Multicurve avec
WETH ne prouvent pas les paramètres de ce parcours Dynamic/USDC. Sources :
https://docs.doppler.lol/reference/examples/multicurve et
https://github.com/whetstoneresearch/doppler-sdk.

Avant création : vérifier adresse/decimals du numéraire, offre et droits de
création du token, garantie de conversion au NANO natif, paramètres de vente,
remboursement et migration, destinataires des fonds et contrôle des positions
LP. Un nom identique n'est pas une garantie de conversion. La sélection USDC
ne prouve toujours pas l'indexation DexScreener pré-graduation.

### Décision utilisateur : reporter le formulaire

L'utilisateur préfère reprendre le formulaire une fois les éléments essentiels
prêts. Conserver le visuel et le brouillon ; aucune création de token, signature,
collecte ou publication n'est autorisée par cette préparation. Priorité actuelle :
reprise WAN, définition et qualification de l'adossement Base/NANO, puis vente
et migration de liquidité. La date provisoire n'est pas un calendrier de lancement.

## 2026-10-02 — Vérification du parcours token Dynamic

L'inspection du SDK officiel constate une configuration de token et une
factory personnalisable ; cela ne qualifie pas l'import d'un ERC-20 existant
ni l'adossement au NANO natif. La disponibilité d'une factory adaptée dans
l'application reste à vérifier avant la vente.
[Sources, constats et limites](../docs/qualification/doppler-token-path-20261002/README.md).
