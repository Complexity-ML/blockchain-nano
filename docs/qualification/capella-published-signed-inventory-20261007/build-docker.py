"""Run only after original V35 is terminal and all its writers are stopped.
Fresh compiler dependencies determine every core consumer of the changed ABI.
This is a dependency-complete incremental candidate, not a clean release build.
"""
from pathlib import Path
import subprocess,json,sys,hashlib,shlex,re
r=Path(__file__).parent;arch=sys.argv[1];assert arch in ('arm64','amd64')
if arch=='arm64':
 docker=['/Users/boris/.docker/bin/docker'];repo=Path('/Users/boris/Dev/blockchain-nano');image='e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';volume='nano-clean-arm64-build-20261002';container='nano-restored-three-node-mac-muxdz001';terminal=Path('/private/tmp/nano-restoration-v35-trace-bound-prepare-20261007/original-exit.json')
else:
 docker=['sudo','-n','docker'];repo=Path('/home/ubuntu/blockchain-nano-git');image='fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';volume='nano-clean-rpc-build-20261002';container='nano-restored-three-node-vps-muxdz001';terminal=r/'v35-original-exit.json'
t=json.loads(terminal.read_text());assert t['session']==76628 and t['exitCode'] in (0,1)
s=json.loads(subprocess.check_output(docker+['inspect',container,'--format','{{json .State}}'],text=True));assert not s['Running']
if arch=='arm64':assert not any('/nano run ' in l for l in subprocess.check_output(['ps','-axo','pid,command'],text=True).splitlines())
else:assert not subprocess.check_output(docker+['ps','-q','--filter','volume=nano-restored-three-node-vps-20261005-data'],text=True).strip()
m=json.loads((r/'manifest.json').read_text())
for rel,h in m['files'].items():assert hashlib.sha256((repo/rel).read_bytes()).hexdigest()==h['baseSHA256'];assert hashlib.sha256((r/'source'/rel).read_bytes()).hexdigest()==h['draftSHA256']
logs=r/(arch+'-build');logs.mkdir();old='/build/coherent-e33ad89e-'+arch;parent='/build/startup-checkpoint-memo-'+arch+'-20261007';out='/build/published-signed-inventory-'+arch+'-20261007'
base=docker+['run','--rm','--cpus','1','--network','none','--read-only','--tmpfs','/tmp','--ulimit','nofile=65536:65536','-v',volume+':/build','-v',str(repo)+':/work:ro','-v',str(r)+':/patch:ro','-w',old]
def tool(args):return subprocess.check_output(base+['--entrypoint',args[0],image]+args[1:],text=True)
def run(name,args,env=None,expected=0):
 (logs/(name+'-command.json')).write_text(json.dumps({'args':args,'env':env or {},'expectedExit':expected,'cwd':old})+'\n')
 with (logs/(name+'.log')).open('w') as f:x=subprocess.run(base+sum([['-e',k+'='+v] for k,v in (env or {}).items()],[])+['--entrypoint',args[0],image]+args[1:],stdout=f,stderr=subprocess.STDOUT)
 (logs/(name+'-exit.json')).write_text(json.dumps({'exitCode':x.returncode})+'\n');assert x.returncode==expected,(name,x.returncode);print(name+' exit'+str(x.returncode),flush=True)
def flags(target):
 rows=tool(['cat',old+'/CMakeFiles/'+target+'.dir/flags.make']).splitlines();return sum([shlex.split(next(x.split(' = ',1)[1] for x in rows if x.startswith(k+' = '))) for k in ['CXX_DEFINES','CXX_INCLUDES','CXX_FLAGS']],[])
