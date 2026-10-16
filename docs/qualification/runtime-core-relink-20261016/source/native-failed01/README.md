Proposition SOURCE ONLY — réemploi Native avec le runtime de maintenance validé tôt

Ce contrôleur public n’a été importé ni exécuté par son auteur. Root seul lit les
descripteurs privés nommés dans le source, vérifie leurs pins et exécute les outils.
Le fichier réutilise les définitions publiques du contrôleur `3c62250a…/17840`
sous un nom de module distinct : son ancien `main` ne s’exécute jamais.

Invocation prévue par Root, après gel et revue :

```
python3 qualify-native-runtime-relink-root.py --revision HEAD_COMPLET_40_HEX --output-root E/current-native-runtime-relink-root-20261016-SUFFIX_NEUF
```

`E` désigne le dossier parent de cette proposition. La destination doit être un
enfant physique nouveau de E portant ce préfixe précis. Il faut Darwin arm64,
UID501/GID20, et un dépôt propre au SHA explicitement fourni. Les commandes Git
sont des lectures bornées, sans réglage de hooks, signature ou configuration.

Le contrôleur compare les arbres produit à la base bd11 : seule la modification
de `src/consensus_capella_runtime.cpp` est permise dans src/include/third_party/network.
Le nouveau runtime doit avoir les octets publics `57f18bb9…/28682`. Il reprend
les flags de son entrée unique dans la compile DB canonique conservée, fait un
vrai `-M`, admet les dépendances sélectionnées avant CXX, puis impose le vrai MD.
Le runtime est une nouvelle projection0400 de `git show` dans O/source/src,
fsync puis trois observations identiques sur2s avant M. L’ancienne9 du chemin R
reste dans l’autorité historique, sans suppression ni rafraîchissement ; ce chemin
ancien n’est pas relu comme un input actuel. Les includes canoniques vers S restent
les mêmes et chaque header atteint est comparé aux octets du Git courant.
Il copie le Core e04c/29262512 dans le nouveau dossier et remplace exactement le
membre `consensus_capella_runtime.cpp.o` : même ordre de 143 membres, 142 autres
payloads inchangés et membre neuf égal à l’objet compilé. Aucun Core global n’est
reconstruit. Le nouveau Core passe en0400 avant sa première admission.

Les sept programmes reliés sont Nano, Exporter, Importer, Audit, Capture, Tool et
Submit. Leurs objets restent strictement ceux de leurs producteurs conservés.
Main, Exporter, Audit et Capture exigent les anciennes9+SHA du fichier objet ET de
son MD, puis l’égalité du nouveau M et de ce MD. Importer, Tool et Submit gardent
exactement leur preuve antérieure : objet9+SHA et ensemble actual-MD conservé
dans le descripteur producteur. Aucun ancien fichier MD n’est réadmis ou présenté
comme retrouvé pour ces trois rôles. Chaque dépendance projet atteinte doit égaler
les octets du Git courant à son chemin producteur, et chaque fournisseur physique
doit déjà posséder son autorité retenue. Une lacune refuse ; aucun fallback SDK
ne transforme un hash ou une observation actuelle en ancienne9.

Chaque lien conserve le préfixe canonique et l’ordre cinq archives/deux objets
ASM ; seuls l’objet propre, la sortie et le Core sélectionné sont substitués.
Le Tool conserve sa macro de fixtures vers l’ancien source. L’Exporter conserve
sa macro vers R et son ancien `@rpath/libnano-test-sync-fault.dylib`, SHA8d8ddc…/34336.
Les six autres nouveaux programmes ne peuvent charger ce support. Les commandes
Mach-O et fournisseurs directs non système sont inspectés, liés aux anciennes9,
puis fencés ; les alias absents des anciennes tables ne sont que des observations
prospectives distinctes, jamais des alias historiques récupérés. Les vingt copies
de fixtures publiques associées aux deux macros sont comparées au Git courant.
Les bibliothèques du cache système et la fermeture récursive du loader restent
explicitement non requalifiées.

Le lot fait 8M, 1CXX runtime, un ar/ranlib, 7réemplois d’objets et 7liens. Les
huit CLI sont Nano `--help`, Nano `rpc` sans URL, cinq refus d’arité des helpers,
et l’Exporter avec le seul sélecteur `--preview-isolated-capella-identities` sans
seed. L’Exporter n’est JAMAIS lancé sans argument (ce serait son ancienne suite).
Ces chemins précoces n’ouvrent ni DB, seed, ledger, signature, réseau ou runtime.
Les nouvelles exécutables sont0500 avant leurs trois observations identiques sur
au moins2s ; la première9 n’est jamais rafraîchie. HOME et TMPDIR sont neufs et
doivent rester vides. L’environnement enfant est une allowlist sans DYLD/LD/NANO.

Le budget global est1100s, chaque commande au plus170s (CLI15s), et les processus
créés ont leur propre groupe avec TERM/KILL/join bornés en cas d’interruption.
Le finally vérifie les anciennes9/SHA et les produits même en échec ; le premier
refus est conservé, sans détails privés sur stdout/stderr. Les journaux/argv et
l’admission privée restent dans le nouveau dossier. Le résultat public ne publie
que résultats, compteurs, pins et étapes ; il engage le SHA/bytes de cette admission.

P2P ne lie pas nano_core : ce lot ne fait aucun M, lien ni exécution P2P. Root garde
son audit séparé des fichiers/aliases/fournisseurs existants. Le présent lot ne
rejoue ni RPC120, ni startup31, ni les anciennes suites ; il n’émet aucune nouvelle
admission de campagne et ne retargete pas d30e/les24assets historiques. Qualification
SDK globale, configure CMake complet, loader complet, duties, finalité et production
restent faux. Tous les résultats de cette proposition sont EN ATTENTE de Root.
