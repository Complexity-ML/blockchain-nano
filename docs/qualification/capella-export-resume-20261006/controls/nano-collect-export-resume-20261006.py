from pathlib import Path
import subprocess,json,tarfile,io,hashlib
out=Path('/private/tmp/nano-export-resume-evidence-20261006');out.mkdir(exist_ok=True)
ssh=['/tmp/nano-wan-dual-family-transport-control-20261004/bin/ssh','ubuntu@145.239.199.166']
sets={
'arm64':('nano-clean-arm64-build-20261002','sha256:e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb',['export-resume-arm64-9ebc4cec-20261006','export-resume-fixed-current-arm64-9ebc4cec-20261006','export-resume-regressions-arm64-9ebc4cec-20261006','export-resume-p2p-arm64-9ebc4cec-20261006']),
'amd64':('nano-clean-rpc-build-20261002','sha256:fd9b0bb98a63963dba141a820ab7a49c08e23afcfb7b5ea9ea939987d75d6f14',['export-resume-amd64-9ebc4cec-20261006','export-resume-regressions-amd64-9ebc4cec-20261006','export-resume-p2p-amd64-9ebc4cec-20261006'])}
containers=['nano-export-resume-arm64-9ebc4cec-20261006','nano-export-resume-fixed-current-arm64-9ebc4cec-20261006','nano-export-resume-regressions-arm64-9ebc4cec-20261006','nano-export-resume-p2p-arm64-9ebc4cec-20261006','nano-export-resume-amd64-9ebc4cec-20261006','nano-export-resume-p2p-amd64-9ebc4cec-20261006']
for arch,(vol,image,dirs) in sets.items():
 prefix=[] if arch=='arm64' else ssh
 command='cd /build && find '+' '.join(dirs)+" -type f \\( -name '*.txt' -o -name '*.log' -o -name '*.sha256' -o -name '*.json' \\) -print0 | tar --null -T - -czf -"
 cmd=['docker','run','--rm','--entrypoint','/bin/bash','-v',vol+':/build:ro',image,'-c',command]
 if prefix:
  import shlex
  cmd=prefix+['sudo '+shlex.join(cmd)]
 r=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE);assert r.returncode==0,r.stderr.decode()
 with tarfile.open(fileobj=io.BytesIO(r.stdout),mode='r:gz') as tf:
  for m in tf.getmembers():
   assert m.isfile() and '..' not in Path(m.name).parts and not m.name.startswith('/')
   p=out/arch/m.name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(tf.extractfile(m).read())
 for name in [n for n in containers if ('amd64' in n)==(arch=='amd64')]:
  cmd=['docker','inspect','--format','{{json .State}}',name]
  if prefix:
   import shlex
   cmd=prefix+['sudo '+shlex.join(cmd)]
  state=json.loads(subprocess.check_output(cmd));assert state['Status']=='exited' and state['ExitCode']==0 and not state['OOMKilled'],state
  (out/(name+'-state.json')).write_text(json.dumps(state,indent=2)+'\n')
for p in Path('/private/tmp/nano-export-resume-p2p-native-20261006/candidate').iterdir():
 if p.suffix in ['.log','.txt','.json']:
  dest=out/'native'/p.name;dest.parent.mkdir(exist_ok=True);dest.write_bytes(p.read_bytes())
assert json.loads((out/'native/program.json').read_text())['networkQualified']==False
for p in out.rglob('controller-exit.txt'):assert p.read_text().strip()=='0',p
for p in out.rglob('baseline-run-exit.txt'):assert p.read_text().strip()=='1',p
for p in out.rglob('fixed-run-exit.txt'):assert p.read_text().strip()=='0',p
print('Collected',len(list(out.rglob('*'))),'paths; terminal receipts verified')
