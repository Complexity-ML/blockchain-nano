Mise à jour du 14 octobre 2026 — diagnostic UTC en lecture seule.

Le CPP pur d496 a été compilé et lié une fois sur Docker Linux ARM64 et AMD64, sans recompilation du cœur. Chaque architecture a exécuté huit nouveaux appels : sept refus exacts et une observation UTC projetée, sans succès forcé. Calcul ARM64 : 8 946 ms / 17 860 KiB ; AMD64 : 43 729 ms / 17 408 KiB. Les preuves AMD sont filtrées et attestées par ROOT in situ ; les logs complets restent sur le VPS.

La nouvelle cible CMake est seulement proposée, non configurée ni installée. La fenêtre startup 2 h / 60 pas et l’horloge publiée ne changent pas. Ces essais synthétiques n’authentifient aucune capture actuelle ; privé, signatures, reprise réelle, V37/NAS, CI billing et audit externe ne sont pas promus. La production reste non qualifiée ; le diagnostic ne démarre aucun runtime ni duty.

---

