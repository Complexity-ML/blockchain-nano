# Réinclusion après réorganisation

Trois tests passent avec le banc final sur Mac (16,45 s) : file/reorg,
transport signé et reprise WAL de l'admission. Le nouveau contrôle se place
après sélection de la branche qui ne contient plus la sortie et réouverture
du propriétaire. La préparation automatique sélectionne la sortie conservée,
construit un nouveau bloc et calcule son état. Le bloc signé est accepté par
`prepare_capella_received_block` ; le registre obtient à nouveau une époque
de sortie. La préparation ne modifie pas le checkpoint vivant du scénario,
qui peut ensuite poursuivre ses contrôles sur les deux branches originales.

Ce cas qualifie la réinclusion dans une transition de bloc valide, pas sa
finalisation sur le WAN. L'inclusion durable et le paiement finalisé sont
qualifiés dans les campagnes réseau distinctes. Les huit rejets contextuels
et les quatre cas WAL d'admission sont également rejoués dans cette sélection.
