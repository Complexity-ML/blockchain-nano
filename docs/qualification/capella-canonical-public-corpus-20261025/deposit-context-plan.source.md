# Dépôts : contexte froid authentifié pour l’audit canonique

SOURCE uniquement, base `9fb` annoncée par Root. Aucun Git, programme, test,
conteneur, réseau, DB, donnée privée ou fichier R modifié. Ce plan ne diagnostique
pas le timeout AMD : les marqueurs de phase restent nécessaires avant une
correction de performances. La borne de ressources auxiliaires reste OUVERTE.
Le profil concerné est `native_deposit_votes` de Nano.

## Ce qui croît et ce qu’on doit pouvoir demander

`src/consensus_capella_deposits.cpp:49–58` construit à chaque pruning les nouveaux
ancêtres puis concatène TOUT l’ancien vecteur. Sa validation relie chaque header
Beacon et chaque header d’exécution jusqu’au `network_anchor` original, et lie
chaque `(executionHash,count,depositRoot)` au préfixe authentifié de l’histoire.
Le nombre de records SSZ chauds ne borne donc pas ce vecteur de métadonnées.

Trois recherches doivent conserver exactement leur résultat :

| Appel actuel | Question à servir depuis le contexte disque |
|---|---|
| `capella_native_deposit_vote`, dépôts:19–26 | Dernier ancêtre du parent avant le début de la période, avec le même fallback à l’ancre originale. Un curseur de période suffit à accélérer cette question, sans devenir une autorité. |
| `find` / `vote_known`, dépôts:9–16,28 | Le tuple COMPLET root/count/hash appartient-il à CETTE ascendance ? Une racine seule, un hash seul ou un tuple d’une autre branche ne suffisent pas. |
| `capella_native_deposit_operations`, dépôts:29–43 | L’état SSZ sélectionne quel ancien tuple, puis quelles preuves ordonnées de dépôts sont obligatoires ? Le curseur consommé peut précéder la fenêtre chaude. |

Il n’existe pas de prédicat d’expiration dans ces recherches. Effacer tous les
ancêtres sauf N ou sauf une période pourrait refuser une sélection ancienne
encore admissible. Aucune limite de ce type n’est proposée.

La deuxième croissance est distincte : `consensus_deposit_history.cpp:25–59`
conserve les feuilles, leurs données et les sous-arbres partagés. L’ajout copie
un chemin de profondeur32, mais la racine courante conserve l’histoire de TOUS
les dépôts. `authenticated_prefix` reconstruit le chemin de frontière de
profondeur32 ; il ne libère pas les sous-arbres qu’il partage. `snapshot` et
`from_snapshot` (:89–107) matérialisent/recréent la liste entière. Borner le
vecteur d’ancêtres seul ne borne donc ni cet arbre ni une restauration ancienne.

## Plus petit composant cohérent à implémenter ensuite

Créer **un `CanonicalDepositContext` local à l’audit**, avec un index append-only
et un cache explicitement borné en records/octets. Il remplace le préfixe froid
du vecteur par des références ; la fenêtre chaude garde ses records habituels.
Il fournit les trois recherches ci-dessus et le chargement des chemins de preuve.
Ce n’est ni un nouveau moteur EVM/consensus ni une autorité de finalité.

1. Durant la seconde passe, enregistrer seulement les tuples de blocs DÉJÀ admis
   par la transition complète et `append_verified_execution`/`validate_execution`
   (histoire:69–86). L’enregistrement lie domaine/ancre, ordinal, racines Beacon
   et parent, slot, hash/parent/numéro d’exécution, count et depositRoot. Son contenu
   et ses liens sont engagés par des hashes/longueurs connues. L’index112 de la
   première passe ne contient pas ces tuples et n’authentifie pas à lui seul la
   vue du contrat : il peut seulement localiser les archives canoniques.
2. Servir les ancêtres pré-fenêtre depuis ce préfixe vérifié, jamais depuis un
   simple `block_by_hash` ni une métadonnée fournie par le peer. Un curseur du
   dernier tuple avant la période réduit les scans ; toute recherche exacte reste
   bornée/annulable et liée au parent de branche. Une entrée manquante/corrompue,
   un cache plein de pins ou un budget dépassé refuse ; aucun fallback vers le
   vecteur complet ou un tuple auto-authentifié.
