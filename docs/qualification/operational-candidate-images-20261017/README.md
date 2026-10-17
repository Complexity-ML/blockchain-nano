# Images candidates et contrôles opérationnels — 17 octobre 2026

Deux images d’exécution actuelles ont été assemblées, une par architecture,
sans recompilation C++ et sans téléchargement de base. Le succès du packaging
VPS et la vérification du contenu de l’image Mac sont des résultats distincts :
**la clôture du packaging Mac reste FAILED**, sa cause exacte n’est pas récupérée.
Aucun service n’a été remplacé, aucune duty reprise, aucune identité créée.
`productionQualified:false` reste explicite.

| Environnement | Image locale immuable | Taille | Résultat observé |
|---|---|---:|---|
| Docker Mac ARM64 | `sha256:8def3021a5f24e06a4d50d9f6fb6ff56310097aeff1ecf98e5aab561908b1f80` |273274744 octets|Deux exécutables/chargeurs vérifiés ; audit des licences et métadonnées réussi. Packaging final FAILED.|
| Docker VPS AMD64 | `sha256:f4d71ca230255a39bfdf5cec0eed38ffad37428a0da9c9b7344211736f0949e0` |252321914 octets|Packaging, clôture des sources et suppression du contexte PASS.|
| Mac natif |Pas d’image supplémentaire|—|Les assemblages C++ antérieurs sont conservés, sans nouvelle exécution dans ce lot.|

Les bases locales ARM `6617aab2…` et AMD `2e35327f…` ont été réutilisées.
Les images contiennent Nano/P2P et les licences, utilisent `10001:10001` et
leur commande par défaut est `--help`. Elles ne contiennent ni données de chaîne
ni clés de validateurs. Les helpers de qualification restent hors de ces images.
Une identité d’image et un `ldd` réussi ne qualifient pas le réseau, les duties,
tous les appels différés des bibliothèques ni la reprise de consensus.

## Sources et fichiers réellement sélectionnés

Le code opérationnel initial provient de
`72b0042b0dddb0ee3b487e5489291b6ff93c6e37`, poussé puis récupéré sur le VPS
par Git. Les binaires restent ceux construits depuis `d26e4744…`, avec la liaison
vérifiée à `7bd24dea…` : les arbres C++ et CMake sont identiques, la différence
étant un include portable dans un test. Le changement C++ effectif reste
`2095012b…`. [Liaisons et limites acquises](../rpc-runtime-assemblies-20261017/README.md).
Aucun marqueur historique de compilation n’a été inventé, aucun des 24 actifs logiciels admis
n’a été réorienté vers de nouveaux fichiers.

| Architecture | Nano SHA-256 | P2P SHA-256 |
|---|---|---|
|ARM64|`917082c9450c2fceedf72c2230805c9054a8bb11b21264bc89b92bba5f883e0f`|`b46a7eff684eb82b91ecb056c7e723fee84c5c70852bb5091904cd47dd00d03d`|
|AMD64|`45bd9c8f832f8ab178049927bbaf1eb3a2fc22d559a5b2863a433d77091c427f`|`b87645b74fb4b3139d4d9d9f43443e2afaff6b773375941a7fb624f113d88795`|

La preuve source conserve SHA-256 `376c0766…` /1533 octets. Le packager la
vérifie en intégrité seulement, sans analyser sa sémantique :
`sourceProvenanceSemanticallyVerified:false` ne devient pas true par présence
d’un fichier. Les liaisons C++ et les anciennes identités physiques sont
contrôlées séparément par Root. Le VPS conserve sept anciens actifs logiciels
strictement identiques, et la clôture Root ARM conserve ses six entrées retenues.
Cette dernière ne récupère pas rétroactivement les identités internes perdues
du contrôle de packaging ayant échoué.

## Nouveaux contrôles et refus conservés

