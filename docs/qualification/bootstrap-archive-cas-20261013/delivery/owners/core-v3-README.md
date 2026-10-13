# Candidat C++ : archive signée du premier bloc Capella

Version 3 : tous les getters publics de preuve/plan retournent des valeurs, sans alias vers les octets privés. Le défaut de signature modifiable via le getter const-reference de V2 est conservé comme finding P1 historique.

Version 2 : les deux constructeurs explicites et apply prennent aussi la preuve/le plan par valeur avant tout callback. Version 1 conservée séparément.

Proposition isolée, à partir du dépôt `4228fcdaec166b32f6ad87bf8dd08c1459384bb0`.
Quatre fichiers de production sont proposés. Aucun dépôt, build qualifié,
checkpoint, journal, témoin ou copie de campagne n'a été modifié. Aucun test,
compilateur, Docker, SSH, runtime ou signature n'a été exécuté par l'auteur.
Les tests et la cible CMake sont préparés séparément par le collègue chargé du
banc ; ils ne figurent pas dans la qualification de cette proposition.

Le diagnostic effectivement exécuté par root a trouvé un checkpoint PRIMARY
conforme à son engagement externe mais aucune archive du bloc d'ancrage déjà
signé. Ce candidat traite ce manque précis ; il ne rend pas les nœuds prêts à
reprendre leurs duties et ne qualifie ni finalité commune ni production.

## Autorité et API

`prepare_capella_signed_bootstrap_archive(Genesis, CapellaSlotState, Bytes,
CapellaForkBlockRules, maximumSlots, maximumSyncTrials, checkpoint)` reçoit ses
valeurs par copie avant le premier callback. Il valide les règles, le Genesis
d'exécution, le parent Capella au slot 0, le proposant et sa signature, puis
rejoue la transition complète 0 → 1 avec le moteur de production : RANDAO,
opérations de consensus, exécution, retraits, racine post-état et reçus.
Il retourne une autorité privée, sans constructeur public, désérialiseur,
signer ou certificat booléen. Aucun nombre de validateurs n'est codé en dur.
La preuve n'autorise aucun autre bloc ni checkpoint : son Genesis, profil,
header, état post complet, comptes, bloc d'exécution et reçus doivent être
égaux à l'ancrage réseau original encore retenu dans le Store courant.
L'absence de cet ancrage après pruning est un refus, sans substitution.

Le constructeur initial explicite ajoute l'archive dans la première
publication ordinaire, uniquement depuis PRIMARY sans checkpoint et Genesis
canonique seul, et WITNESS sans checkpoint. Son intention initiale conserve
le format ordinaire existant et ses règles de reprise existantes.

Sur un Store existant, `prepare_bootstrap_archive(proof, primaryPin,
witnessPin, checkpoint)` produit un plan immuable. Il exige un propriétaire
d'exécution, PRIMARY couplé au tip et WITNESS dans son namespace indépendant.
Il lie les deux engagements courants et conserve le snapshot brut exact.
Le plan expose `next_primary()` et `next_witness()` : l'opérateur doit retenir
indépendamment les deux engagements futurs **avant** `apply_bootstrap_archive`.
Le candidat n'écrit pas ce reçu extérieur à la place de l'opérateur.

## Publication et reprise

Le supplément utilise le format distinct
`nano-capella-bootstrap-archive-recovery-v1`. Il conserve les cinq champs
canoniques format/previous/next/snapshot/archive. PRIMARY prend la séquence
suivante avec le même digest ; WITNESS prend son propre engagement sur
l'intention exacte. L'intention est écrite synchroniquement avant le batch
PRIMARY atomique avec CAS sur checkpoint, tip, identité et archive immutable.
Le batch a un suffixe d'exécution vide, aucun blob ajouté et aucune rétention.
Un budget conservateur refuse les batchs impossibles avant la première écriture.

Le publisher ordinaire est inchangé. Le supplément ne resérialise pas le
Store, ne nettoie pas les votes/opérations en attente, ne modifie pas clock,
finalité, caches, frames, mempool, événements, états historiques ou journaux.
Il expose seulement la nouvelle publication cohérente avec le nouveau token.
Il n'appelle aucun callback après l'intention durable. Toute erreur durable
latche le Reactor indisponible : réouverture obligatoire, aucune signature.

Le constructeur de reprise explicite exige la même autorité et les deux
engagements futurs retenus. Il authentifie l'enveloppe entière, les références
PRIMARY et WITNESS, le couplage d'exécution, puis termine un supplément en
réutilisant l'intention WITNESS déjà durable ; il ne l'incrémente pas à nouveau.
Le chemin de reprise ordinaire refuse le nouveau format, ainsi qu'une ancienne
intention archive-only de même digest. Une reprise déjà terminée n'écrit rien.

L'idempotence exige l'archive exacte **et** son index de slot présent/vide.
Un index manquant, corrompu ou orphelin est refusé ; il n'est pas réparé ici.
Les doubles de bloc, l'admission, les règles de fork et le raccourci de
publication ordinaire sur digest identique restent inchangés.

## Limites et suite requise

L'opérateur doit exclure tous les autres writers et travailler sur des copies
complètes arrêtées. Cette API n'a aucun point d'entrée CLI/RPC/gossip nouveau,
et ne remplace pas cette précondition opérationnelle par une déclaration.
Les structures de preuve et de plan vivent dans le processus ; leur copie
bornée n'est ni un format persisté ni un moyen d'importer une autorité.

Compilation cohérente et fermeture réelle des dépendances, tests adverses,
WITNESS-first interruption réelle, reprise froide et assertions de préservation
restent à exécuter par root. Les preuves des anciens cœurs ne qualifient pas
ce nouveau candidat. L'application aux copies de campagne, les duties, la
finalité commune, l'allowance de reprise et tous les critères de production
restent ouverts.
