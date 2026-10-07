from pathlib import Path
import json,shutil,hashlib
r=Path(__file__).parent;src=r/'full-state-profile';repo=Path('/Users/boris/Dev/blockchain-nano');dest=repo/'docs/qualification/capella-startup-memo-full-state-profile-20261007';dest.mkdir(exist_ok=True);assert not list(dest.iterdir())
result=json.loads((src/'result.json').read_text());state=json.loads((src/'state.json').read_text());assert result['sourceHashesUnchanged'] and result['sourceFiles']==2936 and result['runtimeReadyWithin180Seconds'] and not result['failure'];assert state['ExitCode']==0 and not state['OOMKilled']
rows=[];stop=None;ipc=False
for l in (src/'runtime.log').read_text().splitlines():
 try:x=json.loads(l)
 except:continue
 if x.get('event')=='capella-startup-stage':rows.append(x)
 if x.get('event')=='capella-runtime-stopped':stop=x
 if x.get('event')=='altair-ipc-ready':ipc=True
assert stop and stop['clockHealthy'] and stop['validator']['healthy'] and stop['validator']['localKeys']==33
assert stop['validator']['attestationsAdmitted']==stop['validator']['proposalsAdmitted']==0
capsule=json.loads((repo/'docs/qualification/v33-post-stop-readonly-20261007/vps/capture.json').read_text())
def identity(k):return ('index',k['index']) if k['index'] is not None else ('key',k['publicKey'])
assert {identity(k):k['restart'] for k in stop['validatorKeys']}=={identity(k):k['restart'] for k in capsule['validatorKeys']}
for f in src.iterdir():shutil.copy2(f,dest/f.name)
for name in ['profile-vps.py','profile-vps-original.log','full-state-profile-handle.json','archive-profile.py','archive-profile-first-refusal.json']:shutil.copy2(r/name,dest/name)
summary={'revision':'c499649857b88a39df3d7badb97156daf96eb2c6','nanoSHA256':'e791a675bc5c369d79c56c45d8793041376f091958ffdc1f1b54225a616dd714','originalSession':95451,'exitCode':0,'terminalChunk':'dd44ea','runtimeReadyMs':next(x['elapsedMs'] for x in rows if x['stage']=='runtime-ready'),'stageDurationsMs':{x['previousStage']:x['previousElapsedMs'] for x in rows if x['previousStage']},'sourceFiles':2936,'sourceHashesUnchanged':True,'signerCommitmentsExactUnchanged':33,'ipcObserved':ipc,'networkQualified':False,'productionQualified':False}
(dest/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
(dest/'README.md').write_text('''# Full-state startup profile after certificate reuse — 7 October 2026

Original95451 exited0; isolated Docker node exited0, no OOM. Corrected incremental core c4996498, exact AMD64 nano hash in summary. Complete independent copy of the current V33 audited source:2936 files, all primary/witness/journal files, independent inodes and exact source hashes before/after. Source writer stopped137 remains untouched; disposable profile publications are not canonical adopted state. Keys/config/environment/file dictionaries stay private on VPS.

CPU2,4GiB,nofile65536, network none, original consensus/clock/cache/catalogue limits and180-second readiness deadline unchanged. Recovery commitment updated only from exact current audited capsule. Runtime ready35500ms: restore32883ms, migration911ms, signing472ms, catch-up1210ms, initial poll1ms. IPC readiness was not observed before this immediate stop; runtime-ready and JSON-RPC listener were logged.33 healthy keys, signer commitments exactly unchanged, zero admitted proposals/attestations and no new finality.

Previous b641 isolated profile was138973ms (restore83990ms,migration32005ms,catch-up21392ms). These are single runs, not repeated paired benchmarks: host load/caches/time-driven catch-up can differ. The new run proves complete-copy startup under the existing deadline in this scope, not a guaranteed speedup, V33 resolution, live duties, WAN finality or production readiness. Next use independently audited current source and unchanged budgets in the three-node campaign. All historical failures and remaining production/bridge/Doppler/independent-review requirements remain open.
''')
(dest/'manifest.json').write_text(json.dumps({'sourceRevision':summary['revision'],'networkQualified':False,'productionQualified':False,'files':{str(f.relative_to(dest)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(dest.rglob('*')) if f.is_file()}},indent=2)+'\n')
msg='Profil complet VPS après correction c4996498 : original95451 terminal0, nœud arrêté0 sansOOM, runtime prêt35500ms sur copie indépendante2936 fichiers ; source exacte inchangée et33 engagements de signature conservés. Restauration32883ms/migration911ms. [Preuves](qualification/capella-startup-memo-full-state-profile-20261007/README.md). Mesures uniques non appariées, zéro duty/finalité nouvelle ; pas de gain WAN garanti, V33 FAILED et autres critères/revue indépendante ouverts. Préparer maintenant la reprise des trois nœuds, mêmes limites et délais.'
for name in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/name;s=p.read_text();marker='## État courant vérifié — 7 octobre 2026\n\n';assert marker in s;p.write_text(s.replace(marker,marker+msg+'\n\nÉtat précédent conservé : ',1))
print('Full-state profile archived, all33 signer commitments exact, no WAN claim')
