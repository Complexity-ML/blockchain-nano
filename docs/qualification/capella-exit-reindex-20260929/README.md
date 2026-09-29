# Sortie après réindexation par réorganisation de dépôts

`consensus_capella_exit_reindex` passe sur Mac : **174,46 s** (CTest total
174,47 s). Base `ebe1d16`, patch et empreintes conservés.

Le banc produit deux branches avec deux dépôts dans un ordre opposé. Un troisième
propriétaire de base suit successivement les branches : la clé A passe de
l'index 64 à 65, alors que la clé B occupe désormais l'index 64. Après reprise,
A est activée à l'époque 13, signe neuf attestations et sa branche est finalisée
jusqu'à l'époque 20. Aucun RPC/P2P réel n'est utilisé dans ce cas local.

Après la durée de présence requise (une époque dans ce profil synthétique) :

- Une sortie signée par A pour son ancien index 64 est refusée sans mutation.
- Une signature altérée pour l'index 65 est également refusée sans mutation.
- La bonne sortie pour 65 est admise, sa répétition est idempotente et ses
  octets exacts sont retrouvés après réouverture des deux propriétaires.
- Le bloc suivant la sélectionne automatiquement. L'index 65 obtient son
  époque de sortie ; l'index 64 conserve `FAR_FUTURE_EPOCH`. Les clés restent
  associées à leurs bons indices.

Le test ne prétend pas qu'une sortie déjà éligible peut toujours être réindexée
par une réorganisation valide : l'activation dépend aussi de la finalité. Il
qualifie précisément l'usage d'un index ancien après la réorganisation et la
nouvelle activation. Il ne finalise pas le bloc de sortie dans cette branche
et ne refait pas le paiement : ces étapes sont des preuves séparées, et le
scénario bout en bout Mac/VPS reste ouvert.

```sh
ctest --test-dir build-release -R '^consensus_capella_exit_reindex$' -V
```

Les espaces finaux des journaux texte sont normalisés.
