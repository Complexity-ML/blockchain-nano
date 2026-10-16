# Comparaison RPC avec Geth 1.16.0 — 17 octobre 2026

**Geth 03 a terminé les 258 comparaisons et ses clôtures passent. Le résultat de comparaison reste FAILED : 50 cas divergent.** Les 208 autres satisfont les critères de l’oracle ; ce résultat ne signifie ni égalité de toutes les valeurs ni parité générale Nano/Geth. Le [manifeste public](evidence/manifest.json) et le [triage](evidence/triage.json) conservent les réponses et les tentatives précédentes.

## Authentification et observations

Root a acquis l’archive ARM64 officielle v1.16.0, commit `57f8971b89b3ce3d31ec09d6a1e29151c90e7d19`, et vérifié sa signature détachée contre l’empreinte Linux Builder `FDE5A1A044FA13D2F7ADA019A61A13569BA28146`. Le [reçu d’acquisition](evidence/authentication/admission-public.json) et le [complément GPG](evidence/authentication/authentication-supplement.json) distinguent cette authentification de l’exécution : archive 17 471 238 octets, SHA256 `1610266a6136c2b32fcbba1c2db6953335c1318e39bd4b0e735a63956407c7e7` ; exécutable 46 058 336 octets, SHA256 `fe91f2e0fc8b0ad01b4d8e847734d1fcdb4efd5368b1635e98fb776418e5e445`. Ces SHA sont calculés après acquisition authentifiée, sans prétendre être des sommes antérieurement publiées.

Deux genesis locaux isolés, Paris et Shanghai, servent chacun 129 enregistrements publics : chainId 1337, hauteur 0, BASEFEE 1 Gwei, plafond 30M, six allocations vérifiées. Le profil post-Merge adapte explicitement la difficulté à zéro. En-têtes et allocations restent inchangés. Les réponses candidates Nano déjà qualifiées sont réutilisées : aucun test C++ n’est rejoué.

| Comparaison observée | Cas |
|---|---:|
| Conformes aux critères qualitatifs | 208 |
| Extensions/types explicitement refusés Nano : 36 acceptés Geth, 12 rejetés sous d’autres codes/classes | 48 |
| createAccessList : une adresse redondante et 2 400 gaz supplémentaires | 2 |
| Devis estimateGas différents, supplémentaires et non bloquants | 18 |

Les 50 cas divergents restent FAILED : 48 différences de classe, puis deux listes d’accès dont gasUsed diffère aussi. Aucun returnData comparable de eth_call ne diffère. Les 18 devis supplémentaires restent différents et non bloquants selon les critères de bornes/rejeu de l’oracle ; **toutes les valeurs ne sont donc pas égales**. Une acceptation Geth d’un paramètre refusé Nano ne prouve pas l’activation du type de transaction correspondant dans Nano.

## Diagnostic des listes d’accès et clôtures

**Six diagnostics distincts**, trois par profil, réutilisent les mêmes instances et ne sont pas comptés dans les 258 comparaisons. Le nonce fourni 9 conserve l’adresse supplémentaire et +2 400 gaz ; nonce 3 ou nonce omis rendent une liste vide et le gasUsed exact Nano, `0xe905` sous Paris et `0xe907` sous Shanghai. La [source Geth v1.16.0](https://raw.githubusercontent.com/ethereum/go-ethereum/v1.16.0/internal/ethapi/api.go) construit l’exclusion CREATE depuis le nonce fourni, tandis que [l’exécution EVM](https://raw.githubusercontent.com/ethereum/go-ethereum/v1.16.0/core/vm/evm.go) crée depuis le nonce d’état. Les observations corroborent cette redondance sans justifier un changement C++ Nano.

Le [traitement Geth 03](evidence/attempt03/result-public.json), dont la [trace brute](evidence/attempt03/comparisons.jsonl) reste conservée, dure 14 765 ms ; l’invocation hôte termine exit1 en 18 602 ms, en raison du résultat de comparaison. completed=true, firstFailure=null et cleanupErrors=[] ; les 14 entrées hôte restent inchangées. Six nouveaux contrôles disposent chacun de 11 observations stables sur 2 048–2 085 ms, puis leur clôture stricte passe. Cette première admission prospective ne rafraîchit aucune autorité ancienne. La [clôture Root](evidence/closure-root.json) confirme le conteneur absent.

Geth 01 reste FAILED avant comparaison : la conversion genesis omettait des balance=0 explicites requises. Geth 02 reste FAILED après 258 observations : ctime d’un nouveau contrôle décalé de 159 ms. Les 116 divergences Anvil antérieures restent conservées. Aucun de ces essais n’est requalifié rétrospectivement. Les pilotes [V1](source/runner-v1.mjs), [V2](source/runner-v2.mjs) et [V3 exécuté](source/runner-v3.mjs) restent distincts.

## Portée

Aucun Core ni image n’est construit ; aucun ancien test C++ ou CI n’est rejoué. Aucune signature de transaction ou de consensus, nouvelle clé de consensus, campagne ou réseau public ; la signature locale ENR n’est pas auditée. Les preuves EEST/Doppler acquises gardent leur périmètre, sans reset ni obligation de rejouer. Aucun EIP n’est promu par ce lot. Restent distincts : non-parité RPC observée, assemblage Native et admissions runtime après le Core, cycle/reprise/duties/finalité, opérations/bridge/lancement, CI GitHub bloquée par facturation et revue indépendante. Goal actif ; production non qualifiée.
