# Inventory V2 : explication hors ligne de l'écart historique de gaz

Le [contrôle 04 adopté par Root](root-04-result-public.json) retrouve **les 65 hashes exacts** de l'ancien scénario Anvil à partir des transactions Nano et des adresses de la fixture publique de référence. Pour ces 65 transactions, l'écart de gaz correspond exactement à celui du coût intrinsèque ; après soustraction de ce coût, les gaz d'exécution calculés sont égaux. Le total de **324 unités** correspond à **27 octets non nuls supplémentaires ×12**. Aucun EVM, client ou scénario de contrats n'a été rejoué pour cette analyse.

La reconstruction conserve nonce, chain ID 31337, valeur, gas limit et données du scénario. Elle projette les adresses CREATE et les mots ABI liés aux termes, routes et identifiants de transfert de la fixture ; chaque raw type 2 reconstruit doit ensuite retrouver le hash historique exact. Les octets zéro/non nuls du calldata coûtent 4/16 ; pour des longueurs identiques, les autres termes intrinsèques, dont le coût d'initcode EIP-3860, restent égaux. Il ne s'agit pas d'une correction du calcul de gaz de Nano.

La lecture de la [source primaire Anvil v1.5.1](anvil-fee-source-note.md) a identifié la complétion du plafond absent : base fee et suggestion minimale de 1 Gwei, avec tip de transaction nul dans cette fixture. Les deux essais « deux fois le prix effectif courant/précédent » ne décrivaient pas ce plafond. La formule retenue n'est admise ici que parce que les **65 hashes historiques** sont retrouvés, pas parce qu'elle donne un total de gaz plausible. Cette valeur Anvil de 1 Gwei ne devient aucun paramètre Nano.

| Tentative conservée | Verdict externe | Portée |
|---|---|---|
| [01](root-01-result-public.json) | FAILED, Node exit1 | Plafond supposé courant×2 ; [1 hash sur65](root-01-stdout.log), premier écart à la transaction2. |
| [02](root-02-result-public.json) | FAILED, Node exit1 | Plafond supposé précédent×2 ; [1 hash sur65](root-02-stdout.log), même garde stricte. |
| [03](root-03-result-public.json) | FAILED malgré Node exit0 | Le contrôle final Root des sources a refusé. Le succès du calcul ne remplace pas la fence ; aucune cause précise de ce refus n'est requalifiée ici. |
| [04](root-04-result-public.json) | PASS borné, Node exit0 | [65 hashes sur65](root-04-stdout.log), 6 FD suivis fermés, groupe possédé absent ; 3,719s. |

04 utilise le même programme d'analyse que 03, SHA `6e72a3a549ebea6020ca531d42acc4110e4c8bf4a8ee57b74c2e10f81045e69f`. Root a effectué l'admission prospective de trois copies **nouvelles**, déjà connues en bytes, avec trois échantillons stables sur au moins deux secondes ; aucun engagement ancien n'est réappris pour transformer 03 en réussite. Le [stdout Node](root-04-stdout.log) conserve `passed:false` et `terminalEvidenceReady:true` ; seul le reçu externe Root adopte le terminal réel, après les fences et la disparition du groupe. Le [stderr](root-04-stderr.log) est vide.

Ce résultat explique l'écart comptable entre deux jeux historiques d'enveloppes/calldata. Il ne remplace pas un rejeu indépendant de la même transaction, ni la qualification des opcodes, receipts, racines d'état ou du consensus. Le différentiel Anvil historique ayant échoué sur une racine d'état reste FAILED ; cette analyse n'en change pas le statut. Les qualifications Geth distinctes conservent leur périmètre. Aucun fork, EIP, bridge, terme économique, réseau ou statut de production n'est promu. La campagne 04 de consensus reste signée, terminale et FAILED ; la revue indépendante et les critères encore ouverts demeurent ouverts.

La [projection des 65 lignes](transactions-65-public.json) conserve les hashes, contrôles, comptes d’octets et coûts par transaction ; elle lie par SHA-256 le résultat complet retenu, sans recopier les bytecodes. C’est une projection des résultats déjà exécutés, sans nouveau passage de l’EVM.
