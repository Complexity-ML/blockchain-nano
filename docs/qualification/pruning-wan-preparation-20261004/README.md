# Préparation de la restauration WAN avec pruning

Banc de test sur feature/finalized-retention-recovery, commit 092a153c.
Sélecteur NANO_TEST_HISTORICAL_STATE_PRUNING=1 uniquement avec campagne
Capella backup/restore. Configuration clonée et pruning automatique/historique
explicitement activés sur les trois nœuds. Audits initial et terminal exigent
rejeu d’états réellement élagués depuis le genesis : nombre entier positif,
canonicalHistoryVerified true et allHistoricalStatesMaterialized false.
Vérification syntaxe du harness et trois tests de contrôles exit 0 ; refus
sélecteurs/profils incorrects, mutation du config d’origine et faux audits.
Ce résultat prépare une campagne : aucun réseau WAN avec pruning lancé ici.

Contrôleur AMD64 indépendant déjà actif à révision 94535766, image et cœur
4734f285 réutilisés, deux nouveaux scénarios seulement. Sources/binaries
avant/après, délais 120 s stockage et 180 s Capella, fixtures sur disque
persistant dès le départ. Rapports live sur le VPS :
/home/ubuntu/nano-pruning-recovery-amd64-control-20261004.
Stockage terminal exit 0 à cette observation ; Capella encore en cours.
Pas de qualification générale ni de runtime promu sur main.
