# Participation au réseau Nano

## Conditions présentes dans le code

`contracts/NanoStaking.sol` ne contient ni propriétaire, ni proxy, ni rôle
administrateur, ni liste d'adresses autorisées. L'inscription est une fonction
publique payante : le dépôt est attribué à `msg.sender`. Les conditions sont
communes aux participants, et non accordées individuellement par un opérateur.

- Dépôt minimum : 10 000 NANO.
- Au maximum 1 000 inscriptions simultanées.
- Verrouillage de 100 blocs, renouvelé lors d'un ajout de dépôt.
- Le dernier validateur ne peut pas retirer tant qu'un autre n'est pas inscrit.
- Le retrait rembourse le déposant ; aucune fonction administrative ne permet
  de retirer les fonds d'un autre compte.

Ces contraintes limitent l'entrée et la sortie ; « sans autorisation d'un
administrateur » ne signifie donc pas « sans conditions ». Le plafond est
également vérifié par le lecteur C++ du registre. Un acteur disposant de fonds
suffisants peut occuper plusieurs places ; l'absence de liste d'autorisation
n'est pas une résistance Sybil indépendante du coût du stake.

La transaction doit être incluse et exécutée avant que le nouvel inscrit
apparaisse dans l'ensemble de validateurs dérivé de l'état. Le bootstrap d'un
réseau signé dépend initialement de la clé déclarée dans le genesis. Les
signatures du réseau sont liées à son identifiant de chaîne. Ce contrôle de
rejeu ne filtre pas les applications auxquelles les utilisateurs accèdent.

## Vérifications exécutées

Les tests `staking` et `staking_paris` exécutent le même scénario natif avec les
règles expérimentales et avec Paris explicitement configuré dans un genesis
temporaire. Ils vérifient notamment :

- inscription signée d'un second compte sans permission du premier ;
- absence de droit de vote avant inclusion de l'inscription ;
- dérivation du nouvel ensemble après exécution et vérification par un pair ;
- impossibilité de voter pour une racine d'état incorrecte, même avec signature ;
- retrait après déverrouillage et refus de retrait du dernier validateur ;
- atomicité d'un remboursement refusé et rejet d'une tentative de réentrance ;
- conservation des dépôts, du registre et des engagements après réouverture.

Les deux tests passent. Ils ne constituent pas un test exhaustif du plafond de
1 000 validateurs, ni une preuve d'absence de censure. Le contrat accepte aussi
un déposant qui est lui-même un contrat ; la correspondance entre ces dépôts et
une clé capable de signer les votes doit encore être qualifiée pour la production.

## Limites de la conclusion

L'inclusion équitable des transactions, l'accès au réseau P2P, la résistance à
la saturation du registre et les transitions d'effectif sous attaque nécessitent
une qualification distincte. Un consensus BFT ne peut pas garantir la progression
si trop de poids de vote refuse de participer. Les tests ci-dessus prouvent les
parcours observés, pas une liberté absolue ni une préparation production.
