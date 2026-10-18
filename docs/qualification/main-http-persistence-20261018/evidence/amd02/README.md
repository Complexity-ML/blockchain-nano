# AMD02 : main HTTP et cinq preuves indépendantes passés

Le host V3 Root 10741 termine exit 0 ; la lecture indépendante Root 69605 termine
exit 0 après SSH et Node exit 0 / groupes fermés. Sources et anciens reçus
restent inchangés. Ils correspondent au main bc99 et au Nano C6
`cc0eed49d62d9decccf128b773241191d9ef39abc4cb2c90f876a4a8f9ece94b` / 13 997 336 octets,
dans l'image AMD64 `sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14` existante.

Le [host](dispatch/stdout.log) rapporte 57 appels HTTP, trois serveurs, deux CLI mine
(timestamps 12/24), deux réouvertures, trois TERM/join exit 0 et cinq preuves.
Wallets vides, mode dev désactivé, refus mine RPC, nonce/balance/code/storage,
transaction/reçu/historique passent. Worker : 2 821 ms ; enveloppe host : 23 068 ms.
Aucune compilation C++, image, clé, signature ou duty créée ; aucun ancien test rejoué.

La fence finale de 644 entrées conserve quatre lignes originales de logiciels
et 628 fichiers provider Python RO. Le contrôle 866 conserve son ancienne ligne,
sans revendiquer sa propre FD9 de construction initiale. Les 265 anciens produits
ne sont pas tous réaudités. Après nettoyage, fence et publication passent, CID absent,
image fd9 inchangée. Les images/volumes restent intacts.

La [lecture indépendante](independent-readback/result-public.json) récolte 97
fichiers publics / 96 961 octets puis vérifie cinq vecteurs MPT, hash des trois
en-têtes et hash/sender de la transaction existante, sans signature. Les vecteurs
sont identiques octet par octet à Native. Fence de 108 entrées PASS, SSH/Node
rejoints avec exit 0, zéro RPC en direct / compilation C++ / ancien test JS rejoué.
Les anciens flags `independentNodeVerificationPending:true` restent historiques ;
ce résultat séparé ferme leur contrôle borné.

Le [descripteur de staging](staging/source-descriptor-public.json) conserve le
pin de la source dont le paquet SSH Base64 est omis. Ses cinq payloads publics
sont archivés dans le paquet. Ni base, ni contrôle privé, ni dépendance SDK, ni
observation de copie Root ne sont publiés. Ce scénario reste PoW synthétique
Shanghai ; authenticité consensus/finalité Beacon et production restent false.
