# Sélection RPC des headers — qualification C7 bornée

Paquet public gelé proposé à Root pour import ; aucune publication Git ni mise à jour du binaire réseau n’est effectuée par son assemblage.

La base source revue est main `801d793f0f05c3b46f97d34d86973f1487e36561`, intégrant C6 depuis `bc99…`. Le candidat C7 est l’overlay gelé de dix fichiers, MF `99815701…`, et non une révision publiée inventée. Le producteur réseau documenté reste `1925eeeabc9f7d0aef41f2c3fe43c11c31f1f352`. Un Git pull documentaire ou de sources ne prouve aucun remplacement de ce binaire.

## Changement et frontière

Les quatre lecteurs d’état et `eth_getProof` sélectionnent désormais des headers dans leur worker détaché. L’index `execution-header/v1/<height>` est dérivé, synchronisé aux mêmes quatre Put/Delete que block/state/hash. Il lie nombre, hash déclaré/recalculé et reverse index canonique sur un snapshot RocksDB. Un index présent invalide refuse ; il n’autorise aucun fallback ou réparation. Index absent seulement : projection SAX legacy bornée, lecture linéaire du corps et checkpoints tous les 4 096 octets, y compris un long token. Aucun backfill, startup scan, nouvel archive d’états orphelins ou changement des layouts des limites RPC.

Le contexte Beacon ne copie que tip/finalized Header, contrôle le tip durable hors verrous et réessaie au maximum quatre fois sous la même deadline. Il compare aussi la génération d’événements existante pour détecter le cas A→B→A dans la source. Les méthodes globales de vérification des corps/certificats restent inchangées. L’index ne certifie ni le corps, ni une racine finalisée indépendante. Les I/O restent non préemptibles ; MPT et fallback legacy restent O(N) dans leurs données respectives.

## ARM : mécanique passée, clôture globale FAILED

Le premier résultat global de la tentative ARM02 reste **FAILED** : 26 fichiers sources non consommés ont changé de ctime. Les SHA source restent exacts ; l’acteur du changement n’est pas connu. Ni une lecture du code, ni l’audit suivant ne permettent de reclasser ce whole invocation en PASS.

Les trois résultats mécaniques ont atteint leurs oracles définis avant exécution :

| CLI | Résultat mécanique ARM | Portée |
| --- | --- | --- |
| `--state-read-cost-self-test` | 47 contrôles / 23 refus attendus | Index sans body, legacy borné/cancel, progression RPC/commit, fixture Beacon réel |
| `--state-readers-self-test` | 805 contrôles / 179 refus, cinq groupes | Selectors, quatre writers, reorg/pruning, réouverture et HTTP local |
| `--get-proof-self-test` | Cinq groupes / 63 refus / 24 vecteurs | Preuves et gros corps avec petit header/état ; flux de 30 records conservé |

Le fixture Beacon crée douze signatures BLS publiques, sous plafond32 ; aucune clé/duty de campagne. Cinq enfants linéaires vérifient les contextes prêts, un changement réel, quatre changements bornés puis disponibilité stable, et une contradiction de stockage volontaire terminale. **`fullBeaconAbaQualified=false`** : aucun vrai fork aller/retour n’est produit.

L’[audit public distinct de réutilisation ARM](arm/scoped-reuse/result-public.json) constate :

- Core `65c91631ffe93515e500fd1eeacb4e6fd784fa04d431a193c40ec14733077938`, 41 306 456 octets, 143 membres ; 44 remplacés, 99 inchangés.
- 244 lignes de produits originaux encore exactes, tous les inputs hôte consommés exacts et 2 578 fichiers guest sélectionnés ; les 26 chemins changés n’intersectent pas ces matériaux.
- 2 094 sources inventoriées et tous les pins d’octets inchangés, mais l’identité metadata du snapshot global n’est pas conservée.
- Trois tests mécaniques passés ; audit distinct : zéro compilation, lien et test neufs. SDK/release globale et production non qualifiés.
- Flux actuel de preuves SHA `6140a2c49bd37114645f9652400dc0a5543ccaf5c18b66f00e5cbdd1a3a1ffd5`, 41 688 octets. Les 24 vecteurs correspondent au contenu canonique déjà vérifié indépendamment, SHA canonique `43ce557522d6a3a7ee4ccff548e98a941fab98221a505bb534fc78d64cc0c54d` ; aucun vérificateur indépendant n’est réexécuté par cet audit.

