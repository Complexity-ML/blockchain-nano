from pathlib import Path
import json,subprocess
base=Path('/private/tmp');campaign=base/'nano-restored-three-node-v20-startup-diagnostic-20261006/campaign';terminal=json.loads((campaign/'terminal-exit.json').read_text());assert terminal.get('originalSession')==34949 and terminal['exitCode'] in [0,1]
assert json.loads((campaign/'result.json').read_text())['stopReceipts']==3
r=base/'nano-delivery-cause-draft-20261006';out=base/'nano-delivery-cause-native-draft-proof-20261006';out.mkdir(exist_ok=False);old=base/'nano-startup-diagnostic-native-20261006'
for case in ['core_rpc_diagnostic','capella_attestation_delivery']:
 c=json.loads((old/(case+'-compile-command.json')).read_text());c.insert(1,'-I'+str(r));c[c.index('-c')+1]=str(r/('test_'+case+'.cpp'));c[c.index('-o')+1]=str(out/(case+'.o'))
 l=json.loads((old/(case+'-link-command.json')).read_text());l=[str(out/(case+'.o')) if x==str(old/(case+'.o')) else x for x in l];l[l.index('-o')+1]=str(out/case)
 for phase,args in [('compile',c),('link',l),('run',[str(out/case)])]:
  name=case+'-'+phase;(out/(name+'-command.json')).write_text(json.dumps(args,indent=2)+'\n')
  with (out/(name+'.log')).open('w') as f:p=subprocess.run(args,stdout=f,stderr=subprocess.STDOUT)
  (out/(name+'-exit.json')).write_text(json.dumps({'exitCode':p.returncode})+'\n');assert p.returncode==0,name
 print(case,(out/(case+'-run.log')).read_text().strip(),flush=True)
