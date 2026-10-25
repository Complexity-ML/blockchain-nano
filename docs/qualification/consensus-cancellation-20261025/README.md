# Consensus : annulation coopérative du registre

Les neuf unités Core modifiées de la candidate C++ `e53e06c05f1befa7fa3aeb4970f22987c073d1e3` ont été compilées dans les images Docker ARM64/Mac et AMD64/VPS existantes. Les changements JavaScript ultérieurs des candidates `2a9b947b` puis `21a92877` ne modifient pas ces sources C++. **Dix invocations de régression sont qualifiées par architecture**, en conservant leur provenance exacte ; aucun réseau de validateurs n’a été lancé pour ce lot.

Les callbacks sont transmis pendant l’inspection et la copie du registre, les pénalités, les tris d’activation, les sorties et les sanctions, ainsi qu’à leurs appelants. L’exception d’annulation est propagée ; les mutations sont préparées hors de l’état appelant et le dernier contrôle précède sa publication. Les huit signatures historiques restent disponibles via leurs wrappers, sans changement de disposition des objets ni des règles économiques. Les durées physiques d’un appel cryptographique/OS ne deviennent pas préemptibles par cette API.

Le nouveau test exécute **28 cas répartis en huit groupes** avec 8 192 lignes synthétiques de registre. Trois clés déterministes de fixture signent ; les autres lignes sont des données opaques de parcours. Cela ne représente ni 8 192 validateurs admis ni 8 192 nœuds. L’annulation précoce/tardive doit conserver tout l’état initial et la même exception. Les comparaisons positives vérifient les anciennes/nouvelles API et des soldes/pénalités explicites.

| Résultat externe | ARM64/Mac | AMD64/VPS |
|---|---|---|
| [Terminal](arm64-terminal-public.json) / [terminal](amd64-terminal-public.json) | Docker exit0, 13,009s | Docker exit0, 245,949s |
| Huit invocations déjà terminées | Réutilisées, sans rejeu | Réutilisées, sans rejeu |
| Capella `--local-exits` et `--exit-wire` | Deux nouveaux exit0 sur binaire existant | Deux nouveaux exit0 après compilation du seul driver Capella |
| Recompilation Core dans la continuation | Aucune | Aucune |
| Optimisation Core / driver Capella | O3 / O3 | O3 / O1 |
| FD tenus / fermés | 2 967 / 2 967 | 2 961 / 2 961 |

Le driver de test Capella AMD64 a dépassé son ancien timer de compilation O3 de 240s. Ce FAILED reste conservé ; seule sa compilation a été reprise en O1 avec timer600s, puis les deux modes exécutés. Le Core de production testé reste le même archive O3. Ce résultat ne revendique pas une compilation O3 achevée du driver AMD64, un benchmark de production ou un build CMake propre complet.

Chaque archive conserve143 noms de membres uniques : neuf objets remplacés sont identiques aux objets compilés, les134 autres payloads sont identiques à l’archive antérieure. Les18 unités réellement compilées ont des dépendances indépendantes `-MM` concordantes avec leurs `.d` de compilation. Les134 objets conservés ont leurs headers historiques : la fermeture MMD de tout le Core contre les nouveaux headers n’est pas revendiquée. Les anciens symbols restent effectivement liés et appelés par les tests.

Les huit invocations réutilisées couvrent le nouveau test d’annulation, les transitions de slashing, l’époque de slashing, les opérations de registre, la transition d’époque, les fixtures officielles de registre et de sorties, et le slashing Altair. Leurs markers terminaux exit0 et stdout sont retenus dans les résultats ; la compilation, le lien et les loaders ont aussi leurs reçus. Les deux modes Capella vérifient sorties/rejets, files, réinclusion et réouvertures de fixture ; `networkActivated:false` reste explicite. Ils ne prouvent pas un cycle de validateurs sur trois machines.

Les [échecs historiques de ces contrôles](historical-attempts-public.json) demeurent FAILED : refus de fixture SSZ vide à l’admission, en-têtes réseau omis de l’archive, refus de métadonnées Mac non attribué, premier contrôle du tar Mac refusé, fixtures runtime Capella manquantes et timer de compilation AMD64 dépassé. Un mauvais répertoire du préparateur Root a aussi laissé l’ancien plafond FD pour la première continuation ARM, qui a refusé avant Docker. Les nouvelles admissions prospectives des copies Git connues n’ont jamais rafraîchi les autorités conservées ; les fences finales des lots réutilisés et des continuations sont strictes. La réussite ultérieure n’explique pas rétroactivement le refus historique des métadonnées.

Ces contrôles de compilation/composants sont distincts de la campagne04 signée, terminale et FAILED. Aucun journal de validation, témoin, clé de campagne, volume existant ou domaine signé n’a été réinitialisé. Aucune image n’a été reconstruite. Restent notamment ouverts la reprise réelle des duties de la candidate sur trois environnements, le cycle complet des validateurs/sanctions, les limites RAM du rejeu d’archives, les partitions/WAN et ressources dans leur périmètre exact, la CI bloquée et la revue de sécurité indépendante. Aucune activation EVM/EIP, production prête ou autorisation de nouvelle campagne n’est déduite de ces résultats.
