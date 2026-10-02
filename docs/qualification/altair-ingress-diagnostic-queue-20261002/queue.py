from pathlib import Path
import subprocess,json,time,hashlib,gzip,os
root=Path('/home/ubuntu/nano-altair-ingress-diagnostic-control-20261002');source=Path('/home/ubuntu/nano-altair-ingress-diagnostic-candidate-20261002');parent=Path('/home/ubuntu/nano-full-coherent-0f53b42d-amd64-reports-20261002');report=Path('/home/ubuntu/nano-altair-ingress-diagnostic-reports-20261002');revision='e8efb812af0e0274916cc2d4fecfc8dadf13e080'
control={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in root.iterdir() if p.suffix in ['.py','.sh']}
def state(phase,**kw):
 (root/'state.json').write_text(json.dumps({'phase':phase,'observedAt':time.time(),'pid':os.getpid(),**kw},indent=2)+'\n')
def call(cmd,**kw):return subprocess.run(cmd,capture_output=True,text=True,**kw)
def capture_sources():
 r=call(['git','-C',str(source),'ls-files','-z'],check=True);return {p:hashlib.sha256((source/p).read_bytes()).hexdigest() for p in r.stdout.split('\0') if p}
try:
 end=time.monotonic()+24*3600
 while True:
  assert control=={n:hashlib.sha256((root/n).read_bytes()).hexdigest() for n in control}
  r=call(['sudo','-n','docker','inspect','--format','{{json .State}}','nano-full-coherent-0f53b42d-amd64'],timeout=45)
  live=json.loads(r.stdout) if r.returncode==0 else None
  terminal=all((parent/f).exists() for f in ['build-exit.txt','ctest-exit.txt','container-exit.txt','source-compare-exit.txt','source-after.json','programs-after.json','link-inputs-after.sha256'])
  if not (live and live['Running']) and terminal:break
  state('waiting-original-full-campaign',containerState=live,observationExit=r.returncode,observationError=r.stderr,newDiagnosticStarted=False)
  assert time.monotonic()<end,'Observation limit exceeded; original campaign not restarted'
  time.sleep(15)
 assert (parent/'build-exit.txt').read_text().strip()=='0' and (parent/'source-compare-exit.txt').read_text().strip()=='0'
 assert json.loads((parent/'source-before.json').read_text())==json.loads((parent/'source-after.json').read_text())
 assert json.loads((parent/'programs-before.json').read_text())==json.loads((parent/'programs-after.json').read_text())
 assert (parent/'link-inputs-before.sha256').read_bytes()==(parent/'link-inputs-after.sha256').read_bytes()
 assert 'full configured block bound failed before core validation' in (parent/'LastTest.log').read_text()
 assert call(['git','-C',str(source),'rev-parse','HEAD'],check=True).stdout.strip()==revision
 assert not call(['git','-C',str(source),'status','--porcelain'],check=True).stdout
 assert call(['git','-C',str(source),'diff','--name-only','0f53b42d3e78da85aa9a2c2f2993472747aabe8a',revision],check=True).stdout.strip()=='tests/test_altair_p2p.cpp'
 before=capture_sources(); original=json.loads((parent/'source-after.json').read_text())
 assert set(before)==set(original)
 assert all(before[k]==original[k] for k in before if k!='tests/test_altair_p2p.cpp')
 report.mkdir(exist_ok=False);(report/'source-before.json.gz').write_bytes(gzip.compress(json.dumps(before,sort_keys=True).encode(),mtime=0));(report/'source-revision.txt').write_text(revision+'\n');state('running-diagnostic',newDiagnosticStarted=True)
 cmd=['sudo','-n','docker','run','--rm','--name','nano-altair-ingress-diagnostic-amd64','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','3g','--tmpfs','/tmp:rw,noexec,nosuid,size=512m','-v',str(source)+':/diag:ro','-v','nano-linux-sync-build:/build:ro','-v',str(root)+':/control:ro','-v',str(report)+':/reports','sha256:0fa8a608c09ff5ad1e804afc160c04d4964898bed149fcb47a2b587a0a9c194c','bash','/control/run.sh']
 (report/'command.json').write_text(json.dumps(cmd,indent=2)+'\n')
 with (report/'container.log').open('w') as log:r=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT)
 (report/'container-exit.txt').write_text(str(r.returncode)+'\n');after=capture_sources();(report/'source-after.json.gz').write_bytes(gzip.compress(json.dumps(after,sort_keys=True).encode(),mtime=0));assert before==after
 result={'diagnosticRevision':revision,'runtimeParent':'0f53b42d3e78da85aa9a2c2f2993472747aabe8a','sourceUnchanged':True,'originalFullCampaignPreserved':True,'standaloneDiagnosticOnly':True,'containerExit':r.returncode,'productionQualified':False}
 (report/'result.json').write_text(json.dumps(result,indent=2)+'\n');state('diagnostic-terminal',**result)
except Exception as e:
 state('diagnostic-refused',error=repr(e),productionQualified=False);raise
