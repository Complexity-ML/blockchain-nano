# Tâche locale d'attestation Altair

État au 25 septembre 2026 : construction et signature locales reliées au
coordinateur durable et au journal anti-slashing. Aucun planificateur autonome,
appel RPC de signature ou démarrage de validateur Beacon dans `nano run` n'est
activé. Ce profil Altair reste distinct de l'exécution EVM Paris/Shanghai.

## Autorité des données

`prepare_altair_attestation_duty` reçoit un Store déjà authentifié, un index de
validateur et le créneau attendu. L'appelant ne fournit ni bloc choisi, ni
checkpoints, ni comité, ni domaine de signature. La fonction utilise :

- le créneau/époque de l'horloge du Store et le choix de branche existant ;
- l'état du bloc choisi, vérifié contre son header/racine, avancé jusqu'au créneau ;
- le checkpoint de frontière d'époque obtenu par parcours des ancêtres ;
- le registre et le RANDAO à cette frontière pour dériver l'affectation de comité ;
- le checkpoint justifié de l'état avancé comme source du vote ;
- le fork et le genesis authentifiés pour choisir le domaine attester.

Le résultat contient un seul bit de participation. Une clé inconnue, inactive,
sortie, sanctionnée dans l'état courant ou marquée comme équivoquante est refusée.
Sans bloc courant valide, la préparation attend le délai d'attestation configuré.
Les budgets de transitions et de comité du profil s'appliquent ; ce ne sont pas
des plafonds supplémentaires du registre de validateurs.

Ces règles suivent la section [Attesting du validateur Ethereum](https://ethereum.github.io/consensus-specs/specs/phase0/validator/#attesting).
L'authenticité de l'ancre et les preuves de possession des clés à l'admission
restent des préconditions du Store ; cette fonction n'authentifie pas un snapshot
arbitraire reçu du réseau et ne vérifie pas un payload d'exécution EVM.

## Liaison avec le journal

`AltairDurableReactor::sign_attestation` exige l'engagement attendu du Store et
le compare au checkpoint sur disque. Le journal dédié doit correspondre au
chain ID, aux deux identités de genesis, à la clé publique dérivée du registre
et au nombre de slots par époque. Une identité différente est refusée avant
réservation ou appel du fournisseur de signature.

L'horloge locale est un callback indépendant fourni par le service de validation,
jamais une date fournie par un pair. Elle doit rester dans le créneau demandé et
ne pas être antérieure à l'horloge du Store. Elle est consultée avant et après
la préparation, avant le fournisseur puis à son retour. Une expiration pendant
le fournisseur conserve la réservation anti-slashing, mais ne retourne pas la
signature au demandeur. Le composant ne peut pas empêcher un backend malveillant
de diffuser lui-même une signature.

Le coordinateur garde son verrou pendant la tâche. Ce verrou est partagé par
les instances utilisant le même `ChainStore` ; une autre instance ne peut pas
changer le Store pendant la signature. Une instance restée sur un ancien
checkpoint est refusée. Le journal prend ensuite son propre verrou et réserve
sur disque avant la signature. Le fournisseur, l'horloge et les callbacks de
contrôle doivent être bornés et ne pas réentrer dans ces coordinateurs.

Le résultat n'est pas publié automatiquement dans le Store. Le chemin d'admission
des votes doit encore le vérifier dans sa fenêtre temporelle, puis archiver le
payload et son effet sur le choix de branche de manière atomique, comme pour les
votes reçus du réseau. Une attestation déjà signée ne permet pas de signer une
seconde tête dans la même époque : le journal conserve le premier engagement.

## Preuves et limites

Campagne `build-cmake/qualification/altair-attester-duty-20260925/tests.log` :

- ancre et bloc officiel du cas `shorter_chain_but_heavier_weight`, fichiers
  vérifiés contre le manifeste épinglé du dépôt ;
- tâche synthétique au slot 1, validateur 1 ; la clé de test est dérivée selon
  `privkeys[i] = i + 1` dans le [helper upstream v1.6.0-beta.0](https://github.com/ethereum/consensus-specs/blob/v1.6.0-beta.0/tests/core/pyspec/eth2spec/test/helpers/keys.py),
  puis comparée à la clé publique de l'état avant utilisation ;
- attestation signée réellement en BLS, domaine vérifié, puis acceptée par
  l'admission des votes au slot suivant et conservée dans son archive durable ;
- 14 refus avant fournisseur : mauvais créneau/horloge/engagement, index absent,
  annulation, cinq identités de journal incorrectes, expiration pendant la
  préparation, seconde tête dans l'époque et coordinateur périmé ;
- variantes synthétiques sanctionnée/inactive de l'ancre, attente du délai
  d'attestation sans bloc courant et refus d'un équivoquant connu ;
- expiration pendant le fournisseur : signature non retournée, réservation
  toujours présente ;
- régressions de publication durable du coordinateur, du journal et de la CLI.

Le test est un scénario composé à partir d'une ancre officielle, pas un nouveau
vecteur officiel complet de tâches de validateur. Il ne démontre pas la finalité
d'un réseau autonome. La préparation recalcule actuellement des projections et
commités par tâche ; aucun débit à 100 000 ou 1 000 000 validateurs n'est revendiqué.

Restent à intégrer et qualifier : propositions et construction complète des
blocs avant signature, RANDAO et tâches SyncCommittee, planification et horloge
opérationnelles (dont grands sauts/reprise après longue absence), gestion des
clés, file de diffusion locale, traitement groupé des tâches, liaison atomique
avec l'exécution et finalité distribuée. Ces contrôles ne rendent pas inoffensive
une clé volée utilisée hors du logiciel.

Le cas positif où le bloc choisi est déjà au créneau demandé a détecté puis
corrigé une tentative d'avancée de zéro slot : l'état authentifié est désormais
réutilisé. `current-slot-failure.log` conserve l'échec et `duty-final.log` valide
la correction, la construction d'une nouvelle tâche puis son refus spécifique
par le journal anti-slashing lorsque la première tête a déjà été signée.

`p2p-regression.log` passe la propagation existante de quatre blocs et un vote
officiels entre deux vrais démons libp2p, avec ACK perdu/reprise et changement de
tête distant. Il s'agit d'une régression du transport et du coordinateur ; ce
test ne diffuse pas automatiquement la nouvelle tâche locale et ne prouve pas
une finalité distribuée.
