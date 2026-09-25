# Sauvegarde et reprise du Store Bellatrix

`bellatrix_store_snapshot` sérialise le Store local déjà authentifié : états
Beacon de toutes les branches, blocs/états/reçus EVM correspondants, historique
d'exécution antérieur à l'ancre, votes, equivocations, checkpoints, horloge et
proposer boost. Le registre justifié est reconstruit depuis l'état SSZ ; ses poids
ne sont pas acceptés sous forme d'une seconde copie indépendante.

L'enveloppe canonique `nano-bellatrix-store-v1` lie les paramètres Altair communs,
les limites et paramètres Bellatrix, chain ID, politique de rejeu, durée des slots
et profil de proposer boost. Le lecteur exige le profil local attendu et un digest
obtenu indépendamment. Il borne l'enveloppe avant parsing, les nombres d'objets et
les octets SSZ individuels/cumulés. La borne d'enveloppe inclut les états EVM JSON ;
elle ne garantit pas une borne équivalente sur la mémoire du DOM pendant le parsing.

La relecture vérifie les racines Beacon/EVM, transactions, reçus et métadonnées,
les relations de parenté des deux chaînes, l'historique de l'ancre, les marqueurs
d'epoch et la cohérence de l'horloge. Pour les blocs hors ancre, les métadonnées
FFG sont comparées aux checkpoints dérivés de leur état. Une réémission identique
au snapshot original est requise, ce qui refuse les champs supplémentaires et
les encodages non canoniques.

Ce format n'est **pas une admission de snapshot fourni par un pair**. Le digest
indépendant authentifie un Store local préalablement validé. Les signatures des
votes et les corps signés Beacon ne sont pas archivés ici, et les transitions ne
sont pas intégralement rejouées. La validation de cohérence ne remplace pas leur
admission initiale ni l'origine de confiance de l'ancre.

## Test intégré avec RocksDB

`consensus_bellatrix_signed_execution_chain` persiste le Store complet via
`commit_execution_checkpoint`. Après une première sauvegarde de la chaîne, le test
publie la branche concurrente puis revient sur la première. Chaque publication
est suivie d'une fermeture/réouverture RocksDB, d'une revalidation du Store et d'une
comparaison de sa tête EVM avec le bloc, l'état et le reçu réellement persistés.
Les branches concurrentes restent disponibles dans le snapshot pour la suite.

Le scénario couvre trois réouvertures, six refus (digest, profil/chain ID, état EVM,
historique de l'ancre, doublon et métadonnées de reçu), la borne d'enveloppe et trois
annulations de sérialisation. Il conserve les neuf blocs signés, deux bascules de
tête pilotées par votes BLS et les refus de blocs/annulations précédents.

Preuves : `build-cmake/qualification/bellatrix-store-recovery-20260925/`.
Les essais de SIGKILL et d'erreur de sync restent ceux de la primitive de stockage
(`storage_execution_checkpoint*`) ; le scénario Bellatrix ferme proprement la base.
Aucune coupure électrique physique ni partition réseau n'est démontrée ici.

## Coordinateur et périmètre restant

Le coordinateur local sérialise désormais les événements, conserve les enveloppes
signées admises, calcule le suffixe canonique et protège la finalité avant l'appel
au stockage : voir [BELLATRIX-DURABLE-REACTOR.md](BELLATRIX-DURABLE-REACTOR.md).
Le test direct des snapshots ci-dessus reste distinct de ce parcours événementiel.

La signature locale avec journal anti-slashing, le transport, la qualification
complète des pannes du service, le pruning, les performances et la qualification
distribuée restent ouverts.
