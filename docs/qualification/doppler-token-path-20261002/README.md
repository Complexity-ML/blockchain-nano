# Parcours token Doppler — inspection source du 2 octobre 2026

SDK officiel inspecté à la révision `d923227f08afc5b8530a094d1dffc3986a174312`.
Les liens et empreintes des deux fichiers consultés figurent dans sources.json.

Constats limités : `CreateDynamicAuctionParams` exige `token: TokenConfig` ;
les variantes de ce type portent nom/symbole/URI et options d'émission ou de
contrôle, sans champ d'adresse d'un token existant. `DynamicAuctionBuilder.build()`
exige token, vente, migration et utilisateur. `withTokenFactory(address)` remplace
l'adresse du module factory : ce n'est pas une preuve d'import d'un ERC-20 existant.

Cette inspection ne démontre ni l'impossibilité d'une extension du protocole,
ni la disponibilité d'une factory personnalisée dans l'interface doppler.lol,
ni l'adossement automatique du token vendu à l'actif natif NANO.
Avant vente : vérifier l'ABI et les permissions Airlock/factory, l'offre et les
pouvoirs d'émission, puis qualifier la conservation de la réserve canonique,
les transferts et retraits Base–Nano avec le parcours exact retenu.
Aucun contrat déployé, aucune vente ouverte, aucune promesse d'indexation T0.
