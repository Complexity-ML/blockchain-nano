# Candidat RPC AMD64 — délai global épuisé, échec terminal conservé

La qualification Docker AMD64 du candidat C++ `c6cdf780…` est **FAILED**. Root a
joint le dispatcher réel : session85431, exit1 après1 872,585s. Le worker disposait
de1 800s au total ; au premier CXX de `test_rpc_get_proof.cpp`, il ne restait que
0,753294s. `TimeoutExpired` est l'erreur terminale conservée. Il n'y a ni relance
automatique ni réussite des tests déduite des produits déjà construits.

Le HEAD documentaire d'exécution était `6183a799…`, le candidat séparé
`c6cdf780c68eab8b06306d76257bda9028928ea3`, et la base Core retenue
`1925eeeabc9f7d0aef41f2c3fe43c11c31f1f352`. Ces révisions ne sont pas confondues.

## Travail achevé avant la deadline

Le [rapport du worker](proof/failure-01/outputs/result-public.json) et
l'[audit Root](proof/failure-01/root-failure-audit-public.json) conservent :

| Observation achevée | Quantité |
| --- | ---: |
| Dépendances indépendantes `-M` |152 :143 Core,8 runtime,1 premier test |
| Compilations C++ terminées |52 :44 Core et8 runtime |
| Mise à jour d'archive |1 |
| Liens runtime / contrôles loader |8 /8 |
| Compilations de tests terminées |0 |
| Tests C++ / invocations CLI |0 /0 |

Les huit programmes liés sont Nano, BootstrapAudit, BootstrapImport,
CheckpointExporter, DepositAudit, LifecycleSubmit, SlashingFixture et
StoppedCapture. Aucun n'est proclamé qualifié par une invocation CLI ou un test
fonctionnel dans ce lot. Le premier CXX de test interrompu n'est pas compté parmi
les52 compilations achevées ; une sortie incomplète/non admise n'est pas réutilisable.

La [preuve d'archive](proof/failure-01/outputs/core-member-proof.json) conserve143
membres,44 sélectionnés et99 hors sélection inchangés ; huit corps diffèrent
réellement. Le Core produit est `3ef6c2cd…` /40 988 476octets. C'est une archive
observée dans une enveloppe FAILED, pas une promotion de candidat globale.

## Clôtures après l'échec et après cleanup

Le [résultat hôte](proof/failure-01/result-public.json) conserve l'erreur originale,
les anciennes entrées hôte inchangées, la clôture complète après cleanup, l'image
finale AMD64 conforme et le conteneur propre absent. Aucun rebuild ni pruning
d'image/volume. La [clôture hôte](proof/failure-01/host-final-fence-public.json)
vérifie2 571 fichiers sélectionnés sans rafraîchir les anciennes autorités.

L'[audit post-cleanup des produits](proof/failure-01/retained-product-postcleanup-audit-public.json)
vérifie265 lignes originelles neuf champs/SHA : zéro produit changé. Il se lie
à l'autorité privée originale, dont seuls le chemin et le pin public sont
rapportés ; son contenu reste sur le VPS. Les anciennes lignes ne sont pas
remplacées par des observations nouvelles. L'enveloppe d'échec reste FAILED et
les sorties non admises ne deviennent pas admissibles par cet audit.

Le [catalogue de lecture](proof/failure-01/readback-catalog-public.json) épingle
les six rapports publics retournés. Leur arborescence relative est conservée,
avec les trois fichiers worker sous outputs/. Les logs stdout/stderr hôte,
l'argv public et la [sortie du dispatcher](proof/dispatch-01/dispatch-exit-public.json)
sont conservés séparément. Aucun contrôle privé, SDK, clé, journal ou capsule
n'est inclus dans ce paquet.

## Préparation01 conservée

Avant la campagne de compilation, la
[préparation de base01](proof/preparation-01/prepare.stderr.log) a terminé exit1
sur une SyntaxError de l'expression Python `1276if ... else1271`. Ce refus de
préparation est distinct de la deadline AMD ; il n'a lancé aucun C++/Docker.
Son stderr, son stdout vide et son reçu de sortie restent copiés à l'identique.
La préparation corrigée ne réécrit pas ce refus.

## Sources et portée de cette publication

Le [descripteur public](source-descriptor.json) identifie exactement le host3be73f,
le worker V2e9bfc4 et le helper aa20f1. Les corps, MF et README de ces paquets
source ne sont pas inclus ici : aucun paquet partiel n'est présenté comme un
ensemble complet. Les contrôles privés et autorités détaillées restent exclus.
L'auteur de cette publication n'a exécuté ni ces sources, ni C++/test, ni Docker
ou SSH ; il a copié seulement les preuves publiques et produit leurs métadonnées.

Aucun vecteur de preuve AMD n'a été émis puis vérifié indépendamment ; **zéro
qualification des24 vecteurs AMD**. Aucun serveur/RPC, duty, signature, reprise ou
nouvelle finalité n'est qualifié. Les acquis Native/ARM conservent leur portée et
leurs propres échecs ; ils ne rendent pas ce lot AMD réussi. Le candidat C6 n'est
pas importé sur main par ce dossier et aucune qualification globale n'en découle.

Une éventuelle continuation devra partir des autorités originales strictes et
d'un budget explicite, conserver cet échec et éviter les recompilations déjà
achevées lorsque leur réemploi est démontré. Rien n'est automatiquement relancé
ou autorisé par ce rapport. Le lot contrôleur terminal distinct, manifeste976…,
reste intact et ne qualifie pas ces binaires.

Les quatre documents normatifs recevront seulement un append sur leurs corps
actuels, avec6183 comme ascendant minimal et sans futur HEAD inventé. Pas de
reconstruction d'image ou redémarrage pour ces seuls documents. Campagne03 FAILED
consommée, aucune04 autorisée ; consensus/cycle/opérations restant, CI globale,
revue indépendante et relation Base–Nano/bridge/termes Doppler restent ouverts.
Aucun EIP/fork/profil ni production promu, aucun fonds/vente/déploiement public.
