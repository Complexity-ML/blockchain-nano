# VPS duty floor : qualification ciblée du 25 octobre 2026

Candidate : `e4589f7f9b328f09d3a98b6a496d3b398235d5be`.
Test source : `8cc685ad419307966be6f25dc422afd93ef4b16a1873c02c64117e7f3206ece3`.
Workflow source : `8c7f3aadd18699c666f4392c485fe599e15af8483bba25100335233518e5b62c`.
Ces deux sources sont restées inchangées pendant la qualification rapportée.

| Observation | Verdict retenu |
| --- | --- |
| Linux Docker, Node18.19.1/Python3.12.3, UID1000 | **7 PASS, 0 FAIL, 0 SKIP**, exit Docker0 ; 87 FDs ouverts/fermés. |
| VPS Root01 | **FAILED préflight** : ancienne base `e2404c56` déclarée, base attendue incorrecte ; aucun lancement Docker, aucun test. |
| Mac UID/EUID501, suite ordinaire | **PARTIAL_UNEXPLAINED** : 5 PASS, 1 FAIL (timeout Python5000ms), 1 SKIP visible. |
| Mac, reprise ciblée du lecteur | 1 PASS TAP ; contrôleur **FAILED** : comptage de filtrage Node18 appliqué à Node25. Ce résultat ne remplace pas la suite ordinaire. |
| Mac, troisième tentative ciblée | **FAILED**, 1 test/1 timeout ; groupe de processus observé absent. Cause du timeout non établie. |
| Mac, mode obligatoire UID1000 | Refus attendu **confirmé** : exit1, contexte UID501 incorrect, groupe de processus absent. |
| Job GitHub `vps-duty-worker` | **Non qualifié par cette campagne** ; exécution réelle requise. |

La qualification Linux Root02 utilise l'image existante
`sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14`,
`--init`, réseau `none`, runtimes et sources montés en lecture seule ; zéro build
d'image. Elle ne qualifie pas la fermeture complète de Node/Python ni le système
de fichiers natif Mac. Le reçu Root02 conserve comme base source inchangée
`ced2e0e96b540d0015f22cade2366955be3cecb8`.

Les sept groupes couvrent l'arithmétique du floor, les projections du vrai lecteur
Python, les refus de substitution/rejeu/alias, la validation publique, le caller
et le dispatcher entier. Le dernier exige réellement Linux UID1000/EUID1000.
Aucun faux UID ni garde de production affaiblie n'est utilisé.

Commande ciblée, dans un contexte Linux UID1000 préparé :

```sh
NANO_REQUIRE_VPS_DUTY_WORKER_TEST=1 node --test tests/multihost/vps-duty-floor.test.mjs
```

Hors de ce contexte, la commande ordinaire conserve un skip visible pour le seul
septième groupe. Le mode obligatoire refuse le mauvais contexte avant tests.
Les timeouts Mac sont conservés ; aucune nouvelle exécution n'est prescrite pour
transformer leur historique en réussite.

Reçus publics copiés sans modification :

- [Root02 Linux](retained-public/linux-root02-result.json), [TAP](retained-public/linux-root02-stdout.log), [stderr vide](retained-public/linux-root02-stderr.log).
- [Root01 préflight refusé](retained-public/linux-root01-preflight-refused.json).
- [Portée Mac](retained-public/mac-scope.json), [tentative01](retained-public/mac-attempt01-failure.json), [tentative02/post-validateur](retained-public/mac-attempt02-post-validator-failure.json), [tentative03](retained-public/mac-attempt03-targeted-terminal.json).

Aucun runtime Nano, signature, clé, fonds ou terme économique n'a été engagé par
ces tests. Reprise des duties, finalité, conformité EVM/RPC, revue indépendante,
CI GitHub et production demeurent des qualifications séparées et ouvertes.
