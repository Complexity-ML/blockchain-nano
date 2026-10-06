import pathlib,json,shutil,subprocess,hashlib,os
r=pathlib.Path('/Users/boris/Dev/blockchain-nano');root=pathlib.Path('/private/tmp/nano-catalog-reactor-failfast-20261006');src=root/'source';out=root/'native';root.mkdir();shutil.copytree('/private/tmp/nano-catalog-busy-core-fixture-20261006/source',src);out.mkdir()
for name in ['CMakeLists.txt','include/nano/consensus_capella_reactor.hpp','network/capella_inventory_sessions.hpp','src/consensus_capella_reactor.cpp','tests/test_capella_transition.cpp']:shutil.copy2(r/name,src/name)
def manifest():return {str(p.relative_to(src)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(src.rglob('*')) if p.is_file()}
initial=manifest();(root/'source-before.json').write_text(json.dumps(initial,indent=2))
old='/private/tmp/nano-catalog-reservation-integrated-native-20261006';oldsrc='/private/tmp/nano-catalog-reservation-integrated-docker-20261006/source'
def run(name,args,env=None):
 (out/(name+'-command.json')).write_text(json.dumps(args,indent=2))
 with (out/(name+'.log')).open('w') as f:p=subprocess.run(args,cwd='/tmp/nano-pruning-wan-native-build-20261004',stdout=f,stderr=subprocess.STDOUT,env=env)
 (out/(name+'-exit.json')).write_text(json.dumps({'exitCode':p.returncode}));print(name,p.returncode,flush=True)
 if p.returncode:raise RuntimeError(name)
for name in ['consensus_capella_reactor','rpc','consensus_capella_runtime']:
 file=pathlib.Path(old)/('dependency-'+name+'-command.json' if name=='consensus_capella_reactor' else 'compile-'+name+'-command.json')
 args=json.loads(file.read_text());args=[a.replace(oldsrc,str(src)).replace(old,str(out)) for a in args]
 if '-MM' in args:
  i=args.index('-MM');args=args[:i]+['-c',str(src/'src'/ (name+'.cpp')),'-o',str(out/(name+'.cpp.o')),'-MMD','-MF',str(out/(name+'.cpp.d'))]
 else:args+=['-MMD','-MF',str(out/(name+'.cpp.d'))]
 args.insert(1,'-I'+str(src/'network'));run('compile-'+name,args)
shutil.copy2(pathlib.Path(old)/'libnano_core.a',out/'libnano_core.a');run('archive',['/usr/bin/ar','r',str(out/'libnano_core.a')]+[str(out/(n+'.cpp.o')) for n in ['consensus_capella_reactor','rpc','consensus_capella_runtime']])
probe=pathlib.Path('/private/tmp/nano-v27-reactor-wait-probe-20261006/native');args=json.loads((probe/'compile-command.json').read_text());args=[a.replace('/private/tmp/nano-v27-reactor-wait-probe-20261006/source',str(src)).replace(str(probe),str(out)) for a in args];run('compile-test',args)
args=json.loads((probe/'link-command.json').read_text());args=[a.replace(str(probe),str(out)).replace(old+'/libnano_core.a',str(out/'libnano_core.a')) for a in args];run('link-test',args)
for name,mode,fields in [('reactor-failfast','--beacon-reads',{'NANO_TEST_CATALOG_REACTOR_FAILFAST':'1'}),('generation-reservation','--beacon-reads',{'NANO_TEST_CATALOG_GENERATION_RACE':'1'}),('capture-control','--beacon-reads',{'NANO_TEST_CATALOG_CAPTURE_UNLOCKED':'1'}),('worker-control','--beacon-reads',{'NANO_TEST_CATALOG_WORKER_FAILFAST':'1'}),('worker-http','--queued-bls-rpc',{'NANO_TEST_CATALOG_WORKER_FAILFAST':'1'})]:run('test-'+name,[str(out/'candidate'),mode],os.environ|fields)
final=manifest();(root/'source-after.json').write_text(json.dumps(final,indent=2));assert final==initial
(root/'result.json').write_text(json.dumps({'sourceUnchanged':True,'tests':5,'networkQualified':False,'productionQualified':False},indent=2))
