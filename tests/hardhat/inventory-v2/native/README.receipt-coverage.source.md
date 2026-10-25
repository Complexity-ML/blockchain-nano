# Couverture des reçus Native / Geth retenu — SOURCE ONLY

Ce lot est distinct des campagnes Native24/65/22 et Geth65 déjà acquises.
L'auteur n'exécute aucun programme, test, compilateur, Git, Docker ou SSH et ne
lit aucune capsule, clé ou donnée privée. Seuls les petits reçus publics
autorisés par Root et les sources publiques ont servi à préparer le raccord.

## Entrée

```text
node receipt-coverage.mjs NEW_OUTPUT NODE_MODULES CASE_ROOT \
 SHIM SHIM_SHA NATIVE_ACCOUNTING NATIVE_SHA \
 T8N_OUTPUT_ROOT T8N_TRACE TRACE_SHA ROOT_ADOPTION ADOPTION_SHA ROOT_ACTUAL_RESULT
```

Tous les chemins sont absolus. Root fournit les SHA externes, un nouveau dossier
de sortie et le helper nouvellement compilé. NODE_MODULES fournit seulement
ethers6.16.0 déjà présent. CASE_ROOT fournit les helpers owned-command/deadline
existants, lus puis tenus et importés depuis leurs bytes.

T8N_OUTPUT_ROOT contient les anciens step-01/result.json à step-65/result.json.
Chaque fichier doit correspondre au SHA et au nombre d'octets de la trace
externe. ROOT_ADOPTION est l'adoption publique de Root (format
nano-retained-t8n-receipt-reference-adoption-v1), épinglée séparément ; elle lie
NativeAccounting, la trace et le reçu effectif ROOT_ACTUAL_RESULT.
Le driver vérifie aussi les bytes de ce dernier, son passed:true, son vrai exit
Docker0, ses FDs fermés et son identité Geth1.16.7/sourceCandidate. Le rapport
provisoire de l'ancien driver ne remplace pas cette adoption externe.

Cohorte publique actuelle fournie par Root :

- adoption : 096270e416236509c5c816bdb106e0409ba1ce819d504a2583850b4525df9e84, 821B ;
- trace : a3fb25aa29c559c8640996731bd6ea2a7e5a7ad8eeae871d5a4288f266b5d2cf, 76891B ;
- Native accounting : 6f92730dd60640c8f02a6808bd30118cf2345f2e542229d706215687e4e250fc ;
- Root actual result : ba23565a59ddcf443b380a9c2262a13842d1dfa425ae9ba72d63230c94692140, 767B.

Ces pins ne sont pas une décision économique ou une autorisation de lancement.
La provenance du binaire Geth et de son exécution précédente reste fournie par
Root ; ce driver ne relance pas Geth et ne vérifie pas une signature de release.

## Delta C++ et périmètre du rejeu

La copie source de tests/inventory_v2_execution_runner.cpp dérive exactement
du parent10997f1ad206ba8644b21643bef9ac34a16b4a8cf4c5bdcac6c1b97fdc06ba40 :
six lignes ajoutent cinq champs d'observation après apply_transaction :
receiptType, cumulativeGasUsed, logsBloom, encodedReceipt, singletonReceiptsRoot.
Les autres bytes du testshim sont conservés. Engine, Core et headers de la base
CoreG2 d23409b3 restent inchangés. La bibliothèque Core existante demeure
a1475733637348e9a22e72d6f8be3d61ebeafb39443cb168110ca353781ae2e6, 41077050B.
La base documentaire annoncée par Root à la livraison est maincab8498f.

Root peut compiler uniquement ce helper avec sa recette/link closure retenue.
Aucune nouvelle image, compilation Solidity ou exécution des24cas/modèle n'est
demandée. Le patch court et la copie pleine sont deux formes du même delta ;
appliquer une seule forme.

Le nouveau driver refait les65sends Native pour observer leurs nouveaux reçus :
ce rejeu est précisément le nouveau périmètre, jamais présenté comme absent.
Il décode les65transactions brutes conservées, exige type0, chainId31337,
nonce0..64, prix1, gasLimit30M et le même expéditeur public.
Le helper signe de nouveau avec la même clé de fixture publique et déterministe ;
chaque retour DOIT reproduire exactement les anciens bytes signés et leur hash.
Un écart refuse le lot. Aucune nouvelle clé de validateur n'est créée ou lue.

Il effectue132requêtes JSONL : metadata,65send,65lectures du compte coinbase,
puis root. Shanghai, gas de contexte60M, baseFee0, coinbase0x9999 et l'état
initial sont vérifiés. Les racines avant/après, gas, nonce, débits réels et
crédits coinbase doivent rester identiques à l'ancien rejeu, et les lectures
ne doivent pas modifier la racine.

## Nouvelle comparaison obligatoire

Pour chacune des65transitions, le driver compare tous les logs dans leur ordre :
adresse20octets, chaque topic32octets et tous les bytes de data. Seule la casse
hexadécimale des représentations est normalisée ; aucun log ou zéro n'est retiré.
Les métadonnées artificielles de bloc ajoutées par t8n aux logs ne font pas partie
de leur contenu EVM et ne sont pas reprises dans le helper.

À partir des logs Geth retenus, il recalcule indépendamment le bloom256,
l'encodage legacy RLP([status,cumulativeGasUsed,bloom,logs]) et le trie singleton.
Le statut0 est encodé comme entier vide, sans préfixe de type0. La clé du trie
est RLP(index0)=0x80, donc le chemin compact de feuille vaut0x2080 ; sa valeur
reste les bytes du reçu encodé. Le hash calculé doit égaler receiptsRoot de Geth.
Les cinq nouveaux champs C++ doivent tous égaler ces résultats. Les reçus
revert doivent ainsi conserver les mêmes logs vides et bloom nul.

Ce singleton est propre à notre contexte synthétique d'une transaction par étape.
Il n'est pas une racine de reçus d'un bloc canonique de Nano. Aucun blockHash,
blockNumber ou transactionIndex miné n'est exposé ni accepté depuis le helper.
mined:false et consensusValidated:false restent obligatoires.

## Fermeture et verdict

Le budget coopératif reste540000ms, dont60000ms de clôture. Une seule requête
peut être active ; timeout30s, lignes2MiB, stdout32MiB et stderr16KiB sont bornés.
Tout ok:false de protocole, réponse extra/malformée ou première divergence est
fatal. La trace partielle est conservée ; aucun deuxième essai ou choix d'autre
clé n'est effectué.

EOF doit produire exit0 et close0 sans signal, protocole vidé, stderr vide et
groupe possédé absent. Les inputs et les65résultats Geth restent tenus par FD
avec SHA/full9/path fences ; toutes les ouvertures se ferment même en échec.
Les sorties restent provisoires passed:false/terminalEvidenceReady et exigent
le vrai exit externe observé par Root. Root garde sa propre borne et les preuves
de provenance/link/namespace du helper.

Un succès qualifierait uniquement ces65reçus, dans ce contexte Shanghai et cette
cohorte signée. Cela ne qualifie pas des blocs minés, le RPC, le consensus, tous
les contrats/forks, le bridge réel, les réserves économiques ou la production.
