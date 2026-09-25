# Bloc Bellatrix signé et exécution EVM

`prepare_bellatrix_state_transition` traite un bloc Bellatrix signé à partir de
parents Beacon et EVM authentifiés par l'appelant. Il retourne un candidat
contenant les deux nouveaux états et les reçus, sans publier dans le choix de
branche ni écrire en base.

Le parcours vérifie le schéma du corps, avance les slots/epochs Bellatrix,
vérifie le parent Beacon, le proposant, sa signature BLS et son reveal RANDAO.
La vérification du reveal prépare un candidat privé ; le mix du nouvel état
n'est modifié qu'après l'exécution. Le payload EVM utilise donc le mix précédent,
le timestamp dérivé du slot et le parent d'exécution enregistré dans l'état.
Viennent ensuite Eth1, les slashings, attestations, dépôts, sorties, l'agrégat
sync et la comparaison avec la racine finale annoncée par le bloc signé.

Les opérations communes restent celles du parcours Altair qualifié. Une
projection interne et réversible permet de les réutiliser, avec paramètres de
pénalité Bellatrix. Les racines du corps, de l'état complet et des slots restent
Bellatrix. Le dernier champ d'exécution est conservé lors des reconstructions
de listes ; il n'est pas omis du hash final. Les APIs publiques Altair restent
distinctes et n'activent pas ce parcours.

`prepare_bellatrix_unsigned_block` construit un candidat local sans signature
de bloc. Il valide le proposant, le reveal, l'exécution et les autres opérations
pour dériver la racine à signer. Il ne constitue ni une admission, ni une
autorisation de signer : la liaison tête/slot/horloge au journal anti-slashing
doit encore être intégrée au reactor Bellatrix.

Les budgets de taille et de slots sont vérifiés. Les exceptions d'annulation
restent des interruptions locales, y compris pendant le calcul de racine du
corps. Un refus tardif, après le rejeu EVM, ne modifie aucun état parent.

Source normative : [Bellatrix block processing](https://ethereum.github.io/consensus-specs/specs/bellatrix/beacon-chain/#block-processing).
Ce parcours est limité à une continuation post-Merge à partir de parents déjà
authentifiés ; l'activation du fork et la validation du bloc terminal PoW ne sont
pas implémentées par cette API.

## Qualification du parcours complet en mémoire

`consensus_bellatrix_signed_execution_chain` part d'un état de validateurs dérivé
du cas officiel Bellatrix `sanity/slots/slots_1`, épinglé au commit
`bc5c1a7fb2a8871aaffd4b16ee4dd9c72bb81908`. L'en-tête d'exécution initial est
remplacé pour lier cet état à une allocation EVM synthétique. Les clés BLS de
fixture sont vérifiées contre le registre. Aucun fonds réel n'est utilisé.

Le scénario produit et vérifie neuf blocs avec sélection des proposants parmi
64 validateurs, signatures BLS réelles, neuf transactions signées de type 2,
incréments de stockage EVM et franchissement d'une frontière d'epoch. Une preuve
de double proposition est incluse dans le dernier bloc. Les balances de chaque
validateur sont comparées à un bloc témoin sans preuve : pénalité Bellatrix et
récompense du proposant doivent être exactes.

Sept blocs invalides sont refusés : signature, racine finale, parent Beacon,
reveal RANDAO, hash d'exécution, agrégat sync et proposant non sélectionné. Les
enveloppes sont resignées lorsque nécessaire pour atteindre le contrôle visé.
Sept interruptions et les trois budgets (état, bloc, slots) sont vérifiés sans
mutation des parents.

Preuves : `build-cmake/qualification/bellatrix-signed-execution-20260925/`.
Les premiers logs conservent une dépendance d'en-tête BLS manquante dans la cible
de test et une mauvaise sélection du fichier de configuration de l'horloge ;
la configuration finale est épinglée par son SHA-256. Le binaire `nano` est
reconstruit et les six suites ciblées incluent les blocs Altair officiels,
les slots Altair/Bellatrix, les conteneurs et la liaison Beacon–EVM.

## Limites et prochain raccordement

Construction et réception partagent le même parcours : ce scénario n'est pas
une comparaison indépendante du post-état Bellatrix avec un autre client.
Les autres opérations sont présentes dans le code commun mais ne sont pas
toutes exercées dans cette chaîne synthétique. La présence de 64 clés ne signifie
pas que 64 processus réseau ont participé ; aucune finalité distribuée n'est
revendiquée pour ce scénario.

L'admission Bellatrix dans le choix de branche est maintenant raccordée en
mémoire (voir [BELLATRIX-FORK-STORE.md](BELLATRIX-FORK-STORE.md)). Restent la
signature locale durable, le commit atomique et la reprise Beacon–EVM, puis leur qualification
sur plusieurs nœuds et sous pannes. Le parcours en mémoire ne prouve pas encore
une aptitude à la production et n'active aucun fork du réseau existant.
