# Restauration WAN prunée — terminal du 5 octobre 2026

Révision 092a153ca1d203434beb14fe2102e3a092dc454c, provenance des livrables dans ../pruning-wan-delivery-20261004. Contrôleur terminal exit 0 ; revue indépendante des reçus, engagements et audits exit 0. Trois processus de nœuds sur deux hôtes : Mac natif, Docker Mac, Docker VPS ; 64 clés synthétiques, dont 21 restaurées sur VPS. Réseau privé SSH ; aucune vente ou fonds réels.

Restauration depuis sauvegarde physique avec journaux, témoins et engagements conservés séparément. Finalité commune 2 → 3 en 80 506 ms (borne inchangée 180 000 ms), observation stable 5 → 8, racine finale 0xe688b7f18358a57a4803057e2108f6c794d6cfb207cb822f53cfc154deb9fd6e. Les 21 journaux ont avancé sans régression ; validateur restauré : 122 attestations, 19 propositions, 423 messages sync, aucun échec de duty rapporté. Source sauvegardée et fichiers des audits en lecture seule inchangés.

Audits canonique et ancêtre finalisé : 14 états prunés rejoués initialement, 59 au terminal ; 77 blocs et une transaction au terminal. Le système n'a donc pas seulement rouvert une base contenant tous ses états. Hashes, reçus et racines exacts dans result.json et les traces compressées, indexées par archive-manifest.json.

Limites : pas de qualification du fork choice historique ou des votes historiques autonomes, d'une panne électrique, d'un mesh public, d'un downgrade ou de toute la production. Le délai initial d'environ trois minutes avant l'avancement des heads reste observé sans cause démontrée. Le refus Unauthorized du diagnostic privé n'est pas une preuve de santé. Le pruning demeure sur branche isolée ; aucune promotion du runtime main.

Aucune clé privée, archive RocksDB ou configuration contenant les clés n'est ajoutée à Git. Toutes les erreurs et preuves antérieures demeurent conservées.
