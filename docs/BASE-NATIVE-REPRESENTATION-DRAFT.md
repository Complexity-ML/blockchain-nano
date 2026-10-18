# Base–Nanobees : options de relation avec l’actif natif

**Brouillon technique, aucune adoption.** L’utilisateur n’a pas encore arrêté la relation économique. Une représentation Base convertible d’un même actif est l’hypothèse conseillée à examiner, pas un choix validé. Offre, ratio, noms/symbole, allocations et inflation ne sont pas fixés ; aucun 1:1 n’est configuré dans le dépôt. Aucun contrat, test, fonds, vente ou déploiement n’est ajouté par cette fiche. C8 reste prioritaire.

## Trois options réellement différentes

| Option | Droit du porteur Base | Travail et limite |
|---|---|---|
| **Un actif économique, représentation Base couverte** | Conversion d’une quantité définie de tokens Base contre une quantité définie de NANO natif séquestré, sous les règles et disponibilités publiées du bridge. | Réserve native financée et vérifiable, route aller/retour, vérificateur de finalité, retraits et gouvernance à construire/qualifier. Pas de garantie de prix fiat ou d’absence de perte. |
| **Jeton Base séparé** | Utilité Base publiée ; aucun droit promis sur le NANO natif. | Offre/allocations et utilité propres. Pas de prétendue parité ni de conversion automatique : deux actifs, donc demande et prix potentiellement différents. |
| **Droit différé, claim gated** | Seulement un droit conditionnel précisément défini, avec activation, éligibilité, réserve, échéance et issue d’échec explicites. | Ne pas présenter un claim encore fermé comme du NANO déjà convertible. Le token Base, la cessibilité du claim et l’absence de garantie d’activation doivent être spécifiés ; un simple bouton désactivé ne crée aucun droit. |

## Hypothèse à étudier : préémission Doppler, deux séquestres

Supposer une offre Base **S** déjà émise pour Doppler, comprenant toutes les allocations (vente, LP, trésorerie, équipe, vesting et invendus concernés). Un adaptateur pourrait éviter tout mint/burn supplémentaire :

- Avant ouverture, isoler une réserve native initiale **R₀ = S × p/q** ; cette réserve est indisponible pour les dépenses de l’équipe et ne doit pas être comptée comme NANO natif libre en circulation.
- **Base → Nano** : verrouiller b tokens Base, puis libérer n = p×b/q NANO natifs seulement après authentification de l’événement source finalisé.
- **Nano → Base** : verrouiller n NANO natifs, puis libérer b = q×n/p tokens Base **déjà verrouillés**. Ne jamais libérer plus de Base que le séquestre en détient et n’en émettre aucun pour couvrir un manque.

S et b sont des unités atomiques Base ; R et n des unités atomiques natives. p/q est un ratio positif irréductible **immuable**, tenant compte des décimales des deux actifs. Ce n’est pas nécessairement le ratio affiché entre deux tokens entiers. Réserve initiale et chaque conversion doivent être exactement divisibles : q divise S et b ; p divise n. Refuser le dust non convertible, sans arrondi silencieux. Bornes et multiplication/division sans débordement restent à spécifier ; les frais doivent être séparés du principal couvert.

Avec **R = réserve native comptabilisée** et **B = quantité Base comptabilisée verrouillée**, sans frais sur le principal et après règlement des deux jambes :

```text
q × R + p × B = p × S
```

À l’origine B=0 et R=pS/q. Chaque aller réduit R et augmente B ; chaque retour fait l’inverse. R et B ne désignent pas les simples soldes bruts : un don au séquestre ne crée pas de droit de libération. La réserve couvre également les tokens Base encore détenus dans Doppler/LP ou dans une allocation conservée, pas uniquement les tokens déjà achetés.

**La conversion n’est pas atomique entre chaînes.** Entre verrouillage et libération, la valeur verrouillée et les obligations en attente doivent être comptabilisées séparément. L’égalité ci-dessus est un invariant de règlement ; ce n’est pas un contrôle valide des deux soldes bruts pris à des moments indépendants. Il faut spécifier les compteurs de principal en transit, les blocs/hash de chaque vue, les consommations uniques et les conditions de remboursement. Un timeout local ne prouve pas qu’une libération distante n’a pas eu lieu.

