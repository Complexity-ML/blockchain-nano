# Main HTTP et persistance C6 : Native et AMD64 qualifiés dans ce scénario

Ce paquet public clôt le scénario **PoW synthétique / Shanghai** du vrai binaire
point d’entrée C++ `main` et de la commande `serve`, en Native Mac et dans le Docker AMD64 du VPS. Les résultats
ne qualifient pas le consensus Beacon ni la production globale.

Main publié : `bc99b0ec2cf8bc6bcb21a26ec980ef2c4e8573a8`, intégrant la candidate
C++ C6 `c6cdf780c68eab8b06306d76257bda9028928ea3`. Les qualifications antérieures
restent attachées à leur révision. Le producteur réseau `1925eeeabc9f7d0aef41f2c3fe43c11c31f1f352`
et les anciens domaines signés restent distincts. La source R n'a pas été
modifiée pendant ce lot ; les images existantes ont été réutilisées.

## Résultats terminaux et preuve indépendante

| Environnement | Exécution réelle | Vérification indépendante | Portée |
| --- | --- | --- | --- |
| Native Mac | Root 61492, exit 0 | Node exit 0, cinq vecteurs et trois en-têtes | 57 appels HTTP, trois serveurs, deux CLI mine, deux réouvertures |
| Docker AMD64 VPS | Root 10741, exit 0 | Root 69605, SSH et Node exit 0, cinq vecteurs et trois en-têtes | Même scénario, mêmes octets de vecteurs |

Le scénario utilise une base RocksDB isolée : un seul processus Nano peut
l'ouvrir à la fois. Chaque serveur est arrêté avec TERM et rejoint avec exit 0
avant l'ouverture suivante ; aucun kill forcé. Les deux CLI `mine` utilisent
les timestamps 12 et 24. La transaction publique déjà signée est admise via HTTP,
puis fournie explicitement à la première CLI ; la mempool n'est pas présumée
durable. La seconde CLI produit un bloc vide. Les contrôles portent sur les
wallets déverrouillés vides, `dev_mode=false`, le refus du mine RPC, nonce,
balance, code, storage, transaction, reçu et états historiques par hash.

Les preuves HTTP sont lues aux hauteurs `[0,1,0,2,1]`. Le pilote Node indépendant
vérifie leurs chemins MPT et valeurs, recompose les hash des trois en-têtes et
vérifie le hash de la transaction existante. Il récupère le sender sans créer
de signature. Les deux fichiers de vecteurs ont les mêmes octets et le SHA256
`6b60c1c6ecd2a3e51c4a88ea6a5cd656a0dfaa7660785b9063c0ec5f7203f288`.
Le reçu a `status=1`, `gasUsed=0xd5ee`, le nonce émetteur passe de 0 à 1, le
slot 1 de la cible de 0 à `0x67` ; code, nonce et balance de la cible restent
inchangés. Aucun stateRoot post-EEST complet n'est exigé : les récompenses PoW
Nano diffèrent. Le [Genesis](source/fixture/genesis.json) conserve le prestate,
le gas, la base fee et le chainId de la fixture, avec les adaptations PoW décrites.

Les résultats [Native](evidence/native02/result-public.json) et
[Node Native](evidence/native02/independent-proof.stdout.log) restent distincts.
La fence Root Native porte sur 31 entrées, celle du worker sur six ; toutes
passent. Côté AMD, le [host](evidence/amd02/dispatch/stdout.log) et la
[lecture indépendante](evidence/amd02/independent-readback/result-public.json)
restent distincts. Le host clôt 644 entrées après nettoyage, dont quatre logiciels
originaux et 628 fichiers du provider Python montés en lecture seule. Le
collecteur lit 97 fichiers publics / 96 961 octets, sans nouvel appel RPC,
compilation C++ ou rejeu des anciens tests JavaScript ; sa fence de 108 entrées
passe et ses deux groupes de processus sont fermés.

Les flags indépendants `pending` des anciens reçus worker/host ne sont pas
réécrits : le reçu Node séparé clôt précisément ce contrôle. Les bases, les
métadonnées complètes d'admission et les dépendances SDK ne sont pas publiées.

## Échecs et préparations conservés

Native 01, Root 55054, reste **FAILED** après 16 RPC, avant mine/Node : son oracle
comparait des adresses minuscules à des adresses DATA20 checksum valides. Le
[diagnostic](history/native01-oracle-case-failure-root.json), les logs et
l'ancien résultat restent inchangés. Serveur TERM/exit 0 et fences passent ;
aucun défaut C++ n'est établi. Le [worker V2](source/worker-v2/source.patch)
valide largeur/hex puis compare sans tenir compte de la casse pour les adresses
`from`, `to` et `miner` seulement ; les autres assertions restent strictes.
Le contrôleur Root V2 change seulement le namespace de sortie 01 vers 02.

Les [préparations AMD](history/amd-preparation/README.md) conservent les refus
préflight 70652 (`KeyError docker` avant Docker), provider 39240 (fichier vide)
et le probe dont le consommateur termine exit 1 malgré un observateur exit 0.
Préflight 43355, provider 71158 (628 fichiers / 51 répertoires) et consommateur
94498 passent ensuite ; aucune préparation ne démarre Nano.

AMD 01, Root 94771, reste **FAILED** avant Nano/HTTP/conteneur : la borne 32 Mio
du lecteur refuse Docker de 47 253 395 octets. La
[fence et le diagnostic](history/amd01-host-material-limit-failed/README.md)
sont conservés. Host V3 relève uniquement la borne du chemin Docker à 64 Mio
et utilise TOOL/OUT 02 neufs. Root 88897 relit ensuite exactement les deux
reçus publics AMD01 / 1 834 octets, sans Node : `collectionPassed=true` conserve
`hostPassed=false`, `passed=false`. Un succès de copie ne requalifie pas l'échec.

Le [refus de préparation du paquet](history/public-package-preparation-refusal.json)
reste publié : SyntaxError d'un script ponctuel de copie avant toute instruction
exécutée. Il ne s'agit pas d'un échec du projet, aucun test ou source R touché.

## Livraison et limites

Sources Root, workers V1/V2, hosts V2/V3, collecteur, pilote, fixture complète et
licence sont archivés avec leurs reçus et pins. Le [catalogue](public-copy-catalog.json)
joint chaque copie exacte à sa source publique et le [manifeste](payload-manifest.json)
épingle les payloads sans se hacher lui-même. Le paquet SSH Base64 de staging
est omis ; son descripteur conserve le pin, les cinq fichiers publics sont
archivés séparément. Les stdout SSH Base64, observations de copie Root, bases,
autorités privées, contrôles complets, JSON producteurs, clés et `node_modules`
sont exclus.

Ces contrôles ne ferment pas récursivement toutes les dépendances natives,
ni tous les 265 anciens produits ; ce drapeau reste false dans le host. Les
racines MPT sont cohérentes avec les en-têtes exécutés, **pas authentifiées par
le consensus ni la finalité Beacon**. Construction O(N), budgets/annulation des
preuves et absence d'archive d'états orphelins conservent leurs limites publiées.
PoW/HTTP/reopen ne qualifient ni partitions multi-nœuds, ni cycle des validateurs,
ni devoirs repris, ni sécurité économique, bridge, fair launch ou production.
Les quatre documents normatifs gardent intégralement leurs anciens corps.

Aucune création de clé ou signature, duty, campagne 04, fonds, vente, déploiement
public, nouvelle machine ou dépense n'est autorisée par ce paquet. Les échecs
historiques, la tentative 03 consommée et la revue de sécurité indépendante non
réalisée restent ouverts. `productionQualified=false`.
