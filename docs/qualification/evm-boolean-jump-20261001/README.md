# ISZERO/JUMP — nouveaux cas et mesure ciblée

Candidat 98868b93e73888129234d2233df80b254d94844b : ISZERO réutilise le stockage GMP du sommet de pile ; JUMP inconditionnel ne construit plus un entier constant pour sa condition. Opcode/gas, exceptions et checkpoints inchangés. Le commentaire uint64 est corrigé : cette conversion ne se trouve pas dans la boucle principale du scénario RPC, donc son microbenchmark ne justifie pas le débit RPC.

20 nouveaux contrôles : quatre largeurs de mots / exactitude booléenne / indépendance après DUP, sous-passement ISZERO, frontière gas JUMP/JUMPDEST, destination énorme ignorée pour JUMPI non pris, destinations invalides et sous-passements de saut. ARM64 Docker avec objets types/EVM/tests recompilés puis exécutables relinkés contre bibliothèques en lecture seule : core 4 025 checks, RPC 1 007, EEST local 5 387+505 correspondances, zéro divergence/non supporté/erreur, trois exit 0. Ce n’est pas encore une reconstruction release complète.

Contrat réel du test de charge, 600 000 itérations, trois appels distincts avant/après : même gas, returnData et stateRoot sur chaque paire. Médiane ARM64 : 102,472 ms avant / 84,430 ms après (rapport 1,214 ; durée réduite de 17,6 %). Cette mesure n’est pas un résultat de charge RPC, WAN ou finalité.

Reconstructions CMake et campagnes de 20 tests sur ARM64/AMD64 sont lancées sur cette révision dans les images existantes : cœur/RPC/HTTP/admission, crypto à froid/EEST, huit charges RPC, deux parcours volumineux locaux, deux sync libp2p et deux fautes de fragments. Résultats encore ouverts ; pas de promotion sur main du code expérimental. Gates originaux et tous les échecs historiques conservés.