3. Pour borner aussi les données de l’arbre, utiliser le MÊME composant pour des
   nœuds/feuilles immuables adressés par contenu. Garder la frontière d’ajout et
   les chemins loués dans le cache, charger les autres à la demande. Recalculer
   chaque hash de nœud jusqu’à la racine attendue, puis le mix-in du count ; les
   données de feuille et les33 éléments de preuve gardent l’encodage actuel.
   Les sous-arbres froids ne deviennent pas des `shared_ptr` vers toute l’histoire.
   Sans ce point, annoncer seulement « ancêtres bornés », pas « auxiliaires bornés ».

Réutiliser `CapellaCheckpointProvider` pour la durée de vie/fermeture et les
callbacks RO hash+longueur ; reprendre les règles de leases/cache sans éviction
des pins de `CapellaColdRecordCache`. Mais ne PAS employer le loader actuel tel
quel : `consensus_capella_cold_records.cpp:14–19` reconstruit lui aussi toute
l’histoire via `ExecutionDepositHistory::from_snapshot`. Les nouvelles petites
références de contexte/nœud doivent être validées sans décoder un record complet.
Les références n’autorisent pas l’écriture de la DB auditée ; le backend audit
reste un scratch neuf hors DB, avec budget disque déclaré et disposition privée.

L’identité/ascendance est validée une fois à l’admission du préfixe, puis chaque
référence est relue contre son engagement ; déplacer l’ancre ne réapprend aucune
autorité. La période, les anciennes données sélectionnées et les preuves restent
équivalentes aux fonctions actuelles. Le provider doit faire partie du contexte
de replay partagé, sans registre global ni curseur mutable commun aux branches.
Un changement du layout public des stores impose une vraie fermeture de rebuild
des appelants ; on ne réutilise pas aveuglément les objets historiques.

## Formats et qualification : choix explicites, pas de migration implicite

Pour le premier raccord AUDIT SEULEMENT, un sidecar de processus peut laisser
les formats persistés inchangés : les anciens snapshots restent lus/validés selon
leur version, et l’état de replay avec références n’est jamais publié/sérialisé
comme un ancien snapshot inline. Une ancre ancienne non vide exige elle aussi
une importation en flux ou une admission mémoire explicite ; la parser entièrement
en JSON annulerait la borne. Refuser ce cas non pris en charge, sans allégation
de compatibilité complète, est préférable à une expansion cachée.

Pour étendre ensuite ce mécanisme aux checkpoints de nœuds, une nouvelle version
LOCAL explicite est nécessaire. `capella_store_snapshot.cpp:153–158,227–229`
encode/restaure aujourd’hui `depositAncestors` inline en `nano-capella-store-v3` ;
les références froides enveloppées conservent ce tableau. L’histoire
`nano-execution-deposit-history-v1` contient toujours la liste entière. Préserver
les lecteurs/bytes anciens ; ne pas réinterpréter ces champs comme des références.
Publier les nouveaux fragments/références par le batch CAS atomique de la DB
primaire, puis suivre le protocole existant de publication/reprise primaire et
témoin ; aucune transaction atomique entre deux DB distinctes n’est supposée.
La migration doit lier les nouveaux engagements. Aucun ancien reçu, token de
restauration ou journal de signature n’est réinitialisé par ce plan.

Qualification minimale future : histoire plus longue que la fenêtre avec des
dépôts NON NULS, sélection et preuve d’un tuple ancien hors fenêtre ; comparaison
exacte votes/opérations/racines avec l’oracle actuel ; branche sœur au même count,
root ou slot refusée ; trous/ordre/count/root/code/proofs altérés refusés ; leases
concurrentes, fermeture du provider, annulation et capacité avant croissance sans
mutation DB ; restauration ancienne et nouvelle version séparées si elles sont
livrées. Mesurer séparément records/octets chauds, entrées disque, feuilles et
RSS réel. Le nombre de dépôts est une donnée de test, pas une nouvelle limite
de validateurs ou de protocole. Pas de nouveau gate arbitraire, relâchement de
budget, campagne05, performance ou production qualifiée par ce document.
