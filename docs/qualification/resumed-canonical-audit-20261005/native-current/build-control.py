from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import subprocess,shlex,json,hashlib,shutil,sys
r=Path('/Users/boris/Dev/blockchain-nano');base=Path('/tmp/nano-pruning-wan-native-build-20261004');parent=Path('/tmp/nano-startup-catchup-native-build-20261005');out=Path('/private/tmp/nano-current-consensus-native-20261005');out.mkdir()
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
protected=[parent/'libnano_core.a',parent/'nano-capella-transition-tests'];before={str(p):digest(p) for p in protected};(out/'parent-before.json').write_text(json.dumps(before,indent=2)+'\n')
revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=r,text=True).strip()
changed=subprocess.check_output(['git','diff','--name-only','6b7a437e','HEAD','--','src','include'],cwd=r,text=True).splitlines(); headers=[p for p in changed if p.endswith('.hpp')];units=[]
for source in sorted((r/'src').glob('*.cpp')):
 if source.name=='main.cpp':continue
 dep=base/'CMakeFiles/nano_core.dir/src'/(source.name+'.o.d')
 if str(source.relative_to(r)) in changed or not dep.exists() or any(Path(h).name in dep.read_text() for h in headers):units.append(source)
(out/'dependency-closure.json').write_text(json.dumps({'checkout':revision,'parentCoreProducer':'6b7a437e','changedHeaders':headers,'changedFiles':changed,'recompiledUnits':[str(p.relative_to(r)) for p in units]},indent=2)+'\n')
sources=units+[r/'src/main.cpp',r/'tests/test_capella_transition.cpp',r/'tests/capella/paused_capture.cpp']+[r/h for h in headers]; hashes={str(p.relative_to(r)):digest(p) for p in sources};(out/'sources.json').write_text(json.dumps(hashes,indent=2)+'\n')
def flags(target):
 vals={}
 for l in (base/'CMakeFiles'/f'{target}.dir/flags.make').read_text().splitlines():
  if ' = ' in l:k,v=l.split(' = ',1);vals[k]=shlex.split(v)
 return [a.replace('/tmp/nano-pruning-wan-source-20261004',str(r)) for a in vals['CXX_DEFINES']+vals['CXX_INCLUDES']+vals['CXX_FLAGS']]+['-I'+str(r/'third_party/ckzg/blst/bindings')]
def run(name,cmd):
 (out/(name+'-command.json')).write_text(json.dumps(cmd,indent=2)+'\n')
 with (out/(name+'.log')).open('w') as f:p=subprocess.run(cmd,cwd=base,stdout=f,stderr=subprocess.STDOUT)
 (out/(name+'-exit.txt')).write_text(str(p.returncode)+'\n');print(name,p.returncode,flush=True)
 if p.returncode:raise RuntimeError(name+' failed: '+(out/(name+'.log')).read_text()[-2000:])
def compile_core(p):run(p.stem+'-compile',['/usr/bin/c++',*flags('nano_core'),'-c',str(p),'-o',str(out/(p.name+'.o'))])
try:
 with ThreadPoolExecutor(max_workers=2) as pool:list(pool.map(compile_core,units))
 shutil.copy2(parent/'libnano_core.a',out/'libnano_core.a');run('archive',['/usr/bin/ar','r',str(out/'libnano_core.a'),*[str(out/(p.name+'.o')) for p in units]]);run('archive-index',['/usr/bin/ranlib',str(out/'libnano_core.a')])
 for target,source,template in [('nano',r/'src/main.cpp','nano'),('nano-capella-transition-tests',r/'tests/test_capella_transition.cpp','nano-capella-transition-tests'),('nano-capella-paused-capture',r/'tests/capella/paused_capture.cpp','nano-capella-transition-tests')]:
  obj=out/(target+'.o');run(target+'-compile',['/usr/bin/c++',*flags(template),'-c',str(source),'-o',str(obj)])
  link=shlex.split((base/'CMakeFiles'/f'{template}.dir/link.txt').read_text());link=[str(obj) if a.startswith('CMakeFiles/') else str(out/target) if a==template else str(out/'libnano_core.a') if a=='libnano_core.a' else a for a in link];run(target+'-link',link)
 for mode in ['prepared-block','configured-runtime','scheduler']:run(mode,[str(out/'nano-capella-transition-tests'),'--'+mode])
 assert before=={str(p):digest(p) for p in protected};assert hashes=={str(p.relative_to(r)):digest(p) for p in sources}
 (out/'parent-unchanged.json').write_text(json.dumps(before,indent=2)+'\n');(out/'programs.json').write_text(json.dumps({n:digest(out/n) for n in ['nano','libnano_core.a','nano-capella-transition-tests','nano-capella-paused-capture']},indent=2)+'\n');(out/'controller-exit.txt').write_text('0\n')
except Exception as e:
 (out/'controller-exit.txt').write_text('1\n');print(str(e),flush=True);sys.exit(1)
