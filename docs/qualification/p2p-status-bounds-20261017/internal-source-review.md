# Revue indépendante SOURCE — statuts P2P bornés

17 octobre 2026. **GO SOURCE**, qualification physique à effectuer par Root. Lecture des cinq overlays, de leur diff, du README et du manifeste ; vérification des sept payloads (109835 octets), des cinq baselines R et du `json_frame.hpp` inchangé. Le patch appliqué en mémoire aux baselines reconstruit exactement les cinq overlays. Aucun programme, compilateur, test, Docker/SSH, SDK ou fichier privé consulté ; aucune écriture R.

Référence relue : `p2p-status-bounds-source-20261017/manifest.json` SHA-256 `38291606640b1ebd2117174334de34ffc1726f20e7e8c88978670b76c81c6f62`, 2749 octets. Gel final vérifié : manifeste `e7186c0453f0f95b2b7bc1177be774536991ad2b04b89a90942724c5e57c6848` /2762 octets ; inverser son seul statut redonne exactement le draft relu.

| Chemin produit | Contrôle indépendant |
|---|---|
| Handshake entrant | Lecture4096 avant accumulation/parsing, validation, projection dans `add`, réponse4096. |
| Handshake sortant | `exchange` limite requête et réponse à4096 ; seule la projection devient `Peer::status`. |
| Chainsync sortant | Requête vide bornée et réponse4096 ; validation puis projection avant remplacement dans `poll_status`. |
| Chainsync entrant | Réponse4096 ; le chemin historique ne lit pas de requête et ne retient pas de nouveau statut. |

`JsonFrame::feed` refuse le fragment excédant le cumul avant `data_.append` et avant le parseur. Le buffer fixe64KiB demeure : le patch borne l’accumulation, sans promettre un buffer4096. Le défaut des autres appels reste16MiB, avec `json_frame.hpp` byte-exact. Les signatures précédentes du lecteur parsed et du lecteur raw restent distinctes. Les routes blocs/BFT/fragments/SSZ ne sont pas modifiées.

Les deux seuls points de stockage distant (`add` et `poll_status`) projettent les17 champs reconnus. `best_hash` reste facultatif pour les anciens pairs ; s’il existe, sa largeur et son hexadécimal canonique sont contrôlés. Les extensions racine sont ignorées après réception bornée. Les objets Altair/Bellatrix/Capella sont copiés entiers : la règle historique des trois champs imbriqués, dont le refus d’un champ supplémentaire, n’est pas contournée. Le statut interne, ses drapeaux IPC, la session Capella, le protocole réutilisable et le slot annoncé gardent leurs usages existants.

Les nouveaux selectors `--status-bounds-self-test` évitent les anciennes branches. Le test de forme présente neuf hashes invalides, dont8192 octets, puis contrôle legacy, champs reconnus et compatibilité des trois annonces. Le test de flux appelle le vrai inline du produit :4096 fragmenté accepté,4097 fragmenté/monobloc refusés une fois,4097 générique accepté, écriture surdimensionnée refusée. Les callbacks et streams sont en mémoire. Contre les anciens headers, les premiers échecs attendus sont l’acceptation du mauvais hash puis celle de4097 octets ; aucune erreur de compilation ne doit être appelée preuve causale.

Un raccord de revue a été corrigé avant exécution : conserver directement le type `Callback` dans `requires` et les branches du helper de test, au lieu d’une conversion prématurée en `Reply`. Test final `baa9ae3a…`/7103 octets ; autre test `91f6b9af…`/6964. Sources produit inchangées depuis le premier peer : `main.cpp da64d4a5…`/59304, `peer_status.hpp73c65afd…`/2313, `json_stream.hpp c4f0780f…`/4243.

Aucun autre défaut matériel retenu. Le plan baseline ciblée puis deux fichiers candidats complets est cohérent avec la modification du lecteur partagé. Cette revue n’établit ni RSS/OOM réel, transport réseau, anti-Sybil, charge prolongée, nouvelle admission des binaires, finalité WAN ou production. Elle n’autorise aucune campagne privée.
