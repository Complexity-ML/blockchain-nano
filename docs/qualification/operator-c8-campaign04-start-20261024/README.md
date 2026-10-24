# Campagne04 : préparation et premier avancement commun

La campagne privée est **en cours**. Trois runtimes, sur deux machines physiques, ont démarré et partagé deux avancées de finalité (époques 3 et 4), trois lectures stables et le même bloc d’exécution finalisé. Les restaurations complètes, la reprise des duties, les sorties/retraits, la sanction et la clôture finale restent à démontrer. Ces observations ne qualifient pas la production.

Les sources Mac et VPS étaient à `f572653d9692e35f2602e73b268c95a0520f755e`; le lot JavaScript testé est `ed651411` et les bindings logiciels G2 gardent leur base `d23409b3c032be87de303c4864ec7dac557ced34`. Aucun binaire C++, profil EVM ou image Docker n’a été reconstruit ou modifié par ce lot. Paris/Shanghai demeure le périmètre activé; aucun EIP n’est promu.

## Préparation et conservation

L’accord humain distinct04 couvre 65 identités de test, dont 33 clés au VPS existant, et neuf heures maximum clôture comprise. Docker Desktop 29.8 a démarré sous cet accord. Les 110 conteneurs préexistants étaient arrêtés avec restart=no ; 128 volumes ont été conservés. Le premier contrôle a refusé le démarrage faute de disque VPS, avant tout pilote. Un nouveau contrôle a ensuite terminé réellement à0, avec trois mesures conformes et sa sonde indépendamment absente. Il n’a pas lancé de validateur.

Onze anciens arbres de sources du 5 octobre ont été archivés en privé sur le même VPS, puis leurs seules copies d’origine retirées après vérification. Les 11 archives conservent193 132 fichiers / 7 420 406 706 octets logiques dans 2 469 254 037 octets compressés. La relecture indépendante corrigée 02 a terminé à 0 : couverture exacte, SHA, modes/propriétaires, CRC gzip, 33 fichiers tenus et fermés, anciens noms absents; leurs hashes concordent avec les reçus d’opérations. La génération 01 de ce vérificateur reste NO_GO malgré son exit 0: son contrôle final de couverture était incomplet. Aucun ancien inode n’est revendiqué. L’espace VPS observé après les archivages était 8,21 Go; aucune image ni volume Docker n’a été supprimé.

## Échec initial et continuation sans prolongation

Le premier lancement du 24 octobre à 20:09:03.990 UTC a terminé avec exit 1 après 54 747 ms. L’export était vide et le domaine non réservé. L’exporteur dépendait d’un chemin de fixtures compilé sous /private/tmp désormais absent. Ses 7 685 fixtures / 227 734 656 octets ont été restaurés depuis les empreintes originales conservées (inventaire SHA f1cce7c2…). Les copies durables et de runtime ont de nouvelles identités physiques; le binaire n’a pas changé. Un premier copier ayant refusé une comparaison de stat incluant l’atime reste consigné; ses copies partielles ont été vérifiées avant continuation.

La même campagne 04 a repris à 20:19:38 UTC, avec les mêmes 65 identités, seed et registre anti-double-signature, sans suppression ni réinitialisation. Son budget est réduit à 31 690 586 ms, réserve de clôture 2 700 000 ms incluse. L’échéance initiale reste le 25 octobre 2026 à 05:09:03.990 UTC. Le contrôleur tient 7 685 descripteurs de fixtures; leur fence finale et fermeture restent à observer. Le domaine frais est maintenant signé et ne doit plus être réexporté ou réinitialisé.

## Portée des preuves

`initial-common-finality.json` atteste le premier avancement partagé de cette nouvelle campagne synthétique, avec lien au bloc d’exécution. Les résultats de fin, acteurs absents, restauration intégrale et cycle des validateurs restent ouverts; un fichier provisoire ne vaut pas terminal réel. Les restaurations historiques, partitions/WAN, grands historiques, charge bornée, CI, reproductibilité, revue indépendante et critères des quatre documents normatifs gardent leurs périmètres propres. Le dernier refus CI vérifié avant étapes pour facturation/plafond n’est pas une réussite. Base–Nano 1:1 reste proposé/non adopté; bridge USDC, Doppler et indexation T0 ne sont pas qualifiés. Aucun fonds réel, vente, déploiement public, flood, machine ou dépense supplémentaire.

La decote de 120 s du calcul de continuation n’est pas la marge restante au lancement : la marge effective observee est de 74,988 s avant la borne initiale.
