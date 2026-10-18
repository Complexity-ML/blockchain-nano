# Hôte AMD main/HTTP V3 — borne Docker et nouveaux espaces

Successeur source seulement du V2 gelé `../main-http-amd-host-proposal-v2-20261018`. Le premier essai AMD Root `94771` a réellement échoué avant Nano : huit entrées tenues, zéro processus Nano et zéro HTTP. Le diagnostic public constate `/usr/bin/docker` à 47 253 395 octets, nlink 1, au-delà de la borne générique de 32 Mio. Aucun échec C++ ou changement d’autorité n’est établi par ce refus.

Le delta produit est limité à trois substitutions : borne de 64 Mio uniquement lorsque le chemin physique lu est `/usr/bin/docker`, `TOOLS` vers `/home/ubuntu/main-http-persistence-tools-20261018-02` et `OUT` vers `/home/ubuntu/main-http-amd-qualification-root-20261018-02`. La ligne Docker originale reste issue du contrôle ac2 authentifié ; full9, SHA, nlink et stabilité intra-FD restent stricts. Tous les autres chemins, dont les providers Python, restent bornés à 32 Mio. Les deux espaces 01 et le parent V2 restent intacts et FAILED n’est pas reclassé.

Root seul peut exécuter, après staging des cinq fichiers publics gelés :

```sh
PYTHONDONTWRITEBYTECODE=1 python3 /home/ubuntu/main-http-persistence-tools-20261018-02/host.py --revision bc99b0ec2cf8bc6bcb21a26ec980ef2c4e8573a8 --python-runtime-control /home/ubuntu/main-http-python-provider-root-20261018-02/control.json 6d1969aac717c7f980a6845756158d52fe5ce0436c1c13511094ac019df97100 241262
```

Le runtime Python copié de test et son contrôle externe restent ceux du V2 : montage RO unique, `-S`, cinq variables explicites et `PYTHONPATH` vide, lignes Root tenues directement, fichiers vides connus seulement, inventaire et clôtures après cleanup. Ce provider n’est pas annoncé comme Python intégré à l’image ni runtime de production.

Worker `2959cc8a…`, fixtures, image fd9, limites CPU/mémoire, 57 RPC, cinq vecteurs, trois serveurs et deux `mine`, nettoyage possédé, premières erreurs et fences restent inchangés. Les quatre readbacks sont désormais relatifs à OUT02 : `result-public.json`, `host-final-fence-public.json`, `outputs/worker/result-public.json` et `outputs/worker/proof-vectors.jsonl`. Node indépendant reste réservé à Root. Aucun build, test C++ rejoué, nouveau wallet, clé, duty ou campagne ; aucune finalité Beacon ou aptitude production revendiquée. L’auteur n’a ni exécuté/importé le contrôleur, ni lu SDK/providers/autorités privées, ni écrit dans R.
