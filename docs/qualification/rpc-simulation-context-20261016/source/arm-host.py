#!/usr/bin/env python3
"""Root owned, single bounded RPC-context batch; no consensus duties or public network."""
import hashlib,json,os,pathlib,shlex,stat,subprocess,sys,time,uuid
E=pathlib.Path('/Users/boris/Documents/Codex/2026-09-20/users-boris-dev-blockchain-python/qualification-resume-20261012')
R=pathlib.Path('/Users/boris/Dev/blockchain-nano')
W=E/'rpc-simulation-context-root-source-20261016'
IMAGE='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb'
D=['/Applications/Docker.app/Contents/Resources/bin/docker','--config',str(E/'fresh-three-owner-current-20261016-02/docker-client'),'--host','unix:///Users/boris/.docker/run/docker.sock']
F=('st_dev','st_ino','st_mode','st_nlink','st_uid','st_gid','st_size','st_mtime_ns','st_ctime_ns')
O=pathlib.Path(sys.argv[1]);assert O.parent==E and not O.exists() and O.name.startswith('rpc-simulation-context-arm-root-qualification-')
os.umask(0o077);O.mkdir(mode=0o700)
def save(name,z): (O/name).write_text(json.dumps(z,sort_keys=True,indent=2)+'\n')
def raw(path):
 p=pathlib.Path(path);assert p.is_absolute() and p.resolve()==p
 with p.open('rb') as f:
  a=os.fstat(f.fileno());assert stat.S_ISREG(a.st_mode) and a.st_nlink==1 and a.st_size<=150000000
  b=f.read();assert len(b)==a.st_size
  row={k:getattr(a,k) for k in F};assert row=={k:getattr(os.fstat(f.fileno()),k) for k in F}=={k:getattr(p.lstat(),k) for k in F}
 return dict(row,sha256=hashlib.sha256(b).hexdigest()),b
held={}
def hold(p,old=None):
 row,b=raw(p)
 if old is not None:assert row==old,(str(p),'old authority changed')
 if str(p) in held:assert held[str(p)]==row,(str(p),'selected input changed')
 held[str(p)]=row;return b
def doc(p):return json.loads(hold(p))
def run(args,leaf,timeout=60):
 save(leaf+'-argv.private.json',args);t=time.monotonic()
 with (O/(leaf+'.log')).open('xb') as f:p=subprocess.run(args,stdout=f,stderr=subprocess.STDOUT,timeout=timeout)
 save(leaf+'-exit.json',dict(exitCode=p.returncode,elapsedMs=round((time.monotonic()-t)*1000)))
 assert p.returncode==0,leaf+' failed; original log retained'
 return (O/(leaf+'.log')).read_bytes()
source=O/'source';source.mkdir();results=O/'results';results.mkdir();controls=O/'controls';controls.mkdir()
def copy(p,rel):
 b=hold(p);q=source/rel;q.parent.mkdir(parents=True,exist_ok=True);assert not q.exists();q.write_bytes(b);q.chmod(0o444);return q
head=subprocess.check_output(['/usr/bin/git','-C',str(R),'rev-parse','HEAD']).decode().strip()
assert subprocess.check_output(['/usr/bin/git','-C',str(R),'status','--porcelain'])==b''
assert head=='cc51bc0ce37b36724f11a9e5fc1c3504056a6f74'
assert subprocess.check_output(['/usr/bin/git','-C',str(R),'diff','--name-only','68d64a5330d16c971dd603b56e0f14c88c1459c8','HEAD','--','include','src','third_party','network'])==b''
for prefix in ('include/','third_party/','network/','tests/support/'):
 for rel in subprocess.check_output(['/usr/bin/git','-C',str(R),'ls-files','-z','--',prefix]).decode().split('\0'):
  if rel and pathlib.Path(rel).suffix in ('.hpp','.h','.inl','.tcc'):
   assert hold(R/rel)==subprocess.check_output(['/usr/bin/git','-C',str(R),'show','HEAD:'+rel]);copy(R/rel,rel)
for rel in ('tests/test_rpc_access_list.cpp','tests/test_rpc_simulation_block_id.cpp','tests/fixtures/python-rpc.json'):copy(R/rel,rel)
def proposed(directory,pin,selected):
 root=E/directory;b=hold(root/'manifest.json');assert hashlib.sha256(b).hexdigest()==pin
 mf=json.loads(b)
 for row in mf['payloads']:
  b=hold(root/row['path']);assert len(b)==row['bytes'] and hashlib.sha256(b).hexdigest()==row['sha256']
  if row['path'] in selected:copy(root/row['path'],selected[row['path']])
