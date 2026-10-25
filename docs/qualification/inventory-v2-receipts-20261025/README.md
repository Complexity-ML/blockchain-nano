# Reçus Native / Geth retenu — 25 octobre 2026

Qualification bornée de la candidate
ae0fee421b9154986a918c2580390388f3686c6d : les 65 transactions signées retenues
ont passé les 18 comparaisons par transaction, dont tous les 373 logs dans leur
ordre, les blooms, les reçus legacy encodés et leurs racines singleton.
La trace contient 1170 indicateurs de comparaison vrais, sans première divergence.
Cette preuve complète celle des 65 racines d'état Shanghai ; elle ne qualifie
pas un fork entier, le RPC, le consensus ou la production.

| Étape | Résultat conservé |
| --- | --- |
| Build Root01, source 80dd3d0d | Un seul helper compilé en 11,534 s ; exit Docker 0, 836 descripteurs fermés. Core non recompilé, zéro image construite. |
| Reçus Root01, source 80dd3d0d | FAILED avant lancement du helper : Docker 1, 0 transaction, 0 requête, 7/7 descripteurs driver et 28/28 Root fermés. |
| Reçus Root02, source ae0fee42 | PASS externe, 6,286 s ; 65 transactions Native, 132 requêtes JSONL, exit/close du helper 0 sans signal, groupe possédé joint. 76/76 descripteurs driver et 29/29 Root fermés. |

Le refus initial venait de logs:null dans un reçu Geth sans logs. Le correctif
exige le champ logs et représente seulement ce null côté référence par [].
Les retours Native restent des tableaux stricts. Le premier refus et les
manifestes source initiaux restent conservés ; la réussite ne les remplace pas.

Le helper 70b407e55c16b575c7dbe9390f46f6e98d466e40602b50f4a5612009967f1d4d,
3318144 B, ajoute uniquement cinq observations du reçu au testshim C++ :
type, gas cumulé, bloom, bytes encodés et racine singleton. La bibliothèque
Core G2 a1475733637348e9a22e72d6f8be3d61ebeafb39443cb168110ca353781ae2e6
est inchangée. Ce helper a été réutilisé après le correctif JavaScript.

Ce nouveau lot refait seulement les 65 sends Native, avec les anciens bytes
signés strictement identiques. Il ne relance ni Geth, ni les 24 cas Solidity, ni
une compilation Solidity. Les résultats Geth 1.16.7 déjà produits sont repris
par leur SHA/bytes, liés à la trace et à l'adoption externe de Root.

Les 18 comparaisons portent sur les bytes signés, hash, étape, racines avant/après,
statut, gas/prix, nonce/débits, type/cumul du reçu, logs/bloom, encodage/root,
absence de métadonnées minées et crédits coinbase. Les logs comparent adresse,
topics et data ; les métadonnées artificielles de bloc de t8n sont exclues.
Le RLP legacy et le trie singleton sont calculés depuis les champs du reçu Geth retenu, dont ses logs,
puis comparés aux observations C++ et à receiptsRoot de Geth.

Le contexte reste Shanghai synthétique, chainId 31337, une transaction par étape.
Ces racines singleton ne sont pas celles de blocs canoniques produits par Nano.
mined:false, consensusValidated:false et productionQualified:false restent
explicites. Aucun réseau, validateur, bridge réel, réserve économique ou terme
de lancement n'est qualifié ici.

Le contrôleur interne reste volontairement passed:false et provisoire ; ses
75 descripteurs fermés précèdent la dernière publication. Le terminal réel confirme 76 descripteurs
fermés et le reçu externe Root adopte le succès après exit Docker 0. Une sortie
JSON provisoire seule ne constitue donc pas la preuve de réussite.

Preuves publiques conservées byte pour byte :

- [Build unique](retained-public/build-root01-result.json).
- [Premier refus](retained-public/runtime-root01-refusal-result.json) et [terminal à zéro transaction](retained-public/runtime-root01-refusal-stdout.log).
- [Adoption Root02](retained-public/runtime-root02-result.json) et [terminal réel](retained-public/runtime-root02-stdout.log).
- [Contrôleur provisoire](retained-public/receipt-coverage-controller.json), [132 requêtes](retained-public/receipt-coverage-protocol.json), [trace 65×18](retained-public/receipt-coverage-trace.json) et [pins des entrées](retained-public/input-bindings.json).

Le [complément autonome sur la racine initiale](../inventory-v2-initial-root-20261025/README.md)
emploie une transition sans transaction, sans rejouer ces 65 transactions.
Ce complément n'est pas adopté par les preuves du présent lot.
Les autres exigences EVM/RPC, validateurs, reprise, charge, CI,
revue de sécurité indépendante et relation token Base/Nano restent dans leur
état antérieur ; aucune clôture globale n'est déduite de ce résultat.
