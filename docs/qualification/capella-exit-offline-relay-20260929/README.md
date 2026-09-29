# Livraison autonome après admission hors connexion

`consensus_capella_exit_network` passe sur Mac en 427,43 s (CTest 427,44 s).
Le non-validateur reçoit seul la sortie via RPC privé, sans pair connecté.
Après arrêt propre, son propriétaire est rouvert avec témoin : les 112 octets
signés sont conservés exactement dans la file. Il redémarre avec le vrai daemon,
sans la variable de soumission. La sortie est livrée au producteur par le catalogue
`voluntary_exit` de cpp-libp2p, puis incluse automatiquement.

Le retrait est finalisé au bloc 56, hash
`0xd8aa29f1199808782c23a899eb62462fba9e4a189fbaad76a0a5221b0244b756`.
Les deux bases rouvertes retrouvent un seul paiement du capital, la somme exacte
des versements et le bon destinataire ; le journal du signataire ne contient
aucune proposition/attestation après son inéligibilité.

Deux processus locaux, un seul producteur détenant les 64 clés synthétiques.
Ce cas ne simule ni un accusé de réception perdu en vol, ni une partition WAN.
La preuve WAN/SIGKILL est séparée. Le patch et les empreintes correspondent
exactement au banc exécuté, avant l'ajout ultérieur du contrôle de réinclusion.