Cette réutilisation est conditionnée à ses entrées sélectionnées et produits encore tenus. Elle ne qualifie ni un autre Core, ni un autre snapshot SDK, ni une image runtime actuelle. Le manifeste source gelé demeure SOURCE_ONLY dans son rôle de parent, sans modification rétroactive.

## AMD02 — reçu terminal et collecte publique

**AMD02 : PASS terminal, dispatcher exit 0 et collecte publique qualifiée.** Le worker a effectué 143 scans M Core et trois scans de tests, 44 compilations Core et trois unités, une mise à jour d’archive, trois liens, trois vérifications du chargeur et trois tests. Les oracles restent 47/23, 805/179 et cinq groupes/63 refus/24 vecteurs. Les 143 membres comprennent 44 remplacés et 99 inchangés ; clôture après nettoyage et image AMD exacte conservées. Les 24 vecteurs égalent le contenu canonique précédemment vérifié, sans nouveau vérificateur indépendant.

Core AMD SHA `dc4af8d2c04e684844205d2d6b4c5d99d9ebb6074a6197148e16e489006cad8e`, 41076890 octets. 4 corps d’objets diffèrent dans les 44 remplacements, constat descriptif sans conclusion sémantique supplémentaire. Pic RSS du test coût : 58984 KiB, sous 131 072 KiB. La sortie dispatcher vaut 0, comme la collecte SSH ; le conteneur détenu est absent après nettoyage. L’image reste `sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14`. Le reçu brut [hôte AMD](amd/result-public.json), le [worker](amd/outputs/result-public.json), le [fence final](amd/host-final-fence-public.json) et le [reçu de collecte](amd/readback-result-public.json) gardent leurs rôles distincts. Aucun build d’image, ancien test rejoué, duty, réseau de campagne ou lancement public. Le refus préparatoire AMD01 concernant l’ancien provider KZG généré est conservé (zéro CXX, test et Docker). Le premier lecteur Root a ensuite refusé une comparaison de stat incluant atime, sans exporter d’octets ; les champs initiaux n’ont pas été publiés et la cause metadata précise n’est pas caractérisée. Son échec et la lecture ultérieure fondée sur les neuf champs prévus restent séparés, sans rafraîchissement d’autorité.

## Autres limites

Native C7 n’est pas qualifié par ce lot ; les réussites Native/HTTP C6 restent acquises dans leur périmètre antérieur. L’authenticité des racines par finalité de consensus indépendante, un vrai FullBeaconABA, WAN/partitions prolongées/asymétriques, finalité commune qui avance, reprise des duties et cycle complet des validateurs restent ouverts. Paris/Shanghai est le profil exercé ; aucun fork ultérieur activé, conformité EIP/RPC universelle, CI globale verte, sécurité indépendante, production, bridge réel ou conditions Doppler ne sont déduits.

La campagne03 demeure FAILED et consommée ; aucune04, ouverture de vente, fonds, dépense ou déploiement public n’est autorisé par ce lot. Le rejet préparatoire d’un appel privé dans le premier draft de test est conservé comme refus SOURCE avant admission/compilation/exécution, remplacé par le réacteur public ; ce n’est pas un échec Core exécuté. Tous les échecs historiques demeurent.

## Import et limites de la livraison

Le plan épinglé importe dix fichiers source et les preuves publiques, puis ajoute quatre suffixes aux corps normatifs conservés intégralement. Root doit encore contrôler le parent Git exact, le diff et les blobs avant commit/push/pull VPS. Aucun commit futur ni qualification Native C7 n’est présumé. L’assemblage et la collecte ne relancent ni C++ ni Node. Le manifeste source historique reste FROZEN_SOURCE_ONLY ; les reçus de qualification sont archivés séparément.
