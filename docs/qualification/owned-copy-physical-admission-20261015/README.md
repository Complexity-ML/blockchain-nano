# Copies de maintenance : admission physique avant ouverture RocksDB

Le garde partagé refuse désormais les fichiers de base à plusieurs liens physiques, les liens symboliques et les entrées spéciales avant une ouverture RocksDB mutable. Le helper de migration vérifie aussi avant sa lecture initiale et répète le contrôle lors du passage RO→RW. Le marqueur d’usage est lié à un FD borné et à un seul lien ; son texte historique reste identique.

Quatre fichiers fonctionnels correspondent exactement aux propositions gelées : header partagé, insertion dans le CLI de migration, nouveau consommateur et registration CMake. Le CPP de compaction reste inchangé et reçoit le garde par le header. Le README CLI reçoit une section opérateur datée, avec tout son corps précédent conservé. Aucun format durable, règle de consensus, profil, frais, ABI du core ou API RPC ne change. Le helper de migration reste non installé ; les cinq exécutables installés restent nano, trois outils de maintenance et nano-p2p.

| Résultat public | ARM64, preuves JSON directes | AMD64, sorties ROOT filtrées |
|---|---|---|
| CTests ciblées | 2 PASS | 2 PASS |
| Consommateur physique | 9 groupes, 10 lignes | 9 groupes, 10 lignes |
| Appels CLI | 21 : 3 succès, 18 refus | 21 : 3 succès, 18 refus |
| Signatures nouvelles | 0 | 0 |
| RSS propre du consommateur | 16768 KiB | 18172 KiB |
| Temps consommateur | 705 ms | Inconnu dans la sortie filtrée |
| Temps contrôleur d’unité | 7123 ms | 30844 ms |
| Usage final du volume | 212 KiB | 212 KiB |
| Baseline anciens CLI | 2 acceptations, campagne séparée | Non exécutée, non requise |

Le build ARM rapporte 7 compilations CXX, 8 liens et 6 couples M/MD véritables. KZG a réellement été recompilé ; ROOT retrouve pourtant les mêmes octets pour les 143 objets, libnano_core.a et nano que dans le parent RPC qualifié. Cela ne signifie ni zéro recompilation ni de nouveaux tests core. AMD prépare une fois en 48258 ms, puis configure/construit une fois en 149731 ms avec 6 couples M/MD et parents inchangés. Ses nombres totaux de compilations/liens et ses octets binaires ne sont pas exportés ni inférés de ces compteurs. Les admissions de compilation/reliage restent distinctes des résultats CTest. Aucune image n’est construite.

La baseline ARM emploie deux anciens CLI et un fichier supplémentaire ignoré par RocksDB, mais physiquement partagé. Les deux l’acceptent ; les originaux gardent leurs octets et neuf champs matériels exacts. Les copies de travail sont volontairement compactées ou migrées. Aucune corruption de la source ni qualification du correctif n’est déduite de ces acceptations. Le consommateur baseline dure 39 ms, son contrôleur 4988 ms, RSS 16760 KiB et usage final 12 KiB. Cette mesure reste distincte de la campagne corrigée.

Le sélecteur corrigé accepte trois appels sur copies indépendantes, dont une migration puis son rejeu idempotent, et refuse 18 appels dangereux. Chaque refus conserve toute l’arborescence du cas : source, copie, requête, marqueur et alias externes, avec SHA des fichiers réguliers et neuf champs matériels exacts hors atime. Une racine 0755 et un sous-répertoire ordinaire restent acceptés. Les payloads de checkpoint sont synthétiques et opaques ; ce test ne qualifie aucun checkpoint Capella authentifié. Le contrat borne le RSS propre à 128 MiB et le temps consommateur à 120 s ; le timeout CTest est 130 s.

```sh
cmake --build BUILD --target nano-owned-copy-physical-admission-tests nano-bootstrap-archive-maintenance nano-capella-duty-readiness nano-install-tests -j2
ctest --test-dir BUILD --output-on-failure -R '^(storage_owned_copy_physical_independence|installed_runtime_package_loadability)$'
```

L’installation exige déjà le nano, le nano-p2p et les SDK/libs courants réellement liés par CMake. Le test d’installation effectue une vraie installation, puis cinq commandes d’arguments/help sans DB ni démarrage de nœud. Elles s’ajoutent aux 21 appels du consommateur physique. Les tests ROOT utilisent un volume Linux neuf exécutable pour TMPDIR, les sources/build/réseau en lecture seule et un seul fichier de manifeste d’installation neuf en surcouche RW. La mémoire Docker est de 512 MiB ; les 128 MiB du contrat concernent le RSS propre du consommateur. Les 212 KiB `du` sont une mesure finale, sans prétention de pic ni quota disque dur. Les entrées protégées sont inchangées et les conteneurs/volumes possédés absents après les campagnes, selon les preuves ARM et les sorties AMD filtrées.

L’opérateur doit garder les copies arrêtées et leur namespace exclusif jusqu’à destruction des DB. Le garde ne ferme pas une mutation par un autre processus du même UID ou privilégié après sa dernière vérification ; le marqueur n’est pas une autorisation cryptographique. Les audits canoniques externes de migration et un timeout opérateur restent requis. Aucun plafond arbitraire de fichiers/validateurs ou assouplissement de quota n’est introduit.

Les refus ARM de préparation et de collecte restent conservés dans history.json : les quatre ctime de surcouches neuves ont une admission distincte, sans rafraîchir les anciennes entrées ; le helper configure réellement avec exit0 mais refuse ensuite sa publication, qui est admise séparément. La cause des changements de ctime reste inconnue. Le finding statique du VPS V1 concerne seulement l’admission des nouvelles sorties ; V2 le ferme avant le premier vrai test, sans échec d’unité inventé ni rejeu du prepare/build.

Tous les corps historiques restent entiers sous quatre préfixes additifs ; CSV EIP et documentation API RPC sont byte exacts. Les anciens refus privés/CLI/WAL, Anvil FAILED 11/16 et nuances RPC sont conservés. Originaux Mac/NAS, V37 FAILED, anciennes clés arrêtées, restore complet, duties/readiness/finalité commune de trois nœuds, WAN, perte électrique physique, strict 4 MiB, CI bloquée par facturation, audit externe et autres critères non démontrés restent ouverts. Aucun ancien probe privé n’est relu et aucun suivi NAS n’est demandé. Aucun fork, exploitation, RC complet ou production n’est qualifié.

Ce petit paquet contient le contrat, les résultats publics compacts, le patch, les préfixes publiés et les bindings de sources/histoire. Les sources importables et quatre documents composés sont livrés séparément pour l’import exact. Maps complètes, SDK, ELF, logs VM, contrôleurs ROOT et binaires externes restent hors du paquet.
