# Migration Bellatrix vers Capella

Deux API distinctes : `upgrade_capella_state` authentifie et transforme un
état Bellatrix ; `prepare_capella_fork_slots` applique un calendrier fourni
par l'appelant, avance Bellatrix jusqu'à la frontière, migre, puis continue
en Capella. La migration survient même sans bloc au slot du fork.

La version précédente est conservée, la version courante et l'époque sont
remplacées. Le header conserve tous ses champs et reçoit une withdrawals_root
nulle (pas la racine SSZ d'une liste vide). Curseurs de retrait et résumés
historiques sont initialisés à zéro/vide. Les autres champs sont conservés.

Qualification : 14 couples officiels pre/post, comparaison SSZ intégrale,
fixtures et preset Bellatrix épinglés par SHA-256. Référence consensus-specs
v1.4.0, `specs/capella/fork.md`; corpus consensus-spec-tests commit
`194a91fd9e6f97118156421aac4c80e6e47a9a7f`, minimal/capella/fork/fork.
Les paramètres communs sont ceux déjà épinglés par les helpers de test.

Le scénario programmé migre à l'époque 1, puis avance trois slots sans bloc.
Il compare le chemin direct au chemin découpé et contrôle la racine Capella
mise en cache à la frontière. L'entrée exactement à la frontière est aussi
vérifiée avec un budget de zéro slot. Dix refus couvrent calendrier, budget,
authentification, version, overflow, double migration, header futur, marqueur
d'époque incohérent, configuration et cardinalité balances/validateurs.
Trois annulations (début, milieu, fin) laissent le parent inchangé.

Limites : candidat mémoire, pas d'activation réseau ni commit durable. Le
premier payload Shanghai prolongeant un parent Paris reste à raccorder : le
pont d'exécution impose actuellement le même profil au parent. La composition
avec les blocs signés, Store/fork-choice/réacteur, reprise et tests distribués
à travers le fork restent ouverts. Aucun EIP n'est clôturé par ces tests.
