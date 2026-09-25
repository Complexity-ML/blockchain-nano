# Signature locale Bellatrix

Le coordinateur durable expose `sign_randao`, `sign_proposal` et
`sign_attestation`. Les clés privées restent dans le fournisseur de signature.
Le Store authentifié détermine le proposant, le comité, le domaine de fork, les
racines de vote et la clé publique attendue ; l'appelant ne choisit pas ces champs.

La proposition est construite contre la tête courante avec ses parents Beacon et
EVM associés. Son payload est exécuté avec l'historique de cette branche avant de
réserver sa signature. Le journal anti-slashing dédié à la clé est synchronisé
avant l'appel au fournisseur pour les propositions/attestations. RANDAO utilise
le contrôle d'identité et de retrait du journal, sans inventer une réservation de
bloc pour une signature d'epoch.

Le jeton du Store, le chain ID, les genesis exécution/validateurs, la clé et la
durée d'epoch doivent correspondre. Une horloge locale indépendante est contrôlée
avant, pendant et après signature. Une réservation survit à une expiration ou
à une erreur du fournisseur ; elle ne doit pas être annulée pour permettre une
seconde signature conflictuelle. Les clés copiées sur plusieurs bases indépendantes
ne sont pas protégées par un verrou local partagé.

## Qualification locale

Le parcours Bellatrix produit huit propositions locales identiques octet pour
octet à la réception normale, huit reveals RANDAO et une attestation issue d'une
affectation de comité. Cinq refus couvrent un jeton périmé, un slot expiré, une
proposition concurrente au même slot, cette même concurrence après réouverture
du journal et un fournisseur qui termine après la frontière de slot.

Preuves : `build-cmake/qualification/bellatrix-local-signing-20260925/`.
Les suites communes du journal anti-slashing et des duties Altair restent testées.
Ce scénario ne couvre pas à lui seul toutes les combinaisons de slashings,
partitions ou erreurs disque au moment de la signature Bellatrix.

Le résultat signé doit encore passer par l'admission normale du coordinateur.
Aucune diffusion automatique, service de validation autonome ou activation du
consensus Bellatrix sur le VPS n'est introduit ici. Transport, rattrapage,
qualification distribuée et paramètres du lancement restent ouverts.
