## 14 octobre 2026 — Raccordement bootstrap et pending qualifiés en laboratoire

Sur fixtures publiques/synthétiques, le raccordement owner existant, entrées authentifiées et reprise avant horloge passe six scénarios de démarrage sur Docker Linux ARM/AMD64. Pending non vide : cinq cas passent par architecture, quatre SIGKILL, deux pertes WAL explicitement simulées, queue/enveloppe et WITNESS conservés, reprise typée idempotente. Trois signatures synthétiques, enfants zéro. ARM : 1 065 ms/22 892 KiB ; AMD64 : 3 768 ms/24 364 KiB.

Candidat final `b6b7c450` : code de production compilé dans le lot `99de4496`, archive de 142 objets ; seul test pending V4 recompilé/relié. Échecs include/Genesis/horloge de fixture conservés. CLI offline V2/banc relus uniquement, non compilés/testés/appliqués. Copie privée, fenêtre de reprise UTC expirée, journaux, duties/finalité commune, V37/NAS, natif, reprise naturelle AMD64, CI facturation et audit externe restent ouverts. Quatre preuves locales non publiées. Profil Shanghai, EVM/RPC, forks, EIP/CSV et critères inchangés ; production non qualifiée.

