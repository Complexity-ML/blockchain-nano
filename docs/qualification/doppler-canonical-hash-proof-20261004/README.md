# Preuve de migration attachée à un hash canonique

Le vérificateur local exige l'identité du reçu et de tous ses logs : hash de
transaction, numéro/hash de bloc, absence de `removed`, événement Airlock unique
avec champs `asset`/`pool` nommés. Les six lectures d'état utilisent EIP-1898
`{blockHash, requireCanonical:true}`, sans fallback par hauteur. Le header est
contrôlé avant et après. Une erreur invalide la preuve complète.

Cinq tests ciblés passent : lectures pinées, bloc remplacé avant lecture, reorg
au terme, bloc indisponible pendant lecture sans fallback, six mutations de
reçu/log/événement refusées. Ce sont des RPC contrôlés, pas des reorgs publics.
Le cycle complet sur le vrai Anvil privé passe ensuite exit 0 avec cette API,
achat/revente/graduation V2 et les trois transactions adversariales. Aucun test
C++ n'a été rejoué pour cette modification d'outil.

L'essai A échoue sur la revente : au parent l'appel identique passe, au bloc
miné une seconde plus tard il reproduit `V4TooLittleReceived(53353679,39696989)`.
Il n'atteint pas la migration et reste exit 1. B fixe explicitement les timestamps
locaux à +1 seconde par transaction et place le devis en début d'époque, sans
changer les limites de gas, proceeds ou la sortie minimale à 99 % du devis.
Les timestamps/devis sont enregistrés. Cela rend cette fixture déterministe,
sans prétendre empêcher le slippage sur un marché réel. Un contrôle de syntaxe
avait initialement un chemin relatif répété ; le contrôle au bon chemin passe.

Source normative vérifiée : https://eips.ethereum.org/EIPS/eip-1898.
La canonicalité est déclarée par le RPC : ce n'est ni une preuve cryptographique
de finalité Base/L1, ni un bridge, ni une garantie contre un fournisseur malveillant.
Anvil loopback 19546, chain 31337, fork Base 52173454, fonds fictifs, arrêté après
B. Profil Nano, backing de Nanobees Base/natif et paramètres réels restent ouverts.
