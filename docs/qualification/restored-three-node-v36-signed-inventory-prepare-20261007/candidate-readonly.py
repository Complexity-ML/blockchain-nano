from pathlib import Path
import subprocess,json,hashlib
r=Path(__file__).parent;out=r/'candidate-readonly';out.mkdir(mode=0o700);source=Path('/private/tmp/nano-restored-three-node-v35-trace-bound-20261007');root=Path('/private/tmp/nano-restored-three-node-v36-signed-inventory-20261007');old=Path('/private/tmp/nano-v35-post-stop-readonly-20261007')
assert not any('/nano run ' in x for x in subprocess.check_output(['ps','-axo','pid,command'],text=True).splitlines())
def hashes(p):return {str(x.relative_to(p)):hashlib.sha256(x.read_bytes()).hexdigest() for x in sorted(p.rglob('*')) if x.is_file()}
def remap(x):
 if isinstance(x,str):return x.replace(str(source/'mac-native/data'),str(root/'mac-native/data')).replace(str(source/'mac-docker/data'),str(root/'mac-docker/data'))
 if isinstance(x,list):return [remap(v) for v in x]
 if isinstance(x,dict):return {k:remap(v) for k,v in x.items()}
 return x
for node,label,arch in [('mac-native','native','native'),('mac-docker','mac-docker','arm64')]:
 p=out/label;p.mkdir(mode=0o700);data=root/node/'data';before=hashes(data);assert before==json.loads((root/node/'data-copy-hashes.json').read_text());capsule=json.loads((old/label/'capture.json').read_text());assert json.loads((old/label/'exit.json').read_text())['exitCode']==0
 for f in ['config.json','request.json']:(p/f).write_text(json.dumps(remap(json.loads((old/label/f).read_text())),indent=2)+'\n');(p/f).chmod(0o600)
 args=['--capture',str(source/'config/genesis.json'),str(p/'config.json'),str(data/'chain'),str(p/'request.json')]
 if arch=='native':cmd=['/private/tmp/nano-published-signed-inventory-draft-20261007/native-build/programs/capture',*args]
 else:
  cmd=['/Users/boris/.docker/bin/docker','run','--rm','--network','none','--read-only','--tmpfs','/tmp','--ulimit','nofile=65536:65536','-v','nano-clean-arm64-build-20261002:/build:ro']
  for path in [root,source,old,r]:cmd+=['-v',str(path)+':'+str(path)+':ro']
  cmd+=['--entrypoint','/build/published-signed-inventory-arm64-20261007/capture','e30ce85e363309a64814c300fbf07b482e28b841c8f9577cba9b4e70070d37fb',*args]
 (p/'command.json').write_text(json.dumps(cmd)+'\n')
 with (p/'capture.json').open('w') as f,(p/'error.log').open('w') as e:x=subprocess.run(cmd,stdout=f,stderr=e)
 (p/'exit.json').write_text(json.dumps({'exitCode':x.returncode})+'\n');assert x.returncode==0
 current=json.loads((p/'capture.json').read_text())
 for field in ['restart','validatorKeys','canonicalReplay']:assert current[field]==capsule[field],field
 assert hashes(data)==before
 (p/'binding.json').write_text(json.dumps({'files':len(before),'hashesUnchanged':True,'sameRestartAndValidatorPins':True,'sameCanonicalReplay':True,'newCandidateCapture':True,'signersStarted':False,'networkQualified':False},indent=2)+'\n');print(label+' candidate read-only0, pins/replay exact, files unchanged',flush=True)
