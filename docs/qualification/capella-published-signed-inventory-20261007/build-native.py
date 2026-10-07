from pathlib import Path
import subprocess,json,sys,hashlib,shlex,re,os
r=Path(__file__).parent;repo=Path('/Users/boris/Dev/blockchain-nano');docker=['/Users/boris/.docker/bin/docker'];container='nano-restored-three-node-mac-muxdz001';terminal=Path('/private/tmp/nano-restoration-v35-trace-bound-prepare-20261007/original-exit.json')
t=json.loads(terminal.read_text());assert t['session']==76628 and t['exitCode'] in (0,1)
s=json.loads(subprocess.check_output(docker+['inspect',container,'--format','{{json .State}}'],text=True));assert not s['Running']
assert not any('/nano run ' in l for l in subprocess.check_output(['ps','-axo','pid,command'],text=True).splitlines())

m=json.loads((r/'manifest.json').read_text())
for rel,h in m['files'].items():assert hashlib.sha256((repo/rel).read_bytes()).hexdigest()==h['baseSHA256'];assert hashlib.sha256((r/'source'/rel).read_bytes()).hexdigest()==h['draftSHA256']
logs=r/'native-build';logs.mkdir();old='/private/tmp/nano-coherent-e33ad89e-20261006/native-build';parent='/private/tmp/nano-startup-checkpoint-memo-20261007/native-repair';out=str(logs/'programs')
def tool(args):
 if args[0]=='sha256sum':args=['/usr/bin/shasum','-a','256',*args[1:]]
 return subprocess.check_output(args,text=True,cwd=old)
def run(name,args,env=None,expected=0):
 (logs/(name+'-command.json')).write_text(json.dumps({'args':args,'env':env or {},'expectedExit':expected,'cwd':old})+'\n')
 with (logs/(name+'.log')).open('w') as f:x=subprocess.run(args,cwd=old,env={**os.environ,**(env or {})},stdout=f,stderr=subprocess.STDOUT)
 (logs/(name+'-exit.json')).write_text(json.dumps({'exitCode':x.returncode})+'\n');assert x.returncode==expected,(name,x.returncode);print(name+' exit'+str(x.returncode),flush=True)
def flags(target):
 rows=tool(['cat',old+'/CMakeFiles/'+target+'.dir/flags.make']).replace('/private/tmp/nano-coherent-e33ad89e-20261006/source',str(repo)).splitlines();return sum([shlex.split(next(x.split(' = ',1)[1] for x in rows if x.startswith(k+' = '))) for k in ['CXX_DEFINES','CXX_INCLUDES','CXX_FLAGS']],[])
parents=[parent+'/'+n for n in ['nano','test','capture','libnano_core.a']];before=tool(['sha256sum']+parents);(logs/'parent-before.sha256').write_text(before)
qualified=repo/'docs/qualification/capella-startup-reference-memo-20261007'/('native-repair/programs.json')
q=json.loads(qualified.read_text());assert all(hashlib.sha256(Path(parent,n).read_bytes()).hexdigest()==h for n,h in q.items())
run('stage',['sh','-c','test ! -e '+out+' && mkdir '+out+' && cp '+parent+'/libnano_core.a '+out+'/libnano_core.a'])
def link(name,target,obj,library):
 args=shlex.split(tool(['cat',old+'/CMakeFiles/'+target+'.dir/link.txt']));positions=[i for i,v in enumerate(args) if v.endswith('.cpp.o')];assert len(positions)==1;args[positions[0]]=obj;args=[library if x=='libnano_core.a' else x for x in args];args[args.index('-o')+1]=out+'/'+name;run('link-'+name,args)
fixture='/private/tmp/nano-published-signed-inventory-draft-20261007/source/tests/test_capella_transition.cpp'
# Original header/layout is intentional here: never link a new ABI fixture
# against an old library. The new scenario uses only existing public methods.
run('compile-parent-fixture',['/usr/bin/clang++',*flags('nano-capella-transition-tests'),'-I/Users/boris/Dev/blockchain-nano/tests','-c',fixture,'-o',out+'/parent-fixture.o'])
link('parent-test','nano-capella-transition-tests',out+'/parent-fixture.o',parent+'/libnano_core.a')
run('parent-held-counterexample',[out+'/parent-test','--beacon-reads'],{'NANO_TEST_SIGNED_INVENTORY_PUBLISHED':'1'},1)
assert 'signed inventory blocked, mixed durable publication or lost bounds/cancellation' in (logs/'parent-held-counterexample.log').read_text()
# Generate dependencies afresh for every core source, not merely the edited TU.
core_sources=['/Users/boris/Dev/blockchain-nano/'+str(p.relative_to(repo)) for p in sorted((repo/'src').glob('*.cpp')) if p.name!='main.cpp']
core_sources=[('/private/tmp/nano-published-signed-inventory-draft-20261007/source/src/consensus_capella_reactor.cpp' if x.endswith('/consensus_capella_reactor.cpp') else x) for x in core_sources]
run('fresh-core-dependencies',['/usr/bin/clang++','-I/private/tmp/nano-published-signed-inventory-draft-20261007/source/include',*flags('nano_core'),'-I/Users/boris/Dev/blockchain-nano/src','-MM',*core_sources])
text=(logs/'fresh-core-dependencies.log').read_text().replace('\\\n',' ');rules=[line for line in text.splitlines() if re.match(r'^[^\s:]+\.o:\s',line)];assert len(rules)==len(core_sources),(len(rules),len(core_sources));affected=[]
for line in rules:
 deps=shlex.split(line.split(': ',1)[1]);source=next(x for x in deps if x.endswith('.cpp'))
 if any(x.endswith('/nano/consensus_capella_reactor.hpp') for x in deps):affected.append(source)
