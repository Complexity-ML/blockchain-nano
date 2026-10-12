# AMD64 Linux — contrôle indépendant des données privées exportées

Le même source de helper, SHA-256 `25223c9adc0ca0afffa01a40ef5e0dcc9eee5cb4fc81d23daee65d77ef99a312`, a été compilé et lié sur le VPS autorisé, puis exécuté une seule fois sur l’export AMD64 réel. Compilation0/link0 ; binaire helper `ef164274c7a88a8bd24be7f5211cae5f0e764152b8724f8a20a85a7ee50a87f8`. Archive core85 `783ccae2127905741238dd58618124f9582aae380625ef3fe592523a58a056dd` réutilisée, image existante `fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14` sans rebuild.

Le conteneur du helper avait user1000, 1CPU/2Go, réseau désactivé, filesystem en lecture seule, toutes capacités retirées et `no-new-privileges`. Le volume des parents et la fixture étaient montés en lecture seule. Seuls ses outputs de compilation étaient inscriptibles pendant le build ; le contrôle utilisait `argc=2`, sans chemin de journaux. Aucun nœud, RPC, génération de signature ou callback de signataire n’a été lancé.

Exécution réelle : exit0, environ0,265s. Manifest contrôlé `c3bfa46d1f9b8234faf88c0ecd6eddc7f5685e1944beb398622c0f71522aeb6d` ; campagne `b8e4445eeb935542127c8d36deeb3a50031b104bbe31cb06b3b057f893b39170`. Résultats acquis :

- Deux fichiers d’environnements uid1000/mode0600, 65 scalaires BLS big endian avec round-trip et correspondance exacte aux clés publiques/configurations.
- 129 clés publiques dans leur groupe, 64 credentials de retrait et deux agrégats sync recomputés.
- Quatre racines et signatures BLS d’amorçage vérifiées ; transaction de dépôt ECDSA, émetteur, calldata et allocation de financement concordants.

Les seeds, environnements privés et inventaires privés ne sont pas téléchargés. L’inventaire avec les métadonnées/hashes avant/après reste mode0600 dans le dossier dédié du VPS `isolated-private-binding-audit-amd64-20261012/runtime`. Les sources privées de retrait et d’émetteur ECDSA ne sont pas disponibles dans l’export : aucune correspondance de ces scalaires privés n’est revendiquée. Seed lue : non ; journaux créés :0 ; signatures/callbacks :0.

Seuls les reçus publics sous `build/` et `runtime/` ont été récupérés ici. `runtime/run-receipt.json` et `runtime/audit.log` lient la sortie et le vrai manifest. Les hashes de la fixture et de tous les parents sont identiques avant/après. Le helper a une architecture AMD64 native, avec archive et dépendances Linux cohérentes ; il qualifie ici le traitement cryptographique des données réellement exportées sur ce système. Il ne démarre pas le runtime Nano ni ses duties, ne teste pas les RPC, ne copie pas de journaux et ne démontre aucune finalité réseau.

La nouvelle campagne AMD64 a un domaine distinct des campagnes natives/ARM64. Ses données ne doivent pas être mélangées aux 65 journaux natifs préparés pour la campagnecb532. V37 resteFAILED et la production reste non qualifiée.
