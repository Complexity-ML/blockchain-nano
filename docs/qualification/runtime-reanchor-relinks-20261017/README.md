# Reliages des consommateurs du Core reactor — 17 octobre 2026

**Les trois environnements passent les reliages ciblés : 18 M, 18 liens et 21 contrôles CLI, zéro CXX.** Ce rapport distinct complète la [qualification exits/reorg/pruning](../exit-reorg-prune-20261017/README.md) sans recopier ses tests ni son historique. Il ne démarre aucun runtime/duty et ne qualifie pas de nouvelle campagne.

| Environnement et reçu public | Opérations réellement passées | Core déjà qualifié et repris |
| --- | --- | --- |
| [Native](evidence/native01/result-public.json) | 7 M/réemplois d’objets/liens, 8 CLI ; zéro CXX. | 3401bf77…/29252992 B. |
| [Docker ARM02](evidence/arm02/result-public.json) | 6 M/réemplois/liens/ldd, 7 CLI ; zéro CXX. | 36f77c8b…/41045640 B. |
| [Docker AMD01](evidence/amd01/result-public.json) | 5 M/réemplois/liens/ldd, 6 CLI ; zéro CXX. | a3fdf87a…/40811064 B. |

Native relie Nano, exporter, bootstrapImport, bootstrapAudit, stoppedCapture, lifecycleTool et lifecycleSubmit. ARM n’inclut pas exporter ; AMD n’inclut ni exporter ni bootstrapImport. Les pins de chaque produit sont dans les reçus. Aucune archive, commande P2P ou ancienne suite de tests n’est modifiée/exécutée par ces reliages. Les preuves 143 membres/142 inchangés du correctif reactor sont reprises, sans nouveau build Core. Source 1925eeeabc9f7d0aef41f2c3fe43c11c31f1f352 et reactor 38649222… ; aucune opération Git effectuée par l’auteur.

## Dépendances et autorités

Les sept comparaisons M/MD normalisées Native passent. Tool/Submit restent faux en comparaison littérale, et l’identité brute MD n’est pas récupérée pour Import/Tool/Submit. ARM passe les six comparaisons après remappages explicitement retenus ; Tool reste faux en comparaison littérale et normalisée sans remappage. AMD passe les cinq comparaisons littérales/normalisées/remappées. Aucune de ces distinctions n’est transformée en égalité brute universelle.

Les clôtures [Native](evidence/native01/final-fence-public.json), [ARM](evidence/arm02/selected-host-final-fence-public.json) et [AMD](evidence/amd01/originals-final-fence-public.json) passent sans refresh des anciennes autorités ; les deux dernières tiennent respectivement 1227 et 45 entrées hôte sélectionnées. Les workers Linux conservent leurs entrées locales neuf champs/SHA ; ils ne prétendent pas récupérer les anciennes identités hôte, guest SDK ou binds Mac d’autres conteneurs. L’autorité originale hôte du Core est jointe séparément à sa provenance producteur guest stat/SHA, distincte de FD. Les descripteurs restent privés : seuls leurs hashes/tailles déjà publics figurent ici, jamais leur contenu.

Les [cleanups ARM](evidence/arm02/cleanup-public.json) et [AMD](evidence/amd01/cleanup-public.json) confirment les conteneurs possédés absents et les sorties conservées. ARM ne supprime aucun ancien volume et ne construit aucune image ; AMD ne revendique aucun nouveau build d’image. Root rapporte le worker ARM terminal 0 en 381,514 s. Sa sonde `NoSuchContainer` après clôture est cohérente avec le cleanup, sans redémarrage.

## Refus préparatoire et limites

[ARM01](evidence/arm01/failure-public.json) reste FAILED : mauvais chemin client Docker fourni par Root, refus au garde avant toute commande Docker, zéro CXX/lien. Ce n’est pas un échec d’image. Sa [clôture 32 entrées](evidence/arm01/selected-host-final-fence-public.json) passe en conservant le premier refus ; les propositions documentaires V1/V2 restent intactes.

Le bilan est 18 consommateurs sélectionnés, 11 ldd Linux et 21 chemins CLI bornés, avec réemploi d’objets authentifiés ; aucune DB ouverte sous Linux, aucun import/bootstrap/export réel. La fermeture récursive/lazy symbols, le SDK et la configuration CMake globaux ne sont pas requalifiés. L’index 24, les trois P2P et les admissions globales ne sont pas rejoints/réémis par ce rapport. Les images candidates font l’objet d’un lot séparé : les reliages ne prouvent pas leur fabrication.

Aucun runtime, réseau ou duty démarré ; aucune nouvelle signature opérationnelle. Tentative 03 autorisée mais inactive ; reprise/finalité commune et production non qualifiées. Copies exactes : [MANIFEST.json](MANIFEST.json). Plan d’ajout aux quatre corps historiques intacts : [DOC-APPEND.md](DOC-APPEND.md).
