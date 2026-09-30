# Préparation du diagnostic de reprise

La tentative A est terminale : second délai manqué (187,621 s contre 180 s).
Ses trois traces de duties sont vides ; elle ne conserve pas les head slots
pendant la partition. Les comptes terminaux des boucles ne donnent pas la
chronologie du retard : VPS maxPoll 5623 ms et maxProposal 4167 ms, pour des
slots de 6000 ms, sans pollOverSlot ni panne déclarée. Cela n’exclut pas une
attente réseau ou FFG et ne démontre pas une cause CPU/disque.

Le nouveau mode opt-in collecte des snapshots diagnostic head/slot/état et
checkpoints justifiés/finalisés, avec bornes temporelles de lecture. Les données
de finalité de sécurité et les budgets du scénario restent inchangés. Quatre
tests préparatoires passent ; aucune nouvelle campagne réseau encore qualifiée.
Le cœur C++, les deux images et les critères de fermeture ne sont pas modifiés.
