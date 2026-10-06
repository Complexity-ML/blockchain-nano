from pathlib import Path
import json,subprocess
r=Path('/private/tmp/nano-startup-diagnostic-draft-20261006');out=Path('/private/tmp/nano-startup-diagnostic-observer-native-repair-proof-20261006');out.mkdir(exist_ok=False);old=Path('/private/tmp/nano-signed-anchor-native-20261006')
compile=json.loads((old/'capella_block_delivery-compile-command.json').read_text());compile.insert(1,'-I'+str(r));compile[compile.index('-c')+1]=str(r/'test_altair_core_recovery.cpp');compile[compile.index('-o')+1]=str(out/'recovery.o')
link=json.loads((old/'capella_block_delivery-link-command.json').read_text());link=[str(out/'recovery.o') if x.endswith('/capella_block_delivery.o') else x for x in link];link[link.index('-o')+1]=str(out/'recovery')
for name,args in [('compile',compile),('link',link),('run',[str(out/'recovery')])]:
 (out/(name+'-command.json')).write_text(json.dumps(args,indent=2)+'\n')
 with (out/(name+'.log')).open('w') as f:p=subprocess.run(args,stdout=f,stderr=subprocess.STDOUT)
 (out/(name+'-exit.json')).write_text(json.dumps({'exitCode':p.returncode})+'\n');assert p.returncode==0,name
print('Recovery observer modes and fail-closed stage diagnostics pass in isolated native draft')
