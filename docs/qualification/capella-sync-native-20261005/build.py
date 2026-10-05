import pathlib,subprocess,shlex,json,hashlib,sys
repo=pathlib.Path('/Users/boris/Dev/blockchain-nano');base=pathlib.Path('/tmp/nano-pruning-wan-native-build-20261004');out=pathlib.Path('/tmp/nano-capella-sync-native-build-20261005');out.mkdir()
def run(args,name,cwd=None):
 (out/(name+'-command.json')).write_text(json.dumps(args,indent=2))
 with (out/(name+'.log')).open('w') as log:r=subprocess.run(args,cwd=cwd,stdout=log,stderr=subprocess.STDOUT)
 (out/(name+'-exit.txt')).write_text(str(r.returncode)+'\n')
 if r.returncode:sys.exit(r.returncode)
def flags(target):
 vals={}
 for line in (base/'CMakeFiles'/target/'flags.make').read_text().splitlines():
  if ' = ' in line:
   k,v=line.split(' = ',1);vals[k]=shlex.split(v)
 return [a.replace('/tmp/nano-pruning-wan-source-20261004',str(repo)) for a in vals['CXX_DEFINES']+vals['CXX_INCLUDES']+vals['CXX_FLAGS']]
run(['cp','/tmp/nano-native-nano2-build-20261005/libnano_core.a',str(out/'libnano_core.a')],'copy-core')
for name in ['consensus_capella_runtime','node','rpc']:
 run(['/usr/bin/c++',*flags('nano_core.dir'),'-c',str(repo/'src'/(name+'.cpp')),'-o',str(out/(name+'.cpp.o'))],'compile-'+name)
 run(['ar','r',str(out/'libnano_core.a'),str(out/(name+'.cpp.o'))],'archive-'+name)
for target,source,obj,program in [('nano.dir','src/main.cpp','main.cpp.o','nano'),('nano-capella-transition-tests.dir','tests/test_capella_transition.cpp','test.cpp.o','nano-capella-transition-tests')]:
 run(['/usr/bin/c++',*flags(target),'-c',str(repo/source),'-o',str(out/obj)],'compile-'+program)
 link=shlex.split((base/'CMakeFiles'/target/'link.txt').read_text())
 link=[str(out/obj) if a.startswith('CMakeFiles/') else str(out/program) if a==program else str(out/'libnano_core.a') if a=='libnano_core.a' else a for a in link]
 run(link,'link-'+program,cwd=base)
(out/'programs.json').write_text(json.dumps({str(out/p):hashlib.sha256((out/p).read_bytes()).hexdigest() for p in ['nano','nano-capella-transition-tests','libnano_core.a']},indent=2)+'\n')
(out/'controller-exit.txt').write_text('0\n')
