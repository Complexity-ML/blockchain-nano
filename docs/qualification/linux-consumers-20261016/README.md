# Consommateurs Linux — réutilisation ARM et qualification ciblée AMD

Le lot conserve les logiciels qualifiés et ne reconstruit que le consommateur affecté. Sur la base publique `a3a894b4…`, ARM Submit est rapproché des sources courantes sans compilation, lien ou CLI rejoué. Sur AMD, un nouveau Tool avec le header uint64/GMP `6681926f…` est compilé/relié ; le Submit existant est réutilisé et son CLI est réellement exécuté. Aucun Core ni image n’est reconstruit.

Le [reçu ARM reuse](evidence/arm-submit-reuse.public.json) vérifie huit fichiers projet contre les blobs de la base a3 et l’égalité du MD conservé avec le M producteur et l’ensemble admis (441dépendances). Le header Tool corrigé n’est pas atteint par Submit. Les trois groupes/26refus du managed-selftest antérieur sont réutilisés : zéro nouveau scan, CXX, lien, CLI ou Docker. Les433noms de dépendances SDK n’ont pas été lus comme fichiers SDK ; aucune nouvelle qualification SDK/loader n’est délivrée. Le digest brut de l’ancienne admission et les neuf champs historiques du programme restent non récupérés. L’auditeur reuse était inline Root : aucune source autonome exacte de cette invocation n’a été conservée ; aucun script reconstruit n’est présenté comme celui exécuté.

Le [terminal AMD réel](evidence/amd-terminal.public.json) rapporte39phases réussies, deux M indépendants égaux aux MD conservés et le nouveau MD Tool égal à son M : une TU Tool compilée (27 726ms), un lien (506ms), zéro compilation/lien Submit. Ces durées sont celles du travail ciblé, pas un benchmark de nœud. Les trois CLI policy/observe/managed-submit passent neuf groupes/47refus attendus, créent sept signatures BLS de fixtures et zéro signature opérationnelle. Ils ne lisent ni environnement applicatif ni base et n’utilisent ni réseau ni vrai exec. Ils n’exécutent pas le chemin complet des paiements/retraits uint64.

| Identité retenue pour le rôle | SHA-256 | Octets |
| --- | --- | ---: |
| Tool AMD nouveau | `45090eaee0ed9738ef6d2deb2dda8c843e36de4dafd5ee218f1b814693b0e442` | 6 589 712 |
| Submit AMD réutilisé | `ba626c763b6e98c3b69416eb249c6c66fae2ea74c6863f4b8b6abb26d38fa2e3` | 482 584 |
| Submit ARM réutilisé | `69efabf4badf3b99b0325747e5a8e5231dcb691b2c042c55bdc2508e5b204d05` | 536 264 |

L’ancien Tool AMD `415cad1b…` reste un produit historique, remplacé pour ce rôle courant ; le nombre d’identités de rôles reste24, pas25. La base productrice AMD712b, le Core3816 et l’image fd9b sont conservés séparément de la base effective a3. L’[audit des locateurs publics](evidence/amd-public-build-locator-audit.public.json) et la [préparation](evidence/amd-setup.public.json) relient cette sélection ; ils ne prétendent pas récupérer l’ancien audit SDK privé.

La [postqualification V3](evidence/amd-postqualification.public.json) vérifie143**enregistrements utilisés** neuf champs/SHA inchangés, pas143objets Core, ainsi que les trois nouveaux produits. Les493lignes SDK ont été enregistrées/vérifiées dans le même conteneur pendant la qualification ; la postqualification après suppression ne relit aucun SDK. Les portées de dépendances Tool524/Submit442 sont distinctes de ce snapshot493 et ne s’additionnent pas. Le checkout source est propre, le seul delta source runtime/consommateurs est le header déclaré et le conteneur possédé est absent. Cette collecte ne recompile et ne rejoue aucun CLI ; elle ne consomme aucun domaine de campagne.