proposed('rpc-simulation-context-source-proposal-20261016','eb8b89887291de0e1fabe97bc08c5bd4e66469f62dbe2d6a461d60090761b2ce',{'src/rpc.cpp':'src/rpc.cpp','tests/test_rpc.cpp':'tests/test_rpc.cpp'})
proposed('rpc-simulation-context-tests-proposal-v2-20261016','a6660516daebb9eb8a2382174b82fb16822e59121a583db6e31f0a61d9797d7b',{'source/tests/test_rpc_simulation_context.cpp':'tests/test_rpc_simulation_context.cpp'})
proposed('rpc-simulation-context-ci-proposal-attempt02-20261016','d16cb016c7b188ef8ff1560d9a5017ef2985b16a216f1f493c81c0d1e616f22d',{'source/'+r:r for r in ('CMakeLists.txt','scripts/ci/core.sh','scripts/ci/check-core.cmake','scripts/ci/check-core.test.mjs')})
# Keep previously admitted old Core and six archive/object rows exactly.
core=E/'current-arm-runtime-relink-root-20261016-01/runtime-programs/libnano_core.a'
old=doc(E/'current-arm-runtime-relink-root-20261016-01/runtime-bundle-first-admission.private.json')['observations'][-1]['table'][str(core)]
hold(core,old);assert old['sha256']=='35779b74ff3894d8857d06732caa801cb340381f4cf6dc5801074c7e372dbdf6'
retained={'/baseline/libnano_core.a':dict(sha256=old['sha256'],bytes=old['st_size'])}
mounts=['--mount',f'type=bind,src={core},dst=/baseline/libnano_core.a,readonly']
hb=doc(E/'current-arm-helpers-root-attempt02-20261016/outputs/software-admission.private.json')
closure=doc(E/'fresh-arm-bootstrap-import-current-core-root-20261016/selected-original-closure.private.json')
for guest,v in hb['guestMaterials'].items():
 if guest.startswith('/old/'):
  matches=[(p,z) for p,z in closure['files'].items() if z.get('sha256')==v['sha256'] and z.get('st_size')==v['bytes']];assert len(matches)==1
  p,old=matches[0];hold(p,old);mounts+=['--mount',f'type=bind,src={p},dst={guest},readonly'];retained[guest]=dict(sha256=v['sha256'],bytes=v['bytes'])
assert len(retained)==7
prior=E/'rpc-simulation-context-arm-root-qualification-20261016-04/results/causal-baseline.json'
b=hold(prior);z=json.loads(b);assert z['expectedCausalFailures']==4
for g,z in zip(('fees','balances','nonces','bounds'),z['newDisjointGroups']):
 assert z['group']==g and z['error']['selectedGroup']==g and z['error']['stage']==g and not z['error']['passed'] and len(z['error']['caseRecords'])==1
