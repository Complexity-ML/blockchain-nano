# Doppler cohérent et garde de ressources avant campagne

Trois défauts de cohérence des outils de lancement sont corrigés : les termes sont copiés avant les attentes SDK, le reçu de migration est détaché du cache fournisseur, et les lectures code/appels utilisent un même hash de bloc canonique. Le transport RPC partagé impose des identifiants de réponse exacts, une seule branche result/error, des budgets de délai/taille, et refuse les redirections. Les deux interfaces gardent des listes de méthodes fermées, sans signature ni envoi de transaction. Les dépendances restent verrouillées ; aucun code C++, Core ou image n'est reconstruit.

## Résultats réels Root

**43 cas distincts passent** : 33 Doppler/transport concernés, 8 nouveaux cas de politique de ressources et 2 nouveaux cas de sélection CI. Dix-huit de ces cas sont nouveaux. Le premier passage Doppler reste conservé : 32/33, avec un échec de sérialisation BigInt dans une assertion du nouveau test. Seule cette assertion a changé ; son scénario a ensuite passé seul. Les 32 cas réussis et les sources produit n'ont pas été rejoués/modifiés pour ce correctif de test. Les tests de transport utilisent des mocks en mémoire ou le loopback ; aucune transaction, clé ou somme réelle n'est impliquée. Voir les reçus [Doppler](doppler/result-public.json) et [ressources](resources/result-public.json).

Le workflow Doppler dédié utilise Node 25.9 et des actions épinglées, npm ci --ignore-scripts et les trois fichiers de tests, dont le fichier historique test.mjs. Son YAML est parsé localement. L'exception de sélection C++/réseau est limitée à son chemin exact : les changements mixtes restent déclencheurs. Le changement du classificateur lui-même reste un déclencheur réseau selon la politique existante. Aucun succès GitHub CI n'est déduit des tests locaux ; les échecs historiques de facturation restent ouverts.

Le garde externe est livré sous scripts/operations/campaign-resource-guard.mjs, hors des sept arbres de sources C++ qualifiés. Il exige un mode explicite --probe-only ou --launch-reviewed-campaign, des programmes/configurations épinglés et trois mesures initiales. Les permissions du launcher existant restent obligatoires : ce garde ne crée aucune autorisation. En exécution, une mesure manquante/insuffisante/invalide ou le délai de sept heures demande un unique SIGINT au ChildProcess qu'il a créé, avec attente de nettoyage limitée ; les tests de cette logique utilisent des enfants injectés. Aucun ancien PID de campagne n'est signalé par ce lot.

Le vrai **probe-only passe trois mesures** du Mac, de sa VM Docker et du VPS. Le seul conteneur de mesure est supprimé, confirmé indépendamment ; le volume existant reste conservé, monté RO et sans lecture de ses fichiers. Le VPS ne reçoit qu'une commande fixe df/MemAvailable par SSH, aucune configuration privée de campagne. Aucun nœud, exporter, P2P, DB ou campagne ne démarre. Les données brutes sont dans [resources.public.jsonl](resources/resources.public.jsonl).

| Minimum des trois mesures | Disque disponible (octets) | Mémoire de politique (octets) |
| --- | ---: | ---: |
| Mac | 317926850560 | 7736000512 |
| VM Docker Mac | 911776034816 | 7694680064 |
| VPS | 6908792832 | 7197163520 |

Sur Mac, la mémoire est une estimation free+inactive+speculative, pas une garantie MemAvailable. Le disque virtuel Docker est aussi borné par le disque physique du Mac. Le VPS dispose d'environ 6,4 Gio libres. Les seuils de test sont 4 Gio/3 Gio disque/mémoire au démarrage, 2 Gio/1 Gio en cours ; cela ne prouve pas la tenue pendant sept heures ou sous charge. La notice source [du garde](resources/guard-source-README.md) conserve son état avant exécution Root ; les reçus séparés portent les résultats effectivement acquis.

## Limites et suite

Le hash pour eth_estimateGas est une extension fournisseur exigée sans fallback, **pas** une méthode standardisée par [EIP-1898](https://eips.ethereum.org/EIPS/eip-1898) ; voir la [spécification estimateGas](https://ethereum.github.io/execution-apis/api/methods/eth_estimateGas/) et l'[extension Geth](https://github.com/ethereum/go-ethereum/blob/master/ethclient/ethclient.go). Son support réel sur un fournisseur Base reste non qualifié. Les simulations historiques par hauteur ne qualifient pas le nouveau chemin strict. Les assertions RPC ne sont pas des preuves cryptographiques de finalité.

La description nativeAssetRelationship ne crée pas de couverture ou conversion Base–NANO. WrappedNano wrappe l'actif natif de sa chaîne seulement ; aucun bridge lock/mint/burn/release interchaînes n'est livré par ce lot. Modèle de confiance, réserve canonique, conservation de l'offre, retraits et reprise idempotente restent à définir et implémenter ; ni cotation T0 ni indexation tierce ne sont garanties.

La base Git avant ce lot est aa09f7f683e9e14d18a7af7c38ba0d5c91ee7ce0 ; les sources C++ sélectionnées restent bd11bd9f9dedf3f003b707d1c38fb1ecbabae91d. Le manifeste d30e et les trois admissions de code ne changent pas. Le test RPC adapté à RocksDB reste qualifié uniquement sur Native ; sa variante Linux courante attend un prochain lot ciblé. La campagne privée fraîche, ses transferts/clés/signatures contrôlées attendent toujours l'autorisation dédiée : aucune permission n'est déduite de l'ancien audit de 3,8 Ko autorisé.

La reprise effective, l'avancement de finalité commune, le cycle complet des validateurs et sanctions, partitions/WAN/charge, CI/release et revue indépendante restent ouverts. Journaux Mac historiques absents/NAS refusé, V37/campagne08 FAILED et tous les autres échecs sont conservés. Aucun fork/EIP/profil, SDK/DAG/loader global ou statut production n'est promu. Les quatre documents de suivi reçoivent une préface ; leurs corps historiques restent byte-exacts. CSV EIP, RPC-BEHAVIOR et header de gel inchangés. Le goal reste actif.
