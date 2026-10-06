from pathlib import Path
import hashlib,json,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');root=Path('/private/tmp/nano-head-fallback-20261006');replay=Path('/private/tmp/nano-head-fallback-real-replay-20261006');out=repo/'docs/qualification/head-catalog-fallback-integrated-20261006'
def read(p):return json.loads(p.read_text())
def copy_metadata(src,dst):
 dst.mkdir(exist_ok=False)
 for f in src.iterdir():
  if f.is_file() and f.suffix in ['.txt','.log','.json','.sha256','.cpp','.sh']:shutil.copy2(f,dst/f.name)
receipts=read(root/'terminal-exits.json');assert all(v['exitCode']==0 for v in receipts.values())
manifest=read(root/'source-manifest.json')
for f,h in manifest.items():assert hashlib.sha256((root/'network'/f).read_bytes()).hexdigest()==h;assert (repo/'network'/f).read_bytes()==(root/'network'/f).read_bytes(),f
cases=['capella_catalog_relay','capella_head_catalog_relay','capella_depth_catchup','capella_block_delivery','capella_attestation_delivery','capella_export_resume']
for case in cases:
 assert (root/('arm-'+case+'-exit.txt')).read_text().strip()=='0'
 assert (root/'amd-test-proof'/('amd-'+case+'-exit.txt')).read_text().strip()=='0'
 assert read(root/'native'/(case+'-run-exit.json'))['exitCode']==0
 arm=(root/('arm-'+case+'-run.log')).read_text().strip();assert arm==(root/'amd-test-proof'/('amd-'+case+'-run.log')).read_text().strip()==(root/'native'/(case+'-run.log')).read_text().strip(),case
for arch in ['arm64','amd64']:
 p=root/(arch+'-runtime-proof');assert (p/'controller-exit.txt').read_text().strip()=='0';assert (p/'usage-exit.txt').read_text().strip()=='1';assert 'Usage: nano-p2p' in (p/'usage.log').read_text()
assert read(root/'native/runtime-usage-exit.json')['exitCode']==1
r=read(replay/'summary.json');assert r['exitCode']==0 and r['attempts']==26 and all(f['admitted'] for f in r['frontiers']) and r['originalHashesUnchanged']
out.mkdir(exist_ok=False);shutil.copytree(root/'network',out/'network');copy_metadata(root/'native',out/'native');copy_metadata(root/'arm64-runtime-proof',out/'arm64-runtime');copy_metadata(root/'amd64-runtime-proof',out/'amd64-runtime');copy_metadata(root/'amd-test-proof',out/'amd64-tests');copy_metadata(replay,out/'real-archive-replay')
for f in root.iterdir():
 if f.is_file() and f.suffix in ['.txt','.log','.json','.sh']:shutil.copy2(f,out/f.name)
for name in ['nano-edit-relay-fallback-20261006.py','nano-add-head-history-regression-20261006.py','nano-build-head-fallback-native-20261006.py','nano-launch-head-fallback-amd-20261006.py','nano-collect-head-fallback-20261006.py','nano-collect-head-fallback-attempt-1-20261006.py','nano-head-fallback-arm-controller-20261006.log','nano-head-fallback-amd-controller-20261006.log','nano-head-fallback-native-controller-20261006.log','nano-head-fallback-arm-runtime-controller-20261006.log','nano-head-fallback-amd-runtime-controller-20261006.log','nano-head-fallback-real-replay-controller-20261006.log']:
 shutil.copy2(Path('/private/tmp')/name,out/name)
(out/'README.md').write_text('''# Repli borné tête → catalogue historique — qualification ciblée

Base f4c458cd ; cœur 5787915e inchangé. Deux en-têtes du relais et trois fichiers de tests modifiés, sources gelées vérifiées. Une tête non confirmée peut servir le catalogue dans le même délai et quantum. Un bloc définitivement refusé n’empêche plus le scan des entrées indépendantes ; il ne reçoit aucun ACK et l’échec reste visible. Un catalogue ne peut annoncer la réussite d’une tête absente de la connaissance ACK.

Mac natif, Docker ARM et Docker AMD : mêmes sorties des six unités pertinentes (46 scénarios catalogue, 19 priorité de tête, rattrapage/protections de refus, 28 livraison ancêtres, 14 dépendances de votes et reprise/cache). Annulations, réponses tardives/perdues, doublons, quotas et absence de faux ACK sont vérifiés dans leur périmètre modélisé. Les trois nouveaux exécutables nano-p2p sont compilés et leurs empreintes conservées ; invocation sans arguments : sortie 1 attendue et usage vérifié. Aucun réseau live qualifié par ce contrôle. Images/volumes et dépendances existants réutilisés ; aucun build d’image.

Rejeu comparatif : mêmes archives post-v13, même cœur, mêmes limites et copie chain locale neuve sans clés/journaux. Ancien relais (preuve v13-real-archive-relay-replay) : après 128 cycles, une frontière jamais envoyée, 81 publications. Relais corrigé : les deux frontières admises en 26 cycles, chacune après 22/23 tentatives ; 117 publications réelles, 208 racines ACK. Le dernier cycle reste FAILED à cause des autres refus ; aucun faux succès effacé. Bases originales en lecture seule, empreintes inchangées. Le transport/HTTP/tickets/pacing/concurrence live ne sont pas reproduits : ce résultat ne démontre pas la finalité WAN.

Erreur du collecteur initial : python3 absent de l’image, sortie 127 ; conservée. Collecte réparée avec tar existant, aucun test/build relancé pour cela. Échecs v13 et historiques conservés. Prochaine étape : reprise privée des trois propriétaires avec journaux/engagements post-v13 exacts et nouveaux binaires, trois nouvelles finalités communes en 900 s. Ressources, caches/pruning, WAN/partitions, rotations/cycle validateurs, EVM/profil actif, lancement/bridge, production et revue indépendante restent ouverts.
''')
note='\n\n2026-10-06 — Repli tête/catalogue corrigé et qualifié dans son périmètre : mêmes six unités sur Mac natif et Docker ARM/AMD, nouveaux binaires P2P identifiés. Catalogue borné accessible après tête non confirmée ; refus de bloc ne masque plus les entrées indépendantes, sans faux ACK ni succès de tête absente. Rejeu sur mêmes archives/cœur/limites : deux frontières admises en 26 cycles et 117 publications, contre une frontière jamais envoyée après 128 cycles avant correction. Originaux inchangés ; transport modélisé, résultat partiel reste FAILED sur autres refus. Finalité WAN/production non démontrées ; prochaine reprise sur engagements/journaux post-v13 exacts. Erreur initiale de collecte conservée et réparée sans relancer les builds/tests. [Preuves](qualification/head-catalog-fallback-integrated-20261006/README.md).\n'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;p.write_text(p.read_text()+note)
shutil.copy2('/private/tmp/nano-archive-head-fallback-20261006.py',out/'archive.py')
m={str(f.relative_to(out)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(out.rglob('*')) if f.is_file() and f.name!='manifest.json'};(out/'manifest.json').write_text(json.dumps(m,indent=2)+'\n');print('Verified and archived',len(m),'proof files; no databases/binaries archived')
