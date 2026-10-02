import json,subprocess,sys,time
from pathlib import Path
control=Path(__file__).resolve().parent
full=Path('/home/ubuntu/nano-full-coherent-0f53b42d-amd64-reports-20261002')
state=control/'queue-state.json'
def save(value):state.write_text(json.dumps(value,indent=2)+'\n')
for _ in range(5760):
 r=subprocess.run(['sudo','-n','docker','inspect','--format','{{json .State}}','nano-full-coherent-0f53b42d-amd64'],capture_output=True,text=True,timeout=30)
 if r.returncode==0:
  info=json.loads(r.stdout)
  if info['Running']:
   save({'phase':'waiting-verified-live-full-campaign','pid':info['Pid'],'observedAt':time.time(),'newAuditStarted':False});time.sleep(15);continue
  save({'phase':'full-container-terminal-awaiting-host-manifests','exit':info['ExitCode'],'newAuditStarted':False})
 elif (full/'container-exit.txt').is_file():
  save({'phase':'full-host-terminal','exit':int((full/'container-exit.txt').read_text()),'newAuditStarted':False})
 else:
  # Missing handle/failed observation is not enough to launch or replace anything.
  save({'phase':'observation-unresolved','stderr':r.stderr[:300],'newAuditStarted':False});time.sleep(15);continue
 if not (full/'source-compare-exit.txt').is_file():time.sleep(15);continue
 for f in ['build-exit.txt','ctest-exit.txt','container-exit.txt','source-compare-exit.txt']:
  if (full/f).read_text().strip()!='0':
   save({'phase':'full-gate-refused','failedExitFile':f,'newAuditStarted':False});raise SystemExit(2)
 save({'phase':'launching-readonly-signed-audit','newAuditStarted':True})
 r=subprocess.run([sys.executable,str(control/'run.py')])
 save({'phase':'audit-terminal','exit':r.returncode,'newAuditStarted':True});raise SystemExit(r.returncode)
save({'phase':'observation-deadline','newAuditStarted':False});raise SystemExit(124)