retained['/inputs/causal-baseline.json']=dict(sha256=hashlib.sha256(b).hexdigest(),bytes=len(b))
mounts+=['--mount',f'type=bind,src={prior},dst=/inputs/causal-baseline.json,readonly']
# Already assembled candidate Core, linked and partially exercised in08;
# its source/member/hash receipt is retained, no Core object rebuilt here.
prior=E/'rpc-simulation-context-arm-root-qualification-20261016-08/results'
continuity=doc(prior/'core-member-continuity.json');assert continuity['passed'] and continuity['unchangedMembers']==142
candidate=prior/'libnano_core.a';b=hold(candidate);pin=dict(sha256=hashlib.sha256(b).hexdigest(),bytes=len(b));assert pin['sha256']==continuity['newCore']['sha256']=='43b6b8ee60f7b614117dc46c79f80eb3cdf930202d2d32c35bc24983aa8f49f5' and pin['bytes']==41040028
retained['/inputs/candidateCore.a']=pin;mounts+=['--mount',f'type=bind,src={candidate},dst=/inputs/candidateCore.a,readonly'];save('candidate-core-first-host-admission.private.json',{str(candidate):held[str(candidate)]})
recipe=doc(E/'current-arm-helpers-root-attempt02-20261016/controls/recipes.json')['programs'][1]
recipe=dict(format='nano-rpc-simulation-context-recipe-v1',uid=501,gid=20,architecture='linux-arm64',image=IMAGE,elfMachine=183,compileFlags=recipe['compileFlags'],coreFlags=recipe['compileFlags']+['-Wall','-Wextra','-Wpedantic'],linkPrefix=recipe['linkPrefix'],linkTail=recipe['linkTail'],retainedPins=retained,baselineEvidence='/inputs/causal-baseline.json',candidateCore='/inputs/candidateCore.a',regressions=[dict(role='nano-rpc-tests',source='/work/tests/test_rpc.cpp',arguments=[]),dict(role='nano-rpc-access-list-tests',source='/work/tests/test_rpc_access_list.cpp',arguments=['--self-test']),dict(role='nano-rpc-simulation-block-id-tests',source='/work/tests/test_rpc_simulation_block_id.cpp',arguments=['--eip1898-self-test'])])
(controls/'recipe.json').write_text(json.dumps(recipe,sort_keys=True)+'\n')
expected={'/work/'+str(p.relative_to(source)):dict(sha256=raw(p)[0]['sha256'],bytes=p.stat().st_size) for p in source.rglob('*') if p.is_file()}
(controls/'expected.json').write_text(json.dumps(expected,sort_keys=True)+'\n')
(controls/'worker.pl').write_bytes(hold(W/'worker.pl'))
for p in controls.iterdir():p.chmod(0o444)
new_paths=[p for root in (source,controls) for p in root.rglob('*') if p.is_file()]
observations=[];previous=None;stable=0;since=time.monotonic();start=since
while True:
 table={str(p):raw(p)[0] for p in new_paths};now=time.monotonic();assert now-start<30
 observations.append(dict(elapsedMs=round((now-start)*1000),table=table))
 if table==previous:stable+=1
 else:stable=1;since=now
 if stable>=3 and now-since>=2:break
 previous=table;time.sleep(1)
held.update(table);save('new-source-first-admission.private.json',dict(observations=observations,oldAuthoritiesRefreshed=False));save('host-inputs-before.private.json',held)
name='nano-rpc-context-'+uuid.uuid4().hex
save('container-owner.private.json',dict(name=name,image=IMAGE,label='nano.rpc.context',owner=name))
actual=run(D+['image','inspect',IMAGE,'--format','{{.Id}} {{.Architecture}}'],'image-inspect').decode().strip();assert actual==IMAGE+' arm64'
first=None;cleanup_errors=[]
try:
 args=D+['run','--rm','--pull=never','--name',name,'--label','nano.rpc.context='+name,'--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--user','501:20','--cpus','2','--memory','3g','--pids-limit','128','--tmpfs','/tmp:rw,nosuid,nodev,size=256m','--mount',f'type=bind,src={source},dst=/work,readonly','--mount',f'type=bind,src={controls},dst=/controls,readonly','--mount',f'type=bind,src={results},dst=/outputs',*mounts,'--workdir','/outputs','--entrypoint','/usr/bin/perl',IMAGE,'/controls/worker.pl','--rpc-simulation-context-qualification']
 run(args,'batch',1200)
except BaseException as e:first=e
finally:
 try:
  inspect=subprocess.run(D+['inspect',name],capture_output=True,timeout=30)
  (O/'cleanup-inspect.stdout').write_bytes(inspect.stdout);(O/'cleanup-inspect.stderr').write_bytes(inspect.stderr)
  if inspect.returncode==0:
   z=json.loads(inspect.stdout);assert len(z)==1 and z[0]['Name']=='/'+name and z[0]['Image']==IMAGE and z[0]['Config']['Labels']['nano.rpc.context']==name
   identifier=z[0]['Id'];assert len(identifier)==64
   subprocess.run(D+['stop','--time','5',identifier],check=True,capture_output=True,timeout=30)
   after=subprocess.run(D+['inspect',identifier],capture_output=True,timeout=30);assert after.returncode==1 and b'no such object' in after.stderr.lower()
  else:assert inspect.returncode==1 and b'no such object' in inspect.stderr.lower()
 except BaseException as e:cleanup_errors.append(str(e));first=first or e
 changed=[]
 for p,old in held.items():
  try:
   if raw(p)[0]!=old:changed.append(p)
  except BaseException as e:changed.append(p);first=first or e
 save('host-inputs-final-fence.json',dict(passed=not changed,selectedFiles=len(held),changed=changed,oldAuthoritiesRefreshed=False))
 save('host-terminal.json',dict(passed=first is None and not changed,error=str(first) if first else None,cleanupErrors=cleanup_errors,productionQualified=False,imageBuilds=0,consensusDutiesStarted=0,publicNetwork=False,head=head))
if first:raise first
assert not changed
