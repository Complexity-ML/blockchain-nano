from pathlib import Path
import subprocess,json,sys,hashlib
r=Path(__file__).parent;arch=sys.argv[1];assert arch in ('arm64','amd64');probe=r/(arch+'-probe');probe.mkdir()
if arch=='arm64':docker=['/Users/boris/.docker/bin/docker'];repo=Path('/Users/boris/Dev/blockchain-nano');image='e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb';volume='nano-clean-arm64-build-20261002';container='nano-restored-three-node-mac-muxcwsxz'
else:docker=['sudo','-n','docker'];repo=Path('/home/ubuntu/blockchain-nano-git');image='fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14';volume='nano-clean-rpc-build-20261002';container='nano-restored-three-node-vps-muxcwsxz'
s=json.loads(subprocess.check_output(docker+['inspect',container,'--format','{{json .State}}'],text=True));assert not s['Running'] and s['ExitCode']==0 and not s['OOMKilled']
m=json.loads((r/'manifest.json').read_text())
for rel,h in m['files'].items():assert hashlib.sha256((repo/rel).read_bytes()).hexdigest()==h['baseSHA256'];assert hashlib.sha256((r/'source'/rel).read_bytes()).hexdigest()==h['draftSHA256']
out='/build/core-trace-bound-'+arch+'-20261007';base=docker+['run','--rm','--cpus','1','--network','none','--read-only','--tmpfs','/tmp','-v',volume+':/build','-v',str(repo)+':/work:ro','-v',str(r)+':/patch:ro']
def run(name,args,expected=0):
 (probe/(name+'-command.json')).write_text(json.dumps({'args':args,'expectedExit':expected})+'\n')
 with (probe/(name+'.log')).open('w') as f:x=subprocess.run(base+['--entrypoint',args[0],image]+args[1:],stdout=f,stderr=subprocess.STDOUT)
 (probe/(name+'-exit.json')).write_text(json.dumps({'exitCode':x.returncode})+'\n');assert x.returncode==expected,(name,x.returncode);print(name+' exit'+str(x.returncode),flush=True)
run('stage',['mkdir','-p',out])
run('compile-parent-counterexample',['/usr/bin/clang++','-std=c++20','-O1','/patch/parent-counterexample.cpp','-o',out+'/parent-counterexample'])
run('parent-counterexample',[out+'/parent-counterexample'],1)
run('compile-diagnostic-fixture',['/usr/bin/clang++','-std=c++20','-O1','-pthread','-I/work/third_party','/patch/source/network/test_core_rpc_diagnostic.cpp','-o',out+'/diagnostic-test'])
run('diagnostic-fixture',[out+'/diagnostic-test'])
print('Scoped diagnostics passed; main/core HTTP integration and network qualification still pending',flush=True)
