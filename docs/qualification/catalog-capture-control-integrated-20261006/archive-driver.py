from pathlib import Path
import json,hashlib,shutil,subprocess
r=Path('/Users/boris/Dev/blockchain-nano');p=Path('/private/tmp/nano-catalog-capture-unlocked-docker-20261006');n=Path('/private/tmp/nano-catalog-capture-coherent-native-20261006')
assert json.loads((p/'arm64-repair-terminal-exit.json').read_text())['exitCode']==0
assert json.loads((p/'amd64-repair-terminal-exit.json').read_text())['exitCode']==0
m=json.loads((p/'source-manifest.json').read_text());assert all(hashlib.sha256((r/f).read_bytes()).hexdigest()==h for f,h in m.items())
for arch in ['arm64','amd64']:
 q=p/(arch+'-proof');assert set((q/'rebuilt-sources.txt').read_text().split())=={'rpc.cpp','consensus_capella_runtime.cpp'}
 for stage in ['controller','runtime-controller','regression-controller']:assert (q/(stage+'-exit.txt')).read_text().strip()=='0'
 for mode in ['catalog-concurrency','configured-runtime','automatic-recovery','http','capture-control']:assert (q/('test-'+mode+'-exit.txt')).read_text().strip()=='0'
for mode in ['catalog-concurrency','configured-runtime','automatic-recovery','http','capture-control']:assert json.loads((n/('test-'+mode+'-exit.json')).read_text())['exitCode']==0
for q in [p/'arm64-proof',p/'amd64-proof',n]:
 rows=[json.loads(l) for l in (q/'test-capture-control.log').read_text().splitlines() if l.startswith('{')];cases=[v for v in rows if v.get('scenario')=='catalog-capture-outside-control-lock'];assert len(cases)==1 and len(cases[0]['cases'])==4
 assert all(v['generationResponsive'] and v['controlResponsive'] and v['lateCaptureRefused'] and v['primaryUnchanged'] for v in cases[0]['cases'])
a=r/'docs/qualification/catalog-capture-control-integrated-20261006';a.mkdir(exist_ok=False)
for arch in ['arm64','amd64']:shutil.copytree(p/(arch+'-proof'),a/arch)
(a/'native').mkdir()
for f in n.iterdir():
 if f.is_file() and f.suffix in ['.json','.log','.d']:shutil.copy2(f,a/'native'/f.name)
for f in p.iterdir():
 if f.is_file() and f.suffix in ['.json','.log','.sh']:shutil.copy2(f,a/f.name)
shutil.copy2('/private/tmp/nano-build-catalog-capture-coherent-native-20261006.py',a/'native-driver.py')
(a/'implementation.patch').write_text(subprocess.check_output(['git','diff','--','network/capella_inventory_sessions.hpp','tests/test_capella_transition.cpp','CMakeLists.txt'],cwd=r,text=True))
(a/'README.md').write_text('''# Capture catalogue hors verrou de contrôle — qualification intégrée

Parent main 000c9c85, cœur parent d285db64. 349 sources figées et vérifiées avant/après sur Docker ARM64 et AMD64, puis comparées au worktree. Scan des dépendances : seuls rpc.cpp et consensus_capella_runtime.cpp reconstruits, autres objets réutilisés du cœur qualifié inchangé. Les runtimes nano, lecteurs et fixtures sont liés avec le nouvel archive. Images/volumes existants ; aucune image reconstruite. Aucun nœud de campagne ni stockage de campagne ouvert.

La capture est sérialisée par un verrou distinct. Le registre reste disponible pour génération/release/stop pendant la capture et son attente du réacteur. La capture réserve son budget restant jusqu’à son drainage ; la publication revérifie arrêt, génération, retraite de requête et lease. Les quotas restent inchangés. Une capture échouée détruit son snapshot avant de libérer la réservation.

Originaux ARM64 1980, AMD64 4618 et natif cohérent 65381 : sorties 0 vérifiées. Cinq modes concernés passent sur les trois : concurrence catalogue (32 travailleurs, quotas, doublons et 128 courses release/acquire), runtime configuré, reprise automatique, HTTP, nouveau contrôle de capture. Ce dernier vérifie séparément retrait de requête, arrêt, expiration et annulation : contrôles réactifs pendant la capture retenue, publication tardive refusée, primaire inchangé ; capacité récupérée sauf arrêt permanent.

Premiers pilotes Docker : erreur de montage, sorties 125, avant compilation ; commandes/logs/reçus conservés. Réparation uniquement du chemin de montage après terminal original, mêmes sources/images/volumes. Les pilotes natifs précédents liés au cœur parent ne sont pas utilisés comme qualification du runtime cohérent.

Ce lot ne prouve pas la cause WAN et ne qualifie aucune finalité distribuée. La file HTTP commune peut encore retenir ses deux travailleurs sur une capture et un acquéreur en attente : sa contention et la propagation des votes récents restent ouvertes avant une nouvelle campagne WAN. EVM/forks, autres critères de production, bridge, lancement et revue indépendante restent ouverts. Pas de fonds réels ni de vente.
''')
manifest={str(f.relative_to(a)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(a.rglob('*')) if f.is_file()};(a/'manifest.json').write_text(json.dumps(manifest,indent=2))
for doc in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 with (r/'docs'/doc).open('a') as f:f.write('\n\n2026-10-06 — Capture catalogue hors verrou de contrôle : lot intégré natif/ARM64/AMD64, originaux 65381/1980/4618 sorties 0 ; 349 sources figées, deux consommateurs ABI reconstruits, cinq modes concernés réussis. Quatre scénarios neufs (release/stop/expiry/cancel), quotas et courses conservés. Échecs initiaux de montage conservés. Pas de finalité WAN démontrée ; contention de la file HTTP et votes récents encore ouverts avant nouvelle campagne. [Preuves](qualification/catalog-capture-control-integrated-20261006/README.md).\n')
print('Archive verified',len(manifest),'files')
