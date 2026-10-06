from pathlib import Path
import subprocess,json,hashlib
root=Path('/private/tmp/nano-head-fallback-20261006');out=root/'native';out.mkdir(exist_ok=False);template=Path('/private/tmp/nano-dispatch-recovery-p2p-native-20261006');compile_args=json.loads((template/'compile-command.json').read_text());link_args=json.loads((template/'link-command.json').read_text())
compile_args[1:1]=['-I'+str(root/'network')]
for case in ['capella_catalog_relay','capella_head_catalog_relay','capella_depth_catchup','capella_block_delivery','capella_attestation_delivery','capella_export_resume','runtime']:
 source=root/'network'/('main.cpp' if case=='runtime' else 'test_'+case+'.cpp');obj=out/(case+'.o');program=out/case;args=compile_args.copy();args[args.index('-c')+1]=str(source);args[args.index('-o')+1]=str(obj)
 if case=='capella_export_resume':args.insert(1,'-DNANO_TEST_EXPORT_CACHE')
 (out/(case+'-compile-command.json')).write_text(json.dumps(args,indent=2)+'\n')
 with (out/(case+'-compile.log')).open('w') as f:r=subprocess.run(args,stdout=f,stderr=subprocess.STDOUT)
 (out/(case+'-compile-exit.json')).write_text(json.dumps({'exitCode':r.returncode})+'\n');assert r.returncode==0,case
 args=[str(obj) if x==str(template/'test.o') else str(program) if x==str(template/'test') else x for x in link_args];(out/(case+'-link-command.json')).write_text(json.dumps(args,indent=2)+'\n')
 with (out/(case+'-link.log')).open('w') as f:r=subprocess.run(args,stdout=f,stderr=subprocess.STDOUT)
 (out/(case+'-link-exit.json')).write_text(json.dumps({'exitCode':r.returncode})+'\n');assert r.returncode==0,case
 if case!='runtime':
  with (out/(case+'-run.log')).open('w') as f:r=subprocess.run([str(program)],stdout=f,stderr=subprocess.STDOUT)
  (out/(case+'-run-exit.json')).write_text(json.dumps({'exitCode':r.returncode})+'\n');assert r.returncode==0,case
  print((out/(case+'-run.log')).read_text(),flush=True)
 (out/(case+'-program.json')).write_text(json.dumps({'sha256':hashlib.sha256(program.read_bytes()).hexdigest(),'sourceSHA256':hashlib.sha256(source.read_bytes()).hexdigest()},indent=2)+'\n')
print('Native tests and updated P2P executable built against frozen candidate relay',flush=True)
