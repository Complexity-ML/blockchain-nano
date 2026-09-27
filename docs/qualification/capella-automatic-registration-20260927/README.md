# Inscription automatique des clés locales Capella

## Écart corrigé

Le chargeur exigeait un index existant au démarrage. Le nouveau mode explicite
`index: null` dérive la clé publique BLS puis résout son index dans la tête
authentifiée à chaque poll. Une clé absente attend sans devoir ; une clé présente
reste soumise aux règles d’activation. Les index explicites restent stricts.
Le journal et le contrôle d’identité du reactor restent obligatoires avant
signature. Les reçus identifient les clés automatiques par `publicKey` et gardent
`index: null`. Les traces utilisent les index résolus.

## Qualification finale

**4/4 tests passent en 167,85 s** de temps réel (deux processus concurrents) :

- `consensus_capella_automatic_registration` : chargeur réel avec 64 clés connues
  à index automatique et une clé absente, reprise des témoins/journaux, attente
  de la clé absente, identification du reçu et traces de clés locales.
- `consensus_capella_deposit_auto_registration` : nouveau scénario complet,
  clé absente du genesis résolue après son dépôt, puis trois attestations produites
  par `CapellaValidatorLoop` et admises par deux reactors. Aucun appel au
  signataire avant activation. Dépôt consommé une fois au slot 80, activation
  époque 17, finalité époque 18 au slot 161 ; réouverture des bases vérifiée.
- `consensus_capella_configured_runtime` : régression du mode à index explicite.
- `consensus_capella_scheduled_finality` : régression de la boucle de validation,
  finalité époque 3, reprise des journaux et refus d’horloge reculant.

Le scénario d’activation a 65 validateurs au final. Il ne simule pas un réseau
de 65 machines. Les propositions de ce scénario long restent pilotées par le
banc ; les trois nouvelles attestations passent par la vraie boucle automatique.
La configuration automatique des clés connues couvre également les propositions
via la boucle dans le scénario configuré.

## Limites

Tests locaux, horloges synthétiques, pas HTTP/P2P/WAN pour ces nouveaux parcours.
Pas de crash au milieu de l’inscription, ni de reorg changeant l’index de la clé
pendant la signature. La résolution est renouvelée et les contrôles de signature
restent actifs, mais ce comportement concurrent demande une preuve distincte.

La résolution projette le registre sous verrou du reactor à chaque poll utilisant
ce mode. Coût à 100 000/un million de validateurs non qualifié ; ce n’est pas une
preuve de capacité illimitée. Les limites de stockage/transport et l’infrastructure
des témoins restent des critères ouverts. Aucun profil Ethereum supplémentaire,
aucune conformité EIP-6110 complète et aucune production globale déclarée.

[Configuration opérateur](../../CAPELLA-VALIDATOR-REGISTRATION.md).
