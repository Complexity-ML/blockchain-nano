# Clôtures terminales indépendantes — qualification ciblée du contrôleur

Le contrôleur JavaScript V2 passe sa qualification Root réelle : **quatre
analyses syntaxiques, quatorze nouveaux groupes et un groupe de garde modifié**,
tous exit0, sans skip. Le groupe de garde exerce trois refus du vrai driver avant
l'accès aux chemins seed/output/réseau absents. Les cinq groupes d'admission
inchangés ne sont pas rejoués. Ce lot qualifie l'outillage et son ordre de clôture,
pas le cycle réseau, un nouveau runtime C++, ni la production.

## Sources et preuves

* Base publique minimale : Git `6183a799df411edfaecd549bdca43bad20b51508`.
  Le paquet source V2 [conservé octet pour octet](proposal/manifest.json) comprend
  quatre fichiers proposés, source.patch contre cette base et delta V1/V2.
  Son manifeste reste `12b82cf3…` /3 503 octets ; ses huit payloads sont inchangés.
* [Résultat Root](proof/result-public.json) : 50 fichiers publics dans une
  projection neuve ; sources et Node sélectionnés inchangés sous leur clôture
  neuf champs/SHA. Les six processus enfants ont effectivement terminé exit0 ;
  Root a également joint le contrôleur principal, session38962 exit0.
* [Nouveaux scénarios](proof/terminal-tests.stdout.log),
  [sortie terminale](proof/terminal-tests.exit-public.json) : 14/14, aucun skip.
* [Garde du driver](proof/driver-guard.stdout.log),
  [sortie terminale](proof/driver-guard.exit-public.json) : 1/1, trois refus
  d'autorisation, aucun skip. Ce n'est ni six groupes rejoués ni 20/20 annoncé.
* Les quatre sorties de parse, stdout/stderr et les pins de la
  [projection publique](proof/projection-public.json) sont conservés séparément.
  Les autorités privées de matériel ne sont pas exportées par ce dossier.

Le README du paquet source décrit les vérifications à effectuer au moment où
l'auteur l'a gelé : il reste historique et n'est pas réécrit pour annoncer la
réussite Root ultérieure. Le présent rapport et les reçus Root portent ce statut.

## Ce que le lot corrige

Une erreur de clôture lifecycle ne court-circuite plus les contrôles indépendants
sources/binaires, les nettoyages et leurs clôtures finales. La première erreur
reste l'erreur terminale ; les erreurs suivantes et celles du journal d'échec
gardent des résultats distincts.

Les deux jonctions propres sont confirmées seulement après retour non forcé et
fermeture du handle. Un échec, même après fermeture, interdit les captures ; un
ancien reçu ne remplace pas un arrêt réussi, et Docker doit être observé arrêté
sous son identité propriétaire. Les audits des journaux exigent les trois
captures arrêtées réussies. Les engagements individuels précèdent la publication
commune, elle-même requise avant la clôture lifecycle.

Les contrôles originaux source/copie, artefacts, index et fichiers connus restent
stricts et indépendants. Ils ont lieu après les dernières sondes logiciel et leur
nettoyage. Seules leurs nouvelles opérations readonly sont ensuite nettoyées sous
les mêmes gardes de nom/image/UUID, puis les absences et les clôtures held/output
sont observées. Une opération encore active impose FAILED ; aucun troisième
passage logiciel, rejeu, reconstruction ou pruning n'est ajouté.

Les mutations de pins/propriétés après cleanup, les jonctions forcées ou levant
une erreur après fermeture, les captures incomplètes et les échecs de publication
sont injectés dans le vrai bloc finally extrait. Les adapters sont synthétiques.
Le groupe d'autorisation du vrai driver importe sa fermeture publique puis refuse
avant toute ouverture de seed/output/réseau ; il ne lance aucun nœud.

## Refus conservés, sans requalification rétroactive

* [V1 refusée à la revue source](refusals/terminal-controller-v1-source-review-refusal-root-20261018.json) :
  fixture sans path, absence de mémoire des jonctions propres, frontière avant
  les dernières sondes et pin CI du driver ancien. Aucun parse/test exécuté ; le
  parent V1, manifeste `81f87330…` /2 200 octets, reste intact.
* [Premier consommateur Root refusé avant exécution](refusals/terminal-controller-qualifier-prestart-refusal-root-20261018.json) :
  contrôles d'octets connus et clôture de première erreur incomplets.
* [KeyError du consommateur39e495d7](refusals/terminal-controller-v2-root-qualification-prestart-refusal-20261018.json) :
  nom de champ de manifeste erroné, aucun namespace créé, zéro parse/test.
* [Projection01 non admise](refusals/terminal-controller-v2-root-projection-admission-refusal-20261018.json) :
  vraie session10874 exit1 ; les trois premiers échantillons neuf champs
  différaient malgré SHA/taille connus conformes. Acteur et champs divergents
  non retenus ; zéro parse/test. Aucune ancienne autorité n'a été rafraîchie.

La projection02 est une nouvelle première admission prospective, pas la réparation
des observations ni des identités de01. Ces refus ne sont pas des échecs C++.

## Limites et livraison

Zéro C++, SDK privé, image, commande Docker/SSH, réseau réel, clé ou signature
n'est exercé par cette qualification. Les adapters opérationnels réels et leur
cycle complet restent à qualifier sous une autorisation dédiée. La campagne03
reste FAILED et consommée ; aucune04 n'est autorisée. Son acteur de variation
ctime reste inconnu ; ce lot ne prouve pas une panne du consensus ni sa clôture.

Aucun EIP/fork/profil, admission runtime, duty, finalité nouvelle, reprise ou
production n'est promu. Revue indépendante, CI globale/facturation, écarts de
consensus/ressources/opérations et relation Base–Nano/bridge/termes Doppler restent
ouverts. Le nouveau fichier de scénarios n'est pas enregistré par un changement
CMake/workflow dans ce lot ; seule la garde déjà sélectionnée conserve ses pins
cohérents. La réussite ciblée ne constitue pas une réussite GitHub CI globale.

Root importera les quatre sources et ce dossier seulement après la fin du lot
AMD encore distinct. Le plan conserve le préfixe documentaire actuel de chaque
fichier normatif, exige au minimum l'ascendance6183 et ne présume aucun futur HEAD.
Aucune image n'est reconstruite et aucun service n'est redémarré pour ces seuls
outils JavaScript et documents. Aucun fonds, vente ou déploiement public.
