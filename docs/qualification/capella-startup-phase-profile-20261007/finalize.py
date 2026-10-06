from pathlib import Path
import json,hashlib,shutil
repo=Path('/Users/boris/Dev/blockchain-nano');root=Path(__file__).parent;proof=repo/'docs/qualification/capella-startup-phase-profile-20261007';proof.mkdir()
def read(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(p,x):p.write_text(json.dumps(x,indent=2)+'\n')
for kind in ['refusal','v2']:
 x=read(root/kind/'result.json');assert x['sourceHashesUnchanged'] and x['sourceFiles']==2936 and x['usesIndependentCopy'];shutil.copytree(root/kind,proof/kind)
assert read(root/'refusal/state.json')['ExitCode']==1
assert read(root/'v2/state.json')['ExitCode']==0 and not read(root/'v2/state.json')['OOMKilled'];assert read(root/'v2/result.json')['runtimeReadyWithin180Seconds']
rows=[];stop=None;ipc=False
for l in (root/'v2/runtime.log').read_text().splitlines():
 try:x=json.loads(l)
 except json.JSONDecodeError:continue
 if x.get('event')=='capella-startup-stage':rows.append(x)
 if x.get('event')=='capella-runtime-stopped':stop=x
 if x.get('event')=='altair-ipc-ready':ipc=True
assert ipc and stop and stop['clockHealthy'] and stop['validator']['localKeys']==33 and stop['validator']['proposalsAdmitted']==0 and stop['validator']['attestationsAdmitted']==0
save(proof/'summary.json',{'runtimeReadyMs':next(x['elapsedMs'] for x in rows if x['stage']=='runtime-ready'),'previousStageDurationsMs':{x['previousStage']:x['previousElapsedMs'] for x in rows if x['previousStage']},'ipcObserved':True,'nodeExitCode':0,'stopReceiptObserved':True,'dutiesAdmitted':0,'sourceFiles':2936,'sourceHashesUnchanged':True,'network':'none','networkQualified':False,'productionQualified':False})
for n in ['run-vps.py','run-vps-v2.py','fetch-public.py','original.log','v2-original.log','original-handles.json','finalize.py']:shutil.copy2(root/n,proof/n)
(proof/'README.md').write_text('''# Isolated VPS startup profile — 7 October 2026

Two complete independent host-local copies of the2936-file V33 audited source, with all journals/witnesses and independent inodes. Source hashes reverified before/after every diagnostic, no source writer. Existing AMD64 image/build volume, diagnostic core b6413091 hash4c6c68ef29de669c7704f854e6b463b9e7961eaff2f8449bd350ab8409bfd965. CPU2/memory4GiB/nofile65536, no published ports and Docker network none. Original clocks/consensus/cache/catalogue limits unchanged. Copies/config/env/hash dictionaries stay privately on VPS; only selected public metadata/logs exported.

First diagnostic86734 orchestration exit0 but node exit1: exact recovery commitment guard refused the stale pre-V33 token. This is a failure, not a passing recovery. Its copy/log/result retained. Second49180 exit0 uses a fresh copy and the exact V33 audited current core commitment; only that configuration field updated, with capsule SHA compared to committed proof. No guard relaxed and no binary recompile/restart of the same process.

Second diagnostic runtime-ready138973ms, IPC observed, clean node exit0,33 healthy keys, zero admitted duties and no advancing finality. Stage durations: reactor recovery83990ms, checkpoint migration/setup32005ms, signing journals464ms, clock catch-up21392ms, initial clock poll1095ms. Full source2936 files remained unchanged after stop. Runtime/core clock publications on the disposable copy are not adopted as canonical network state.

This is a single isolated profile, not a paired speed benchmark, WAN run or production qualification. Differences from V33 include the current audited publication, warm caches, diagnostic tracing and network isolation. V33 remains FAILED. Measurements locate recovery/migration as dominant cost; they do not prove the root cause of V33's specific timeout or explain WAN A. Next optimize authenticated cold-checkpoint reuse while preserving validation of both primary and independent witness bytes, corruption rejection, cancellation and exact signing protections; then rerun affected recovery tests and the three-node common-finality campaign. No deadline increase, funds, sale or public deployment.
''')
save(proof/'manifest.json',{'sourceRevision':'b6413091fadbdd6589ef58f104562883cd0fe7cf','sourceAudit':'v33-post-stop-readonly-20261007','nodeProgramsUnchanged':True,'networkQualified':False,'productionQualified':False,'files':{str(p.relative_to(proof)):sha(p) for p in sorted(proof.rglob('*')) if p.is_file()}})
msg='Profilage VPS sur copies isolées terminé : refus initial86734 (orchestrateur0/nœud1) conservé pour engagement périmé ; seconde copie avec engagement exact de l’audit49180/0, runtime prêt138973ms, IPC observé et arrêt0,33 clés saines mais zéro duty/finalité nouvelle. Restauration83990ms, migration/configuration32005ms, journaux464ms, catch-up21392ms, premier poll1095ms. Source2936 fichiers strictement inchangée. [Mesures et différences avec V33](qualification/capella-startup-phase-profile-20261007/README.md). Coût dominant localisé ; optimisation des revalidations authentifiées encore à implémenter/qualifier. Mesure isolée unique, aucune conclusion de performance WAN ou production ; V33 FAILED et tous critères restants conservés.'
for n in ['PRODUCTION-GOAL.md','VALIDATOR-EIP-ROADMAP.md','EVM-PRODUCTION-EIP-ROADMAP.md','EIP-IMPLEMENTATION-STATUS.md']:
 p=repo/'docs'/n;s=p.read_text();needle='## État courant vérifié — 7 octobre 2026\n\n';assert needle in s;p.write_text(s.replace(needle,needle+msg+'\n\nÉtat précédent conservé : ',1))
print('Isolated profile archived with initial refusal; source untouched; production gates unchanged')
