# Nanobees sur Base et NANO natif — proposition de relation entre actifs

Statut au 18 octobre 2026 : recommandation de conception, non adoptée et non
implémentée. L’utilisateur a répondu « je sais pas » à la question sur le rôle du
token Base. Cela ne fixe ni ratio, ni offre, ni inflation, ni gouvernance et
n’autorise pas une vente. Le nom Nanobees et le choix Doppler/Base/USDC sont les
choix déjà exprimés ; le symbole et les termes économiques restent à confirmer.

## Usage proposé

Le jeton Nanobees vendu sur Base représenterait du NANO natif convertible à
raison de **1 jeton Base pour 1 NANO natif**, avant frais explicitement décrits.
Les USDC servent à acheter ou vendre sur le marché ; ce ratio ne signifie pas
1 USDC, ne garantit aucun cours et ne donne pas de droit au remboursement en
dollars. Une conversion exige le bridge et sa finalité, elle n’est pas instantanée.

Sur Base, l’utilisateur détient un ERC-20. Après conversion vers Nano, il détient
l’actif natif utilisable pour le gas et, suivant les règles effectivement
activées, le staking. Le token Base seul ne valide pas Nano et ne paie pas son
gas natif. Cette proposition ne crée ni droit aux bénéfices du projet ni revenu
garanti. Elle décrit un actif économique et deux formes de détention ; la
conversion reste soumise au risque du bridge et de la réserve.

## Trois comptabilités distinctes

| Élément | Rôle | Ce qu’il ne prouve pas |
| --- | --- | --- |
| Réserve NANO sur Nano | Couvre les droits de conversion des jetons Base | Aucun cours en USDC |
| USDC de la vente Doppler | Contrepartie des achats, selon les contrats choisis | Aucun adossement automatique en NANO |
| Liquidité après graduation | Marché résultant du migrateur retenu | Aucun transfert automatique sur Nano ou droit de récupérer les USDC |

Le bridge du token Nanobees et le bridge USDC sont deux routes et deux risques
à qualifier séparément. Acheter sur Base n’exige pas à lui seul de déplacer les
USDC vers Nano. L’indexation externe à T0 reste un autre critère.

## Adaptation proposée à la préémission Doppler

Le `MintBurnGateway` existant exige `totalSupply() == outstanding`, avec un
compteur initial nul. Une offre Doppler préémise ne respecte pas cette règle.
Il faut conserver ce refus : initialiser arbitrairement le compteur ou supprimer
le contrôle ne prouverait pas la réserve.

Pour une offre Base fixe `S`, la piste proposée est un **inventaire en escrow** :

1. Avant l’ouverture, immobiliser sur Nano au moins `S` NANO provenant d’une
   allocation identifiable. Couvrir toute l’offre Base créée, y compris les
   invendus, allocations vestées, frais en tokens et tokens destinés aux pools.
   La réserve ne peut pas également servir au staking, à une autre garantie ou
   aux dépenses de l’équipe. Une enveloppe de réserve séparée est nécessaire.
2. Base vers Nano : immobiliser les jetons Base dans un escrow ; après preuve de
   finalité et consommation unique du message, payer le NANO réservé. Les jetons
   Base restent dans l’inventaire du bridge et ne sont pas réémis.
3. Nano vers Base : immobiliser du NANO supplémentaire ; après preuve et
   consommation unique, libérer la quantité correspondante de jetons Base
   disponible dans cet inventaire. L’absence d’inventaire doit avoir une issue
   définie avant de prendre les fonds ; elle n’autorise pas une émission cachée.

Cette piste évite d’exiger du token Doppler une fonction `mint` distante. Elle
requiert néanmoins deux nouveaux endpoints compatibles, une authentification
réelle et une procédure d’initialisation : les contrats actuels ne fournissent
pas cette route. `WrappedNano` peut seulement servir d’enveloppe du NANO local ;
le déployer sur Base ne couvre pas du NANO distant. Le choix précis de détention
native ou WNANO dans la réserve reste à spécifier et tester.

## Conservation et transferts en attente

Dans une comptabilité réconciliée à des checkpoints finalisés et compatibles :

