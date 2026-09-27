# Premier payload Shanghai après Paris

Le pont EL Capella accepte désormais un parent Paris ou Shanghai. Bellatrix
continue à refuser un parent Shanghai ; les champs Cancun/Prague restent
interdits. Le choix du fork appartient au contexte consensus authentifié,
jamais à une simple détection du contenu envoyé par le pair.

Pour un parent Paris, la transition couplée exige le withdrawals_root SSZ
nul du header migré. La racine SSZ de la liste reçue remplace ensuite cette
valeur ; le header EL engage séparément la racine MPT des retraits.

La qualification conserve le cas Shanghai existant et ajoute un état
Bellatrix synthétique construit depuis les champs communs d'une fixture de
retraits, puis réellement transformé par upgrade_capella_state. Le premier
payload traite quatre retraits et une transaction PUSH0/SSTORE. Le résultat
couplé est comparé aux états attendus ; les parents restent inchangés.

Sept refus et trois annulations par profil, plus un header non migré rejeté
pour Paris. Un contrôle distinct refuse Shanghai→Paris dans l'admission
Bellatrix. Régression : candidat Bellatrix, payload couplé Capella, blocs
signés Capella et migration officielle : 4/4.

Ce scénario synthétique ne prouve pas l'activation réseau ni un bloc signé
franchissant lui-même le calendrier. Restent à composer le calendrier,
l'admission signée et le Store/réacteur, puis commit durable, récupération
et qualification distribuée. Le profil réseau reste inchangé.

Références : https://eips.ethereum.org/EIPS/eip-4895 et
https://github.com/ethereum/consensus-specs/blob/v1.4.0/specs/capella/fork.md.
