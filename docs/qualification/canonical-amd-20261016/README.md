# Qualification canonique AMD64 — 16 octobre 2026

Le code public Git `712b8357d05f09cc28df6e04920c9955e59b7b69` a produit sur le VPS neuf artefacts AMD64 identifiés : l’archive Core, sept exécutables Core et `nano-p2p`. L’essai 06 et la lecture indépendante des identités ont réussi. Cela qualifie leur construction et le test ciblé de lecture des validateurs ; aucun nœud ni réseau de consensus n’a été démarré.

## Preuves acquises

- Les 42 767 fichiers publics suivis (1 028 396 270 octets) correspondent aux objets Git vérifiés. La préparation source originale et ses neuf champs de métadonnées avec SHA restent inchangés.
- Core contient 143 objets/membres d’archive. Les sept consommateurs Core et le consommateur réseau ont été construits par les essais antérieurs ; l’essai 06 conserve leurs sorties originales hors du sous-dossier `Testing` déclaré.
- La fermeture vérifie 151 compilations Core/consommateurs et sept compilations de dépendances supplémentaires : 158 sondes indépendantes `-M` correspondent aux dépendances `-MD`, soit 157 C/C++ et une assembleur. Deux recettes LLVM sont vérifiées séparément.
- CMake n’avait pas produit de dépendances `.o.d` pour l’unique objet assembleur concerné. L’essai 06 compile une seule copie dans un nouvel emplacement avec de vraies dépendances MD, exige des octets identiques à l’objet original et compare son `-M` au nouveau MD. L’absence du MD original reste explicite. Aucune recompilation Core ou consommateur n’a lieu en 06.
- Un seul CTest API, auparavant non exécuté dans cette campagne, réussit : cinq groupes, neuf statuts, 21 lectures et 16 refus attendus ; deux signatures BLS de fixtures, zéro signature d’audit, sans base de données, duties, RPC de nœud ou réseau de consensus.
- Les neuf artefacts sont identifiés dans [la preuve publique](evidence/artifact-identities.public.json). Le producteur a conservé trois observations stables sur 2 522 ms ; la lecture indépendante retrouve ses métadonnées/SHA originaux inchangés. Les contrôles de l’image SDK en lecture seule passent avant/après qualification. Les lignes SDK ont été enregistrées après la construction, ce qui demeure explicite.

L’image de dépendances existante `fd9b…` est utilisée hors réseau, sans image construite/téléchargée et sans lecture des anciennes configurations ou cartes SDK privées de l’hôte. Cette voie publique est distincte de l’audit privé antérieurement refusé. La limite configurée est de deux CPU et 3 Gio. Le pic de **72 187 904 octets** concerne seulement le conteneur de fermeture/assembleur/test 06 ; il ne mesure ni la construction complète, ni un nœud, ni une charge réseau. Le temps 06 est de 167 925 ms à distance (171 015 ms côté contrôleur). Son conteneur est absent à la fin.

## Échecs conservés

| Essai | Terminal observé | Cause et limite de preuve |
| --- | --- | --- |
| 01 | Échec à la création du conteneur, sans C++ | Le code révélait une variable locale `name` masquant le nom du conteneur. `UnboundLocalError` est une inférence de source ; le terminal ne publie pas la classe d’exception. |
| 02 | Échec de préparation source, `RuntimeError` | Le diagnostic Root a établi une réserve disque dupliquée trop élevée ; aucune compilation. |
| 03 | Échec pendant Core, `FileNotFoundError` | 138 objets partiels ont été observés ensuite. Une disparition transitoire dans le scanner d’espace mutable est inférée ; aucune cause physique n’est affirmée. |
| 04 | Compilation terminée puis échec de fermeture, `FileNotFoundError` | Le diagnostic Root établit l’absence du MD assembleur original. Aucun test API n’avait encore été exécuté. |
| 05 | Refus de préparation, `RuntimeError` | Les caches CMake emploient les noms `clang`/`clang++` ; la garde précédente exigeait à tort des chemins absolus. Refus avant conteneur, assembleur ou test. |
| 06 | Réussite | Fermeture, copie assembleur et premier test API ; les échecs précédents demeurent des échecs. |

Les six terminaux typés sont conservés byte pour byte sous `evidence/`. Les causes issues des diagnostics Root sont distinguées des classes publiées par ces terminaux. Les anciennes sources et sorties restent conservées dans la campagne originale ; elles ne sont pas recopiées ici.

## Limites et suite

`sourceAdmissionDigestPreviouslyRetained` et `admissionDescriptorDigestPreviouslyRetainedOffsite` restent **false**. Les valeurs originales conservées ont été comparées strictement ; les digests retenus après qualification ne créent aucun engagement historique rétroactif.

Les trois fichiers de `source/` sont les opérateurs publics exacts de l’essai 06 et de la lecture d’identités : une continuation de la préparation publique existante, son répartiteur SSH et l’inspecteur des sorties, pas un constructeur autonome destiné à un autre environnement. Ils nécessitent leurs artefacts de campagne conservés ; ce paquet ne contient aucun binaire, carte SDK, configuration privée ou journal brut.

Le statut de production reste **non qualifié** et l’objectif reste actif. Cette preuve ne démarre pas les duties, ne prouve pas une reprise de l’ancienne identité, un cycle complet de validateurs, une finalité commune à trois propriétaires, un transport SST Mac–Linux, une résistance sous charge, une CI verte ou de nouveaux forks/EIP. Les admissions de programmes de la prochaine campagne, les permissions opérationnelles privées, la reprise et le cycle réseau complet restent à fermer. Les anciennes preuves et les anciennes limites de provenance sont conservées dans les quatre feuilles de route.
