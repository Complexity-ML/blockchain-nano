import hashlib,json,shlex,subprocess,sys,time
from pathlib import Path
control=Path(__file__).resolve().parent;target='ubuntu@145.239.199.166'
state=control/'pipeline-state.json';reference='/Users/boris/Dev/blockchain-nano/docs/qualification/snapshot-account-view-20261002/source-manifest.json.gz'
arm=Path('/tmp/nano-full-coherent-0f53b42d-arm64-reports-20261002')
remote_full='/home/ubuntu/nano-full-coherent-0f53b42d-amd64-reports-20261002'
remote_offline='/home/ubuntu/nano-full-coherent-0f53b42d-amd64-offline-reports-20261002'
full_host=Path('/tmp/nano-full-coherent-0f53b42d-amd64-host-reports-20261002')
offline_host=Path('/tmp/nano-full-coherent-0f53b42d-amd64-offline-host-reports-20261002')
controllers={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in control.iterdir() if p.is_file() and p.suffix in ['.py','.mjs']}
def verify_controls():assert controllers=={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in control.iterdir() if p.is_file() and p.suffix in ['.py','.mjs']},'Controller changed while waiting'
def save(value):state.write_text(json.dumps(value,sort_keys=True,indent=2)+'\n')
def remote(args):
 r=subprocess.run(['ssh','-o','BatchMode=yes',target,shlex.join(args)],capture_output=True,text=True,timeout=45)
 if r.returncode:raise RuntimeError('SSH observation failed: '+r.stderr[:300])
 return r.stdout

def observe(path,container,remote_host=False):
 script="""import json,subprocess,sys;from pathlib import Path
p=Path(sys.argv[1]);name=sys.argv[2]
args=(['sudo','-n'] if sys.argv[3]=='remote' else [])+['docker','inspect','--format','{{json .State}}',name]
r=subprocess.run(args,capture_output=True,text=True,timeout=30)
files={f:int((p/f).read_text().strip()) for f in ['build-exit.txt','ctest-exit.txt','container-exit.txt','source-compare-exit.txt','qualification-exit.txt'] if (p/f).is_file()}
print(json.dumps({'handleReturnCode':r.returncode,'containerState':json.loads(r.stdout) if r.returncode==0 else None,'stderr':r.stderr[:300] if r.returncode else '', 'terminalFiles':files}))"""
 if remote_host:return json.loads(remote(['python3','-c',script,str(path),container,'remote']))
 r=subprocess.run([sys.executable,'-c',script,str(path),container,'local'],capture_output=True,text=True,timeout=45)
 if r.returncode:raise RuntimeError('Local observation failed: '+r.stderr[:300])
 return json.loads(r.stdout)
def terminal(observation):
 if observation['containerState'] is not None and observation['containerState']['Running']:return False
 return all(f in observation['terminalFiles'] for f in ['build-exit.txt','ctest-exit.txt','container-exit.txt','source-compare-exit.txt'])
def file_manifest(path,remote_host=False):
 script="import hashlib,json,sys;from pathlib import Path;p=Path(sys.argv[1]);print(json.dumps({str(f.relative_to(p)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(p.rglob('*')) if f.is_file()},sort_keys=True))"
 return json.loads(remote(['python3','-c',script,str(path)])) if remote_host else {str(f.relative_to(path)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(path.rglob('*')) if f.is_file()}
def acquire(remote_dir,destination):
 # A failed/partial prior copy is never overwritten or consumed as terminal.
 assert not destination.exists() and not Path(str(destination)+'.staging').exists(),'Preserve previous copy attempt'
 before=file_manifest(remote_dir,True);assert before
 stage=Path(str(destination)+'.staging')
 r=subprocess.run(['scp','-rq',target+':'+remote_dir,str(stage)],capture_output=True,text=True)
 (control/(destination.name+'-scp.log')).write_text(r.stdout+r.stderr)
 assert r.returncode==0,'SCP copy failed; retain staging for diagnosis'
 copied=file_manifest(stage);after=file_manifest(remote_dir,True)
 assert copied==before==after,'Terminal report copy changed or incomplete'
 stage.rename(destination)
 (destination/'terminal-copy-complete.txt').write_text('0\n')
 (control/(destination.name+'-copy-audit.json')).write_text(json.dumps({'files':len(before),'allFileHashesMatchBeforeAndAfter':True,'copiedOnlyAfterTerminal':True},indent=2)+'\n')
def audit_full(path,arch):
 verify_controls()
 r=subprocess.run([sys.executable,str(control/'full-audit.py'),str(path),reference],capture_output=True,text=True,timeout=45)
 (control/(arch+'-full-audit.stdout')).write_text(r.stdout);(control/(arch+'-full-audit.stderr')).write_text(r.stderr)
 (control/(arch+'-full-audit.exit')).write_text(str(r.returncode)+'\n')
 return r.returncode==0
# Confirm a single controller; immutable scripts are bound before waiting.
save({'phase':'started','pid':__import__('os').getpid(),'controllerHashes':controllers,'newWanNodesStarted':False})
qualified=set()
for _ in range(5760):
 verify_controls()
 try:
  a=observe(arm,'nano-full-coherent-0f53b42d-arm64')
  b=observe(remote_full,'nano-full-coherent-0f53b42d-amd64',True)
 except Exception as error:
  save({'phase':'observation-unresolved','reason':str(error),'newWanNodesStarted':False});time.sleep(15);continue
 save({'phase':'waiting-existing-campaigns','arm':a,'amd':b,'observedAt':time.time(),'qualifiedArchitectures':sorted(qualified),'newWanNodesStarted':False})
 for arch,obs in [('arm64',a),('amd64',b)]:
  if arch in qualified or not terminal(obs):continue
  if arch=='amd64':acquire(remote_full,full_host)
  ok=audit_full(arm if arch=='arm64' else full_host,arch)
  if not ok:
   save({'phase':'full-audit-refused','architecture':arch,'newWanNodesStarted':False});raise SystemExit(2)
  qualified.add(arch)
 if len(qualified)!=2:time.sleep(15);continue
 offline_state=json.loads(remote(['cat','/home/ubuntu/nano-full-0f53b42d-amd-offline-control-20261002/queue-state.json']))
 if offline_state.get('phase') in ['full-gate-refused','audit-terminal','observation-deadline'] and offline_state.get('exit',0)!=0:
  save({'phase':'signed-offline-refused','queue':offline_state,'newWanNodesStarted':False});raise SystemExit(2)
 if offline_state.get('phase')!='audit-terminal':time.sleep(15);continue
 assert offline_state['exit']==0
 acquire(remote_offline,offline_host)
 verify_controls()
 # launch.py re-executes all audits/EEST guards and attests actual binaries before fixtures/nodes.
 save({'phase':'invoking-gated-restore-launcher','newWanNodesStarted':False,'controlsUnchanged':True})
 with (control/'restore-launcher.log').open('w') as log:r=subprocess.run([sys.executable,str(control/'launch.py')],stdout=log,stderr=subprocess.STDOUT)
 launcher_state=json.loads((control/'launch-state.json').read_text()) if (control/'launch-state.json').is_file() else None
 save({'phase':'launcher-terminal','exit':r.returncode,'launcherInvoked':True,'launcherState':launcher_state,'newWanNodesStartedProvenByThisController':False})
 raise SystemExit(r.returncode)
save({'phase':'observation-deadline','newWanNodesStarted':False});raise SystemExit(124)
