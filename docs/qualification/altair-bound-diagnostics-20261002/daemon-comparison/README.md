# Altair AMD64 : effet du diagnostic, sans correction revendiquée

Core figé c10426b2, sources réseau originales 418aceb8, image et bibliothèques statiques existantes. Même test/probe, trame pleine de 16 777 370 octets, assertions et délai CTest de 120 secondes conservés.

- Daemon recompilé avec traces de refus avant admission et ouverture acceptée : un cas passe en 3,91398 s.
- Daemon recompilé sans ces nouvelles traces : même cas échoue en 2,19625 s ; reset distant pendant l’écriture de la trame pleine, avant lecture ACK.

Les manifestes sources, programmes, bibliothèques cœur et réseau sont identiques avant/après chaque exécution ; vérification post-test à zéro. La recompilation seule ne suffit donc pas. La réussite instrumentée ne démontre ni correction, ni cause précise ; un effet de timing ou de génération de code reste possible. Aucun runtime promu, aucun critère de production clos.

Les deux échecs de préparation précédents sont conservés : collision de headers lors de la première compilation ; hachage du répertoire de contrôle au lieu de ses fichiers lors du second lancement. Ces tentatives n’ont pas exécuté le test.

Les fichiers volumineux sont compressés sans modifier leur contenu ; RAW-SHA256SUMS décrit les octets originaux. Les binaires, objets et bases temporaires restent sur le VPS et ne sont pas ajoutés au dépôt.

## Contrôle avec trace des refus uniquement

Le test reste en échec (2,17 s). La dernière trace montre `unknown-peer` sur le flux plein, avant admission. Les refus attendus des scénarios négatifs sont conservés. Le diagnostic localise le reset dans le contrôle des pairs, sans démontrer encore pourquoi le pair a été supprimé. Une comparaison suivante fait répondre le pair de qualification aux demandes de statut périodiques, avec annonce identique au handshake et daemon original inchangé. Aucun contrôle de sécurité contourné.