assert any(x.endswith('/consensus_capella_reactor.cpp') for x in affected)
(logs/'affected-core-sources.json').write_text(json.dumps({'coreSourceCount':len(core_sources),'affected':affected,'derivedFromFreshCompilerDependencies':True},indent=2)+'\n')
objects=[]
for source in affected:
 name=Path(source).name;obj=out+'/'+name+'.o';objects.append(obj);run('compile-core-'+name,['/usr/bin/clang++','-I/private/tmp/nano-published-signed-inventory-draft-20261007/source/include',*flags('nano_core'),'-I/Users/boris/Dev/blockchain-nano/src','-MD','-MF',out+'/'+name+'.d','-c',source,'-o',obj])
old_members=tool(['/usr/bin/ar','t',out+'/libnano_core.a']).splitlines();assert all(Path(x).name in old_members for x in objects)
run('replace-all-affected-core',['/usr/bin/ar','r',out+'/libnano_core.a',*objects]);assert tool(['/usr/bin/ar','t',out+'/libnano_core.a']).splitlines()==old_members
programs=[('test','nano-capella-transition-tests',fixture,['-I/Users/boris/Dev/blockchain-nano/tests']),('nano','nano','/Users/boris/Dev/blockchain-nano/src/main.cpp',[]),('capture','nano-capella-paused-capture','/Users/boris/Dev/blockchain-nano/tests/capella/paused_capture.cpp',['-I/Users/boris/Dev/blockchain-nano/third_party/ckzg/blst/bindings'])]
for name,target,source,extra in programs:
 obj=out+'/'+name+'.o';run('compile-'+name,['/usr/bin/clang++','-I/private/tmp/nano-published-signed-inventory-draft-20261007/source/include',*flags(target),*extra,'-c',source,'-o',obj]);link(name,target,obj,out+'/libnano_core.a')
run('held-signed-inventories',[out+'/test','--beacon-reads'],{'NANO_TEST_SIGNED_INVENTORY_PUBLISHED':'1'})
assert (logs/'held-signed-inventories.log').read_text().count('published-signed-inventory-during-held-admission')==3
run('ephemeral-sync-failfast',[out+'/test','--beacon-reads'],{'NANO_TEST_CATALOG_REACTOR_FAILFAST':'1'})
run('published-block-regression',[out+'/test','--beacon-reads'],{'NANO_TEST_BLOCK_INVENTORY_PUBLISHED':'1'})
run('signed-slashing-pruning-restart',[out+'/test','--pruning-slashing'])
run('startup-recovery-and-migration',[out+'/test','--prepared-block'],{'NANO_TEST_STARTUP_REFERENCE_MEMO':'1','NANO_TEST_COLD_REACTOR':'1','NANO_TEST_FRAGMENT_CHECKPOINT':'1'})
run('cold-runtime-durable-wrapper',[out+'/test','--configured-runtime'],{'NANO_TEST_DURABLE_CANONICAL_MEMO':'1','NANO_TEST_RUNTIME_REFERENCE_MEMO':'1','NANO_TEST_COLD_RUNTIME':'1'})
after=tool(['sha256sum']+parents);assert before==after;(logs/'parent-after.sha256').write_text(after);(logs/'programs.sha256').write_text(tool(['sha256sum']+[out+'/'+n for n in ['nano','test','capture','libnano_core.a']]))
for rel,h in m['files'].items():assert hashlib.sha256((repo/rel).read_bytes()).hexdigest()==h['baseSHA256'];assert hashlib.sha256((r/'source'/rel).read_bytes()).hexdigest()==h['draftSHA256']
(logs/'binding.json').write_text(json.dumps({'allAffectedCoreConsumersRecompiled':True,'selectedProgramsRecompiled':True,'parentUnchanged':True,'sourceFiles':m['files'],'networkQualified':False,'cleanReleaseQualified':False},indent=2)+'\n')
print('Dependency-complete scoped candidate qualified; no WAN or clean-release claim',flush=True)
