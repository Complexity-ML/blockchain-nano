# Admission durable : mêmes octets vérifiés et conservés

Le cœur copiait certaines entrées empruntées après un callback. Celui-ci pouvait modifier un wire entre son décodage/authentification et sa persistance, ou changer une recette de bloc/signature pendant son traitement. Les quatre CPP conservent désormais une copie bornée avant le premier callback concerné. Les octets authentifiés sont ceux qui sont archivés. Les règles de consensus, frais, profils, formats durables, signatures publiques et ordre témoin→primaire restent inchangés.

Qualification nouvelle : un CTest final passe une fois sur chaque architecture Docker Linux, neuf familles et dix lignes par architecture, 57 signatures de test chacune. ARM64 : RSS25964KiB ; AMD64/VPS : RSS26280KiB. Contrôleurs :15.2068s et41.0660s, incluant leurs fences ; le consumer ARM finit en4685ms. Les ressources respectent180s/131072KiB/1024 signatures. Source/build readonly, réseau désactivé, volume temporaire Linux possédé ; volumes et conteneurs supprimés. Aucun build d’image et aucun ancien groupe de scénarios rejoué.

Les familles couvrent BLS/sorties et originaux invalides non réparables, votes et slashings, archive de bloc, recettes locales, sync/agrégation, fonctions de signature, sept annulations sans publication, réouverture non vide v3/v4/v5 et hot/fragmented/cold-reference, puis codecs SSZ. Les deux scénarios de bloc vérifient une branche authentifiée retenue. Ils ne prouvent ni sélection comme HEAD, ni finalité, ni retraite des queues. L’ancre synthétique à512 permet de respecter le délai réel de sortie sans inventer un historique finalisé.

La causalité est conservée : le consumer V1 échoue sur l’ancien cœur au premier scénario BLS attendu. La version finale V3 est un objet différent : V2 corrige les permissions de ses seules bases temporaires, V3 corrige l’hypothèse admission=HEAD. Le refus AMD V1 et les deux échecs V2 restent enregistrés. Les refus de métadonnées avant compilation ne sont pas requalifiés en succès historiques ; la cause environnementale reste inconnue.

Les champs/limites exacts sont dans CONTRACT.json, les résultats dans results.json, les échecs dans history.json et la compilation dans builds.json/source-bindings.json. Les logs ARM complets restent dans le dossier de qualification externe du Mac ; les logs, MMD, SDK pins et ELF AMD restent sur le VPS. Les revues internes par agents ne constituent pas une revue de sécurité indépendante externe.

Pour construire le nouveau test dans l’environnement de dépendances du projet :

```sh
cmake --build build --parallel 2 --target nano-capella-durable-owned-admission-tests
ctest --test-dir build -R '^consensus_capella_durable_owned_admission$' --output-on-failure
```

Le runner qualifié utilise un volume Linux temporaire privé ; cette commande directe n’étend pas la qualification aux fichiers RocksDB sur un bind Mac. Le target ne lance ni RPC, ni P2P, ni duty avec des clés opérationnelles.

Ouvert : restauration des anciens journaux/témoins privés et finalité commune originale Mac natif/Mac Docker/VPS, V37FAILED, gates consensus/résilience/ressources de PRODUCTION-GOAL, candidats de transactions empruntés, sync_current et références signer/journal/DB, CI et audit externe. Aucun fork/EIP n’est promu. La correction CSV1559/3198 documente l’oracle de pourboire déjà implémenté ; elle n’ajoute pas une nouvelle qualification RPC. Pas de production prête, de déploiement public, de fonds ou de vente.
