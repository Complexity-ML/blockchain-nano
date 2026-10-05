import pathlib,subprocess,json,hashlib,sys
repo=pathlib.Path('/Users/boris/Dev/blockchain-nano');out=pathlib.Path('/private/tmp/nano-monotonic-runtime-native-build-20261005');control=pathlib.Path(__file__).parent
protected=json.loads((out/'programs.json').read_text())
for p,h in protected.items():assert hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()==h
flags=json.loads((out/'compile-nano-capella-transition-tests-command.json').read_text());flags=flags[:flags.index('-c')]
compile=flags+['-c',str(repo/'tests/multihost/rotation_audit.cpp'),'-o',str(out/'rotation-audit.cpp.o')]
link=json.loads((out/'link-nano-command.json').read_text());link=[str(out/'rotation-audit.cpp.o') if a.endswith('/main.cpp.o') else str(out/'nano-capella-rotation-audit') if a==str(out/'nano') or a=='/tmp/nano-monotonic-runtime-native-build-20261005/nano' else a for a in link]
for name,args,cwd in [('compile',compile,repo),('link',link,pathlib.Path('/tmp/nano-pruning-wan-native-build-20261004')),('self-test',[str(out/'nano-capella-rotation-audit'),'--self-test'],repo)]:
 (control/(name+'-command.json')).write_text(json.dumps(args,indent=2))
 with (control/(name+'.log')).open('w') as f:r=subprocess.run(args,cwd=cwd,stdout=f,stderr=subprocess.STDOUT)
 (control/(name+'-exit.txt')).write_text(str(r.returncode)+'\n')
 if r.returncode:sys.exit(r.returncode)
for p,h in protected.items():assert hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()==h
(control/'native-result.json').write_text(json.dumps({'exit':0,'sourceSha256':hashlib.sha256((repo/'tests/multihost/rotation_audit.cpp').read_bytes()).hexdigest(),'auditorSha256':hashlib.sha256((out/'nano-capella-rotation-audit').read_bytes()).hexdigest(),'runtimeProgramsUnchanged':protected,'networkQualified':False},indent=2)+'\n')
