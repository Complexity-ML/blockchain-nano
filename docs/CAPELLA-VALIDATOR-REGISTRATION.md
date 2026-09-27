# Clés locales et inscription des validateurs Capella

Le mode existant `index: "0x..."` lie la clé à un index déjà présent dans le
registre authentifié au démarrage. Une clé ne correspondant pas à cet index
est refusée.

Le mode `index: null` permet de préparer le nœud avant l’inscription :

```json
{
  "index": null,
  "feeRecipient": "0x000000000000000000000000000000000000002c",
  "keyEnv": "NANO_VALIDATOR_BLS_KEY",
  "journal": "/var/lib/nano/validator-journal",
  "restart": null,
  "recoveryWitness": "/var/lib/nano/validator-witness"
}
```

Cet objet est une entrée de `capella.validator.keys`. Les autres paramètres du
runtime, le profil et le snapshot restent requis. Il ne crée pas de dépôt,
ne finance pas le validateur et ne modifie pas son poids de consensus.

## Résolution et signature

La clé publique BLS est dérivée du secret fourni par l’environnement. À chaque
poll, la boucle cherche ces clés publiques dans le registre de la tête
authentifiée. Une clé absente ne reçoit aucun devoir ; elle est comptée dans
`awaitingRegistration`. Une clé enregistrée mais non active n’est plus comptée
comme absente, mais reste soumise aux règles d’activation des devoirs.

Les index ne sont pas déduits du nombre de validateurs ni de l’ordre local des
configurations. Ils sont résolus à nouveau, notamment après un changement de
branche. Le journal reste lié à la clé publique et aux identités de chaîne ;
chaque signature vérifie toujours sa correspondance avec le devoir authentifié.
Une résolution ambiguë ou plusieurs clés locales résolues vers le même index
font échouer la boucle avant signature.

Le reçu de reprise conserve `index: null` et ajoute `publicKey` pour identifier
la clé du journal. Une reprise par engagement explicite doit associer le token
à cette clé, pas à la position du tableau. Le témoin de reprise reste nécessaire
pour le mode de récupération qui le demande ; ce mode ne protège pas de la
perte ou du rollback simultané du journal et de son témoin.

## Limites actuelles

La résolution authentifie directement le SSZ puis parcourt les enregistrements
sans projeter tout le registre. Les recherches locales de quatre clés mesurées
à 100 000 et un million d'entrées prennent respectivement 146 ms et 1,43 s,
authentification comprise. Ce coût reste linéaire sous verrou à chaque poll ;
les parcours à index fixe évitent cette recherche. Ce ne sont pas des mesures
d'un réseau de cette taille. [Preuves et limites](qualification/capella-registration-ssz-scale-20260927/README.md).

Une réorganisation concurrente entre résolution et signature reste
soumise au contrôle d’identité du reactor : une erreur peut arrêter la boucle
locale, elle ne permet pas de signer pour une autre clé.

Ce mode concerne Capella natif Nano. Il ne déclare pas le cycle EIP-6110 Ethereum
complet ni l’activation publique du réseau.