L’offre S doit rester compatible avec la politique Doppler réelle : inflation, minter/upgrade, rebase ou nouveaux tokens changeraient l’engagement de couverture. Une émission supplémentaire ne pourrait être couverte qu’avec une augmentation native authentifiée et une règle explicitement conçue ; elle n’est pas permise par ce modèle fixe. L’inflation du réseau natif, les récompenses et l’éventuel staking/slashing de la réserve sont des questions distinctes. Une réserve engagée ailleurs ou indisponible ne doit pas être annoncée comme intégralement retirable.

## Ce n’est pas le bridge déjà livré

Le [protocole existant](../docs/BRIDGE-PROTOCOL-DRAFT.md) comporte des endpoints abstraits ERC-20 lock/mint/burn/release, avec qualification synthétique bornée et **aucune route réelle activée**. Son `MintBurnGateway` exige que `totalSupply == outstanding`, avec encours initial nul : une offre Doppler S préémise ne respecte pas cette précondition. Ce modèle à deux séquestres est donc un **nouvel adaptateur**. Il ne suffit pas d’activer une route existante ou de modifier son compteur.

`CanonicalEscrow` n’est pas un séquestre natif implicite. Choisir une garde directe de NANO natif ou un passage par le wrapper local WNANO imposerait une comptabilité/protection supplémentaire explicitement vérifiée. Le vérificateur réel Base↔Nano, les domaines, les preuves de finalité et la gouvernance restent absents/non qualifiés. Une signature de relais, un RPC cohérent ou plusieurs clés sur le même VPS ne démontrent pas cette finalité.

## Trois masses à ne pas confondre

- **Réserve canonique NANO** : principal natif qui couvre le droit de conversion Base ; à engager avant d’en promettre la disponibilité.
- **Proceeds USDC** : produit des achats de la vente. Il ne remplace pas la couverture en NANO et n’est pas automatiquement disponible pour l’équipe ou transférable vers Nano.
- **Liquidité Doppler/DEX** : tokens et numéraire affectés aux positions de migration, selon migrateur et bénéficiaires réels. Elle ne constitue pas la réserve native et ses droits de retrait ne sont pas présumés. Un prix USDC de marché n’est pas une garantie de remboursement en USDC.

## Décisions minimales à prendre ensuite

1. Retenir représentation convertible, actif séparé ou claim conditionnel ; publier les droits exacts avant toute vente.
2. Fixer S, les décimales, p/q et toutes les allocations ; prouver la provenance/disponibilité de R₀. Aligner les pouvoirs d’émission et l’inflation Doppler avec ce choix.
3. Choisir qui vérifie quoi dans chaque sens, la finalité exigée, les limites de compromission et les droits pause/upgrade/reprise ; définir l’issue d’un retrait bloqué et le financement séparé des frais.
4. Vérifier la compatibilité du token et du parcours Doppler retenus (transférabilité, phases, LP/migration et USDC), puis les conditions d’ouverture. Visibilité du marché et indexation externe **à T0 restent deux preuves distinctes**.

## Preuves nécessaires avant d’annoncer une conversion disponible

- Réserve financée, allocations exhaustives et couverture réconciliée aux blocs identifiés ; contrôles impossibles à contourner par un rôle admin/minter/upgrade, ou confiance explicitement publiée.
- Conservation du principal pour aller/retour, dust, dons, frais séparés, plafond, stock Base insuffisant et messages en transit ; absence de double libération/remboursement après désordre, doublon, arrêt et reprise idempotente.
- Inclusion et finalité source réelles, reorg avant finalité, finalité contradictoire, mauvais domaine/actif/montant/destinataire, rejeu et compromission du modèle d’autorisation. Aucune émission/libération sur une simple observation RPC.
- Même scénario sur Base compatible et moteur/nœud Nano réellement qualifiés, avec pannes aux deux jambes, puis revue de sécurité indépendante. Les tests communs actuels du bridge ne qualifient pas ce nouvel adaptateur.

Ces exigences complètent, sans les remplacer, [FAIR-LAUNCH](../contracts/FAIR-LAUNCH.md) et les [critères de sécurité du bridge](../docs/BRIDGE-SECURITY-REQUIREMENTS.md). Il s’agit d’une proposition documentaire, sans nouveau contrat ni autorisation de fonds.
