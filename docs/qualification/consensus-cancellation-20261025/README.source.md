# Annulation coopérative du registre et des opérations — SOURCE ONLY

Base R annoncée par Root : `2ec642be`. Ce lot ne modifie que la copie sous `source/`. Aucun compilateur, test, Node, nœud, Docker, SSH, Git, donnée privée, DB ou campagne n'a été exécuté/lancé par l'auteur. Les changements restent à qualifier par Root ; aucun ancien échec ni résultat n'est réécrit.

L'inspection de tous les validateurs dans `inspect_exit_registry` ne rappelait pas le checkpoint du caller. Certaines API de sortie/proposer-slashing l'abandonnaient complètement, comme leurs callers Altair/Capella. Le calcul des pénalités faisait aussi plusieurs parcours sans annulation. Le correctif transmet ce même callback aux parcours réels, aux comparaisons du tri d'activation, à la copie privée du registre/buckets et aux écritures SSZ de sorties déjà faites dans un candidat local. Il ne change aucune formule d'activation, churn, délai, signature, domaine, sanction, récompense, gas, règle de profil ou limite de validateurs.

Le helper privé `src/consensus_registry_checkpoint.hpp` conserve la copie historique rapide si le callback est vide ; sinon il construit un candidat ligne/bucket par ligne/bucket avec annulation. Les sanctions/sorties/avancements d'époque modifient uniquement cette copie, puis vérifient le callback **avant** le swap final sans rappel après mutation du caller. Une `ExecutionCancelled` traverse les couches sans être convertie en preuve invalide ou en `nullopt`. Les sorties SSZ Altair restent des données locales abandonnées si une exception survient ; aucune publication durable n'est ajoutée.

## ABI et objets réellement concernés

Les anciennes signatures restent définies, comme wrappers appelant les nouvelles surcharges avec callback vide. Les nouvelles surcharges n'ont pas d'argument par défaut, évitant les appels ambigus. Huit API sont concernées : `ConsensusCommittee::verify`, `verify_attester_slashing`, `process_phase0_attester_slashing`, `process_phase0_proposer_slashing`, `process_phase0_voluntary_exit`, `process_consensus_voluntary_exit`, `phase0_slashing_penalties`, `advance_phase0_slashing_epoch`. Aucun membre de données ou disposition de classe n'est changé. L'API registry-attester qui possédait déjà un checkpoint conserve exactement sa signature.

Neuf unités Core doivent être recompilées et reliées à la candidate distincte, avec leurs nouveaux M/MD et sources :

| Unité `src/` | Changement |
|---|---|
| `consensus_slashing.cpp` | Inspection, staging, opérations, pénalités et wrappers ABI. |
| `consensus_committee.cpp` | Vérification d'indices coopérative ; BLS entouré de contrôles. |
| `consensus_block_operations.cpp` | Copie privée et transmission au proposer-slashing. |
| `consensus_registry_operations.cpp` | Transmission aux sorties volontaires. |
| `consensus_altair_slashing.cpp` | Transmission au proposer-slashing, parcours candidats et buckets. |
| `consensus_altair_transition.cpp` | Transmission aux sorties et contrôle du writeback SSZ local. |
| `consensus_capella_reactor.cpp` | Transmission au validateur de sortie avant tout publish. |
| `consensus_altair_slots.cpp` | Transmission au calcul des pénalités d'époque. |
| `consensus_epoch.cpp` | Même transmission dans l'orchestrateur Phase0. |

Trois headers publics étendent les déclarations sans retirer leurs anciens symboles ; le header de staging est privé. Les neuf unités ci-dessus sont les implémentations modifiées, pas une preuve que la fermeture MMD des headers se limite à neuf objets. Root détermine les autres recompilations et admissions nécessaires depuis les dépendances réelles. Les objets conservés gardent leur provenance et leurs headers historiques, avec leurs appels ABI inchangés ; aucune fermeture entièrement compilée contre les nouveaux headers n'est revendiquée. Cela ne dispense pas Root de vérifier la composition et les liens réels. Aucun Core historique, ancien binaire ou admission n'est requalifié par simple substitution d'objet.

## Nouveaux contrôles C++ préparés

Le nouveau `nano-consensus-cancellation-tests`, CTest `consensus_registry_cancellation`, contient huit groupes sur un registre composant de 8 192 lignes. Seules les trois clés BLS publiques fixes déjà utilisées par les fixtures unitaires signent ; les autres lignes sont des valeurs distinctes opaques de parcours, jamais des validateurs admis/PoP ou des nœuds réseau. Le test ne qualifie donc pas un réseau de 8 192 nœuds, ni l'admission cryptographique de toutes ces lignes.

Un doublon volontaire à la dernière ligne rend causal le premier contrôle : la même `ExecutionCancelled` doit remonter après 64 visites, **avant** l'invalidité tardive ; l'ancien code de registry-update atteindrait d'abord ce doublon sans rappeler le callback. Les autres groupes annulent pendant la copie et au dernier checkpoint de vrais parcours, contrôlent l'état complet égal à son entrée après interruption et interdisent la conversion d'exception ou la poursuite. Ils couvrent mise à jour/activation, sorties et dispatcher, proposer-slashing, registry-attester, committee-attester, pénalités différées et swap d'époque. Les positifs sans annulation comparent anciennes/nouvelles API et des montants/époques attendus explicitement. Les huit API historiques sont effectivement appelées ; deux signatures sont aussi sélectionnées par pointeur pour vérifier la résolution des surcharges.

Après compilation, Root doit exécuter ce nouveau contrôle et les régressions réellement touchées : opérations/sanctions et epoch Phase0, sorties volontaires officielles, slashing Altair et chemin local Capella de sorties. Les noms existants pertinents sont notamment `consensus_slashing_transition`, `consensus_slashing_epoch`, `consensus_phase0_registry_operations`, `consensus_phase0_ordered_epoch`, `ethereum_official_registry_epoch_updates`, `ethereum_official_voluntary_exit`, `ethereum_official_altair_slashing`, `consensus_capella_exit_wire` et `consensus_capella_local_exits`. Root choisit les sélecteurs et leurs liens candidats exacts ; ni rejeu EVM complet, ni flood, ni campagne 05 ou nouvelle identité réseau ne sont demandés. Les anciens tests inchangés ne servent pas de substitut aux nouveaux contrôles d'annulation.

## Limites maintenues

Il s'agit d'annulation **coopérative**. Un appel BLS en cours, une allocation/copie contiguë SSZ existante et la destruction des copies lors du déroulement d'une exception ne deviennent pas préemptibles. Aucun hard wall, RSS maximale, résistance réseau sous charge, rollback disque, reprise de signataire ou cycle complet n'est démontré ici. Les vieux codecs de persistance Phase0 sans API d'annulation ne gagnent pas de nouvelle promesse. L'audit historique à mémoire croissante est documenté séparément dans `consensus-production-gap-audit-source-01` ; il n'est pas corrigé par ce lot. 04 reste signée, fermée et FAILED ; WAN A, les autres exigences, la revue indépendante et la production restent ouverts.
