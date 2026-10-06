from pathlib import Path
import json,hashlib,subprocess
base=Path('/private/tmp');root=base/'nano-restored-three-node-v18-namespace-selection-20261006';audit=base/'nano-v18-post-stop-native-diagnostic-20261006';template=base/'nano-catalog-selection-native-20261006';draft=base/'nano-v18-graph-pressure-capture-draft-20261006';out=base/'nano-v18-graph-pressure-native-proof-20261006'
def read(p):return json.loads(p.read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
t=read(root/'campaign/terminal-exit.json');assert t['originalSession']==53478 and t['exitCode'] in [0,1]
assert read(root/'campaign/result.json')['stopReceipts']==3
for n in ['mac-native','mac-docker']:assert read(audit/(n+'-exit.json'))['exitCode']==0 and read(audit/(n+'-immutability.json'))['hashesUnchanged']
assert read(root/'campaign/native-state.json')['exitCode']==0
assert not any('/nano run ' in x for x in subprocess.check_output(['ps','-axo','pid,command'],text=True).splitlines())
r=read(root/'campaign/resources.json');state=json.loads(subprocess.check_output(['docker','inspect',r['localContainer'],'--format','{{json .State}}'],text=True));assert not state['Running'] and state['ExitCode']==0 and not state['OOMKilled']
out.mkdir(exist_ok=False)
def run(name,args,cwd=None):
 (out/(name+'-command.json')).write_text(json.dumps(args,indent=2)+'\n')
 with (out/(name+'.log')).open('w') as f:r=subprocess.run(args,stdout=f,stderr=subprocess.STDOUT,cwd=cwd)
 (out/(name+'-exit.json')).write_text(json.dumps({'exitCode':r.returncode}));assert r.returncode==0,name
c=read(template/'nano-capella-paused-capture-compile-command.json');c.insert(1,'-I/Users/boris/Dev/blockchain-nano/src');c[c.index('-c')+1]=str(draft/'paused_capture.cpp');c[c.index('-o')+1]=str(out/'capture.o');run('compile',c)
l=read(template/'nano-capella-paused-capture-link-command.json');l=[str(out/'capture.o') if x==str(template/'nano-capella-paused-capture.o') else str(out/'capture') if x==str(template/'nano-capella-paused-capture') else x for x in l];run('link',l,'/tmp/nano-pruning-wan-native-build-20261004')
fields=['retainedBlocks','retainedForkNodes','blockCapacity','atBlockCapacity','anchorSlot','finalizedEpoch','clockEpoch'];summary={}
for n in ['mac-native','mac-docker']:
 data=root/n/'data';before=read(audit/(n+'-before.json'))
 def hashes():return {str(f.relative_to(data)):sha(f) for f in sorted(data.rglob('*')) if f.is_file()}
 assert hashes()==before,'Audited source changed'
 args=[str(out/'capture'),'--capture',str(root/'config/genesis.json'),str(audit/(n+'-config.json')),str(data/'chain'),str(audit/(n+'-request.json'))]
 (out/(n+'-command.json')).write_text(json.dumps(args,indent=2)+'\n')
 with (out/(n+'-capture.json')).open('w') as stdout,(out/(n+'-error.log')).open('w') as stderr:r=subprocess.run(args,stdout=stdout,stderr=stderr)
 (out/(n+'-exit.json')).write_text(json.dumps({'exitCode':r.returncode}));assert r.returncode==0
 assert hashes()==before;d=read(out/(n+'-capture.json'));old=read(audit/(n+'-capture.json'));summary[n]={k:d['participationDiagnostics'].pop(k) for k in fields};assert d==old,'Extended reader changed baseline capture proof'
 print(n,json.dumps(summary[n]),flush=True)
(out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n');(out/'scope.json').write_text(json.dumps({'readerSHA256':sha(out/'capture'),'sourceSHA256':sha(draft/'paused_capture.cpp'),'originalCoreUnchanged':True,'readonly':True,'allOtherCaptureFieldsIdenticalToQualifiedReader':True,'dataHashesUnchanged':True,'productionQualified':False},indent=2)+'\n')
