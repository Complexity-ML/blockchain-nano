from pathlib import Path
import json,subprocess,hashlib
repo=Path('/Users/boris/Dev/blockchain-nano');frozen=Path('/private/tmp/nano-catalog-selection-docker-20261006/source');out=Path('/private/tmp/nano-catalog-selection-native-network-20261006');out.mkdir(exist_ok=False);core=Path('/private/tmp/nano-catalog-selection-native-20261006');template=Path('/private/tmp/nano-dispatch-recovery-p2p-native-20261006')
def run(name,args):
 (out/(name+'-command.json')).write_text(json.dumps(args,indent=2)+'\n')
 with (out/(name+'.log')).open('w') as f:r=subprocess.run(args,stdout=f,stderr=subprocess.STDOUT)
 (out/(name+'-exit.json')).write_text(json.dumps({'exitCode':r.returncode})+'\n');assert r.returncode==0,name
compile_args=json.loads((template/'compile-command.json').read_text());link_args=json.loads((template/'link-command.json').read_text());compile_args[1:1]=['-I'+str(frozen/'network')]
for case in ['runtime']:
 source=frozen/'network'/('main.cpp' if case=='runtime' else 'test_'+case+'.cpp');obj=out/(case+'.o');program=out/case;args=compile_args.copy();args[args.index('-c')+1]=str(source);args[args.index('-o')+1]=str(obj);run(case+'-compile',args)
 args=[str(obj) if x==str(template/'test.o') else str(program) if x==str(template/'test') else x for x in link_args];run(case+'-link',args)
 if case!='runtime':run(case+'-run',[str(program)]);print((out/(case+'-run.log')).read_text()[-700:],flush=True)
 else:
  r=subprocess.run([str(program)],capture_output=True,text=True);(out/'runtime-usage.log').write_text(r.stdout+r.stderr);(out/'runtime-usage-exit.json').write_text(json.dumps({'exitCode':r.returncode,'expectedExitCode':1})+'\n');assert r.returncode==1 and 'Usage' in r.stdout+r.stderr
 (out/(case+'-program.json')).write_text(json.dumps({'sha256':hashlib.sha256(program.read_bytes()).hexdigest(),'sourceSHA256':hashlib.sha256(source.read_bytes()).hexdigest()},indent=2)+'\n')
print('Native P2P compiled from frozen catalogue-selection sources; no WAN qualification.',flush=True)
