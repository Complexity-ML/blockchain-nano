# Admission des fichiers lifecycle neufs

Le pilote ne lit plus immédiatement un fichier qu'il vient de créer avec le
reader strict des entrées conservées. Il engage les octets et les huit champs
hors ctime sur son propre FD, puis exige trois observations FD/path complètes
identiques sur au moins deux secondes, dans le budget existant de trente
secondes. Un changement de ctime avant première admission invalide l'échantillon ;
une mutation des huit autres champs ou des octets refuse immédiatement. Après
admission, les neuf champs restent fixes. Les chemins échoués ou déjà admis ne
peuvent pas recevoir une nouvelle autorité de création.

Le changement touche les helpers JavaScript et Python propriétaire, pas le
consensus, l'EVM ou RocksDB. Le reçu de sortie Python neuf rejoint lui aussi son
admission avant retour. Les groupes existants restent groupés ; ce reçu ajoute
au moins deux secondes, sans augmenter les budgets du cycle. La tenue temporelle
de la vraie campagne reste à qualifier.

## Vérifications acquises sur Mac

Les sept groupes JavaScript de la proposition01 passent lors de la première
exécution. Le huitième refuse le stderr du lanceur Python Apple malgré sa sortie
zéro. Le test accepte maintenant un interpréteur explicitement configuré et
vérifié, sans accepter de stderr supplémentaire. Avec Python3.14.6 explicitement
choisi, ce seul groupe est exécuté : ses seize scénarios FS/temps mockés passent.
Les sept corps JavaScript, leur fixture et le harness Python restent byte-exacts ;
les deux helpers testés sont également identiques entre les propositions01/02.
Il n'existe pas de nouvelle exécution globale « huit groupes PASS ».

La nouvelle génération nommée du driver guard passe ses six groupes, avec les
trois refus d'autorisation avant lecture des seed/artifacts et avant réseau. Les
seize cas applicables du protocole runtime passent ; deux cas Linux sont sautés
explicitement sur Mac. Leurs 55 refus attendus restent vérifiés. Ces tests
utilisent Node25.9.0, les sources exactes dans une projection privée et un TMPDIR
propre. Toutes les fermetures de groupes sont vérifiées ; les 119 lignes de
sources/logiciels/projection retenues restent inchangées.

Les empreintes historiques du guard sont conservées. `generation20261018`
sélectionne explicitement les deux nouveaux SHA des helpers et garde toutes les
autres empreintes. Aucun ancien reçu, domaine, plan ou admission signé n'est
retargeté. Un nouveau CTest est enregistré dans le bloc existant réseau/Node,
avec un interpréteur optionnel `NANO_LIFECYCLE_TEST_PYTHON`. Il reste absent si
`NANO_NETWORK_EXECUTABLE` ou Node n'est pas configuré. La configuration CMake
globale et ce CTest via CTest n'ont pas été exécutés dans ce lot ; les commandes
Node sont exécutées directement. La fermeture complète des runtimes n'est pas
qualifiée ; la reproductibilité non plus.

## Refus et limites conservés

Deux préparations Root sont refusées avant tout test : lecteur logiciel trop
restrictif pour les 78 liens du launcher système Python, puis ctime d'un clone
source neuf. Aucun clone refusé n'est réadmis. La première exécution de tests
reste 7 PASS/1 FAIL à cause du stderr Apple.

Le premier agrégat Root de la phase Python configurée reste FAILED : il attendait
sept entrées SKIP, alors que le sélecteur Node25 ne rapporte que le cas choisi.
La sortie réelle est zéro, un test passé, zéro échec et zéro skip. Cette phase
est vérifiée séparément sans être rejouée. Les deux phases suivantes passent
sous les mêmes pins, sans remise à zéro d'autorités.

Le défaut source corrigé n'identifie pas la cause de la campagne03 ou des
autres variations de métadonnées. La campagne03 reste FAILED et son accord
consommé ; aucune04 autorisée. Aucun owner entrypoint, exporteur, clé privée,
signature, DB, nœud de blockchain ou réseau n'est démarré par cette qualification.
La compatibilité Linux du lot et ses deux cas dédiés restent ouverts à ce stade.
La supervision Source03 reste non intégrée et son SIGABRT Node inexpliqué.

Le C++ de production, Core, images et runtime réseau restent inchangés ; aucun
EIP/fork, cycle complet, reprise de duties, finalité commune ou production n'est
promu. Tous les critères de production intégraux restent applicables, notamment
la revue indépendante non réalisée. Aucun fonds, vente, nouveau domaine signé ou
déploiement public. Économie Base–Nano/bridge USDC/Doppler/T0 non qualifiés.
