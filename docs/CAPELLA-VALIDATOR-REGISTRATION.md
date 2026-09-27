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
authentification comprise. Ce coût reste linéaire sous verrou sur un défaut de
cache ; les parcours à index fixe évitent cette recherche. Ce ne sont pas des mesures
d'un réseau de cette taille. [Preuves et limites](qualification/capella-registration-ssz-scale-20260927/README.md).

Un cache interne à une seule entrée conserve le résultat pour la racine d'état
et l'ensemble exact de clés demandées. Il n'est alimenté qu'après authentification
complète d'une tête déjà admise ; les paramètres du reactor sont immuables.
Une nouvelle racine ou une demande différente impose un recalcul. Le cache ne
retient aucun snapshot, n'est pas persistant, et le contrôle d'annulation reste
actif sur un hit. Il réduit les polls répétés, pas le coût de chaque nouvel état.

Avant toute proposition, attestation ou signature sync automatique, le reactor
revérifie la paire clé/index contre sa tête courante, sous le même verrou que
la signature. Une clé disparue ou un index modifié déclenche `StaleCapellaDuty` :
la boucle abandonne ce poll et résout de nouveau au prochain, sans appeler le
signataire. Une clé configurée différente de celle du journal reste une erreur
bloquante ; elle n'est pas masquée comme simple changement de branche.

Le contrôle d'identité complet du devoir et le journal anti-slashing restent
obligatoires. Une bascule locale entre deux branches à dépôts inversés est maintenant
qualifiée : index 64 puis 65, deux réouvertures, ancien index refusé sans
signature. [Preuves](qualification/capella-competing-deposit-reorg-20260927/README.md).
L'activation après cette bascule est qualifiée localement : époque 13, neuf
attestations de A à l'index 65, finalité commune époque 20 conservée après
réouverture des deux propriétaires. [Preuves](qualification/capella-reindexed-activation-20260927/README.md).
Une erreur de synchronisation WAL au vote de bascule est qualifiée sur macOS :
issue entièrement ancienne ou nouvelle, archives cohérentes et rejeu vérifiés.
[Preuves](qualification/capella-deposit-reorg-sync-20260927/README.md).
La disponibilité sous reorgs soutenues, les autres points d'interruption,
Linux/VPS, les témoins indépendants et le transport distribué restent ouverts.

Ce mode concerne Capella natif Nano. Il ne déclare pas le cycle EIP-6110 Ethereum
complet ni l’activation publique du réseau.

[Qualification du cache et régression d’activation](qualification/capella-registration-cache-20260927/README.md) : 3/3 tests passent ; changement de racine et de clés contrôlé.

[Qualification des index périmés avant signature](qualification/capella-registration-signing-guard-20260927/README.md).