Le moniteur refuse désormais les redirections HTTP ainsi que les enveloppes RPC
contenant simultanément `result` et `error`, même falsy. Dans Docker sans réseau,
l’ancien code produit19 échecs attendus et un contrôle positif ; le candidat
produit20 résultats verts (17 feuilles et3 groupes). Les21 anciens cas ne sont
pas rejoués. Les lecteurs sont injectés : aucune livraison d’alerte ni finalité
réelle n’est déduite. Deux refus du contrôleur de qualification (format de sortie,
puis métadonnées d’une nouvelle copie) et une erreur de syntaxe avant exécution
précèdent le résultat final ; ils ne sont pas des échecs du consensus.

La nouvelle entrée de packaging accepte deux fichiers physiques distincts avec
un descripteur épinglé indépendamment. Les9 scénarios initiaux passent avec
Docker injecté, sans vrai build. Le shell historique, sa recette, ses licences
et ses tests restent inchangés ; aucune ancienne suite C++/EVM n’est rejouée.

Sur Mac, la première commande de build est refusée avant assemblage : le client
isolé ne trouvait pas le Buildx déjà installé. Une configuration neuve raccorde
ce plugin, sans modifier l’ancienne configuration. Un seul assemblage réel
suit : ses contrôles ID/métadonnées, deux empreintes et chargeurs passent, puis
un guard final refuse `Artifact identity changed`. Il n’identifiait pas le
fichier et ses lignes internes ne sont pas conservées. **Ne pas attribuer
cet échec au fichier IID, au ctime ou à un autre fichier sans preuve.**
L’audit séparé réutilise les probes binaires/chargeurs liés à cet ID immuable et
vérifie en lecture seule les12 fichiers de licences/inventaire. Il n’effectue
aucun rebuild et ne transforme pas le refus précédent en succès.

Sur VPS, la première invocation Node échoue avant toute commande Docker faute
de `libatomic.so.1`. Un seul provider public de35120 octets est extrait de
l’image de dépendances déjà présente, hash `9d8a3446…`, avec comparaison FD/path,
empreintes et première admission stable. Il sert uniquement à l’outil Node sur
l’hôte : ni apt, ni téléchargement, ni modification du binaire Node ou des
providers C++ des images. La reprise effectue le premier assemblage AMD réel,
puis tous ses contrôles et clôtures passent. Les cinq fichiers de preuve
rapatriés ne contiennent ni clés, ni bases, ni capsules.

Un correctif ultérieur fournit le fichier et les champs concernés lors d’un
refus. Le nouvel IID temporaire seul obtient sa première autorité après trois
observations stables sur au moins2s, dans une fenêtre coopérative de30s ; les
premiers bytes/SHA restent immuables. Les sources déjà admises ne sont jamais
réadmises. Le scénario ajouté échoue sur l’ancienne version ; les10 cas candidats
passent ensuite une fois. Les9 précédents sont ici une régression pertinente,
car leur fonction partagée de lecture/guard a changé. Docker reste injecté,
aucune image réelle n’est reconstruite et l’échec ARM ancien reste inchangé.
Le module livré est épinglé `0d9bafa3…` /15520 octets ; son test
`076fb804…` /10677 octets. Ce correctif n’est pas une clôture réelle du packaging
ARM de l’image déjà construite.

## Limites de clôture

Le run GitHub38983351634 échoue avant toute étape pour facturation/plafond du
compte, avec core/network ignorés. Aucun rerun ni dépense. Ce n’est ni un échec
de compilation ni une CI verte.

L’autorisation `call_00fc…` appartient à la tentative02 déjà exécutée et FAILED.
La tentative03 reste en attente d’un accord distinct ; aucun seed, clé, domaine
ou devoir de validation n’a été créé ici. Les anciens journaux ne sont pas
réinitialisés. NAS/PID28722 et les anciens volumes restent intacts.

Le cycle complet entrée/sortie/retrait/sanction/reprise des trois propriétaires,
la finalité commune qui avance, les écarts WAN/partitions/charge, les procédures
complètes et la revue indépendante restent des critères ouverts. Les termes
Base–Nano/USDC/bridge/Doppler restent séparés, sans fonds réels ni vente publique.
Les quatre documents de production conservent leurs anciens corps byte-exacts.