- `R` est la réserve native effectivement détenue et affectée à cette route ;
- `B` est l’offre Base hors escrow de conversion, pools et vesting compris ;
- `PN` est le total des paiements natifs engagés mais non acquittés ;
- `PB` est le total des libérations Base engagées mais non acquittées.

Exiger `R >= B + PN + PB`, dans des unités converties exactement. Une entrée
dans l’escrow Base déplace un droit de `B` vers `PN` ; son paiement diminue `R`
et `PN`. Un dépôt natif inverse augmente `R` et réserve `PB` ; la livraison Base
déplace `PB` vers `B`. Les jetons Base réservés à un transfert ou à son possible
remboursement ne sont pas disponibles pour un second transfert.

L’inventaire Base libre est un compteur comptabilisé : il exclut séparément les
jetons bloqués pour `PN` et ceux déjà réservés pour `PB`. Le solde brut du
contrat, notamment un don non sollicité, ne crée pas un droit supplémentaire.
Frais, retraits et remboursements doivent préserver la couverture des engagements
restants ; aucun administrateur ne peut prélever une réserve déjà engagée.

Cette équation est un critère à implémenter et vérifier, pas un résultat acquis.
Additionner des lectures RPC prises à des instants différents sur les deux
réseaux ne prouve pas cet invariant. Un message retardé conserve son obligation ;
un timeout seul ne permet jamais à la fois remboursement et paiement distant.
La même opération reprise après panne doit conserver son identifiant et ne
produire qu’un paiement. Les unités, décimales, arrondis et frais doivent être
explicites ; aucun taux ni frais n’est choisi par cette note.

Une offre Base fixe limite la capacité de cette représentation, pas le nombre de
validateurs ni l’offre native globale. Les récompenses natives éventuelles ne
créent pas automatiquement des jetons Base. Une inflation Base ou une capacité
évolutive demanderait une autre conception, un nouvel adossement et une décision
explicite ; la fixture de simulation à inflation nulle n’adopte pas cette règle.

## Conditions avant toute vente

Figer les deux domaines et déploiements canoniques, le token prévu, la provenance
de `S`, les autorités/possibilités de mise à niveau et les droits de retrait.
Vérifier les pouvoirs d’émission et les restrictions de transfert du module
Doppler réellement déployé, avant et après graduation et vesting. Une réserve
annoncée ou un événement de dépôt ne remplace pas ces vérifications.

Le bootstrap doit lier l’offre créée et l’adresse token aux engagements de la
réserve, avec finalité source et protection contre les forks/reorgs. Il doit
empêcher une ouverture effective de la vente si réserve et conversion ne sont
pas prêtes. Un contrôle d’interface ou une date programmée ne constituent pas
à eux seuls une barrière on-chain. L’activation et l’issue d’un échec entre
réservation, création et ouverture restent des travaux de conception ouverts.

Qualifier ensuite aller-retour, offre entière, invendus/vesting/pools, retrait
concurrent, manque d’inventaire, reorgs, rejeu, messages contradictoires,
compromission des autorités, arrêt et reprises idempotentes. Sans vérificateur de
finalité et modèle de confiance retenus, aucune route n’est activable.

## Sources et limites de cette préparation

Lecture du dépôt `652805dae4db675b5708fdb28a30f389f7929fe1` :
`contracts/bridge/MintBurnGateway.sol`, `CanonicalEscrow.sol`,
`BaseBridgeEndpoint.sol`, `contracts/FAIR-LAUNCH.md` et
`tools/doppler-launch/README.md`. Aucun contrat modifié, test lancé pour cette
note, wallet, fonds, vente, signature ou déploiement public.

Sources officielles relues le 18 octobre 2026 :
[présentation des modules Doppler](https://docs.doppler.lol/explainer) et
[Airlock, source amont main](https://github.com/whetstoneresearch/doppler/blob/main/src/Airlock.sol).
Elles décrivent création, modules autorisés et migration ; elles ne fournissent
pas ce bridge Nano ni l’approbation d’une factory personnalisée. La branche
amont mutable n’atteste pas le bytecode des contrats Base déjà déployés ; aucune
mise à jour du SDK local ou des configurations réelles n’est déduite ici.
