from pathlib import Path
import json,subprocess,hashlib
root=Path('/private/tmp/nano-catalog-reactor-failfast-20261006');out=root/'native';src=root/'source';old=Path('/private/tmp/nano-catalog-reservation-integrated-native-20261006')
def run(name,args):
 (out/(name+'-command.json')).write_text(json.dumps(args,indent=2))
 with (out/(name+'.log')).open('w') as f:p=subprocess.run(args,cwd='/tmp/nano-pruning-wan-native-build-20261004',stdout=f,stderr=subprocess.STDOUT)
 (out/(name+'-exit.json')).write_text(json.dumps({'exitCode':p.returncode}));print(name,p.returncode,flush=True)
 if p.returncode:raise RuntimeError(name)
prefix=json.loads((out/'compile-consensus_capella_reactor-command.json').read_text());prefix=prefix[:prefix.index('-c')]
for program,file in [('nano','src/main.cpp'),('nano-capella-paused-capture','tests/capella/paused_capture.cpp')]:
 run('compile-'+program,prefix+['-c',str(src/file),'-o',str(out/(program+'.o')),'-MMD','-MF',str(out/(program+'.d'))])
 args=json.loads((old/(program+'-link-command.json')).read_text());args=[a.replace('/private/tmp/nano-network-status-coherent-native-20261006/'+program+'.o',str(out/(program+'.o'))).replace(str(old/'libnano_core.a'),str(out/'libnano_core.a')).replace(str(old/program),str(out/program)) for a in args]
 run('link-'+program,args)
run('nano-help',[str(out/'nano'),'--help'])
for mode in ['wire','configured-runtime','automatic-recovery']:run('test-'+mode,[str(out/'candidate'),'--'+mode])
(root/'native-runtime-hashes.json').write_text(json.dumps({n:hashlib.sha256((out/n).read_bytes()).hexdigest() for n in ['nano','nano-capella-paused-capture','candidate','libnano_core.a']},indent=2)+'\n')
