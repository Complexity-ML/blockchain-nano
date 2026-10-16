# Rejeu SST ciblé sur un volume Linux

Source uniquement, sans exécution par l’auteur. Ce runner ne compile rien et ne rejoue ni startup ni anciennes suites. Il lance une seule fois le binaire déjà qualifié `nano-owned-copy-physical-admission-tests --pruning-sst-reclaim`, qui appelle son CLI canonique `/outputs/nano-storage-maintenance` trois fois.

Root fournit un contrôle borné16KiB, lié extérieurement par SHA256/octets, avec exactement :

`{format:"nano-pruning-sst-disk-runner-input-v1", owner, imageId, dockerArgv, dockerEndpoint, oldOutputs, outputRoot, programs}`

- `owner` vaut `mac-docker` (501:20, imagee30/ARM64) ou `vps-docker` (1000:1000, imagefd9/AMD64), avec leurs IDs complets fixés dans le source.
- `dockerArgv` est le préfixe `["/chemin/physique/docker"]` ou `["/usr/bin/sudo","-n","/usr/bin/docker"]`. `dockerEndpoint` est un socket unix explicite. Le runner ajoute un répertoire `--config` neuf et transmet seulement PATH/HOME/LANG/LC_ALL ; Root conserve séparément les anciennes fences des utilitaires Docker/sudo/Python et du source runner.
- `oldOutputs` est `{type:"volume",name,labels:{...}}` pour ARM ; nom, Driverlocal et tous les labels attendus sont contrôlés AVANT montage RO avec `volume-nocopy`. AMD peut fournir `{type:"bind",path}` physique RO. Aucun ancien volume n’est créé, initialisé, supprimé ou écrit.
- `outputRoot` est un chemin absolu neuf, parent physique possédé par l’utilisateur, hors R ; logs, contrôles de commande et reçus nouveaux sont privés0600.
- `programs` contient exactement les deux feuilles ci-dessus, chacune `{materialGNU9:[neuf chaînes originales],sha256,bytes}`. L’ordre est dev,ino,modehex,nlink,uid,gid,size,mtime,ctime, sans atime. Root extrait ces lignes de l’ancienne admission authentifiée ; le runner ne reconstitue ni rafraîchit aucune ancienne ligne.

Le runner inspecte l’image immutable sans pull/build, puis crée un volume local UUID et le contrôle avant montage. Un conteneur setup avec rootFSRO/networknone/cap-dropALL+CHOWN vérifie le volume vide, effectue chmod0700 AVANT chown vers l’owner. Le conteneur test a cap-dropALL, no-new-privileges,2CPU/3GiB/128pids ; /outputsRO et /tmp sur le NOUVEAU volume, `volume-nocopy` partout. Il vérifie les sources/flagsRW réels des montages Docker et exige `stat -f -c %T /tmp` différent de tmpfs/ramfs. Aucun tmpfs n’est demandé.

Le wrapper Perl lit les deux exécutables par FD régulier O_NOFOLLOW, vérifie nlink1/owner/mode0500, les neuf chaînes GNU FD/chemin et SHA256/octets exacts, avant ET après le seul test. Comparaison scalaire `eq`, pas égalité des types JSON dualvars. /tmp doit être vide avant/après. Les quatre lignes JSON C++ sont conservées sans réécriture ; le runner contrôle noms/terminaux positifs,64transactions,65536SSTORE,63états supprimés,3CLI, gains SST et allocations≥64KiB, durée≤120s/RSS≤131072KiB. Le binaire C++ déjà pincé reste responsable de ses oracles détaillés et du baseline SST≥1MiB.

Bornes : échéance globale210s, appel lecteur/test135s (borne C++120s inchangée), commandes15s ; cleanup séparé25s. Chaque commande conserve stdout/stderr et exitCode/elapsedMs AVANT de refuser son code ; une interruption conserve aussi son type quand disponible. Seuls les groupes clients créés sont arrêtés en cas de timeout ; puis les deux conteneurs propres sont inspectés par ID complet/nom/image/label avant suppression. Aucune recherche ni signal à un PID global ; aucun prune/volume-rm. Le nouveau volume est retenu, vide sur succès et conservé tel quel sur refus pour inspection Root. Le premier échec est préservé si cleanup refuse.

Succès : `result-public.json` engage SHA/octets du reçu privé `disk-probe-admission.private.json`, qui lie contrôle original, lignes des deux programmes, montage neuf, filesystem et bytes stdout. Stdout du runner ne renvoie que les pins publics. Le reçu donne une mesure dans un filesystem Linux non tmpfs ; il ne garantit pas de place rendue au backing hôte/VM, ni finalité, panne électrique, pruning EVM complet, nouvelle campagne ou production. Les64signatures éventuelles sont celles de la clé ECDSA unitaire publique777 du test, jamais des signatures opérationnelles.

Invocation Root, avec ses outils et fences déjà admis :

```sh
/ROOT/PYTHON runner.py --control /ROOT/CONTROL.json --sha256 ROOT_SHA256 --bytes ROOT_BYTES
```

L’auteur n’a lu aucun contrôle privé/SDK/binaire ni lancé Python runner, Perl, Docker, SSH, Git ou compilateur. Seul le parsing AST du source Python et les hashes des sources publiques sont effectués. Syntaxe Perl et qualification réelle restent Root.
