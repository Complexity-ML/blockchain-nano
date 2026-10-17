# Doppler actuel : nouveau cycle privé sur fork Anvil

Le 17 octobre 2026, Root termine une nouvelle exécution du driver courant : **exit 0, 183 879 ms, 19 étapes**, sous la révision `4158176c66060fb1a2b23f08b2a878b88484dfaa`. La [clôture auditée](evidence/root-audit-public.json), le [terminal](evidence/command-public.json) et la [qualification Root](evidence/qualification-public.json) sont conservés byte-exacts. Les 19 étapes comprennent aussi les devis et observations ; ce ne sont pas 19 transactions.

Anvil 1.5.1 démarre une instance privée neuve, chaîne locale 31337, depuis le checkpoint Base 52 173 454, hash `0xba5cc2e9e91ce6f64babb91e8bbf5a38d71e9f2fd01bc17473519a5c3dc6be5d`. Le header initial et l’ancre attendue concordent avant impersonation. Le cycle couvre une enchère **Dynamic V4**, puis le migrateur historique **V2** et une paire V2 ; aucune généralisation aux autres migrateurs ou à une migration V3. Le [résultat du cycle](evidence/proof/cycle/result.json) et le [rapport du worker](evidence/proof/result-public.json) donnent les reçus réels locaux.

## Ce qui est observé

Le pilote et l’enveloppe recalculent les différences brutes de soldes des deux swaps. Chaque débit d’entrée est exact et chaque sortie reçue atteint `floor(devis × 99 / 100)`, avec rapprochement du reçu de transaction réussi. Montants ci-dessous en unités brutes :

| Swap | Entrée débitée | Minimum de sortie | Sortie reçue |
|---|---:|---:|---:|
| Achat | 400000000 | 198306861692540012944734 | 200309961305595972671449 |
| Vente | 20030996130559597267144 | 39351948 | 39749443 |

Le minimum encodé dans SWAP/TAKE_ALL est lié au driver épinglé ; ce contrôle ne revendique pas de décodeur indépendant du calldata. Les [observations supplémentaires](evidence/proof/independent-real-proof.public.json) retrouvent la preuve du pilote par un client RPC transmettant réellement les requêtes à Anvil. Six `eth_call` utilisent le même `blockHash` et `requireCanonical:true`, encadrés par les contrôles de header : données Airlock, `token0`, `token1`, réserves, supply LP et solde LP bénéficiaire. Il s’agit d’assertions canoniques **Anvil**, pas de lectures Nano ni d’une preuve cryptographique de finalité Base.

Le reçu de migration attribue le mint LP de la paire au migrateur, puis le crédit au bénéficiaire : indices **17 < 20 < 24**, le dernier étant `Migrate` émis par **Airlock**. Le crédit LP net attribuable vaut `7646187948056960`. Trois transactions réelles locales sont refusées : migration prématurée, vol de propriété et double migration ; les snapshots de contrat et soldes restent identiques pour chacune. Les [cinq jalons publics](evidence/proof/events.public.jsonl) sont également conservés.

Le [fence hôte](evidence/host-input-fence-public.json) ferme 17 entrées inchangées : **12 anciennes lignes, quatre nouveaux payloads et un contrôle** ; cela ne signifie pas 17 admissions historiques récupérées. Le contrôle neuf a sa propre première admission. Les enfants sont joints et le conteneur possédé est absent. Le rapport enfant mesure séparément 46 105 ms ; aucune cause n’est déduite de l’écart avec la durée hôte. Zéro compilation C++, build d’image ou redémarrage de chaîne est rapporté.

## Historiques et limites

Le [premier refus de préparation de l’audit Root](evidence/root-audit-preparation-refusal.json), `KeyError: absentAfterCleanup`, reste FAILED. L’audit attendait ce seul champ tandis que le contrôleur admettait aussi `alreadyAbsent` pour son conteneur possédé `--rm`. Le cycle n’a pas été rejoué et ce refus ne devient pas un échec Anvil. Le rejet synthétique Base **InvalidGamma**, les refus et cycles antérieurs restent conservés. Les [37 tests offline antérieurs](../doppler-offline-invariants-20261017/README.md) ne sont pas rejoués ici.

Le token `TEST`, le créateur de fixture, le bénéficiaire `…dEaD`, la gouvernance noOp, le financement ETH et le minter USDC impersonnés sont synthétiques et locaux. Aucune clé personnelle ni identité consensus nouvelle, transaction publique, fonds réel ou déploiement public. Le token Base préémis reste sans garantie de conversion native : ce cycle ne résout ni réserve, ratio, remboursement, autorité de bridge, modèle de supply, termes économiques réels, T0 ni indexation. La reproductibilité globale des dépendances reste explicitement non qualifiée.

Les [deux contrôleurs publics](source/README.md) et leur [manifest source historique](source/manifest.json) sont archivés sans réécriture. Seuls `host.py`, `worker.mjs` et leur README sont repris parmi ses payloads : aucun contrôle, même modèle, ni SDK n’est copié ; ce sous-ensemble n’est pas présenté comme le paquet source complet.

Ce lot ne modifie pas les sources C++, les 52 fichiers de harnais, les binaires ou engagements de la campagne03. Celle-ci conserve son HEAD d’exécution `b1f…` ; la révision des outils et documents `4158…` ne la retargete pas et le présent contrôle ne la requalifie pas. Aucun nouveau duty, reprise ou finalité Nano n’est démontré. [Objectif de production](../../PRODUCTION-GOAL.md) toujours actif ; production non qualifiée. Tous les anciens corps et statuts EIP sont conservés. Les pins des seuls fichiers publiés sont dans [MANIFEST.json](MANIFEST.json).
