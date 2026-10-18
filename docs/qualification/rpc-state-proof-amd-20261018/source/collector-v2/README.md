# Collecteur AMD V2 — joint des origines et admission des copies

Source Root proposée, **non exécutée par l’auteur**. Elle corrige seulement le
joint des chemins de la V1 et utilise un nouveau namespace d’audit. La V1, ses
sorties exit1 et le refus public restent intacts. Le manifeste ci-joint est
final ; ne modifier aucun fichier après les pins et GO de revue.

## Échec conservé et correction exacte

La V1 `amd-rpc-continuation-audit-source-20261018/collect.py`
(f084b785… /28 578 B) a réellement terminé exit1 avant lecture du dispatcher,
SSH ou Node. Le refus Root public
`amd-rpc-continuation-collector-source-origin-join-refusal-root-20261018.json`
conserve ce résultat. Le collecteur s’était trompé de chemin en appliquant les
rows FD9 historiques des sources originales aux copies distinctes UNIT/source.
Les quatre copies avaient déjà les mêmes octets ; leurs inodes, modes et dates
étaient différents par construction. Aucune mutation récente de ces copies
n’est établie par cet échec. Il ne s’agit ni d’un échec du worker C++ terminé,
ni d’un rejet des preuves par Node : ces étapes n’avaient pas été invoquées.

La source du qualificateur public original
`qualify-rpc-proof-verifier-js-root-20261017.py` décrit quatre origines précises :

* `S/tests/qualification/rpc-proof.mjs` et `S/tests/qualification/rpc-proof.test.mjs` ;
* `R/tests/qualification/account-proof.mjs` et `R/tests/hardhat/package.json`.

S = `E/rpc-eip1186-account-storage-proof-source-proposal-20261017/source` ;
R = `/Users/boris/Dev/blockchain-nano` ; E est
`/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012`.
`newSourcePins` est joint **à ces quatre chemins originaux uniquement**. Chaque
row publique doit égaler exactement la row correspondante dans l’autorité
historique et celle déjà vérifiée/held. Aucune row originale n’est remplacée.

Les trois fichiers réellement chargés par le driver sous UNIT/source restent
les copies distinctes rpc-proof.mjs, account-proof.mjs et hardhat/package.json.
Ils sont ajoutés à l’admission prospective existante : trois observations FD9/SHA
identiques sur ≥2s, SHA et taille connus à chaque observation, avant Node.
Leurs pins restent respectivement b59224b4… /5340 B, 2a07c2f6… /2439 B et
39a64e23… /550 B. Aucune autorité FD9 historique des copies n’est revendiquée :
le qualificateur original n’en avait jamais enregistré. La copie rpc-proof.test
n’est pas requise par le driver et aucun des huit tests JS n’est rejoué.

Les trois champs de provenance publics rapportent le nombre d’origines jointes,
le nombre de copies admises prospectivement et l’absence de revendication FD9
historique des copies. Si une étape échoue, aucun compteur ultérieur n’est
présumé réussi.

## Namespace et commande Root futurs

* HEAD attendu4388f77b9e9c1b3b94ba5da6f3e486eaa13a3f5c.
* C++ séparé C6c6cdf780c68eab8b06306d76257bda9028928ea3 ; runtime1925 reste distinct.
* Ancien output local01 conservé, jamais réutilisé.
* Nouveau output exclusif `E/rpc-candidate-amd-continuation-public-audit-root-20261018-02`.
* Dispatcher existant `E/amd-rpc-continuation-dispatch-root-20261018-01`.
* Même sortie distante `/home/ubuntu/rpc-candidate-amd-continuation-root-20261018-01`.

Root, après revue/admission source et vérification de la vraie sortie terminale
du dispatcher, pourra lancer uniquement ce nouveau consommateur :

```sh
python3 /Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012/amd-rpc-continuation-audit-source-v2-20261018/collect.py \
  1f20ae6cde3a0de220376b260395f86e1eef025306d5154b2889fbbe705440a6 8680
```

Le pin de l’unique autorité locale Node est fourni en argv et comparé avant sa
lecture. L’auteur n’a pas lu cette autorité, de SDK ni de contrôles privés.
La V2 ne modifie pas l’autorité ni les dépendances originales.

## Contrat fonctionnel conservé

`delta-v1.patch` donne le delta exact contre la V1 gelée ; `source.patch` donne
le patch complet contre le parent113284fc… /10 803 B, lui aussi intact.
Aucun comportement indépendant de la correction n’est ajouté :

* Même56 fichiers publics exacts :4fixes,4candidats×4 et9CLI retenues×4 ;
  aggregate16Mio/wire32Mio/stderr1Mio, lecture distante et locale bornées.
  Zéro fichier privé transféré ; les observations readback ne sont pas des
  autorités historiques. Une lecture SSH bornée120s consomme les seuls résultats
  déjà produits ; aucun host/worker/test C++ n’est relancé.
* Même contrat host582389ce… /39 355 B, source4388, image AMD inchangée,
 4M/CXX/liens/ldd/tests candidats, coreProof:null/mmd4,9CLI séparées,
 265 anciens produits et16 nouveaux tenus jusqu’après CLI/cleanup/image-final.
  Core143 membres/44 sélectionnés/99 inchangés conservé. La première enveloppe
  FAILED/TimeoutExpired reste FAILED ;156M combinés incluent son ancien scan
  proof interrompu, sans répétition Core/runtime.
* Même driver94b5a31b… /3291 B, modules déjà qualifiés aux pins connus,
  données effectives du stdout proof vérifiées en24 vecteurs par une invocation
  Node bornée60s. Le résultat lie le SHA réellement transféré ; aucune invocation
  de test JS, RPC ou C++ supplémentaire. L’égalité éventuelle Native/ARM reste
  séparée du critère de validité cryptographique.
* Première erreur conservée, erreurs secondaires distinctes, cleanup du seul
  enfant créé et jamais PID28722. Toutes les clôtures finales sont tentées
  indépendamment. verifiedProofVectors24 exige aussi les fences et la publication
  finales ; sinon0 avec verifierReportedVectors séparé. Root doit vérifier le
  vrai exit du collecteur, pas seulement un JSON.

## Revue et limites

L’auteur a seulement lu les sources/reçus publics, préparé ce nouveau dossier
et calculé les pins/patchs. Aucun import/parse/exécution du collecteur, test,
C++/Node, SSH/Docker ou opération R/Git. La revue source/AST du pair et les
contrôles Root restent à effectuer après livraison de ces octets finaux ; un GO
source n’est pas une exécution du collecteur ni une qualification réseau.

C6 n’est pas importée à main ; pas de production, nouvelle campagne04, devoirs,
finalité, fonds, vente, bridge/publication ou nouvelle dépense autorisés ici.
L’échec V1 et le refus Root, l’ancienne enveloppe AMD FAILED et tous les anciens
journaux/autorités restent conservés sans reclassement. Cette correction ne
renouvelle aucune autorisation consommée.
