# Déclaration POSIX de mkdtemp dans le test RPC

Proposition source seule : ajouter `#include <unistd.h>` à `tests/test_rpc_simulation_context.cpp`. Le fichier complet reprend exactement la TU V2 `1790fd79cca93cc807ef8539e1138d06eeacd53a8760cbc5be95ac30a0377b6c` (18 497 octets), avec cette unique ligne supplémentaire. Aucun scénario, bytecode, assertion, sélecteur ou compteur n’est modifié.

Root a observé un refus de compilation Native du nouveau test : `use of undeclared identifier mkdtemp`. Ce correctif rend explicite la déclaration POSIX attendue par ce compilateur. Aucun SDK ni résultat privé n’a été lu par l’auteur ; aucun compilateur, test, contrôleur ou programme du projet n’a été exécuté. R et la tentative Native 01 restent intacts.

La continuation Root peut réutiliser son nouveau Core qualifié et son objet RPC retenu, puis compiler seulement cette TU avant les sept réemplois et liens prévus. Les attentes du nouveau mode restent quatre groupes, 476 checks, 126 refus et 258 caseRecords ; ce sont les attentes conservées, pas un résultat Native acquis. L’inclusion seule ne justifie aucun rejeu RPC Linux, Geth ou Anvil.
