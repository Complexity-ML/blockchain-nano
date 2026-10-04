from pathlib import Path
import shlex,subprocess,json
root=Path(__file__).resolve().parent;groups=[]
for line in (root/'flags.make').read_text().splitlines():
 if line.startswith('CXX_DEFINES = '):groups.append({})
 for name in ['DEFINES','INCLUDES','FLAGS']:
  prefix='CXX_'+name+' = '
  if line.startswith(prefix):groups[-1][name]=shlex.split(line[len(prefix):])
files=['src/storage.cpp','src/chain_beacon.cpp','src/consensus_capella_audit.cpp','src/consensus_capella_reactor.cpp','src/consensus_capella_runtime.cpp','tests/test_capella_transition.cpp','tests/test_execution_checkpoint.cpp']
commands=[]
for i,f in enumerate(files):
 flags=groups[1 if i>=5 else 0];commands.append(['clang++',*flags['DEFINES'],*flags['INCLUDES'],*flags['FLAGS'],'-fsyntax-only','/work/'+f])
script='set -e\n'+'\n'.join(shlex.join(c) for c in commands)+'\n'
args=['docker','run','--rm','-i','--name','nano-retention-syntax-20261004','--network','none','--read-only','--cap-drop','ALL','--security-opt','no-new-privileges','--cpus','2','--memory','3g','--mount','type=bind,src=/tmp/nano-finalized-state-retention-20261004,dst=/work,readonly','--mount','type=volume,src=nano-clean-arm64-build-20261002,dst=/build,readonly','--workdir','/work','sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb','bash','-s']
(root/'syntax-commands.json').write_text(json.dumps(commands,indent=2)+'\n')
with (root/'syntax.log').open('w') as log:r=subprocess.run(args,input=script,text=True,stdout=log,stderr=subprocess.STDOUT,timeout=180)
(root/'syntax-exit.txt').write_text(str(r.returncode)+'\n');print('Docker syntax exit:',r.returncode);raise SystemExit(r.returncode)