parents=[parent+'/'+n for n in ['nano','test','capture','libnano_core.a']];before=tool(['sha256sum']+parents);(logs/'parent-before.sha256').write_text(before)
qualified=repo/'docs/qualification/capella-startup-reference-memo-20261007'/(arch+'-repair/programs.sha256')
for line in qualified.read_text().splitlines():assert line in before.splitlines()
run('stage',['sh','-c','test ! -e '+out+' && mkdir '+out+' && cp '+parent+'/libnano_core.a '+out+'/libnano_core.a'])
def link(name,target,obj,library):
 args=shlex.split(tool(['cat',old+'/CMakeFiles/'+target+'.dir/link.txt']));positions=[i for i,v in enumerate(args) if v.endswith('.cpp.o')];assert len(positions)==1;args[positions[0]]=obj;args=[library if x=='libnano_core.a' else x for x in args];args[args.index('-o')+1]=out+'/'+name;run('link-'+name,args)
fixture='/patch/source/tests/test_capella_transition.cpp'
# Original header/layout is intentional here: never link a new ABI fixture
# against an old library. The new scenario uses only existing public methods.
run('compile-parent-fixture',['/usr/bin/clang++',*flags('nano-capella-transition-tests'),'-I/work/tests','-c',fixture,'-o',out+'/parent-fixture.o'])
link('parent-test','nano-capella-transition-tests',out+'/parent-fixture.o',parent+'/libnano_core.a')
run('parent-held-counterexample',[out+'/parent-test','--beacon-reads'],{'NANO_TEST_SIGNED_INVENTORY_PUBLISHED':'1'},1)
assert 'signed inventory blocked, mixed durable publication or lost bounds/cancellation' in (logs/'parent-held-counterexample.log').read_text()
# Generate dependencies afresh for every core source, not merely the edited TU.
core_sources=['/work/'+str(p.relative_to(repo)) for p in sorted((repo/'src').glob('*.cpp')) if p.name!='main.cpp']
core_sources=[('/patch/source/src/consensus_capella_reactor.cpp' if x.endswith('/consensus_capella_reactor.cpp') else x) for x in core_sources]
run('fresh-core-dependencies',['/usr/bin/clang++','-I/patch/source/include',*flags('nano_core'),'-I/work/src','-MM',*core_sources])
text=(logs/'fresh-core-dependencies.log').read_text().replace('\\\n',' ');rules=[line for line in text.splitlines() if re.match(r'^[^\s:]+\.o:\s',line)];assert len(rules)==len(core_sources),(len(rules),len(core_sources));affected=[]
for line in rules:
 deps=shlex.split(line.split(': ',1)[1]);source=next(x for x in deps if x.endswith('.cpp'))
 if any(x.endswith('/nano/consensus_capella_reactor.hpp') for x in deps):affected.append(source)
assert any(x.endswith('/consensus_capella_reactor.cpp') for x in affected)
(logs/'affected-core-sources.json').write_text(json.dumps({'coreSourceCount':len(core_sources),'affected':affected,'derivedFromFreshCompilerDependencies':True},indent=2)+'\n')
objects=[]
for source in affected:
 name=Path(source).name;obj=out+'/'+name+'.o';objects.append(obj);run('compile-core-'+name,['/usr/bin/clang++','-I/patch/source/include',*flags('nano_core'),'-I/work/src','-MD','-MF',out+'/'+name+'.d','-c',source,'-o',obj])
old_members=tool(['/usr/bin/ar','t',out+'/libnano_core.a']).splitlines();assert all(Path(x).name in old_members for x in objects)
run('replace-all-affected-core',['/usr/bin/ar','r',out+'/libnano_core.a',*objects]);assert tool(['/usr/bin/ar','t',out+'/libnano_core.a']).splitlines()==old_members
programs=[('test','nano-capella-transition-tests',fixture,['-I/work/tests']),('nano','nano','/work/src/main.cpp',[]),('capture','nano-capella-paused-capture','/work/tests/capella/paused_capture.cpp',['-I/work/third_party/ckzg/blst/bindings'])]
for name,target,source,extra in programs:
 obj=out+'/'+name+'.o';run('compile-'+name,['/usr/bin/clang++','-I/patch/source/include',*flags(target),*extra,'-c',source,'-o',obj]);link(name,target,obj,out+'/libnano_core.a')
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
