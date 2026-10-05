from pathlib import Path
import subprocess,json,tarfile,io,shlex
out=Path('/private/tmp/nano-capacity-evidence-20261006');out.mkdir(exist_ok=True)
ssh=['/tmp/nano-wan-dual-family-transport-control-20261004/bin/ssh','ubuntu@145.239.199.166']
import sys
arch=sys.argv[1]
vol='nano-clean-arm64-build-20261002' if arch=='arm64' else 'nano-clean-rpc-build-20261002'
image='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb' if arch=='arm64' else 'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
folder=f'capacity-pressure-{arch}-5902c983-20261006'
prefix=[] if arch=='arm64' else ssh
command='cd /build && find '+folder+" -type f \\( -name '*.txt' -o -name '*.log' -o -name '*.sha256' -o -name '*.json' \\) -print0 | tar --null -T - -czf -"
cmd=['docker','run','--rm','--entrypoint','/bin/bash','-v',vol+':/build:ro',image,'-c',command]
r=subprocess.run(prefix+['sudo '+shlex.join(cmd)] if prefix else cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE);assert r.returncode==0,r.stderr.decode()
with tarfile.open(fileobj=io.BytesIO(r.stdout),mode='r:gz') as tf:
 for m in tf.getmembers():
  assert m.isfile() and '..' not in Path(m.name).parts and not m.name.startswith('/')
  dest=out/arch/m.name;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(tf.extractfile(m).read())
cmd=['docker','run','--rm','--entrypoint','/bin/bash','-v',vol+':/build:ro',image,'-c','sha256sum /build/'+folder+'/libnano_core.a /build/'+folder+'/baseline /build/'+folder+'/fixed']
r=subprocess.check_output(prefix+['sudo '+shlex.join(cmd)] if prefix else cmd);(out/arch/folder/'programs.sha256').write_bytes(r)
containers=[f'nano-capacity-pressure-{arch}-5902c983-20261006']
if arch=='arm64':containers+=['nano-capacity-regressions-arm64-5902c983-20261006','nano-capacity-regressions-fixed-arm64-5902c983-20261006']
for name in containers:
 cmd=['docker','inspect','--format','{{json .State}}',name]
 state=json.loads(subprocess.check_output(prefix+['sudo '+shlex.join(cmd)] if prefix else cmd));assert state['Status']=='exited' and not state['OOMKilled'],state
 expected=1 if name=='nano-capacity-regressions-arm64-5902c983-20261006' else 0
 assert state['ExitCode']==expected,state
 (out/(name+'-state.json')).write_text(json.dumps(state,indent=2)+'\n')
for name,expected in [('controller-exit.txt',0),('baseline-run-exit.txt',1),('fixed-run-exit.txt',0),('configured-runtime-exit.txt',0),('scheduler-exit.txt',0)]:assert (out/arch/folder/name).read_text().strip()==str(expected),(arch,name)
print('Collected',arch,'terminal receipts, logs and candidate hashes')