Les limites historiques restent explicites : `sourceAdmissionDigestPreviouslyRetained:false`, `admissionDescriptorDigestPreviouslyRetainedOffsite:false`, `oldRecipeFullNineRecovered:false`, `historicalSdkAuthorityRecovered:false`, sans rafraîchissement des anciennes autorités. La nouvelle admission privée des produits est engagée par le producteur ; elle ne transforme pas un engagement postérieur d’un ancien descripteur en preuve historique. Aucune admission opérationnelle des24rôles n’est émise.

Les refus sont conservés. Le [préflight local](evidence/amd-local-preflight-refusal.public.json) a arrêté avant SSH/Docker/CXX/CLI parce que les octets revus n’étaient pas encore gelés0400. Le [collecteur postqual V1](evidence/amd-postqualification-v1-refusal.public.json) confondait les octets du stdout JSON avec ceux de sa sérialisation canonique ; aucune altération n’est démontrée. Le [V2](evidence/amd-postqualification-v2-refusal.public.json) cherchait un ancien nom de champ au lieu de `usedMaterialsFullNineSha`. Tous deux refusent avant auditrows/Git/Docker, sans nouveau test. Le stdout qualifié3c22…/7253 et le terminal canonique f384…/6620 désignent le même objet avec deux sérialisations différentes ; leurs pins ne sont pas interchangeables. Les sources publiques réellement utilisées (opérateur AMD, préparation/dispatch, trois collecteurs) sont conservées sous `source/` ; entrées privées, SDK, programmes et logs bruts sont exclus.

Le lot complète les [preuves ARM Tool](../arm-tool-u64-20261016/README.md) et [Native effectives](../native-effective-20261016/README.md) sans les rejouer. L’échec global historique Native/septfichiers hors fermeture sélectionnée et l’alias historique inconnu restent ouverts. La carte reste9Native+8ARM+7AMD,24connus/0admis. L’importeur ARM3b635 reste lié au Core9a03 et sa mise en cohérence avec9cc n’est pas qualifiée ici. Provenance transitive P2P, recette CMake effective complète, tous symboles lazy, permissions de nouvelle campagne, vrai HTTP/exec, fullpayout, cycle des validateurs, reprise des duties et finalité commune restent ouverts. La CI et aucun EIP/fork ne sont promus. Source C++/header/CSV/RPC restent inchangés ; goal actif, production non qualifiée.


L’archivage V2 conserve exactement les octets du source AMD exécuté, y compris sa ligne vide finale. La publication V1 s’est arrêtée avant tout commit/push : le contrôle normal `git diff --cached --check` signalait cette ligne vide dans la copie d’archive `.py`, alors que le contrôle des documents suivis passait. Aucun test, CXX ou CLI n’a été rejoué et aucun réglage Git n’a été contourné. Le reçu Root `ac0132fe…/609` est lié et repris dans le manifeste de livraison.

Le seul source concerné est livré en `source/amd/build-amd-consumers-root.py.b64`, une ligne RFC4648 suivie d’un LF. Le décodage doit produire exactement33 273octets, SHA-256 `8a60662bec1a363b287db3c34338bf20de4079ff46dd7875752554f1be89cffa`, avant de vérifier le manifeste producteur `source/amd/manifest.json` f1699146… resté inchangé. Son nom original demeure `build-amd-consumers-root.py` : le manifeste producteur décrit les octets décodés, le manifeste de livraison décrit leur représentation encodée. Aucune normalisation du source n’est effectuée.

Exemple de validation, sans lancer l’opérateur :

```python
from pathlib import Path
from base64 import b64decode
from hashlib import sha256
encoded = Path("source/amd/build-amd-consumers-root.py.b64").read_bytes()
assert encoded.endswith(b"\n") and b"\n" not in encoded[:-1]
source = b64decode(encoded[:-1], validate=True)
assert len(source) == 33273
assert sha256(source).hexdigest() == "8a60662bec1a363b287db3c34338bf20de4079ff46dd7875752554f1be89cffa"
```

Les statuts, preuves, autres sources et quatre historiques/appendices1904octets de V1 sont identiques. Seuls la représentation de ce source, ce README et le manifeste de livraison changent ; aucune nouvelle qualification n’est créée.
