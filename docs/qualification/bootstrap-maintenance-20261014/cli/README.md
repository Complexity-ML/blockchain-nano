# Maintenance des copies arrêtées — proposition publique

Cet outil C++ proposé est un seul exécutable installé, `nano-bootstrap-archive-maintenance`. Il ajoute l’archive d’un bloc Genesis → slot 1 déjà signé à deux copies arrêtées PRIMARY/WITNESS. Il charge l’autorité publique par l’API générale de replay complet. Aucun code de consensus, owner, runtime, horloge ou stockage du candidat de base ne change.

**État : code proposé, non compilé et non exécuté.** Aucun résultat ni application à des copies privées n’est revendiqué. Les tests du wrapper, la faute de rétention/fsync et l’interruption réelle restent à qualifier. La fenêtre de rattrapage UTC, l’arrêt opérationnel, les journaux anti-double-signature, les données du nœud complet, les duties et la finalité commune sont des gates séparés.

## Usage

```
nano-bootstrap-archive-maintenance plan REQUEST REQUEST_SHA NEW_OUT_DIR
nano-bootstrap-archive-maintenance apply REQUEST REQUEST_SHA NEW_OUT_DIR PRIMARY_NEXT PRIMARY_SHA WITNESS_NEXT WITNESS_SHA
nano-bootstrap-archive-maintenance recover REQUEST REQUEST_SHA NEW_OUT_DIR PRIMARY_NEXT PRIMARY_SHA WITNESS_NEXT WITNESS_SHA
```

Les SHA attendues sont fournies séparément par l’opérateur. Chaque invocation exige une sortie neuve, sous un parent privé déjà existant, hors des quatre arbres protégés. Les chemins absolus doivent être canoniques sans aucun composant symlink ; sur Darwin, utiliser les chemins physiques canoniques, pas `/var` ou `/tmp` s’ils sont des alias. Les entrées privées appartiennent à l’UID appelant et sont des fichiers réguliers sans hardlink ni écriture groupe/autres. Une umask qui retire les droits requis entraîne un refus.

Les sorties sont un répertoire 0700 et des fichiers 0600 exclusifs, jamais remplacés : `operation-intent.json`, `primary-next.json`, `witness-next.json`, puis `complete.json` en dernier. Les deux NEXT et leur répertoire sont synchronisés avant l’ouverture qui peut reprendre un intent, et avant tout supplément. Ils sont re-retenus dans la nouvelle sortie lors d’apply/recover. Les deux SHA des fichiers NEXT issus du plan doivent être conservées indépendamment avant leur réutilisation ; le reçu complete ne les remplace pas. Un échec laisse les fichiers partiels en place sans reçu complete ni revendication de succès.

Le stdout est limité au statut, à l’opération et à des booléens/comptes. Il ne contient aucun token, état, digest de copie, chemin privé ou exception brute. Tous les reçus détaillés restent sur l’hôte opérateur ; ils ne doivent pas être publiés comme preuves publiques.

## Frontières vérifiées

- Deux sources immuables et deux copies indépendantes, privées, quatre inodes/arborescences disjoints. Le plan exige la copie physique byte-hashée identique à sa source arrêtée. Inventaires bornés avant/après ; aucun original n’est ouvert en écriture.
- Marker explicite `.nano-owned-bootstrap-archive-copy` dans le parent de chaque copie, contenu `owned-offline-bootstrap-archive-v1` avec éventuellement un newline. Le marker et les deux déclarations stopped ne suffisent pas : request/receipt/pins externes, identité/inventaire réel et LOCK exclusif du propriétaire existant sont également requis. L’exclusion des lecteurs et les processus arrêtés restent l’autorité opérationnelle du reçu ; l’outil ne fait pas un audit universel des processus de la machine.
- PRIMARY est ouvert par le constructeur Blockchain strictement Existing ; WITNESS par Existing et une liaison existante sur le même handle. Pas de création d’identité, Genesis ou DB manquante. Le noyau vérifie l’ancre retenue, le poststate, l’exécution couplée, l’archive et son index.
- Plan reconstruit en mémoire, jamais désérialisé depuis un booléen de confiance. Si hasWork, NEXT PRIMARY = séquence courante + 1 avec digest identique ; NEXT WITNESS = séquence courante + 1 avec son propre intent. NoWork conserve les deux tokens.
- Apply compare les deux predictions au plan réel. Recover exige les deux NEXT retenus et l’autorité typée ; une reprise générique, un intent absent/incorrect ou des pins contradictoires refuse. Une reprise n’ayant jamais publié WITNESS doit passer par une nouvelle planification/application correcte, pas par un fallback automatique.
- Contrôle logique SHA256 ordonné/framed de toutes les lignes sauf exactement les trois clés PRIMARY archive/index/token et deux clés WITNESS intent/token autorisées. Le payload raw PRIMARY, ses références cold/fragment, les données d’exécution, receipts et queues durables restent dans l’invariant. Réouverture froide avec préparation NoWork exigée après apply/recover. Le cas recover répété avec les mêmes NEXT est idempotent.

RocksDB peut faire tourner ses fichiers LOG lors d’une ouverture RW même sans supplément. Le plan enregistre donc l’inventaire physique des copies **après leur fermeture** ; apply le vérifie. Recover après interruption utilise les identités et invariants logiques, pas un ancien inventaire physique qui serait nécessairement périmé. Les sources restent soumises à leur inventaire physique initial exact. Les invariants reposent explicitement sur SHA256, pas sur une comparaison littérale de toutes les lignes conservées en RAM. Les identités des répertoires portent dev/inode/UID/GID/mode et chemin ; nlink de répertoire n’est pas un certificat de contenu et peut changer par la création de cette sortie privée. Les fichiers d’entrée sont liés à leurs métadonnées complètes et relus via leur FD détenu.

Le chargement général vérifie les artefacts d’autorité avant de retourner une preuve owned ; cette preuve n’emprunte pas les fichiers pendant la publication. L’outil relit ses request/Genesis/profile/stop/NEXT via leurs FD avant et après. L’outil ne modifie, compacte ou purge aucun autre namespace, n’ouvre aucun journal/clé/env, ne signe pas, ne rebascule pas l’horloge et ne démarre aucun runtime/réseau.

## Qualification minimale à venir

Le contrat indépendant référencé dans manifest.json décrit sept groupes de tests **non exécutés** : owner réellement existant et LOCK tenu ; provenance/alias/hardlink ; plan et deux NEXT durables ; rétention refusée avant CAS ; vrai apply avec readback ; vraie interruption fresh-exec et reprise typée ; idempotence/pins périmés. Le shim de test devra observer le vrai WAL après l’ouverture, jamais deviner un nom ou substituer un état partiel synthétique. Aucune signature neuve ni ancienne qualification n’est comptée comme succès de ce wrapper.

Ce schéma ne prouve pas une sauvegarde complète des journaux/peer/V37/NAS. Le profil demeure celui effectivement activé. Aucun état EIP, fork, duty, réseau, production ou transfert privé n’est promu.
