# Transition signée Capella

Les API de construction et d'admission Capella composent maintenant les
slots/epochs, parent beacon, proposant, signature BLS, RANDAO, retraits,
exécution Shanghai, vote Eth1, opérations communes, changements BLS, sync
aggregate et racine d'état annoncée. Les deux états restent privés jusqu'au
retour. L'activation de fork et la publication durable sont séparées.

Le RANDAO est vérifié avant exécution mais appliqué après, afin que le
payload utilise le mix antérieur. Les retraits débitent les balances avant
les autres opérations ; les changements de clés précèdent sync aggregate.
La configuration impose le même domaine genesis aux dépôts et changements.

Scénario construit à partir d'un état officiel modifié et de clés de test :
neuf blocs signés avec transactions PUSH0/SSTORE, franchissement d'époque,
changement de clé de retrait au premier bloc, paiement au deuxième bloc.
Le curseur initial est explicitement positionné à 48 pour qu'un balayage
minimal de 16 validateurs atteigne l'index 0 au deuxième bloc. Un changement
ne garantit donc pas systématiquement un paiement dès le bloc suivant.

20 refus : signatures altérées, mauvaises racines postérieures resignées,
signature de changement BLS invalide et rejeu après mutation des credentials.
18 annulations avant/après traitement ; parents inchangés. Construction et
réception concordent sur l'état consensus, l'état EVM et les effets Shanghai.
Régression Altair officiel, chaîne Bellatrix construite et chaîne Capella
construite : 3/3.

Limites : ce n'est pas le corpus officiel complet des blocs Capella. Les
opérations communes sont réutilisées ; ce scénario n'inclut pas toutes leurs
combinaisons dans un bloc Capella. Restent les campagnes correspondantes,
la migration Bellatrix→Capella, le Store/fork-choice/réacteur, le réseau et
la persistance atomique. Aucun profil réseau existant n'est activé ou modifié.
