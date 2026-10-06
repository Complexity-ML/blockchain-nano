# Codec de manifeste Capella fragmenté

Source de départ main `5fff149c`, stockage immuable de cette même révision. Le codec local `src/consensus_capella_checkpoint_fragments.hpp` remplace les lignes complètes du graphe d’un checkpoint canonique par des références `[racine, SHA256, longueur]`. Le manifeste engage aussi la longueur et le SHA256 du checkpoint complet d’origine. Les autres métadonnées, votes et opérations restent dans le document. Le codec n’est pas une validation consensus ; l’entrée doit déjà provenir du sérialiseur Capella validé et l’expansion doit encore passer par restore_durable.

L’expansion vérifie le hash externe du manifeste avant de parser ses références ou d’appeler le lecteur de fragments. Elle vérifie structure canonique, ordre/absence de duplication des racines, bornes de nombre et d’octets, longueur/hash exacts de chaque fragment, correspondance de racine, puis longueur/hash exacts du checkpoint restitué. Un fragment manquant ou altéré est refusé, sans remplacement implicite par une autre source. L’interruption est propagée. Formats d’entrée testés : document store seul et enveloppe durable avec métadonnées et opérations.

## Preuves précises

Docker ARM64 original 48446/0 et Docker AMD64 original 99548/0. Une fixture nouvelle compilée/reliée par architecture ; archive de stockage parent montée en lecture seule et hash inchangé. Images existantes e30ce85e/fd9b0bb9, aucune reconstruction d’image, aucune recompilation du cœur, aucun réseau lancé. Sources figées vérifiées inchangées et identiques au worktree ; métadonnées Apple du VPS listées séparément.

La fixture synthétique contient trois lignes de records, un checkpoint de 25269 octets et un manifeste de 854 octets. Les fragments restent persistés séparément : **ce n’est pas une réduction mesurée du volume total ni des IO de Nano**. Restitution byte-for-byte des records/métadonnées/opérations et réouverture du manifeste RocksDB en lecture seule passent. Quinze refus : digest externe erroné (zéro fetch), champs supplémentaires, longueur/digest global invalides, substitution de fragment, longueurs nulles/débordantes/non canoniques, racines dupliquées/inversées, digest de fragment absent, fetch absent/altéré et budgets de nombre. Ces refus ne modifient pas le checkpoint existant.

## Limites et raccordement requis

**Codec implémenté et qualifié en stockage synthétique ; réacteur/runtime non raccordés.** Aucun checkpoint réel restauré en ce nouveau format dans cette qualification, aucune migration automatique, aucun témoin séparé de fragments, aucun cache de branches froides. Le writer actuel et la candidate runtime `02ec0ab0` conservent leur format complet et leur limite 1024. La saturation V28 reste ouverte. Pas de qualification native macOS, physique ARM64↔AMD64, EVM ou WAN de ce codec.

Prochain lot : intégrer manifeste/fragments dans la publication du réacteur et dans le témoin séparé ; restaurer les anciens checkpoints et reprendre avec la nouvelle représentation sans perdre les engagements externes. Puis remplacer la rétention complète des états par un cache borné/rechargement authentifié, maintenir le fork choice et qualifier saturation, archives absentes/altérées, interruption/récupération et finalité commune. Tous les échecs et critères originaux, revue indépendante, bridge et Doppler restent ouverts. Production non qualifiée.
