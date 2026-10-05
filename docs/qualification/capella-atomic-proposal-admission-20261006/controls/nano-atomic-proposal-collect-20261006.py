from pathlib import Path
import subprocess,json,tarfile,io,shlex,sys
arch=sys.argv[1];out=Path('/private/tmp/nano-atomic-proposal-evidence-20261006');out.mkdir(exist_ok=True)
ssh=['/tmp/nano-wan-dual-family-transport-control-20261004/bin/ssh','ubuntu@145.239.199.166']
vol='nano-clean-arm64-build-20261002' if arch=='arm64' else 'nano-clean-rpc-build-20261002'
image='sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb' if arch=='arm64' else 'sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14'
folder=f'atomic-proposal-{arch}-d71f574a-20261006';prefix=[] if arch=='arm64' else ssh
name=f'nano-atomic-proposal-{arch}-d71f574a-20261006'
cmd=['docker','inspect','--format','{{json .State}}',name]
state=json.loads(subprocess.check_output(prefix+['sudo '+shlex.join(cmd)] if prefix else cmd));assert state['Status']=='exited' and state['ExitCode']==0 and not state['OOMKilled'],state
(out/(name+'-state.json')).write_text(json.dumps(state,indent=2)+'\n')
command='cd /build && find '+folder+' atomic-runtime-'+arch+'-d71f574a-20261006'+" -type f \\( -name '*.txt' -o -name '*.log' -o -name '*.sha256' -o -name '*.json' \\) -print0 | tar --null -T - -czf -"
cmd=['docker','run','--rm','--entrypoint','/bin/bash','-v',vol+':/build:ro',image,'-c',command]
r=subprocess.run(prefix+['sudo '+shlex.join(cmd)] if prefix else cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE);assert r.returncode==0,r.stderr.decode()
with tarfile.open(fileobj=io.BytesIO(r.stdout),mode='r:gz') as tf:
 for m in tf.getmembers():
  assert m.isfile() and '..' not in Path(m.name).parts and not m.name.startswith('/')
  dest=out/arch/m.name;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(tf.extractfile(m).read())
for n,e in [('controller',0),('proposal-gap',1),('atomic-proposal',0),('capacity-pressure',0),('scheduler',0),('scheduler-registration',0)]:assert (out/arch/folder/(n+'-exit.txt')).read_text().strip()==str(e),(arch,n)
print('Collected',arch,'original terminal receipts and public build evidence')

name=f'nano-atomic-runtime-{arch}-d71f574a-20261006'
cmd=['docker','inspect','--format','{{json .State}}',name]
state=json.loads(subprocess.check_output(prefix+['sudo '+shlex.join(cmd)] if prefix else cmd));assert state['Status']=='exited' and state['ExitCode']==0 and not state['OOMKilled'],state
(out/(name+'-state.json')).write_text(json.dumps(state,indent=2)+'\n')
for label in ['controller','nano-link','capture-link','nano-help']:
 assert (out/arch/f'atomic-runtime-{arch}-d71f574a-20261006'/(label+'-exit.txt')).read_text().strip()=='0'
