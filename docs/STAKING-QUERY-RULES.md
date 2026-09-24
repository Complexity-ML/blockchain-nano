# Budgets de lecture du staking

Le genesis peut définir `config.stakingQueryRules` sur une chaîne PoS signée
ayant un `stakingContract` :

```json
{
  "version": 1,
  "perCallGas": 5000000,
  "syncGas": 50000000
}
```

`perCallGas` borne chaque appel statique au contrat. `syncGas` borne la somme
des appels nécessaires pour reconstruire le registre. Les deux sont des entiers
positifs représentables en uint64 ; `syncGas` doit être au moins `perCallGas`.
Un appel consomme au plus le minimum du budget individuel et du budget restant.
L’épuisement fait échouer la synchronisation, sans publier de registre partiel.

La version et les deux budgets sont engagés dans l’identité du genesis : ce ne
sont pas des préférences locales que chaque opérateur peut changer. Une base
existante ne peut pas être rouverte avec une nouvelle identité. En l’absence
de cette section, les règles historiques et le hash du genesis sont conservés.
Une migration de réseau existant nécessiterait une transition explicite ;
modifier son fichier genesis n’est pas une procédure de mise à jour.

Augmenter le budget peut accroître le temps CPU et les besoins mémoire. Ce
paramétrage ne démontre aucune capacité nouvelle du registre : `getValidators()`
retourne encore la liste entière et chaque requête copie l’état. La pagination
et la synchronisation incrémentale restent nécessaires à la cible de grande
échelle. Aucune valeur d’exploitation nouvelle n’a été qualifiée ici.

Preuves : `stakingQueryRulesQualification` dans
`tests/fixtures/ethereum-official.json`.

## Version 2 : pages de validateurs

Pour une nouvelle chaîne disposant du contrat avec l’interface paginée :

```json
{
  "version": 2,
  "perCallGas": 5000000,
  "syncGas": 50000000,
  "pageSize": 64
}
```

`pageSize` est un entier uint64 strictement positif, engagé dans le genesis.
64 est un exemple de configuration, pas un plafond de validateurs. Les deux
budgets de gas restent applicables à la synchronisation complète.

Le moteur appelle `getValidatorCount()`, puis
`getValidatorEntries(start, limit)`. Chaque entrée contient une adresse, le
stake et le booléen d’inscription. Il exige le nombre exact d’entrées attendu,
un encodage ABI canonique et des adresses uniques sur toutes les pages.
Les appels sont exécutés sur le même état de bloc. Toute erreur invalide la
synchronisation entière ; aucune reprise silencieuse sur l’interface v1.

La version 1 et les chaînes sans section explicite conservent `getValidators()`
et les getters individuels. Un ancien contrat déployé n’acquiert pas les nouveaux
getters par une mise à jour du binaire. La version 2 doit donc être provisionnée
avec le contrat correspondant et une identité de genesis distincte.

La pagination réduit la taille des retours et le nombre de copies d’état :
1 + ceil(N/pageSize) appels au lieu de 1 + 2N. Elle ne rend pas le registre
incrémental : tous les validateurs sont encore parcourus à chaque synchronisation.
Elle ne qualifie pas à elle seule un registre réseau de 100 000 ou 1 million.

Les getters statiques utilisent désormais une vue constante de l'état EVM,
sans copie complète de l'état par appel ni snapshot de compte par frame statique.
Les frames mutables conservent leurs snapshots et leur rollback. La qualification
locale `evm-readonly-20260924` couvre 20 cas statiques, l'annulation et le rollback
d'un parent mutable, ainsi que les régressions staking/RPC/blobs (9 tests Release).
Les 18 artefacts sont indexés dans `readonlyEvmQualification` ; le corpus officiel
complet doit encore être rejoué après ce changement. Le parcours du registre
reste linéaire et soumis au budget de synchronisation.
