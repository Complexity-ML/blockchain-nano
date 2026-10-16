# Doppler : enchère Dynamic V4 et attribution LP V2 dans Anvil local

Le 16 octobre 2026, Root a exécuté et qualifié un cycle local sur Anvil v1.5.1, à partir de la révision `f851c3bd52b9ef59a690748a37a1cd4668c45b5e`. Le fork utilise l’état Base au bloc **52 173 454**, hash `0xba5cc2e9e91ce6f64babb91e8bbf5a38d71e9f2fd01bc17473519a5c3dc6be5d`, et la chaîne locale **31337**. Le contrôleur terminal sort avec le code 0 ; le conteneur est absent après nettoyage. Le premier refus de préflight O1 reste conservé et n’est pas requalifié par ce résultat O2.

Le pilote termine ses **19 étapes**, comprenant création locale, achat, vente et migration : une enchère **Dynamic V4** aboutit au migrateur historique **V2** puis à un pair V2. Une vérification supplémentaire, sur un vrai client RPC transmettant les requêtes à Anvil, retrouve exactement la preuve du pilote. Ses **six appels `eth_call`** utilisent tous le hash canonique du bloc de migration avec `requireCanonical:true` : `getAssetData`, `token0`, `token1`, `getReserves`, `totalSupply` et `balanceOf`. Les headers encadrant la vérification sont concordants. Le pilote appelle déjà le vérificateur avant et après le refus de seconde migration ; le contrôleur l’appelle une troisième fois. Ce contrôle réutilise le même algorithme.

La migration `0x44251f63df5d3553f2a192ef9c3096669e41d88430c05087cf605d9fc0acaa73` est minée localement au bloc **52 173 470**. Le reçu réel montre les événements `Transfer` de la paire `0xF55D221167E987A792A10e9fB231a42524e5d28E` (mint vers le migrateur historique V2 puis transfert au bénéficiaire), suivis de `Migrate` émis par Airlock, aux indices **17 < 20 < 24**. Le crédit LP attribuable net est positif : **7 646 187 948 056 960 unités brutes**. Trois transactions adverses sont réellement minées avec un statut d’échec : migration prématurée, tentative de prise de propriété et seconde migration. Leurs reçus et consommations de gas sont rapprochés des rapports ; les projections d’état et de soldes contrôlées restent égales avant/après chaque refus.

## Ce qui est synthétique

- Le nouvel ERC-20 local `TEST`, nommé `Nanobees SDK TEST`, à l’adresse `0x23D748314E74dca601acB35517FeC41AEe51629a`, est explicitement **non adossé** au Nano natif. Son URI est `https://example.invalid/test.json`.
- Le créateur `0x1111111111111111111111111111111111111111` et l’adversaire `0x2222222222222222222222222222222222222222` sont impersonnés et financés dans Anvil. Le rôle `masterMinter` du contrat USDC du fork est également impersonné pour créer **10 000 USDC locaux**. Ces soldes ne représentent aucun transfert sur Base publique.
- La gouvernance choisie est `noOp` : le bénéficiaire LP est `0x000000000000000000000000000000000000dEaD`. Ce scénario ne démontre pas une distribution vers un timelock administrable. Les horodatages et blocs locaux sont avancés par les méthodes de test Anvil.
- Les bytecodes historiques Base sont lus depuis le fork ; les transactions de création, échanges, refus et migration sont exécutées uniquement dans son état local. Aucun port public n’est publié, aucune transaction publique n’est soumise, aucune clé personnelle n’est chargée et aucune identité consensus n’est créée.

## Conservation des preuves et limites

Les six fichiers source Linux conservent leurs neuf champs et SHA après l’exécution ; les **quinze entrées hôte** sont inchangées, sans rafraîchir d’ancienne autorité. La durée du résultat enfant est **50 556 ms**, celle de la commande Root **185 002 ms** ; ce rapport n’attribue pas leur écart à une cause non vérifiée. Aucune image n’a été construite. Les versions enregistrées de `viem`, du SDK Doppler et du routeur correspondent au verrou, mais la reproductibilité complète de l’arbre de dépendances reste non qualifiée.

Ce lot apporte une intégration locale réelle de l’attribution LP historique V2, distincte des 29 unités antérieures. La canonicalité reste une assertion RPC, pas une preuve cryptographique de finalité Base. Il ne qualifie ni migration V3 ou tous les migrateurs, ni Nano natif, ni pont ou adossement, ni déploiement public, ni audit de sécurité global, ni production. La campagne lifecycle 02 FAILED, les anciennes erreurs et les gates CI/facturation restent inchangés ; aucun fork ou EIP n’est promu.

Les [résultats Root](evidence/qualification-public.json), la [commande terminale](evidence/command-public.json), la [fence hôte](evidence/host-input-fence-public.json), le [lancement](evidence/launch-public.json), le [résultat enfant](evidence/proof/result-public.json), la [preuve RPC](evidence/proof/independent-real-proof.public.json), les [événements](evidence/proof/events.public.jsonl) et le [cycle](evidence/proof/cycle/result.json) restent rattachés au [manifeste du lot](MANIFEST.json). Le reçu de qualification engage exactement les trois résultats suivants :

| Résultat | Octets | SHA-256 |
| --- | ---: | --- |
| Enfant | 3 417 | `5392916cfee7fc7dc0788f98b04914bd6260d88b023a889b8e894eab970ed528` |
| Cycle | 18 996 | `7a763202087372513bd6cdc7cd85d0bad1c53dd73410bc1388265e789a617524` |
| Vérification RPC concordante | 4 520 | `b6391fbebede0c30afbf6180f45acc4980c7991537266d49bf1bc9730221ae18` |
