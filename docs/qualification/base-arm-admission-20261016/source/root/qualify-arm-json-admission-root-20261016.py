#!/usr/bin/env python3
"""Metadata-only qualification; never import or run a historical controller."""
import ast, hashlib, json, os, pathlib, re, stat, subprocess, threading, time, types
E=pathlib.Path(__file__).resolve().parent
O=E/'arm-json-admission-qualification-root-20261016-01'
S=E/'current-arm-runtime-json-admission-fix-source-20261016'
B=E/'current-arm-runtime-relink-source-20261016-v2/host.py'
def pin(b):return {'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b)}
assert pin(B.read_bytes())=={'sha256':'4ca2509093abcb2106cac8ad900dbcb011b0a64584b02b56c8117246ee4e60c8','bytes':33326}
mf=json.loads((S/'manifest.json').read_bytes())
for z in mf['payloads']:assert pin((S/z['path']).read_bytes())=={k:z[k] for k in ('sha256','bytes')}
O.mkdir(mode=0o700)
fixed=O/'host.py';fixed.write_bytes(B.read_bytes())
q=subprocess.run(['/usr/bin/patch','--batch','-p1','-i',str(S/'host-v2-first-admission.patch')],cwd=O,capture_output=True)
(O/'patch-stdout.log').write_bytes(q.stdout);(O/'patch-stderr.log').write_bytes(q.stderr)
assert q.returncode==0 and pin(fixed.read_bytes())=={k:mf['patchedSource'][k] for k in ('sha256','bytes')}
os.chmod(fixed,0o400)
tree=ast.parse(fixed.read_bytes())
names={'pin','material','js','save','stable','host_fence'}
defs=[n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name in names]
assert {n.name for n in defs}==names
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
m=types.SimpleNamespace(F=F,HELD={})
ns=dict(hashlib=hashlib,json=json,os=os,pathlib=pathlib,re=re,stat=stat,time=time,m=m,O=O,CREATED=True)
exec(compile(ast.Module(body=defs,type_ignores=[]),str(fixed),'exec'),ns)
legacy=E/'qualify-complete-nano-arm-root-attempt02-20261016.py'
assert pin(legacy.read_bytes())=={'sha256':'a73f0267359a4ac155b5a63a005385930b558a3408343622e8909931ffe92442','bytes':14095}
old=ast.parse(legacy.read_bytes())
olddefs=[n for n in old.body if isinstance(n,ast.FunctionDef) and n.name in {'hold','save'}]
oldns=dict(pathlib=pathlib,json=json,material=ns['material'],HELD=m.HELD)
exec(compile(ast.Module(body=olddefs,type_ignores=[]),str(legacy),'exec'),oldns)
m.hold=oldns['hold'];m.save=oldns['save']
def put(name,b):
 p=O/name
 with p.open('xb') as f:f.write(b);f.flush();os.fsync(f.fileno())
 return p
checks=[]
# Deterministic causal baseline: a metadata-only change after eager first hold.
eager=put('eager.json',b'{"publicSynthetic":true}\n')
m.hold(eager);eager_before=dict(m.HELD[str(eager)])
os.chmod(eager,0o400)
try:ns['host_fence']('before-worker')
except AssertionError:pass
else:raise AssertionError('eager admission unexpectedly accepted changed metadata')
diag=json.loads((O/'fence-metadata-before-worker.private.json').read_bytes())
assert diag['path']==str(eager) and diag['expectedNine']=={k:eager_before[k] for k in F}
assert diag['diagnosticContentReads']==0 and diag['oldAuthoritiesRefreshed'] is False
assert diag['fdPathNineEqual'] and 'st_mode' in diag['metadataFieldsDifferent']
assert m.HELD[str(eager)]==eager_before
checks.append('eager_hold_refuses_metadata_change_and_records_first_failure')
# New namespace only: delayed metadata is permitted to settle BEFORE first hold.
m.HELD.clear()
paths=[put('settled-result.json',b'{"publicSyntheticResult":true}\n'),put('settled-binding.json',b'{"publicSyntheticBinding":true}\n')]
known={str(p):pin(p.read_bytes()) for p in paths}
def change_before_admission():
 time.sleep(.2)
 for p in paths:os.chmod(p,0o400)
t=threading.Thread(target=change_before_admission);t.start()
table=ns['stable'](paths,known,'new-worker-json-first-admission-test');t.join()
assert table==m.HELD
observations=json.loads((O/'new-worker-json-first-admission-test.private.json').read_bytes())['observations']
assert len(observations)>=4 and observations[0]['table']!=table
ns['host_fence']('after-cleanup')
assert all(ns['js'](p,known[str(p)]) for p in paths)
checks.append('both_new_known_byte_copies_stabilize_before_first_hold')
# Once admitted, mutations must NEVER refresh the authority.
saved={k:dict(v) for k,v in m.HELD.items()};os.chmod(paths[0],0o600)
try:ns['host_fence']('exit-finally')
except AssertionError:pass
else:raise AssertionError('post-admission mutation accepted')
assert m.HELD==saved
try:m.hold(paths[0])
except AssertionError:pass
else:raise AssertionError('held authority refreshed')
assert m.HELD==saved
checks.append('post_admission_mutation_refused_without_refresh')
bad=put('wrong-bytes.json',b'{"wrong":true}\n')
try:ns['stable']([bad],{str(bad):pin(b'{"expected":true}\n')},'wrong-bytes')
except AssertionError:pass
else:raise AssertionError('wrong known bytes accepted')
assert str(bad) not in m.HELD
checks.append('wrong_bytes_refused_before_admission')
# Diagnostic failures cannot mask the original gate exception.
original=RuntimeError('synthetic original material gate failure')
material0,save0=ns['material'],ns['save']
def refuse_material(_):raise original
def refuse_save(*_):raise OSError('synthetic diagnostic write refusal')
ns['material'],ns['save']=refuse_material,refuse_save
try:ns['host_fence']('exit-finally')
except RuntimeError as caught:assert caught is original
else:raise AssertionError('original exception lost')
finally:ns['material'],ns['save']=material0,save0
assert m.HELD==saved
checks.append('original_gate_exception_preserved_if_diagnostic_cannot_be_saved')
result=dict(format='nano-arm-json-admission-metadata-qualification-v1',passed=True,checks=checks,checkCount=len(checks),fixedController=pin(fixed.read_bytes()),sourceFunctionsExtractedOnly=True,metadataOnlySyntheticInputs=True,controllerMainExecuted=False,oldAuthoritiesRefreshed=False,originalHistoricalRowsRecovered=False,historicalFailureReclassified=False,runtimeDutiesResumed=False,productionQualified=False,cppCommands=0,dockerCommands=0,oldCxxTestsReplayed=False,stabilizationObservations=len(observations),stabilizationSpanMs=observations[-1]['elapsedMs'])
(O/'result-public.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
print(json.dumps(result,sort_keys=True))
